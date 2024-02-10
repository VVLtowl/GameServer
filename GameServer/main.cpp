//#define _WINSOCK_DEPRECATED_NO_WARNINGS
#include "GameManager.h"


int main()
{
	GameManager manager;

	//init game
	manager.Start();

	//update game
	manager.Loop();

	return 0;
}