//#include <windows.h>
//#pragma comment(lib, "winmm.lib")
#include <windowsx.h>
#include <winsock2.h>
#include <windows.h> 
#include "GameManager.h"




void Player::Move(Vector3 dir)
{

}

void GameManager::Start()
{
	//init data
	dwExecLastTime = timeGetTime();
	FPS = 1;

	//start server
	server.Start();
	server.Bind();
	//server.Listen();

	//set state
	state = State::ServerListen;
}

void GameManager::Loop()
{
	while (1)
	{
		DWORD dwCurrentTime = timeGetTime();

		if ((dwCurrentTime - dwExecLastTime) >= (1000 / FPS))
		{
			dwExecLastTime = dwCurrentTime;

			//update
			UpdateServer();
		}
	}
}

void GameManager::UpdateServer()
{
	switch (state)
	{
	case State::ServerListen:
	{
		//server.Accept();

		//try udp
		char msgBuf[LEN_MSG];
		server.SRecvFromC(msgBuf);
	}
	break;

	case State::ServerReadyToGame:
	{

	}
	break;
	}
}
