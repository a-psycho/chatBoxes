#include <stdio.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>
#include <signal.h>
#include "mysocket.h"
#include "myprotocol.h"

Socket msg_socket(1);

void signal_handler(int signum){
	puts("catched signal SIGPIPE!");
	return;
}

int main(int argc,char** argv){
	signal(SIGPIPE,signal_handler);
	char s[1024];
	char addr[]="127.0.0.1";
	msg_socket.cbind(addr);
	msg_socket.connects(addr,3333);
	msg_socket.printAddr();

	//登录或者注册
	myprotocol tmp;
	tmp.msgtype=SIGN_IN;
	printf("input name ans passwd:\n");
	fgets(s,1024,stdin);
	s[strlen(s)-1]='\0';
	puts(s);
	tmp.msg_body_length=sizeof(s);
	write(msg_socket.fd,(char*)&tmp,sizeof(tmp));
	write(msg_socket.fd,s,sizeof(s));

	puts("here");
	//接收在线成员列表
	read(msg_socket.fd,(char*)&tmp,sizeof(tmp));
	if(tmp.msg_body_length)
		read(msg_socket.fd,s,tmp.msg_body_length);
	if(tmp.msgtype==LOG_FAILED)
		puts("username or password is wrong!");
	printf("onlien user: %s\n",s);

	scanf("%s",s);
	close(msg_socket.fd);
	return 0;
}
