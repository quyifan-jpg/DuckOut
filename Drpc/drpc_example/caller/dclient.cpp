/*
 * @Author: your name
 * @Date: 2022-01-04 20:03:45
 * @LastEditTime: 2022-01-05 19:08:58
 * @LastEditors: your name
 * @Description: 打开koroFileHeader查看配置 进行设置: https://github.com/OBKoro1/koro1FileHeader/wiki/%E9%85%8D%E7%BD%AE
 * @FilePath: \30dayMakeCppServer\code\day01\client.cpp
 */
#include <sys/socket.h>
#include <arpa/inet.h>
#include <string.h>

int main() {
    int sockfd = socket(AF_INET6, SOCK_STREAM, 0);

    struct sockaddr_in6 serv_addr;
    bzero(&serv_addr, sizeof(serv_addr));
    serv_addr.sin6_family = AF_INET6;
    serv_addr.sin6_addr = in6addr_any;
    serv_addr.sin6_port = htons(8888);

    //bind(sockfd, (sockaddr*)&serv_addr, sizeof(serv_addr)); 客户端不进行bind操作

    connect(sockfd, (sockaddr*)&serv_addr, sizeof(serv_addr));    
    send(sockfd, "hello", 5, 0);

    return 0;
}
