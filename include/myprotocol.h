#ifndef MY_PROTOCOL
#define MY_PROTOCOL

typedef enum MSGTYPE{
	NONE,
	ONLINE,
	OFFLINE,
	ACK_YES,
	SIGN_IN,
	SIGN_UP,
	LOG_OUT,
	MSG_TO_USR,
	FILE_TO_USR,
	ONLINE_USERS,
	USER_EXIST,
	LOG_FAILED,
	LOG_SUCCESS
}MSGTYPE;

typedef struct myprotocol{
	MSGTYPE msgtype;
	int msg_body_length;
	int suid;
	int duid;
	int msgid;

	//用在文件传输中的头部信息
	int file_size;
	int file_name_length;
}myprotocol;

#endif
