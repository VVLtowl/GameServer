//#define _WINSOCK_DEPRECATED_NO_WARNINGS
#include "GameManager.h"
#include <cstdlib>


GameManager manager;

void Clean()
{
	manager.threadOver = true;
}

int main()
{

	//init game
	manager.Start();

	//update game
	manager.Loop();

	std::atexit(Clean);

	return 0;
}