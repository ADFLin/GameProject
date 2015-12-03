#ifndef WorldEditFrame_h__
#define WorldEditFrame_h__

#include <wx/wx.h>
#include "common.h"
#include "WorldEditor.h"
#include "ui/WorldEditorFrameUI.h"
#include "TObjectEditor.h"

#include "TEditData.h"
#include "wxPropIO.h"


#include "THeighField.h"


BEGIN_DECLARE_EVENT_TYPES()
	DECLARE_EVENT_TYPE(wxEVT_FLY_EVENT, 10000)
END_DECLARE_EVENT_TYPES()

enum EditMode
{
	MODE_EDIT_TERRAIN   ,
	MODE_SELECT_OBJECT  ,
};

class  WorldEditorFrame;
class  TResManager;

struct EmLogicGroup;

class WorldEditorFrame : public WorldEditorFrameUI
{
public:
	WorldEditorFrame( wxWindow* parent, wxWindowID id = wxID_ANY );

	HWND getDrawHWND(){  return(HWND) m_DrawPanel->GetHWND();  }
	void init();


	void OnIdle(wxIdleEvent& event);
	void OnDraw(wxEraseEvent& event);
	void OnMouse( wxMouseEvent& event );
	void OnKeyDown( wxKeyEvent& event );
	void OnCommand( wxCommandEvent& event );


	void createEditData( int comID );


	void updatePropGrid();
	void OnPGChange( wxPropertyGridEvent & event );
	void OnUpdateUI( wxUpdateUIEvent& event )
	{
	}

	void OnUpdateButton( wxCommandEvent& event );


	void OnDrawSize( wxSizeEvent& event );

	//Edit Tree Event
	//void OnTreeSelChanged( wxTreeEvent& event );
protected:
	void reloadEditTree();
	void OnTreeActivated( wxTreeEvent& event );
	void OnTreeMenu( wxTreeEvent& event );
	void OnTreeBeginDrag( wxTreeEvent& event );
	void OnTreeEndDrag( wxTreeEvent& event );
	void OnTreeLabelEditBegin( wxTreeEvent& event );
	void OnTreeLabelEditEnd( wxTreeEvent& event );

protected:
	void OnEditTerrainMouse( wxMouseEvent& event );
	void OnSelectObjMouse( wxMouseEvent& event );

	void OnModelListSelected( wxListEvent& event );
	void OnModelListRightDown( wxMouseEvent& event );
	void OnRoleListRightDown( wxMouseEvent& event );
	void OnRoleListSelected( wxListEvent& event );

	void addNoteBook( wxWindow* window , wxChar const* str );

	void setCurEditData( EmDataBase* eData );
	void addTreeItem( EmDataBase* eData , wxTreeItemId parentID );

protected:

	void loadRole();
	void loadModel();
	void updateModelListCtrl();
	void updateRoleListCtrl();

	void createTreeIcon();

	EmDataBase* getEditData( wxTreeItemId id );

	typedef TPropLoader< wxPropIO > PropLoader;
	PropLoader propLoader;
	wxArrayTreeItemIds m_dragItemID;

	unsigned m_selectRoleID;

	TObjectEditor*  m_objEdit;
	TResManager*    manager;

	wxTreeItemId    m_curEditItemID;
	wxTreeItemId    m_curMenuItemID;

	EmDataBase*     m_curEditData;

	int      selectModelID;
	EditMode  m_mode;


	bool     m_isGameOver;
	long     m_FrameTime;
	long     m_updateTime;


	BrushObj*      brushObj;
	TerBrush       brush;
	THeighField*   heighField;

	Vec3D    hitPos;
	Vec3D    testPos;

	DECLARE_EVENT_TABLE()

};

class SignalConDlg : public SignalConDlgUI
{
public:
	
	SignalConDlg( EmLogicGroup* logicGroup ,wxWindow* parent, wxWindowID id = wxID_ANY  );

	void refreshLogicObj();
	void refreshSignal( EmDataBase* data );
	void refreshSlot( EmDataBase* data );
	void refreshConnect();

	void OnChoice( wxCommandEvent& event );
	void OnAddClick( wxCommandEvent& event );
	void OnRemoveClick( wxCommandEvent& event );
	void OnModifyClick( wxCommandEvent& event );
	void OnListItemSelected( wxListEvent& event );
	void OnListItemDeselected( wxListEvent& event );


	typedef EmLogicGroup::ConnectInfo ConnectInfo;
	ConnectInfo*  m_selectCon;
	EmLogicGroup* m_logicGroup;
};


class TerrainToolDlg : public TerrainToolDlgUI
{
public:
	TerrainToolDlg( THeighField& field , TerBrush& brush , BrushObj& obj ,
		            wxWindow* parent, wxWindowID id = wxID_ANY );

	void OnScroll( wxScrollEvent& event );
	void OnTextEnter( wxCommandEvent& event );
	void OnEffectRadioBox( wxCommandEvent& event );
	TerBrush&      brush;
	THeighField&   field;
	BrushObj&      brushObj;
};





#endif // WorldEditFrame_h__

