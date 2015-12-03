#include "WorldEditorFrame.h"

#include <wx/process.h>

#include "common.h"
#include "TLevel.h"
#include "TActor.h"
#include "TMessageShow.h"
#include "TEntityManager.h"
#include "TResManager.h"
#include "TPhySystem.h"
#include "TEventSystem.h"
#include "TLevel.h"
#include "UtilityFlyFun.h"

#include "TActorTemplate.h"
#include "TEditDataProp.h"

#include "debug.h"

#include "icon1.xpm"
#include "icon2.xpm"
#include "icon3.xpm"
#include "icon4.xpm"
#include "icon5.xpm"


enum
{
	ID_TERRAIN_TOOL_DLG = 70000,
};

enum
{
	COM_START = 30000 ,
	COM_GO_TO_OBJECT ,

	COM_SELECT_OBJECT ,
	COM_SELECT_ROLE ,

	COM_SAVE_ROLE_DATA ,

	COM_DELETE_OBJ ,
	COM_SAVE_EDIT_DATA ,
	COM_SAVE_TERRAIN_DATA ,
	COM_SET_TERRAIN ,
	COM_OPEN_SCENE ,
	COM_EDIT_MODEL ,
	COM_SAVE_MODEL_DATA,
	COM_TRANS_SCENE_TO_CW4 ,

	COM_TERRAIN_TOOL ,

	COM_DELETE_EDIT_DATA ,

	COM_EDIT_ROLE ,
	COM_NEW_ROLE  ,


	COM_CREATE_DATA_BEGIN ,
	COM_CREATE_ACTOR ,
	COM_CREATE_PLAYER_BOX_TGR ,
	COM_CREATE_LOGIC_GROUP ,
	COM_CREATE_CHANGE_LEVEL ,
	COM_CREATE_OBJECT ,
	COM_CREATE_PHY_OBJ ,
	COM_CREATE_SPAWN_ZONE ,

	COM_CREATE_DATA_END ,

	COM_CREATE_GROUP ,

	COM_EDIT_LOGIC_GROUP ,
	COM_COPY_OBJ ,
	
	COM_END,
};


BEGIN_EVENT_TABLE( WorldEditorFrame , WorldEditorFrameUI )
	EVT_COMMAND_RANGE(COM_START,COM_END, wxEVT_FLY_EVENT , WorldEditorFrame::OnCommand)
	EVT_MENU_RANGE(COM_START,COM_END,WorldEditorFrame::OnCommand )
	EVT_PG_CHANGED( ID_ObjPropGrid , WorldEditorFrame::OnPGChange )
	EVT_IDLE (WorldEditorFrame::OnIdle)
	//EVT_UPDATE_UI( ID_ObjListCtrl , OnUpdateUI )
END_EVENT_TABLE()

DEFINE_EVENT_TYPE(wxEVT_FLY_EVENT)


int  g_createIndex = 0;

TerLiftEffect    s_HeightEffect;
TerTextureEffect s_TextureEffect;
TerSmoothEffect  s_SmoothEffect;

TerEffect* g_terEffect = &s_HeightEffect;

class EditTreeItemData : public wxTreeItemData
{
public:
	EditTreeItemData( EmDataBase* data )
		:editData(data){}
	EmDataBase* editData;
};


WorldEditorFrame::WorldEditorFrame( wxWindow* parent, wxWindowID id /*= wxID_ANY */ ) 
	:WorldEditorFrameUI( parent , id )
{
	m_curEditData = NULL;
	m_isGameOver = false;
	m_FrameTime = 0;
	m_updateTime = g_GlobalVal.frameTick;

	m_mode = MODE_EDIT_TERRAIN;


	wxMenu *fileMenu = new wxMenu;
	fileMenu->Append( COM_OPEN_SCENE         , wxT("&Open Scene") );
	fileMenu->Append( COM_TRANS_SCENE_TO_CW4 , wxT("Trans Scene To CW4") );
	fileMenu->AppendSeparator();
	fileMenu->Append( COM_SAVE_MODEL_DATA    , wxT("&Save Model Data") );
	fileMenu->Append( COM_SAVE_EDIT_DATA     , wxT("&Save Edit Data \tAlt-X") );
	fileMenu->Append( COM_SAVE_TERRAIN_DATA  , wxT("&Save Terrain Obj \tAlt-Y") );
	fileMenu->Append( COM_SAVE_ROLE_DATA     , wxT("Save Role Data") );


	wxMenu*viewMenu = new wxMenu;
	viewMenu->Append( COM_TERRAIN_TOOL , wxT("&Terrain Tool") );

	wxMenuBar *menuBar = new wxMenuBar;
	menuBar->Append(fileMenu , _T("&File"));
	menuBar->Append(viewMenu , _T("&View"));


	SetMenuBar(menuBar);


	wxPGId pgid;

	m_ObjPropGrid->SetFont( wxFont( 9 , 74, 90, 90, false, wxT("@·s²Ó©úÅé") ) );
	m_ObjPropGrid->SetMarginColour(wxColour(200,255,255) );


	propLoader.m_propGrid = m_ObjPropGrid;


	m_ModelList->InsertColumn(1,wxT("ID"));
	m_ModelList->InsertColumn(2,wxT("Name"));


	m_RoleListCtrl->InsertColumn(0,wxT("ID"));
	m_RoleListCtrl->InsertColumn(1,wxT("Name"));

	selectModelID = -1;

	m_GroupTreeCtrl->SetDoubleBuffered( true );

	createTreeIcon();

	//m_objListCtrl->SetColumnWidth(0,240);
}


void WorldEditorFrame::init()
{
	manager = &TResManager::instance();
	m_objEdit = getWorldEditor();

	propLoader.output("Group");
	m_ObjPropGrid->Refresh();

	reloadEditTree();

	//m_ObjPropGrid->SetPropertyAttributeAll(wxPG_BOOL_USE_CHECKBOX,(long)1);

	loadRole();
	loadModel();

	brush.radius   = 10;
	brush.falloff  = 50;
	brush.strength = 100;

	brush.texLayer = 1;

	heighField = new THeighField( getGame()->getCurLevel()->getFlyScene() , 64 , 64 , 50 );
	brushObj   = new BrushObj( getGame()->getCurLevel()->getFlyScene() , brush , 30 );
}

void WorldEditorFrame::reloadEditTree()
{
	m_GroupTreeCtrl->DeleteAllItems();

	wxTreeItemId rootID = m_GroupTreeCtrl->AddRoot( wxT("Root") , -1 , -1 ,
		new EditTreeItemData( EmGroupManager::instance().getGroupData(ROOT_GROUP) ) );

	EmGroup* gData = EmGroupManager::instance().getRootGroupData();

	for( EmGroup::iterator iter = gData->children.begin() ;
		 iter != gData->children.end() ; ++iter )
	{
		addTreeItem( *iter , rootID );
	}
}


void WorldEditorFrame::addTreeItem( EmDataBase* eData , wxTreeItemId parentID )
{
	class TreeVisitor : public EmVisitor
	{
#define DEFULT_VISIT( type , icon )\
		void visit( type* eData )\
		{\
			wxTreeItemId childID = treeCtrl->AppendItem(\
			parentID , eData->getName(), icon , icon  , new EditTreeItemData( eData ) );\
		}
#define VISIT( type )\
	void visit( type* eData )

		DEFULT_VISIT( EmSceneObj     , 2 )
		DEFULT_VISIT( EmPlayerBoxTGR , 2 )
		DEFULT_VISIT( EmActor        , 2 )
		DEFULT_VISIT( EmChangeLevel  , 2 )
		DEFULT_VISIT( EmLogicGroup   , 2 )
		DEFULT_VISIT( EmPhyObj       , 2 )
		DEFULT_VISIT( EmSpawnZone    , 2 )

		VISIT( EmGroup )
		{
			wxTreeItemId childID = treeCtrl->AppendItem(
				parentID , eData->getName(), 0 , 0  , new EditTreeItemData( eData ) );
			for ( EmGroup::iterator iter = eData->children.begin(); 
				iter != eData->children.end() ; ++iter )
			{
				frame->addTreeItem( *iter , childID );
			}
		}

#undef  DEFULT_VISIT
#undef  VISIT
	public:
		WorldEditorFrame* frame;
		wxTreeItemId parentID;
		wxTreeCtrl*  treeCtrl;
	} visitor;

	visitor.frame    = this;
	visitor.treeCtrl = m_GroupTreeCtrl;
	visitor.parentID = parentID;

	eData->accept( visitor );

	m_GroupTreeCtrl->Refresh( parentID );
}

void WorldEditorFrame::loadModel()
{
	m_ModelList->DeleteAllItems();
	int num = manager->getResNum( RES_ACTOR ) + manager->getResNum( RES_OBJECT );
	for ( int i = 0 ; i < num ; ++i )
	{
		wxListItem item;
		item.m_col = 0;
		item.m_itemId = i;
		long itemid = m_ModelList->InsertItem(item);
	}
	updateModelListCtrl();
}


void WorldEditorFrame::loadRole()
{
	m_RoleListCtrl->DeleteAllItems();

	int num = TRoleManager::instance().getMaxRoleID();
	for ( int i = 0 ; i <= num ; ++i )
	{
		wxListItem item;
		item.m_col = 0;
		item.m_itemId = i;
		long itemid = m_RoleListCtrl->InsertItem(item);
	}
	updateRoleListCtrl();
}


void WorldEditorFrame::updateRoleListCtrl()
{

	int num = TRoleManager::instance().getMaxRoleID();

	for ( int i = 0 ; i <= num ; ++i )
	{
		SRoleInfo* data =  TRoleManager::instance().getRoleInfo( i );

		if ( !data )
			continue;

		m_RoleListCtrl->SetItem( i , 0 , wxString()<< data->roleID );
		m_RoleListCtrl->SetItem( i , 1,  data->roleName );
		m_RoleListCtrl->SetItemPtrData( i ,(wxUIntPtr) data->roleID );
	}

}

void WorldEditorFrame::updateModelListCtrl()
{
	class ModelVisitCallback : public TResVisitCallBack
	{
	public:
		void visit( TResData* data)
		{
			TObjModelData* model = ( TObjModelData* ) data;

			listCtrl->SetItem( i , 0 , wxString::Format(wxT("%d") , data->resID ) );
			listCtrl->SetItem( i , 1, data->resName );
			listCtrl->SetItemPtrData( i ,(wxUIntPtr) data->resID );

			++i;
		}
		int i;
		wxListCtrl* listCtrl;
	} callback;

	callback.listCtrl = m_ModelList;
	callback.i = 0;

	TResManager::instance().visitRes( RES_ACTOR , callback );
	TResManager::instance().visitRes( RES_OBJECT , callback );

}




void WorldEditorFrame::addNoteBook( wxWindow* window , wxChar const* str )
{
	window->Reparent( m_Notebook );
	m_Sizer->Detach( window );
	window->Show();

	m_Notebook->AddPage( window , str );
}

void WorldEditorFrame::OnCommand( wxCommandEvent& event )
{
	static int objNameID = 1;
	static int actorNameID = 1;

	switch ( event.GetId() )
	{
	case COM_GO_TO_OBJECT:
		{
			TCamera& cam = getGame()->getCamControl();
			EmObjData* data = ( EmObjData*) getEditData( m_curMenuItemID );

			Vec3D& pos = data->getPosition();

			cam.setPosition( pos - 500 * cam.getViewDir() );
		}
		break;
	case  COM_SELECT_OBJECT:
		{
			//EditData& data = g_editDataVec[ selectEditObjIndex ];
			//m_objEdit->selectObject( data.flyID );
			//objEditData.obj.Object( data.flyID );
			//++objEditData.group;
			//updatePropGrid();
		}
		break;
	case COM_SAVE_EDIT_DATA:
		{
			String const& path = getGame()->getCurLevel()->getMapDir();
			String const& name = getGame()->getCurLevel()->getMapName();
			String file = path + "/" + name + ".oe" ;
			m_objEdit->saveEditData( file.c_str() );
		}

		break;
	case COM_TERRAIN_TOOL:
		{
			wxWindow* win = FindWindowById( ID_TERRAIN_TOOL_DLG , this );

			if ( win )
			{
				win->Layout();
				win->Show( true );
			}
			else
			{
				TerrainToolDlg* dlg = new TerrainToolDlg( *heighField , brush , *brushObj , this , ID_TERRAIN_TOOL_DLG );
				dlg->Show( TRUE );
			}
		}
		break;
	case COM_SAVE_TERRAIN_DATA:
		{
			String const& path = getGame()->getCurLevel()->getMapDir();
			String const& name = getGame()->getCurLevel()->getMapName();
			String file = path + "/" + name;
			m_objEdit->saveTerrainData( file.c_str() );
		}
		break;
	case COM_OPEN_SCENE:
		{
			wxFileDialog dialog( this , wxT("Select Scene") , wxT("Data") , wxEmptyString ,
				"Fly Scene Files (*.cwn)|*.cwn" );

			if ( dialog.ShowModal() == wxID_OK )
			{
				wxString& dir = dialog.GetDirectory();

				size_t index = dir.find_last_of('\\');
				wxString name( &dir[++index] );
				getGame()->openScene( name.c_str() );

				reloadEditTree();
			}
			break;
		}
	case COM_TRANS_SCENE_TO_CW4:
		{
			wxFileDialog dialog( this , wxT("Select Scene") , wxT("Data") , wxEmptyString ,
				"Fly Scene Files (*.cwn)|*.cwn" );

			if ( dialog.ShowModal() == wxID_OK )
			{


				wxString& dir = dialog.GetDirectory();
				
				size_t index = dir.find_last_of('\\');
				wxString name( &dir[++index] );
				getGame()->openScene( name.c_str() );

				FnWorld& world = UF_GetWorld();

				world.SetObjectPath( (char*) dir.c_str() );
				world.SetTexturePath( (char*) dir.c_str() );
				world.SetScenePath( (char*) dir.c_str() );

				FnScene scene;
				scene.Object( UF_GetWorld().CreateScene() );

				if ( scene.Load( (char*)name.c_str() ) )
				{
					scene.SaveCW4( (char*)name.char_str() );
				}

				world.DeleteScene( scene.Object() );
			}
		}
		break;
	case  COM_SAVE_ROLE_DATA:
		TRoleManager::instance().saveData( ROLE_DATA_PATH );
		break;

	case COM_SAVE_MODEL_DATA:
		//TResManager::instance().saveRes("model.txt");
		break;

	
	case  COM_DELETE_OBJ:
		{
			//m_objEdit->destorySelectObj();
			//loadEditObj();
		}
		break;

	case  COM_COPY_OBJ:
		{

		}
		break;
	case  COM_CREATE_GROUP:
		{
			wxArrayTreeItemIds arID;
			m_GroupTreeCtrl->GetSelections(arID);

			if ( arID.size() != 1)
				break;

			wxTreeItemId id = arID[0];
			wxTreeItemId parentID = m_GroupTreeCtrl->GetItemParent( id );
			EmDataBase* data = getEditData( id );
			if ( parentID.IsOk() && dynamic_cast< EmGroup*>( data ) == NULL )
			{
				id = parentID;
				data = getEditData( id );
			}
			unsigned group = 0;
			if ( data )
				group = data->getGroup();
			
			EmGroup* groupData = 
				EmGroupManager::instance().createGroup( "new group" , group );

			addTreeItem( groupData , parentID );
		}
		break;
	case COM_SET_TERRAIN:
		{
			EmGroup* gData = (EmGroup*) getEditData( m_curMenuItemID );

			for( EmGroup::iterator iter = gData->children.begin();
				 iter != gData->children.end(); ++iter) 
			{
				EmDataBase* eData = *iter;
				if ( EmObjData* objData = dynamic_cast<EmObjData*>( eData ) )
				{
					objData->setFlag( objData->getFlag() | EDF_TERRAIN );
				}
			}
		}

	case COM_EDIT_MODEL:
		{
			TObjModelData* data = TResManager::instance().getModelData( selectModelID );
			wxString command = "ActorEditor.exe ";
			command << int( data->resID );

			wxExecute( command );
		}
		break;

	case  COM_EDIT_LOGIC_GROUP:
		{
			EmLogicGroup* gData = (EmLogicGroup*) getEditData( m_curMenuItemID );
			SignalConDlg* dlg = new SignalConDlg( gData , this );
			dlg->Show( true );
		}
		break;
	case  COM_EDIT_ROLE:
		{
			propLoader.removeAll();
			SRoleInfo* data = TRoleManager::instance().getRoleInfo( m_selectRoleID );
			propLoader.load( data );
		}
		break;
	case COM_DELETE_EDIT_DATA:
		{
			getWorldEditor()->destoryEditData( getEditData( m_curMenuItemID ) );
			m_GroupTreeCtrl->Delete( m_curMenuItemID );
			//getWorldEditor()->
		}
	default:
		if ( COM_CREATE_DATA_BEGIN  < event.GetId() &&
			 COM_CREATE_DATA_END    > event.GetId() )
		{
			createEditData( event.GetId() );
		}
	}

	m_curMenuItemID = m_GroupTreeCtrl->GetRootItem();

	updatePropGrid();
}


void WorldEditorFrame::OnKeyDown( wxKeyEvent& event )
{
	TCamera& camControl = getGame()->getCamControl();

	int key = event.GetKeyCode();

	if ( key == FY_W )
		camControl.moveForward();
	if ( key == FY_S )
		camControl.moveBack();
	if ( key == FY_A )
		camControl.moveLeft();
	if ( key == FY_D )
		camControl.moveRight();
}


void WorldEditorFrame::OnRoleListSelected( wxListEvent& event )
{
	m_selectRoleID = event.GetData();

	wxCommandEvent comEvent( wxEVT_FLY_EVENT , COM_SELECT_ROLE );
	ProcessEvent( comEvent );
	event.Skip();

}

void WorldEditorFrame::OnRoleListRightDown( wxMouseEvent& event )
{
	wxMenu menu;

	menu.Append( COM_CREATE_ACTOR , wxT("Create Role") );
	menu.Append( COM_EDIT_ROLE    , wxT("Edit..") );
	menu.Append( COM_NEW_ROLE    , wxT("New Role Data") );
	m_RoleListCtrl->PopupMenu( &menu , event.GetPosition() );
}

void WorldEditorFrame::OnModelListRightDown( wxMouseEvent& event )
{
	if ( selectModelID == -1 )
		return;

	TObjModelData* data = manager->getModelData( selectModelID );

	wxMenu menu;
	if ( data->resType == RES_OBJECT )
	{
		menu.Append( COM_CREATE_PHY_OBJ , wxT("Create Physical Object"));
	}
	else if( data->resType == RES_ACTOR )
	{
		menu.Append( COM_EDIT_MODEL , wxT("Edit TActor") );
	}

	PopupMenu( &menu , event.GetPosition() );
}

void WorldEditorFrame::OnModelListSelected( wxListEvent& event )
{
	selectModelID = (int) event.GetItem().GetData();
	event.Skip();
}

void WorldEditorFrame::OnMouse( wxMouseEvent& event )
{


	switch ( m_mode )
	{
	case MODE_EDIT_TERRAIN:
		OnEditTerrainMouse( event );
		break;
	case MODE_SELECT_OBJECT:
		OnEditTerrainMouse( event );
		break;
	}


	event.Skip();
}

void WorldEditorFrame::OnEditTerrainMouse( wxMouseEvent& event )
{
	static bool editing = false;

	TObjectEditor& objEdit = *getWorldEditor();

	FnCamera& cam = getGame()->getFlyCamera();
	TObjControl& objControl = getWorldEditor()->getObjControl();

	FnViewport& viewport = getGame()->getScreenViewport();

	TCamera& camControl = getGame()->getCamControl();

	FnObject& obj = heighField->getObject();

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

	if ( event.Dragging() && event.LeftIsDown() && !event.ControlDown() )
	{
		camControl.trunViewByMouse( oldX ,oldY , x , y );
		oldX = x;
		oldY = y;
	}

	static wxPoint paintPos;
	static int     mouseOffset;


	Vec3D camPos;
	cam.GetWorldPosition( camPos );

	if ( !viewport.HitPosition( obj.Object() , cam.Object() , x , y , hitPos ) )
	{
		editing = false;
		return ;
	}

	float h;
	if ( heighField->getHeight( hitPos , &h) )
	{
		testPos = hitPos;
		testPos[2] = h;

		//DevMsg( 3 , "aa = %f %f" , hitPos.z() , h );
	}

	if ( !editing  && event.ControlDown() &&
		 ( event.LeftDown() || event.RightDown() ) )
	{
		editing = true;
	}

	wxPoint dp = paintPos - event.GetPosition(); 
	mouseOffset += abs(dp.x) + abs(dp.y);
	paintPos = event.GetPosition();

	if ( mouseOffset > 10 )
	{
		if ( event.ControlDown() && editing )
		{
			if ( event.LeftIsDown() )
			{
				brush.beInv = false;
				heighField->paint( hitPos , brush , *g_terEffect );
				mouseOffset = 0;
			}
			else if ( event.RightIsDown() )
			{
				brush.beInv = true;
				heighField->paint( hitPos , brush , *g_terEffect );
				mouseOffset = 0;
			}
		}
	}



	brushObj->updatePos( hitPos , *heighField , brush );

}

void WorldEditorFrame::OnSelectObjMouse( wxMouseEvent& event )
{
	TObjectEditor& objEdit = *getWorldEditor();
	TObjControl& objControl = getWorldEditor()->getObjControl();
	TCamera& camControl = getGame()->getCamControl();


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


	if ( event.LeftDClick() )
	{
		EmDataBase* eData = getWorldEditor()->selectEditData( x , y );
		if ( eData )
		{
			setCurEditData( eData );
		}

		//if ( TCheckHotKeyStatus(FY_CTRL) && selectModelID != -1 )
		//{
		//	TObjModelData* modelData = TResManager::instance().getModelData( selectModelID );
		//	if ( modelData->resType == RES_ACTOR )
		//	{
		//		EditData& data = m_objEdit->createActor("actor" , selectModelID );
		//		addEditData( data );
		//		FnObject obj; obj.Object( data.flyID );
		//		obj.SetPosition( (float*)&m_objEdit->getLastHitPos() );

		//		if ( TPhyEntity* phyEntity = TPhyEntity::upCast( data.entity ) )
		//		{
		//			phyEntity->updateTransformFromFlyData();
		//		}
		//	}
		//	else
		//	{
		//		EditData& data = m_objEdit->createObject("obj" , selectModelID );
		//		addEditData( data );
		//		FnObject obj; obj.Object( data.flyID );
		//		obj.SetPosition( (float*)&m_objEdit->getLastHitPos() );

		//		if ( TPhyEntity* phyEntity = TPhyEntity::upCast( data.entity ) )
		//		{
		//			phyEntity->updateTransformFromFlyData();
		//		}
		//	}
		//}
	}

	if ( event.RightDown() )
	{
		//if ( m_objEdit->getSelectIndex() >= 0 )
		//{
		//	wxMenu* menu = new wxMenu;
		//	menu->Append( COM_COPY_OBJ , wxT("Copy Obj") );
		//	m_DrawPanel->PopupMenu( menu , event.GetPosition() );
		//}
	}

	if ( event.Dragging() && event.LeftIsDown() )
	{
		if ( TCheckHotKeyStatus(FY_CTRL) )
		{
			objControl.roateObject( x , y );
		}
		else if ( TCheckHotKeyStatus(FY_SHIFT) )
		{
			objControl.moveObject( x, y , 0 );
		}
		else
		{
			camControl.trunViewByMouse( oldX ,oldY , x , y );
		}
		oldX = x;
		oldY = y;
	}

	if ( event.LeftUp() )
	{
		objControl.roateObject( x , y , false );
		objControl.moveObject( x , y , 0 , false  );
	}
	if ( event.Dragging() && event.RightIsDown() )
	{
		objControl.moveObject( x ,y , 1 , false );
	}
	if ( event.RightUp() )
	{
		objControl.moveObject( x , y , false );
	}

	updatePropGrid();
}
void WorldEditorFrame::OnIdle( wxIdleEvent& event )
{
	static DWORD beforeTime = GetTickCount();
	unsigned numLoops=0;

	DWORD  presentTime = GetTickCount();
	if ( (presentTime - beforeTime ) >= m_updateTime )
	{
		getGame()->update( 1 );
		beforeTime += m_updateTime;
		m_FrameTime += m_updateTime;
		++numLoops;
	}

	m_DrawPanel->Refresh();
	event.RequestMore( true );
}

void WorldEditorFrame::OnDrawSize( wxSizeEvent& event )
{
	wxSize& size = event.GetSize();
	if ( getGame() )
		getGame()->changeScreenSize( size.x , size.y );
	
}

void WorldEditorFrame::OnDraw( wxEraseEvent& event )
{
	getGame()->render(1);
}




void WorldEditorFrame::updatePropGrid()
{

	if ( m_curEditData )
	{
		m_curEditData->OnUpdateData();
	}
	
	propLoader.outputAll();
	m_ObjPropGrid->Refresh();

}

void WorldEditorFrame::OnPGChange( wxPropertyGridEvent & event )
{
	wxPGId id = event.GetProperty();
	wxPGVariant val = id.GetProperty().DoGetValue();

	wxString const& name = event.GetPropertyName();

	propLoader.input( name.c_str() );
}

void WorldEditorFrame::OnUpdateButton( wxCommandEvent& event )
{
	static bool start = false;

	start = !start;

	if ( start )
	{
		getGame()->setMode( MODE_RUN );
		m_UpdateButton->SetLabel( wxT("Pause Game") );
	}
	else
	{
		getGame()->setMode( MODE_EDIT );
		m_UpdateButton->SetLabel( wxT("Start Game") );
	}
}

void WorldEditorFrame::OnTreeActivated( wxTreeEvent& event )
{
	EmDataBase* editData = getEditData( event.GetItem() );
	setCurEditData( editData );
}

void WorldEditorFrame::createEditData( int comID )
{
	EmDataBase* eData = NULL ;
	wxString    name;
	switch (  comID )
	{
	case COM_CREATE_PHY_OBJ:
		eData = new EmPhyObj( selectModelID  );
		name = "PhyObject";
		break;
	case COM_CREATE_ACTOR:
		eData = new EmActor( m_selectRoleID );
		name = "TActor";
		break;
	case COM_CREATE_PLAYER_BOX_TGR:
		eData = new EmPlayerBoxTGR( Vec3D( 30 , 30 , 30 ) );
		name  = "PlayerBoxTGR";
		break;
	case  COM_CREATE_LOGIC_GROUP:
		eData = new EmLogicGroup;
		name = "LogicGroup";
		break;
	case COM_CREATE_CHANGE_LEVEL:
		eData = new EmChangeLevel( new TChangeLevel );
		name = "ChangeLevel";
		break;
	case COM_CREATE_SPAWN_ZONE:
		eData = new EmSpawnZone( Vec3D(0,0,0) );
		name = "SpawnZone";
		break;
	}

	if ( eData )
	{
		name.Append( wxString() << g_createIndex++ );

		eData->OnCreate();
		eData->setName( name.c_str() );
		eData->setGroup( ROOT_GROUP );
		getWorldEditor()->addEditData( eData );
		addTreeItem( eData , m_GroupTreeCtrl->GetRootItem() );

		setCurEditData( eData );
	}

}


void WorldEditorFrame::OnTreeMenu( wxTreeEvent& event )
{
	wxArrayTreeItemIds arID;
	m_GroupTreeCtrl->GetSelections(arID);


	wxPoint clientpt = event.GetPoint();
	wxMenu menu;

	EmDataBase* eData = getEditData( arID[0] );

	class MenuVisitor : public EmVisitor
	{
#define DEFULT_VISIT( type )\
		void visit( type* eData ){}

#define VISIT( type )\
		void visit( type* eData )

#define OBJ_VISIT( type )\
		void visit( type* eData )\
		{   menu->Append( COM_GO_TO_OBJECT , wxT("Go to Object") ); }

		OBJ_VISIT( EmSceneObj     )
		OBJ_VISIT( EmPlayerBoxTGR )
		OBJ_VISIT( EmActor )
		OBJ_VISIT( EmPhyObj )
		DEFULT_VISIT( EmChangeLevel  )
		DEFULT_VISIT( EmSpawnZone    )
		VISIT( EmLogicGroup )
		{   menu->Append( COM_EDIT_LOGIC_GROUP , wxT("Edit Logic..") );  }
		VISIT( EmGroup )
		{   menu->Append( COM_SET_TERRAIN , wxT("Set Terrain") );  }

#undef  OBJ_VISIT
#undef  DEFULT_VISIT
#undef  VISIT

	public:
		wxMenu*  menu;
	} visitor;

	visitor.menu = &menu;

	if ( arID.size() == 1)
	{
		m_curMenuItemID = arID[0];
		menu.Append( COM_CREATE_GROUP , wxT("&Create Group"));
		eData->accept( visitor );
		menu.Append( COM_DELETE_EDIT_DATA , wxT("Delete Data") );
	}
	menu.AppendSeparator();
	menu.Append( COM_CREATE_PLAYER_BOX_TGR  , wxT("Player Box Trigger"));
	menu.Append( COM_CREATE_CHANGE_LEVEL    , wxT("ChangeLevel"));
	menu.Append( COM_CREATE_LOGIC_GROUP     , wxT("Logic Group") );
	menu.Append( COM_CREATE_SPAWN_ZONE      , wxT("Spawn Zone") );
	
	m_GroupTreeCtrl->PopupMenu(&menu, clientpt );
	event.Skip();
}

EmDataBase* WorldEditorFrame::getEditData( wxTreeItemId id )
{
	if ( !id.IsOk() )
		return NULL;

	EditTreeItemData* data = (EditTreeItemData*) 
		m_GroupTreeCtrl->GetItemData( id );

	return data->editData;
}

void WorldEditorFrame::OnTreeBeginDrag( wxTreeEvent& event )
{
	if ( event.GetItem() != m_GroupTreeCtrl->GetRootItem() )
	{
		event.Allow();
		m_GroupTreeCtrl->GetSelections(m_dragItemID);
	}
}

void WorldEditorFrame::OnTreeEndDrag( wxTreeEvent& event )
{
	wxTreeItemId itemDst = event.GetItem();

	EmDataBase* dstData = getEditData( itemDst );
	if ( !itemDst.IsOk() )
		return;

	if ( EmGroup* groupData = dynamic_cast< EmGroup*>( dstData ) )
	{
		unsigned gID = groupData->groupID;

		for ( int i = 0 ; i < m_dragItemID.size() ; ++i )
		{
			wxTreeItemId itemSrc = m_dragItemID[i];

			EditTreeItemData* itemData = (EditTreeItemData*)
				m_GroupTreeCtrl->GetItemData( itemSrc );
			EmDataBase* editData = itemData->editData ;

			m_GroupTreeCtrl->Delete( itemSrc );

			editData->setGroup( gID );
			addTreeItem( editData , itemDst );
		}
	}
	else if ( EmLogicGroup* logicGroup = dynamic_cast< EmLogicGroup* >( dstData ) )
	{
		for ( int i = 0 ; i < m_dragItemID.size() ; ++i )
		{
			wxTreeItemId itemSrc = m_dragItemID[i];

			EmDataBase* eData = getEditData( itemSrc );

			logicGroup->addEditData( eData );

			//editData->setGroup( gID );
			//wxTreeItemId id = m_GroupTreeCtrl->AppendItem( itemDst , editData->getName() , 
			//	editData->editType , editData->editType , itemData );

		}

	}

	m_GroupTreeCtrl->Refresh( m_GroupTreeCtrl->GetRootItem());
}

void WorldEditorFrame::createTreeIcon()
{
	int size = 16;

	// Make an image list containing small icons
	wxImageList *images = new wxImageList(size, size, true);

	// should correspond to TreeCtrlIcon_xxx enum
	wxBusyCursor wait;
	wxIcon icons[4];
	icons[0] = wxIcon(icon3_xpm);
	icons[1] = wxIcon(icon5_xpm);
	icons[2] = wxIcon(icon1_xpm);
	icons[3] = wxIcon(icon2_xpm);


	int sizeOrig = icons[0].GetWidth();
	for ( size_t i = 0; i < WXSIZEOF(icons); i++ )
	{
		if ( size == sizeOrig )
		{
			images->Add(icons[i]);
		}
		else
		{
			images->Add(wxBitmap(wxBitmap(icons[i]).ConvertToImage().Rescale(size, size)));
		}
	}
	m_GroupTreeCtrl->AssignImageList(images);
}

void WorldEditorFrame::setCurEditData( EmDataBase* eData )
{
	propLoader.removeAll();

	class PropVisitor : public EmVisitor
	{
#define DEFULT_VISIT( type )\
		void visit( type* eData ){ loader->load( eData ); }

		DEFULT_VISIT( EmGroup        )
		DEFULT_VISIT( EmSceneObj     )
		DEFULT_VISIT( EmPlayerBoxTGR )
		DEFULT_VISIT( EmChangeLevel  )
		DEFULT_VISIT( EmLogicGroup   )
		DEFULT_VISIT( EmPhyObj       )
		DEFULT_VISIT( EmSpawnZone    )
	

		void  visit( EmActor* eData )
		{
			loader->load( eData );
			wxPGId id = wxProp->ReplaceProperty("Prop" , wxLongStringProperty("Prop") );
			loader->changePropID( "Prop" , id );
		}

#undef  DEFULT_VISIT

	public:
		wxPropertyGrid* wxProp;
		PropLoader*     loader;
	} visitor;

	visitor.loader = &propLoader;
	visitor.wxProp = m_ObjPropGrid;

	m_curEditData = eData;

	eData->accept( visitor );
	eData->OnSelect();

	updatePropGrid();
}

void WorldEditorFrame::OnTreeLabelEditEnd( wxTreeEvent& event )
{
	EmDataBase* eData = getEditData( event.GetItem() );
	eData->setName( event.GetLabel().c_str() );
	propLoader.output( "Name" );
	m_ObjPropGrid->Refresh( m_ObjPropGrid->GetPropertyByLabel("Name") );
}

void WorldEditorFrame::OnTreeLabelEditBegin( wxTreeEvent& event )
{
	EmDataBase* eData = getEditData( event.GetItem()  );

	if ( dynamic_cast< EmSceneObj* >( eData ) )
		event.Veto();

	event.Skip();
}

void SignalConDlg::refreshLogicObj()
{
	std::vector< EmDataBase* >& eDataVec = m_logicGroup->eDataVec;

	m_SenderChoice->Clear();
	m_HolderChoice->Clear();

	for ( int i = 0 ; i < eDataVec.size() ; ++i )
	{
		EmDataBase* eData = eDataVec[i];

		if ( eData == NULL )
		{
			m_SenderChoice->Append( " " , eData );
			m_HolderChoice->Append( " " , eData );
		}
		else
		{
			TRefObj* obj = eData->getLogicObj();
			assert( obj );

			m_SenderChoice->Append( eData->getName() , eData );
			m_HolderChoice->Append( eData->getName() , eData );
		}
	}
}

void SignalConDlg::refreshSignal( EmDataBase* data )
{
	m_SignalChoice->Clear();

	if ( !data )
		return; 

	TRefObj* obj = data->getLogicObj();
	if ( !obj )
		return;

	char const* name[64];

	int num = obj->getSignalList( name , ARRAY_SIZE(name) );

	for( int i = 0 ; i < num ; ++i )
	{
		m_SignalChoice->Append( name[i] );
	}
}

void SignalConDlg::refreshSlot( EmDataBase* data )
{
	m_SlotChoice->Clear();

	if ( !data )
		return; 

	TRefObj* obj = data->getLogicObj();
	if ( !obj )
		return;

	char const* name[64];

	int num = obj->getSlotList( name , ARRAY_SIZE(name) );

	for( int i = 0 ; i < num ; ++i )
	{
		m_SlotChoice->Append( name[i] );
	}
}

SignalConDlg::SignalConDlg( EmLogicGroup* logicGroup ,wxWindow* parent, wxWindowID id /*= wxID_ANY */ ) 
	:SignalConDlgUI( parent, id  )
	,m_logicGroup( logicGroup )
	,m_selectCon( NULL )
{

	m_ConListCtrl->InsertColumn( 0 , wxT("") );
	m_ConListCtrl->InsertColumn( 1 , wxT("sender") );
	m_ConListCtrl->InsertColumn( 2 , wxT("signal") );
	m_ConListCtrl->InsertColumn( 3 , wxT("holder") );
	m_ConListCtrl->InsertColumn( 4 , wxT("slot") );

	m_ConListCtrl->SetColumnWidth( 0 , 15 );
	m_ConListCtrl->SetColumnWidth( 1 , 100 );
	m_ConListCtrl->SetColumnWidth( 2 , 100 );
	m_ConListCtrl->SetColumnWidth( 3 , 100 );
	m_ConListCtrl->SetColumnWidth( 4 , 100 );

	refreshLogicObj();
	refreshConnect();
}

void SignalConDlg::OnChoice( wxCommandEvent& event )
{
	EmDataBase* eData = ( EmDataBase* ) event.GetClientData();
	switch ( event.GetId() )
	{
	case ID_SenderChoice:
		refreshSignal(eData);break;
	case ID_HolderChoice:
		refreshSlot(eData);break;
	}
}

void SignalConDlg::refreshConnect()
{
	typedef EmLogicGroup::ConInfoList ConInfoList;

	m_ConListCtrl->DeleteAllItems();

	int index = 0;
	for( ConInfoList::iterator iter = m_logicGroup->conList.begin();
		 iter != m_logicGroup->conList.end() ; ++iter )
	{
		EmLogicGroup::ConnectInfo& info = *iter;

		EmDataBase* sData = m_logicGroup->eDataVec[ info.sID ];
		EmDataBase* hData = m_logicGroup->eDataVec[ info.hID ];

		wxListItem item;
		item.m_col = 0;
		item.m_itemId = index++;
		long itemid = m_ConListCtrl->InsertItem(item);

		m_ConListCtrl->SetItem( itemid , 0 , wxString() << index );
		m_ConListCtrl->SetItem( itemid , 1 , sData->getName() );
		m_ConListCtrl->SetItem( itemid , 2 , info.signalName.c_str() );
		m_ConListCtrl->SetItem( itemid , 3 , hData->getName() );
		m_ConListCtrl->SetItem( itemid , 4 , info.slotName.c_str() );
		m_ConListCtrl->SetItemData( itemid , (long) &info );
	}

}

void SignalConDlg::OnAddClick( wxCommandEvent& event )
{
	m_logicGroup->addConnect( 
		m_SenderChoice->GetSelection() ,
		m_SignalChoice->GetStringSelection().c_str() ,
		m_HolderChoice->GetSelection() ,
		m_SlotChoice->GetStringSelection().c_str() );

	refreshConnect();
}

void SignalConDlg::OnListItemSelected( wxListEvent& event )
{
	m_selectCon = ( ConnectInfo* ) event.GetItem().GetData();

	m_MofidyButton->Enable( true );
	m_RemoveButton->Enable( true );
}

void SignalConDlg::OnListItemDeselected( wxListEvent& event )
{
	m_selectCon = NULL;

	m_MofidyButton->Enable( false );
	m_RemoveButton->Enable( false );
}

void SignalConDlg::OnRemoveClick( wxCommandEvent& event )
{
	m_logicGroup->removeConnect( m_selectCon );

	refreshConnect();
}

void SignalConDlg::OnModifyClick( wxCommandEvent& event )
{

}

void TerrainToolDlg::OnScroll( wxScrollEvent& event )
{
	switch( event.GetId())
	{
	case ID_StrSlider:
		brush.strength = event.GetPosition();
		m_StrTCtrl->SetValue( wxString() << event.GetPosition() );
		break;
	case ID_RadiusSlider:
		brush.radius = event.GetPosition();
		m_RadiusTCtrl->SetValue( wxString() << event.GetPosition() );
		brushObj.updateRadius( brush );
		break;
	case ID_FallOffSlider:
		brush.falloff = event.GetPosition();
		m_FallOffTCtrl->SetValue( wxString() << event.GetPosition() );
		brushObj.updateRadius( brush );
		break;
	}
	event.Skip();
}


static void appendVal( float& val , wxSlider* slider , wxTextCtrl* textCtrl )
{
	wxString& str = textCtrl->GetValue();

	double value;
	if ( str.ToDouble( &value ) )
	{
		val = value;
		slider->SetValue( val );
	}
	else
	{
		value = val;
		textCtrl->SetValue( wxString() << value );
		slider->SetValue( val );
		slider->Refresh();
	}

}
void TerrainToolDlg::OnTextEnter( wxCommandEvent& event )
{
	double val;
	switch( event.GetId())
	{
	case ID_StrTCtrl:
		appendVal( brush.strength , m_StrSlider , m_StrTCtrl );
		break;
	case ID_RadiusTCtrl:
		appendVal( brush.radius , m_RadiusSlider , m_RadiusTCtrl );
		brushObj.updateRadius( brush );
		break;
	case ID_FallOffTCtrl:
		appendVal( brush.falloff , m_FallOffSlider , m_FallOffTCtrl );
		brushObj.updateRadius( brush );
		break;
	}

	event.Skip();

}

TerrainToolDlg::TerrainToolDlg( THeighField& field , TerBrush& brush , BrushObj& obj , wxWindow* parent, wxWindowID id /*= wxID_ANY */ ) 
	:TerrainToolDlgUI( parent , id )
	,field( field ) 
	,brush( brush )
	,brushObj( obj )
{
	m_StrTCtrl->SetValue( wxString() << m_StrSlider->GetValue() );
	m_RadiusTCtrl->SetValue( wxString() << m_RadiusSlider->GetValue() );
	m_FallOffTCtrl->SetValue( wxString() << m_FallOffSlider->GetValue() );
}

void TerrainToolDlg::OnEffectRadioBox( wxCommandEvent& event )
{
	switch ( event.GetSelection() )
	{
	case 0:
		g_terEffect = &s_HeightEffect;
		break;
	case 1:
		g_terEffect = &s_TextureEffect;
		break;
	case 2:
		g_terEffect = &s_SmoothEffect;
		break;
	}
	event.Skip();
}
