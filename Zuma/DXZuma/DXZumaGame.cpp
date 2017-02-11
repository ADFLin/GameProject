#include "ZumaPCH.h"
#include "DXZumaGame.h"
#include "ResID.h"

bool DXZumaGame::onEvent( int evtID , ZEventData const& data , ZEventHandler* sender )
{
	if ( !ZumaGame::onEvent( evtID , data , sender ) )
		return false;

	return true;
}

IRenderSystem* DXZumaGame::createRenderSystem()
{
	D3D9RenderSystem* renderSystem = new D3D9RenderSystem;
	if( !renderSystem->init( mFrame.getD3DDevice() ) )
	{
		delete renderSystem;
		return NULL;
	}
	return renderSystem;
}

bool DXZumaGame::setupWindow( char const* title , int w , int h )
{
	return mFrame.create( ( LPTSTR) title , w , h , &SysMsgHandler::MsgProc );
}

