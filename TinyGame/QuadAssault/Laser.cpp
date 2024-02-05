#include "Laser.h"

#include "Level.h"
#include "Player.h"
#include "LaserBullet.h"


void Laser::init( Player* player )
{
	BaseClass::init( player );

	mCDSpeed=1500;
	mEnergyCast=0.1;
}

void Laser::tick()
{
	BaseClass::tick();
}

void Laser::doFire( FireHelper& helper )
{
	for(int i=0; i<1; i++)
	{
		Vec2f offset = -4 * helper.dir;
		helper.fireT< LaserBullet >( offset );		
	}
}