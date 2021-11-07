#ifndef TinyGameApp_h__
#define TinyGameApp_h__

#include "GameLoop.h"
#include "WindowsPlatform.h"
#include "WindowsMessageHander.h"

#include "DrawEngine.h"
#include "StageBase.h"
#include "TaskBase.h"

#include "GameModuleManager.h"
#include "GameControl.h"
#include "GameGUISystem.h"
#include "GameWidget.h"

#include <cassert>

class MouseMsg;
class DrawEngine;

struct UserProfile;

class StageBase;
class NetWorker;
class ServerWorker;
class ClientWorker;
class LevelMode;
enum  StageID;

class GameStageMode;
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
			int64 deltaTime = time - timeFrame;
			if ( deltaTime > 0 )
			{
				fpsSamples[idxSample] = 1000.0f * (frameCount) / deltaTime;
				timeFrame = time;
				frameCount = 0;

				++idxSample;
				if( idxSample == NUM_FPS_SAMPLES )
					idxSample = 0;

				mFPS = 0;
				for( int i = 0; i < NUM_FPS_SAMPLES; ++i )
					mFPS += fpsSamples[i];

				mFPS /= NUM_FPS_SAMPLES;
			}
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


class TinyGameApp : public GameLoopT< TinyGameApp , WindowsPlatform >
				  , public WindowsMessageHandlerT< TinyGameApp , MSG_DEUFLT | MSG_DATA >
				  , public StageManager
				  , public TaskListener
				  , public IGUIDelegate
				  , public IGameNetInterface
	              , public IGameWindowProvider
{
public:

	TinyGameApp();
	~TinyGameApp();

	//StageManager
	void        setTickTime( long time ){ GameLoop::setUpdateTime( time ); }

	//IGameNetInterface
	NetWorker*  getNetWorker(){ return mNetWorker; }
	NetWorker*  buildNetwork( bool beServer );	
	void        closeNetwork();

	//IGUIDelegate
	virtual void addGUITask(TaskBase* task, bool bGlobal) override;
	virtual void dispatchWidgetEvent(int event, int id, GWidget* ui) override;

	//IGameWindowSupporter
	virtual GameWindow& getMainWindow() override;
	virtual bool  reconstructWindow(GameWindow& window) override;
	virtual GameWindow* createWindow(Vec2i const& pos, Vec2i const& size, char const* title) override;
	bool createWindowInternal(GameWindow& window, int width , int height, TCHAR const* title);

protected:
	//StageManager
	StageBase*     createStage( StageID stageId );
	GameStageMode* createGameStageMode(StageID stageId);
	StageBase*     resolveChangeStageFail( FailReason reason );
	bool           initializeStage(StageBase* stage);
	void           postStageChange( StageBase* stage );
	void           prevStageChange();
	void           postStageEnd(StageBase* stage);
	//~StageManager

	void  importUserProfile();
	void  exportUserProfile();

public: 
	//GameLoop
	bool  initializeGame() CRTP_OVERRIDE;
	void  finalizeGame() CRTP_OVERRIDE;
	long  handleGameUpdate(long shouldTime) CRTP_OVERRIDE;
	void  handleGameRender() CRTP_OVERRIDE;
	void  handleGameIdle( long time ) CRTP_OVERRIDE;

	//WindowsMessageHandlerT
	bool  handleMouseEvent( MouseMsg const& msg ) CRTP_OVERRIDE;
	bool  handleKeyEvent(KeyMsg const& msg) CRTP_OVERRIDE;
	bool  handleCharEvent( unsigned code )  CRTP_OVERRIDE;
	bool  handleWindowActivation( bool beA ) CRTP_OVERRIDE;
	void  handleWindowPaint( HDC hDC ) CRTP_OVERRIDE;
	bool  handleWindowDestroy( HWND hWnd ) CRTP_OVERRIDE;

	// TaskHandler
	void  onTaskMessage( TaskBase* task , TaskMsg const& msg );


private:
	void               loadModules();
	void               render( float dframe );
	void               cleanup();

	ServerWorker*      createServer();
	ClientWorker*      createClinet();

	bool               mbLockFPS;
	GameWindow         mGameWindow;
	GameStageMode*     mStageMode;
	RenderEffect*      mRenderEffect;
	NetWorker*         mNetWorker;
	bool               mShowErrorMsg;
	FPSCalculator      mFPSCalc;

	enum class ConsoleShowMode
	{
		Screen,
		GUI,
		None ,

		Count ,
	};
	ConsoleShowMode mConsoleShowMode = ConsoleShowMode::None;
	void setConsoleShowMode(ConsoleShowMode mode);

	class ConsoleFrame* mConsoleWidget = nullptr;
	bool            mbInitializingStage = false;
};

#endif // TinyGameApp_h__
