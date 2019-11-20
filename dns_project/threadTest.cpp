#include"stafx.h"
//带中继测试1
void threadTest(int p,sockaddr_in dnsServer){
	int sock = (int)p;

	SOCKADDR_IN thAddr;
	thAddr.sin_port = htons(sock);
	thAddr.sin_family = AF_INET;
	thAddr.sin_addr.S_un.S_addr = htonl(INADDR_ANY);

	//创建用于中继查询的端口
	SOCKET tfdSock = socket(AF_INET, SOCK_DGRAM, 0);
	bool  createSock = true;
	if (tfdSock == INVALID_SOCKET && EN_DEBUG)
	{
		std::cout << "thread create socket failed" << std::endl;
		createSock = false;
	}

	char* localsendBuffer = new char[BUFFER_SIZE];
	char* localrecvBuffer = new char[BUFFER_SIZE];
	//中继查询缓冲区


	//绑定中继端口
	int bindRes = bind(tfdSock, (SOCKADDR*)& thAddr, sizeof(SOCKADDR));
	if (bindRes == SOCKET_ERROR && EN_DEBUG)
	{
		std::cout << "tread bind socket failed" << std::endl;
		createSock = false;
	}

	//创建数据库链接
	MYSQL* db = mysql_init((MYSQL*)0);
	bool connectSQL = mysql_real_connect(db, host, user, password, dbname, port, NULL, 0);

	while (createSock && connectSQL)
	{
		DNSSeg* dnsSeg = NULL;
		//停等任务消息

		WaitForSingleObject(queueFull, INFINITE);
		//等待queueFull信号
		int waitResult = WaitForSingleObject(queueMutex, 2000);
		if (waitResult == WAIT_OBJECT_0)//等待queueMutex信号 
		{
			dnsSeg = taskQueue.front();
			taskQueue.pop();
			//从任务队列获取任务
			ReleaseSemaphore(queueMutex, 1, NULL);
			//释放queueMutex信号
		}
		else
		{
			if (EN_DEBUG == 1)
			{
				std::cout << "wait for mutex failed" << std::endl;
			}
		}
		ReleaseSemaphore(queueEmpty, 1, NULL);
		//释放queueEmpty信号
		if (dnsSeg != NULL)
		{
			bool queryResult = false;
			bool relayRes = false;
			bool resGetIP = false;
			bool send = true;
			for (int i = 0; i < dnsSeg->queNum; i++)
			{
				std::string domain = "", IP = "";
				if (dnsSeg->addrQue[i]->type == 0x0001 || dnsSeg->addrQue[i]->type == 0x001c)
				{
					bool getResult = getDomain(domain, dnsSeg->addrQue[i]->addr);
					//从问题中获取域名获取查询域名,
					bool sqlIP = getIP(domain, IP);
					if (sqlIP) {
						if (((IP.length() <= 16) && (dnsSeg->addrQue[i]->type == 0x0001))
							|| ((IP.length() > 11) && (dnsSeg->addrQue[i]->type == 0x001c)))//防止IPV4地址发给IPv6报文
							resGetIP = true;
					}

					if ((!getResult) && EN_DEBUG)
						std::cout << "get domain_string failed" << std::endl;
				}

				//对于已经获取到的IP生成应答内容
				if (resGetIP) {
					dnsSeg->addrAns[dnsSeg->ansNum] = new AddrAns;
					dnsSeg->addrAns[dnsSeg->ansNum]->aclass = dnsSeg->addrQue[i]->qclass;
					//偏移量为具体偏移最高两位|c000
					dnsSeg->addrAns[dnsSeg->ansNum]->name = dnsSeg->addrQue[i]->begin | 0xc000;
					dnsSeg->addrAns[dnsSeg->ansNum]->type = dnsSeg->addrQue[i]->type;
					dnsSeg->addrAns[dnsSeg->ansNum]->live = 0x0000012c;
					if (dnsSeg->addrQue[i]->type == 0x0001) {//ipv4
						dnsSeg->addrAns[dnsSeg->ansNum]->len = 0x0004;
						dnsSeg->addrAns[dnsSeg->ansNum]->ip = new char[4];
						inet_pton(AF_INET, IP.c_str(), dnsSeg->addrAns[dnsSeg->ansNum]->ip);
					}
					if (dnsSeg->addrQue[i]->type == 0x001c) {//ipv6
						dnsSeg->addrAns[dnsSeg->ansNum]->len = 0x0010;
						dnsSeg->addrAns[dnsSeg->ansNum]->ip = new char[16];
						inet_pton(AF_INET6, IP.c_str(), dnsSeg->addrAns[dnsSeg->ansNum]->ip);
					}
					dnsSeg->ansNum++;
					send = true;
				}//完成目标地址的转换和写入
			}//处理过程

			//查询成功时生成报文
			if (resGetIP)
			{
				if (!genResponse(dnsSeg))
					send = false;
			}

			//对于没有查询结果的部分使用中继功能
			if (resGetIP == false)
			{
				if (relayRes = relaySeg(dnsSeg, tfdSock, localsendBuffer, localrecvBuffer, dnsServer))
					send=true;
				else
					send = false;
			}
			//中继,查找成功时置resGetIP为true，转发整个报文并将最终结果进行转发
			if (send)//对与生成的中继报文直接发送收到的消息
			{
						WaitForSingleObject(sendMutex, INFINITE);//等待控制信号量
						//从dnsSeg.souce进行发送
						//sendto(FDSOCK, dnsSeg->source, dnsSeg->len, 0, (SOCKADDR*)& dnsSeg->clientAddr, sizeof(SOCKADDR));
						//发送信息
						for (int i = 0; i < dnsSeg->len; i++)
						{
							if (i % 16 == 0) printf("\n");
							printf("0x%.2x ", (unsigned char)dnsSeg->source[i]);
						}
						ReleaseSemaphore(sendMutex, 1, NULL);//释放相应信号量
			}
			else {
					if (EN_DEBUG) std::cout << "get IP error" << std::endl;
				}
			delDNS(dnsSeg);
			//释放dnsSeg
		}
		else {
			if (EN_DEBUG)
				std::cout << "thread func get DNSSeg Error " << std::endl;
		}
	}
	if (createSock == false && EN_DEBUG)
		std::cout << "thread create Sock failed" << std::endl;
	if (connectSQL == false && EN_DEBUG)
		std::cout << " thread connect SQL error" << mysql_error(db) << std::endl;
	if (EN_DEBUG) {
		std::cout << "thread end" << std::endl;
	}

	//结束后处理
	if (connectSQL)
		mysql_close(db);
	return;
}

