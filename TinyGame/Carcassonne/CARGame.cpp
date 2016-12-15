#include "CARGame.h"

#include "CARStage.h"
#include "CARExpansion.h"

#include "StageBase.h"
#include "GameServer.h"
#include "GameSettingHelper.h"
#include "GameRoomUI.h"

#include "GameSettingPanel.h"
#include "GameRoomUI.h"
#include "GameWidgetID.h"
#include "DataSteamBuffer.h"

namespace CAR
{
	EXPORT_GAME(GameInstance)

	GameInstance::GameInstance()
	{
		
	}

	GameInstance::~GameInstance()
	{

	}

	StageBase* GameInstance::createStage( unsigned id )
	{
		if( id == STAGE_NET_GAME || id == STAGE_SINGLE_GAME )
		{
			return new CAR::LevelStage;
		}
		return NULL;
	}

	bool GameInstance::getAttribValue( AttribValue& value )
	{
		switch( value.id )
		{
		case ATTR_NET_SUPPORT:
			value.iVal = true;
			return true;
		case ATTR_SINGLE_SUPPORT:
			value.iVal = true;
			break;
		}
		return false;
	}

	void GameInstance::beginPlay( StageModeType type, StageManager& manger )
	{
		IGameInstance::beginPlay( type , manger );
	}

	struct ExpInfo
	{
		int exp;
		char const* name;
	};

	ExpInfo UsageExp[] = 
	{
		{ EXP_INNS_AND_CATHEDRALS , "Inns And Cathedrals" } ,
		{ EXP_TRADERS_AND_BUILDERS , "Traders And Builders" } ,
		{ EXP_THE_PRINCESS_AND_THE_DRAGON , "The Princess And The Dragon" } ,
		{ EXP_THE_TOWER , "The Tower" } ,
		{ EXP_ABBEY_AND_MAYOR , "Abbey And Mayor" } ,
		{ EXP_KING_AND_ROBBER , "King And Robber" } ,
		{ EXP_THE_RIVER , "The River" } ,
		{ EXP_THE_RIVER_II , "The River II" } ,
		{ EXP_BRIDGES_CASTLES_AND_BAZAARS , "Bridges , Castles And Bazaars" } , 
	};

	class CNetRoomSettingHelper : public NetRoomSettingHelper
	{
	public:
		CNetRoomSettingHelper( GameInstance* game )
			:mGame( game )
		{
			mExpMask = 0;
		}

		enum
		{
			UI_RULE_CHOICE = UI_GAME_ID ,

			UI_EXP ,
			UI_EXP_END = UI_EXP + NUM_EXPANSIONS ,

		};
		enum
		{
			MASK_BASE = BIT(1) ,
			MASK_RULE = BIT(2) ,
		};
		virtual bool checkSettingVaildSV()
		{
			SVPlayerManager* playerMgr = static_cast< SVPlayerManager* >( getPlayerListPanel()->getPlayerManager() );
			return true;
		}
		virtual void clearUserUI()
		{
			getSettingPanel()->removeGui( MASK_BASE | MASK_RULE );
		}
		virtual void doSetupSetting( bool beServer )
		{
			mExpMask = 0;
			setMaxPlayerNum( CAR::MaxPlayerNum );
			setupBaseUI();
		}

		virtual bool onWidgetEvent( int event ,int id , GWidget* widget )
		{
			if ( UI_EXP <= id && id < UI_EXP_END )
			{
				if ( widget->cast< GCheckBox >()->isCheck )
				{
					mExpMask |= BIT( id - UI_EXP );
				}
				else
				{
					mExpMask &= ~BIT( id - UI_EXP );
				}
				modifyCallback( getSettingPanel() );
				return false;
			}
			return true;
		}

		void setupBaseUI()
		{
			GCheckBox* checkBox;
			int id = UI_EXP;
			for( int i = 0 ; i < ARRAY_SIZE( UsageExp ) ; ++i )
			{
				checkBox = mSettingPanel->addCheckBox( UI_EXP + UsageExp[i].exp , UsageExp[i].name , MASK_BASE );
				checkBox->isCheck = ( mExpMask & BIT( UsageExp[i].exp ) ) != 0;
			}
		}

		virtual void setupGame( StageManager& manager , StageBase* subStage )
		{
			LevelStage* myStage = static_cast< LevelStage* >( subStage );
			myStage->getSetting().mExpansionMask = mExpMask;
		}
		virtual void doExportSetting( DataSteamBuffer& buffer )
		{
			buffer.fill( mExpMask );
		}
		virtual void doImportSetting( DataSteamBuffer& buffer )
		{
			getSettingPanel()->removeGui( MASK_BASE | MASK_RULE );
			getSettingPanel()->adjustGuiLocation();

			buffer.take( mExpMask );
			setupBaseUI();
		}
		uint32        mExpMask;
		GameInstance* mGame;
	};

	SettingHepler* GameInstance::createSettingHelper( SettingHelperType type )
	{
		switch( type )
		{
		case SHT_NET_ROOM_HELPER:
			return new CNetRoomSettingHelper( this );
		}
		return NULL;
	}

}//namespace CAR

