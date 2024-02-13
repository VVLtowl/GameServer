#pragma once
#include "NetworkCommon.h"
#include <unordered_map>
#include <vector>

class Server:
	public NetworkUnit
{
public:
	void Start();//UDP only need this
	void Bind();
	void Listen();
	void Accept();
	int SRecvFromC(char* msgBuf);//UDP, return clientID

private:
	int m_EnableID;
	int GetEnableClientID();

public:
	int m_Port = DEFAULT_PORT;
	HOSTENT* m_HostEnt = nullptr;

	//UDP
	SOCKET m_UDPSocket;
	std::vector<SOCKADDR_IN> m_UDPAddrs;
	
	//TCP todo: repair
	SOCKET m_TCPListenSocket;
	std::unordered_map<int, SOCKET> m_TCPSockets;

	//todo how to provide clientID to clients
};

