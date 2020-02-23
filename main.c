#include <winsock2.h>
#include <windows.h>
#include <stdio.h>
#pragma comment(lib,"ws2_32.lib")

void main() {
    //初始化WSA
    WORD sockVersion = MAKEWORD(2, 2);
    WSADATA wsaData;
    if (WSAStartup(sockVersion, &wsaData) != 0) {
        return;
    }

    //创建套接字
    SOCKET slisten = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (slisten == INVALID_SOCKET) {
        printf("socket error !");
        return;
    }

    //绑定IP和端口
    struct sockaddr_in sin;
    sin.sin_family = AF_INET;
    sin.sin_port = htons(8888);
    sin.sin_addr.S_un.S_addr = INADDR_ANY;
    if (bind(slisten, (LPSOCKADDR) &sin, sizeof(sin)) == SOCKET_ERROR) {
        printf("bind error !");
    }

    //开始监听
    if (listen(slisten, 5) == SOCKET_ERROR) {
        printf("listen error !");
        return;
    }

    //循环接收数据
    SOCKET sClient;
    struct sockaddr_in remoteAddr;
    int nAddrlen = sizeof(remoteAddr);
    //char revData[255];
    printf("等待连接...\n");
    sClient = accept(slisten, (SOCKADDR *) &remoteAddr, &nAddrlen);
    //while (1) {
    //}
    char revData[255];
    if (sClient == INVALID_SOCKET) {
        printf("accept error !");
        //continue;
    }
    //接收数据
    int ret = recv(sClient, revData, 255, 0);
    if (ret > 0) {
        revData[ret] = 0x00;
        printf(revData);
    }
    closesocket(sClient);
    closesocket(slisten);
    WSACleanup();
}