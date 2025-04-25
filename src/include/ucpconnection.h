// // ucx_connection.h
// #pragma once
// #include <ucp/api/ucp.h>
// #include <functional>
// #include <unordered_map>
// #include <vector>
// #include <sys/epoll.h>
// #include <unistd.h>
// #include <memory>

// class UCXConnection;
// using UCXConnectionPtr = std::shared_ptr<UCXConnection>;

// using ConnectionCallback = std::function<void(const UCXConnectionPtr&)>;
// using MessageCallback = std::function<void(const UCXConnectionPtr&, const char*, size_t)>;

// class UCXServer {
// public:
//     UCXServer(ucp_context_h context, ucp_worker_h worker);
//     ~UCXServer();

//     bool listen(const struct sockaddr* addr, size_t addrlen);
//     void setConnectionCallback(ConnectionCallback cb);
//     void setMessageCallback(MessageCallback cb);
//     void run();

// private:
//     void pollEvents();
//     void onConnection(ucp_conn_request_h conn_request);
//     void progress();

//     ucp_context_h context_;
//     ucp_worker_h worker_;
//     ucp_listener_h listener_;
//     int epoll_fd_;
//     ConnectionCallback connection_cb_;
//     MessageCallback message_cb_;
// };

// class UCXConnection : public std::enable_shared_from_this<UCXConnection> {
// public:
//     UCXConnection(ucp_worker_h worker, ucp_ep_h ep, MessageCallback msg_cb);
//     ~UCXConnection();

//     void asyncSend(const char* data, size_t size);
//     void asyncRecv();

// private:
//     static void sendHandler(void* request, ucs_status_t status);
//     static void recvHandler(void* request, ucs_status_t status, ucp_tag_recv_info_t* info);

//     ucp_worker_h worker_;
//     ucp_ep_h ep_;
//     MessageCallback msg_cb_;
//     std::vector<char> recv_buffer_;
// };
