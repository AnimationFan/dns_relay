#pragma once
#include <iostream>
#include<WinSock2.h>
#include<WS2tcpip.h>
#pragma comment(lib,"ws2_32.lib")
#include<thread>
#include<queue>//ʹ�ñ�׼�����ļ���ɶ��в���
#include<Windows.h>
#include"string"
#include"mysql.h";

//���� ����
#define DNS_PORT 53
#define POOL_SIZE 4
#define QUEUE_SIZE 8
#define BUFFER_SIZE 1024
#define EN_DEBUG 1
#define SQL_PARAM_SIZE 30
#define TTL 300


//�����������߳�ת��ʹ�õ���ʼ�˿�
#define THREAD_SOCK 8200
struct DNSFlag
{//�ڱ������ܳ�Ϊ�����ֽڣ�����ֱ����char��ʾ
	char QR;//0bit����ѯ��Ӧ��־��0��ѯ��1��Ӧ
	char OPcode;//1-4bit��0��ʾ��׼��ѯ��1��ʾ�����ѯ��2��ʾ������״̬����
	char AA;//5bit����ʾ��Ȩ�ش�
	char TC;//6bit����ʾ�ɽضϵ�
	char RD;//7bit����ʾ�����ݹ�
	char RA;//8bit����ʾ���õݹ�
	char rcode;//12-15bit,��ʾ�����룬��λ��0�޴�3���ִ���2����������
	//ֻ��������Ӧ���Ĳ�ʹ�ã����඼����
};

struct AddrQue
{
	char* addr;//ʹ�ñ����������
	unsigned short begin;
	unsigned short len;
	unsigned  short type;
	unsigned  short qclass;
};
struct AddrAns {//������ans��ʹ�÷�Χ������ֻ������ipv4��ַ��ipv6��ַ�Ĳ�ѯ
	char* ip;
	unsigned short name;//��ƫ�Ʊ�ʾ
	unsigned short type;//
	unsigned short aclass;
	unsigned int live;
	unsigned short len;
};
struct DNSSeg
{
	SOCKADDR_IN clientAddr;
	unsigned short id;//ǰ�����ֽ����Ա���ỰID
	unsigned short flag;//flag���ֽڱ���
	DNSFlag cflag;//������flag
	unsigned short queNum;//������
	unsigned short ansNum;//�ش���
	unsigned short autNum;//��Ȩ��Դ��¼��
	unsigned short addNum;//������Դ��¼��
	AddrQue** addrQue;//Ŀ����ʽΪ���飬����Ԫ�ؽ�βΪ0
	AddrAns** addrAns;
	char* source;
	unsigned short len;
	bool mid;//�м̱�־
};



#pragma once
//global variable 
extern std::queue<DNSSeg*> taskQueue;
extern HANDLE queueMutex;
extern HANDLE queueEmpty;
extern HANDLE queueFull;	
extern HANDLE sendMutex;//��Ҫ�����̷߳�����Ӧ����ʱ���ƶ˿�ʹ��
extern SOCKET FDSOCK;//ʹ�õ�socket������
extern char host[SQL_PARAM_SIZE];
extern char user[SQL_PARAM_SIZE];
extern char password[SQL_PARAM_SIZE];
extern char dbname[SQL_PARAM_SIZE];
extern int port;
extern sockaddr_in dnsServer;


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
bool getIP(std::string& domain, std::string& IP, MYSQL* db);//SQL�����IP��ѯ
bool relaySeg(DNSSeg* dnsSeg, SOCKET fdSock, char* localsendBuffer, char* localrecvBuffer, sockaddr_in localDNS);
bool genResponse(DNSSeg* dnsSeg);//����Ŀ�걨��
bool  delDNS(DNSSeg*  dnsSeg);
bool writeIP(std::string& IP, DNSSeg* dnsSeg, int i);

//test.cpp
void threadTest(int p, sockaddr_in dnsServer);