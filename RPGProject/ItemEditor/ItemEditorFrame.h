#ifndef ActorEditorFrame_h__
#define ActorEditorFrame_h__

#include "ui/ItemEditorFrameUI.h"

#include "CCamera.h"
#include "CEntity.h"
#include "TItemSystem.h"
#include <wx/msw/winundef.h>

struct ItemDataInfo;
struct PropModifyInfo;

class TItemManager;
class TEquipment;
class TObjControl;
class PhysicsWorld;
class CHero;

class ItemEditorFrame : public ItemEditorFrameUI
{
public:
	ItemEditorFrame( wxWindow* parent , wxWindowID id );

	~ItemEditorFrame();

	bool init();

protected:

	bool initCFly();

	void OnIdle( wxIdleEvent& event );
	void OnDraw( wxEraseEvent& event );
	void OnDrawPanelMouse( wxMouseEvent& event );
	void OnCommand( wxCommandEvent& event );
	void OnDrawPanelResize( wxSizeEvent& event );
	void OnNewItemButton( wxCommandEvent& event );

	void OnItemListSelected( wxListEvent& event );
	void OnItemListDeselected( wxListEvent& event );
	void OnItemListRightClick( wxListEvent& event );
	void OnItemListColClick( wxListEvent& event );
	void OnChangeSetting( wxSpinEvent& event );
	void OnChangeSetting( wxCommandEvent& event );

	void OnModelNameRemove( wxCommandEvent& event );

	void OnItemTypeChoice( wxCommandEvent& event );
	void OnPMListSelected( wxListEvent& event );
	void OnPMListDeselected( wxListEvent& event );
	void OnPMAddClick( wxCommandEvent& event );
	void OnPMModifyClick( wxCommandEvent& event );
	void OnPMRemoveClick( wxCommandEvent& event );
	void OnKeyDown( wxKeyEvent& event );
	void OnIconClick( wxCommandEvent& event );
	void OnChioceModelClick( wxCommandEvent& event );
	void OnEditModelCheck( wxCommandEvent& event );


	void transOBJ2WC3();
	void OnModifyItem( wxCommandEvent& event );
	void updateItemList( unsigned index );
	void updateData();
	void updateGUI();
	void updatePMListCtrl();


protected:
	CEntity*    mHeroEntity;
	CameraView  mCamera;

	FristViewCamControl mCamCtrl;

	GameFramework* mGameFramework;
	IGameMod*      mGameMod;

	Viewport*     mCFViewport;
	PhysicsWorld* mPhyWorld;
	CFScene*      mCFScene;
	CFWorld*      mCFWorld;
	CFCamera*     mCFCamera;

	CHero* mHeroLogicComp;

	int rotateAxis;
	int itemType;

	CFObject*     mModelObj;
	ItemDataInfo  mItemInfo;
	//TObjControl*  m_objCtrl;

	std::vector< String >& m_iconNameVec;

	int  selectItemIndex;
	int  selectPMIndex;
	long m_updateTime;

	DECLARE_EVENT_TABLE()


	void changeModel( char const * name );
	void changeEquipModel( TEquipment* equip , CFObject* model );
	void refreshItemListCtrl();

	template< class IOEval > 
	void processGUIStream( IOEval& eval );
};

class PropModifyDlg : public PropModifyDlgUI
{
public:
	PropModifyDlg( PropModifyInfo* info , wxWindow* parent , wxWindowID id = wxID_ANY );

	bool TransferDataFromWindow();
	bool TransferDataToWindow();

	template< class IOEval > 
	void processGUIStream( IOEval& eval );

	void updateData();
	void updateGUI();

	int  valOp;
	PropModifyInfo* m_info;
};

class ItemIDChangeDlg : public ItemIDChangeDlgUI
{
public:
	ItemIDChangeDlg( unsigned cID ,wxWindow* parent , wxWindowID id = wxID_ANY  )
		:ItemIDChangeDlgUI( parent , id )
	{
		changeID = cID;
	}

	bool TransferDataToWindow()
	{
		m_IDSpinCtrl->SetValue( changeID );
		return true;
	}
	bool TransferDataFromWindow()
	{
		changeID = m_IDSpinCtrl->GetValue();
		return true;
	}
	unsigned changeID;
};


class SkIconEditorDlg : public SkIconEditorDlgUI
{
public:
	SkIconEditorDlg( wxWindow* parent, wxWindowID id = wxID_ANY );

	void OnIconSelect( wxCommandEvent& event );
	void OnIconClick( wxCommandEvent& event );
	void OnOKClick( wxCommandEvent& event );
	void OnSKChoice( wxCommandEvent& event );
};

#endif // ActorEditorFrame_h__