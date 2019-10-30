#include"stafx.h"
SOCKET FDSOCK;
SOCKET connect() {
	WSADATA wsaData;
	int nRet;
	if ((nRet = WSAStartup(MAKEWORD(2, 2), &wsaData)) != 0) {
		std::cout<<"WSAStartup failed"<<std::endl;
		return -1;
	}


	SOCKADDR_IN  serAddr;
	SOCKET fdSock = socket(AF_INET, SOCK_DGRAM, 0);
	//使用ｉｐｖ４；使用数据报；使用ＵＤＰ协议
	if (fdSock==INVALID_SOCKET&&EN_DEBUG)
	{
		std::cout << "create socket failed"<< WSAGetLastError() <<std::endl;
	}

	serAddr.sin_port = htons(DNS_PORT);
	serAddr.sin_family = AF_INET;
	serAddr.sin_addr.S_un.S_addr = htonl(INADDR_ANY);//地址设定接收所有的包
	int bindRes = bind(fdSock,(SOCKADDR*)&serAddr,sizeof(SOCKADDR));
	if (bindRes == SOCKET_ERROR)
	{
		std::cout << "bind failed" << std::endl;
		return -1;
	}
	//更新全局变量
	FDSOCK = fdSock;
	return fdSock;
}