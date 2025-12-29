#include "ABPCH.h"
#include "ABGame.h"
#include "ABStage.h"
#include "ABNetwork.h"
#include "GameSettingHelper.h"
#include "NetGameMode.h"
#include "GameSettingPanel.h"
#include "DataStreamBuffer.h"

namespace AutoBattler
{
	EXPORT_GAME_MODULE(GameModule)

	class CRoomSettingHelper : public NetRoomSettingHelper
	{
		enum
		{
			UI_USE_BOT = UI_GAME_ID,
		};
		
		GCheckBox* mUIUseBot;
		bool mUseBot;

	public:
		CRoomSettingHelper()
		{
			mUIUseBot = NULL;
			mUseBot = true;
		}

		virtual void onClearUserUI() override
		{
			mUIUseBot = NULL;
		}

		virtual void doSetupSetting( bool beServer ) override
		{
			setMaxPlayerNum( AB_MAX_PLAYER_NUM );
			if (mSettingPanel)
			{
				mUIUseBot = mSettingPanel->addCheckBox(UI_USE_BOT, "Use Bot For Empty Slot");
				if (mUIUseBot)
					mUIUseBot->bChecked = mUseBot;
			}
			else
			{

			}
		}


		virtual bool onWidgetEvent(int event, int id, GWidget* widget) override
		{
			if (id == UI_USE_BOT)
			{
				mUseBot = mUIUseBot->bChecked;
				modifyCallback(getSettingPanel());
				return false;
			}
			return true;
		}

		virtual void doExportSetting( DataStreamBuffer& buffer ) override
		{
			if ( mUIUseBot )
				mUseBot = mUIUseBot->bChecked;

			buffer.fill( mUseBot );
		}


		virtual void setupGame(StageManager& manager, StageBase* stage) override
		{
			LevelStage* abStage = static_cast<LevelStage*>(stage);
			abStage->mUseBots = mUseBot;
		}

		virtual void doImportSetting( DataStreamBuffer& buffer ) override
		{
			buffer.take( mUseBot );
			if ( mUIUseBot )
				mUIUseBot->bChecked = mUseBot;
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

	void GameModule::beginPlay(StageManager& manger, EGameMode modeType)
	{
		if (modeType == EGameMode::Net)
		{
			manger.changeStage(STAGE_NET_GAME);
		}
		else
		{
			manger.changeStage(STAGE_SINGLE_GAME);
		}
	}

	INetEngine* GameModule::createNetEngine()
	{
		// Create ABNetEngine for dedicated server mode
		ABNetEngine* engine = new ABNetEngine();
		return engine;
	}

}//namespace AutoBattler

