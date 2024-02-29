//#include <windows.h>
//#pragma comment(lib, "winmm.lib")
#define _CRT_SECURE_NO_WARNINGS
#include <windowsx.h>
#include <winsock2.h>
#include <windows.h> 
#include "GameManager.h"
#include "NetworkGameData.h"




void Player::Move(Vector3 dir)
{
	position.x += dir.x;
	position.y += dir.y;
	position.z += dir.z;
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

	//init network manager
	NetworkManager::InitServer();
	InitNetworkCommands();
}

void GameManager::Loop()
{
	while (1)
	{
		DWORD dwCurrentTime = timeGetTime();

		//update
		UpdateServer();

		if ((dwCurrentTime - dwExecLastTime) >= (1000 / FPS))
		{
			dwExecLastTime = dwCurrentTime;

			UpdatePlayer();
		}
	}
}

void GameManager::InitNetworkCommands()
{
	NetworkManager::SetCommand(
		BHID_C2S::Player_InputMove,
		[&](const MsgContent& msg)
		{
			//get data
			auto data = (Data_C2S_PlayerMove*)msg.Data;

			Vector3 move;
			move.x = data->leftRight;
			move.z = data->upDown;
			int clientID = data->id;

			player[clientID].Move(move);
		});
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
		int clientID = server.SRecvFromC(msgBuf);
		if (clientID >= 0)
		{
			sprintf(msgBuf, "test addr msg!");
			server.SendTo(&(server.m_UDPSocket), msgBuf,&(server.m_UDPAddrs[clientID]));
		}

		//if two players start game
		if (clientID == 0)
		{
			sprintf(msgBuf, "test addr msg!");
			InitPlayer();
		}
	}
	break;

	case State::ServerReadyToGame:
	{

	}
	break;

	case State::GameLoop:
	{
		//try udp, get client player input
		char msgBuf[LEN_MSG];
		int clientID = server.SRecvFromC(msgBuf);
		
		if (clientID>=0)
		{
			MsgContent msg;
			DecodeMsgContent(msgBuf, msg);
			int bhid = msg.BHID;

			if (bhid >= 0)
			{
				auto data = (Data_C2S_PlayerMove*)msg.Data;

				NetworkManager::Commands[bhid](msg);
			}
		}


		//analyze command
	/*	if (strcmp(msgBuf, "left")==0)
		{
			Vector3 move;
			move.x = -1;
			player[clientID].Move(move);
		}
		else if (strcmp(msgBuf, "right")==0)
		{
			Vector3 move;
			move.x = 1;
			player[clientID].Move(move);
		}*/
	}
	break;

	}
}

void GameManager::InitPlayer()
{
	std::cout << "init player" << std::endl;

	player[0].position.x = 0;
	player[0].position.y = 0;
	player[0].position.z = 1;

	player[1].position.x = 0;
	player[1].position.y = 0;
	player[1].position.z = -1;

	state = State::GameLoop;
}

void GameManager::UpdatePlayer()
{
	if (state != State::GameLoop)return;

	std::cout << "update player" << std::endl;

	char msgBuf[LEN_MSG];
	for (int clientID = 0; clientID < 1; clientID++)
	{
		Data_S2C_ObjectPos data;
		sizeof(data);
		
		data.player1X = player[0].position.x;
		data.player1Y = player[0].position.y;
		data.player1Z = player[0].position.z;

		data.player2X = player[1].position.x;
		data.player2Y = player[1].position.y;
		data.player2Z = player[1].position.z;

		data.shuttleX = shuttle.position.x;
		data.shuttleY = shuttle.position.y;
		data.shuttleZ = shuttle.position.z;

		MsgContent msg;
		msg.BHID = (int)BHID_S2C::Object_Position;
		msg.DataLen = sizeof(data);
		msg.Data = (void*)(&data);

		auto msgBuf = EncodeMsgContent(msg);
		server.SendTo(&(server.m_UDPSocket), msgBuf, &(server.m_UDPAddrs[clientID]));

		//give up
		/*for (int playerID = 0; playerID < 2; playerID++)
		{
			sprintf(msgBuf, "%d%lf", playerID,player[playerID].position.x);
			server.SendTo(&(server.m_UDPSocket), msgBuf, &(server.m_UDPAddrs[clientID]));
		}*/
	}
}
