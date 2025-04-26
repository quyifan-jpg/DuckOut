#pragma once

#include <atomic>
#include <functional>
#include <memory>
#include <mutex>
#include <set>
#include <string>
#include <thread>
#include <netinet/in.h>
#include <vector>
#include <deque>
#include <condition_variable>
// 简单的线程池实现
class ThreadPool
{
public:
    explicit ThreadPool(size_t numThreads)
    {
        for (size_t i = 0; i < numThreads; ++i)
        {
            workers_.emplace_back([this]
                                  {
                while (true) {
                    std::function<void()> task;
                    {
                        std::unique_lock<std::mutex> lock(mutex_);
                        condition_.wait(lock, [this] { 
                            return !running_ || !tasks_.empty(); 
                        });
                        
                        if (!running_ && tasks_.empty())
                            return;
                            
                        task = std::move(tasks_.front());
                        tasks_.pop_front();
                    }
                    task();
                } });
        }
    }

    ~ThreadPool()
    {
        {
            std::unique_lock<std::mutex> lock(mutex_);
            running_ = false;
        }
        condition_.notify_all();
        for (auto &worker : workers_)
            worker.join();
    }

    template <class F>
    void enqueue(F &&f)
    {
        {
            std::unique_lock<std::mutex> lock(mutex_);
            tasks_.emplace_back(std::forward<F>(f));
        }
        condition_.notify_one();
    }

private:
    std::vector<std::thread> workers_;
    std::deque<std::function<void()>> tasks_;
    std::mutex mutex_;
    std::condition_variable condition_;
    bool running_ = true;
};

// 缓冲区类，类似于muduo的Buffer
class Buffer
{
public:
    void append(const char *data, size_t len);
    std::string retrieveAllAsString();

private:
    std::string buffer_;
};

// TCP连接类，简化版的muduo TcpConnection
class TcpConnection : public std::enable_shared_from_this<TcpConnection>
{
public:
    TcpConnection(int fd, const struct sockaddr_in &addr);
    ~TcpConnection();

    void send(const std::string &message);
    void shutdown();
    bool connected() const;

private:
    int fd_;
    struct sockaddr_in peerAddr_;
};

// Epoll服务器类
class EpollServer
{
public:
    using ConnectionCallback = std::function<void(std::shared_ptr<TcpConnection>)>;
    using MessageCallback = std::function<void(std::shared_ptr<TcpConnection>, std::shared_ptr<Buffer>)>;

    EpollServer();
    ~EpollServer();

    bool start(const std::string &ip, int port);
    void stop();

    void setConnectionCallback(const ConnectionCallback &cb);
    void setMessageCallback(const MessageCallback &cb);

private:
    void epollLoop();
    void handleNewConnection();
    void handleReadEvent(int fd);
    void closeConnection(int fd);

    int listenFd_;
    int epollFd_;
    std::atomic<bool> running_;
    std::thread epollThread_;
    std::unique_ptr<ThreadPool> threadPool_;

    std::mutex mutex_;
    std::set<int> clientFds_; // 当前连接的客户端socket文件描述符

    ConnectionCallback connectionCallback_;
    MessageCallback messageCallback_;
};
