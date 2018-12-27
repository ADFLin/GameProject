#include "Explosion.h"
#include "Level.h"

IMPL_OBJECT_CLASS( Explosion , OT_EXPLOSION , "Explsion" )

Explosion::Explosion()
{

}

Explosion::Explosion( Vec2f const& pos , float radius )
	:BaseClass( pos )
	,radius( radius )
{

}

Explosion::~Explosion()
{

}

void Explosion::init()
{
	BaseClass::init();

	mbDead=false;

	mCurIntensity=0;
	mMaxIntensity=4;
	mGrowthRate=35;
	mDeathRate=10;

	color=Vec3f(1.0, 0.75, 0.5);
}

void Explosion::onSpawn( unsigned flag )
{
	BaseClass::onSpawn( flag );

	mLight.host   = this;
	mLight.radius = radius;
	mLight.setColorParam(color, 0);
	mLight.isExplosion = true;
	getLevel()->addLight( mLight );
}

void Explosion::onDestroy( unsigned flag )
{
	mLight.remove();
	BaseClass::onDestroy( flag );
}

void Explosion::setParam(float intensity, float growthRate, float deathRate)
{
	mMaxIntensity = intensity*2;
	mGrowthRate = growthRate;
	mDeathRate = deathRate;
}

void Explosion::setColor( Vec3f const& c )
{
	this->color = c;
}

void Explosion::tick()
{

	if( mbDead )
	{
		if( mCurIntensity>0 )
			mCurIntensity -= mDeathRate * TICK_TIME;
		else
		{			
			destroy();
			mCurIntensity=0;
		}
	}
	else
	{
		if(mCurIntensity<mMaxIntensity)
			mCurIntensity += mGrowthRate * TICK_TIME;
		else
		{
			mbDead=true;
			mCurIntensity=mMaxIntensity;
		}
	}	
	mLight.intensity = mCurIntensity;	
}

