#include "common.h"

#include <wx/wx.h>

#include "DesignerFrame.h"

GlobalVal g_GlobalVal( 30 );
#include "Platform.h"
#include "ISystem.h"
#include "CGameMod.h"
#include "LevelEditor.h"
#include "GameFramework.h"

class GameDesignerApp : public wxApp
{
public:
	virtual bool OnInit();
	virtual int  OnExit();

	bool initGUISystem();
private:
	GameFramework* mGameFramework;
	IGameMod*      mGameMod;
};

IMPLEMENT_APP( GameDesignerApp )

bool GameDesignerApp::initGUISystem()
{
	wxImage::AddHandler( new wxPNGHandler );
	wxImage::AddHandler( new wxICOHandler );
	wxUpdateUIEvent::SetUpdateInterval( 500 );
	return true;
}

bool GameDesignerApp::OnInit()
{
	initGUISystem();

	wxSize displaySize = wxGetDisplaySize();
	DesignerFrame* mainFrame = new DesignerFrame( NULL , wxID_ANY , "Designer" );

	SSystemInitParams sysParams;
	sysParams.hInstance     = ::GetModuleHandle( NULL );
	sysParams.hWnd          = mainFrame->getRenderWnd();
	sysParams.bufferWidth   = displaySize.x;
	sysParams.bufferHeight  = displaySize.y;
	sysParams.platform  = new CWinPlatform( sysParams.hInstance , sysParams.hWnd );
	sysParams.isDesigner = true;

	mGameFramework = new GameFramework;
	if ( !mGameFramework->init( sysParams ) )
		return false;

	gEnv->OSPlatform = sysParams.platform;

	mGameMod = new CGameMod;
	if ( !mGameMod->init( mGameFramework ) )
		return false;
	gEnv->gameMod = mGameMod;

	if ( !mainFrame->init() )
		return false;

	LevelEditor* levelEditor = mGameFramework->getLevelEditor();
	levelEditor->createEmptyLevel( "unnamed" );

	mainFrame->Layout();
	mainFrame->Show( true );
	SetTopWindow( mainFrame );

	return true;
}

int GameDesignerApp::OnExit()
{
	delete mGameMod;
	delete mGameFramework;


	return 0;
}
