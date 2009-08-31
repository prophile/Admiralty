#include <map>
#include <vector>
#include <queue>
#include <set>

namespace Admiralty
{

	struct Entity
	{
		unsigned int id; // unique entity ID
		unsigned int entClass; // classification system
		unsigned int allegiance; // group to which this entity is allied
		float x, y; // x and y coordinates
		float velX, velY; // x and y velocity
		float strength, importance, health; // in scale 0..1
		bool isPassive, isImmobile; // is not doing much
	};
	
	struct ShipClass
	{
		unsigned int id; // class id
		unsigned int specialist; // what this is a specialist in dealing with
		unsigned int cost; // resource cost
		float strength; // in scale 0..1
	};
	
	enum ActionType
	{
		ACTION_TYPE_IDLE,
		ACTION_TYPE_BUILD,
		ACTION_TYPE_GOTO,
		ACTION_TYPE_INTERCEPT,
		ACTION_TYPE_ESCORT
	};
	
	struct Action
	{
		ActionType type;
		unsigned int subject;
		unsigned int target;
		float x, y;
	};
	
	namespace Internal
	{
	
		const static int INFLUENCE_GRID_SIZE = 32;
		const static int INFLUENCE_GRID_SAMPLES = 12;
	
		class InfluenceGrid
		{
		private:
			float _left, _right;
			float _top, _bottom;
			float _influence[INFLUENCE_GRID_SIZE * INFLUENCE_GRID_SIZE];
		public:
			InfluenceGrid ( float left, float right, float bottom, float top );
			~InfluenceGrid ();
			
			void Reset ();
			void AddInfluence ( float x, float y, float amount );
			void ApplyWeight ( float weight );
			void AddGrid ( const InfluenceGrid& grid );
			void Select ( float& x, float& y );
		};
	
	};
	
	enum AdmiralType
	{
		ADMIRAL_TYPE_DEFENSIVE,
		ADMIRAL_TYPE_OFFENSIVE,
		ADMIRAL_TYPE_BESERK
	};
	
	class Admiral
	{
	private:
		std::map<unsigned int, Entity> _ents;
		float _left, _right, _bottom, _top;
		AdmiralType _type;
		unsigned int _resources;
		unsigned int _allegiance;
		std::vector<ShipClass> _classes;
		std::queue<Action> _actions;
		std::set<unsigned int> _allies;
		Internal::InfluenceGrid _friendlyImportanceGrid, _friendlyStrengthGrid;
		Internal::InfluenceGrid _enemyImportanceGrid, _enemyStrengthGrid;
		Internal::InfluenceGrid _masterGrid;
		void PopulateActions ( float totalEnemyStrength, float totalFriendlyStrength );
		bool IsAlly ( unsigned int side );
	public:
		Admiral ( float left, float right, float bottom, float top, unsigned int allegiance, AdmiralType t );
		void SetResourceCount ( unsigned int resources ) { _resources = resources; }
		void DeclareEntity ( Entity ent );
		void AddAlly ( unsigned int ally );
		void DeclareClass ( ShipClass aClass );
		void RemoveEntity ( unsigned int id );
		void Recalculate ();
		Action NextAction ();
	};
}
	