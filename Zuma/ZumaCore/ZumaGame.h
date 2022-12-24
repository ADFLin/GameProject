#ifndef ZumaGame_h__
#define ZumaGame_h__


#include "GameLoop.h"
#include "WindowsPlatform.h"
#include "WindowsMessageHandler.h"

#include "ZTask.h"
#include "ZUISystem.h"
#include "AudioPlayer.h"
#include "ZStage.h"
#include "ZLevelManager.h"
#include "ZParticle.h"

#include <fstream>

#include "LogSystem.h"

namespace Zuma
{
	class LevelStage;
	struct ZComboInfo;

	class LogRecorder : public LogOutput
	{
	public:
		LogRecorder( char const* logPath )
		{
			stream.open( logPath );
			addChannel( LOG_ERROR );
			addChannel( LOG_WARNING );
		}

		virtual void receiveLog( LogChannel channel , char const* str )
		{
			if ( stream )
				stream << str << "\n";
		}
		std::ofstream stream;
	};

	enum ModeState
	{
		MS_ADV_GROUP_FINISH ,
		MS_ADV_NEXT_LEVEL ,
		MS_MODE_END ,
	};

	enum ZLevelStats
	{
		ZLS_FINISH ,
		ZLS_FAIL   ,
		ZLS_NOLIFE ,
	};

	class ZLevelMode : public ZEventHandler
	{
	public:

		virtual LevelStage* createCurLevelStage() = 0;
		virtual LevelStage* createNextLevelStage() = 0;
		virtual ModeState   getCurLevelFlag() = 0;

		bool  onEvent( int evtID , ZEventData const& data , ZEventHandler* sender )
		{
			return true;
		}
	};

	class ZAdventureMode : public ZLevelMode
	{
	public:
		ZAdventureMode()
		{

		}
		ZAdventureMode( unsigned idStage , int idxLv );

		LevelStage* createCurLevelStage()
		{  return createLevelStage( mIdStage , mIdxLv ); }
		LevelStage* createNextLevelStage()
		{  return createLevelStage( mIdStage , mIdxLv + 1 ); }

		ModeState   getCurLevelFlag();
		size_t      getLevelNum(){ return mLvList.size(); }
		ZLevelInfo& getLevelInfo( int idxLv ){ return mLvList[ idxLv ];}

		void        setStage( int idStage )
		{ 
			mIdStage = idStage; 
		}

	private:
		LevelStage* createLevelStage( unsigned idStage , int idxLv );

		LevelInfoVec  mLvList;
		unsigned      mIdStage;
		int           mIdxLv;

	};


	class GameInitializer
	{
	public:
		virtual bool           setupWindow( char const* title , int w , int h ) = 0;
		virtual IRenderSystem* createRenderSystem() = 0;
	};

	class GameCore :  public ZStageController
	{

	public:
		GameCore();
		~GameCore();
		bool   init( GameInitializer& initializer );
		void   update( long time );
		void   render( float dFrame );
		void   cleanup();

		bool   onEvent( int evtID , ZEventData const& data , ZEventHandler* sender );

		bool   onUIEvent( int evtID , int uiID , ZWidget* ui );
		void   onStageEnd( ZStage* stage , unsigned flag  );

		MsgReply   onMouse( MouseMsg const& msg );
		MsgReply   onKey( KeyMsg const& msg );
		ZUserData&  getUserData()    { return mUserData; }

		void    changeLevel( LevelStage* stage );
		void    applySetting();
		void    showComboText( ZComboInfo& info );


		ZUISystem      uiSystem;
		AudioPlayer    audioPlayer;
		ResManager     resManager;

		LogRecorder    logRecorder;
		ZSetting       mSetting;
		ZUserData      mUserData;
		ZLevelMode*    mLevelMode;

		bool            isGmaePause;
		float           showFPS;
		unsigned        renderFrame;

	};

#define MSG_ZUMA (MSG_DEUFLT| MSG_ACTIAVTE | MSG_DESTROY | MSG_PAINT )

	class ZumaGame : public GameCore
		           , public GameLoopT< ZumaGame , WindowsPlatform >
		           , public WindowsMessageHandlerT< ZumaGame , MSG_ZUMA >
				   , public GameInitializer
	{
	public:
		ZumaGame(){}
		//GameLoop
		bool   initializeGame() CRTP_OVERRIDE;
		void   finalizeGame() CRTP_OVERRIDE { GameCore::cleanup(); }
		void   handleGameRender() CRTP_OVERRIDE { GameCore::render( 0.0f ); }	
		void   handleGameIdle( long time ) CRTP_OVERRIDE { handleGameRender();  }
		long   handleGameUpdate( long time ) CRTP_OVERRIDE { GameCore::update( time ); return time; }

		//SysMsgHandler
		MsgReply handleMouseEvent( MouseMsg const& msg ) CRTP_OVERRIDE { return GameCore::onMouse( msg );}
		MsgReply handleKeyEvent( KeyMsg const& msg ) CRTP_OVERRIDE { return GameCore::onKey( msg ); }
		bool   handleWindowActivation( bool beA ) CRTP_OVERRIDE { isGmaePause = !beA; return true; }
	};

}//namespace Zuma

#endif // ZumaGame_h__
