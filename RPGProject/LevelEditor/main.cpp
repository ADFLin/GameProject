#include <wx/wx.h>

#include "WorldEditor.h"
#include "WorldEditorFrame.h"


int g_ScreenWidth  = 800;
int g_ScreenHeight = 600;

class SpriteDesignerApp : public wxApp
{
public:
	virtual bool OnInit();
	virtual int  OnExit();

private:
	WORLDid wID;
};



IMPLEMENT_APP(SpriteDesignerApp)

bool SpriteDesignerApp::OnInit()
{
	::FyUseHighZ( TRUE );
	WorldEditorFrame* frame = new WorldEditorFrame( NULL , wxID_ANY );

	HWND hwnd = frame->getDrawHWND();
	wID = FyCreateWorld( hwnd , 0 , 0 , g_ScreenWidth , g_ScreenHeight, 32 , FALSE );
	
	WorldEditor* game = new WorldEditor; 
	game->OnInit( wID );
	wxUpdateUIEvent::SetUpdateInterval( 500 );

	frame->init();
	frame->Maximize();
	frame->Show( true );
	frame->Layout();
	frame->SetFocus();
	SetTopWindow( frame );
	return true;
}

int SpriteDesignerApp::OnExit()
{
	delete g_Game;
	FyEndWorld( wID );
	return true;
}





