#ifndef GameSettingHelper_h__
#define GameSettingHelper_h__

#include "GameModule.h"
#include "GamePlayer.h"

#include "Widget/GameRoomUI.h"

class ServerWorker;
class GameSettingPanel;
class PlayerListPanel;
class StageManager;
class DataSteamBuffer;
class GameStageBase;

class  NetRoomSettingHelper : public SettingHepler
{
public:
	TINY_API NetRoomSettingHelper();

	

	template < class T >
	T*  getWidget( int id )
	{
		T* ui = GUI::CastFast< T >( mSettingPanel->findChild( id ) );
		return ui;
	}

	void setupSetting( ServerWorker* server )
	{
		mServer = server;
		doSetupSetting( mServer != NULL );
	}

	TINY_API void importSetting( DataSteamBuffer& buffer );
	TINY_API void exportSetting( DataSteamBuffer& buffer );

	TINY_API void sendSlotStateSV();
	TINY_API bool addPlayerSV( PlayerId id );
	TINY_API void emptySlotSV( SlotId id , SlotState state );
	TINY_API void sendPlayerStatusSV();

	virtual void clearUserUI() = 0;
	virtual void setupGame( StageManager& manager, StageBase* gameStage ){}
	virtual void doSetupSetting( bool beServer ) = 0;
	virtual void doExportSetting( DataSteamBuffer& buffer ) = 0;
	virtual void doImportSetting( DataSteamBuffer& buffer ) = 0;

	virtual bool onWidgetEvent( int event ,int id , GWidget* widget ){ return true; }

	TINY_API void   addGUIControl( GWidget* ui );

	PlayerListPanel*  getPlayerListPanel(){ return mPlayerListPanel; }
	GameSettingPanel* getSettingPanel(){ return mSettingPanel; }
	TINY_API void              setMaxPlayerNum( int num );
protected:

	ServerWorker*     mServer;
	PlayerListPanel*  mPlayerListPanel;
	GameSettingPanel* mSettingPanel;
};
#endif // GameSettingHelper_h__
