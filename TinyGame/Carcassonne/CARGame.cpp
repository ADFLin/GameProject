#include "CARGame.h"

#include "CARStage.h"

#include "StageBase.h"
#include "GameServer.h"
#include "GameSettingHelper.h"
#include "GameRoomUI.h"

#include "GameSettingPanel.h"
#include "GameRoomUI.h"
#include "GameWidgetID.h"
#include "DataStreamBuffer.h"

namespace CAR
{

	CGamePackage::CGamePackage()
	{
		
	}

	CGamePackage::~CGamePackage()
	{

	}

	StageBase* CGamePackage::createStage( unsigned id )
	{
		return NULL;
	}

	GameSubStage* CGamePackage::createSubStage( unsigned id )
	{ 
		if ( id == STAGE_NET_GAME || id == STAGE_SINGLE_GAME )
		{
			return new CAR::LevelStage;
		}
		return NULL;
	}

	bool CGamePackage::getAttribValue( AttribValue& value )
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

	void CGamePackage::enter( StageManager& manger )
	{
		manger.changeStage( STAGE_SINGLE_GAME );
	}

	class CNetRoomSettingHelper : public NetRoomSettingHelper
	{
	public:
		CNetRoomSettingHelper( CGamePackage* game )
			:mGame( game ){}

		enum
		{
			UI_RULE_CHOICE = UI_GAME_ID ,

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
			setupBaseUI();
		}

		void setupBaseUI()
		{
			GChoice* choice = mSettingPanel->addChoice( UI_RULE_CHOICE , LAN("Game Rule") , MASK_BASE );
		}

		bool onWidgetEvent( int event ,int id , GWidget* widget )
		{
			switch( id )
			{
			case UI_RULE_CHOICE:
				getSettingPanel()->removeGui( MASK_RULE );
				getSettingPanel()->adjustGuiLocation();
				modifyCallback( getSettingPanel() );
				return false;
			}

			return true;
		}

		virtual void setupGame( StageManager& manager , GameSubStage* subStage )
		{

		}
		virtual void doSendSetting( DataStreamBuffer& buffer )
		{
			setMaxPlayerNum( CAR::MaxPlayerNum );

			//int rule = mGame->getRule();
			//buffer.fill( rule );

		}
		virtual void doRecvSetting( DataStreamBuffer& buffer )
		{
			getSettingPanel()->removeGui( MASK_BASE | MASK_RULE );
			//getSettingPanel()->adjustGuiLocation();
			//setupBaseUI();
		}
		CGamePackage* mGame;
	};

	SettingHepler* CGamePackage::createSettingHelper( SettingHelperType type )
	{
		switch( type )
		{
		case SHT_NET_ROOM_HELPER:
			return new CNetRoomSettingHelper( this );
		}
		return NULL;
	}

}//namespace CAR

EXPORT_GAME( CAR::CGamePackage )