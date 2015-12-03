#ifndef Weapon_h__
#define Weapon_h__

#include "CommonFwd.h"

namespace Shoot2D
{

	class BulletGen
	{
	public:
		virtual void generate( Weapon* weapon ) = 0;
	};

	struct WeaponInfo
	{

	};


	class Weapon 
	{
	public:
		enum WeaponStats
		{
			WS_CD,
			WS_FIRING,
		};

		enum BulletType
		{
			BT_NORMAL,
			BT_MISSILE,
		};

		Object*    owner;
		Object*    destObj;
		float      bulletSpeed;
		Vec2D      difPos;
		ModelId    bullet;
		BulletType bulletType;
		BulletGen* gen;
		int        maxFireCount;
		int        maxSaveCount;
		int        deleyTime;
		int        cdtime;

		Weapon(BulletGen* gen);
		~Weapon();

		WeaponStats getStats(){ return stats; }

		Object* createBullet();

		void update(long time);
		void fire();
		bool tryFire();
		void init();

	protected:

		int fireCount;
		int saveCount;
		int fireTime;
		int idleTime;

		void makeCD();
		WeaponStats stats;
	};


	class DirArrayGen : public BulletGen
	{
	public:
		DirArrayGen( Vec2D const& d );

		void generate( Weapon* weapon );
		void setDir( Vec2D const& d );
		void setDifPos(Vec2D val) { difPos = val; }
		size_t num;
		float offset;
		Vec2D difPos;
		Vec2D dir;
	};

	class SimpleGen : public BulletGen
	{
	public:
		void generate( Weapon* weapon );
	};

}//namespace Shoot2D


#endif // Weapon_h__
