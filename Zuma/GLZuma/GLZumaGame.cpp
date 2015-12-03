#include "ZumaPCH.h"
#include "GLZumaGame.h"

#include "ResID.h"
#include "ZEvent.h"
#include "ZFont.h"
#include "ZBallConGroup.h"


GLZumaGame::GLZumaGame()

{

}

bool GLZumaGame::onEvent( int evtID , ZEventData const& data , ZEventHandler* child )
{
	if ( !ZumaGame::onEvent( evtID , data , child ) )
		return false;

	return true;
}

bool GLZumaGame::onUIEvent( int evtID , int uiID , ZWidget* ui )
{
	if ( !ZumaGame::onUIEvent( evtID ,uiID ,ui ) )
		return false;

	switch ( uiID )
	{
	case UI_QUIT_GAME:
		setLoopOver( true );
		break;
	}

	return true;
}

IRenderSystem* GLZumaGame::createRenderSystem()
{
	GLRenderSystem* renderSystem = new GLRenderSystem;
	if ( !renderSystem->init( getHWnd() , getHDC() , getHRC() ) )
	{
		delete renderSystem;
		return NULL;
	}
	return renderSystem;
}

bool GLZumaGame::setupWindow( char const* title , int w , int h )
{
	return create( (LPTSTR)title , w , h , &SysMsgHandler::MsgProc );
}

