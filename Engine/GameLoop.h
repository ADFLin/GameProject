#ifndef GameLoop_h__
#define GameLoop_h__

#include "Core/CRTPCheck.h"
#include "Clock.h"

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
		mIsOver = false;
		mFrameTime  = 0;
		mUpdateTime = 15;
		mBusyTime = 200;
	}

	void setUpdateTime( long  time ){ mUpdateTime = time; }
	void setBusyTime(long time) { mBusyTime = time; }
	long getUpdateTime(){ return mUpdateTime; }
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
	long  mFrameTime;
	long  mUpdateTime;
	long  mBusyTime;
	bool  mIsOver;
	//long m_MaxLoops;
};

template < class T  , class PP  >
void GameLoopT< T , PP >::run()
{
	mClock.reset();
	if ( !_this()->initializeGame() )
		return;

	uint64 beforeTime = mClock.getTimeMilliseconds();
	unsigned numLoops=0;
	while( !mIsOver )
	{
		uint64  presentTime = mClock.getTimeMilliseconds();
		uint64  intervalTime = presentTime - beforeTime;

		if ( intervalTime < mUpdateTime )
		{
			_this()->handleGameIdle( mUpdateTime - intervalTime );

		}
		else
		{
			if( intervalTime > mBusyTime )
			{
				_this()->handleGameLoopBusy(intervalTime);
				beforeTime = presentTime - mUpdateTime;
				intervalTime = mUpdateTime;
			}

			if( !Platform::updateSystem() )
			{
				mIsOver = true;
			}

			if( mIsOver )
				break;

			long updateTime = _this()->handleGameUpdate(intervalTime);
			_this()->handleGameRender();

			beforeTime += updateTime;
			mFrameTime += updateTime;
			++numLoops;
		}
	}

	_this()->finalizeGame();
}


#endif // GameLoop_h__



      
      
