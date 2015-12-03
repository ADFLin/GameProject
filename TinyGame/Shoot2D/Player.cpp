#include "Player.h"

#include "Common.h"
#include "ObjModel.h"
#include "Weapon.h"

#include <algorithm>

namespace Shoot2D
{

	void PlayerAction::update( Object& obj , long time )
	{
		::GetKeyboardState(m_key);

		bool left  = KeyDown('A');
		bool right = KeyDown('D');
		bool top   = KeyDown('W');
		bool down  = KeyDown('S');
		bool fire  = KeyDown('K');

		Vec2D speed = obj.getVelocity();
		float maxSpeed = 100;
		float acc = 6000 * time / 1000.0;
		if ( left && !right )
			speed.x -= acc;
		if ( right && !left )
			speed.x += acc;

		if ( (  left &&  right ) || 
			 ( !left && !right )  )
		{
			if ( speed.x > 0 ) 
				speed.x = std::max( 0.0f , speed.x - acc );
			else
				speed.x = std::min( 0.0f , speed.x + acc );
		}

		speed.x = fixSpeed( speed.x , maxSpeed );

		if ( top && !down )
			speed.y -= acc;
		if ( down && !top )
			speed.y += acc;

		if ( ( top && down )|| 
			( !top && !down )  )
		{
			if ( speed.y > 0 ) 
				speed.y = std::max( 0.0f , speed.y - acc );
			else
				speed.y = std::min( 0.0f , speed.y + acc );
		}

		speed.y = fixSpeed( speed.y , maxSpeed );
		obj.setVelocity( speed );

		if ( fire ) 
			static_cast< Vehicle& >( obj ).tryFire();
	}

	PlayerFlight::PlayerFlight( Vec2D const& pos ) 
		:Vehicle(MD_PLAYER,pos)
	{
		m_Team = TEAM_FRIEND;

		LevelGen* gen  = new LevelGen(7);
		Weapon* weapon = new Weapon(gen);
		weapon->bullet       = MD_BULLET_1;
		weapon->bulletSpeed  = 300;
		weapon->cdtime       = 800;
		weapon->maxFireCount = 4;
		weapon->maxSaveCount = 5;
		weapon->deleyTime    = 500;

		setWeapon( weapon );
	}

	void PlayerFlight::upgradeWeapon()
	{
		LevelGen* gen = (LevelGen*) weapon->gen;
		gen->Levelup();
	}

	void PlayerFlight::processCollision( Object& obj )
	{
		if ( obj.getModelId() == MD_LEVEL_UP )
		{
			upgradeWeapon();
		}
	}

	void LevelGen::generate( Weapon* weapon )
	{
		DirArrayGen gen( Vec2D(0,-1) );
		gen.offset = 5;

		float const offset1 = 0.10;
		float const offset2 = 0.25;
		float const offset3 = 0.4;

		switch ( m_level )
		{
		case 1:
			gen.num = 2;
			gen.generate( weapon );
			break;
		case 2:
			gen.num = 2;
			gen.setDifPos( Vec2D(10,0)  );
			gen.generate( weapon );
			gen.setDifPos( Vec2D(-10,0) );
			gen.generate( weapon );
			break;
		case 3:
			gen.num = 2;
			gen.generate( weapon );
			gen.setDir(  Vec2D( offset1,-1) );
			gen.generate( weapon );
			gen.setDir(  Vec2D(-offset1,-1) );
			gen.generate( weapon );
			break;
		case 4:
			gen.num = 4;
			gen.generate( weapon );
			gen.setDir(  Vec2D( offset1,-1) );
			gen.generate( weapon );
			gen.setDir(  Vec2D(-offset1,-1) );
			gen.generate( weapon );
			break;
		case 5:
			gen.num = 4;
			gen.generate( weapon );
			gen.setDir(  Vec2D( offset1,-1) );
			gen.generate( weapon );
			gen.setDir(  Vec2D(-offset1,-1) );
			gen.generate( weapon );

			gen.num = 2;
			gen.setDir(  Vec2D( offset2,-1) );
			gen.generate( weapon );
			gen.setDir(  Vec2D(-offset2,-1) );
			gen.generate( weapon );
			break;
		case 6:
			gen.num = 4;
			gen.generate( weapon );
			gen.setDir(  Vec2D( offset1,-1) );
			gen.generate( weapon );
			gen.setDir(  Vec2D(-offset1,-1) );
			gen.generate( weapon );
			gen.setDir(  Vec2D( offset2,-1) );
			gen.generate( weapon );
			gen.setDir(  Vec2D(-offset2,-1) );
			gen.generate( weapon );
			break;
		case 7:
			gen.num = 4;
			gen.generate( weapon );
			gen.setDir(  Vec2D( offset1,-1) );
			gen.generate( weapon );
			gen.setDir(  Vec2D(-offset1,-1) );
			gen.generate( weapon );
			gen.setDir(  Vec2D( offset2,-1) );
			gen.generate( weapon );
			gen.setDir(  Vec2D(-offset2,-1) );
			gen.generate( weapon );
			gen.setDir(  Vec2D( offset3,-1) );
			gen.generate( weapon );
			gen.setDir(  Vec2D(-offset3,-1) );
			gen.generate( weapon );
			break;
		}
	}

}//namespace Shoot2D