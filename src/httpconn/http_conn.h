#ifndef HTTP_CONN_H
#define HTTP_CONN_H
#include <sys/socket.h>
#include <netinet/in.h>
#include<arpa/inet.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <errno.h>
#include <stdarg.h>
#include <sys/mman.h>
#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>
#include <fcntl.h>
#include "../locker/locker.h"
#include <sys/epoll.h>
#include<sys/uio.h>
class http_conn
{
public:
    // 最大文件长度
    static const int FILENAME_LEN = 1024;
    // 读缓冲区大小
    static const int READ_BUFFER_SIZE = 2048;
    // 写缓冲区大小
    static const int WRITE_BUFFER_SIZE = 1024;
    // 所有socket上的事件都被注册到同一个epoll内核事件表中
    static int m_epollfd;
    // 用户数量
    static int m_user_count;

    // HTTP请求方法
    enum METHOD
    {
        GET = 0,
        POST,
        HEAD,
        PUT,
        DELETE,
        TRACE,
        OPTIONS,
        CONNECT,
        PATCH
    };

    // 解析客户请求时，主状态机的状态
    // 当前正在分析请求行\当前正在分析头部字段\当前正在分析请求体
    enum CHECK_STATE
    {
        CHECK_STATE_REQUESTLINE = 0,
        CHECK_STATE_HEADER,
        CHECK_STATE_CONTENT
    };

    // 行的读取状态
    enum LINE_STATUS
    {
        LINE_OK = 0, // 读到了一个完整的行
        LINE_BAD,    // 行出错
        LINE_OPEN    // 行数据不完整
    };

    // 服务器处理HTTP请求的可能结果
    enum HTTP_CODE
    {
        NO_REQUEST,  // 请求不完整,需要继续读取客户数据
        GET_REQUEST, // 获得了一个完整的客户请求
        BAD_REQUEST, // 客户请求有语法错误
        NO_RESOURCE,
        FORBIDEN_REQUEST, // 客户对资源没有访问权限
        FILE_REQUEST,     // 请求文件
        INTERNAL_ERROR,   // 非法错误
        CLOSED_CONNECTION // 客户端已经关闭连接
    };

    // 暴露的函数接口

    http_conn(/* args */);
    ~http_conn();
    // 初始化新接受的连接
    void init(int sockfd, const sockaddr_in &addr, char *root_);
    // 关闭连接
    void close_conn(bool real_close = true);
    // 处理客户请求
    void process();
    // 非阻塞读
    bool read();
    // 非阻塞写
    bool write();

private:
    // 初始化连接
    void init();
    // 解析http请求
    HTTP_CODE process_read();
    // 填充http应答
    bool process_write(HTTP_CODE ret);

    // process_read调用，来解析HTTP请求的过程函数
    HTTP_CODE parse_request_line(char *text);
    HTTP_CODE parse_headers(char *text);
    HTTP_CODE parse_content(char *text);
    HTTP_CODE do_request();
    inline char *get_line() { return m_read_buf + m_start_line; }
    LINE_STATUS parse_line();

    // process_write调用，以填充HTTP应答的过程函数
    void unmap();
    bool add_respones(const char *format, ...);
    bool add_content(const char *content);
    bool add_status_line(int status, const char *title);
    bool add_headers(int content_length);
    bool add_content_length(int content_length);
    bool add_linger();
    bool add_blank_line();

    // 根目录
    char *m_root;

    // 读HTTP连接的socket和对方的socket地址
    int m_sockfd;
    sockaddr_in m_address;

    // 读缓冲区
    char m_read_buf[READ_BUFFER_SIZE];
    // 标识读缓冲中已经读入的客户数据的最后一个字节的下一个位置
    int m_read_idx;
    // 当前正在分析的字符在读缓冲区中的位置
    int m_checked_idx;
    // 当前正在解析的行的起始位置
    int m_start_line;

    // 写缓冲区
    char m_write_buf[WRITE_BUFFER_SIZE];
    // 写缓冲区中待发送的字节数
    int m_write_idx;

    // 主状态机当前所处的状态
    CHECK_STATE m_check_state;
    // 请求方法
    METHOD m_method;

    // 客户请求的目标文件的完整路径
    char m_real_file[FILENAME_LEN];
    // 客户请求的目标文件的文件名
    char *m_url;
    // HTTP版本协议号，这里只支持1.1
    char *m_version;
    // hostname
    char *m_host;
    // HTTP请求的消息体的长度
    int m_content_length;
    // HTTP请求是否保持长连接
    bool m_linger;

    // 客户请求的目标文件被mmap到内存中的起始地址
    char *m_file_address;
    // 目标文件的状态
    struct stat m_file_stat;
    // 我们使用writev来执行写操作，所以定义如下成员
    struct iovec m_iv[2];
    // 被写内存块的数量
    int m_iv_count;

    int bytes_to_send;
    int bytes_have_send;
};
#endif