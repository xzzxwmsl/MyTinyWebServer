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
const char *ok_size_zero = "<html><body>Test</body></html>";
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

    bytes_have_send = 0;
    bytes_to_send = 0;

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

// 循环读取客户数据直到没有数据可读
bool http_conn::read()
{
    if (m_read_idx > READ_BUFFER_SIZE)
    {
        return false;
    }

    int bytes_read = 0;
    while (true)
    {
        bytes_read = recv(m_sockfd, m_read_buf + m_read_idx, READ_BUFFER_SIZE - m_read_idx, 0);
        if (bytes_read == -1)
        {
            // EWOULDBLOCK : 发送时缓冲区已满，或者读取时缓冲区已空
            if (errno == EAGAIN || errno == EWOULDBLOCK)
            {
                break;
            }
            else
            {
                return false;
            }
        }
        else if (bytes_read == 0)
        {
            return false;
        }
        // 将已读取量加上去
        m_read_idx += bytes_read;
    }
    return true;
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
    m_url += strspn(m_url, " \t");
    m_version = strpbrk(m_url, " \t");
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
        m_url = strchr(m_url, '/');
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

// 分析HTTP headers部分
http_conn::HTTP_CODE http_conn::parse_headers(char *text)
{
    if (text[0] == '\0')
    {
        if (m_content_length != 0)
        {
            // 还需要处理请求体部分
            m_check_state = CHECK_STATE_CONTENT;
            return NO_REQUEST;
        }
        // 获得了一个完整的请求
        return GET_REQUEST;
    }
    else if (strncasecmp(text, "Host:", 5) == 0)
    {
        // Host 字段
        text += 5;
        // 跳过空格
        text += strspn(text, " \t");
        m_host = text;
    }
    else if (strncasecmp(text, "Content-Length:", 15) == 0)
    {
        text += 15;
        text += strspn(text, " \t");
        if (strlen(text) == strspn(text, "0123456789"))
        {
            // 保证长度一定是数字，否则报错
            m_content_length = atoi(text);
        }
        else
        {
            return BAD_REQUEST;
        }
    }
    else if (strncasecmp(text, "Connection:", 11))
    {
        text += 11;
        text += strspn(text, " \t");
        // 是Keep-Alive的话（返回0）则保持长连接，置为True
        m_linger = !(strcasecmp(text, "Keep-Alive"));
    }
    else
    {
        printf("UNKNOWN HEADER : %s", text);
    }
    // 没有返回的，则说明还需要继续请求数据
    return NO_REQUEST;
}

// 处理HTTP请求体
http_conn::HTTP_CODE http_conn::parse_content(char *text)
{
    if (m_read_idx > m_content_length + m_checked_idx)
    {
        text[m_content_length] = '\0';
        // m_content = text // 请求体
        return GET_REQUEST;
    }
    else
    {
        // 还没接收完
        return NO_REQUEST;
    }
}

// 主状态机，分析http请求的入口函数
http_conn::HTTP_CODE http_conn::process_read()
{
    LINE_STATUS line_status = LINE_OK;
    HTTP_CODE ret = NO_REQUEST;
    char *text = nullptr;
    while ((m_check_state == CHECK_STATE_CONTENT && line_status == LINE_OK) || ((line_status == parse_line()) == LINE_OK))
    {
        // 获取新的一行
        text = get_line();

        m_start_line = m_checked_idx;
        // printf("got http line %s\n", text);
        switch (m_check_state)
        {
        case CHECK_STATE_REQUESTLINE:
        {
            ret = parse_request_line(text);
            if (ret == BAD_REQUEST)
            {
                return BAD_REQUEST;
            }
            break;
        }
        case CHECK_STATE_HEADER:
        {
            ret = parse_headers(text);
            if (ret == BAD_REQUEST)
            {
                return BAD_REQUEST;
            }
            else if (ret == GET_REQUEST)
            {
                return do_request();
            }
            break;
        }
        case CHECK_STATE_CONTENT:
        {
            ret = parse_content(text);
            if (ret == GET_REQUEST)
            {
                return do_request();
            }
            line_status = LINE_OK;
            break;
        }
        default:
            return INTERNAL_ERROR;
        }
    }
    return NO_REQUEST;
}

// 得到了一个完整的request后，分析目标文件属性。
// 如果目标文件存在且可读,且不是目录
// 那么就使用mmap将目标文件映射到内存地址m_file_address
// 中并告知调用者获取文件成功
http_conn::HTTP_CODE http_conn::do_request()
{
    strcpy(m_real_file, m_root);
    int root_len = strlen(m_root);
    /*  strncpy
        把 src 所指向的字符串复制到 dest
        最多复制 n 个字符。当 src 的长度小于 n 时
        dest 的剩余部分将用空字节填充。
    */
    // 写明访问文件
    strncpy(m_real_file + root_len, m_url, FILENAME_LEN - root_len - 1);
    if (stat(m_real_file, &m_file_stat) < 0)
    {
        // 文件不存在
        return NO_RESOURCE;
    }
    if (S_ISDIR(m_file_stat.st_mode))
    {
        return BAD_REQUEST;
    }
    if (!(m_file_stat.st_mode & S_IROTH))
    {
        return FORBIDEN_REQUEST;
    }

    // 访问真实文件
    int fd = open(m_real_file, O_RDONLY);
    // 映射到内存上
    m_file_address = reinterpret_cast<char *>(mmap(0, m_file_stat.st_size, PROT_READ, MAP_PRIVATE, fd, 0));
    close(fd);
    return FILE_REQUEST;
}

// 取消内存映射区
void http_conn::unmap()
{
    if (m_file_address)
    {
        munmap(m_file_address, m_file_stat.st_size);
        m_file_address = 0;
    }
}

// 写HTTP响应
// epoll_wait返回EPOLLOUT后，由服务器主线程调用，来将数据写入到socket中
bool http_conn::write()
{
    int temp = 0;
    if (bytes_to_send == 0)
    {
        // 没有要发送的,重新初始化
        modfd(m_epollfd, m_sockfd, EPOLLIN);
        init();
        return true;
    }

    while (true)
    {
        temp = writev(m_sockfd, m_iv, m_iv_count);
        if (temp < 0)
        {
            /*
                TCP写缓冲区没有空间,则等待下一轮的
                EPOLLOUT事件.在此之间服务器不能立即接收同一个客户的下一个请求
                但是可以保证连接的完整性
            */
            if (errno == EAGAIN)
            {
                modfd(m_epollfd, m_sockfd, EPOLLOUT);
                return true;
            }
            else
            {
                unmap();
                return false;
            }
        }

        // 发送成功
        bytes_have_send += temp;
        bytes_to_send -= temp;

        if (bytes_have_send >= m_iv[0].iov_len)
        {
            // 发送报文成功了,但是文件还没有发送完成
            m_iv[0].iov_len = 0;
            m_iv[1].iov_base = m_file_address + (bytes_have_send - m_write_idx);
            m_iv[1].iov_len = bytes_to_send;
        }
        else
        {
            m_iv[0].iov_base = m_write_buf + bytes_have_send;
            m_iv[0].iov_len = m_write_idx - bytes_have_send;
        }

        // 发送完成
        if (bytes_to_send <= 0)
        {
            unmap();
            // 重新注册socket可读事件
            modfd(m_epollfd, m_sockfd, EPOLLIN);
            if (m_linger)
            {
                init();
                return true;
            }
            else
            {
                return false;
            }
        }
    }
}

bool http_conn::add_respones(const char *format, ...)
{
    if (m_write_idx >= WRITE_BUFFER_SIZE)
    {
        return false;
    }
    va_list arg_list;
    va_start(arg_list, format);
    // 将要写的内容放置到m_write_buf上，然后返回长度
    int len = vsnprintf(m_write_buf + m_write_idx, READ_BUFFER_SIZE - 1 - m_write_idx, format, arg_list);
    if (len >= WRITE_BUFFER_SIZE - 1 - m_write_idx)
    {
        return false;
    }
    m_write_idx += len;
    va_end(arg_list);
    return true;
}

// 响应行
bool http_conn::add_status_line(int status, const char *title)
{
    return add_respones("%s %d %s\r\n", "HTTP/1.1", status, title);
}

// 响应头
bool http_conn::add_headers(int content_length)
{
    add_content_length(content_length);
    add_linger();
    // 响应头和响应体之间应该加个空行
    add_blank_line();
}

// 响应体
bool http_conn::add_content(const char *content)
{
    return add_respones("%s", content);
}

bool http_conn::add_content_length(int content_length)
{
    return add_respones("Content-Length: %d\r\n", content_length);
}

bool http_conn::add_linger()
{
    return add_respones("Connection: %s\r\n", (m_linger) ? "Keep-Alive" : "Close");
}

bool http_conn::add_blank_line()
{
    return add_respones("%s", "\r\n");
}

// 由工作线程的process调用，将数据写入到缓冲区中，然后主线程注册EPOLLOUT事件
bool http_conn::process_write(HTTP_CODE ret)
{
    switch (ret)
    {
    case INTERNAL_ERROR:
    {
        add_status_line(500, error_500_title);
        add_headers(strlen(error_500_form));
        if (!add_content(error_500_form))
        {
            return false;
        }
        break;
    }
    case NO_RESOURCE:
    {
        add_status_line(404, error_404_title);
        add_headers(strlen(error_404_form));
        if (!add_content(error_404_form))
        {
            return false;
        }
        break;
    }
    case BAD_REQUEST:
    {
        add_status_line(400, error_400_title);
        add_headers(strlen(error_400_form));
        if (!add_content(error_400_form))
        {
            return false;
        }
        break;
    }
    case FORBIDEN_REQUEST:
    {
        add_status_line(403, error_403_title);
        add_headers(strlen(error_403_form));
        if (!add_content(error_403_form))
            return false;
        break;
    }
    case FILE_REQUEST:
    {
        add_status_line(200, ok_200_title);
        if (m_file_stat.st_size != 0)
        {
            add_headers(m_file_stat.st_size);
            // 第一个iovec指向报文的缓冲区
            m_iv[0].iov_base = m_write_buf;
            m_iv[0].iov_len = m_write_idx;
            // 第二个iovec指向文件的mmap区域
            m_iv[1].iov_base = m_file_address;
            m_iv[1].iov_len = m_file_stat.st_size;
            m_iv_count = 2;
            bytes_to_send = m_write_idx + m_file_stat.st_size;
            // 发送文件
            return true;
        }
        else
        {
            add_headers(strlen(ok_size_zero));
            if (!add_content(ok_size_zero))
            {
                return false;
            }
        }
        break;
    }
    default:
        return false;
        break;
    }
    // 除了FILE_REQUEST，都会执行本操作，发送报文缓冲区
    m_iv[0].iov_base = m_write_buf;
    m_iv[0].iov_len = m_write_idx;
    m_iv_count = 1;
    bytes_to_send = m_write_idx;
    return true;
}

void http_conn::process()
{
    // 解析HTTP请求
    HTTP_CODE read_ret = process_read();
    // HTTP报文不完整
    if (read_ret == NO_REQUEST)
    {
        modfd(m_epollfd, m_sockfd, EPOLLIN);
        return;
    }

    // 填充响应报文
    bool write_ret = process_write(read_ret);
    if (!write_ret)
    {
        close_conn();
    }
    modfd(m_epollfd, m_sockfd, EPOLLOUT);
}