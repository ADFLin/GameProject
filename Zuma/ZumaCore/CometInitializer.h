#ifndef ParticleInitializer_h__
#define ParticleInitializer_h__

#include "CometBase.h"

namespace Comet 
{
	class ZoneInitializer : public Initializer
	{
	public:
		ZoneInitializer( Zone* zone );
	protected:
		SharePtr< Zone > mZone;
	};

	class Position : public ZoneInitializer
	{	
	public:
		//virtual 
		void initialize( Particle& p );
	};

	class Velocity : public ZoneInitializer
	{
	public:
		//virtual 
		void initialize( Particle& p );
	};

	class Life : public Initializer
	{
	public:
		Life( TimeType max , TimeType min , IRandom* random = nullptr );

		virtual void initialize( Particle& p );
	private:
		TimeType mMaxValue;
		TimeType mMinValue;
		SharePtr< IRandom > mRandom;
	};

	class Color
	{





	};


	class Angle
	{



	};

}//namespace Comet


#endif // ParticleInitializer_h__
