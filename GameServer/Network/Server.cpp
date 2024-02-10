#include "Server.h"

void Server::Start()
{
	//launch
	Launch();

	if (!m_IsStart)
	{
		return;
	}

	//set HostEnt by this pc
	char hostName[256];
	gethostname(hostName, (int)sizeof(hostName));
	m_HostEnt = gethostbyname(hostName);

	//set IP by host
	IN_ADDR inaddr;
	memcpy(&inaddr, m_HostEnt->h_addr_list[0], 4);
	m_IP = std::string(inet_ntoa(inaddr));

	//clear id
	m_EnableID = 0;

	//TCP==========================================================
	//create TCPListenSocket
	m_TCPListenSocket = socket(AF_INET, SOCK_STREAM, 0);
	assert(m_TCPListenSocket > 0, "create TCPListenSocket error");
	u_long mode = 1; // 将 mode 设置为非零表示启用非阻塞模式
	ioctlsocket(m_TCPListenSocket, FIONBIO, &mode);
	std::cout << "create TCP socket\n";

	//create TCPSocket wait for connect
	m_TCPSockets.clear();

	//UDP==========================================================
	//create UDPSocket
	m_UDPSocket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	assert(m_UDPSocket > 0, "create UDPSocket error");
	ioctlsocket(m_UDPSocket, FIONBIO, &mode);
	std::cout << "create UDP socket\n";

	m_UDPAddrs.clear();
}

void Server::Bind()
{
	if (!m_IsStart)
	{
		return;
	}

	SOCKADDR_IN socketAddr;
	ZeroMemory(&socketAddr, LEN_ADDRIN);
	socketAddr.sin_family = AF_INET;
	socketAddr.sin_port = htons(m_Port);
	socketAddr.sin_addr.s_addr = htonl(INADDR_ANY);

	//TCP==========================================================
	//TCPListenSocket bind
	{
		int ret = bind(m_TCPListenSocket, (LPSOCKADDR)&socketAddr, LEN_ADDRIN);
		if (ret == SOCKET_ERROR)
			//failed
		{
			return;
		}
		else
			//success
		{
			std::cout << "bind TCP socket\n";
		}
	}

	//UDP==========================================================
	//UDPSocket bind
	{
		socketAddr.sin_port = htons(m_Port + OFFSET_UDP_PORT);
		int ret = bind(m_UDPSocket, (LPSOCKADDR)&socketAddr, LEN_ADDRIN);
		if (ret == SOCKET_ERROR)
			//failed
		{
			return;
		}
		else
			//success
		{
			std::cout << "bind UDP socket\n";
		}
	}
}

void Server::Listen()
{
	if (!m_IsStart)
	{
		return;
	}

	//TCPListenSocket start listen
	int ret = listen(m_TCPListenSocket, 0);
	if (ret == SOCKET_ERROR)
		//failed
	{
		return;
	}
	else
		//success
	{
		std::cout << "TCP socket listen...\n";
	}
}

void Server::Accept()
{
	if (!m_IsStart)
	{
		return;
	}

	//check can accept
	if (m_TCPSockets.size() >= MAX_CLIENT_NUM)
	{
		return;
	}

	//accept by TCPListenSocket and set up TCPSocket
	int clientID = GetEnableClientID();
	int fromLen = sizeof(SOCKADDR);
	SOCKET tempSock;
	SOCKADDR_IN tempAddr;
	tempSock = accept(m_TCPListenSocket, (SOCKADDR*)&tempAddr, &fromLen);
	if (tempSock == INVALID_SOCKET)
		//fail
	{
		return;
	}
	else
		//success
	{
		m_TCPSockets.emplace(clientID, tempSock);//copy val
		m_UDPAddrs.emplace(clientID, tempAddr);//copy val
		std::cout << "accept TCP client socket\n";
		return;
	}
}

void Server::SRecvFromC(char* msgBuf)
{
	int clientID = GetEnableClientID();
	SOCKADDR_IN tempAddr; 
	if (RecvFrom(&(m_UDPSocket), msgBuf, &tempAddr) == 0)
	{
		std::cout << "wait, enable ID: " <<clientID <<std::endl;
		return;
	}
	m_UDPAddrs.emplace(clientID, tempAddr);
	m_EnableID++;

	//show client ip
	char ipBuffer[LEN_MSG]; // 定义用于存储 IP 地址字符串的缓冲区
	inet_ntop(AF_INET, &(tempAddr.sin_addr), ipBuffer, INET_ADDRSTRLEN); // 将二进制 IP 地址转换为字符串形式
	std::cout <<"["<< ipBuffer <<"]: " << msgBuf << std::endl;
}

int Server::GetEnableClientID()
{
	return m_EnableID;
}