#include"stafx.h"
DWORD WINAPI ThreadFunc(LPVOID);
HANDLE queueMutex = NULL;
HANDLE queueEmpty = NULL;
HANDLE queueFull = NULL;
HANDLE sendMutex = NULL;
HANDLE SQLMutex = NULL;
std::queue<DNSSeg*> taskQueue = std::queue<DNSSeg*>();
char host[SQL_PARAM_SIZE] ;
char user[SQL_PARAM_SIZE];
char password[SQL_PARAM_SIZE] ;
char dbname[SQL_PARAM_SIZE] ;
int port = 3306;
sockaddr_in dnsServer;
MYSQL* db=NULL;
bool connectSQL;



//�����޸�Ϊ�ļ���ȡ����ʽ
bool init() {
	//SQL��ز�������
	memset(host, 0x00, SQL_PARAM_SIZE);
	memcpy(host, "127.0.0.1", sizeof("107.0.0.1"));
	
	memset(user, 0x00,SQL_PARAM_SIZE);
	memcpy(user,"homework_user",sizeof("homework_user"));

	memset(password, 0x00, SQL_PARAM_SIZE);
	memcpy(password,"123456",sizeof("123456"));
	
	memset(dbname, 0x00, SQL_PARAM_SIZE);
	memcpy(dbname,"homework" , sizeof("homework"));

	char dnsip[] = "10.3.9.5";

	//dnsĿ�����������
	dnsServer.sin_family = AF_INET;
	dnsServer.sin_port = htons(DNS_PORT);
	inet_pton(AF_INET, dnsip, (void*)& dnsServer.sin_addr);


	//��ʼ��SQL������
	db = mysql_init((MYSQL*)0);
	connectSQL = mysql_real_connect(db, host, user, password, dbname, port, NULL, 0);
	//�����ʼ������
	bool resCT;
	bool resCS;
	if (connectSQL) {
		resCT = createThreadPool();
		resCS = initSemaphere();
	}
	return resCT && resCS;
}


//�����̳߳�
bool createThreadPool() {
	for (int i = 0; i < POOL_SIZE; i++)
	{
		LPDWORD threadID = NULL;
		HANDLE result=CreateThread(NULL, 0,ThreadFunc,(LPVOID)(THREAD_SOCK+i),0,threadID);
		if (result == NULL)
			return false;
	}
	return true;
}
//��ʼ���ź���
bool initSemaphere() {
	queueMutex=CreateSemaphore(NULL,1,1,NULL);
	//��ȫ��������ֵ�����ֵ,����
	queueEmpty = CreateSemaphore(NULL, 8, 8, NULL);
	queueFull = CreateSemaphore(NULL, 0, 8, NULL);
	sendMutex = CreateSemaphore(NULL, 1, 1, NULL);
	SQLMutex = CreateSemaphore(NULL, 1, 1, NULL);
	return true;
}


//�߳�ִ�к���
DWORD WINAPI ThreadFunc(LPVOID p){
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
	int bindRes = bind(tfdSock, (SOCKADDR*)&thAddr, sizeof(SOCKADDR));
	if(bindRes==SOCKET_ERROR&&EN_DEBUG)
	{
		std::cout << "tread bind socket failed" << std::endl;
		createSock = false;
	}

	
	
	
	while (createSock&&connectSQL)
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
						if(((IP.length()<=16)&&(dnsSeg->addrQue[i]->type==0x0001))
							||((IP.length()>11)&&(dnsSeg->addrQue[i]->type==0x001c)))//��ֹIPV4��ַ����IPv6����
						resGetIP = true;
					}
						
					if ((!getResult) && EN_DEBUG)
						std::cout << "get domain_string failed" << std::endl;
				}
					
				//�����Ѿ���ȡ����IP����Ӧ������
					if (resGetIP) {
						dnsSeg->addrAns[dnsSeg->ansNum] = new AddrAns;
						dnsSeg->addrAns[dnsSeg->ansNum]->aclass=dnsSeg->addrQue[i]->qclass;
						//ƫ����Ϊ����ƫ�������λ|c000
						dnsSeg->addrAns[dnsSeg->ansNum]->name = dnsSeg->addrQue[i]->begin|0xc000;
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
			
			if (send) 
			{
					WaitForSingleObject(sendMutex, INFINITE);//�ȴ������ź���
					//��dnsSeg.souce���з���
					sendto(FDSOCK, dnsSeg->source, dnsSeg->len, 0, (SOCKADDR*)& dnsSeg->clientAddr, sizeof(SOCKADDR));
					if (EN_DEBUG) 
					{
						for (int i = 0; i < dnsSeg->len; i++)
						{
							if (i % 16 == 0) printf("\n");
							printf("0x%.2x ", (unsigned char)dnsSeg->source[i]);
						}
					}
					//������Ϣ
					ReleaseSemaphore(sendMutex, 1, NULL);//�ͷ���Ӧ�ź���
			}
			else 
			{
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
	{
		WaitForSingleObject(SQLMutex, INFINITE);
		std::cout << " thread connect SQL error" << mysql_error(db) << std::endl;
		ReleaseSemaphore(SQLMutex,1,NULL);
	}
	if (EN_DEBUG) {
		std::cout << "thread end" << std::endl;
	}

	//��������
	
	return 0;
}


void assignWork(DNSSeg *dnsSeg)
{
	DWORD waitResult=WaitForSingleObject(queueEmpty, 2000);
	//�ȴ��ź�����INFINITE��ʾ����
	//�ȴ������п�λ���ȴ�ʱ��Ϊ1000ms
	if (waitResult == WAIT_OBJECT_0) {
		waitResult=WaitForSingleObject(queueMutex, 2000);
		//�ȴ�������еĿ�����
		if (waitResult == WAIT_OBJECT_0)
		{
			taskQueue.push(dnsSeg);
			ReleaseSemaphore(queueMutex,1,NULL);
			//�ͷ�������п����ź�
			ReleaseSemaphore(queueFull,1,NULL);
			//�ͷ�full�ź�
		}
		else {
			std::cout << "wait queue failed" << std::endl;
		}
	}
	else
	{
		if (EN_DEBUG == 1)
			std::cout << "wait empty  failed" << std::endl;
	}
	
}


//�����̣����ղ���������뵽������
void processor(SOCKET fdSock) {
	SOCKADDR_IN clientAddr;
	int client_len = sizeof(clientAddr);
	char buffer[BUFFER_SIZE];
	bool initRes = init();
	if (initRes == false && EN_DEBUG)
		std::cout << "init failed" << std::endl;
	while (initRes)
	{
		memset(buffer, 0x0, BUFFER_SIZE);
		int recvRes = recvfrom(fdSock, buffer, BUFFER_SIZE, 0, (SOCKADDR*)& clientAddr, &client_len);
		if (recvRes <= 0)
		{
			if (EN_DEBUG == 1)
			{
				std::cout << "error\n" << std::endl;
			}
		}
		else
		{
			DNSSeg* dnsSeg = new DNSSeg;
			memcpy(&(dnsSeg->clientAddr),&clientAddr,sizeof(clientAddr)) ;
			recvDNS(buffer,BUFFER_SIZE,*dnsSeg);//����dnsSeg
			assignWork(dnsSeg);//����dnsSeg����
		}
	}
}


void recvDNS(char* buffer,int bufferSize,DNSSeg &dnsSeg) {
	//segment ͷ�� 
	dnsSeg.id = (unsigned short)(unsigned char)buffer[0] << 8 | (unsigned short)(unsigned char)buffer[1];
	dnsSeg.flag = (unsigned short)buffer[2] << 8 | (unsigned short)buffer[3];
	dnsSeg.queNum = (unsigned short)buffer[4] << 8 | (unsigned short)buffer[5];
	dnsSeg.ansNum = (unsigned short)buffer[6] << 8 | (unsigned short)buffer[7];
	dnsSeg.autNum = (unsigned short)buffer[8] << 8 | (unsigned short)buffer[9];
	dnsSeg.addNum = (unsigned short)buffer[10] << 8 | (unsigned short)buffer[11];
	//���ɿɶ�ȡ��flag
	transFlag(dnsSeg.flag, dnsSeg.cflag);

	//��ȡѯ����Ϣ
	dnsSeg.addrQue = new AddrQue* [dnsSeg.queNum];
	unsigned short j=12;
	for(unsigned short i = 0; i < dnsSeg.queNum; i++) {
		dnsSeg.addrQue[i] = new AddrQue;
		unsigned short start = j;
		while (buffer[j]!=0)
		{
			j = j + buffer[j]+1 ;
		}
		dnsSeg.addrQue[i]->addr = new char[j-start+1];
		memcpy(dnsSeg.addrQue[i]->addr, &buffer[start], j - start + 1);
		dnsSeg.addrQue[i]->len = j-start+1;
		dnsSeg.addrQue[i]->begin = start;
		//��ȡ���ͺ�class����
		dnsSeg.addrQue[i]->type= (unsigned short)buffer[j+1] << 8 | (unsigned short)buffer[j+2];
		dnsSeg.addrQue[i]->qclass= (unsigned short)buffer[j+3] << 8 | (unsigned short)buffer[j+4];
		
		if (j + 5 < bufferSize) j+=5;
	}
	//ֱ�ӿ���������������ת������
	dnsSeg.source = new char[j];
	memcpy(dnsSeg.source, buffer, j);
	dnsSeg.len = j;
	dnsSeg.mid = false;

	dnsSeg.addrAns =new AddrAns*[dnsSeg.queNum];
	dnsSeg.ansNum = 0;

	if (EN_DEBUG)
	{
		for (int i = 0; i < j; i++) {
			if (i % 16 == 0)
				printf("\n");
			printf("0x%.2x ", dnsSeg.source[i]);
		}
	}
}

void transFlag(short flag,DNSFlag &cflag) {
	cflag.QR = (char)((unsigned  short)(flag&0x8000)>>15);
	
	cflag.OPcode= (char)((unsigned  short)(flag & 0x7800)>>11);

	cflag.AA = (char)((unsigned  short)(flag & 0x0400) >> 10);

	cflag.TC = (char)((unsigned  short)(flag & 0x0200) >> 9);

	cflag.RD= (char)((unsigned  short)(flag & 0x0100) >> 8);

	cflag.RA= (char)((unsigned  short)(flag & 0x0080) >> 7);

	cflag.rcode = (char)((unsigned  short)(flag & 0x000f) >> 0);
}

//���ձ���������ʽ
bool getDomain(std::string& s, char* cstr) {
	if (cstr == NULL) return false;
	s = "";
	int j = 0;
	while (cstr[j] != 0) {
		std::string temp = std::string(cstr, j + 1, cstr[j]);
		s = s + temp;
		j = j + cstr[j] + 1;
		if (cstr[j] != 0) s += ".";
	}
	return true;
}

//δ���ڵ�����IP�������ء���
bool getIP(std::string& domain, std::string& IP) {
	WaitForSingleObject(SQLMutex, INFINITE);
	MYSQL_RES* queryRes = NULL;
	MYSQL_ROW row;

	std::string question="select ip from dns where domain = '";
	question = question + domain;
	question = question + "'    ";
	
	int res = mysql_real_query(db, question.c_str(), (unsigned int)question.length());//��ѯ�ɹ���0
	if (res) {
		ReleaseSemaphore(SQLMutex, 1, NULL);
		return false;
	}
	else {
		queryRes = mysql_store_result(db);
		if (queryRes == NULL)
		{
			ReleaseSemaphore(SQLMutex, 1, NULL);
			return false;
		}
		else {
			if(row=mysql_fetch_row(queryRes))
			IP = std::string(row[0]);
		}
	}
	mysql_free_result(queryRes);
	if (!IP.compare(""))
	{
		ReleaseSemaphore(SQLMutex, 1, NULL);
		return false;
	}
	ReleaseSemaphore(SQLMutex, 1, NULL);
	return true;
}



bool relaySeg(DNSSeg* dnsSeg, SOCKET fdSock, char* localsendBuffer, char* localrecvBuffer, sockaddr_in localDNS) {
	if (dnsSeg->source == NULL) return false;
	memcpy(localsendBuffer, dnsSeg->source, dnsSeg->len);
	int localDNSLen = sizeof(localDNS);
	int sendRes = sendto(fdSock, localsendBuffer, dnsSeg->len, 0, (SOCKADDR*)& localDNS, sizeof(localDNS));
	if (sendRes <= 0) return false;
	int recvRes = recvfrom(fdSock, localrecvBuffer, BUFFER_SIZE, 0, (sockaddr*)& localDNS, &localDNSLen);
	if (recvRes <= 0) return false;
	if ((localrecvBuffer[2] & 0x80) < 0)
		return false;

	//�������ݱ���ƫ��
	unsigned short ansNum = ((unsigned short)localrecvBuffer[6] << 8 | (unsigned short)localrecvBuffer[7]);
	ansNum = ansNum + ((unsigned short)localrecvBuffer[8] << 8 | (unsigned short)localrecvBuffer[9]);
	ansNum = ansNum + ((unsigned short)localrecvBuffer[10] << 8 | (unsigned short)localrecvBuffer[11]);
	//ָ��ƫ�ƶɹ�������
	int j = 12;
	for (;;) {
		if (localrecvBuffer[j] != 0)
			j = j + localrecvBuffer[j] + 1;
		else {
			j = j + 5;
			break;
		}
	}
	//ʶ��Ӧ������
	for (int i = 0; i < ansNum; i++)
	{
		unsigned short len = ((unsigned short)localrecvBuffer[j + 10] | (unsigned short)localrecvBuffer[j + 11]);
		j = j + 12 + len;
	}
	//�ͷ�ԭ������ı��Ľ����滻
	if (dnsSeg->source != NULL)
		delete[] dnsSeg->source;
	dnsSeg->source = new char[j];
	memcpy(dnsSeg->source, localrecvBuffer, j);
	dnsSeg->len = j;
	dnsSeg->mid = true;
	return true;
}

bool genResponse(DNSSeg* dnsSeg) {
	if (dnsSeg == NULL) return false;
	//������Ҫ�ĳ���
	int len2 = dnsSeg->len;
	for (int i = 0; i < dnsSeg->ansNum; i++)
	{
		len2 = len2 + dnsSeg->addrAns[i]->len + 12;
	}
	char* result = new char[len2];
	if (result == NULL) return false;
	memset(result, 0x00, len2);

	//ID�ֶ�
	result[0] = (char)(dnsSeg->id >> 8);
	result[1] = (char)(dnsSeg->id);
	//flag�ֶ�
	result[2] = (char)(dnsSeg->flag >> 8);
	result[3] = (char)(dnsSeg->flag);
	result[2] = (char)(result[2] | 0x80);//����Ϊ��Ӧ����
	result[3] = (char)(result[3] & 0xf8);//�趨�����λ������ȫ0

	//��������
	result[4] = (char)(dnsSeg->queNum >> 8);
	result[5] = (char)(dnsSeg->queNum);

	//�ش�����
	result[6] = dnsSeg->ansNum >> 8;
	result[7] = dnsSeg->ansNum;

	//��Ȩ����
	result[8] = dnsSeg->autNum >> 8;
	result[9] = dnsSeg->autNum;

	//������Դ
	result[10] = dnsSeg->addNum >> 8;
	result[11] = dnsSeg->addNum;

	//��������
	int j = 12;
	for (int i = 0; i < dnsSeg->queNum; i++) {
		if (dnsSeg->addrQue[i]->addr == NULL) return false;
		memcpy(&result[j], dnsSeg->addrQue[i]->addr, dnsSeg->addrQue[i]->len);
		j += dnsSeg->addrQue[i]->len;
		result[j] = dnsSeg->addrQue[i]->type >> 8;
		result[j + 1] = dnsSeg->addrQue[i]->type;
		result[j + 2] = dnsSeg->addrQue[i]->qclass >> 8;
		result[j + 3] = dnsSeg->addrQue[i]->qclass;
		j += 4;
	}
	//���Ŵ�
	for (int i = 0; i < dnsSeg->ansNum; i++) {
		result[j] = (dnsSeg->addrAns[i]->name) >> 8;//...ƫ��
		result[j + 1] = (dnsSeg->addrAns[i]->name);

		result[j + 2] = (dnsSeg->addrAns[i]->type) >> 8;//...type
		result[j + 3] = (dnsSeg->addrAns[i]->type);

		result[j + 4] = (dnsSeg->addrAns[i]->aclass) >> 8;//...class
		result[j + 5] = (dnsSeg->addrAns[i]->aclass);

		//TTL���ж��壬��Ϊ
		result[j + 6] = (unsigned int)(TTL) >> 24;//...TTL�ĸ��ֽ�
		result[j + 7] = (unsigned int)(TTL) >> 16;
		result[j + 8] = (unsigned int)(TTL) >> 8;
		result[j + 9] = (unsigned int)(TTL);

		result[j + 10] = dnsSeg->addrAns[i]->len >> 8;//len
		result[j + 11] = dnsSeg->addrAns[i]->len;

		//...resource
		if (dnsSeg->addrAns[i]->ip == NULL) return false;
		memcpy(&result[j + 12], dnsSeg->addrAns[i]->ip, dnsSeg->addrAns[i]->len);
		j = j + 12 + dnsSeg->addrAns[i]->len;
	}
	delete[] dnsSeg->source;
	dnsSeg->source = result;
	dnsSeg->len = j;
	return true;
}

bool delDNS(DNSSeg* dnsSeg) {
	if (dnsSeg == NULL) return false;
	if (dnsSeg->source == NULL) return false;
	delete[] dnsSeg->source;

	if (dnsSeg->addrQue == NULL) return false;
	for (int i = 0; i < dnsSeg->queNum; i++)
	{
		if (dnsSeg->addrQue[i] == NULL) return false;
		if (dnsSeg->addrQue[i]->addr == NULL) return false;
		delete[] dnsSeg->addrQue[i]->addr;
		delete dnsSeg->addrQue[i];
	}
	delete[] dnsSeg->addrQue;

	if (dnsSeg->addrAns == NULL) return false;
	for (int i = 0; i < dnsSeg->ansNum; i++)
	{
		if (dnsSeg->addrAns[i] == NULL) return false;
		if (dnsSeg->addrAns[i]->ip == NULL) return false;
		delete[] dnsSeg->addrAns[i]->ip;
		delete dnsSeg->addrAns[i];
	}
	delete[] dnsSeg->addrAns;
	delete dnsSeg;
	return true;
};
