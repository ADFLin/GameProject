#ifndef CometClock_h__
#define CometClock_h__

#include "CometBase.h"

namespace Comet 
{

	class ConstClock : public Clock
	{
	public:
		ConstClock( Real spawnSpeed )
			:mSpawnSpeed( spawnSpeed )
			,mRemaining( Real( 0.0 ) )
		{

		}
		//virtual
		int  tick( TimeType time ) 
		{
			mRemaining += mSpawnSpeed * time;
			int result = (int)mRemaining;
			mRemaining -= result;
			return result;
		}

		float  getSpawnSpeed() const { return mSpawnSpeed; }
	private:
		Real mRemaining;
		Real mSpawnSpeed;

	};

}//namespace Comet 

#endif // CometClock_h__
