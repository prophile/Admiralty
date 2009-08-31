#include "Admiralty.h"

#include <iostream>

using namespace Admiralty;

int main ()
{
	Entity friendlyShip;
	friendlyShip.id = 0;
	friendlyShip.entClass = 0;
	friendlyShip.allegiance = 0;
	friendlyShip.x = friendlyShip.y = 0.0f;
	friendlyShip.velX = 0.0f;
	friendlyShip.velY = 0.0f;
	friendlyShip.strength = 0.6f;
	friendlyShip.importance = 0.5f;
	friendlyShip.health = 1.0f;
	friendlyShip.isPassive = true;
	friendlyShip.isImmobile = false;
	Entity enemyShip(friendlyShip);
	enemyShip.id = 1;
	enemyShip.x = 100.0f;
	enemyShip.y = 4.0f;
	enemyShip.velX = -0.5f;
	enemyShip.allegiance = 1;
	Admiral friendlyController ( -200.0f, 200.0f, -200.0f, 200.0f, 0, ADMIRAL_TYPE_OFFENSIVE );
	friendlyController.SetResourceCount(100);
	friendlyController.DeclareEntity(friendlyShip);
	friendlyController.DeclareEntity(enemyShip);
	friendlyController.Recalculate();
	Action act;
	do
	{
		act = friendlyController.NextAction();
		switch (act.type)
		{
			case ACTION_TYPE_IDLE:
				std::cout << "no more orders" << std::endl;
				break;
			case ACTION_TYPE_BUILD:
				std::cout << "ORDER: build ship of class " << act.subject << std::endl;
				break;
			case ACTION_TYPE_GOTO:
				std::cout << "ORDER: send ship " << act.subject << " to coordinates (" << act.x << ", " << act.y << ")" << std::endl;
				break;
			case ACTION_TYPE_INTERCEPT:
				std::cout << "ORDER: send ship " << act.subject << " to intercept ship " << act.target << std::endl;
				break;
			case ACTION_TYPE_ESCORT:
				std::cout << "ORDER: send ship " << act.subject << " to escort ship " << act.target << std::endl;
				break;
		}
	} while(act.type != ACTION_TYPE_IDLE);
	Admiral enemyController ( -200.0f, 200.0f, -200.0f, 200.0f, 1, ADMIRAL_TYPE_DEFENSIVE );
	enemyController.SetResourceCount(100);
	enemyController.DeclareEntity(friendlyShip);
	enemyController.DeclareEntity(enemyShip);
	enemyController.Recalculate();
	do
	{
		act = enemyController.NextAction();
		switch (act.type)
		{
			case ACTION_TYPE_IDLE:
				std::cout << "no more orders" << std::endl;
				break;
			case ACTION_TYPE_BUILD:
				std::cout << "ORDER: build ship of class " << act.subject << std::endl;
				break;
			case ACTION_TYPE_GOTO:
				std::cout << "ORDER: send ship " << act.subject << " to coordinates (" << act.x << ", " << act.y << ")" << std::endl;
				break;
			case ACTION_TYPE_INTERCEPT:
				std::cout << "ORDER: send ship " << act.subject << " to intercept ship " << act.target << std::endl;
				break;
			case ACTION_TYPE_ESCORT:
				std::cout << "ORDER: send ship " << act.subject << " to escort ship " << act.target << std::endl;
				break;
		}
	} while(act.type != ACTION_TYPE_IDLE);
	std::cout << "sizeof(Admiral) = " << sizeof(Admiral) << std::endl;
	return 0;
}
