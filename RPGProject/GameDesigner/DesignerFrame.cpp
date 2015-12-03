#include "DesignerFrame.h"

#include "RenderPanel.h"

#include "GameFramework.h"
#include "RenderSystem.h"

#include "CFWorld.h"
#include "CFViewport.h"

#include "CSceneCGF.h"


enum
{
	COM_MENU_START = 2000 ,
	COM_IMPORT_SCENE ,
	COM_MENU_END     ,
};


BEGIN_EVENT_TABLE( DesignerFrame , wxFrame )
	EVT_IDLE ( DesignerFrame::OnIdle )
	//EVT_ERASE_BACKGROUND( DesignerFrame::OnEraseBackground )
	EVT_SIZE( DesignerFrame::OnResize )
	//EVT_CLOSE( DesignerFrame::OnClose )
	EVT_MENU_RANGE( COM_MENU_START , COM_MENU_END , DesignerFrame::OnMenuCommand )
END_EVENT_TABLE()

DesignerFrame::DesignerFrame( wxWindow* parent , wxWindowID id , wxString const& title ) 
	:wxFrame( parent , id , title )
{
	mHaveSaveLevel = false;
	for (int i = 0 ; i < 4 ; ++i )
		mRenderPanel[i] = NULL;

	wxMenu* editMenu = new wxMenu;
	editMenu->Append( COM_IMPORT_SCENE , wxT("Import &Scene") );
	wxMenuBar* menuBar = new wxMenuBar;
	menuBar->Append(editMenu, _T("&Edit"));

	SetMenuBar( menuBar );
}


bool DesignerFrame::init()
{
	RenderPanel* panel = createRenderPanel( 0 );

	wxBoxSizer* bosSizer = new wxBoxSizer( wxVERTICAL );
	wxSizerFlags flags;
	flags.Expand().Border( wxALL , 0 ).Proportion( 1 );
	bosSizer->Add( panel , flags );

	SetSizer( bosSizer );
	return true;
}


class CSceneCGFSetup
{
public:
	void exec( ICGFBase* cgf )
	{
		CSceneCGF* sceneCGF = static_cast< CSceneCGF* >( cgf );
		sceneCGF->fileName = fileName;
		sceneCGF->sceneDir = sceneDir;
	}
	String fileName;
	String sceneDir;
};

void DesignerFrame::OnMenuCommand( wxCommandEvent& event )
{
	wxString SceneDir("Data/UI/Items/");

	switch( event.GetId() )
	{
	case COM_IMPORT_SCENE:
		{
			wxString SceneDir("Data/Map/");
			wxFileDialog dialg( this , wxT("Select Scene") , SceneDir , wxEmptyString ,
				"Scene files (*.cwn)|*.cwn" );

			if ( dialg.ShowModal() == wxID_OK )
			{
				CSceneCGFSetup setup;
				setup.sceneDir = dialg.GetDirectory();
				setup.fileName = dialg.GetFilename();

				gEnv->framework->getLevelEditor()->createCGF( CGF_SCENE , CGFCallBack( &setup , &CSceneCGFSetup::exec ) );
			}
		}
		break;

	}
}

RenderPanel* DesignerFrame::createRenderPanel( int idx )
{
	if ( mRenderPanel[idx ] )
		return mRenderPanel[idx];

	int id = ID_RENDER_PANEL_1 + idx;

	RenderPanel* panel = new RenderPanel( this , id );
	mRenderPanel[idx] = panel;

	panel->Connect( wxEVT_LEFT_DOWN, wxMouseEventHandler( DesignerFrame::OnRenderPanelMouse ), NULL, this );
	panel->Connect( wxEVT_LEFT_UP, wxMouseEventHandler( DesignerFrame::OnRenderPanelMouse ), NULL, this );
	panel->Connect( wxEVT_MIDDLE_DOWN, wxMouseEventHandler( DesignerFrame::OnRenderPanelMouse ), NULL, this );
	panel->Connect( wxEVT_MIDDLE_UP, wxMouseEventHandler( DesignerFrame::OnRenderPanelMouse ), NULL, this );
	panel->Connect( wxEVT_RIGHT_DOWN, wxMouseEventHandler( DesignerFrame::OnRenderPanelMouse ), NULL, this );
	panel->Connect( wxEVT_RIGHT_UP, wxMouseEventHandler( DesignerFrame::OnRenderPanelMouse ), NULL, this );
	panel->Connect( wxEVT_MOTION, wxMouseEventHandler( DesignerFrame::OnRenderPanelMouse ), NULL, this );
	panel->Connect( wxEVT_LEFT_DCLICK, wxMouseEventHandler( DesignerFrame::OnRenderPanelMouse ), NULL, this );
	panel->Connect( wxEVT_MIDDLE_DCLICK, wxMouseEventHandler( DesignerFrame::OnRenderPanelMouse ), NULL, this );
	panel->Connect( wxEVT_RIGHT_DCLICK, wxMouseEventHandler( DesignerFrame::OnRenderPanelMouse ), NULL, this );
	panel->Connect( wxEVT_LEAVE_WINDOW, wxMouseEventHandler( DesignerFrame::OnRenderPanelMouse ), NULL, this );
	panel->Connect( wxEVT_ENTER_WINDOW, wxMouseEventHandler( DesignerFrame::OnRenderPanelMouse ), NULL, this );
	panel->Connect( wxEVT_MOUSEWHEEL, wxMouseEventHandler( DesignerFrame::OnRenderPanelMouse ), NULL, this );

	panel->Connect( wxEVT_KEY_UP , wxKeyEventHandler( DesignerFrame::OnRenderPanelKey ) , NULL , this );
	panel->Connect( wxEVT_KEY_DOWN , wxKeyEventHandler( DesignerFrame::OnRenderPanelKey ) , NULL , this );

	return panel;
}

void DesignerFrame::OnIdle( wxIdleEvent& event )
{
	Refresh();
}

HWND DesignerFrame::getRenderWnd()
{
	return (HWND)GetHWND();
}

void DesignerFrame::OnRenderPanelMouse( wxMouseEvent& event )
{

	event.Skip();
}

void DesignerFrame::OnRenderPanelKey( wxKeyEvent& event )
{

	event.Skip();

}

void DesignerFrame::OnClose( wxCloseEvent& event )
{
	
	event.Skip();
}

void DesignerFrame::OnEraseBackground( wxEraseEvent& event )
{

	event.Skip();
}

void DesignerFrame::OnResize( wxSizeEvent& event )
{

	event.Skip();
}
