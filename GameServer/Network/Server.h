#pragma once
#include "NetworkCommon.h"
#include <unordered_map>

class Server:
	public NetworkUnit
{
public:
	void Start();//UDP only need this
	void Bind();
	void Listen();
	void Accept();
	void SRecvFromC(char* msgBuf);//UDP

private:
	int m_EnableID;
	int GetEnableClientID();

public:
	int m_Port = DEFAULT_PORT;
	HOSTENT* m_HostEnt = nullptr;

	//UDP
	SOCKET m_UDPSocket;
	std::unordered_map<int, SOCKADDR_IN> m_UDPAddrs;
	
	//TCP
	SOCKET m_TCPListenSocket;
	std::unordered_map<int, SOCKET> m_TCPSockets;

	//todo how to provide clientID to clients
};

