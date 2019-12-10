#include "GreedySnakePCH.h"
#include "GreedySnakeGame.h"

#include "GreedySnakeStage.h"
#include "GreedySnakeScene.h"
#include "GreedySnakeMode.h"

#include "GameSettingHelper.h"
#include "Widget/GameRoomUI.h"

namespace GreedySnake
{

	StageBase* GameModule::createStage( unsigned id )
	{
		switch( id )
		{
		case STAGE_REPLAY_GAME:
		case STAGE_SINGLE_GAME:
		case STAGE_NET_GAME:
			return new LevelStage;
		}
		return NULL;
	}

	bool GameModule::queryAttribute( GameAttribute& value )
	{
		switch( value.id )
		{
		case ATTR_NET_SUPPORT:
		case ATTR_SINGLE_SUPPORT:
			value.iVal = true;
			return true;
		case ATTR_AI_SUPPORT:
			value.iVal = true;
			return true;
		case ATTR_CONTROLLER_DEFUAULT_SETTING:
			mController.clearAllKey();
			mController.initKey( ACT_GS_MOVE_E , KEY_ONCE , EKeyCode::D , EKeyCode::Right );
			mController.initKey( ACT_GS_MOVE_N , KEY_ONCE , EKeyCode::W , EKeyCode::Up    );
			mController.initKey( ACT_GS_MOVE_S , KEY_ONCE , EKeyCode::S , EKeyCode::Down  );
			mController.initKey( ACT_GS_MOVE_W , KEY_ONCE , EKeyCode::A , EKeyCode::Left  );
			mController.initKey( ACT_GS_CHANGE_DIR, KEY_ONCE, EKeyCode::E);
			return true;
		}
		return false;
	}

	void GameModule::beginPlay( StageManager& manger, StageModeType modeType )
	{
		changeDefaultStage(manger, modeType);
	}

	class CNetRoomSettingHelper : public NetRoomSettingHelper
	{

	public:
		virtual void clearUserUI()
		{

		}
		virtual void doSetupSetting( bool beServer )
		{
			setMaxPlayerNum( BattleMode::MaxPlayerNum );
		}
		virtual void setupGame( StageManager& manager , StageBase* subStage )
		{

		}

		virtual void doExportSetting( DataStreamBuffer& buffer ){}
		virtual void doImportSetting( DataStreamBuffer& buffer ){}
	};

	SettingHepler* GameModule::createSettingHelper( SettingHelperType type )
	{
		switch( type )
		{
		case SHT_NET_ROOM_HELPER: return new CNetRoomSettingHelper;
		}
		return NULL;
	}

}


EXPORT_GAME_MODULE(GreedySnake::GameModule);