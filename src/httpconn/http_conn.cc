#include "http_conn.h"

// 定义HTTP的响应状态信息
const char *ok_200_title = "OK";
const char *error_400_title = "BAD_REQUEST";
const char *error_400_form = "Your request had ban syntax or is inherently impossible to satisfy.\n";
const char *error_403_title = "Forbidden";
const char *error_403_form = "You do not have permission to get file from this server.\n";
const char *error_404_title = "Not Found";
const char *error_404_form = "The requested file was not found on this server.\n";
const char *error_500_title = "Internal error";
const char *error_500_form = "There was an unusual problem serving the requested file.\n";

int setnonblocking(int fd)
{
    int oldopt = fcntl(fd, F_GETFL);
    int newopt = oldopt | O_NONBLOCK;
    fcntl(fd, F_SETFL, newopt);
    return oldopt;
}

void addfd(int epollfd, int fd, bool one_shot)
{
    epoll_event event;
    event.data.fd = fd;
    // EPOLLRDHUP:对端断开连接
    event.events = EPOLLIN | EPOLLET | EPOLLRDHUP;
    // 避免因为本线程处理不及时，主线程寻找其他线程为客户服务
    // 一旦响应，就会从epoll注册内核中删除该fd
    if (one_shot)
    {
        event.events |= EPOLLONESHOT;
    }
    epoll_ctl(epollfd, EPOLL_CTL_ADD, fd, &event);
    setnonblocking(fd);
}

void removefd(int epollfd, int fd)
{
    epoll_ctl(epollfd, EPOLL_CTL_DEL, fd, nullptr);
    close(fd);
}

void modfd(int epollfd, int fd, int newevent)
{
    epoll_event event;
    event.data.fd = fd;
    event.events = newevent | EPOLLET | EPOLLRDHUP | EPOLLONESHOT;
    epoll_ctl(epollfd, EPOLL_CTL_MOD, fd, &event);
}

int http_conn::m_epollfd = -1;
int http_conn::m_user_count = 0;

void http_conn::close_conn(bool real_close)
{
    if (real_close && m_epollfd != -1)
    {
        removefd(m_epollfd, m_sockfd);
        m_sockfd = -1;
        --m_user_count;
    }
}

void http_conn::init(int sockfd, const sockaddr_in &addr, char *root_)
{
    m_sockfd = sockfd;
    m_address = addr;
    m_root = root_;
    addfd(m_epollfd, m_sockfd, true);
    ++m_user_count;
    init();
}

void http_conn::init()
{
    memset(m_read_buf, '\0', READ_BUFFER_SIZE);
    memset(m_write_buf, '\0', WRITE_BUFFER_SIZE);
    memset(m_real_file, '\0', FILENAME_LEN);

    m_check_state = CHECK_STATE_REQUESTLINE;
    m_method = GET;

    m_url = nullptr;
    m_version = nullptr;
    m_host = nullptr;
    m_content_length = 0;

    m_checked_idx = 0;
    m_start_line = 0;
    m_read_idx = 0;

    m_write_idx = 0;

    // 默认关闭长连接
    m_linger = false;
}

// 从状态机，用于分析出一行的内容
http_conn::LINE_STATUS http_conn::parse_line()
{
    char temp;
    // m_checked_idx指向当前buffer中正在分析的字符
    // m_read_idx指向当前buffer中客户数据的尾部的下一个字节
    // HTTP报文中每行以 \r\n 结尾
    while (m_checked_idx < m_read_idx)
    {
        temp = m_read_buf[m_checked_idx];
        if (temp == '\r')
        {
            if (m_checked_idx + 1 == m_read_idx)
            {
                // 本次分析还没有读取完，等待继续读取客户数据
                return LINE_OPEN;
            }
            else if (m_read_buf[m_checked_idx + 1] == '\n')
            {
                // 读取了一个完整的行，将\r\n覆盖
                m_read_buf[m_checked_idx++] = '\0';
                m_read_buf[m_checked_idx++] = '\0';
                return LINE_OK;
            }
            else
            {
                // HTTP请求错误
                return LINE_BAD;
            }
        }
        else if (temp == '\n')
        {
            // 这种情况有两种：
            // 1.上次读了\r返回了 LINE_OPEN,第二次读取，读到了\n
            // 2.语法错误
            if (m_checked_idx > 1 && m_read_buf[m_checked_idx - 1] == '\r')
            {
                m_read_buf[m_checked_idx - 1] = '\0';
                m_read_buf[m_checked_idx++] = '\0';
                return LINE_OK;
            }
            else
            {
                return LINE_BAD;
            }
        }
        m_checked_idx++;
    }
    // 还没读取完
    return LINE_OPEN;
}

// 解析http请求行，获取请求方法，目标url和版本号
http_conn::HTTP_CODE http_conn::parse_request_line(char *text)
{
    // 寻找text中首个以 空格 或 '\t' 开头的字符串,如果没有，说明这个HTTP请求有问题
    m_url = strpbrk(text, " \t");
    if (m_url == nullptr)
    {
        return BAD_REQUEST;
    }
    char *method = text;
    // 将该为置为空并将指针向后移
    *m_url++ = '\0';

    // 忽略大小的比较，相等返回0
    if (strcasecmp(text, "GET") == 0)
    {
        printf("The request method is GET\n");
    }
    else
    {
        // 目前仅支持GET
        return BAD_REQUEST;
    }
    // strspn : 寻找字符串url中第一个不是" \t"匹配的位置
    m_url += strspn(url, " \t");
    m_version = strpbrk(url, " \t");
    if (m_version == nullptr)
    {
        return BAD_REQUEST;
    }
    *m_version++ = '\0';
    m_version += strspn(m_version, " \t");
    if (strcasecmp(m_version, "HTTP/1.1") != 0)
    {
        // 只支持HTTP1.1
        return BAD_REQUEST;
    }
    // 检查url是否合法
    if (strncasecmp(m_url, "http://", 7) == 0)
    {
        m_url += 7;
        // 找到第一个以"/"开头的位置，即资源url
        m_url = strchr(m_url, "/");
    }
    if (m_url == nullptr || m_url[0] != '/')
    {
        return BAD_REQUEST;
    }
    // 即访问url是"hostname:port/"的形式，转到默认界面
    if (strlen(m_url) == 1)
    {
        strcat(m_url, "root.html");
    }
    // 转到解析请求header
    m_check_state = CHECK_STATE_HEADER;
    // 继续读取数据
    return NO_REQUEST;
}