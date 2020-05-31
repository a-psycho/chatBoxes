#ifndef ONLINE_USER
#define ONLINE_USER
//在线成员
#define MAX_ONELINE_USERS 32

struct Online_user{
	int sock_fd;
	char* username;
};
struct Online_user_list{
	int index[MAX_ONELINE_USERS]={0,};
	Online_user online_user[MAX_ONELINE_USERS];
};

#endif
