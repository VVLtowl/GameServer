//#include <windows.h>
//#pragma comment(lib, "winmm.lib")
#define _CRT_SECURE_NO_WARNINGS
#include <windowsx.h>
#include <winsock2.h>
#include <windows.h> 
#include "GameManager.h"
#include "NetworkGameData.h"


#pragma region ========== player ==========
void Player::Move(Vector3 dir)
{
	position.x += dir.x;
	position.y += dir.y;
	position.z += dir.z;
}
#pragma endregion

#pragma region ========== game manager ==========
void GameManager::Start()
{
	//init data
	dwExecLastTime = timeGetTime();
	FPS = 1;

	//start server
	server.Start();
	server.Bind();
	//server.Listen();

	//init network manager
	NetworkManager::InitServer();
	InitNetworkCommands();
	InitUpdatersAndEvents();
	InitPlayer();

	//set first state
	SetState(State::WaitGame);
}

void GameManager::InitUpdatersAndEvents()
{
	//set updater fps
	update_recvClient.FPS = 1000;
	update_gameLoop.FPS = 60;
	update_syncClient.FPS = 30;

	//set up events
	{
		DoNothing = []()
		{
			//do nothing
		};
		RecvFromClients = [&]()
		{
			char msgBuf[LEN_MSG];
			int clientID = server.SRecvFromC(msgBuf);

			if (clientID >= 0)
			{
				MsgContent msg;
				DecodeMsgContent(msgBuf, msg);
				int bhid = msg.BHID;
				if (bhid >= 0)
				{
					NetworkManager::Commands[bhid](msg);
				}
			}
		};
		UpdateGame = []()
		{
			//update objects' motion

		};
		SyncObjectsOfClients = [&]()
		{
			//send objects' data to all clients
			Data_S2C_ObjectPos2 data;
			SetUpObjectsPosData(data);
			MsgContent msg;
			msg.BHID = (int)BHID_S2C::SyncObjectPosition;
			msg.DataLen = sizeof(data);
			msg.Data = (void*)(&data);
			SendToAll(msg);
		};
	}
}

void GameManager::SetState(State sta)
{
	state = sta;
	NetworkManager::ResetCommands();

	switch (state)
	{
	case State::WaitGame:
	{
		//set up update event
		update_recvClient.SetUpdateEvent(RecvFromClients);
		update_gameLoop.SetUpdateEvent(DoNothing);
		update_syncClient.SetUpdateEvent(SyncObjectsOfClients);

		//set up recv command
		NetworkManager::SetCommand(
			BHID_C2S::User_TryJoin,
			ApproveUserJoin);

		NetworkManager::SetCommand(
			BHID_C2S::Player_InputMove,
			MovePlayer);

		NetworkManager::SetCommand(
			BHID_C2S::User_Quit,
			ApproveUserQuit);
	}
	break;

	case State::Playing:
	{
		//set up update event
		update_recvClient.SetUpdateEvent(RecvFromClients);
		update_gameLoop.SetUpdateEvent(UpdateGame);
		update_syncClient.SetUpdateEvent(SyncObjectsOfClients);

		//set up recv command
		NetworkManager::SetCommand(
			BHID_C2S::User_TryJoin,
			RefuseUserJoin);

		NetworkManager::SetCommand(
			BHID_C2S::Player_InputMove,
			MovePlayer);

		NetworkManager::SetCommand(
			BHID_C2S::Player_InputHit,
			PlayerTryHit);

		NetworkManager::SetCommand(
			BHID_C2S::User_Quit,
			ApproveUserQuit);
	}

	break;

	}

}

void GameManager::Loop()
{
	while (1)
	{
		update_recvClient.Update();
		update_gameLoop.Update();
		update_syncClient.Update();
	}
}

void GameManager::InitNetworkCommands()
{
	ApproveUserJoin = [&](const MsgContent& msg)
	{
		//set id
		int clientID = server.m_UDPAddrs.size() - 1;

		//send exist players to this client
		{
			Data_S2C_ObjectPos2 data;
			SetUpObjectsPosData(data);
			MsgContent msg;
			msg.BHID = (int)BHID_S2C::AddExistPlayers;
			msg.DataLen = sizeof(data);
			msg.Data = (void*)(&data);
			SendToOne(clientID, msg);
		}

		//create player
		Player player;
		players.emplace_back(player);
		auto newPlayer = &players[clientID];

		//check client id and send to this client
		{
			Data_UserID data;
			data.id = clientID;
			MsgContent msg;
			msg.BHID = (int)BHID_S2C::SetID;
			msg.DataLen = sizeof(data);
			msg.Data = (void*)(&data);
			SendToOne(clientID, msg);
		}

		//send player information to all clients
		{
			Data_Pos data;
			//test set start position of player
			newPlayer->position.x = clientID;
			newPlayer->position.y = 0;
			newPlayer->position.z = 1;
			data.x = newPlayer->position.x;
			data.y = newPlayer->position.y;
			data.z = newPlayer->position.z;
			MsgContent msg;
			msg.BHID = (int)BHID_S2C::AddPlayer;
			msg.DataLen = sizeof(data);
			msg.Data = (void*)&data;
			SendToAll(msg);
		}
	};
	RefuseUserJoin = [&](const MsgContent& msg)
	{
		//todo
	};
	MovePlayer = [&](const MsgContent& msg)
	{
		auto data = (Data_C2S_PlayerMove*)msg.Data;

		//move player
		int clientID = data->id;
		Vector3 move;
		move.x = data->leftRight;
		move.z = data->upDown;
		players[clientID].Move(move);
	};
	PlayerTryHit = [&](const MsgContent& msg)
	{
		//todo
	};
	ApproveUserQuit = [&](const MsgContent& msg)
	{
		auto data = (Data_UserID*)msg.Data;

		//remove player,
		//dont remove udpaddress because need to use address to approve client quit 
		int clientID = data->id;
		players.erase(players.begin() + clientID);

		//approve client quit
		MsgContent msg2;
		msg2.BHID = (int)BHID_S2C::ApproveQuit;
		SendToOne(clientID, msg2);

		//reset clients id
		//remove addr at first
		server.RemoveAddr(clientID);
		SendRemovePlayer(clientID);
		ResetClientsID();
	};
}

void GameManager::InitPlayer()
{
	std::cout << "init player" << std::endl;

	players.clear();
}

void GameManager::SendToAll(const MsgContent& msg)
{
	for (int clientID = 0; clientID < players.size(); clientID++)
	{
		SendToOne(clientID, msg);
	}
}

void GameManager::SendToOne(int clientID, const MsgContent& msg)
{
	auto msgBuf = EncodeMsgContent(msg);
	server.SendTo(&(server.m_UDPSocket), msgBuf, &(server.m_UDPAddrs[clientID]));
}

void GameManager::ResetClientsID()
{
	for (int i = 0; i < server.m_UDPAddrs.size(); i++)
	{
		Data_UserID data;
		data.id = i;
		MsgContent msg;
		msg.BHID = (int)BHID_S2C::SetID;
		msg.DataLen = sizeof(data);
		msg.Data = (void*)(&data);
		SendToOne(i, msg);
	}
}

void GameManager::SetUpObjectsPosData(Data_S2C_ObjectPos2& outData)
{
	//player amount plus shuttle
	outData.max = players.size() + 1;

	//set shuttle's pos
	outData.pos[0].x = shuttle.position.x;
	outData.pos[0].y = shuttle.position.y;
	outData.pos[0].z = shuttle.position.z;

	//set all players' pos
	for (int i = 0; i < players.size(); i++)
	{
		if (i + 1 >= outData.max)
		{
			std::cout << "oversize objects\n";
			continue;
		}

		outData.pos[i + 1].x = players[i].position.x;
		outData.pos[i + 1].y = players[i].position.y;
		outData.pos[i + 1].z = players[i].position.z;
	}
}

void GameManager::SendRemovePlayer(int removeClientID)
{
	for (int i = 0; i < server.m_UDPAddrs.size(); i++)
	{
		Data_UserID data;
		data.id = removeClientID;
		MsgContent msg;
		msg.BHID = (int)BHID_S2C::RemovePlayer;
		msg.DataLen = sizeof(data);
		msg.Data = (void*)(&data);
		SendToOne(i, msg);
	}
}

#pragma endregion

#pragma region ========== updater ==========
void Updater::Update()
{
	DWORD dwCurrentTime = timeGetTime();

	//fixed update
	if ((dwCurrentTime - dwExecLastTime) >= (1000 / FPS))
	{
		dwExecLastTime = dwCurrentTime;

		UpdateEvent();
	}
}

void Updater::SetUpdateEvent(std::function<void()> func)
{
	UpdateEvent = func;
}
#pragma endregion



#pragma region ========== team ==========

void Team::AddMember(Player* player)
{
	members.emplace_back(player);
	player->team = this;
}

void Team::RemoveMember(Player* player)
{
	members.remove(player);
}

#pragma endregion