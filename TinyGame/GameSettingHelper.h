#ifndef GameSettingHelper_h__
#define GameSettingHelper_h__

#include "GamePackage.h"
#include "GamePlayer.h"

#include "GameRoomUI.h"

class ServerWorker;
class GameSettingPanel;
class PlayerListPanel;
class StageManager;
class DataStreamBuffer;

class  NetRoomSettingHelper : public SettingHepler
{
public:
	GAME_API NetRoomSettingHelper();

	

	template < class T >
	T*  getUI( int id )
	{
		T* ui = GUI::castFast< T* >( mSettingPanel->findChild( id ) );
		return ui;
	}

	void setupSetting( ServerWorker* server )
	{
		mServer = server;
		doSetupSetting( mServer != NULL );
	}

	GAME_API void recvSetting( DataStreamBuffer& buffer );
	GAME_API void sendSetting( DataStreamBuffer& buffer );

	GAME_API void sendSlotStateSV();
	GAME_API void addPlayerSV( PlayerId id );
	GAME_API void emptySlotSV( SlotId id , SlotState state );
	GAME_API void sendPlayerStatusSV();

	virtual void clearUserUI() = 0;
	virtual void setupGame( StageManager& manager , GameSubStage* subStage ) = 0;
	virtual void doSetupSetting( bool beServer ) = 0;
	virtual void doSendSetting( DataStreamBuffer& buffer ) = 0;
	virtual void doRecvSetting( DataStreamBuffer& buffer ) = 0;

	virtual bool onWidgetEvent( int event ,int id , GWidget* widget ){ return true; }

	GAME_API void   addGUIControl( GWidget* ui );

	PlayerListPanel*  getPlayerListPanel(){ return mPlayerListPanel; }
	GameSettingPanel* getSettingPanel(){ return mSettingPanel; }
	GAME_API void              setMaxPlayerNum( int num );
protected:

	ServerWorker*     mServer;
	PlayerListPanel*  mPlayerListPanel;
	GameSettingPanel* mSettingPanel;
};
#endif // GameSettingHelper_h__
