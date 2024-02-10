#pragma once

#define _WINSOCK_DEPRECATED_NO_WARNINGS
#define _CRT_SECURE_NO_WARNINGS
#define WIN32_LEAN_AND_MEAN

#include <windowsx.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>

#include <string.h>
#include <iostream>
#include <assert.h>

#pragma comment(lib, "winmm.lib")
#pragma comment(lib, "ws2_32.lib")

//for limit tcp sock num
const size_t MAX_CLIENT_NUM = 100;

//net package definition
const size_t LEN_MSG = 1024;

//network object default value 
const size_t LEN_IP = 16;//000.000.000.000x
const char DEFAULT_IP[LEN_IP] = "127.0.0.1";
const int DEFAULT_PORT = 5555;
const int OFFSET_UDP_PORT = 1;
const size_t LEN_ADDRIN = sizeof(SOCKADDR_IN);

/// <summary>
/// the process of using network unit:
/// 
///		UDP:
///		launch
///		client start/ server start bind
///		update send recv
///		shut down
/// 
///		TCP:
///		launch
///		client start connect/ server start bind listen accept
///		update send recv
///		shut down
/// </summary>
class NetworkUnit
{
protected:
	WSADATA m_WsaData;
	bool m_IsStart;
	std::string m_IP = std::string(DEFAULT_IP);

public:
	char m_MsgBuf[LEN_MSG];

public:
	void Launch();
	void ShutDown();

public:
	//TCP
	int Recieve(SOCKET* socket, char* msgBuf);
	void Send(SOCKET* socket, char* msgBuf);

	//UDP
	int RecvFrom(SOCKET* socket, char* msgBuf, SOCKADDR_IN* from);
	void SendTo(SOCKET* socket, char* msgBuf, SOCKADDR_IN* dest);
};

#include "Server.h"
#include "Client.h"


