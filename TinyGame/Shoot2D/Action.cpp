#include "Action.h"

#include "Common.h"
#include "Object.h"

namespace Shoot2D
{
	void PathMove::update( Object& obj , long time )
	{
		if ( nextIndex == INDEX_NONE )
		{
			computeNextMove( obj );
			return;
		}

		if ( nextIndex >= path.size() )
			return;

		float dt = float(time)/ 1000.0f;
		if ( dt > moveTime )
		{
			obj.setPos( path[nextIndex] );
			computeNextMove( obj );
		}
		else
			moveTime -= dt;
	}

	void PathMove::computeNextMove( Object& obj )
	{
		setNextIndex();

		if ( nextIndex >= path.size())
		{
			obj.setVelocity( Vec2D(0,0) );
			return;
		}

		Vec2D& nexPos = path[nextIndex];
		moveSpeed = nexPos - obj.getPos();

		float dist = sqrtf( moveSpeed.length2() );
		moveTime = dist / speed;

		if ( fabs( moveTime ) <= 0.000001 )
		{
			computeNextMove(obj);
			return;
		}

		moveSpeed = ( 1 / moveTime )* moveSpeed;

		obj.setVelocity( moveSpeed );
	}

	void PathMove::setNextIndex()
	{
		switch (flag)
		{
		case GO_BACK:
			if ( nextIndex == 0 )
			{
				nextIndex = 1;
				flag = GO_FRONT;
			}
			else --nextIndex;
			break;

		case GO_FRONT:
			if ( nextIndex >= path.size()-1 )
			{
				nextIndex = path.size() - 2;
				flag = GO_BACK;
			}
			else ++nextIndex;
			break;

		case CYCLE:
			nextIndex = ( nextIndex + 1 ) % path.size();
			break;

		case STOP:
			++nextIndex;
		}
	}

	void SampleAI::update( Object& obj , long time )
	{
		if ( move ) 
			move->update( obj , time );

		static_cast< Vehicle& >( obj ).tryFire();
	}

	void Follow::update( Object& obj , long time)
	{
		Vec2D dif = destObj->getPos() - getCenterPos( obj );
		float s= sqrtf( dif.length2() );
		obj.setVelocity(  speed * time / (1000 *s )  * dif );
	}

	void Dying::update( Object& obj , long time )
	{
		tickTime += time;
		if ( tickTime > totalTime )
		{
			obj.setStats( STATS_DEAD );
		}
	}

	CircleMove::CircleMove( Vec2D const& pos ,float r ,float speed , bool clockwise /*= true*/ ) 
		:Move( speed )
		,org( pos )
		,radius(r)
		,isClockwise(clockwise)
	{

	}

	void CircleMove::update( Object& obj , long time )
	{
		float len = m_speed * time / 1000.0f;
		Vec2D dir = getCenterPos( obj ) - org;
		float d   =  sqrtf( dir.length2() );

		if ( d < 0.001 )
			return;

		if( fabs( radius - d ) > len )
		{
			if ( d > radius )
			{
				obj.setVelocity( (-len / d)* dir );
				return;
			}
			else
			{
				obj.setVelocity( (len / d) * dir );
				return;
			}
		}

		float cos_ = ( radius*radius + d*d - len*len ) / ( 2 * radius * d );
		if ( fabs( cos_ ) > 1 )
			return;

		float theta = acos( cos_ );
		Vec2D rDir = ( radius / d ) * dir;
		Vec2D dotDir( -rDir.y , rDir.x );
		if ( !isClockwise )
			dotDir = - dotDir;
		Vec2D outSpeed = ( cos( theta )* rDir)  + ( sin( theta ) * dotDir) - dir;

		obj.setVelocity( outSpeed );
	}

}//namespace Shoot2D