//#include <windows.h>
//#pragma comment(lib, "winmm.lib")
#define _CRT_SECURE_NO_WARNINGS
#include <windowsx.h>
#include <winsock2.h>
#include <windows.h> 
#include "GameManager.h"
#include "NetworkGameData.h"

#include <thread>
#include <mutex>


#pragma region ========== player ==========
void Player::ChangeToState(PlayerStateType type)
{
	if (type==nowState) return;

	auto pair = states.find(type);
	//State* newState = pair->second;

	if (state != nullptr)
	{
		state->End();
	}
	pair->second->Start();

	nowState = type;
	state = pair->second;
}

void Player::Init()
{
	//set up state
	//set up state idle
	state_idle.type = PlayerStateType::Idle;
	state_idle.isLoop = true;
	//set up state run
	state_run.type = PlayerStateType::Run;
	//set up state hit
	state_hit.type = PlayerStateType::Hit;
	state_hit.isLoop = false;
	state_hit.maxFrame = 1000;
	state_hit.UpdateEvent = [&]()
	{
		//state_hit;
		std::cout << "update";
	};
	state_hit.NextEvent = [&]()
	{
		ChangeToState(PlayerStateType::Idle);
	};

	//emplace to map
	states.emplace(PlayerStateType::Hit, &state_hit);
	states.emplace(PlayerStateType::Idle, &state_idle);
	states.emplace(PlayerStateType::Run, &state_run);

	//set first state
	ChangeToState(PlayerStateType::Idle);
}

void Player::Update()
{
	//check hit
	float rot = 0;
	if (hitInfo.IsHit(rot))
	{
		ChangeToState(PlayerStateType::Hit);
	}

	//update state (state pattern)
	if (state != nullptr)
	{
		state->Update();
	}
}

void Player::Move(Vector3 dir)
{
	position.x += dir.x;
	position.y += dir.y;
	position.z += dir.z;
}

void Player::Hit(float rot)
{
	hitInfo.TriggerHitRot(rot);
}


void Player::Hit::TriggerHitRot(float rot)
{
	isHit = true;
	this->rot = rot;
}
bool Player::Hit::IsHit(float& outRot)
{
	outRot = rot;
	bool temp = isHit;
	isHit = false;
	return temp;
}

Player::State::State()
{
	StartEvent = []() {};
	UpdateEvent = []() {};
	EndEvent = []() {};
	NextEvent = []() {};
}
void Player::State::Start()
{
	frameCount = 0;
	StartEvent();
}
void Player::State::Update()
{
	frameCount++;

	if (frameCount > maxFrame)
	{
		if (isLoop)
		{
			frameCount = 0;
		}
		else
		{
			NextEvent();
		}
	}

	UpdateEvent();
}
void Player::State::End()
{
	frameCount = 0;
	EndEvent();
}
void Player::State::NextState()
{
	NextEvent();
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
	update_syncClient.FPS = 1000;

	//set up events
	{
		DoNothing = [&]()
		{
			//do nothing
			//for (int i = 0; i < players.size(); i++)
			//{
			//	Vector3 mov;
			//	mov.x = 1.0f / update_gameLoop.FPS;
			//	players[i].Move(mov);
			//}
		};
		TestPlayerHit = [&]()
		{
			//update players to check hit
			for (int id = 0; id < players.size(); id++)
			{
				players[id]->Update();
			}
		};
		RecvFromClients = [&]()
		{
			char msgBuf[LEN_MSG];
			while(1)
			{
				int clientID = server.SRecvFromC(msgBuf, true);

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
				else
				{
					break;
				}
			}
		};
		UpdateGame = [&]()
		{
			//update objects' motion
			//update players
			for (int id = 0; id < players.size(); id++)
			{
				players[id]->Update();
			}
		};
		SyncObjectsOfClients = [&]()
		{
			//send objects' data to all clients
			{
				Data_S2C_ObjectPos2 data;
				SetUpObjectsPosData(data);
				MsgContent msg;
				msg.BHID = (int)BHID_S2C::SyncObjectPosition;
				msg.DataLen = sizeof(data);
				msg.Data = (void*)(&data);
				SendToAll(msg);
			}


			//send players' data to all clients
			{
				Data_S2C_PlayerState data;
				SetUpPlayersStateData(data);
				MsgContent msg;
				msg.BHID = (int)BHID_S2C::SyncPlayerState;
				msg.DataLen = sizeof(data);
				msg.Data = (void*)(&data);
				SendToAll(msg);
			}
		};
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
		auto p = new Player();
		players.emplace_back(p);
		auto newPlayer = players[clientID];
		newPlayer->id = clientID;
		newPlayer->Init();

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
		move.x = (float)(data->leftRight) / (float)(update_gameLoop.FPS)*5;
		move.z = (float)(data->upDown) / (float)(update_gameLoop.FPS)*5;
		players[clientID]->Move(move);
	};
	PlayerTryHit = [&](const MsgContent& msg)
	{
		//todo
		auto data = (Data_C2S_PlayerHit*)msg.Data;

		//move player
		int clientID = data->id;
		float rot = data->rot;
		players[clientID]->Hit(rot);
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
		update_gameLoop.SetUpdateEvent(TestPlayerHit);// DoNothing);
		update_syncClient.SetUpdateEvent(SyncObjectsOfClients);

		//set up recv command
		NetworkManager::SetCommand(
			BHID_C2S::User_TryJoin,
			ApproveUserJoin);

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
	std::thread recvClient(&GameManager::RecvClientLoop, this);
	std::thread syncClient(&GameManager::SyncClientLoop, this);

	while (!threadOver)
	{
		//update_recvClient.Update();
		update_gameLoop.Update();
		//update_syncClient.Update();
	}
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

		outData.pos[i + 1].x = players[i]->position.x;
		outData.pos[i + 1].y = players[i]->position.y;
		outData.pos[i + 1].z = players[i]->position.z;
	}
}

void GameManager::SetUpPlayersStateData(Data_S2C_PlayerState& outData)
{
	//set all players' pos
	for (int i = 0; i < players.size(); i++)
	{
		outData.state[i] = (int)(players[i]->nowState);
		if (players[i]->nowState == PlayerStateType::Hit)
		{
			break;
		}
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

void GameManager::RecvClientLoop()
{
	while (!threadOver)
	{
		update_recvClient.Update();
	}
}

void GameManager::SyncClientLoop()
{
	while (!threadOver)
	{
		update_syncClient.Update();
	}
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


