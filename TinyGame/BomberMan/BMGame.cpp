#include "BMPCH.h"
#include "BMGame.h"

#include "BMStage.h"
#include "BMAction.h"

#include "GameSettingHelper.h"
#include "GameRoomUI.h"
#include "GameSettingPanel.h"

EXPORT_GAME( BomberMan::CGamePackage )

namespace BomberMan
{
	GameSubStage* CGamePackage::createSubStage( unsigned id )
	{
		switch( id )
		{
		case STAGE_SINGLE_GAME:
		case STAGE_REPLAY_GAME:
		case STAGE_NET_GAME:
			return new LevelStage;
		}
		return NULL;
	}

	bool CGamePackage::getAttribValue( AttribValue& value )
	{
		switch( value.id )
		{
		case ATTR_NET_SUPPORT:
		case ATTR_SINGLE_SUPPORT:
			value.iVal = true;
			return true;
		case ATTR_REPLAY_SUPPORT:
		case ATTR_REPLAY_INFO_DATA:
			return false;
		case ATTR_CONTROLLER_DEFUAULT_SETTING:
			mController.initKey( ACT_BM_MOVE_LEFT  , 0 , 'A' , VK_LEFT );
			mController.initKey( ACT_BM_MOVE_RIGHT , 0 , 'D' , VK_RIGHT );
			mController.initKey( ACT_BM_MOVE_TOP   , 0 , 'W' , VK_UP );
			mController.initKey( ACT_BM_MOVE_DOWN  , 0 , 'S' , VK_DOWN );
			mController.initKey( ACT_BM_BOMB     , KEY_ONCE , 'L' );
			mController.initKey( ACT_BM_FUNCTION , KEY_ONCE , 'K' );
			return true;
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

		enum 
		{
			MASK_BASE = BIT(1) ,
		};

		enum
		{
			UI_WIN_ROUND  = UI_GAME_ID ,
			UI_PLAYER_NUMBER_CHOICE ,
		};
		virtual void clearUserUI()
		{
			getSettingPanel()->removeGui( MASK_BASE );
		}
		virtual void doSetupSetting( bool beServer )
		{
			IMapGenerator* mapGen = new CSimpleMapGen( Vec2i(15,13) , NULL );
			mMode = new BattleMode( *mapGen ) ;
			setMaxPlayerNum( 5 );

			if ( beServer )
				setupUI();
		}
		virtual void setupGame( StageManager& manager , GameSubStage* subStage )
		{
			static_cast< LevelStage* >( subStage )->setMode( mMode );
			mMode = NULL;
		}
		virtual void doSendSetting( DataStreamBuffer& buffer )
		{
			buffer.fill( (int)mMode->getId() );
			switch( mMode->getId() )
			{
			case MODE_BATTLE:
				{
					BattleMode::Info& info = static_cast< BattleMode* >( mMode )->getInfo();
					buffer.fill( info );
				}
				break;
			}

		}
		virtual void doRecvSetting( DataStreamBuffer& buffer )
		{
			getSettingPanel()->removeGui( MASK_BASE );

			int id;
			buffer.take( id );

			if ( id != mMode->getId() )
			{
				Mode* mode = NULL;
				switch( id )
				{
				case MODE_BATTLE: mode = new BattleMode( mMode->getMapGenerator() ); break;
				}

				assert( mode );
				delete mMode;
				mMode = mode;
			}

			switch( mMode->getId() )
			{
			case MODE_BATTLE:
				{
					BattleMode::Info& info = static_cast< BattleMode* >( mMode )->getInfo();
					buffer.take( info );
				}
				break;
			}

			setupUI();
		}

		virtual bool onEvent( int event ,int id , GWidget* widget )
		{ 
			switch( id )
			{
			case UI_WIN_ROUND:
				static_cast< BattleMode* >( mMode )->getInfo().numWinRounds = 
					GUI::castFast< GSlider* >( widget )->getValue();
				modifyCallback( getSettingPanel() );
				break;

			}
			return true; 
		}

		void setupUI()
		{
			GameSettingPanel* panel = getSettingPanel();

			switch( mMode->getId() )
			{
			case MODE_BATTLE:
				{
					BattleMode::Info& info = static_cast< BattleMode* >( mMode )->getInfo();
					GSlider* slider = panel->addSlider( UI_GAME_ID , "Win Rounds" , MASK_BASE );
					slider->setRange( 1 , 5 );
					slider->setValue( info.numWinRounds );
				}
				break;
			}
		}

	protected:
		Mode*          mMode;

	};

	SettingHepler* CGamePackage::createSettingHelper( SettingHelperType type )
	{
		switch( type )
		{
		case SHT_NET_ROOM_HELPER: return new CNetRoomSettingHelper;
		}
		return NULL;
	}

}//namespace BomberMan

