#include "epollServer.h"
#include "KrpcLogger.h"
#include <cstring>
#include <fcntl.h>
#include <netinet/in.h>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <unistd.h>
#include <arpa/inet.h>

// 设置socket为非阻塞模式
int setNonBlocking(int fd)
{
    int flags = fcntl(fd, F_GETFL, 0);
    if (flags == -1)
    {
        LOG(ERROR) << "fcntl F_GETFL error";
        return -1;
    }
    flags |= O_NONBLOCK;
    if (fcntl(fd, F_SETFL, flags) == -1)
    {
        LOG(ERROR) << "fcntl F_SETFL error";
        return -1;
    }
    return 0;
}

EpollServer::EpollServer() : listenFd_(-1), epollFd_(-1), running_(false)
{
    // 创建线程池
    threadPool_ = std::make_unique<ThreadPool>(8); // 默认8个线程
}

EpollServer::~EpollServer()
{
    stop();
}

bool EpollServer::start(const std::string &ip, int port)
{
    // 创建socket
    listenFd_ = socket(AF_INET, SOCK_STREAM, 0);
    if (listenFd_ < 0)
    {
        LOG(ERROR) << "socket create error";
        return false;
    }

    // 设置端口复用
    int opt = 1;
    if (setsockopt(listenFd_, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0)
    {
        LOG(ERROR) << "setsockopt error";
        close(listenFd_);
        return false;
    }

    // 绑定IP和端口
    struct sockaddr_in serverAddr;
    memset(&serverAddr, 0, sizeof(serverAddr));
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(port);
    if (inet_pton(AF_INET, ip.c_str(), &serverAddr.sin_addr) <= 0)
    {
        LOG(ERROR) << "inet_pton error";
        close(listenFd_);
        return false;
    }

    if (bind(listenFd_, (struct sockaddr *)&serverAddr, sizeof(serverAddr)) < 0)
    {
        LOG(ERROR) << "bind error";
        close(listenFd_);
        return false;
    }

    // 开始监听
    if (listen(listenFd_, 1024) < 0)
    {
        LOG(ERROR) << "listen error";
        close(listenFd_);
        return false;
    }

    // 设置为非阻塞
    if (setNonBlocking(listenFd_) < 0)
    {
        close(listenFd_);
        return false;
    }

    // 创建epoll实例
    epollFd_ = epoll_create1(0);
    if (epollFd_ < 0)
    {
        LOG(ERROR) << "epoll_create1 error";
        close(listenFd_);
        return false;
    }

    // 将监听socket添加到epoll
    struct epoll_event ev;
    ev.events = EPOLLIN;
    ev.data.fd = listenFd_;
    if (epoll_ctl(epollFd_, EPOLL_CTL_ADD, listenFd_, &ev) < 0)
    {
        LOG(ERROR) << "epoll_ctl error";
        close(listenFd_);
        close(epollFd_);
        return false;
    }

    // 启动epoll线程
    running_ = true;
    epollThread_ = std::thread(&EpollServer::epollLoop, this);

    std::cout << "Server started on " << ip << ":" << port << std::endl;
    return true;
}

void EpollServer::stop()
{
    if (!running_)
        return;

    running_ = false;
    if (epollThread_.joinable())
    {
        epollThread_.join();
    }

    if (epollFd_ >= 0)
    {
        close(epollFd_);
        epollFd_ = -1;
    }

    if (listenFd_ >= 0)
    {
        close(listenFd_);
        listenFd_ = -1;
    }

    // 关闭所有客户端连接
    for (auto &fd : clientFds_)
    {
        close(fd);
    }
    clientFds_.clear();

    std::cout << "Server stopped" << std::endl;
}

void EpollServer::setConnectionCallback(const ConnectionCallback &cb)
{
    connectionCallback_ = cb;
}

void EpollServer::setMessageCallback(const MessageCallback &cb)
{
    messageCallback_ = cb;
}

void EpollServer::epollLoop()
{
    constexpr int MAX_EVENTS = 1024;
    struct epoll_event events[MAX_EVENTS];

    while (running_)
    {
        int nfds = epoll_wait(epollFd_, events, MAX_EVENTS, 100); // 100ms timeout

        if (nfds < 0)
        {
            if (errno == EINTR)
                continue; // 被信号中断，重试
            LOG(ERROR) << "epoll_wait error: " << strerror(errno);
            break;
        }

        for (int i = 0; i < nfds; ++i)
        {
            int fd = events[i].data.fd;

            // 处理新连接
            if (fd == listenFd_)
            {
                handleNewConnection();
            }
            // 处理已有连接的读事件
            else if (events[i].events & EPOLLIN)
            {
                threadPool_->enqueue([this, fd]
                                     { handleReadEvent(fd); });
            }
        }
    }
}

void EpollServer::handleNewConnection()
{
    struct sockaddr_in clientAddr;
    socklen_t clientAddrLen = sizeof(clientAddr);

    int clientFd = accept(listenFd_, (struct sockaddr *)&clientAddr, &clientAddrLen);
    if (clientFd < 0)
    {
        LOG(ERROR) << "accept error: " << strerror(errno);
        return;
    }

    // 设置为非阻塞
    if (setNonBlocking(clientFd) < 0)
    {
        close(clientFd);
        return;
    }

    // 添加到epoll监听
    struct epoll_event ev;
    ev.events = EPOLLIN;
    ev.data.fd = clientFd;
    if (epoll_ctl(epollFd_, EPOLL_CTL_ADD, clientFd, &ev) < 0)
    {
        LOG(ERROR) << "epoll_ctl error";
        close(clientFd);
        return;
    }

    // 保存客户端连接
    {
        std::lock_guard<std::mutex> lock(mutex_);
        clientFds_.insert(clientFd);
    }

    // 回调通知新连接
    if (connectionCallback_)
    {
        // 创建一个简单的TcpConnection对象表示连接
        auto conn = std::make_shared<TcpConnection>(clientFd, clientAddr);
        connectionCallback_(conn);
    }

    char clientIP[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &clientAddr.sin_addr, clientIP, sizeof(clientIP));
    std::cout << "New connection from " << clientIP << ":" << ntohs(clientAddr.sin_port)
              << ", socket fd = " << clientFd << std::endl;
}

void EpollServer::handleReadEvent(int fd)
{
    char buffer[4096];
    ssize_t n = read(fd, buffer, sizeof(buffer));

    if (n < 0)
    {
        if (errno != EAGAIN && errno != EWOULDBLOCK)
        {
            LOG(ERROR) << "read error: " << strerror(errno);
            closeConnection(fd);
        }
    }
    else if (n == 0)
    {
        // 连接关闭
        std::cout << "Connection closed by peer, fd = " << fd << std::endl;
        closeConnection(fd);
    }
    else
    {
        // 回调处理消息
        if (messageCallback_)
        {
            struct sockaddr_in addr;
            socklen_t addrLen = sizeof(addr);
            getpeername(fd, (struct sockaddr *)&addr, &addrLen);

            auto conn = std::make_shared<TcpConnection>(fd, addr);
            auto msgBuffer = std::make_shared<Buffer>();
            msgBuffer->append(buffer, n);

            messageCallback_(conn, msgBuffer);
        }
    }
}

void EpollServer::closeConnection(int fd)
{
    // 从epoll中移除
    epoll_ctl(epollFd_, EPOLL_CTL_DEL, fd, nullptr);

    // 从客户端列表中移除
    {
        std::lock_guard<std::mutex> lock(mutex_);
        clientFds_.erase(fd);
    }

    close(fd);
}

// TcpConnection实现
TcpConnection::TcpConnection(int fd, const struct sockaddr_in &addr)
    : fd_(fd), peerAddr_(addr) {}

TcpConnection::~TcpConnection() {}

void TcpConnection::send(const std::string &message)
{
    ssize_t n = write(fd_, message.data(), message.size());
    if (n < 0)
    {
        LOG(ERROR) << "write error: " << strerror(errno);
    }
}

void TcpConnection::shutdown()
{
    if (::shutdown(fd_, SHUT_WR) < 0)
    {
        LOG(ERROR) << "shutdown error: " << strerror(errno);
    }
}

bool TcpConnection::connected() const
{
    return fd_ >= 0;
}

// Buffer实现
void Buffer::append(const char *data, size_t len)
{
    buffer_.append(data, len);
}

std::string Buffer::retrieveAllAsString()
{
    std::string result = buffer_;
    buffer_.clear();
    return result;
}
