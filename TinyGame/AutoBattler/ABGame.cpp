#include "ABPCH.h"
#include "ABGame.h"
#include "ABStage.h"
#include "GameSettingHelper.h"
#include "NetGameMode.h"

namespace AutoBattler
{
	EXPORT_GAME_MODULE(GameModule)

	class CRoomSettingHelper : public NetRoomSettingHelper
	{
	public:
		
		virtual void clearUserUI() override
		{
		}

		virtual void doSetupSetting( bool beServer ) override
		{
			setMaxPlayerNum( AB_MAX_PLAYER_NUM );
		}

		virtual void doExportSetting( DataStreamBuffer& buffer ) override
		{
		}

		virtual void doImportSetting( DataStreamBuffer& buffer ) override
		{
		}
	};

	SettingHepler* GameModule::createSettingHelper( SettingHelperType type )
	{
		if ( type == SHT_NET_ROOM_HELPER )
			return new CRoomSettingHelper;
		return NULL;
	}

	StageBase* GameModule::createStage( unsigned id )
	{
		switch( id )
		{
		case STAGE_SINGLE_GAME:
		case STAGE_NET_GAME:
			return new LevelStage;
		}
		return NULL;
	}

	bool GameModule::queryAttribute(GameAttribute& value)
	{

		switch (value.id)
		{
		case ATTR_NET_SUPPORT:
		case ATTR_SINGLE_SUPPORT: value.iVal = 1; return true;
		}
		return false;
	}

	void GameModule::beginPlay(StageManager& manger, EGameStageMode modeType)
	{
		if (modeType == EGameStageMode::Net)
		{
			manger.changeStage(STAGE_NET_GAME);
		}
		else
		{
			manger.changeStage(STAGE_SINGLE_GAME);
		}
	}

}//namespace AutoBattler
