#include "log.h"

MyMutex Log::m_mutex;

Log* Log::instance = NULL;

Log* Log::getinstanc() {
    // 锁如果加在这，单线程使用时就会加很多无用的锁
    if (instance == NULL) {
        m_mutex.lock();
        if (instance == NULL) { //可能有多个线程刚好通过了第一个if 所以要在判断一次
            instance = new Log;
        }
        m_mutex.unlock();
    }

    return instance;
}

Log::Log() {
    m_fp = nullptr;
    m_filename = nullptr;
    m_MaxLogSize = 0;
}

Log::~Log() {
    Close();
    if (instance) {
        delete instance;
    }
}

void Log::Close() {
    if (m_fp) {
        fclose(m_fp);
    }
}

/***************************************************
 * 初始化日志文件名，日志文件大小
 * 输入：
 *      filename - 日志文件名，绝对路径
 *      MaxLongSize - 日志文件大小
 * 返回值：
 *      true - 成功打开日志文件
 *      false - 打开文件失败
 ***************************************************/
bool Log::init(const char* filename, const long MaxLongSize) {
    m_filename = filename;
    m_MaxLogSize = MaxLongSize;

    if ((m_fp = Fopen(m_filename, "a+")) == NULL) {
        printf ("open log faile\n");
        return false;
    }

    return true;
}

/***************************************************
 * 写入日志文件
 * 参数：
 *      fmt，同printf参数fmt
 * 返回值：
 *      无
 **************************************************/
void Log::Write(const char* fmt, ...) {
    assert(m_fp);

    w_mutex.lock();

    va_list ap;
    va_start(ap, fmt);
    vfprintf(m_fp, fmt, ap);
    va_end(ap);

    fflush(m_fp);

    w_mutex.unlock();
}

void Log::Addtime() {
    char strtime[20] = {0};
    Localtime(strtime);
    Write("%s ", strtime);
}

/**********************************************************
 * 打开文件，自动创建父目录
 * 输入：
 *      filename - 文件名，绝对路径
 *      mode - 同fopen参数mode
 * 返回值：
 *      同fopen返回值，成功返回FILE指针
 *********************************************************/
FILE* Log::Fopen(const char* filename, const char* mode) {
    if (Mkdir(filename) == false) {
        return NULL;
    }

    return fopen(filename, mode);
}

/**********************************************************
 * 按目录层级创建目录
 * 输入：
 *      filename - 目录名，绝对路径
 * 返回值：
 *      true - 成功创建文件夹
 *      false - 创建文件夹失败
 *********************************************************/
bool Log::Mkdir(const char* filename) {
    char tmpfilename[301] = {0};

    int len = strlen(filename);

    for (int ii = 1; ii < len; ii++) {
        if (filename[ii] != '/') {
            continue;
        }

        memset(tmpfilename, 0, sizeof(tmpfilename));
        strncpy(tmpfilename, filename, ii);

        if (access(tmpfilename, F_OK) == 0) {
            continue;
        }

        if ( mkdir(tmpfilename, 0755) != 0) {
            return false;
        }
    }

    return true;
}

/**************************************************************
 * 获取当前时间，格式yyyy-mm-dd hh-mm-ss
 * 输入：
 *      strtime - 传出参数，保存时间字符串
 * 返回值：
 *      无
 *************************************************************/
void Log::Localtime(char* strtime) {
    memset(strtime, 0, 20);

    time_t t_now;
    t_now = time(NULL);

    struct tm sttm = *localtime(&t_now);
    sttm.tm_year += 1900;
    sttm.tm_mon++;

    snprintf (strtime, 20, "%04u-%02u-%02u %02u:%02u:%02u", sttm.tm_year,\
        sttm.tm_mon, sttm.tm_mday, sttm.tm_hour, sttm.tm_min, sttm.tm_sec
    );
}