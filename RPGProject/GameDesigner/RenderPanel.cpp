#include "RenderPanel.h"


#include "GameFramework.h"
#include "RenderSystem.h"

#include "CFWorld.h"
#include "CFViewport.h"
#include "CFRenderSystem.h"


BEGIN_EVENT_TABLE( RenderPanel , wxPanel )
	EVT_ERASE_BACKGROUND( RenderPanel::OnEraseBackground )
	EVT_PAINT( RenderPanel::OnPaint )
	EVT_SIZE( RenderPanel::OnResize )
//EVT_IDLE ( DesignerFrame::OnIdle )
END_EVENT_TABLE()


RenderPanel::RenderPanel( wxWindow* parent , wxWindowID id ) 
	:wxPanel( parent , id )
{
	CFWorld* world = gEnv->renderSystem->getCFWorld();


	mRenderParams.destHWnd = (HWND) GetHWND();
	renderWindow = world->createRenderWindow( mRenderParams.destHWnd );
	int w = renderWindow->getWidth();
	int h = renderWindow->getHeight();
	
	mRenderParams.viewport = world->createViewport( 0 , 0 , w , h );
	mRenderParams.viewport->setBackgroundColor( 0.4 , 0.4 , 0.4 );
	mRenderParams.viewport->setRenderTarget( renderWindow , 0 );
	SetSize( wxSize(100,100) );
}

RenderPanel::~RenderPanel()
{
	mRenderParams.viewport->release();
}

void RenderPanel::OnEraseBackground( wxEraseEvent& event )
{


}

void RenderPanel::OnResize( wxSizeEvent& event )
{
	wxSize size = event.GetSize();
	mRenderParams.viewport->setSize( size.x , size.y );
	event.Skip();
}

void RenderPanel::OnPaint( wxPaintEvent& event )
{
	wxPaintDC dc( this );
	LevelEditor* editor = gEnv->framework->getLevelEditor();
	editor->render3D( mRenderParams );
	renderWindow->swapBuffers();
}
