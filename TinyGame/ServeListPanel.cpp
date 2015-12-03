#include "TinyGamePCH.h"
#include "ServerListPanel.h"

#include "GameClient.h"
#include "PropertyKey.h"
#include "GameNetPacket.h"
#include "GameWidgetID.h"


ServerListPanel::ServerListPanel( ClientWorker* worker, Vec2i const& pos , GWidget* parent ) 
	:GPanel( UI_PANEL , pos , Vec2i(230,300) , parent )
	,mWorker( worker )
{
	GButton* button;

	mServerListCtrl = new GListCtrl( UI_SERVER_LISTCTRL , Vec2i( 10 , 10 ) , Vec2i( 210 , 200 ) , this );

	button= new GButton( UI_REFRESH_SERVER_LIST ,  Vec2i( 10  , 215 ) , Vec2i( 100 , 20 ) , this );
	button->setTitle( LAN("Refresh Server") );

	mConButton = new GButton( UI_CONNECT_SERVER ,  Vec2i( 10 + 110 , 215 ) , Vec2i( 100 , 20 ) , this );
	mConButton->setTitle( LAN("Connect") );
	mConButton->enable( false );

	mIPTextCtrl = new GTextCtrl( UI_ANY , Vec2i( 10 , 240 ) , 120 , this );
	mIPTextCtrl->setValue( Global::getSetting().getStringValue( "LastConServer" , "Net" , "0.0.0.0" ) );

	button= new GButton( UI_ADD_SERVER ,  Vec2i( 135 , 240 - 1 ) , Vec2i( 80 , 20 ) , this );
	button->setTitle( LAN("Add") );

	button= new GButton( UI_MAIN_MENU ,  Vec2i ( 10 , 265 ) , Vec2i( 210 , 20 ) , this );
	button->setTitle( LAN("Exit") );

	mWorker->getEvaluator().setUserFun< SPServerInfo >( this , &ServerListPanel::procServerInfo );
}

ServerListPanel::~ServerListPanel()
{
	mWorker->getEvaluator().removeProcesserFun( this );
}

bool ServerListPanel::onChildEvent( int event , int id , GWidget* ui )
{
	switch ( id )
	{
	case UI_CONNECT_SERVER:
		{
			ServerInfo* info = ( ServerInfo* ) mServerListCtrl->getSelectedItemData();
			if ( info )
			{
				mConCount = 0;
				connectServer( info->ip.c_str() );
			}
		}
		return false;
	case UI_REFRESH_SERVER_LIST:
		refreshServerList();
		return false;
	case UI_SERVER_LISTCTRL:
		switch( event )
		{
		case EVT_LISTCTRL_DCLICK:
			{
				ServerInfo* info = ( ServerInfo* ) mServerListCtrl->getSelectedItemData();
				if ( info )
				{
					mConButton->enable();
					mConCount = 0;
					connectServer( info->ip.c_str() );
				}
			}
			break;
		case EVT_LISTCTRL_SELECT:
			mConButton->enable();
			break;
		}
		return false;
	case UI_ADD_SERVER:
		{
			CSPComMsg com;
			com.str = "server_info";

			NetAddress addr;
			addr.setInternet( mIPTextCtrl->getValue() , TG_UDP_PORT );
			mWorker->addUdpCom( &com , addr );
		}
		return false;
	}
	return true;
}



bool ServerListPanel::connectServer( char const* hostName )
{
	try
	{
		mWorker->connect( hostName );
	}
	catch ( ... )
	{
		return false;
	}
	mConButton->enable( false );
	mConButton->setTitle( LAN("Connecting ...") );
	++mConCount;
	return true;
}

void ServerListPanel::onServerEvent( ClientListener::EventID event , unsigned msg )
{
	switch( event )
	{
	case ClientListener::eCON_RESULT:
		if ( msg )
		{
			ServerInfo* info = ( ServerInfo* ) mServerListCtrl->getSelectedItemData();
			Global::getSetting().setKeyValue( "LastConServer" , "Net" ,info->ip.c_str() );
		}
		else
		{
			mConButton->enable( true );
			mConButton->setTitle( LAN("Connect") );
		}
		break;
	}
}

void ServerListPanel::refreshServerList()
{
	mServerList.clear();
	mServerListCtrl->removeAllItem();
	mWorker->sreachLanServer();
	mConButton->enable( false );
}

void ServerListPanel::procServerInfo( IComPacket* cp )
{
	SPServerInfo* com = cp->cast< SPServerInfo >();
	ServerInfo info;
	info.ip   = com->ip;
	info.name = com->name;
	mServerList.push_back( info );
	unsigned pos = mServerListCtrl->appendItem( com->name );
	mServerListCtrl->setItemData( pos , &mServerList.back() );
}

