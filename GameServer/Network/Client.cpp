#include "Client.h"

void Client::StartUDP()
{
	//launch
	Launch();

	if (!m_IsStart)
	{
		return;
	}

	//create udp socket
	u_long mode = 1; // 将 mode 设置为非零表示启用非阻塞模式
	m_UDPSocket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	assert(m_UDPSocket > 0, "create UDPSocket error");
	ioctlsocket(m_UDPSocket, FIONBIO, &mode);
	std::cout << "create udp socket\n";

	//set HostEnt by ip
	m_ServerHostEnt =
		gethostbyname(m_ServerIP.c_str());

	//prepare for connect server (need server socket address)
	ZeroMemory(&m_ServerAddr, LEN_ADDRIN);
	m_ServerAddr.sin_family = AF_INET;
	m_ServerAddr.sin_port = htons(m_ServerPort);
	m_ServerAddr.sin_addr.s_addr = *((unsigned long*)m_ServerHostEnt->h_addr);
	std::cout << "create server socket address\n";
}

void Client::TryConnectServer()
{
	if (!m_IsStart)
	{
		return;
	}

	//set HostEnt by ip
	m_ServerHostEnt =
		gethostbyname(m_ServerIP.c_str());

	//prepare for connect server (need server socket address)
	ZeroMemory(&m_ServerAddr, LEN_ADDRIN);
	m_ServerAddr.sin_family = AF_INET;
	m_ServerAddr.sin_port = htons(m_ServerPort);
	m_ServerAddr.sin_addr.s_addr = *((unsigned long*)m_ServerHostEnt->h_addr);

	//create tcp socket
	u_long mode = 1; // 将 mode 设置为非零表示启用非阻塞模式
	m_TCPSocket = socket(AF_INET, SOCK_STREAM, 0);
	ioctlsocket(m_TCPSocket, FIONBIO, &mode);
	assert(m_TCPSocket > 0, "create TCPSocket error");

	int ret = connect(m_TCPSocket, (SOCKADDR*)&m_ServerAddr, LEN_ADDRIN);
	if (ret < 0)
		//fail
	{
	}
	else
		//success
	{
		//todo
		//WSAAsyncSelect
		std::cout << "connect to server\n";
	}
} 

bool Client::CheckConnect()
{
	if (!m_IsStart)
	{
		return false;
	}

	//check is connect successfully
	int addrLen = sizeof(m_ServerAddr);
	int ret = getpeername(m_TCPSocket, (SOCKADDR*)&m_ServerAddr, &addrLen);
	if (ret < 0)
		//fail
	{
		closesocket(m_TCPSocket);
		return false;
	}
	else
		//success
	{
		return true;
	}
}
