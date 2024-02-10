#pragma once
#include "NetworkCommon.h"

class Client:
	public NetworkUnit
{
public:
	//UDP
	void StartUDP();

	//TCP
	void TryConnectServer();
	bool CheckConnect();

public:
	int m_ClientID;
	std::string m_ServerIP = std::string(DEFAULT_IP);
	int m_ServerPort = DEFAULT_PORT;
	HOSTENT* m_ServerHostEnt = nullptr;

	//UDP
	SOCKET m_UDPSocket;
	
	//TCP
	SOCKET m_TCPSocket;
	SOCKADDR_IN m_ServerAddr;
};

