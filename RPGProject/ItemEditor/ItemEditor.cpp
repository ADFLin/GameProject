

#include "ItemEditorFrame.h"
#include "CFWorld.h"

#include <wx/msw/winundef.h>
#include <wx/wx.h>


GlobalVal g_GlobalVal( 30 );

char* g_WinTitle = "rcNav Test";
int g_ScreenWidth  = 800;
int g_ScreenHeight = 600;

class ItemEditorApp : public wxApp
{
public:
	virtual bool OnInit();
	virtual int  OnExit();
private:
};

IMPLEMENT_APP(ItemEditorApp)

bool ItemEditorApp::OnInit()
{
	wxImage::AddHandler(new wxPNGHandler);
	wxUpdateUIEvent::SetUpdateInterval( 500 );

	ItemEditorFrame* frame = new ItemEditorFrame( NULL , wxID_ANY );
	if ( !frame->init() )
		return false;

	frame->Layout();
	frame->Show( true );

	SetTopWindow( frame );
	return true;
}

int ItemEditorApp::OnExit()
{
	return 0;
}





