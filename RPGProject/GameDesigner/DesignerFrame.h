#ifndef DesignerFrame_h__
#define DesignerFrame_h__

#include <wx/wx.h>
#include <wx/aui/aui.h>

#include "LevelEditor.h"

enum
{
	ID_RENDER_PANEL_1 = 1000 ,
	ID_RENDER_PANEL_2 ,
	ID_RENDER_PANEL_3 ,
	ID_RENDER_PANEL_4 ,
};

class RenderPanel;

class IControlMode
{
public:
	virtual void procMouseEvent( wxMouseEvent& event ) = 0;
	virtual void procKeyEvent( wxKeyEvent& event) = 0;


	void defaultProcMouseEvent( wxMouseEvent& event )
	{
		if ( event.RightDown() )
		{
			pressPoint = event.GetPosition();
		}
		else if ( event.Dragging() && event.RightDown() )
		{
			wxPoint offset = event.GetPosition() - pressPoint;
			mCamControl.rotateByMouse( offset.x , offset.y );
			pressPoint = event.GetPosition();
		}
	}

	FristViewCamControl mCamControl;
	wxPoint pressPoint;
};

class SelectMode : public IControlMode
{



};

class TranslateMode : public IControlMode
{


	void procKeyEvent( wxKeyEvent& event )
	{

	}
};


class DesignerFrame : public wxFrame
{
public:
	DesignerFrame( wxWindow* parent , wxWindowID id , wxString const& title  );

	bool  init();
	void  createNewLevel( char const* name );
	HWND  getRenderWnd();

	void  OnMenuCommand( wxCommandEvent& event );
	void  OnRenderPanelMouse( wxMouseEvent& event );
	void  OnRenderPanelKey( wxKeyEvent& event );

	void  OnIdle( wxIdleEvent& event );
	void  OnClose( wxCloseEvent& event );
	void  OnEraseBackground( wxEraseEvent& event );
	void  OnResize( wxSizeEvent& event );

	RenderPanel* createRenderPanel( int idx );
	RenderPanel* mRenderPanel[ 4 ];
	bool         mHaveSaveLevel;
	DECLARE_EVENT_TABLE()
};

#endif // DesignerFrame_h__
