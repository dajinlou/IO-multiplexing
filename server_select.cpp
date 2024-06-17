#include "wrap.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include <ctype.h>
#include <iostream>
#include <unistd.h>

using namespace std;

#define SERV_PORT 6666

int main()
{

    int i,j,n,max_i;
    int nready,client[FD_SETSIZE]; //默认为1024
    int maxfd,listenfd,connfd,sockfd;
    char buf[BUFSIZ],str[INET_ADDRSTRLEN]; //默认16

    struct sockaddr_in clie_addr,serv_addr;
    socklen_t clie_addr_len;
    fd_set rset,allset;    //维护一个读集合

    listenfd = Socket(AF_INET,SOCK_STREAM,0);
    int opt = 1;
    setsockopt(listenfd,SOL_SOCKET,SO_REUSEADDR,&opt,sizeof(opt));

    bzero(&serv_addr,sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    serv_addr.sin_port = htons(SERV_PORT);

    Bind(listenfd,(struct sockaddr *)&serv_addr,sizeof(serv_addr));
    Listen(listenfd,128);
    maxfd = listenfd;
    max_i = -1;

    for(int i=0;i<FD_SETSIZE;++i){
        client[i] = -1;
    }
    
    FD_ZERO(&allset);
    FD_SET(listenfd,&allset);//构造select监控文件描述符集

    while(1){
        rset = allset;  //每次循环时都重新设置select监控信号集
        nready = select(maxfd+1,&rset,NULL,NULL,NULL);

        if(nready<0){
            perr_exit("select error");
        }
        if(FD_ISSET(listenfd,&rset)){ //有新的客户端连接请求
            clie_addr_len = sizeof(clie_addr);
            connfd = Accept(listenfd,(struct sockaddr*)&clie_addr,&clie_addr_len); //Accept不会阻塞
            cout<<"receive from "<< inet_ntop(AF_INET,&clie_addr.sin_addr,str,sizeof(str))<<" PORT:"<<ntohs(clie_addr.sin_port)<<endl;

            for(i=0;i<FD_SETSIZE;++i){
                if(client[i]<0){     //查找client数组中没有使用的位置
                    client[i] = connfd; 
                    break;
                }
            }
            if(i == FD_SETSIZE){
                fputs("too many clients\n",stderr);
                exit(1);
            }

            FD_SET(connfd,&allset); //加入到文件描述符集合
            if(connfd>maxfd){
                maxfd = connfd;
            }
            if(i>max_i){
                max_i = i;
            }
            if(--nready == 0){
                continue;
            }

        }
        for(i=0;i<=max_i;++i){ //检测哪个clients有数据就绪
            if((sockfd = client[i])<0){
                continue;
            }
            if(FD_ISSET(sockfd,&rset)){
                if((n=Read(sockfd,buf,sizeof(buf)))==0){
                    Close(sockfd);
                    FD_CLR(sockfd,&allset);   //将关闭的客户端描述符移出
                    client[i]=-1;
                }
                else if(n>0){
                    for(j=0;j<n;++j){
                        buf[j] = toupper(buf[j]);
                    }
                    Write(sockfd,buf,n);
                    Write(STDOUT_FILENO,buf,n);
                }
                if(--nready == 0){
                    break;
                }
            }
        }

    }

    Close(listenfd);






    return 0;
}

