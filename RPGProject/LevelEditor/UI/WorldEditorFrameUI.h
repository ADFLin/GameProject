///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version Aug 25 2009)
// http://www.wxformbuilder.org/
//
// PLEASE DO "NOT" EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#ifndef __WorldEditorFrameUI__
#define __WorldEditorFrameUI__

#include <wx/treectrl.h>
#include <wx/gdicmn.h>
#include <wx/font.h>
#include <wx/colour.h>
#include <wx/settings.h>
#include <wx/string.h>
#include <wx/sizer.h>
#include <wx/panel.h>
#include <wx/splitter.h>
#include <wx/bitmap.h>
#include <wx/image.h>
#include <wx/icon.h>
#include <wx/listctrl.h>
#include <wx/notebook.h>
#include <wx/button.h>
#include <wx/propgrid/propgrid.h>
#include <wx/dialog.h>
#include <wx/propgrid/propdev.h>
#include <wx/propgrid/advprops.h>
#include <wx/frame.h>
#include <wx/choice.h>
#include <wx/statbox.h>
#include <wx/radiobox.h>
#include <wx/stattext.h>
#include <wx/slider.h>
#include <wx/textctrl.h>

///////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
/// Class WorldEditorFrameUI
///////////////////////////////////////////////////////////////////////////////
class WorldEditorFrameUI : public wxFrame 
{
	private:
	
	protected:
		enum
		{
			ID_Notebook = 1000,
			ID_ObjEditPanel,
			ID_GroupTreeCtrl,
			ID_ModelList,
			ID_RoleListCtrl,
			ID_DrawPanel,
			ID_UpdateButton,
			ID_ObjPropGrid,
		};
		
		wxBoxSizer* m_Sizer;
		wxSplitterWindow* m_splitter3;
		wxNotebook* m_Notebook;
		wxPanel* m_ObjEditPanel;
		wxSplitterWindow* m_splitter4;
		wxPanel* m_panel10;
		wxTreeCtrl* m_GroupTreeCtrl;
		wxPanel* m_ModelEditPanel;
		wxListCtrl* m_ModelList;
		wxPanel* m_RoleEditPanel;
		wxListCtrl* m_RoleListCtrl;
		wxPanel* m_panel102;
		wxSplitterWindow* m_splitter41;
		wxPanel* m_DrawPanel;
		
		wxButton* m_UpdateButton;
		wxPanel* m_panel9;
		wxPropertyGrid* m_ObjPropGrid;
		
		// Virtual event handlers, overide them in your derived class
		virtual void OnTreeBeginDrag( wxTreeEvent& event ) { event.Skip(); }
		virtual void OnTreeLabelEditBegin( wxTreeEvent& event ) { event.Skip(); }
		virtual void OnTreeEndDrag( wxTreeEvent& event ) { event.Skip(); }
		virtual void OnTreeLabelEditEnd( wxTreeEvent& event ) { event.Skip(); }
		virtual void OnTreeActivated( wxTreeEvent& event ) { event.Skip(); }
		virtual void OnTreeCollapsing( wxTreeEvent& event ) { event.Skip(); }
		virtual void OnTreeExpanding( wxTreeEvent& event ) { event.Skip(); }
		virtual void OnTreeMenu( wxTreeEvent& event ) { event.Skip(); }
		virtual void OnTreeRightClick( wxTreeEvent& event ) { event.Skip(); }
		virtual void OnTreeSelChanged( wxTreeEvent& event ) { event.Skip(); }
		virtual void OnModelListSelected( wxListEvent& event ) { event.Skip(); }
		virtual void OnModelListRightDown( wxMouseEvent& event ) { event.Skip(); }
		virtual void OnRoleListSelected( wxListEvent& event ) { event.Skip(); }
		virtual void OnRoleListRightDown( wxMouseEvent& event ) { event.Skip(); }
		virtual void OnDraw( wxEraseEvent& event ) { event.Skip(); }
		virtual void OnKeyDown( wxKeyEvent& event ) { event.Skip(); }
		virtual void OnMouse( wxMouseEvent& event ) { event.Skip(); }
		virtual void OnDrawSize( wxSizeEvent& event ) { event.Skip(); }
		virtual void OnUpdateButton( wxCommandEvent& event ) { event.Skip(); }
		
	
	public:
		
		WorldEditorFrameUI( wxWindow* parent, wxWindowID id = wxID_ANY, const wxString& title = wxT("World Editor"), const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxSize( 1062,662 ), long style = wxDEFAULT_FRAME_STYLE|wxTAB_TRAVERSAL );
		~WorldEditorFrameUI();
		void m_splitter3OnIdle( wxIdleEvent& )
		{
			m_splitter3->SetSashPosition( 200 );
			m_splitter3->Disconnect( wxEVT_IDLE, wxIdleEventHandler( WorldEditorFrameUI::m_splitter3OnIdle ), NULL, this );
		}
		
		void m_splitter4OnIdle( wxIdleEvent& )
		{
			m_splitter4->SetSashPosition( 0 );
			m_splitter4->Disconnect( wxEVT_IDLE, wxIdleEventHandler( WorldEditorFrameUI::m_splitter4OnIdle ), NULL, this );
		}
		
		void m_splitter41OnIdle( wxIdleEvent& )
		{
			m_splitter41->SetSashPosition( -200 );
			m_splitter41->Disconnect( wxEVT_IDLE, wxIdleEventHandler( WorldEditorFrameUI::m_splitter41OnIdle ), NULL, this );
		}
		
	
};

///////////////////////////////////////////////////////////////////////////////
/// Class SignalConDlgUI
///////////////////////////////////////////////////////////////////////////////
class SignalConDlgUI : public wxDialog 
{
	private:
	
	protected:
		enum
		{
			ID_ConListCtrl = 1000,
			ID_SenderChoice,
			ID_SignalChoice,
			ID_HolderChoice,
			ID_SlotChoice,
			ID_AddButton,
			ID_MofidyButton,
			ID_RemoveButton,
		};
		
		wxListCtrl* m_ConListCtrl;
		wxChoice* m_SenderChoice;
		wxChoice* m_SignalChoice;
		wxChoice* m_HolderChoice;
		wxChoice* m_SlotChoice;
		wxButton* m_AddButton;
		wxButton* m_MofidyButton;
		wxButton* m_RemoveButton;
		
		// Virtual event handlers, overide them in your derived class
		virtual void OnListItemDeselected( wxListEvent& event ) { event.Skip(); }
		virtual void OnListItemSelected( wxListEvent& event ) { event.Skip(); }
		virtual void OnChoice( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnAddClick( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnModifyClick( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnRemoveClick( wxCommandEvent& event ) { event.Skip(); }
		
	
	public:
		
		SignalConDlgUI( wxWindow* parent, wxWindowID id = wxID_ANY, const wxString& title = wxEmptyString, const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxSize( 481,398 ), long style = wxCAPTION|wxCLOSE_BOX|wxDEFAULT_DIALOG_STYLE|wxRESIZE_BORDER );
		~SignalConDlgUI();
	
};

///////////////////////////////////////////////////////////////////////////////
/// Class TerrainToolDlgUI
///////////////////////////////////////////////////////////////////////////////
class TerrainToolDlgUI : public wxDialog 
{
	private:
	
	protected:
		enum
		{
			ID_EffectRadioBox = 1000,
			ID_StrSlider,
			ID_StrTCtrl,
			ID_RadiusSlider,
			ID_RadiusTCtrl,
			ID_FallOffSlider,
			ID_FallOffTCtrl,
		};
		
		wxRadioBox* m_EffectRadioBox;
		wxStaticText* m_staticText1;
		wxSlider* m_StrSlider;
		wxTextCtrl* m_StrTCtrl;
		wxStaticText* m_staticText12;
		wxSlider* m_RadiusSlider;
		wxTextCtrl* m_RadiusTCtrl;
		wxStaticText* m_staticText11;
		wxSlider* m_FallOffSlider;
		wxTextCtrl* m_FallOffTCtrl;
		
		// Virtual event handlers, overide them in your derived class
		virtual void OnEffectRadioBox( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnScroll( wxScrollEvent& event ) { event.Skip(); }
		virtual void OnTextEnter( wxCommandEvent& event ) { event.Skip(); }
		
	
	public:
		
		TerrainToolDlgUI( wxWindow* parent, wxWindowID id = wxID_ANY, const wxString& title = wxT("Terrain Tool"), const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxSize( 314,220 ), long style = wxDEFAULT_DIALOG_STYLE|wxRESIZE_BORDER );
		~TerrainToolDlgUI();
	
};

#endif //__WorldEditorFrameUI__
