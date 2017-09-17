#include "GreedySnakePCH.h"
#include "GreedySnakeGame.h"

#include "GreedySnakeStage.h"
#include "GreedySnakeScene.h"
#include "GreedySnakeMode.h"

#include "GameSettingHelper.h"
#include "GameRoomUI.h"

namespace GreedySnake
{

	StageBase* GameInstance::createStage( unsigned id )
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

	bool GameInstance::getAttribValue( AttribValue& value )
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
			mController.initKey( ACT_GS_MOVE_E , KEY_ONCE , 'D' , VK_RIGHT );
			mController.initKey( ACT_GS_MOVE_N , KEY_ONCE , 'W' , VK_UP    );
			mController.initKey( ACT_GS_MOVE_S , KEY_ONCE , 'S' , VK_DOWN  );
			mController.initKey( ACT_GS_MOVE_W , KEY_ONCE , 'A' , VK_LEFT  );
			mController.initKey( ACT_GS_CHANGE_DIR, KEY_ONCE, 'E');
			return true;
		}
		return false;
	}

	void GameInstance::beginPlay( StageModeType type, StageManager& manger )
	{
		IGameInstance::beginPlay( type , manger );
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

		virtual void doExportSetting( DataSteamBuffer& buffer ){}
		virtual void doImportSetting( DataSteamBuffer& buffer ){}
	};

	SettingHepler* GameInstance::createSettingHelper( SettingHelperType type )
	{
		switch( type )
		{
		case SHT_NET_ROOM_HELPER: return new CNetRoomSettingHelper;
		}
		return NULL;
	}

}


EXPORT_GAME(GreedySnake::GameInstance);