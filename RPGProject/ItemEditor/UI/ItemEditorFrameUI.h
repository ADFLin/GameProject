///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version Mar 22 2011)
// http://www.wxformbuilder.org/
//
// PLEASE DO "NOT" EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#ifndef __ItemEditorFrameUI__
#define __ItemEditorFrameUI__

#include <wx/listctrl.h>
#include <wx/string.h>
#include <wx/gdicmn.h>
#include <wx/font.h>
#include <wx/colour.h>
#include <wx/settings.h>
#include <wx/choice.h>
#include <wx/button.h>
#include <wx/sizer.h>
#include <wx/statline.h>
#include <wx/panel.h>
#include <wx/stattext.h>
#include <wx/textctrl.h>
#include <wx/spinctrl.h>
#include <wx/bmpcbox.h>
#include <wx/statbox.h>
#include <wx/checkbox.h>
#include <wx/splitter.h>
#include <wx/frame.h>
#include <wx/dialog.h>

///////////////////////////////////////////////////////////////////////////

#define ID_ItemListCtrl 1000
#define ID_ItemTypeChoice 1001
#define ID_BoneChoice 1002
#define ID_NewItemButton 1003
#define ID_ModifyButton 1004
#define ID_ItemNameTextCtrl 1005
#define ID_CDTimeTCtrl 1006
#define ID_BuyCastSpin 1007
#define ID_SellCastSpin 1008
#define ID_MaxSoltNumSpin 1009
#define ID_AddIconButton 1010
#define ID_DelIconButton 1011
#define ID_ModifyIconButton 1012
#define ID_HelpStrTCtrl 1013
#define ID_PMListCtrl 1014
#define ID_PMAddButton 1015
#define ID_PMRemoveButton 1016
#define ID_PMModifyButton 1017
#define ID_EquimentPanel 1018
#define ID_EquipValSpin 1019
#define ID_EquipSoltChoice 1020
#define ID_ModelNameTCtrl 1021
#define ID_ScaleTCtrl 1022
#define ID_EditModelCBox 1023
#define ID_WeaponTypeChoice 1024
#define ID_DrawPanel 1025
#define ID_PropTyeChoice 1026
#define ID_ValOpChoice 1027
#define ID_ValueTextCtrl 1028
#define ID_TimeTextCtrl 1029
#define ID_NoRepeatCheckBox 1030
#define ID_TAddCheckBox 1031
#define ID_TSubCheckBox 1032
#define ID_IDSpinCtrl 1033
#define ID_ChangeButton 1034
#define ID_SKChoice 1035

///////////////////////////////////////////////////////////////////////////////
/// Class ItemEditorFrameUI
///////////////////////////////////////////////////////////////////////////////
class ItemEditorFrameUI : public wxFrame 
{
	private:
	
	protected:
		wxSplitterWindow* m_splitter2;
		wxPanel* m_panel6;
		wxSplitterWindow* m_splitter1;
		wxPanel* m_panel4;
		wxListCtrl* m_ItemListCtrl;
		wxChoice* m_ItemTypeChoice;
		wxChoice* m_BoneChoice;
		wxButton* m_NewItemButton;
		wxButton* m_ModifyButton;
		wxStaticLine* m_staticline2;
		wxPanel* m_EditPanel;
		wxTextCtrl* m_ItemNameTextCtrl;
		wxStaticText* m_staticText12;
		wxTextCtrl* m_CDTimeTCtrl;
		wxSpinCtrl* m_BuyCastSpin;
		wxSpinCtrl* m_SellCastSpin;
		wxSpinCtrl* m_MaxSoltNumSpin;
		wxBitmapComboBox* m_IconCBox;
		wxButton* m_AddIconButton;
		wxButton* m_DelIconButton;
		wxButton* m_ModifyIconButton;
		wxTextCtrl* m_HelpStrTCtrl;
		wxListCtrl* m_PMListCtrl;
		wxButton* m_PMAddButton;
		wxButton* m_PMRemoveButton;
		wxButton* m_PMModifyButton;
		wxStaticLine* m_staticline1;
		wxPanel* m_EquimentPanel;
		wxSpinCtrl* m_EquipValSpin;
		wxChoice* m_EquipSoltChoice;
		wxTextCtrl* m_ModelNameTCtrl;
		wxButton* m_button17;
		wxButton* m_button18;
		wxStaticText* m_staticText16;
		wxTextCtrl* m_ScaleTCtrl;
		wxCheckBox* m_EditModelCBox;
		wxPanel* m_WeaponPanel;
		wxStaticText* m_staticText13;
		wxChoice* m_WeaponTypeChoice;
		wxPanel* m_DrawPanel;
		
		// Virtual event handlers, overide them in your derived class
		virtual void OnItemListColClick( wxListEvent& event ) { event.Skip(); }
		virtual void OnItemListDeselected( wxListEvent& event ) { event.Skip(); }
		virtual void OnItemListRightClick( wxListEvent& event ) { event.Skip(); }
		virtual void OnItemListSelected( wxListEvent& event ) { event.Skip(); }
		virtual void OnItemTypeChoice( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnNewItemButton( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnModifyItem( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnChangeSetting( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnChangeSetting( wxSpinEvent& event ) { event.Skip(); }
		virtual void OnIconClick( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnPMListDeselected( wxListEvent& event ) { event.Skip(); }
		virtual void OnPMListSelected( wxListEvent& event ) { event.Skip(); }
		virtual void OnPMAddClick( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnPMRemoveClick( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnPMModifyClick( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnChioceModelClick( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnModelNameRemove( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnEditModelCheck( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnDraw( wxEraseEvent& event ) { event.Skip(); }
		virtual void OnKeyDown( wxKeyEvent& event ) { event.Skip(); }
		virtual void OnDrawPanelMouse( wxMouseEvent& event ) { event.Skip(); }
		virtual void OnDrawPanelResize( wxSizeEvent& event ) { event.Skip(); }
		
	
	public:
		
		ItemEditorFrameUI( wxWindow* parent, wxWindowID id = wxID_ANY, const wxString& title = wxT("Item Editor"), const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxSize( 950,666 ), long style = wxDEFAULT_FRAME_STYLE|wxTAB_TRAVERSAL );
		
		~ItemEditorFrameUI();
		
		void m_splitter2OnIdle( wxIdleEvent& )
		{
			m_splitter2->SetSashPosition( 0 );
			m_splitter2->Disconnect( wxEVT_IDLE, wxIdleEventHandler( ItemEditorFrameUI::m_splitter2OnIdle ), NULL, this );
		}
		
		void m_splitter1OnIdle( wxIdleEvent& )
		{
			m_splitter1->SetSashPosition( 0 );
			m_splitter1->Disconnect( wxEVT_IDLE, wxIdleEventHandler( ItemEditorFrameUI::m_splitter1OnIdle ), NULL, this );
		}
	
};

///////////////////////////////////////////////////////////////////////////////
/// Class PropModifyDlgUI
///////////////////////////////////////////////////////////////////////////////
class PropModifyDlgUI : public wxDialog 
{
	private:
	
	protected:
		wxStaticText* m_staticText5;
		wxChoice* m_PropTyeChoice;
		wxStaticText* m_staticText7;
		wxChoice* m_ValOpChoice;
		wxTextCtrl* m_ValueTextCtrl;
		wxTextCtrl* m_TimeTextCtrl;
		wxStaticLine* m_staticline5;
		wxCheckBox* m_NoRepeatCheckBox;
		wxCheckBox* m_TAddCheckBox;
		wxCheckBox* m_TSubCheckBox;
		
		
		wxButton* m_button8;
		wxButton* m_button7;
	
	public:
		
		PropModifyDlgUI( wxWindow* parent, wxWindowID id = wxID_ANY, const wxString& title = wxT("Prop Modify"), const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxSize( 297,264 ), long style = wxDEFAULT_DIALOG_STYLE|wxTRANSPARENT_WINDOW ); 
		~PropModifyDlgUI();
	
};

///////////////////////////////////////////////////////////////////////////////
/// Class ItemIDChangeDlgUI
///////////////////////////////////////////////////////////////////////////////
class ItemIDChangeDlgUI : public wxDialog 
{
	private:
	
	protected:
		wxSpinCtrl* m_IDSpinCtrl;
		wxButton* m_button8;
		wxButton* m_button7;
	
	public:
		
		ItemIDChangeDlgUI( wxWindow* parent, wxWindowID id = wxID_ANY, const wxString& title = wxT("Change ItemID"), const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxSize( 187,108 ), long style = wxDEFAULT_DIALOG_STYLE ); 
		~ItemIDChangeDlgUI();
	
};

///////////////////////////////////////////////////////////////////////////////
/// Class SkIconEditorDlgUI
///////////////////////////////////////////////////////////////////////////////
class SkIconEditorDlgUI : public wxDialog 
{
	private:
	
	protected:
		wxBitmapComboBox* m_IconCBox;
		wxButton* m_AddIconButton;
		wxButton* m_ModifyIconButton;
		wxStaticLine* m_staticline4;
		wxButton* m_ChangeButton;
		wxButton* m_OKButton;
		wxChoice* m_SKChoice;
		
		// Virtual event handlers, overide them in your derived class
		virtual void OnIconSelect( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnIconClick( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnChangeClick( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnSKChoice( wxCommandEvent& event ) { event.Skip(); }
		
	
	public:
		
		SkIconEditorDlgUI( wxWindow* parent, wxWindowID id = wxID_ANY, const wxString& title = wxT("Skill Icon Editor"), const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxSize( 292,177 ), long style = wxDEFAULT_DIALOG_STYLE ); 
		~SkIconEditorDlgUI();
	
};

#endif //__ItemEditorFrameUI__
