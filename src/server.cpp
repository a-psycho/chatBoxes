#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <pthread.h>
#include "mysocket.h"
#include "onlineuser.h"
#include "myprotocol.h"


#define BIND_PORT 3333
#define MAX_LISTEN_QUEUE 8

//用户处理线程
void* user_thread(void*);
//发送信息
void msg_send(int,char*,int);
//用户登录和注册函数
int user_sign_in(char*);
int user_sign_up(char*);
//加入在线成员列表
int add_usr_to_onlinelist(int,int,char*);
//发送在线成员给刚登陆用户
void send_all_online_users(int);
//上线掉线信息发送给所有用户
void online_msg_to_all(int,char*);
void offline_msg_to_all(int);

//全局变量
pthread_mutex_t mutex,file_mutex,sock_mutex;
Online_user_list online_user_list;
int msg_header_length=sizeof(myprotocol);

void signal_handler(int signum){
	puts("catched signal SIGPIPE!");
	return;
}

int main(int argc,char** argv){
	//初始化互斥锁
	if(pthread_mutex_init(&mutex,NULL)!=0 && pthread_mutex_init(&file_mutex,NULL)!=0
			&& pthread_mutex_init(&sock_mutex,NULL)!=0){
		puts("initialize mutex failed!");
		exit(-1);
	}

	//套接字处理工作
	Socket sk(1);
	int sock_fd_tmp;
	char addr[]="127.0.0.1";
	sk.sbind(addr,BIND_PORT);
	sk.slisten(MAX_LISTEN_QUEUE);

	//接收请求并创建新线程
	pthread_t tid;
	while(1){
		sock_fd_tmp=sk.saccept();
		pthread_create(&tid,NULL,user_thread,&sock_fd_tmp);
		pthread_detach(tid);
	}
}

void* user_thread(void *args){
	signal(SIGPIPE,signal_handler);
	int uid=0,fd=*(int*)args,online_flag=0;

	//处理用户请求
	char s[1024],*name;
	myprotocol msg_header;
	while(1){
		//获取消息
		int n;
		memset(&msg_header,0,msg_header_length);
		n=read(fd,&msg_header,msg_header_length);
		//消息为文件传送，单独处理
		if(msg_header.msgtype==FILE_TO_USR){
			pthread_mutex_lock(&sock_mutex);
			int dataSize=msg_header.file_size+msg_header.file_name_length;
			msg_send(msg_header.duid,(char*)&msg_header,msg_header_length);
			if(dataSize<=1024 && dataSize>0){
				memset(s,0,1024);
				read(fd,s,dataSize);
				msg_send(msg_header.duid,s,dataSize);
			}
			else{
				for(int i=0;i<dataSize/1024;i++){
					memset(s,0,1024);
					read(fd,s,1024);
					msg_send(msg_header.duid,s,1024);
				}
				if(dataSize%1024){
					memset(s,0,1024);
					read(fd,s,dataSize%1024);
					msg_send(msg_header.duid,s,dataSize%1024);
				}
			}
			pthread_mutex_unlock(&sock_mutex);
			continue;
		}
		memset(s,0,1024);
		if(msg_header.msg_body_length)
			n=read(fd,s,msg_header.msg_body_length);
		if((n==-1 && errno==ECONNRESET) | n==0){
			if(online_flag){
				printf("User %s <uid:%d> lost connection.Server has ended it's thread\n",name,uid);
				offline_msg_to_all(uid);
			}
			close(fd);
			pthread_exit(NULL);
		}
		switch(msg_header.msgtype){
			case SIGN_IN:
				uid=user_sign_in(s);
				if(uid>0){
					name=new char[256];
					sscanf(s,"%s",name);
					printf("user %s <uid:%d> log in\n",name,uid);
					msg_header.duid=uid;
					msg_header.msgtype=LOG_SUCCESS;
					msg_header.msg_body_length=0;
					write(fd,(char*)&msg_header,msg_header_length);
					online_msg_to_all(uid,name);
					send_all_online_users(fd);
					add_usr_to_onlinelist(uid,fd,name);
					online_flag=1;
				}
				else{
					msg_header.msgtype=LOG_FAILED;
					msg_header.msg_body_length=0;
					write(fd,(char*)&msg_header,msg_header_length);
				}
				break;
			case SIGN_UP:
				uid=user_sign_up(s);
				if(uid>0){
					name=new char[256];
					sscanf(s,"%s",name);
					printf("user %s <uid:%d> sing up & log in\n",name,uid);
					msg_header.duid=uid;
					msg_header.msgtype=LOG_SUCCESS;
					msg_header.msg_body_length=0;
					write(fd,(char*)&msg_header,msg_header_length);
					online_msg_to_all(uid,name);
					send_all_online_users(fd);
					add_usr_to_onlinelist(uid,fd,name);
					online_flag=1;
				}
				else{
					msg_header.msgtype=USER_EXIST;
					msg_header.msg_body_length=0;
					write(fd,(char*)&msg_header,msg_header_length);
				}
				break;
			case LOG_OUT:
				printf("user %s <uid:%d> log out\n",name,uid);
				offline_msg_to_all(uid);
				close(fd);
				pthread_exit(NULL);
				break;
			case MSG_TO_USR:
			case ACK_YES:
				pthread_mutex_lock(&sock_mutex);
				msg_send(msg_header.duid,(char*)&msg_header,msg_header_length);
				msg_send(msg_header.duid,s,msg_header.msg_body_length);
				pthread_mutex_unlock(&sock_mutex);
				break;
			default:
				printf("unknow msg type from %s <uid:%d>.Server has shut down the connection!\n",name,uid);
				offline_msg_to_all(uid);
				close(fd);
				pthread_exit(NULL);
		}
	}
}

void msg_send(int uid,char* msg,int length){
	pthread_mutex_lock(&mutex);
	for(int i=0;i<MAX_ONELINE_USERS;i++)
		if(online_user_list.index[i]==uid)
			write(online_user_list.online_user[i].sock_fd,msg,length);
	pthread_mutex_unlock(&mutex);
}

int user_sign_in(char *s){
	int id=0;
	FILE *fp=NULL;
	char s1[32],s2[32];
	char name[32],passwd[32];
	sscanf(s,"%s%s",s1,s2);
	fp=fopen("usermsg.txt","r");
	if(!fp){
		puts("open file failed!");
		exit(-1);
	}
	while(fscanf(fp,"%d %s %s",&id,name,passwd)!=EOF)
		if(strcmp(s1,name)==0 && strcmp(s2,passwd)==0)
			return id;
	fclose(fp);
	return -(id+1);
}

int user_sign_up(char *s){
	FILE *fp;
	int new_uid=1;
	new_uid=user_sign_in(s);
	if(new_uid>0)
		return 0;		//用户已存在
	fp=fopen("usermsg.txt","a");
	if(!fp){
		puts("open file failed!");
		exit(-1);
	}
	pthread_mutex_lock(&file_mutex);
	fprintf(fp,"%d %s\n",-new_uid,s);
	fclose(fp);
	pthread_mutex_unlock(&file_mutex);
	return -new_uid;
}

int add_usr_to_onlinelist(int uid,int fd,char* name){
	pthread_mutex_lock(&mutex);
	for(int i=0;i<MAX_ONELINE_USERS;i++)
		if(online_user_list.index[i]==0){
			online_user_list.index[i]=uid;
			online_user_list.online_user[i].sock_fd=fd;
			online_user_list.online_user[i].username=name;
			pthread_mutex_unlock(&mutex);
			return 0;
		}
	pthread_mutex_unlock(&mutex);
	return -1;
}

void send_all_online_users(int fd){
	char s[1024],t[128];
	memset(s,0,1024);
	memset(t,0,128);
	myprotocol msg_header;
	msg_header.msgtype=ONLINE_USERS;
	pthread_mutex_lock(&mutex);
	for(int i=0;i<MAX_ONELINE_USERS;i++)
		if(online_user_list.index[i]!=0){
			sprintf(t," %d %s",online_user_list.index[i],online_user_list.online_user[i].username);
			strcat(s,t);
		}
	pthread_mutex_unlock(&mutex);
	msg_header.msg_body_length=strlen(s);
	pthread_mutex_lock(&sock_mutex);
	write(fd,(char*)&msg_header,msg_header_length);
	if(msg_header.msg_body_length)
		write(fd,s,msg_header.msg_body_length);
	pthread_mutex_unlock(&sock_mutex);
}

void online_msg_to_all(int uid,char* name){
	myprotocol msg_header;
	msg_header.msgtype=ONLINE;
	msg_header.suid=uid;
	msg_header.msg_body_length=strlen(name);
	pthread_mutex_lock(&mutex);
	for(int i=0;i<MAX_ONELINE_USERS;i++)
		if(online_user_list.index[i]!=0){
			write(online_user_list.online_user[i].sock_fd,(char*)&msg_header,msg_header_length);
			write(online_user_list.online_user[i].sock_fd,name,strlen(name));
		}
	pthread_mutex_unlock(&mutex);
}

void offline_msg_to_all(int uid){
	myprotocol msg_header;
	msg_header.msgtype=OFFLINE;
	msg_header.suid=uid;
	msg_header.msg_body_length=0;
	pthread_mutex_lock(&mutex);
	for(int i=0;i<MAX_ONELINE_USERS;i++){
		if(online_user_list.index[i]==uid){
			online_user_list.index[i]=0;
			delete [] online_user_list.online_user[i].username;
			online_user_list.online_user[i].username=NULL;
		}
		else if(online_user_list.index[i]!=0)
			write(online_user_list.online_user[i].sock_fd,(char*)&msg_header,msg_header_length);
	}
	pthread_mutex_unlock(&mutex);
}

