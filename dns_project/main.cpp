#include"stafx.h"
//bind绑定了的会使用固定的端口，而没有使用bind绑定的话系统会随机分配端口


int main()
{
	SOCKET fdSock = connect();
	processor(fdSock);
	if (!connectSQL)
	{
		if (EN_DEBUG)
			std::cout << "faile to connect SQL" << std::endl;
		std::cout << mysql_error(db);
		std::cout << "end" << std::endl;
		system("pause");
		return 0;
	}
	mysql_close(db);
	closesocket(fdSock);
	WSACleanup();
	return 0;
}



//int main() {
//	//初始化网络环境
//	WSADATA wsaData;
//	int nRet = WSAStartup(MAKEWORD(2,2),&wsaData);
//	if (nRet != 0)
//	{
//		std::cout << "error happened when start up" << std::endl;
//		return -1;
//	}
//	
//	//模拟 的报文内容
//	char seg[] = "\x93\x76\x01\x00\x00\x01" \
//		"\x00\x00\x00\x00\x00\x00\x03\x77\x77\x77\x04\x62\x69\x6e\x67\x03" \
//		"\x63\x6f\x6d\x00\x00\x01\x00\x01";//对www.bing.com 的询问
//	char* recvBuffer = new char[BUFFER_SIZE];
//	memcpy(recvBuffer, seg, sizeof(seg));
//	
//	init();
//
//	DNSSeg* dnsSeg = new DNSSeg;
//	initSemaphere();
//	recvDNS(recvBuffer, BUFFER_SIZE, *dnsSeg);
//	assignWork(dnsSeg);
//
//	while (true);
//	//进程后处理
//	if (connectSQL)
//		mysql_close(db);
//	
//	system("pause");
//	return 0;
//}


//genResponse()测试报文生成
//int main() {
//	//模拟 的报文内容
//	char seg[] ="\x93\x76\x01\x00\x00\x01" \
//		"\x00\x00\x00\x00\x00\x00\x03\x77\x77\x77\x04\x62\x69\x6e\x67\x03" \
//		"\x63\x6f\x6d\x00\x00\x01\x00\x01";//对www.bing.com 的询问
//
//	char* recvBuffer = new char[BUFFER_SIZE];
//	memcpy(recvBuffer, seg, sizeof(seg));
//	
//
//
//	DNSSeg* dnsSeg = new DNSSeg;
//	recvDNS(recvBuffer, BUFFER_SIZE, *dnsSeg);
//	
//	//手动生成内容部分
//	dnsSeg->ansNum = 1;
//	dnsSeg->addrAns[0] = new AddrAns;
//	dnsSeg->addrAns[0]->aclass = 0x0001;
//	dnsSeg->addrAns[0]->type = 0x0001;
//	dnsSeg->addrAns[0]->len = 4;
//	dnsSeg->addrAns[0]->name = 0xc00c;
//	dnsSeg->addrAns[0]->ip = new char[4];
//	memset(dnsSeg->addrAns[0]->ip, 0xff, 4);
//
//	genResponse(dnsSeg);
//	for (int i = 0; i < dnsSeg->len; i++) 
//	{
//		if (i % 16 == 0) printf("\n");
//		printf("0x%.2x ", (unsigned char)dnsSeg->source[i]);
//	}
//	bool  result=delDNS(dnsSeg);
//	system("pause");
//	return 0;
//}



//relaySeg测试,测试完成
//int main() {
//	//已经成功接收报文
//	SOCKET fdSock;
//	WSADATA wsaData;
//	int nRet = WSAStartup(MAKEWORD(2,2),&wsaData);
//	if (nRet != 0)
//	{
//		std::cout << "error happened when start up" << std::endl;
//		return -1;
//	}
//	sockaddr_in dnsServer,local;
//	int tport = 53;
//	dnsServer.sin_family = AF_INET;
//	dnsServer.sin_port = htons(tport);
//	inet_pton(AF_INET, "10.3.9.5",(void*)&dnsServer.sin_addr);
//
//	local.sin_family = AF_INET;
//	local.sin_addr.S_un.S_addr = INADDR_ANY;
//	local.sin_port = htons(8003);
//	fdSock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
//	int bindRes=bind(fdSock, (sockaddr*)& local, sizeof(local));
//	if (bindRes == SOCKET_ERROR)
//	{
//		std::cout << "bind failed" << std::endl;
//		return -1;
//	}
//	int dnsSerLen = sizeof(dnsServer);
//	int contentLen = 0;
//	char sendBuffer[2000];
//	char recvBuffer[2000];
//	memset(recvBuffer, 0x00, 2000);
//
//	char quesSeg[] = "\x4a\x75\x01\x00\x00\x01" \
//		"\x00\x00\x00\x00\x00\x00\x0b\x73\x74\x6f\x72\x65\x65\x64\x67\x65" \
//		"\x66\x64\x03\x64\x73\x78\x02\x6d\x70\x09\x6d\x69\x63\x72\x6f\x73" \
//		"\x6f\x66\x74\x03\x63\x6f\x6d\x00\x00\x01\x00\x01";
//	memcpy(sendBuffer, quesSeg, sizeof(quesSeg));
//
//	TIMEVAL outTime = { 2000,2000 };//设定接收和发送超时时间，单位分别为秒和微秒
//
//	setsockopt(fdSock,SOL_SOCKET,SO_RCVTIMEO,(char*)&outTime,sizeof(outTime));
//	////目标函数部分
//	//sendto(fdSock, sendBuffer, sizeof(quesSeg), 0, (SOCKADDR*)& dnsServer, sizeof(dnsServer));
//	//recvfrom(fdSock, recvBuffer,sizeof(recvBuffer),0,(sockaddr*)&dnsServer,&dnsSerLen);
//	//for (int i = 0; i < 300; i++) {
//	//	if (i % 16 == 0) printf("\n");
//	//	printf("0x%.2x  ", (unsigned char)recvBuffer[i]);
//	//}
//
//	////对响应报文进行提取
//	////限定检索范围只对含有了响应的报文为A或AA进行提取
//	//unsigned short ansNum = ((unsigned char)recvBuffer[6]<<8|(unsigned char)recvBuffer[7]);
//	//std::cout << ansNum << std::endl;
//	////指针偏移渡过问题区
//	//int j = 12;
//	//for (;;) {
//	//	if (recvBuffer[j] != 0)
//	//		j=j+recvBuffer[j]+1;
//	//	else {
//	//		j = j + 5;
//	//		break;
//	//	}
//	//}
//	////识别问题
//	//for (int i=0;i<ansNum;i++)
//	//{
//	//	unsigned short type = ((unsigned char)recvBuffer[j + 2] << 8 | (unsigned char)recvBuffer[j + 3]);
//	//	unsigned short len = ((unsigned char)recvBuffer[j + 10] | (unsigned char)recvBuffer[j + 11]);
//	//	if (type == 0x0001 || type == 0x001c)
//	//	{
//	//		//std::string ans = std::string(recvBuffer[j+12],len);
//	//		char* ip = new char[len];
//	//		memcpy(ip, &recvBuffer[j + 12], len);
//	//		system("pause");
//	//	}
//	//	else j = j + 12 + len;
//	//}
//
//	DNSSeg* dnsSeg = new DNSSeg;
//	
//	dnsSeg->len = sizeof(quesSeg);
//	dnsSeg->source = new char[dnsSeg->len];
//	memcpy(dnsSeg->source,quesSeg,dnsSeg->len);
//	if (relaySeg(dnsSeg, fdSock, sendBuffer, recvBuffer, dnsServer))
//	{
//		for (int j = 0; j < dnsSeg->len; j++)
//		{
//			if (j % 16 == 0) printf("\n");
//			printf("0x%.2x ", (unsigned char)dnsSeg->source[j]);
//		}
//	}
//
//	closesocket(fdSock);
//	WSACleanup();
//	return 0;
//}

//stringIP转charIP 
//int main() {
//	char a[4]="0";
//	std::string IP = "192.168.1.1";
//	inet_pton(AF_INET,IP.c_str(),a);
//	std::cout << "finish" << std::endl;
//	return 0;
//}

//使用MySQL查询相关IP的getIP测试
//int main() {
//	MYSQL* db=mysql_init((MYSQL*)0);
//	if ( mysql_real_connect(db, host, user, password, dbname, port, NULL, 0))
//	{
//		std::cout << "接成功" << std::endl;
//		//char question[300];
//		//sprintf_s(question,"select * from dns where domain = 'www.tencent.com'") ;
//		std::string domain = "www.baidu.com";
//		std::string IP;
//		if (getIP(domain, IP, db))
//			if(IP=="")
//			std::cout << IP << std::endl;;
//		
//		mysql_close(db);
//	}
//	else {
//		std::cout << "连接失败" << std::endl;
//		std::cout << mysql_error(db) << std::endl;
//	}
//		system("pause");
//	return 0;
//}

//int main()
//{
//
//    std::cout << "dns begin to work\n";
//	int fdScok = connect();
//	if (fdScok == INVALID_SOCKET)
//		std::cout << "connect failed\n" << std::endl;
//	else
//		processor(fdScok);
//	system("pause");
//	return 0;
//}

//格式转换测试
//int main() {
//	unsigned char a = 0;
//	unsigned char b = 0;
//	for (; a <= 255; a++)
//	{
//		for (; b <= 255; b++) {
//			if (((unsigned short)a << 8 | (unsigned short)b) != (16*16 * a + b))
//				std::cout << "error" << (16 * a + b) <<std::endl;
//			if (b == 255)
//				break;
//		}
//		if (a == 255) break;
//	}
//	std::cout << "OK" << std::endl;
//	system("pause");
//}

//getStiring 测试
//测试生成由数组地址生成对应的点地址string
//int main(){
//	char a[] = {0x02,0x68,0x6d,0x05,0x62,0x61,0x69,0x64,0x75,0x03,0x63,0x6f,0x6d,0x00,0x00,0x1c,0x00,0x01};
//	//测试地址hm.baidu.com ,type:AAAA, class:IN
//	std::string domain("aaa");
//	getDomain(domain, a);
//	std::cout << domain << std::endl;
//	system("pause");
//}

//recvDNS 测试
//int main() {
//	char a[] = { 0x72,0xa5,0x01,0x00,0x00,0x01,0x00,0x00,0x00,0x00,0x00,0x00,0x03,0x6f,0x74 ,0x66,0x03,0x06d,0x73,0x6e,0x02,0x63,0x6e,0x00,0x00,0x1c,0x00,0x01};
//	DNSSeg dnseg;
//	memset(&dnseg, 0x0, sizeof(DNSSeg));
//	recvDNS((char*)& a, sizeof(a), dnseg);
//	system("pause");
//}

