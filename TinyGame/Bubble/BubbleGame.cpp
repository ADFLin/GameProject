#include "BubblePCH.h"
#include "BubbleGame.h"

#include "BubbleStage.h"
#include "GameReplay.h"


#include "GameSettingHelper.h"
#include "GameClient.h"

namespace Bubble
{
	EXPORT_GAME_MODULE(GameModule)

	void BubbleReplayTemplate::registerNode( OldVersion::Replay& replay )
	{
		replay.registerNode( 0 , sizeof( ActionBitNode ) );
		replay.registerNode( 1 , sizeof( MouseActionBitNode ) );
	}

	bool BubbleReplayTemplate::getReplayInfo( ReplayInfo& info )
	{
		info.name = BUBBLE_NAME;
		info.gameVersion     = MAKE_VERSION(0,0,1);
		info.templateVersion = BubbleReplayTemplate::Version;
		info.setGameData( sizeof( BubbleGameInfo ) );
		return true;
	}

	void BubbleReplayTemplate::startFrame()
	{
		for( int i = 0 ; i < gBubbleMaxPlayerNum ; ++i )
			mActionBit[i] = 0;
	}

	size_t BubbleReplayTemplate::inputNodeData( unsigned id , char* data )
	{
		switch( id )
		{
		case 0:
			{
				ActionBitNode* node = reinterpret_cast< ActionBitNode* >( data );
				mActionBit[ node->pID ] = node->actionBit;
			}
			return 0;
		case 1:
			{
				MouseActionBitNode* node = reinterpret_cast< MouseActionBitNode* >( data );
				mActionBit[ node->pID ] = node->actionBit;
				mMoveOffset[ node->pID ] = node->offset;
			}
			return 0;
		}
		return 0;
	}

	bool BubbleReplayTemplate::checkAction( ActionParam& param )
	{
		switch( param.act )
		{
		case ACT_BB_ROTATE_LEFT:
		case ACT_BB_ROTATE_RIGHT:
		case ACT_BB_SHOOT:
			return ( mActionBit[ param.port  ] & BIT( param.act ) ) != 0;
		case ACT_BB_MOUSE_ROTATE:
			if ( mActionBit[ param.port  ] & BIT( param.act ) )
			{
				param.getResult< int >() = mMoveOffset[ param.port ];
				return true;
			}
			return false;
		}
		return false;
	}

	void BubbleReplayTemplate::listenAction( ActionParam& param )
	{
		switch( param.act )
		{
		case ACT_BB_ROTATE_LEFT:
		case ACT_BB_ROTATE_RIGHT:
		case ACT_BB_SHOOT:
			mActionBit[ param.port ] |= BIT( param.act );
			break;
		case ACT_BB_MOUSE_ROTATE:
			mActionBit[ param.port ] |= BIT( param.act );
			mMoveOffset[ param.port ] = param.getResult< int >();
			break;
		}
	}

	void BubbleReplayTemplate::recordFrame( OldVersion::Replay& replay , unsigned frame )
	{
		for( int i = 0 ; i < gBubbleMaxPlayerNum ; ++i )
		{
			if ( mActionBit[i] & BIT( ACT_BB_MOUSE_ROTATE ) )
			{
				MouseActionBitNode node;
				node.pID       = i;
				node.actionBit = mActionBit[i];
				node.offset    = mMoveOffset[i];
				replay.addNode( frame , 1 , &node );
			}
			else
			{
				ActionBitNode node;
				node.pID = i;
				node.actionBit = mActionBit[i];
				replay.addNode( frame , 0 , &node );
			}
		}
	}

	bool Controller::checkAction( ActionParam& param )
	{
		switch( param.act )
		{
		case ACT_BB_SHOOT:
			{
				if ( checkActionKey( param ) )
					return true;
				MouseMsg* msg = getMouseEvent( LBUTTON );
				if ( msg && msg->onLeftDown() )
					return true;
			}
			return false;
		case ACT_BB_MOUSE_ROTATE:
			{
				MouseMsg* msg;
				msg = getMouseEvent( RBUTTON );
				if ( msg && msg->onRightDown() )
				{
					oldX = msg->x();
				}

				msg = getMouseEvent( MOVING );
				if ( !msg )
					return false;

				if ( msg->isRightDown() )
				{
					param.getResult< int >() = msg->x() - oldX;
					oldX = msg->x();
					return true;
				}
			}
			return false;
		default:
			return checkActionKey( param );
		}

		return false;
	}


	ReplayTemplate* GameModule::createReplayTemplate( unsigned version )
	{
		return new BubbleReplayTemplate;
	}

	StageBase* GameModule::createStage( unsigned id )
	{
		switch( id )
		{
#define CASE_STAGE( ID , TYPE ) \
	case ID : return new TYPE;
		CASE_STAGE( STAGE_SINGLE_GAME , LevelStage )
		CASE_STAGE( STAGE_REPLAY_GAME , LevelStage )
		CASE_STAGE( STAGE_NET_GAME , LevelStage )
#undef  CASE_STAGE
		}
		return NULL;
	}

	bool GameModule::queryAttribute( GameAttribute& value )
	{
		switch ( value.id )
		{
		case ATTR_NET_SUPPORT:
		case ATTR_SINGLE_SUPPORT:
			value.iVal = 1;
			return true;
		case ATTR_CONTROLLER_DEFUAULT_SETTING:
			mController.clearAllKey();
			mController.initKey( ACT_BB_ROTATE_LEFT   ,        1 , EKeyCode::A , EKeyCode::Left  );
			mController.initKey( ACT_BB_ROTATE_RIGHT  ,        1 , EKeyCode::D , EKeyCode::Right );
			mController.initKey( ACT_BB_SHOOT         , KEY_ONCE , EKeyCode::W ,  EKeyCode::Up    );
			return true;
		}
		return false;
	}

	class BuNetRoomSettingHelper : public NetRoomSettingHelper
	{
	public:
		virtual void          clearUserUI()
		{

		}
		virtual void          doSetupSetting( bool beServer )
		{

		}
		virtual void setupGame( StageManager& manager , StageBase* subStage ){}
		//for network
		virtual void doExportSetting( DataStreamBuffer& buffer ){}
		virtual void doImportSetting( DataStreamBuffer& buffer ){}
	};

	SettingHepler* GameModule::createSettingHelper( SettingHelperType type )
	{
		switch( type )
		{
		case SHT_NET_ROOM_HELPER: return new BuNetRoomSettingHelper;
		}
		return NULL;
	}

	void GameModule::beginPlay( StageManager& manger, EGameStageMode modeType )
	{
		changeDefaultStage(manger, modeType);
	}


}// namespace Bubble