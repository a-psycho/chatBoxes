#include <ncurses.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <signal.h>
#include <errno.h>
#include <ncurses.h>
#include <unistd.h>
#include "onlineuser.h"
#include "mysocket.h"
#include "myprotocol.h"

char mygetch();
//登录或注册界面
void signin_or_signup();
//创建界面框架
void stdscr_init();
//接收消息的线程
void* recv_msg_thread(void *);
//处理用户输出
void handle_user_input();
//在线成员添加删除并刷新
void del_usr_from_onlinelist(int);
int add_usr_to_onlinelist(int,char*);
void online_window_refresh();
//是否在在线成员列表中
int in_online_list(int);

struct Msg_showed{
	char* s;
	int uid,type,msgid,state,lines;
	struct Msg_showed *next;
};
class Msg_in_chat{
	public:
		int line,lines_used;
		struct Msg_showed *start,*end;
		Msg_in_chat();
		void chat_refresh();
		void changeState(int,int);
		void addMsgToChat(int,int,int,int,char*);
}msg_in_chat;

int sock_fd,myuid,mid_count;
int chat_heigth,chat_width;
pthread_mutex_t mutex,chat_mutex;
WINDOW *online,*chat,*input;
Online_user_list online_user_list;

void signal_handler(int signum){
	puts("catched signal SIGPIPE!");
	return;
}

int main(int argc,char** argv){
	signal(SIGPIPE,signal_handler);
	char s[1024];
	mid_count=1;
	init_pair(0,7,4);
	init_pair(1,7,2);
	init_pair(2,7,3);
	init_pair(3,7,1);

	//连接服务端
	char addr[]="127.0.0.1";
	Socket msg_socket(1);
	msg_socket.cbind(addr);
	msg_socket.connects(addr,3333);
	sock_fd=msg_socket.fd;
	//登录或注册
	signin_or_signup();
	//创建界面
	initscr();	
	start_color();
	stdscr_init();
	//创建接收消息的线程
	pthread_t tid;
	pthread_create(&tid,NULL,recv_msg_thread,NULL);
	//处理用户输出
	handle_user_input();
	
	msg_socket.sclose();
	pthread_join(tid,NULL);
	getch();
	delwin(chat);
	delwin(input);
	delwin(online);
	endwin();
	return 0;
}

void handle_user_input(){
	wclear(chat);
	wrefresh(chat);
	wclear(input);
	wrefresh(input);
	int n;
	myprotocol tmp;
	char s[1024],s1[1024],s2[1024],*k=NULL;
	init_pair(4,0,4);
	init_pair(3,7,1);
	while(1){
		wmove(input,0,0);
		wattron(input,COLOR_PAIR(4));
		waddstr(input,"-->");
		wattroff(input,COLOR_PAIR(4));
		wrefresh(input);
		memset(s,0,1024);
		wgetstr(input,s);
		if(strcmp(s,"exit")==0 || strcmp(s,"log out")==0){
			tmp.msgtype=LOG_OUT;
			tmp.msg_body_length=0; tmp.suid=myuid;
			write(sock_fd,(char*)&tmp,sizeof(myprotocol));
			delwin(chat);
			delwin(input);
			delwin(online);
			endwin();
			close(sock_fd);
			exit(0);
		}
		n=0;memset(s1,0,1024);memset(s2,0,1024);k=NULL;
		sscanf(s,"%s%s",s1,s2);
		sscanf(s2,"%d",&n);
		k=strstr(strstr(s,s1)+strlen(s1),s2)+strlen(s2)+1;
		if(strcmp(s1,"msg")==0){
			if(in_online_list(n)){
				if(strlen(k)==0){
						wattron(input,COLOR_PAIR(3));
						wprintw(input,"Error: Length of msg must bigger than zero!");
						wattroff(input,COLOR_PAIR(3)); wgetch(input);
						wrefresh(input);
				}
				else{
					tmp.duid=n; tmp.suid=myuid;
					tmp.msgid=mid_count; mid_count++;
					tmp.msgtype=MSG_TO_USR;
					tmp.msg_body_length=strlen(k);
					write(sock_fd,(char*)&tmp,sizeof(myprotocol));
					write(sock_fd,k,strlen(k));
					char *t=new char[strlen(k)+1];
					strcpy(t,k);
					msg_in_chat.addMsgToChat(n,0,tmp.msgid,2,t);
				}
			}
			else{
				wattron(input,COLOR_PAIR(3));
				wprintw(input,"Error: <uid:%2d> isn't in the online list!",n);
				wattroff(input,COLOR_PAIR(3)); wgetch(input);
				wrefresh(input);
			}
		}
		else if(strcmp(s1,"file")==0){;}
		else{
			wattron(input,COLOR_PAIR(3));
			waddstr(input,"Error: unkown command!");
			wattroff(input,COLOR_PAIR(3)); wgetch(input);
			wrefresh(input);
		}
		wclear(input);
		wrefresh(input);
	}
}


void stdscr_init(){
	attron(A_REVERSE);
	box(stdscr,0,0);
	mvvline(1,COLS/4-1,'|',LINES-2);
	mvhline(LINES/10*8,COLS/4,'-',COLS-COLS/4-1);
	attroff(A_REVERSE);
	refresh();
	online=newwin(LINES-2,COLS/4-2,1,1);
	wrefresh(online);
	chat_heigth=LINES/10*8-1;
	chat_width=COLS-COLS/4-2;
	chat=newwin(chat_heigth,chat_width,1,COLS/4);
	wrefresh(chat);
	input=newwin(LINES-LINES/10*8-2,COLS-COLS/4-2,LINES/10*8+1,COLS/4);
	wrefresh(input);
}

void* recv_msg_thread(void* args){
	char s[1024],*s1;
	int n,m=0; char k[8],*t,w[64]; s1=s;
	myprotocol tmp;
	while(1){
		memset(&tmp,0,sizeof(tmp));
		n=read(sock_fd,(char*)&tmp,sizeof(tmp));
		memset(s,0,1024);
		if(tmp.msg_body_length)
			n=read(sock_fd,s,tmp.msg_body_length);
		if((n==-1 && errno==ECONNRESET) | n==0){
			endwin();
			puts("can't find the server!");
			exit(-1);
		}
		switch(tmp.msgtype){
			case ONLINE_USERS:
				while(1){
					if(strlen(s)==0) break;
					t=new char[64];
					sscanf(s1,"%d%s",&m,t);
					add_usr_to_onlinelist(m,t);
					sscanf(s1,"%s%s",k,w);
					if((strlen(k)+strlen(w)+2)==strlen(s1)) break;
					else s1=s1+strlen(k)+strlen(w)+2;
				}
				online_window_refresh();
				break;
			case ONLINE:
				t=new char[64];
				strcpy(t,s);
				add_usr_to_onlinelist(tmp.suid,t);
				online_window_refresh();
				break;
			case OFFLINE:
				del_usr_from_onlinelist(tmp.suid);
				online_window_refresh();
				msg_in_chat.changeState(0,tmp.suid);
				break;
			case ACK_YES:
				Msg_showed *mtmp;
				mtmp=NULL;
				for(mtmp=msg_in_chat.start;mtmp!=NULL;mtmp=mtmp->next){
					if(mtmp->msgid==tmp.msgid && mtmp->uid==tmp.suid && mtmp->type<2){
						mtmp->state=1;
						break;
					}
				}
				msg_in_chat.chat_refresh();
				online_window_refresh();
				msg_in_chat.chat_refresh();
				online_window_refresh();
				break;
			case MSG_TO_USR:
				myprotocol tmp1;
				memset(&tmp1,0,sizeof(tmp1));
				t=new char[strlen(s)+1];
				strcpy(t,s);
				msg_in_chat.addMsgToChat(tmp.suid,2,tmp.msgid,1,t);
				tmp1.msgtype=ACK_YES;
				tmp1.msgid=tmp.msgid;
				tmp1.suid=myuid;
				tmp1.duid=tmp.suid;
				tmp1.msg_body_length=0;
				write(sock_fd,(char*)&tmp1,sizeof(tmp1));
				break;
			case FILE_TO_USR:
				break;
			default:
				endwin();
				puts("unkown msgtype from server! Process has ended!");
				exit(-1);
				break;
		}
	}
}

void signin_or_signup(){
	char a,name[32],passwd[32],s[1024];
	myprotocol msg_header;
	puts("welcome to ChatMini");
	while(1){
		printf("1.sign in\n2.sign up\n3.exit\n");
loop:
		a=mygetch();
		if(a=='3') exit(0);
		if(a=='1') msg_header.msgtype=SIGN_IN;
		else if(a=='2') msg_header.msgtype=SIGN_UP;
		else goto loop;
		printf("username:"); scanf("%s",name);
		printf("passwd:"); scanf("%s",passwd);
		memset(s,0,1024);
		sprintf(s,"%s %s",name,passwd);
		puts(s);
		msg_header.msg_body_length=strlen(s);
		write(sock_fd,(char*)&msg_header,sizeof(myprotocol));
		write(sock_fd,s,strlen(s));
		read(sock_fd,(char*)&msg_header,sizeof(myprotocol));
		if(msg_header.msgtype==LOG_SUCCESS){
			myuid=msg_header.duid; return;
		}
		if(msg_header.msgtype==USER_EXIST) puts("username exist!");
		else puts("username or password is wrong!");
	}
}

char mygetch(){
	char c;
	system("stty -echo");
	system("stty -icanon");
	c=getchar();
	system("stty icanon");
	system("stty echo");
	return c;
}

int add_usr_to_onlinelist(int uid,char* name){
	pthread_mutex_lock(&mutex);
	for(int i=0;i<MAX_ONELINE_USERS;i++)
		if(online_user_list.index[i]==0){
			online_user_list.index[i]=uid;
			online_user_list.online_user[i].username=name;
			pthread_mutex_unlock(&mutex);
			return 0;
		}
	pthread_mutex_unlock(&mutex);
	return -1;
}

void del_usr_from_onlinelist(int uid){
	pthread_mutex_lock(&mutex);
	for(int i=0;i<MAX_ONELINE_USERS;i++)
		if(online_user_list.index[i]==uid){
			online_user_list.index[i]=0;
			delete [] online_user_list.online_user[i].username;
		}
	pthread_mutex_unlock(&mutex);
}

void online_window_refresh(){
	wclear(online);
	init_pair(1,7,2);
	pthread_mutex_lock(&mutex);
	for(int i=0;i<MAX_ONELINE_USERS;i++)
		if(online_user_list.index[i]!=0){
			wattron(online,COLOR_PAIR(1));
			waddstr(online,"->");
			wattroff(online,COLOR_PAIR(1));
			wprintw(online,"uid:%2d username: %s\n",online_user_list.index[i],online_user_list.online_user[i].username);
		}
	pthread_mutex_unlock(&mutex);
	wrefresh(online);
}

Msg_in_chat::Msg_in_chat(){
	start=NULL;end=NULL;line=0;lines_used=0;
}

void Msg_in_chat::chat_refresh(){
	Msg_showed *tmp;
	wclear(chat);
	init_pair(1,7,2);
	init_pair(2,7,3);
	init_pair(3,7,1);
	pthread_mutex_lock(&chat_mutex);
	if(start==NULL){
		pthread_mutex_unlock(&chat_mutex);
		return;
	}
	tmp=start;
	if(line!=0){
		if(tmp->type==0) wprintw(chat,"%s\n",((tmp->s)+chat_width*line-16));
		else if(tmp->type==1) wprintw(chat,"%s\n",((tmp->s)+chat_width*line-17));
		else if(tmp->type==2) wprintw(chat,"%s\n",((tmp->s)+chat_width*line-18));
		else wprintw(chat,"%s\n",((tmp->s)+chat_width*line-19));
		tmp=start->next;
	}
	for(;tmp!=NULL;tmp=tmp->next){
		int i=tmp->state;
		if(i==1) wattron(chat,COLOR_PAIR(1));
		else if(i==2) wattron(chat,COLOR_PAIR(2));
		else wattron(chat,COLOR_PAIR(3));
		if(tmp->type==0) wprintw(chat,"Msg to <uid:%2d>",tmp->uid);
		else if(tmp->type==1) wprintw(chat,"File to <uid:%2d>",tmp->uid);
		else if(tmp->type==2) wprintw(chat,"Msg from <uid:%2d>",tmp->uid);
		else wprintw(chat,"File from <uid:%2d>",tmp->uid);
		wattrset(chat,A_NORMAL);
		wprintw(chat," %s\n",tmp->s);
	}	
	pthread_mutex_unlock(&chat_mutex);
	wrefresh(chat);
}

void Msg_in_chat::changeState(int mid,int uid){
	Msg_showed *tmp;
	pthread_mutex_lock(&chat_mutex);
	if(mid==0){
		for(tmp=start;tmp!=NULL;tmp=tmp->next)
			if(tmp->uid==uid && tmp->state==2) tmp->state=3;
	}
	pthread_mutex_unlock(&chat_mutex);
	chat_refresh();
}

void Msg_in_chat::addMsgToChat(int uid,int type,int mid,int state,char* s){
	Msg_showed *tmp;
	int length=0,l=0,j;
	if(type==0) length+=16;
	else if(type==1) length+=17;
	else if(type==2) length+=18; 
	else length+=19;
	length+=strlen(s);
	pthread_mutex_lock(&chat_mutex);
	if(length%chat_width==0) l=length/chat_width;
	else l=length/chat_width+1;
	tmp=new Msg_showed;
	tmp->uid=uid;tmp->type=type;tmp->lines=l;
	tmp->msgid=mid;tmp->state=state;tmp->s=s;tmp->next=NULL;
	if(start==NULL){
		start=tmp;end=tmp;
		if(l>chat_heigth){ lines_used=chat_heigth; line=l-chat_heigth; }
		else{ line=0; lines_used=l; } 
		pthread_mutex_unlock(&chat_mutex);
		chat_refresh(); return;
	}	
	end->next=tmp;
	end=tmp;
	if((lines_used+l)<=chat_heigth) lines_used+=l;
	else{
		j=lines_used+l-chat_heigth;
		for(tmp=start;tmp!=NULL;tmp=tmp->next){
			if(j==0) break;
			if(j<(tmp->lines-line)){
				line+=1;
				break;
			}
			else{
				j=j-(tmp->lines-line);
				line=0;
			}
		}
		start=tmp;
	}
	pthread_mutex_unlock(&chat_mutex);
	chat_refresh();
}

int in_online_list(int id){
	if(id==0) return 0;
	pthread_mutex_lock(&mutex);
	for(int i=0;i<MAX_ONELINE_USERS;i++)
		if(online_user_list.index[i]==id){
			pthread_mutex_unlock(&mutex);
			return 1;
		}
	pthread_mutex_unlock(&mutex);
	return 0;
}
