#ifndef GameLoop_h__
#define GameLoop_h__

#include "Core/CRTPCheck.h"
#include "Clock.h"
#include "ProfileSystem.h"

class PlatformPolicy
{
public:
	bool updateSystem();
};


template < class T  ,  class PlatformPolicy  >
class GameLoopT : public PlatformPolicy   
{
	inline T* _this(){ return static_cast< T* >( this ); }
	typedef PlatformPolicy Platform;
public:
	typedef GameLoopT< T , PlatformPolicy > GameLoop;

	GameLoopT()
	{
		mIsOver     = false;
		mFrameTime  = 0;
		mUpdateTime = 1000000LLU / 10;
		mBusyTime   = 200000;
	}

	void setUpdateTime(float time){ mUpdateTime = uint64( 1000 * time ) ; }
	void setBusyTime(float time) { mBusyTime = uint64( 1000 * time ); }
	long getUpdateTime(){ return mUpdateTime / 1000; }
	void setLoopOver( bool beOver ){ mIsOver = beOver; }

	void run();
	
protected:

	CRTP_FUNC bool initializeGame(){ return true; }
	CRTP_FUNC void finalizeGame(){}
	CRTP_FUNC long handleGameUpdate( long shouldTime ){ return shouldTime; }
	CRTP_FUNC void handleGameIdle(long time){}
	CRTP_FUNC void handleGameRender(){}
	CRTP_FUNC void handleGameLoopBusy( long deltaTime ){}



	HighResClock mClock;
private:
	uint64 mFrameTime;
	uint64 mUpdateTime;
	uint64 mBusyTime;
	bool   mIsOver;
	//long m_MaxLoops;
};

template < class T  , class PP  >
void GameLoopT< T , PP >::run()
{
	mClock.reset();
	if ( !_this()->initializeGame() )
		return;

	uint64 beforeTime = mClock.getTimeMicroseconds();
	unsigned numLoops=0;
	while( !mIsOver )
	{
		uint64  presentTime = mClock.getTimeMicroseconds();
		uint64  intervalTime = presentTime - beforeTime;

		if ( intervalTime < mUpdateTime )
		{
			_this()->handleGameIdle( (mUpdateTime - intervalTime) / 1000 );
		}
		else
		{
			if( intervalTime > mBusyTime )
			{
				_this()->handleGameLoopBusy(intervalTime / 1000);
				beforeTime = presentTime - mUpdateTime;
				intervalTime = mUpdateTime;
			}

			{
				PROFILE_ENTRY("Platform::updateSystem");
				if (!Platform::updateSystem())
				{
					mIsOver = true;
				}
			}

			if( mIsOver )
				break;

			uint64 updateTime = 1000 * _this()->handleGameUpdate(intervalTime / 1000);
			_this()->handleGameRender();

			beforeTime += updateTime;
			mFrameTime += updateTime;
			++numLoops;
		}
	}

	_this()->finalizeGame();
}


#endif // GameLoop_h__



      
      
