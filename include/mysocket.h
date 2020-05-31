#ifndef CIRCLE_H
#define CIRCLE_H

class Addr{
	public:
		short family;
		unsigned short port;
		unsigned addr;
		char zero[8];
		
		Addr(char*,int);
		unsigned getIp();
		char* getIpStr();
};

class Socket{
	public:
		int fd,type,addrlen;
		Addr *bindAddr;
		char acceptAddr[16];

		Socket();
		Socket(int);
		void sbind(char*,int);
		void cbind(char*);
		void slisten(int);
		int saccept();
		void connects(char*,int);
		int sread(char *,int);
		int swrite(char*);
		int sclose();
		void printAddr();
};

#endif
