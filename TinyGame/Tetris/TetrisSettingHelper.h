#ifndef TetrisSettingHelper_h__
#define TetrisSettingHelper_h__

#include "GameSettingHelper.h"
#include "GameWidget.h"

#include "TetrisGameInfo.h"

namespace Tetris
{
	class CNetRoomSettingHelper : public NetRoomSettingHelper
	{
		typedef NetRoomSettingHelper BaseClass;
	public:

		enum
		{
			UI_MODE_CHOICE  = UI_GAME_ID ,
			UI_PLAYER_NUMBER_CHOICE ,
			UI_GRAVITY_LEVEL_SLIDER ,
		};

		CNetRoomSettingHelper ()
		{

		}
		//SettingHepler
		void          clearUserUI();
		void          doSetupSetting( bool beServer );
		void          doSendSetting( DataStreamBuffer& buffer );
		void          doRecvSetting( DataStreamBuffer& buffer );

		enum 
		{
			MASK_BASE = BIT(1) ,
			MASK_MODE = BIT(2) ,
		};


		bool  onWidgetEvent( int event ,int id , GWidget* widget );

		void  setupBaseUI();
		void  setupNormalModeUI();


		void  setupMaxPlayerNumUI( int num );
		void  setupGame(StageManager& manager, GameStageBase* stage);
		int      mMaxPlayerNum;
		GameInfo mInfo;
	};

}//namespace Tetris


#endif // TetrisSettingHelper_h__
