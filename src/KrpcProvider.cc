// ...existing code for NotifyService remains unchanged...

// 启动RPC服务节点，开始提供远程网络调用服务
void KrpcProvider::Run()
{
    // 读取配置文件中的RPC服务器IP和端口
    std::string ip = KrpcApplication::GetInstance().GetConfig().Load("rpcserverip");
    int port = atoi(KrpcApplication::GetInstance().GetConfig().Load("rpcserverport").c_str());

    // 使用自定义的EpollServer
    EpollServer server;

    // 绑定连接回调和消息回调
    server.setConnectionCallback(std::bind(&KrpcProvider::OnConnection, this, std::placeholders::_1));
    server.setMessageCallback(std::bind(&KrpcProvider::OnMessage, this, std::placeholders::_1, std::placeholders::_2));

    // 将当前RPC节点上要发布的服务全部注册到ZooKeeper上，让RPC客户端可以在ZooKeeper上发现服务
    ZkClient zkclient;
    zkclient.Start(); // 连接ZooKeeper服务器
    // service_name为永久节点，method_name为临时节点
    for (auto &sp : service_map)
    {
        // service_name 在ZooKeeper中的目录是"/"+service_name
        std::string service_path = "/" + sp.first;
        zkclient.Create(service_path.c_str(), nullptr, 0); // 创建服务节点
        for (auto &mp : sp.second.method_map)
        {
            std::string method_path = service_path + "/" + mp.first;
            char method_path_data[128] = {0};
            sprintf(method_path_data, "%s:%d", ip.c_str(), port); // 将IP和端口信息存入节点数据
            // ZOO_EPHEMERAL表示这个节点是临时节点，在客户端断开连接后，ZooKeeper会自动删除这个节点
            zkclient.Create(method_path.c_str(), method_path_data, strlen(method_path_data), ZOO_EPHEMERAL);
        }
    }

    // RPC服务端准备启动，打印信息
    std::cout << "RpcProvider start service at ip:" << ip << " port:" << port << std::endl;

    // 启动网络服务
    if (!server.start(ip, port))
    {
        LOG(ERROR) << "Server start failed";
        return;
    }

    // 主线程阻塞，等待信号
    sigset_t waitset;
    sigemptyset(&waitset);
    sigaddset(&waitset, SIGINT);
    sigaddset(&waitset, SIGTERM);

    // 阻塞当前线程，直到收到SIGINT或SIGTERM信号
    int sig;
    sigwait(&waitset, &sig);

    std::cout << "Receive signal " << sig << ", server stopping..." << std::endl;
    server.stop();
}

// 连接回调函数需要修改为适应新的接口
void KrpcProvider::OnConnection(std::shared_ptr<TcpConnection> conn)
{
    std::cout << "OnConnection" << std::endl;
    // 无需特殊处理，连接关闭时在epollServer内部会处理
}

// 消息回调函数需要修改为适应新的接口
void KrpcProvider::OnMessage(std::shared_ptr<TcpConnection> conn, std::shared_ptr<Buffer> buffer)
{
    std::cout << "OnMessage" << std::endl;

    // 从网络缓冲区中读取远程RPC调用请求的字符流
    std::string recv_buf = buffer->retrieveAllAsString();

    // ...existing code for parsing and handling the message remains largely unchanged...

    // ...existing code for handling the RPC call remains unchanged...

    // 响应函数需要修改，使用新的接口
    google::protobuf::Closure *done = google::protobuf::NewCallback<KrpcProvider,
                                                                    std::shared_ptr<TcpConnection>,
                                                                    google::protobuf::Message *>(this,
                                                                                                 &KrpcProvider::SendRpcResponse,
                                                                                                 conn, response);

    // 在框架上根据远端RPC请求，调用当前RPC节点上发布的方法
    service->CallMethod(method, nullptr, request, response, done); // 调用服务方法
}

// 发送RPC响应函数需要修改为适应新的接口
void KrpcProvider::SendRpcResponse(std::shared_ptr<TcpConnection> conn, google::protobuf::Message *response)
{
    std::string response_str;
    if (response->SerializeToString(&response_str))
    {
        // 序列化成功，通过网络把RPC方法执行的结果返回给RPC调用方
        conn->send(response_str);
    }
    else
    {
        std::cout << "serialize error!" << std::endl;
    }
    // 我们的自定义实现不需要主动关闭连接，采用长连接模式
}

// 析构函数不需要特殊处理event_loop
KrpcProvider::~KrpcProvider()
{
    std::cout << "~KrpcProvider()" << std::endl;
    // 自定义实现中不需要主动退出事件循环
}
