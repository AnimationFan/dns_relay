#include"stafx.h"
//���м̲���1
void threadTest(int p,sockaddr_in dnsServer){
	int sock = (int)p;

	SOCKADDR_IN thAddr;
	thAddr.sin_port = htons(sock);
	thAddr.sin_family = AF_INET;
	thAddr.sin_addr.S_un.S_addr = htonl(INADDR_ANY);

	//���������м̲�ѯ�Ķ˿�
	SOCKET tfdSock = socket(AF_INET, SOCK_DGRAM, 0);
	bool  createSock = true;
	if (tfdSock == INVALID_SOCKET && EN_DEBUG)
	{
		std::cout << "thread create socket failed" << std::endl;
		createSock = false;
	}

	char* localsendBuffer = new char[BUFFER_SIZE];
	char* localrecvBuffer = new char[BUFFER_SIZE];
	//�м̲�ѯ������


	//���м̶˿�
	int bindRes = bind(tfdSock, (SOCKADDR*)& thAddr, sizeof(SOCKADDR));
	if (bindRes == SOCKET_ERROR && EN_DEBUG)
	{
		std::cout << "tread bind socket failed" << std::endl;
		createSock = false;
	}

	//�������ݿ�����
	MYSQL* db = mysql_init((MYSQL*)0);
	bool connectSQL = mysql_real_connect(db, host, user, password, dbname, port, NULL, 0);

	while (createSock && connectSQL)
	{
		DNSSeg* dnsSeg = NULL;
		//ͣ��������Ϣ

		WaitForSingleObject(queueFull, INFINITE);
		//�ȴ�queueFull�ź�
		int waitResult = WaitForSingleObject(queueMutex, 2000);
		if (waitResult == WAIT_OBJECT_0)//�ȴ�queueMutex�ź� 
		{
			dnsSeg = taskQueue.front();
			taskQueue.pop();
			//��������л�ȡ����
			ReleaseSemaphore(queueMutex, 1, NULL);
			//�ͷ�queueMutex�ź�
		}
		else
		{
			if (EN_DEBUG == 1)
			{
				std::cout << "wait for mutex failed" << std::endl;
			}
		}
		ReleaseSemaphore(queueEmpty, 1, NULL);
		//�ͷ�queueEmpty�ź�
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
					//�������л�ȡ������ȡ��ѯ����,
					bool sqlIP = getIP(domain, IP);
					if (sqlIP) {
						if (((IP.length() <= 16) && (dnsSeg->addrQue[i]->type == 0x0001))
							|| ((IP.length() > 11) && (dnsSeg->addrQue[i]->type == 0x001c)))//��ֹIPV4��ַ����IPv6����
							resGetIP = true;
						
					}

					if ((!getResult) && EN_DEBUG)
						std::cout << "get domain_string failed" << std::endl;
				}

				//�����Ѿ���ȡ����IP����Ӧ������
				if (resGetIP) {
					dnsSeg->addrAns[dnsSeg->ansNum] = new AddrAns;
					dnsSeg->addrAns[dnsSeg->ansNum]->aclass = dnsSeg->addrQue[i]->qclass;
					//ƫ����Ϊ����ƫ�������λ|c000
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
				}//���Ŀ���ַ��ת����д��
			}//�������

			//��ѯ�ɹ�ʱ���ɱ���
			if (resGetIP)
			{
				if (!genResponse(dnsSeg))
					send = false;
			}

			//����û�в�ѯ����Ĳ���ʹ���м̹���
			if (resGetIP == false)
			{
				if (relayRes = relaySeg(dnsSeg, tfdSock, localsendBuffer, localrecvBuffer, dnsServer))
					send=true;
				else
					send = false;
			}
			//�м�,���ҳɹ�ʱ��resGetIPΪtrue��ת���������Ĳ������ս������ת��
			if (send)//�������ɵ��м̱���ֱ�ӷ����յ�����Ϣ
			{
						WaitForSingleObject(sendMutex, INFINITE);//�ȴ������ź���
						//��dnsSeg.souce���з���
						//sendto(FDSOCK, dnsSeg->source, dnsSeg->len, 0, (SOCKADDR*)& dnsSeg->clientAddr, sizeof(SOCKADDR));
						//������Ϣ
						for (int i = 0; i < dnsSeg->len; i++)
						{
							if (i % 16 == 0) printf("\n");
							printf("0x%.2x ", (unsigned char)dnsSeg->source[i]);
						}
						ReleaseSemaphore(sendMutex, 1, NULL);//�ͷ���Ӧ�ź���
			}
			else {
					if (EN_DEBUG) std::cout << "get IP error" << std::endl;
				}
			delDNS(dnsSeg);
			//�ͷ�dnsSeg
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

	//��������
	if (connectSQL)
		mysql_close(db);
	return;
}

