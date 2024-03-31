#pragma once
#include "NetworkCommon.h"
#include "SerializedTool.h"
#include "NetworkGameData.h"
#include "Updater.h"
#include <functional>
#include <vector>
#include <list>

#define SCREEN_WIDTH	(960)
#define SCREEN_HEIGHT	(540)



struct Vector3
{
	float x=0;
	float y=0;
	float z=0;
};

class Player;
class Shuttle;
class Team;

#pragma region ========== player ==========
class Player
{
public:
	int id;
	Team* team;
	Vector3 position;
	bool host;

	class Hit
	{
	public:
		bool isHit;
		float rot;
		void TriggerHitRot(float rot);
		bool IsHit(float& outRot);
	};
	Hit hitInfo;

	class State
	{
	public:
		PlayerStateType type;
		bool isLoop;
		int maxFrame;
		int frameCount;
	public:
		State();
		void Start();
		void Update();
		void End();
		void NextState();
		std::function<void()> StartEvent;
		std::function<void()> UpdateEvent;
		std::function<void()> EndEvent;
		std::function<void()> NextEvent;
	};
	PlayerStateType nowState;
	State* state;
	State state_hit;
	State state_idle;
	State state_run;
	std::unordered_map<PlayerStateType,State*> states;

public:
	void ChangeToState(PlayerStateType type);
	void Init();
	void Update();
	void Move(Vector3 dir);
	void Hit(float rot);
};
#pragma endregion


#pragma region ========== shuttle ==========
class Shuttle
{
public:
	Vector3 position;
	Vector3 velocity;
	enum class State
	{
		Fly,
		Land,
	};
	State state;

public:
	void Update();
	void FlyTo(Vector3 dir);
	void Fly();
	void Land();
};
#pragma endregion


#pragma region ========== team ==========
class Team
{
public:
	std::list<Player*> members;
	unsigned short point;
	Vector3 startPos;

public:
	void AddMember(Player* player);
	void RemoveMember(Player* player);
};
#pragma endregion

/// <summary>
/// server
/// player1
/// player2
/// shuttle
/// </summary>
class GameManager
{
public:
	//state
	enum class State
	{
		ServerListen,
		ShowServerReadyToGame,
		ServerReadyToGame,
		GameLoop,

		WaitGame,
		Serve,
		Playing,
		CountPoint,
		Max,
	};
	State state;

	//game object
	std::vector<Player*> players;
	Shuttle shuttle;
	Team teams[2];

	//time control
	DWORD dwExecLastTime;
	SHORT FPS;

	//server
	Server server;

	//update
	Updater update_recvClient;
	Updater update_gameLoop;
	Updater update_syncClient;

public:
	void Start();
	void Loop();

	//init
private:
	void InitUpdatersAndEvents();
	void InitNetworkCommands();
	void InitPlayer();

	//send function
private:
	void SendToAll(const MsgContent& msg);
	void SendToOne(int clientID, const MsgContent& msg);

	//other
private:
	void SetState(State sta);
	void ResetClientsID();
	void SetUpObjectsPosData(Data_S2C_ObjectPos2& outData);
	void SetUpPlayersStateData(Data_S2C_PlayerState& outData);
	void SendRemovePlayer(int removeClientID);

	//update func
private:
	//std::vector<std::function<void()>> RecvClient_UpdateEvents;
	//std::vector<std::function<void()>> GameLoop_UpdateEvents;
	//std::vector<std::function<void()>> SyncClient_UpdateEvents;
	std::function<void()> DoNothing;
	std::function<void()> TestPlayerHit;
	std::function<void()> RecvFromClients;
	std::function<void()> UpdateGame;
	std::function<void()> SyncObjectsOfClients;

	//command func
private:
	std::function<void(MsgContent)> ApproveUserJoin;
	std::function<void(MsgContent)> RefuseUserJoin;
	std::function<void(MsgContent)> MovePlayer;
	std::function<void(MsgContent)> PlayerTryHit;
	std::function<void(MsgContent)> ApproveUserQuit;

	//multi thread
private:
	void RecvClientLoop();
	void SyncClientLoop();
public:
	bool threadOver = false;
};

