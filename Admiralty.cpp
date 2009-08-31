#include "Admiralty.h"

#include <string.h>
#include <math.h>
#include <stdlib.h>
#include <algorithm>
#include <stdio.h>

//#define ADMIRALTY_DEBUG

#ifdef ADMIRALTY_DEBUG
#include <iostream>
#define DEBUG(message) std::cout << "DEBUG: " << message << std::endl;
#else
#define DEBUG(message)
#endif

namespace Admiralty
{

	namespace Internal
	{
	
		InfluenceGrid::InfluenceGrid ( float left, float right, float bottom, float top )
		: _left(left), _right(right),
		  _top(top), _bottom(bottom)
		{
		}
		
		InfluenceGrid::~InfluenceGrid ()
		{
		}
		
		void InfluenceGrid::Reset ()
		{
			memset(_influence, 0, sizeof(_influence));
		}
		
		void InfluenceGrid::AddInfluence ( float x, float y, float amount )
		{
			DEBUG("added influence weight " << amount << " at " << x << ", " << y);
			DEBUG("field is " << _left << " to " << _right << " horizontally, and");
			DEBUG(_bottom << " to " << _top << " vertically");
			x -= _left;
			x /= (_right - _left);
			x *= (float)INFLUENCE_GRID_SIZE;
			y -= _bottom;
			y /= (_top - _bottom);
			y *= (float)INFLUENCE_GRID_SIZE;
			if (x < 0.0f ||
			    y < 0.0f ||
			    x > (float)INFLUENCE_GRID_SIZE ||
			    y > (float)INFLUENCE_GRID_SIZE)
			{
				DEBUG("you're off the edge of the map, mate");
				DEBUG("HERE BE MONSTERS");
				DEBUG(" C: " << x << ", " << y);
				DEBUG("IG: " << INFLUENCE_GRID_SIZE);
				return;
			}
			int index = (int)x + ((int)y * INFLUENCE_GRID_SIZE);
			DEBUG("grid index: " << index << " = " << (int)x << ", " << (int)y);
			_influence[index] += amount;
		}
		
		void InfluenceGrid::ApplyWeight ( float weight )
		{
			for (int i = 0; i < (INFLUENCE_GRID_SIZE * INFLUENCE_GRID_SIZE); i++)
			{
				_influence[i] *= weight;
			}
		}
		
		void InfluenceGrid::AddGrid ( const InfluenceGrid& grid )
		{
			for (int i = 0; i < (INFLUENCE_GRID_SIZE * INFLUENCE_GRID_SIZE); i++)
			{
				_influence[i] += grid._influence[i];
			}
		}
		
		void InfluenceGrid::Select ( float& x, float& y )
		{
			const int MAX = INFLUENCE_GRID_SIZE * INFLUENCE_GRID_SIZE;
			int maxIdx = MAX / 2;
			float max = -200000.0f;
			for (int i = 0; i < INFLUENCE_GRID_SAMPLES; i++)
			{
				int idx = rand() % MAX;
				if (_influence[idx] > max)
				{
					DEBUG("found good target: idx " << idx << " and influence " << _influence[idx]);
					max = _influence[idx];
					maxIdx = idx;
				}
			}
			int cx, cy;
			cx = maxIdx % INFLUENCE_GRID_SIZE;
			cy = maxIdx / INFLUENCE_GRID_SIZE;
			DEBUG("\tindex is: " << cx << ", " << cy);
			float fx, fy;
			fx = cx / (float)INFLUENCE_GRID_SIZE;
			fy = cy / (float)INFLUENCE_GRID_SIZE;
			fx *= (_right - _left);
			fy *= (_top - _bottom);
			fx += _left;
			fy += _bottom;
			x = fx;
			y = fy;
		}
		
		void InfluenceGrid::Dump ( const std::string& path )
		{
#ifdef ADMIRALTY_DEBUG
			FILE* fp = fopen(path.c_str(), "w");
			if (!fp)
				return;
			for (int i = 0; i < INFLUENCE_GRID_SIZE; ++i)
			{
				int influenceBase = (INFLUENCE_GRID_SIZE - i - 1) * INFLUENCE_GRID_SIZE;
				fprintf(fp, "+");
				for (int j = 0; j < INFLUENCE_GRID_SIZE; ++j)
				{
					fprintf(fp, "--------+");
				}
				fprintf(fp, "\n|");
				for (int j = 0; j < INFLUENCE_GRID_SIZE; ++j)
				{
					fprintf(fp, " % 1.3f |", _influence[influenceBase + j]);
				}
				fprintf(fp, "\n");
			}
			fprintf(fp, "+");
			for (int j = 0; j < INFLUENCE_GRID_SIZE; ++j)
			{
				fprintf(fp, "--------+");
			}
			fprintf(fp, "\n");
			fclose(fp);
			return;
#endif
		}
	}
	
	using namespace Internal;
	
	void Admiral::PopulateShipBuildingActions ( bool aggressive )
	{
		unsigned int totalResources = _resources;
		float maintainanceRatio = aggressive ? 1.0f : 1.6f;
		bool madeChanges;
		do
		{
			madeChanges = false;
			for (std::vector<ShipClass>::iterator iter = _classes.begin(); iter != _classes.end(); iter++)
			{
				if (iter->cost * maintainanceRatio > totalResources)
				{
					float random = rand() / (float)RAND_MAX;
					random *= _totalBuildRatio;
					if (random < iter->buildRatio)
					{
						Action act;
						act.type = ACTION_TYPE_BUILD;
						act.subject = iter->id;
						totalResources -= iter->cost;
						madeChanges = true;
					}
				}
			}
		} while(madeChanges);
	}
	
	AdmiralType Admiral::ActiveType ( float totalEnemyStrength, float totalFriendlyStrength )
	{
		if (_type == ADMIRAL_TYPE_GENERIC)
		{
			float enemyRatio = totalEnemyStrength / totalFriendlyStrength;
			if (enemyRatio < 1.1f) return ADMIRAL_TYPE_DEFENSIVE;
			else if (enemyRatio < 1.5f) return ADMIRAL_TYPE_OFFENSIVE;
			else if (enemyRatio < 1.7f) return ADMIRAL_TYPE_BESERK;
			else return ADMIRAL_TYPE_OFFENSIVE;
		}
		else
		{
			return _type;
		}
	}
	
	void Admiral::PopulateActions ( float totalEnemyStrength, float totalFriendlyStrength )
	{
		DEBUG("populating actions");
		bool isLosing = totalEnemyStrength > totalFriendlyStrength;
		std::vector<unsigned int> goodInterceptTargets;
		std::vector<unsigned int> goodEscortTargets;
		// TODO: ship building actions
		PopulateShipBuildingActions(isLosing);
		DEBUG("scanning for good intercept/escort targets");
		for (std::map<unsigned int, Entity>::iterator iter = _ents.begin();
		                                              iter != _ents.end();
		                                              ++iter)
		{
			float weight = (0.2f + iter->second.importance) * sqrtf(1.4f - iter->second.health);
			if (weight > 0.8f && (rand() % 2 == 0))
			{
				if (IsAlly(iter->second.allegiance))
				{
					goodInterceptTargets.push_back(iter->first);
				}
				else
				{
					goodEscortTargets.push_back(iter->first);
				}
			}
		}
		DEBUG("shuffling intercept/escort targets");
		std::random_shuffle(goodInterceptTargets.begin(), goodInterceptTargets.end());
		std::random_shuffle(goodEscortTargets.begin(), goodEscortTargets.end());
		DEBUG("scanning for ships to whom to issue orders");
		for (std::map<unsigned int, Entity>::iterator iter = _ents.begin();
		                                              iter != _ents.end();
		                                              ++iter)
		{
			if (iter->second.allegiance == _allegiance)
			{
				// it's ours
				DEBUG("found one of ours");
				if (iter->second.isPassive && !iter->second.isImmobile)
				{
					DEBUG("giving orders to ship " << iter->second.id);
					// it's not doing anything, time to issue orders
					if (!goodEscortTargets.empty())
					{
						Action act;
						act.subject = iter->first;
						act.type = ACTION_TYPE_ESCORT;
						act.target = goodEscortTargets.back();
						goodEscortTargets.pop_back();
						_actions.push(act);
					}
					else if (!goodInterceptTargets.empty())
					{
						Action act;
						act.subject = iter->first;
						act.type = ACTION_TYPE_ESCORT;
						act.target = goodInterceptTargets.back();
						goodInterceptTargets.pop_back();
						_actions.push(act);
					}
					else
					{
						Action act;
						act.subject = iter->first;
						act.type = ACTION_TYPE_GOTO;
						_masterGrid.Select(act.x, act.y);
						_actions.push(act);
					}
				}
				else
				{
					DEBUG("but it's active or immobile");
				}
			}
		}
	}
	
	bool Admiral::IsAlly ( unsigned int side )
	{
		if (side == _allegiance) return true;
		if (_allies.find(side) != _allies.end()) return true;
		return false;
	}
	
	Admiral::Admiral ( float left, float right, float bottom, float top, unsigned int allegiance, AdmiralType t )
	: _left(left), _right(right), _bottom(bottom), _top(top), _allegiance(allegiance), _resources(0), _type(t),
	  _friendlyImportanceGrid(left, right, bottom, top),
	  _friendlyStrengthGrid(left, right, bottom, top),
	  _enemyImportanceGrid(left, right, bottom, top),
	  _enemyStrengthGrid(left, right, bottom, top),
	  _masterGrid(left, right, bottom, top),
	  _totalBuildRatio(0.0f)
	{
	}
	
	void Admiral::DeclareEntity ( Entity ent )
	{
		_ents[ent.id] = ent;
	}
	
	void Admiral::RemoveEntity ( unsigned int id )
	{
		std::map<unsigned int, Entity>::iterator iter = _ents.find(id);
		if (iter != _ents.end())
			_ents.erase(iter);
	}
	
	void Admiral::DeclareClass ( ShipClass aClass )
	{
		_classes.push_back(aClass);
		_totalBuildRatio += aClass.buildRatio;
	}
	
	void Admiral::AddAlly ( unsigned int ally )
	{
		_allies.insert(ally);
	}
	
	void Admiral::Recalculate ()
	{
		DEBUG("emptying current actions queue");
		// remove pending actions
		while (!_actions.empty()) _actions.pop();
		float totalEnemyStrength = 0.0f, totalFriendlyStrength = 0.0f;
		// calculate the basic importance grids
		DEBUG("resetting influence grids");
		_friendlyImportanceGrid.Reset();
		_friendlyStrengthGrid.Reset();
		_enemyImportanceGrid.Reset();
		_enemyStrengthGrid.Reset();
		_masterGrid.Reset();
		DEBUG("repopulating influence grids");
		for (std::map<unsigned int, Entity>::iterator iter = _ents.begin();
		     iter != _ents.end();
		     ++iter)
		{
			if (IsAlly(iter->second.allegiance))
			{
				// allied ship
				_friendlyImportanceGrid.AddInfluence(iter->second.x, iter->second.y, iter->second.importance);
				_friendlyStrengthGrid.AddInfluence(iter->second.x, iter->second.y, iter->second.strength * iter->second.health);
				totalFriendlyStrength += iter->second.strength * iter->second.health;
			}
			else
			{
				// enemy ship
				_enemyImportanceGrid.AddInfluence(iter->second.x, iter->second.y, iter->second.importance);
				_enemyStrengthGrid.AddInfluence(iter->second.x, iter->second.y, iter->second.strength * iter->second.health);
				_masterGrid.AddInfluence(iter->second.x + iter->second.velX * 5.0f, iter->second.y + iter->second.velY * 5.0f, iter->second.strength * iter->second.health);
				totalEnemyStrength += iter->second.strength * iter->second.health;
			}
		}
		_friendlyImportanceGrid.Dump("/tmp/importance-friendly.grid");
		_friendlyStrengthGrid.Dump("/tmp/strength-friendly.grid");
		_enemyImportanceGrid.Dump("/tmp/importance-enemy.grid");
		_enemyStrengthGrid.Dump("/tmp/strength-enemy.grid");
		_masterGrid.Dump("/tmp/forecast-enemy.grid");
		DEBUG("weighting influence grids");
		AdmiralType type = ActiveType(totalEnemyStrength, totalFriendlyStrength);
		// apply weights
		switch (type)
		{
			case ADMIRAL_TYPE_DEFENSIVE:
				_friendlyImportanceGrid.ApplyWeight(3.0f);
				_friendlyStrengthGrid.ApplyWeight(-0.4f);
				_enemyImportanceGrid.ApplyWeight(0.6f);
				_enemyStrengthGrid.ApplyWeight(0.4f);
				_masterGrid.ApplyWeight(0.4f);
				break;
			case ADMIRAL_TYPE_OFFENSIVE:
				_friendlyImportanceGrid.ApplyWeight(0.8f);
				_friendlyStrengthGrid.ApplyWeight(-0.4f);
				_enemyImportanceGrid.ApplyWeight(3.0f);
				_enemyStrengthGrid.ApplyWeight(1.0f);
				_masterGrid.ApplyWeight(1.0f);
				break;
			case ADMIRAL_TYPE_BESERK:
				_friendlyImportanceGrid.ApplyWeight(0.1f);
				_friendlyStrengthGrid.ApplyWeight(-0.1f);
				_enemyImportanceGrid.ApplyWeight(0.8f);
				_enemyStrengthGrid.ApplyWeight(2.0f);
				_masterGrid.ApplyWeight(3.0f);
				break;
			case ADMIRAL_TYPE_ASSASSIN:
				_friendlyImportanceGrid.ApplyWeight(0.2f);
				_friendlyStrengthGrid.ApplyWeight(-0.5f);
				_enemyImportanceGrid.ApplyWeight(5.0f);
				_enemyStrengthGrid.ApplyWeight(-0.4f);
				_masterGrid.ApplyWeight(-2.0f);
				break;
		}
		// combine into one
		DEBUG("combining influence grids");
		_masterGrid.AddGrid(_friendlyImportanceGrid);
		_masterGrid.AddGrid(_friendlyStrengthGrid);
		_masterGrid.AddGrid(_enemyImportanceGrid);
		_masterGrid.AddGrid(_enemyStrengthGrid);
		_masterGrid.Dump("/tmp/master.grid");
		// determine new actions
		PopulateActions(totalEnemyStrength, totalFriendlyStrength);
	}
	
	Action Admiral::NextAction ()
	{
		if (_actions.empty())
		{
			Action act;
			act.type = ACTION_TYPE_IDLE;
			act.target = 0;
			return act;
		}
		else
		{
			Action act = _actions.front();
			_actions.pop();
			return act;
		}
	}

}
