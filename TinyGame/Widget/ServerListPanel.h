#ifndef ServerListPanel_h__
#define ServerListPanel_h__

#include "GameWidget.h"

#include "GameWorker.h"

class ClientWorker;

class  ServerListPanel : public GPanel
{
	typedef GPanel BaseClass;
public:

	enum
	{
		UI_ADD_SERVER = BaseClass::NEXT_UI_ID ,
		UI_CONNECT_SERVER      ,
		UI_SERVER_LISTCTRL     ,
		UI_REFRESH_SERVER_LIST ,
		NEXT_UI_ID ,
	};
	ServerListPanel( ClientWorker* worker , Vec2i const& pos , GWidget* parent );
	~ServerListPanel();

	virtual bool onChildEvent( int event , int id , GWidget* ui );

	bool  connectServer( char const* hostName );
	void  procServerInfo( IComPacket* cp );

	void  refreshServerList();
	void  onServerEvent( ClientListener::EventID event , unsigned msg );
	struct ServerInfo
	{
		std::string name;
		std::string ip;
	};
	typedef std::list< ServerInfo > ServerInfoList;
	ServerInfoList mServerList;

	DEFINE_MUTEX( mMutexUI )
	int           mConCount;
	ClientWorker* mWorker;
	GListCtrl*    mServerListCtrl;
	GTextCtrl*    mIPTextCtrl;
	GButton*      mConButton;
};


#endif // ServerListPanel_h__
