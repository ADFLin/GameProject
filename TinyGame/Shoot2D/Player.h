#ifndef Player_h__
#define Player_h__

#include "Action.h"
#include "Object.h"
#include "Weapon.h"

#include "WindowsHeader.h"

#include <algorithm>

namespace Shoot2D
{

	class PlayerFlight : public Vehicle
	{
	public:
		PlayerFlight(Vec2D const& pos);
		void upgradeWeapon();
		void processCollision(Object& obj);
	};


	class LevelGen : public BulletGen
	{
	public:
		LevelGen(int level):m_level(level){}

		void generate( Weapon* weapon );

		void Levelup(){ m_level = std::min( m_level+ 1 , 7 ); }
		int  getLevel() const { return m_level; }
		void setLevel(int val) { m_level = val; }

		int m_level;
	};

	class PlayerAction : public Action
	{
	public:
		PlayerAction(BYTE* key)
			:m_key(key)
		{
		}

		void update( Object& obj , long time );
		static float fixSpeed(float speed, float maxSpeed )
		{
			return std::max( - maxSpeed , std::min( speed , maxSpeed ) );
		}
		bool KeyDown(unsigned key){ return m_key[key] > 1; }
		BYTE* m_key;

	};

}//namespace Shoot2D
#endif // Player_h__