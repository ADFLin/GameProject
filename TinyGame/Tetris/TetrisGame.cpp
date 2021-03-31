#include "TetrisPCH.h"
#include "TetrisGame.h"

#include "TetrisStage.h"

#include "GameWidgetID.h"

#include "TetrisLevelMode.h"
#include "TetrisGameInfo.h"
#include "TetrisSettingHelper.h"
#include "TetrisActionId.h"

#include "GameReplay.h"
#include "TetrisRecord.h"

namespace Tetris
{
	EXPORT_GAME_MODULE(GameModule)

	RecordManager& GetRecordManager()
	{
		static RecordManager manager;
		return manager;
	}

	class TetrisReplayTemplate : public ReplayTemplate
	{
	public:
		static uint32 const LastVersion = MAKE_VERSION(0,0,1);

		TetrisReplayTemplate();
		~TetrisReplayTemplate();

		void   registerNode( Replay& replay );

		void   startFrame();
		size_t inputNodeData( unsigned id , char* data );
		bool   checkAction( ActionParam& param );
		void   listenAction( ActionParam& param );

		void   registerNode( OldVersion::Replay& replay );
		void   recordFrame( OldVersion::Replay& replay , unsigned frame );
		bool   getReplayInfo( ReplayInfo& info );

		unsigned  mActionBit[ gTetrisMaxPlayerNum ];
	};

	ReplayTemplate* GameModule::createReplayTemplate( unsigned version )
	{
		return new TetrisReplayTemplate;
	}

	bool GameModule::queryAttribute( GameAttribute& value )
	{
		switch( value.id )
		{
		case ATTR_NET_SUPPORT:
		case ATTR_SINGLE_SUPPORT:
		case ATTR_REPLAY_SUPPORT:
			value.iVal = true;
			return true;
		case ATTR_CONTROLLER_DEFUAULT_SETTING:
			mController.clearAllKey();
			mController.initKey( ACT_MOVE_LEFT  , 2 , EKeyCode::A , EKeyCode::Left);
			mController.initKey( ACT_MOVE_RIGHT , 2 , EKeyCode::D , EKeyCode::Right);
			mController.initKey( ACT_MOVE_DOWN  , 1 , EKeyCode::S , EKeyCode::Down);
			mController.initKey( ACT_ROTATE_CW  , KEY_ONCE , EKeyCode::K , EKeyCode::Num1 );
			mController.initKey( ACT_ROTATE_CCW , KEY_ONCE , EKeyCode::L , EKeyCode::Num2 );
			mController.initKey( ACT_HOLD_PIECE , KEY_ONCE , EKeyCode::I , EKeyCode::Num3 );
			mController.initKey( ACT_FALL_PIECE , KEY_ONCE , EKeyCode::J , EKeyCode::Num0 );
			return true;
		}
		return false;
	}




	struct ActionBitNode
	{
		unsigned pID : 4;
		unsigned actionBit : 28;
	};

	TetrisReplayTemplate::TetrisReplayTemplate()
	{


	}

	TetrisReplayTemplate::~TetrisReplayTemplate()
	{

	}

	void TetrisReplayTemplate::registerNode( OldVersion::Replay& replay )
	{
		replay.registerNode( 0 , sizeof( ActionBitNode ) );
	}

	void TetrisReplayTemplate::startFrame()
	{
		for( int i = 0 ; i < gTetrisMaxPlayerNum ; ++i )
			mActionBit[i] = 0;
	}

	size_t TetrisReplayTemplate::inputNodeData( unsigned id , char* data )
	{
		assert( id == 0 );
		ActionBitNode* node = reinterpret_cast< ActionBitNode* >( data );
		mActionBit[ node->pID ] = node->actionBit;
		return 0;
	}

	bool TetrisReplayTemplate::checkAction( ActionParam& param )
	{
		return ( mActionBit[ param.port ] & BIT( param.act ) ) != 0;
	}

	void TetrisReplayTemplate::listenAction( ActionParam& param )
	{
		mActionBit[ param.port ] |= BIT( param.act );
	}

	void TetrisReplayTemplate::recordFrame( OldVersion::Replay& replay , unsigned frame )
	{
		for( int i = 0 ; i < gTetrisMaxPlayerNum; ++i )
		{
			if ( mActionBit[i] == 0 )
				continue;

			ActionBitNode node;
			node.pID = i;
			node.actionBit = mActionBit[i];
			replay.addNode( frame , 0 , &node );
		}
	}

	bool TetrisReplayTemplate::getReplayInfo( ReplayInfo& info )
	{
		info.name = TETRIS_NAME;
		info.gameVersion     = GameInfo::LastVersion;
		info.templateVersion = TetrisReplayTemplate::LastVersion;
		info.setGameData( sizeof(  GameInfo ) );
		return true;
	}


	SettingHepler* GameModule::createSettingHelper( SettingHelperType type )
	{
		switch( type )
		{
		case SHT_NET_ROOM_HELPER:
			return new CNetRoomSettingHelper;
		}
		return NULL;
	}

	StageBase* GameModule::createStage( unsigned id )
	{
		switch( id )
		{
		case STAGE_GAME_MENU: return new MenuStage;
		case STAGE_ABOUT_GAME: return new AboutGameStage;
		case STAGE_RECORD_GAME: return new RecordStage;
#define CASE_STAGE( ID , TYPE ) \
	case ID : return new TYPE;
			CASE_STAGE(STAGE_SINGLE_GAME, LevelStage)
			CASE_STAGE(STAGE_REPLAY_GAME, LevelStage)
			CASE_STAGE(STAGE_NET_GAME, LevelStage)
#undef  CASE_STAGE

		}
		return NULL;
	}


	void GameModule::beginPlay( StageManager& manger, EGameStageMode modeType )
	{
		LogMsg( "Run Tetris!!!" );
		if( modeType == EGameStageMode::Single )
		{
			manger.changeStage(STAGE_GAME_MENU);
		}
		else
		{
			changeDefaultStage(manger, modeType);
		}
	}

	GameModule::GameModule()
	{

	}

	void GameModule::enter()
	{
		GetRecordManager().init();
	}

	void GameModule::exit()
	{
		GetRecordManager().saveFile( "record.dat" );
		GetRecordManager().clear();
	}


}//namespace Tetris


