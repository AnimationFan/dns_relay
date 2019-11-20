#pragma once
//author is zhuoran 
//github. accout:AnimationFan
#include <iostream>
#include<fstream>
#include<WinSock2.h>
#include<WS2tcpip.h>
#pragma comment(lib,"ws2_32.lib")
#include<thread>
#include<queue>//使用标准队列文件完成队列操作
#include<Windows.h>
#include"string"
#include"mysql.h";

//常量 部分
#define DNS_PORT 53
#define POOL_SIZE 1
#define QUEUE_SIZE 8
#define BUFFER_SIZE 1024
#define EN_DEBUG 1
#define SQL_PARAM_SIZE 30
#define TTL 300



//。。。定义线程转发使用的起始端口
#define THREAD_SOCK 8200
struct DNSFlag
{//在报文中总长为两个字节，这里直接用char表示
	char QR;//0bit，查询相应标志，0查询，1响应
	char OPcode;//1-4bit，0表示标准查询，1表示反向查询，2表示服务器状态请求
	char AA;//5bit，表示授权回答
	char TC;//6bit，表示可截断的
	char RD;//7bit，表示期望递归
	char RA;//8bit，表示可用递归
	char rcode;//12-15bit,表示返回码，三位，0无错，3名字错误，2服务器错误
	//只有最后的响应报文才使用，其余都不用
};

struct AddrQue
{
	char* addr;//使用报文内容填充
	unsigned short begin;
	unsigned short len;
	unsigned  short type;
	unsigned  short qclass;
};
struct AddrAns {//限制了ans的使用范围，仅仅只能用于ipv4地址和ipv6地址的查询
	char* ip;
	unsigned short name;//用偏移表示
	unsigned short type;//
	unsigned short aclass;
	unsigned int live;
	unsigned short len;
};
struct DNSSeg
{
	SOCKADDR_IN clientAddr;
	unsigned short id;//前两个字节用以保存会话ID
	unsigned short flag;//flag的字节编码
	DNSFlag cflag;//编码后的flag
	unsigned short queNum;//问题数
	unsigned short ansNum;//回答数
	unsigned short autNum;//授权资源记录数
	unsigned short addNum;//附件资源记录数
	AddrQue** addrQue;//目标形式为数组，数组元素结尾为0
	AddrAns** addrAns;
	char* source;
	unsigned short len;
	bool mid;//中继标志
};



#pragma once
//global variable 
extern std::queue<DNSSeg*> taskQueue;
extern HANDLE queueMutex;
extern HANDLE queueEmpty;
extern HANDLE queueFull;	
extern HANDLE sendMutex;//主要工作线程发送响应报文时控制端口使用
extern HANDLE SQLMutex;//数据库链接互斥使用

extern SOCKET FDSOCK;//使用的socket描述符
extern char host[SQL_PARAM_SIZE];
extern char user[SQL_PARAM_SIZE];
extern char password[SQL_PARAM_SIZE];
extern char dbname[SQL_PARAM_SIZE];
extern int port;
extern sockaddr_in dnsServer;
extern MYSQL* db;
extern bool connectSQL;

//connect
SOCKET connect();

//processer
void processor(SOCKET fdSock);
void recvDNS(char* buffer, int bufferSize, DNSSeg& dnsSeg);
void transFlag(short flag, DNSFlag& cflag);
void assignWork(DNSSeg* dnsSeg);
bool createThreadPool();
bool initSemaphere();
bool init();
DWORD WINAPI ThreadFunc(LPVOID p);
bool getDomain(std::string& s, char* cstr);
bool getIP(std::string& domain, std::string& IP);//SQL中完成IP查询
bool relaySeg(DNSSeg* dnsSeg, SOCKET fdSock, char* localsendBuffer, char* localrecvBuffer, sockaddr_in localDNS);
bool genResponse(DNSSeg* dnsSeg);//生成目标报文
bool  delDNS(DNSSeg*  dnsSeg);
//test.cpp
void threadTest(int p, sockaddr_in dnsServer);