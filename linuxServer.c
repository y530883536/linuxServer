#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<errno.h>
#include<sys/types.h>
#include<sys/socket.h>
#include<netinet/in.h>
#define DEFAULT_PORT 8888
#define MAXLINE 65536

void changeToAndroidInstruct(char *originInst);

int main(int argc, char** argv)
{
    int    socket_fd, connect_fd;
    struct sockaddr_in     servaddr;
    char    buff[65536];
    int     n;
    //初始化Socket
    if( (socket_fd = socket(AF_INET, SOCK_STREAM, 0)) == -1 ){
        printf("create socket error: %s(errno: %d)\n",strerror(errno),errno);
        exit(0);
    }
    //初始化
    memset(&servaddr, 0, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);//IP地址设置成INADDR_ANY,让系统自动获取本机的IP地址。
    servaddr.sin_port = htons(DEFAULT_PORT);//设置的端口为DEFAULT_PORT

    //将本地地址绑定到所创建的套接字上
    if( bind(socket_fd, (struct sockaddr*)&servaddr, sizeof(servaddr)) == -1){
        printf("bind socket error: %s(errno: %d)\n",strerror(errno),errno);
        exit(0);
    }
    //开始监听是否有客户端连接
    if( listen(socket_fd, 10) == -1){
        printf("listen socket error: %s(errno: %d)\n",strerror(errno),errno);
        exit(0);
    }
    //printf("======waiting for client's request======\n");
    //while(1){
        //阻塞直到有客户端连接，不然多浪费CPU资源。
        /*if( (connect_fd = accept(socket_fd, (struct sockaddr*)NULL, NULL)) == -1){
            printf("accept socket error: %s(errno: %d)",strerror(errno),errno);
            continue;
        }*/
        connect_fd = accept(socket_fd, (struct sockaddr*)NULL, NULL);
        printf("有客户端连上服务器\n");
        //接受客户端传过来的数据

        int number = 0;
        while(1) {
            for (int i = 0; i < sizeof(buff); i++){
                buff[i] = '\0';
            }
            n = recv(connect_fd, buff, MAXLINE, 0);
            if(n == 0) {
                //printf("已经接收到结束的指令");
                break;
            }
            //printf("%s\n",buff);
            //printf("接收到的字符串数量是:%d\n",strlen(buff));
            //字符串的初始位置
            char *start = strstr(buff,"MULTI");
            //字符串的结束位置
            char *end = start + strlen(buff);
            while(1) {
                char msg[100] = {0};
                char *next = strstr(start+1,"MULTI");
                //如果大于0说明还有下一个指令
                if(next > 0) {
                    memcpy(msg,start, next-start);
                    start = next;
                    //printf("%s\n",msg);
                    changeToAndroidInstruct(msg);
                    number ++;
                }
                //没有的话说明后面都没有指令了，当前是最后一条指令
                else{
                    memcpy(msg,start,end-start);
                    //printf("%s\n",msg);
                    changeToAndroidInstruct(msg);
                    number ++;
                    break;
                }
            }
            //changeToAndroidInstruct(buff);
            //printf("执行了一条指令%d\n",number);
        }
        //printf(buff);
        close(connect_fd);
        //printf("传输结束，执行了%d条指令\n",number);
    //}
    close(socket_fd);
}