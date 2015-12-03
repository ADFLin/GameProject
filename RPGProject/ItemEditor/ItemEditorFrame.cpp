#include "ItemEditorFrame.h"
#include "gui/wxGuiIO.h"

#include "common.h"
#include "TResManager.h"
#include "TSkill.h"

#include "CFCamera.h"
#include "CFScene.h"
#include "CFWorld.h"
#include "CFObject.h"
#include "CFActor.h"
#include "ObjFileLoader.h"

#include "PhysicsSystem.h"
#include "CHero.h"
#include "SpatialComponent.h"
#include "ActorMovement.h"
#include "TRoleTemplate.h"

#include "CSceneLevel.h"
#include "Platform.h"
#include "RenderSystem.h"
#include "IScriptTable.h"
#include "GameFramework.h"
#include "CGameMod.h"

int const g_IconSize = 32;
enum
{
	COM_START = 30000 ,
	COM_SAVE_ITEM_DATA ,
	COM_SAVE_SKILL_DATA ,
	COM_CHANGE_ITEM_ID ,
	COM_DELETE_ITEM ,
	COM_EDIT_SKILL_ICON ,
	COM_TRANS_OBJ_TO_CW3 ,
	COM_END ,
};

BEGIN_EVENT_TABLE( ItemEditorFrame , ItemEditorFrameUI )
	EVT_IDLE (ItemEditorFrame::OnIdle)
	EVT_COMMAND_RANGE(COM_START,COM_END, wxEVT_COMMAND_MENU_SELECTED ,ItemEditorFrame::OnCommand)
	//EVT_MOUSE_EVENTS(ActorEditorFrame::OnMouse)
END_EVENT_TABLE()


wxString ItemIconDir("Data/UI/Items/");
wxString SkillIconDir = SKILL_ICON_DIR ;

wxChar const* armourSoltStr[] =
{
	"繷场 HEAD"     , //繷场
	"繴场 NECK"     , //繴场
	"场 CHEST"    , //场
	"竬场 WAIST"    , //竬场
	"籐场 LEG"      , //籐场
	"竲场 FOOT"     , //竲场
	"场 SHOULDER" , //场
	"も得 WRIST"    , //も得
	"も FINGER"   , //も
	"も HAND"        , //も
	"杆耿珇 TRINKET"  , //杆耿珇
};

wxChar const* weaponSoltStr[] =
{
	"蛮も TWO_HAND" ,
	"も MAIN_HAND" ,
	"捌も SUB_HAND" ,
};


wxChar const* PropTypeStr[] =
{
	"PROP_HP"  ,
	"PROP_MP"  ,
	"PROP_MAX_HP",
	"PROP_MAX_MP",
	"PROP_STR" ,
	"PROP_INT" ,
	"PROP_END" ,
	"PROP_DEX" ,
	"PROP_MAT" ,
	"PROP_SAT" ,
	"PROP_DT"  ,
	"PROP_VIEW_DIST",
	"PROP_AT_RANGE" ,
	"PROP_AT_SPEED" ,
	"PROP_MV_SPEED" ,
	"PROP_KEXP" ,
	"PROP_JUMP_HEIGHT"
};


static wxChar const* itemTypeStr[] =
{
	"ITEM_BASE" ,
	"ITEM_WEAPON" ,
	"ITEM_ARMOUR" ,
	"ITEM_FOOD"   ,
};

static wxChar const* itemTypeStrS[] =
{
	"B" ,"W" ,"A" ,"F" ,
};

static wxChar const* weaponTypeStr[] =
{
	"WT_SWORD" ,
	"WT_BOW"   ,
	"WT_AXE"   ,
	"WT_ANIFE" ,
};

enum
{
	ITEMLIST_ID    = 0,
	ITEMLIST_TYPE  ,
	ITEMLIST_NAME  ,
};


#define WXARRAYSTR( name ) wxArrayString( sizeof( name )/sizeof( wxChar*) , name )

ItemEditorFrame::ItemEditorFrame( wxWindow* parent , wxWindowID id ) 
	:ItemEditorFrameUI( parent)
	,m_iconNameVec( TItemManager::getInstance().getIconNameVec() )
	,mCamera()
	,mCamCtrl( &mCamera )
	,mModelObj( NULL )
	,rotateAxis(0)
{

	selectItemIndex = -1;
	selectPMIndex = -1;

	m_updateTime = 30;

	//m_objCtrl = new TObjControl;

	m_ItemListCtrl->InsertColumn( ITEMLIST_ID , wxT("id") );
	m_ItemListCtrl->InsertColumn( ITEMLIST_NAME , wxT("name") );
	m_ItemListCtrl->InsertColumn( ITEMLIST_TYPE , wxT("type") );
	
	m_ItemListCtrl->SetColumnWidth( ITEMLIST_ID , 30 );
	m_ItemListCtrl->SetColumnWidth( ITEMLIST_TYPE , 20 );
	m_ItemListCtrl->SetColumnWidth( ITEMLIST_NAME , 120 );

	m_PMListCtrl->InsertColumn( 0 , wxT("prop") );
	m_PMListCtrl->InsertColumn( 1 , wxT("value") );
	m_PMListCtrl->InsertColumn( 2 , wxT("time") );
	m_PMListCtrl->InsertColumn( 3 , wxT("flag") );

	m_PMListCtrl->SetColumnWidth( 0 , 100 );
	m_PMListCtrl->SetColumnWidth( 1 , 70 );
	m_PMListCtrl->SetColumnWidth( 2 , 60 );
	m_PMListCtrl->SetColumnWidth( 3 , 400 );

	m_ItemTypeChoice->Append( WXARRAYSTR( itemTypeStr ) );
	m_ItemTypeChoice->SetSelection( 0 );

	m_WeaponTypeChoice->Append( WXARRAYSTR( weaponTypeStr ) );
	m_WeaponTypeChoice->SetSelection( 0 );


	m_ModifyButton->Enable( false );

	wxMenu *menuFile = new wxMenu;
	menuFile->Append( COM_SAVE_ITEM_DATA , wxT("&Save Item Data") );
	menuFile->Append( COM_TRANS_OBJ_TO_CW3 , wxT("&Trans OBJ FIle") );
	menuFile->AppendSeparator();
	menuFile->Append( COM_EDIT_SKILL_ICON , wxT("Edit Skill Icon") );
	menuFile->Append( COM_SAVE_SKILL_DATA , wxT("&Save Skill Data") );
	

	wxMenuBar *menuBar = new wxMenuBar;
	menuBar->Append(menuFile, _T("&File"));

	SetMenuBar( menuBar );

}


ItemEditorFrame::~ItemEditorFrame()
{
	mCFViewport->release();
	mCFScene->release();
	mCFWorld->release();
}


bool ItemEditorFrame::init()
{
	if ( !initCFly() )
		return false;

	SSystemInitParams sysParams;
	sysParams.hInstance    = ::GetModuleHandle( NULL );
	sysParams.hWnd         = (HWND)this->GetHandle();
	sysParams.bufferWidth  = this->GetSize().GetX();
	sysParams.bufferHeight = this->GetSize().GetY();
	sysParams.platform  = new CWinPlatform( sysParams.hInstance , sysParams.hWnd );

	mGameFramework = new GameFramework;
	if ( !mGameFramework->init( sysParams ) )
		return false;

	gEnv->OSPlatform = sysParams.platform;

	mGameMod = new CGameMod;
	if ( !mGameMod->init( mGameFramework ) )
		return false;
	gEnv->gameMod = mGameMod;

	GameObjectInitHelper mHelper;
	mHelper.cfWorld    = mCFWorld;
	mHelper.sceneLevel = mGameFramework->getLevelManager()->createEmptyLevel( "BT_LOWER" , SLF_USE_MAP | SLF_USE_MAP_NAVMESH );;

	mCFScene = mHelper.sceneLevel->getRenderScene();

	TResManager::getInstance().init();
	TResManager::getInstance().setCurScene( mCFScene );

	if ( !PhysicsSystem::getInstance().initSystem() )
		return false;

	mPhyWorld = PhysicsSystem::getInstance().createWorld();
	mPhyWorld->setGravity( Vec3D(0,0,0) );

	


	{

		IEntityClass* actorClass = gEnv->entitySystem->findClass( "Actor:Hero" );

		Vec3D worldPos(0,530,500);

		IScriptTable* scriptTable = gEnv->scriptSystem->createScriptTable();
		scriptTable->pushValue( "roleID" , (unsigned)ROLE_HERO );

		EntitySpawnParams params;
		params.propertyTable;
		params.pos           = worldPos;
		params.entityClass   = actorClass; 
		params.propertyTable = scriptTable;
		params.helper        = &mHelper;

		CEntity* entity = gEnv->entitySystem->spawnEntity( params );

	}

	//m_objCtrl->init(  getGame()->getScreenViewport().Object() ,
	//	              getGame()->getFlyCamera().Object() );

	TItemManager::getInstance().loadItemData( ITEM_DATA_PATH );

	refreshItemListCtrl();

	wxString dir("Data/UI/Items/");
	for ( int i = 0 ; i < m_iconNameVec.size() ; ++i )
	{
		wxString path = ItemIconDir + m_iconNameVec[i].c_str() + wxT(".png");

		wxImage img( path , wxBITMAP_TYPE_PNG );
		
		if ( img.IsOk() )
		{
			m_IconCBox->Append( wxString() << i , wxBitmap( img.Scale( g_IconSize , g_IconSize ) ) , (void*)NULL  );
		}
		else
		{
			m_IconCBox->Append( wxString() << i );
		}
	}

	//FnActor fyActor = getGame()->getActor()->getFlyActor();

	//for( int i =0;i < fyActor.GetBoneNumber(); ++i )
	//{
	//	m_BoneChoice->Append( fyActor.GetBoneName(i) );
	//}

	return true;

};

bool ItemEditorFrame::initCFly()
{
	HWND hWnd = (HWND) m_DrawPanel->GetHWND();
	int w = 800;
	int h = 600;

	if ( !CFly::initSystem() )
		return false;

	mCFWorld = CFly::createWorld( hWnd ,800 , 600 , 32 , false );

	if ( !mCFWorld )
		return false;

	mCFViewport = mCFWorld->createViewport( 0, 0 , 800 ,600 );
	mCFViewport->setBackgroundColor( 0 , 0 , 0 );

	mCFCamera = mCFScene->createCamera();
	mCFCamera->setAspect( float(  w ) / h );

	mCFCamera->setNear( 10 );
	mCFCamera->setFar( 20000 );

	mCamera.setPosition( Vec3D(0,-100,50) );
	mCamera.setLookPos( Vec3D(0,0,50) );
}

void ItemEditorFrame::updateItemList( unsigned index )
{
	unsigned itemID = m_ItemListCtrl->GetItemData( index );
	TItemBase* item = TItemManager::getInstance().getItem( itemID );

	m_ItemListCtrl->SetItem( index , ITEMLIST_ID , wxString() << itemID );

	if ( item )
	{
		m_ItemListCtrl->SetItem( index , ITEMLIST_NAME , item->getName() );
		m_ItemListCtrl->SetItem( index , ITEMLIST_TYPE , itemTypeStrS[ item->getType() ] );
	}
	else
	{
		m_ItemListCtrl->SetItem( index , ITEMLIST_NAME , "" );
		m_ItemListCtrl->SetItem( index , ITEMLIST_TYPE , "" );
	}
}

void ItemEditorFrame::refreshItemListCtrl()
{
	m_ItemListCtrl->DeleteAllItems();
	for( int i = 0 ; i <= TItemManager::getInstance().getMaxItemID() ; ++i )
	{
		wxListItem item;
		item.m_col = 0;
		item.m_itemId = i;
		long itemid = m_ItemListCtrl->InsertItem(item);
		m_ItemListCtrl->SetItemData( itemid , i );
		updateItemList( itemid );
	}
}

void ItemEditorFrame::OnCommand( wxCommandEvent& event )
{
	switch ( event.GetId() )
	{
	case COM_SAVE_ITEM_DATA:
		TItemManager::getInstance().saveItemData( ITEM_DATA_PATH );
		break;
	case COM_CHANGE_ITEM_ID:
		{
			unsigned itemID = m_ItemListCtrl->GetItemData( selectItemIndex );
			ItemIDChangeDlg dlg( itemID , this );
			if ( dlg.ShowModal() == wxID_OK )
			{
				unsigned newID = TItemManager::getInstance().changeItemID( itemID , dlg.changeID );
				m_ItemListCtrl->SetItemData( selectItemIndex , newID );
				updateItemList( selectItemIndex );
				//refreshItemListCtrl();
			}
		}
		break;
	case  COM_DELETE_ITEM:
		assert( selectItemIndex > 0 );
		{
			unsigned itemID = m_ItemListCtrl->GetItemData( selectItemIndex );
			TItemManager::getInstance().deleteItem( itemID );
			m_ItemListCtrl->DeleteItem( selectItemIndex );
		}
		break;
	case  COM_EDIT_SKILL_ICON:
		{
			SkIconEditorDlg* dlg = new SkIconEditorDlg( this );
			dlg->Show( true );
		}
		break;
	case COM_SAVE_SKILL_DATA:
		TSkillLibrary::getInstance().saveIconName( SKILL_DATA_PATCH );
		break;
	case COM_TRANS_OBJ_TO_CW3:
		transOBJ2WC3();
		break;
	}
	//event.Skip();
}



void ItemEditorFrame::OnDrawPanelMouse( wxMouseEvent& event )
{
	static int oldX = 0;
	static int oldY = 0;

	int x = event.GetX();
	int y = event.GetY();
	if (x >= 32767) x -= 65536;
	if (y >= 32767) y -= 65536;

	if ( event.LeftDown() )
	{
		oldX = x;
		oldY = y;
	}

	if ( event.Dragging() && event.LeftIsDown() )
	{
		if ( event.ControlDown() )
		{
			int dx = x - oldX;
			int dy = y - oldY;

			static CFly::AxisEnum  const axis[3] = { CFly::CF_AXIS_X , CFly::CF_AXIS_Y , CFly::CF_AXIS_Z };
			if ( mModelObj )
				mModelObj->rotate( axis[ rotateAxis ] , 0.01 * ( abs(dx) > abs(dy) ? dx : dy ) , CFly::CFTO_LOCAL  );
			//m_objCtrl->roateObject( x,y );
			m_ModifyButton->Enable();
		}
		else if ( event.ShiftDown() )
		{
			//m_objCtrl->moveObject( x, y , 0 );
			m_ModifyButton->Enable();
		}
		else
		{
			mCamCtrl.rotateByMouse( x - oldX , y - oldY );
		}
		oldX = x;
		oldY = y;
	}

	if ( event.LeftUp() )
	{
		//m_objCtrl->roateObject( x , y , false );
		//m_objCtrl->moveObject( x , y , 0 , false  );
	}
	if ( event.Dragging() && event.RightIsDown() )
	{
		//m_objCtrl->moveObject( x ,y , 1 , false );
	}
	if ( event.RightUp() )
	{
		//m_objCtrl->moveObject( x , y , false );
	}


	event.Skip();
}

void ItemEditorFrame::OnIdle( wxIdleEvent& event )
{
	static DWORD beforeTime = GetTickCount();
	unsigned numLoops=0;

	DWORD  presentTime = GetTickCount();
	if ( (presentTime - beforeTime ) >= m_updateTime )
	{
		mHeroEntity->update( m_updateTime );

		//getGame()->update( 1 );
		//m_objCtrl->update();
		beforeTime += m_updateTime;
		++numLoops;
	}

	m_DrawPanel->Refresh();
	event.RequestMore( numLoops > 1 );
}

void ItemEditorFrame::OnDraw( wxEraseEvent& event )
{
	mCFCamera->setLookAt( mCamera.getPosition() , mCamera.getLookPos() , Vec3D(0,0,1) );
	mCFScene->render( mCFCamera , mCFViewport );

	char str[256];
	mCFWorld->beginMessage();
	if ( mModelObj )
	{
		
		Vec3D pos = mModelObj->getLocalPosition();
		sprintf_s( str , sizeof( str ) , "pos = ( %.2f , %.2f , %.2f )" , pos.x , pos.y , pos.z );
		mCFWorld->showMessage( 5 , 5 , str , 255 , 255 , 0 );
		Quat q;
		q.setMatrix( mModelObj->getLocalTransform() );
		sprintf_s( str , sizeof( str ) , "q = ( %.3f , %.3f , %.3f , %.3f )" , q.x , q.y , q.z , q.w );
		mCFWorld->showMessage( 5 , 5 + 15 , str , 255 , 255 , 0 );
	}
	Vec3D pos = mCamera.getLookPos();
	sprintf_s( str , sizeof( str ) , "look pos = ( %.2f , %.2f , %.2f )" , pos.x , pos.y , pos.z );
	mCFWorld->showMessage( 5 , 5 + 2 * 15 , str , 255 , 255 , 0 );
	mCFWorld->endMessage();

	mCFWorld->swapBuffers();
}

void ItemEditorFrame::OnDrawPanelResize( wxSizeEvent& event )
{
	//wxSize const& size = event.GetSize();
	wxSize const& size = m_DrawPanel->GetClientSize();

	//mCFWorld->resize( size.x , size.y );
	//mCFViewport->setSize( size.x , size.y );
	mCFCamera->setAspect( float(size.x) / size.y );
}

void ItemEditorFrame::OnNewItemButton( wxCommandEvent& event )
{
	ItemDataInfo info;

	info.type = (ItemType)m_ItemTypeChoice->GetSelection();
	
	info.buyCost = 1;
	info.sellCost = 0;
	info.maxNumInSolt = 1;
	info.equiVal = 0;
	if ( info.type == ITEM_WEAPON )
	{
		info.name = "weapon_new";
		info.slot = EQS_MAIN_HAND;
	}
	else if ( info.type == ITEM_ARMOUR )
	{
		info.name = "armor_new";
		info.slot = EQS_HEAD;
	}
	else
	{
		info.name = "item_new";
	}

	TItemBase* item = TItemManager::getInstance().createItem( &info );

	wxListItem listItem;
	listItem.m_col = 0;
	listItem.m_itemId = item->getID();
	long itemid = m_ItemListCtrl->InsertItem(listItem);

	m_ItemListCtrl->SetItemData( itemid , item->getID() );

	listItem.SetMask( wxLIST_MASK_STATE );
	listItem.SetState( wxLIST_STATE_SELECTED | wxLIST_STATE_FOCUSED );

	m_ItemListCtrl->SetItem( listItem );
	m_ItemListCtrl->EnsureVisible( itemid );

	updateItemList( itemid );
}

void ItemEditorFrame::OnItemListSelected( wxListEvent& event )
{
	selectItemIndex = event.GetIndex();

	if ( selectItemIndex < 0 )
		return;

	unsigned itemID = m_ItemListCtrl->GetItemData( selectItemIndex );
	TItemBase* item =  TItemManager::getInstance().getItem( itemID );

	if ( item )
	{
		m_EditPanel->Enable();
		item->getDataInfo( mItemInfo );


		GuiIO::wxOutputEval eval;
		processGUIStream( eval );

		if ( item->isEquipment() )
		{
			CFObject* obj[2];
			TEquipment* equip = static_cast< TEquipment* >( item );
			if ( equip ->createModel( obj ) )
			{
				changeEquipModel(  equip , obj[0] );
			}
			//getGame()->changeModel( selectItemID  , m_ModelObj );
			//m_objCtrl->setSelectObj( m_ModelObj );
		}
		m_ModifyButton->Enable( false );
	}

	event.Skip();
}


void ItemEditorFrame::OnItemListDeselected( wxListEvent& event )
{
	m_EditPanel->Enable( false );
	event.Skip();
}
void ItemEditorFrame::OnItemListRightClick( wxListEvent& event )
{
	if ( selectItemIndex < 0 )
		return;

	wxMenu* menu = new wxMenu;
	menu->Append( COM_CHANGE_ITEM_ID , wxT("Change ID") );
	menu->Append( COM_DELETE_ITEM    , wxT("Delete Item") );
	m_ItemListCtrl->PopupMenu( menu , event.GetPoint() );
	event.Skip();
}

void ItemEditorFrame::updatePMListCtrl()
{
	for ( int i = m_PMListCtrl->GetItemCount(); i < mItemInfo.PMVec.size(); ++ i)
	{
		wxListItem item;
		item.m_col = 0;
		item.m_itemId = i;
		m_PMListCtrl->InsertItem( item );
	}

	for ( int i = mItemInfo.PMVec.size(); i < m_PMListCtrl->GetItemCount(); ++ i)
	{
		m_PMListCtrl->DeleteItem( m_PMListCtrl->GetItemCount() - 1 );
	}

	wxChar const* opStr[] = { "+" , "x" , "-" , "/" };

	for ( int i = 0 ;i < mItemInfo.PMVec.size() ; ++i )
	{
		PropModifyInfo& info = mItemInfo.PMVec[i];
		int op = info.flag & PF_VALUE_OPERAOTR;
		m_PMListCtrl->SetItem( i , 0 , PropTypeStr[info.prop] );
		m_PMListCtrl->SetItem( i , 1 , wxString( opStr[op] ) + " " + wxString::Format( "%.2f" , info.val ) );
		m_PMListCtrl->SetItem( i , 2 , wxString::Format( "%.2f" , info.time ) );

		wxString flagStr;
		if ( info.flag & PF_TOP_ADD )   flagStr += "TOP_ADD | ";
		if ( info.flag & PF_TOP_SUB )   flagStr += "TOP_SUB | ";
		if ( info.flag & PF_NO_REPEAT ) flagStr += "NO_REPEAT | ";

		flagStr.Remove( flagStr.length() - 2 );

		m_PMListCtrl->SetItem( i , 3 , flagStr );
		m_PMListCtrl->SetItemData( i , i );
	}

}

void ItemEditorFrame::OnPMAddClick( wxCommandEvent& event )
{
	PropModifyInfo info;
	info.prop = PROP_HP;
	info.val   = 0;
	info.flag = PF_VOP_ADD;
	
	PropModifyDlg dlg( &info , this );

	if ( dlg.ShowModal() == wxID_OK )
	{
		mItemInfo.PMVec.push_back( info );
		updatePMListCtrl();
		m_ModifyButton->Enable();
	}

	event.Skip();
}

void ItemEditorFrame::OnPMListSelected( wxListEvent& event )
{
	selectPMIndex = event.GetIndex();

	m_PMModifyButton->Enable();
	m_PMRemoveButton->Enable();

	event.Skip();
}

void ItemEditorFrame::OnPMListDeselected( wxListEvent& event )
{
	selectPMIndex = -1;

	m_PMModifyButton->Enable( false );
	m_PMRemoveButton->Enable( false );

	event.Skip();
}

void ItemEditorFrame::OnPMModifyClick( wxCommandEvent& event )
{
	if ( selectPMIndex < 0 )
		return;

	PropModifyInfo& info = mItemInfo.PMVec[ selectPMIndex ];

	PropModifyDlg dlg( &info , this );

	if ( dlg.ShowModal() == wxID_OK )
	{
		updatePMListCtrl();
		m_ModifyButton->Enable();
	}
	event.Skip();
}

void ItemEditorFrame::OnPMRemoveClick( wxCommandEvent& event )
{
	if ( selectPMIndex < 0 )
		return;

	mItemInfo.PMVec.erase( mItemInfo.PMVec.begin() + selectPMIndex );
	updatePMListCtrl();
	m_ModifyButton->Enable();
	event.Skip();
}

void ItemEditorFrame::OnItemTypeChoice( wxCommandEvent& event )
{
	event.Skip();
}

void ItemEditorFrame::OnModifyItem( wxCommandEvent& event )
{
	if ( selectItemIndex < 0 )
		return;

	GuiIO::wxInputEval eval;
	processGUIStream( eval );

	TItemManager::getInstance().modifyItem( selectItemIndex ,  mItemInfo );

	m_ItemNameTextCtrl->SetValue( mItemInfo.name.c_str() );
	updateItemList( selectItemIndex );

	m_ModifyButton->Enable( false );
	event.Skip();
}



template< class IOEval >
void ItemEditorFrame::processGUIStream( IOEval& eval )
{
	using namespace GuiIO;

	eval << GuiStream( this ,  &ItemEditorFrame::updateGUI );

	eval & GuiStream( m_ItemNameTextCtrl , mItemInfo.name )
		 & GuiStream( m_ItemTypeChoice   , mItemInfo.type )
		 & GuiStream( m_MaxSoltNumSpin   , mItemInfo.maxNumInSolt )
		 & GuiStream( m_BuyCastSpin      , mItemInfo.buyCost)
		 & GuiStream( m_SellCastSpin     , mItemInfo.sellCost )

	     & GuiStream( m_IconCBox         , mItemInfo.iconID )
		 & GuiStream( m_CDTimeTCtrl      , mItemInfo.cdTime )
		 & GuiStream( m_HelpStrTCtrl     , mItemInfo.helpStr );


	if ( mItemInfo.type == ITEM_WEAPON || 
		 mItemInfo.type == ITEM_ARMOUR )
	{
		eval & GuiStream( m_EquipValSpin    ,  mItemInfo.equiVal )
			 & GuiStream( m_EquipSoltChoice ,  mItemInfo.slot )
			 & GuiStream( m_ModelNameTCtrl  ,  mItemInfo.modelName )
		     & GuiStream( m_ScaleTCtrl      ,  mItemInfo.scale );
	}

	if ( mItemInfo.type == ITEM_WEAPON )
	{
		eval & GuiStream( m_WeaponTypeChoice , mItemInfo.weaponType );
	}

	eval >> GuiStream( this , &ItemEditorFrame::updateData );
};


void ItemEditorFrame::updateData()
{

	if ( mItemInfo.type == ITEM_WEAPON )
	{
		mItemInfo.slot = EquipSlot( EQS_TWO_HAND + mItemInfo.slot );
	}
	else if ( mItemInfo.type == ITEM_ARMOUR )
	{

	}

	if ( mItemInfo.type == ITEM_ARMOUR || 
		 mItemInfo.type == ITEM_WEAPON )
	{
		mItemInfo.pos = mModelObj->getLocalPosition();
		mItemInfo.rotation.setMatrix( mModelObj->getLocalTransform() );

		Mat4x4 trans;
		trans.setQuaternion( mItemInfo.rotation );

		int i = 1;
	}
}

void ItemEditorFrame::updateGUI()
{

	if ( mItemInfo.type == ITEM_WEAPON )
	{
		m_EquipSoltChoice->Clear();
		m_EquipSoltChoice->Append( WXARRAYSTR( weaponSoltStr ) );
		mItemInfo.slot =  EquipSlot( mItemInfo.slot - EQS_TWO_HAND );

		m_EquimentPanel->Show( true );
	}
	else if ( mItemInfo.type == ITEM_ARMOUR )
	{
		m_EquipSoltChoice->Clear();
		m_EquipSoltChoice->Append( WXARRAYSTR( armourSoltStr ) );

		m_EquimentPanel->Show( true );
	}
	else
	{
		m_EquimentPanel->Hide();
	}

	m_WeaponPanel->Show( mItemInfo.type == ITEM_WEAPON  );

	m_EditPanel->Layout();
	updatePMListCtrl();
}

void ItemEditorFrame::OnIconClick( wxCommandEvent& event )
{
	if ( event.GetId() == ID_AddIconButton || 
		 event.GetId() == ID_ModifyIconButton )
	{
		wxFileDialog dialg( this , wxT("Select Picture") , ItemIconDir , wxEmptyString ,
			"PNG files (*.png)|*.png" );

		if ( dialg.ShowModal() == wxID_OK )
		{
			wxString path = dialg.GetPath();
			wxString name = dialg.GetFilename();

			name[ name.find('.')] = '\0';
			wxImage img( path , wxBITMAP_TYPE_PNG );

			if ( img.IsOk() )
			{
				if ( event.GetId() == ID_AddIconButton )
				{
					int index = m_IconCBox->Append( wxString() << m_iconNameVec.size() , wxBitmap( img.Scale( g_IconSize , g_IconSize ) ) , (void*)NULL  );
					m_IconCBox->SetSelection( index );
					m_iconNameVec.push_back( name.c_str() );
				}
				else
				{
					int index = m_IconCBox->GetSelection();
					if ( index >= 0 )
					{
						m_IconCBox->SetItemBitmap( index , wxBitmap( img.Scale( g_IconSize , g_IconSize ) ) );
						m_iconNameVec[index] = name.c_str();
					}
				}
			}
		}
	}
	else
	{

	}
}

void ItemEditorFrame::OnChioceModelClick( wxCommandEvent& event )
{
	wxFileDialog dialg( this , wxT("Select Model") , ITEM_DATA_DIR , wxEmptyString ,
		"Fly files (*.cw3)|*.cw3|Obj files (*.obj)|*.obj" );

	if ( dialg.ShowModal() == wxID_OK )
	{
		wxString name = dialg.GetFilename();
		name.find_last_of('.');

		size_t pos = name.find('.');
		wxChar const* subName = name.c_str() + ( pos );

		if ( wxString(".obj") == subName )
		{
			wxString path = dialg.GetPath();
			CFly::ObjModelData data;
			CFly::LoadObjFile( path.c_str() , data );
			name[ name.find('.')] = '\0';
			path.Replace( wxT(".obj") , wxT(".cw3") );
			CFly::saveCW3File( path.c_str() , data );
		}

		name[ pos ] = '\0';

		m_ModelNameTCtrl->SetValue( name );
		changeModel( name );
	}
}

void ItemEditorFrame::OnKeyDown( wxKeyEvent& event )
{
	int key = event.GetKeyCode();

	switch( key )
	{
	case 'W': mCamCtrl.moveForward(); break;
	case 'S': mCamCtrl.moveBack();    break;
	case 'A': mCamCtrl.moveLeft();    break;
	case 'D': mCamCtrl.moveRight();   break;
	case 0x31: rotateAxis = 0; break;
	case 0x32: rotateAxis = 1; break;
	case 0x33: rotateAxis = 2; break;
	}
}

void ItemEditorFrame::changeModel( char const * name )
{
	//m_ModelObj = TResManager::getInstance().cloneModel( name );
	//getGame()->changeModel( selectItemID  , m_ModelObj );
	//m_objCtrl->setSelectObj( m_ModelObj );
}

void ItemEditorFrame::changeEquipModel( TEquipment* equip , CFObject* model )
{
	if( !equip )
		return;

	if ( equip->isWeapon() )
	{
		mHeroLogicComp->changeEquipmentModel( EQS_HAND_R , model );
	}
	else
	{
		mHeroLogicComp->changeEquipmentModel( equip->getEquipSolt() , model );
	}

	mModelObj = model;
}

void ItemEditorFrame::transOBJ2WC3()
{
	wxFileDialog dialg( this , wxT("Select Model") , ITEM_DATA_DIR , wxEmptyString ,
		"Obj files (*.obj)|*.obj" );

	if ( dialg.ShowModal() == wxID_OK )
	{
		wxString name = dialg.GetFilename();
		name.find_last_of('.');

		size_t pos = name.find('.');
		wxChar const* subName = name.c_str() + ( pos );

		if ( wxString(".obj") == subName )
		{
			wxString path = dialg.GetPath();
			CFly::ObjModelData data;
			CFly::LoadObjFile( path.c_str() , data );
			name[ name.find('.')] = '\0';
			path.Replace( wxT(".obj") , wxT(".cw3") );
			CFly::saveCW3File( path.c_str() , data );
		}
	}
}

static int wxCALLBACK ItemIDCmp( long data1, long data2, long sortData )
{
	if ( data1 == data2 )
		return 0;
	if ( data1 > data2 )
		return sortData;
	return -sortData;
}

static int wxCALLBACK ItemNameCmp( long data1, long data2, long sortData )
{
	TItemBase* item1 = TItemManager::getInstance().getItem( data1 );
	if ( !item1 )
		return -sortData;
	TItemBase* item2 = TItemManager::getInstance().getItem( data2 );
	if ( !item2 )
		return sortData;

	return sortData * strcmp( item1->getName() , item2->getName() );
}

static int wxCALLBACK ItemTypeCmp( long data1, long data2, long sortData )
{
	TItemBase* item1 = TItemManager::getInstance().getItem( data1 );
	if ( !item1 )
		return -sortData;
	TItemBase* item2 = TItemManager::getInstance().getItem( data2 );
	if ( !item2 )
		return sortData;

	long result  = ItemIDCmp( item1->getType() , item2->getType() , sortData );

	if ( !result && item1->isEquipment() )
	{
		if ( item1->isArmour() )
		{
			return result = ItemIDCmp( 
				static_cast< TArmour*>( item1 )->getEquipSolt() ,
				static_cast< TArmour*>( item2 )->getEquipSolt() , sortData );
		}
		else if ( item2->isWeapon() )
		{
			return result = ItemIDCmp( 
				static_cast< TWeapon*>( item1 )->getWeaponType() ,
				static_cast< TWeapon*>( item2 )->getWeaponType() , sortData );
		}
	}
	return result;
}


void ItemEditorFrame::OnItemListColClick( wxListEvent& event )
{
	static int prevClickCol = -1;
	static long factor = 1;

	if ( event.GetColumn() == prevClickCol )
		factor = -factor;
	else
		factor = 1;
	prevClickCol = event.GetColumn();

	switch( event.GetColumn() )
	{
	case ITEMLIST_ID: m_ItemListCtrl->SortItems( ItemIDCmp , factor ); break;
	case ITEMLIST_NAME: m_ItemListCtrl->SortItems( ItemNameCmp , factor ); break;
	case ITEMLIST_TYPE: m_ItemListCtrl->SortItems( ItemTypeCmp , factor ); break;
	}
}

void ItemEditorFrame::OnEditModelCheck( wxCommandEvent& event )
{
	if ( event.IsChecked() )
	{
		m_DrawPanel->SetClientSize( wxSize( 500 , -1 ) );
	}
	else
	{
		m_DrawPanel->SetClientSize( wxSize( 200 , -1 ) );
	}
	m_EditPanel->Layout();
}

void ItemEditorFrame::OnChangeSetting( wxCommandEvent& event )
{
	m_ModifyButton->Enable();
}

void ItemEditorFrame::OnChangeSetting( wxSpinEvent& event )
{
	m_ModifyButton->Enable();
}

void ItemEditorFrame::OnModelNameRemove( wxCommandEvent& event )
{
	mItemInfo.modelName.clear();
	m_ModelNameTCtrl->Clear();
	OnChangeSetting( event );
}

PropModifyDlg::PropModifyDlg( PropModifyInfo* info , wxWindow* parent , wxWindowID id /*= wxID_ANY */ ) 
	:PropModifyDlgUI( parent , id )
{
	m_info = info;
	m_PropTyeChoice->Append( WXARRAYSTR( PropTypeStr ) );
}

template< class IOEval >
void PropModifyDlg::processGUIStream( IOEval& eval )
{
	using namespace GuiIO;
	
	eval << GuiStream( this , &PropModifyDlg::updateGUI );

	eval & GuiStream( m_PropTyeChoice  , m_info->prop )
		 & GuiStream( m_ValOpChoice    , valOp )
	     & GuiStream( m_ValueTextCtrl  , m_info->val )
		 & GuiStream( m_TimeTextCtrl   , m_info->time );

	eval >> GuiStream( this ,  &PropModifyDlg::updateData );
}

bool PropModifyDlg::TransferDataToWindow()
{
	GuiIO::wxOutputEval eval;
	processGUIStream( eval );
	return true;
}


bool PropModifyDlg::TransferDataFromWindow()
{
	GuiIO::wxInputEval eval;
	processGUIStream( eval );
	return true;
}

void PropModifyDlg::updateGUI()
{
	//prop  = m_info->prop;
	valOp = m_info->flag & PF_VALUE_OPERAOTR;
	m_NoRepeatCheckBox->SetValue( m_info->flag & PF_NO_REPEAT );
	m_TAddCheckBox -> SetValue( m_info->flag & PF_TOP_ADD );
	m_TSubCheckBox -> SetValue( m_info->flag & PF_TOP_SUB );
}


void PropModifyDlg::updateData()
{
	//m_info->prop = PropType( prop );

	m_info->flag |=  valOp;
	m_info->flag |= ( m_NoRepeatCheckBox->IsChecked() ) ? PF_NO_REPEAT : 0 ;
	m_info->flag |= ( m_TAddCheckBox->IsChecked() ) ? PF_TOP_SUB : 0 ;
	m_info->flag |= ( m_TSubCheckBox->IsChecked() ) ? PF_TOP_ADD : 0 ;
}

SkIconEditorDlg::SkIconEditorDlg( wxWindow* parent, wxWindowID id /*= wxID_ANY */ ) :SkIconEditorDlgUI( parent , id )
{
	char const* skillName[128];
	int num = TSkillLibrary::getInstance().getAllSkillName( skillName , ARRAY_SIZE( skillName ) );

	for( int i = 0 ; i < num ; ++i )
	{
		m_SKChoice->Append( skillName[i] );

		char const* iconName = TSkillLibrary::getInstance().getIconName( skillName[i] );

		if ( iconName )
		{
			wxString path = SkillIconDir + iconName + wxT(".png") ;

			wxImage img( path , wxBITMAP_TYPE_PNG );

			if ( img.IsOk() )
			{
				m_IconCBox->Append( iconName , wxBitmap( img ) , (void*)NULL  );
			}
			else
			{
				m_IconCBox->Append( iconName );
			}
		}	
	}
}

void SkIconEditorDlg::OnIconClick( wxCommandEvent& event )
{
	if ( event.GetId() == ID_AddIconButton || 
		event.GetId() == ID_ModifyIconButton )
	{
		wxFileDialog dialg( this , wxT("Select Picture") , SkillIconDir , wxEmptyString ,
			"PNG files (*.png)|*.png" );

		if ( dialg.ShowModal() == wxID_OK )
		{
			wxString path = dialg.GetPath();
			wxString name = dialg.GetFilename();

			name[ name.find('.')] = '\0';
			wxImage img( path , wxBITMAP_TYPE_PNG );

			if ( img.IsOk() )
			{
				if ( event.GetId() == ID_AddIconButton )
				{
					int index = m_IconCBox->Append( name , wxBitmap( img ) , (void*)NULL  );
					wxString skillName = m_SKChoice->GetStringSelection();
					TSkillLibrary::getInstance().setIconName( skillName.c_str() , name.c_str() );
					m_IconCBox->SetSelection( index );
				}
				else
				{
					int index = m_IconCBox->GetSelection();
					if ( index >= 0 )
					{
						m_IconCBox->SetItemBitmap( index , wxBitmap( img ) );
						m_IconCBox->SetString( index , name );
					}
				}
			}
		}
	}
	event.Skip();
}


void SkIconEditorDlg::OnIconSelect( wxCommandEvent& event )
{
	int index = m_SKChoice->GetCurrentSelection();
	wxString& name = m_SKChoice->GetString( index );
	wxString& iconName = m_IconCBox->GetString( event.GetSelection() );

	TSkillLibrary::getInstance().setIconName( name.c_str() , iconName.c_str() );
}

void SkIconEditorDlg::OnSKChoice( wxCommandEvent& event )
{
	wxString& str = event.GetString();

	char const* iconName = TSkillLibrary::getInstance().getIconName( str.c_str() );
	if ( iconName )
	{
		int index = m_IconCBox->FindString( iconName );
		m_IconCBox->SetSelection( index );
	}
	else
	{
		m_IconCBox->SetSelection(-1);
	}
}

void SkIconEditorDlg::OnOKClick( wxCommandEvent& event )
{
	TSkillLibrary::getInstance().saveIconName( SKILL_DATA_PATCH );
	event.Skip(false);
}