#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "mysocket.h"

Addr::Addr(char *a,int b){
	memset(&addr,0,8);
	family=AF_INET;
	if(!inet_aton(a,(struct in_addr*)&addr)){
		printf("Format of IP address is error!\n");
		exit(-1);
	}
	port=htons((unsigned short)b);
}

unsigned Addr::getIp(){
	return addr;
}

char* Addr::getIpStr(){
	return inet_ntoa(*((struct in_addr*)&addr));
}

Socket::Socket(){
	fd=-1;
	type=1;
	memset(acceptAddr,0,16);
}

Socket::Socket(int a){
	fd=-1;
	bindAddr=NULL;
	memset(acceptAddr,0,16);
	if(a==1){
		type=1;
		fd=socket(AF_INET,SOCK_STREAM,0);
	}
	else if(a==2){
		type=2;
		fd=socket(AF_INET,SOCK_DGRAM,0);
	}
	if(fd==-1){
		printf("Create socket failed!\n");
		exit(-1);
	}
}

//服务器要绑定IP和端口
void Socket::sbind(char* a,int b){
	bindAddr=new Addr(a,b);
	if(bind(fd,(struct sockaddr*)bindAddr,16)<0){
		printf("Bind (ip,prot) failed!\n");
		close(fd);
		exit(-1);
	}
}

//客户端只用绑定IP即可
void Socket::cbind(char* a){
	bindAddr=new Addr(a,0);
	if(bind(fd,(struct sockaddr*)bindAddr,16)<0){
		printf("Bind (ip,prot) failed!\n");
		close(fd);
		exit(-1);
	}
}

//服务器创建listen队列
void Socket::slisten(int qlen){
	if(type==2){
		printf("Udp scoket cat's use this functions!\n");
		close(fd);
		exit(-1);
	}
	if(listen(fd,qlen)!=0){
		printf("Create listen-queen failed!\n");
		close(fd);
		exit(-1);
	}
}

//服务器接收请求
int Socket::saccept(){
	addrlen=16;
	int i=accept(fd,(struct sockaddr*)acceptAddr,(socklen_t*)&addrlen);
	//printf("Accept addr from %s : %d\n",inet_ntoa(*(struct in_addr*)(acceptAddr+3)),(unsigned short)*(acceptAddr+2));
	if(i<0){
		printf("Accept failed! <errno : %d>\n",errno);
		exit(-1);
	}
	return i;
}

//tcp或udp连接服务端
void Socket::connects(char *a,int b){
	Addr *tmp=new Addr(a,b);
	if(connect(fd,(struct sockaddr*)tmp,16)<0){
		printf("Connect failed!\n");
		close(fd);
		exit(-1);
	}
}

//接收数据
int Socket::sread(char *a,int b){
	int n;
	while(true){
		n=read(fd,a,b);
		if(n>0)
			return n;
		else if(n==0){
			puts("This socket cat's use <read> as it's closed!");
			close(fd);
			exit(-1);
		}
		else if(errno==EINTR){;}
		else if(errno==ECONNRESET){
			puts("Tcp connection has benn closed!\n");
			close(fd);
			exit(-1);
		}
		else{
			puts("read failed!");
			close(fd);
			exit(-1);
		}
	}
}

//发送字符串
int Socket::swrite(char *a){
	int n;
	while(true){
		n=write(fd,a,strlen(a)+1);
		if(n>0)
			return n;
		else if(errno==EINTR){;}
		else if(errno==EPIPE){
			puts("Tcp connection has benn closed!\n");
			close(fd);
			exit(-1);
		}
		else{
			puts("write failed!");
			close(fd);
			exit(-1);
		}
	}
}

//关闭套接字
int Socket::sclose(){
	return close(fd);
}

void Socket::printAddr(){
	int len=16;
	char addr[16];
	memset(addr,0,16);
	if(getpeername(fd,(struct sockaddr*)addr,(socklen_t*)&len)==0)
		printf("Accept addr from %s : %d\n",inet_ntoa(*(struct in_addr*)(addr+4)),(unsigned short)*(addr+2));
	else
		puts("Get address failed!");
}
