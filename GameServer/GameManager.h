#pragma once
#include "NetworkCommon.h"

#define SCREEN_WIDTH	(960)
#define SCREEN_HEIGHT	(540)



struct Vector3
{
	float x;
	float y;
	float z;
};

#pragma region ========== player ==========
class Player
{
public:
	Vector3 position;
	enum class State
	{
		Hit,
		None,
	};
	State state;

public:
	void Move(Vector3 dir);
	void Hit();
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
	void FlyTo(Vector3 dir);
	void Fly();
	void Land();
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
	};
	State state;

	//game object
	Player player[2];
	Shuttle shuttle;

	//time control
	DWORD dwExecLastTime;
	SHORT FPS;

	//server
	Server server;


public:
	void Start();
	void Loop();

private:
	void UpdateServer();//execute commands
	void InitPlayer();
	void UpdatePlayer();
};

