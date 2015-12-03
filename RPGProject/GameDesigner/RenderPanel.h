#ifndef RenderPanel_h__
#define RenderPanel_h__

#include <wx/wx.h>

#include "common.h"
#include "LevelEditor.h"

class RenderPanel : public wxPanel
{
public:
	RenderPanel( wxWindow* parent , wxWindowID id );
	~RenderPanel();
	SRenderParams& getRenderParams(){ return mRenderParams;  }
protected:
	void OnResize( wxSizeEvent& event );
	void OnPaint( wxPaintEvent& event );
	void OnEraseBackground( wxEraseEvent& event );
	SRenderParams       mRenderParams;
	CFly::RenderWindow* renderWindow;
	DECLARE_EVENT_TABLE()
	

};

#endif // RenderPanel_h__
