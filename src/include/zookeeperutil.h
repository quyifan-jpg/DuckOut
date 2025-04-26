#ifndef _ZOOKEEPER_UTIL_H_
#define _ZOOKEEPER_UTIL_H_

#include <zookeeper/zookeeper.h>
#include <string>
#include <vector>

// ZooKeeper客户端工具类，用于与ZooKeeper服务器交互
class ZkClient
{
public:
    ZkClient();
    ~ZkClient();

    // 启动ZooKeeper客户端
    void Start();

    // 创建ZooKeeper节点
    void Create(const char *path, const char *data, int datalen, int state = 0);

    // 获取ZooKeeper节点的数据
    std::string GetData(const char *path);

    // 获取指定路径下的所有子节点
    std::vector<std::string> GetChildren(const char *path);

    // 获取指定路径下的所有子节点数据
    std::vector<std::string> GetChildrenData(const char *path);

private:
    zhandle_t *m_zhandle; // ZooKeeper客户端句柄
};

#endif
