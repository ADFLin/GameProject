#ifndef ZumaGame_h__
#define ZumaGame_h__


#include "GameLoop.h"
#include "Win32Platform.h"
#include "SysMsgHandler.h"

#include "ZTask.h"
#include "ZUISystem.h"
#include "AudioPlayer.h"
#include "ZStage.h"
#include "ZLevelManager.h"
#include "ZParticle.h"

#include <fstream>

#include "DebugSystem.h"

namespace Zuma
{
	class LevelStage;
	struct ZComboInfo;

	class LogRecorder : public IMsgListener
	{
	public:
		LogRecorder( char const* logPath )
		{
			stream.open( logPath );
			addChannel( MSG_ERROR );
			addChannel( MSG_WARNING );
		}

		virtual void receive( MsgChannel channel , char const* str )
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


	class GameInitialer
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
		bool   init( GameInitialer& initilizer );
		void   update( long time );
		void   render( float dFrame );
		void   cleanup();

		bool   onEvent( int evtID , ZEventData const& data , ZEventHandler* sender );

		bool   onUIEvent( int evtID , int uiID , ZWidget* ui );
		void   onStageEnd( ZStage* stage , unsigned flag  );

		bool   onMouse( MouseMsg const& msg );
		bool   onKey( unsigned key , bool isDown );
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
		           , public GameLoopT< ZumaGame , Win32Platform >
		           , public SysMsgHandlerT< ZumaGame , MSG_ZUMA >
				   , public GameInitialer
	{
	public:
		ZumaGame(){}
		//GameLoop
		bool   onInit();
		void   onRender(){ GameCore::render( 0.0f ); }
		void   onEnd(){ GameCore::cleanup(); }
		void   onIdle( long time ){ onRender();  }
		long   onUpdate( long time ){ GameCore::update( time ); return time; }
		//SysMsgHandler
		bool   onMouse( MouseMsg const& msg ){ return GameCore::onMouse( msg );}
		bool   onKey( unsigned key , bool isDown ){ return GameCore::onKey( key , isDown ); }
		bool   onActivate( bool beA ){ isGmaePause = !beA; return true; }
	};

}//namespace Zuma

#endif // ZumaGame_h__
