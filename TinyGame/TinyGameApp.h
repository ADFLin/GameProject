#ifndef TinyGameApp_h__
#define TinyGameApp_h__

#include "GameLoop.h"
#include "Win32Platform.h"
#include "SysMsgHandler.h"

#include "DrawEngine.h"
#include "StageBase.h"
#include "TaskBase.h"

#include "GamePackageManager.h"
#include "GameControl.h"
#include "GameWidget.h"

#include <cassert>

class MouseMsg;
class DrawEngine;

struct UserProfile;

class StageBase;
class NetWorker;
class ServerWorker;
class ClientWorker;
class Mode;
enum  StageID;


class GameController;


class RenderEffect : public LifeTimeTask
{
public:
	RenderEffect( long time ) : LifeTimeTask( time ){}
	virtual void onRender( long dt ) = 0;
};

class FadeInEffect : public RenderEffect
{
public:
	FadeInEffect( int _color , long time );
	void   release(){ delete this; }
	void onRender( long dt );
	int  color;
	long totalTime;
};

class FPSCalculator
{
public:
	void  init( int64 time )
	{
		timeFrame = time;
		mFPS = 0.0f;
		NumFramePerSample = 5;
		std::fill_n( fpsSamples , NUM_FPS_SAMPLES , 60.0f );
		idxSample = 0;
		frameCount = 0;
	}

	float getFPS(){ return mFPS; }
	void  increaseFrame( int64 time )
	{
		++frameCount;
		if ( frameCount > NumFramePerSample )
		{
			fpsSamples[idxSample] = 1000.0f * ( frameCount ) / ( time - timeFrame );
			timeFrame = time;
			frameCount = 0;

			++idxSample;
			if ( idxSample == NUM_FPS_SAMPLES )
				idxSample = 0;

			mFPS = 0;
			for (int i = 0; i < NUM_FPS_SAMPLES; ++i)
				mFPS += fpsSamples[i];

			mFPS /= NUM_FPS_SAMPLES;
		}
	}

	static int const NUM_FPS_SAMPLES = 8;
	float fpsSamples[NUM_FPS_SAMPLES];
	int   NumFramePerSample;
	int   idxSample;
	int   frameCount;
	int64 timeFrame;
	float mFPS;
};


class TinyGameApp : public GameLoopT< TinyGameApp , Win32Platform >
				  , public SysMsgHandlerT< TinyGameApp , MSG_DEUFLT | MSG_DATA | MSG_DESTROY >
				  , public StageManager
				  , public TaskListener
{
public:

	TinyGameApp();
	~TinyGameApp();

	//StageManager
	void                setTickTime( long time ){ setUpdateTime( time ); }
	NetWorker*          getNetWorker(){ return mNetWorker; }
	NetWorker*          buildNetwork( bool beServer );

protected:
	StageBase*     createStage( StageID stageId );
	GameStage*     createGameStage( StageID stageId );
	StageBase*     onStageChangeFail( FailReason reason );
	void           postStageChange( StageBase* stage );
	void           prevChangeStage();

	void  exportUserProfile();
	void  importUserProfile();

public: 
	//GameLoop
	bool  onInit();
	void  onEnd();
	long  onUpdate( long shouldTime );
	void  onRender();
	void  onIdle( long time );

	//SysMsgHandler
	bool  onMouse( MouseMsg const& msg );
	bool  onKey( unsigned key , bool isDown );
	bool  onChar( unsigned code );
	bool  onActivate( bool beA );
	void  onPaint( HDC hDC ){  render( 0.0f );  }
	void  onDestroy();

	//EventHandler
	bool  onWidgetEvent( int event , int id , GWidget* ui );
	// TaskHandler
	void  onTaskMessage( TaskBase* task , TaskMsg const& msg );

private:
	void               loadGamePackage();
	void               render( float dframe );
	void               closeNetwork();
	ServerWorker*      createServer();
	ClientWorker*      createClinet();


	GameWindow         mGameWindow;
	RenderEffect*      mRenderEffect;
	NetWorker*         mNetWorker;
	bool               mShowErrorMsg;
	FPSCalculator      mFPSCalc;
};

#endif // TinyGameApp_h__
