#ifndef GameLoop_h__
#define GameLoop_h__

class PlatformPolicy
{
public:
	long getMillionSecond();
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
		mBusyTime = 2000;
	}

	void setUpdateTime( long  time ){ mUpdateTime = time; }
	void setBusyTime(long time) { mBusyTime = time; }
	long getUpdateTime(){ return mUpdateTime; }
	void setLoopOver( bool beOver ){ mIsOver = beOver; }

	void run();
	
protected:
	void onIdle( long time )        {}
	bool onInit()                   { return true; }
	void onEnd()                    {}
	long onUpdate( long shouldTime ){ return shouldTime; }
	void onRender()                 {}
	void onLoopBusy( long deltaTime ){}

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
	if ( !_this()->onInit() ) 
		return;

	long beforeTime = Platform::getMillionSecond();
	unsigned numLoops=0;
	while( !mIsOver )
	{
		long  presentTime = Platform::getMillionSecond();
		long  intervalTime = presentTime - beforeTime;

		if ( intervalTime < mUpdateTime )
		{
			_this()->onIdle( mUpdateTime - intervalTime );
		}
		else if( intervalTime > mBusyTime )
		{
			_this()->onLoopBusy(intervalTime);
			beforeTime = presentTime;
		}
		else
		{
			if ( !Platform::updateSystem() )
			{
				mIsOver = true;
			}

			if ( mIsOver )
				break;

			long updateTime = _this()->onUpdate( intervalTime );
			_this()->onRender();

			beforeTime += updateTime;
			mFrameTime += updateTime;
			++numLoops;
		}
	}

	_this()->onEnd();
}


#endif // GameLoop_h__



      
      
