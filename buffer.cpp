#include "buffer.h"

Buffer::Buffer(size_t size_) {
    assert(size_ > 0);

    m_buffer = new char[size_]();
    assert(m_buffer);
    
    memset(m_buffer, '\0', size_);

    size = size_;
    writepos = 0;
    readpos = 0;
    parselength = 0;
}

Buffer::~Buffer() {
    if (m_buffer) {
        delete [] m_buffer;
    }
}

ssize_t Buffer::Read(int fd, int* Errno) {
    char buff[65535];   //临时缓冲区

    size_t writeable = size - writepos;

    struct iovec iov[2];
    iov[0].iov_base = m_buffer;
    iov[0].iov_len = writeable;   //m_buff剩余可写空间
    iov[1].iov_base = buff;
    iov[1].iov_len = sizeof(buff);

    ssize_t len = readv(fd, iov, 2);
    if (len < 0) {
        *Errno = errno;
    }else if ((size_t)len <= writeable) {
        writepos += len;
    }else {
        writepos = size;
        Append(buff, len - writeable);    //扩容存储
    }

    return len;
}

void Buffer::Append(std::string str) {
    size_t len = str.size();
    Append(str.data(), len);
}

void Buffer::Append(const char* buf, size_t len) {
    size_t writeable = size - writepos; //剩余可用空间
    size_t invalid = readpos;   //失效数据长度

    if (invalid + writeable < len) {            //扩容新空间
        char* new_buf = new char[size + len + 1];
        memset(new_buf, '\0', size + len + 1);

        strncpy(new_buf, m_buffer, size);       //原数据移动到新空间
        strncpy(new_buf+size+1, buf, len);      //buf数据移动到新空间
        delete [] m_buffer;                     //释放旧空间
        
        m_buffer = new_buf;
        size = writepos + len + 1;
    }else {
        size_t readable = writepos - readpos;
        strncpy(m_buffer, m_buffer+readpos, readable);      //未处理数据前移
        memset(m_buffer+readable, '\0', size-readable);     //未使用位置清0
        readpos = 0;
        writepos = readable;
        strncpy(m_buffer+writepos, buf, len);
        writepos += len;
    }
}

Buffer::Line_Status Buffer::Parse_Line() {
    //printf ("Parse line\n");
    size_t pos = readpos;
    char cc;
    while (pos < writepos) {
        cc = m_buffer[pos];
        if (cc == '\r') {
            if (pos == writepos-1) {    //这一行只读到了'\r'
                return Incomplete_Line;
            }else if (m_buffer[pos+1] == '\n') {
                m_buffer[pos] = '\0';
                m_buffer[pos+1] = '\0';
                set_parselength(pos+2-readpos);
                return Get_Line;
            }
            return Bad_Line;
        }else if (cc == '\n') {
            if (pos >= 1 && m_buffer[pos-1] == '\r') {
                m_buffer[pos-1] = '\0';
                m_buffer[pos] = '\0';
                set_parselength(pos+1-readpos);
                return Get_Line;
            }
            return Bad_Line;
        }
        pos++;
    }
    return Incomplete_Line;
}

char* Buffer::get_line() {
    return &m_buffer[readpos];
}

void Buffer::set_parselength(size_t len) {
    parselength = len;
}

size_t Buffer::Readable_length() {
    return writepos - readpos;
}

void Buffer::Add_readpos() {
    readpos += parselength;
}

void Buffer::Clear() {
    if (m_buffer) {
        memset(m_buffer, '\0', size);
        readpos = 0;
        writepos = 0;
        parselength = 0;
    }
}