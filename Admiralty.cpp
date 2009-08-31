#include "Admiralty.h"

#include <string.h>
#include <math.h>
#include <stdlib.h>
#include <algorithm>

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
			x -= _left;
			x /= (_right - _left);
			x *= (float)INFLUENCE_GRID_SIZE;
			y -= _bottom;
			y /= (_bottom - _top);
			y *= (float)INFLUENCE_GRID_SIZE;
			if (x < 0.0f || y < 0.0f) return;
			if (x > (float)INFLUENCE_GRID_SIZE ||
			    y > (float)INFLUENCE_GRID_SIZE) return;
			y *= 32.0f;
			_influence[(int)(x + y)] += amount;
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
			float max = -std::numeric_limits<float>::max();
			for (int i = 0; i < INFLUENCE_GRID_SAMPLES; i++)
			{
				int idx = rand() % MAX;
				if (_influence[idx] > max)
				{
					max = _influence[idx];
					maxIdx = i;
				}
			}
			int cx, cy;
			cx = maxIdx % 32;
			cy = maxIdx / 32;
			float fx, fy;
			fx = cx / 32.0f;
			fy = cy / 32.0f;
			fx *= (_right - _left);
			fy *= (_top - _bottom);
			fx += _left;
			fy += _bottom;
			x = fx;
			y = fy;
		}
	
	}
	
	using namespace Internal;
	
	void Admiral::PopulateActions ( float totalEnemyStrength, float totalFriendlyStrength )
	{
		bool isLosing = totalEnemyStrength > totalFriendlyStrength;
		std::vector<unsigned int> goodInterceptTargets;
		std::vector<unsigned int> goodEscortTargets;
		// TODO: ship building actions
		std::random_shuffle(_classes.begin(), _classes.end());
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
		std::random_shuffle(goodInterceptTargets.begin(), goodInterceptTargets.end());
		std::random_shuffle(goodEscortTargets.begin(), goodEscortTargets.end());
		for (std::map<unsigned int, Entity>::iterator iter = _ents.begin();
		                                              iter != _ents.end();
		                                              ++iter)
		{
			if (iter->second.allegiance == _allegiance)
			{
				// it's ours
				if (iter->second.isPassive && !iter->second.isImmobile)
				{
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
	  _masterGrid(left, right, bottom, top)
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
	}
	
	void Admiral::AddAlly ( unsigned int ally )
	{
		_allies.insert(ally);
	}
	
	void Admiral::Recalculate ()
	{
		// remove pending actions
		while (!_actions.empty()) _actions.pop();
		float totalEnemyStrength = 0.0f, totalFriendlyStrength = 0.0f;
		// calculate the basic importance grids
		_friendlyImportanceGrid.Reset();
		_friendlyStrengthGrid.Reset();
		_enemyImportanceGrid.Reset();
		_enemyStrengthGrid.Reset();
		_masterGrid.Reset();
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
		// apply weights
		switch (_type)
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
		}
		// combine into one
		_masterGrid.AddGrid(_friendlyImportanceGrid);
		_masterGrid.AddGrid(_friendlyStrengthGrid);
		_masterGrid.AddGrid(_enemyImportanceGrid);
		_masterGrid.AddGrid(_enemyStrengthGrid);
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
