#include "./httpconn.h"

int HttpConn::conn_num = 0;
const char* HttpConn::src = NULL;

const std::unordered_map<int, std::string> HttpConn::CODE_STATUS = {
    {200, "OK"},
    {400, "Bad Request"},
    {403, "Forbidden"},
    {404, "Not Found"},
    {500, "Internal Server Error"}
};

const std::unordered_map<int, std::string> HttpConn::ERROR_MES = {
    {400, "Your request has bad syntax or is inherently impossible to satisfy.\n"},
    {403, "You do not have permission to get file from this server.\n"},
    {404, "The requested file was not found on this server.\n"},
    {500, "There was an unusual problem serving the requested file.\n"}
};

HttpConn::HttpConn() {
    fd = -1;
    caddr = {0};
}

HttpConn::~HttpConn() {

}

void HttpConn::init(int fd_, struct sockaddr_in& addr_) {
    //printf ("connect init fd = %d\n", fd_);
    fd = fd_;
    caddr = addr_;
    conn_num++;
    init();
}

void HttpConn::init() {
    state_machine = Request_Line;
    method = "";
    path = "";
    version = "";
    body = "";
    head.clear();
    content_length = 0;
    resource_stat = {0};
    resource_addr = NULL;
    iv_count = 0;
    readbuf.Clear();
    writebuf.Clear();
}

void HttpConn::Close() {
    if (fd != -1) {
        close(fd);
        Debug("client fd(%d) quit!!!\n", fd);
        fd = -1;
        conn_num--;
        unmap();
    }
}

bool HttpConn::Onread() {
    //printf ("Onread\n");
    int Errno;
    
    while (true) {
        ssize_t read_len = readbuf.Read(fd, &Errno);

        if (read_len == 0) { //对端关闭连接
            return false;
        }else if (read_len == -1) {
            if (Errno == EAGAIN || Errno == EWOULDBLOCK) {
                break;
            }
            return false;
        }
    }

    return true;
}

ssize_t HttpConn::Onwrite(int* Errno) {
    ssize_t write_len;

    while (true) {
        write_len = writev(fd, iov, iv_count);
        if (write_len <= 0) {
            *Errno = errno;
            break;
        }

        if (iov[0].iov_len == 0 && iov[1].iov_len == 0) {   //内容写完后，再写一次空内容，看对端是否关闭
            //没关闭
            break;
        }

        if (static_cast<size_t>(write_len) > iov[0].iov_len) {
            iov[1].iov_base = (uint8_t *)iov[1].iov_base + (write_len - iov[0].iov_len);
            iov[1].iov_len = iov[1].iov_len - (write_len - iov[0].iov_len);
            if (iov[0].iov_len) {
                iov[0].iov_len = 0;
                writebuf.Clear();
            }
        }else {
            iov[0].iov_base = (uint8_t *)iov[0].iov_base + write_len;
            iov[0].iov_len -= write_len;
            writebuf.set_parselength(write_len);
            writebuf.Add_readpos();
        }
    }

    return write_len;
}

HttpConn::Parse_Status HttpConn::ProcessRequest() {
    //printf("ProcessRequest\n");
    Buffer::Line_Status LineState = Buffer::Get_Line;
    Parse_Status httpret = Incomplete_req;

    char* text = NULL;
    readbuf.set_parselength();

    while ((state_machine == Request_Content && LineState == Buffer::Get_Line) || 
            (LineState = readbuf.Parse_Line()) == Buffer::Get_Line) {
        
        text = readbuf.get_line();  
        //printf ("text = %s\n", text);
        Debug("Parse one line: %s\n", text);

        switch (state_machine) {
            case Request_Line: {
                httpret = parse_request_line(text);
                if (httpret == Bad_req) {
                    return httpret;
                }
                break;
            }
            case Request_head: {
                httpret = parse_request_head(text);
                if (httpret == Bad_req) {
                    return Bad_req;
                }else if (httpret == Get_req) {
                    return repare_resources(); //准备.html资源、解析表单数据
                }
                break;
            }
            case Request_Content: {
                httpret = parse_request_content(text);
                if (httpret == Get_req) {
                    return repare_resources();
                }
                LineState = Buffer::Incomplete_Line;
                break;
            }
            default: {
                return Inner_error;
            }
        }

        readbuf.Add_readpos();
        readbuf.set_parselength();
    }

    if (LineState == Buffer::Bad_Line) {
        return  Bad_req;
    }

    return Incomplete_req;
}

HttpConn::Parse_Status HttpConn::parse_request_line(char* text) {
    //方法 url 版本
    char* url = strpbrk(text, " \t");
    if (url == NULL) {
        return Bad_req;
    }

    *url++ = '\0';
    method = std::string(text);
    
    char* ver = strpbrk(url, " \t");
    if (ver == NULL) {
        return Bad_req;
    }

    *ver++ = '\0';
    version = std::string(ver);

    if (strncasecmp(url, "http://", 7) == 0) {
        url += 7;
        url = strchr(url, '/');
    }

    if (url == NULL || url[0] != '/') {
        return Bad_req;
    }

    if (strcmp(url, "/") == 0) {
        path = "/index.html";
    }else { //扩展功能：提供更多.html
        path = std::string(url);
    }

    state_machine = Request_head;

    return Incomplete_req;
}

HttpConn::Parse_Status HttpConn::parse_request_head(char* text) {
    if (text[0] == '\0') {  //请求空行
        if (get_content_length() == 0) {
            return Get_req;
        }
        state_machine = Request_Content;
        return Incomplete_req;
    }else {
        char* value = strchr(text, ':');
        *value++ = '\0';
        value += strspn(value, " \t");  //跳过所有空格、tab

        std::string k(text);
        std::string v(value);        

        tolower(k);
        tolower(v);
        head[k] = v;
    }

    return Incomplete_req;
}

HttpConn::Parse_Status HttpConn::parse_request_content(char* text) {
    if (readbuf.Readable_length() >= get_content_length()) {
        text[content_length] = '\0';
        Info("body = %s\n", text);
        body = std::string(text);
        readbuf.set_parselength(content_length);
        readbuf.Add_readpos();
        return Get_req;
    }

    return Incomplete_req;
}

HttpConn::Parse_Status HttpConn::repare_resources() {
    //是否存在、有无权限、普通文件/目录
    std::string real_file(src);
    real_file += path;

    if (stat(real_file.c_str(), &resource_stat) < 0) {
        return File_non;
    }

    if (!(resource_stat.st_mode&S_IROTH)) { //其他用户没有读权限
        return Forbidden_req;
    }

    if (S_ISDIR(resource_stat.st_mode)) {   //目录不能访问
        return Bad_req;
    }

    int srcfd = open(real_file.c_str(), O_RDONLY);
    resource_addr = (char*)mmap(0, resource_stat.st_size, PROT_READ, MAP_PRIVATE, srcfd, 0);
    close(srcfd);

    return File_req;
}

bool HttpConn::ProcessResponse(HttpConn::Parse_Status request_code) {
    switch (request_code) {
        case File_req: {        //200
            add_response_line(200);
            add_response_head(keepalive(), resource_stat.st_size);
            //add_response_content();
            iov[0].iov_base = writebuf.get_line();
            iov[0].iov_len = writebuf.Readable_length();
            iov[1].iov_base = resource_addr;
            iov[1].iov_len = resource_stat.st_size;
            iv_count = 2;
            return true;
        }
        case Bad_req: {         //400
            add_response_line(400);
            add_response_head(false, ERROR_MES.find(400)->second.size());
            add_response_content(400);
            break;
        }
        case Forbidden_req: {   //403
            add_response_line(403);
            add_response_head(false, ERROR_MES.find(403)->second.size());
            add_response_content(403);
            break;
        }
        case File_non: {        //404
            add_response_line(404);
            add_response_head(false, ERROR_MES.find(404)->second.size());
            add_response_content(404);
            break;
        }
        case Inner_error: {     //500
            add_response_line(500);
            add_response_head(false, ERROR_MES.find(500)->second.size());
            add_response_content(500);
            break;
        }
        default: {
            return false;
        }
    }

    iov[0].iov_base = writebuf.get_line();
    iov[0].iov_len = writebuf.Readable_length();
    iv_count = 1;
    
    return true;
}

void HttpConn::add_response_line(int code) {
    std::string str("HTTP/1.1 ");
    writebuf.Append(str + std::to_string(code) + " " + CODE_STATUS.find(code)->second + "\r\n");
}

void HttpConn::add_response_head(bool keepalive, size_t filesize) {
    writebuf.Append("Content-Length: ");
    writebuf.Append(std::to_string(filesize) + "\r\n");

    writebuf.Append("Content-Type: ");
    writebuf.Append("text/html\r\n");

    writebuf.Append("Connection: ");
    if (keepalive) {
        writebuf.Append("Keep-Alive\r\n");
    }else {
        writebuf.Append("Close\r\n");
    }

    writebuf.Append("\r\n");
}

void HttpConn::add_response_content(int code) {
    writebuf.Append(ERROR_MES.find(code)->second);
}

int HttpConn::get_content_length() {
    std::unordered_map<std::string, std::string>::const_iterator got = head.find("content-length");
    if (got != head.end()) {
        content_length = std::stoi(got->second);
    }
    return content_length;
}

bool HttpConn::keepalive() {
    std::unordered_map<std::string, std::string>::const_iterator got = head.find("connection");
    if (got != head.end()) {
        if (got->second == "keep-alive") {
            return true;
        }
    }
    return false;
}

void HttpConn::tolower(std::string& str) {
    for (auto& c : str) {
        if (c >= 'A' && c <= 'Z') {
            c = c-'A'+'a';
        }
    }
}

int HttpConn::Getfd() {
    assert(fd != -1);
    return fd;
}

void HttpConn::unmap() {
    if (resource_addr) {
        munmap(resource_addr, resource_stat.st_size);
        resource_addr = NULL;
    }
}
