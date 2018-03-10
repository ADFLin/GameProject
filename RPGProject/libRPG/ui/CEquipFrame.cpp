#include "ProjectPCH.h"
#include "CEquipFrame.h"

#include "CItemBagPanel.h"
#include "CActor.h"
#include "CUISystem.h"


#include "CFCamera.h"
#include "CFViewport.h"
#include "CFWorld.h"
#include "CFScene.h"
#include "CFMaterial.h"

#include "UtilityFlyFun.h"

Vec2i const CEquipFrame::Size( 250 , 400 );

CEquipFrame::Button::Button( EquipSlot slot ,Vec2i const& pos , Vec2i const& size , CWidget* parent ) 
	:CPlayButton( CBT_EQUIP_PANEL , pos , size , parent )
	,slot( slot )
{

}
void CEquipFrame::Button::onUpdateUI()
{
	TEquipTable& table = getActor()->getEquipTable();
	TItemBase* item = table.getEquipment( slot );
	if ( item )
	{
		setFunction( getActor() , item->getName() );
	}
	else
	{
		setFunction( getActor() , NULL );
	}
}

void CEquipFrame::Button::onInputItem( CPlayButton* button )
{
	if ( button->m_cbType == CBT_ITEMBAG_PANEL &&
		 button->m_playInfo.type == PT_EQUIP )
	{

		unsigned slotPos = static_cast< CItemBagFrame::Button* >( button )->getSlotPos();
		getActor()->playItemBySlotPos( slotPos );
	}
}

CActor* CEquipFrame::Button::getActor()
{
	return static_cast< CEquipFrame* >( getParent())->getActor();
}

void CEquipFrame::Button::onClick()
{
	getActor()->removeEquipment( slot );
}

CEquipFrame::CEquipFrame( CActor* actor , Vec2i const& pos ) 
	:BaseClass( pos , Size , nullptr )
{
	m_actor = actor;
	m_equipTable = &actor->getEquipTable();

	int d1 = ( CellSize - BoardSize )/2;
	int d2 = ( CellSize - ItemSize )/2;

	//CModelShowPanel* panel = new CModelShowPanel( 
	//	actor->getModel() ,
	//	Vec2i(0,0) , Vec2i( 200 , 200 ) , this );

	int const offset = 50;

	int const textHeight = 100;
	int const equipHight = 150;

	int const offset2 = 30;


	static Vec2i equipButtonPos[EQS_EQUIP_TABLE_SLOT_NUM] =
	{
		/*EQS_HEAD*/  
		Vec2i( ( Size.x - CellSize )/2 ,  equipHight + 4 * offset ) ,
		//EQS_NECK
		Vec2i( Size.x/2 + offset2 , equipHight + 3 * offset ) ,
		//EQS_CHEST
		Vec2i( ( Size.x - CellSize )/2 ,   equipHight + 3 * offset ) ,
		//EQS_WAIST
		Vec2i( ( Size.x - CellSize )/2 ,   equipHight + 2 * offset ) ,
		//EQS_LEG
		Vec2i( ( Size.x - CellSize )/2 ,   equipHight + 1 * offset ) ,
		//EQS_FOOT
		Vec2i( ( Size.x - CellSize )/2 ,   equipHight + 0 * offset ) ,
		//EQS_SHOULDER
		Vec2i( Size.x/2 - CellSize - offset2 , equipHight + 3 * offset  ) ,
		//EQS_WRIST
		Vec2i( Size.x/2 - CellSize - offset2 , equipHight + 2 * offset ) ,
		//EQS_FINGER
		Vec2i( Size.x/2 + offset2 ,  equipHight + 2 * offset ) ,
		//EQS_HAND_L
		Vec2i( Size.x - 10 - CellSize , equipHight + 3 * offset ) ,
		//EQS_TRINKET
		Vec2i( 30 , equipHight + 4 * offset ) ,
		//EQS_HAND_R
		Vec2i( 10 , equipHight + 3 * offset ) ,
		
	};

	CUISystem::Get().setTextureDir( "Data/UI" );
	Texture* tex = CUISystem::Get().fetchTexture( "slot_traits" );

	mSlotBKSpr->setRenderOption( CFly::CFRO_ALPHA_BLENGING , true );

	for ( int i = 0 ; i < EQS_EQUIP_TABLE_SLOT_NUM ; ++i)
	{
		int x = equipButtonPos[i].x;
		int y = Size.y - equipButtonPos[i].y;

		mSlotBKSpr->createRectArea( x , y , BoardSize , BoardSize , &tex , 1  );
		m_equipButton[i] = new Button( EquipSlot( i ) ,
			Vec2i( x + d2 , y + d2 ) , Vec2i( ItemSize , ItemSize ) , this );

		TEquipment* item = m_equipTable->getEquipment( EquipSlot( i ) );
		if ( item )
		{
			m_equipButton[i]->setFunction( m_actor , item->getName() );
		}
	}
}

void CEquipFrame::onRender()
{
	BaseClass::onRender();
	drawText();
}

void CEquipFrame::drawText()
{
	static int lineOffset = 20;
	Vec2i pos = getWorldPos();

	CFont& font = CUISystem::Get().getDefultFont();

	setupUITextDepth();

	font.begin();

	font.setColor( Color4ub(255,255,255) );

	int x = pos.x + 35;
	int y = pos.y + 370 ;


	//CFly::ShuFa::setDepth( getDepth() - 0.1f  );

	font.write( x , y - 0 * lineOffset ,
		"主手攻擊力 : %d" , (int)getActor()->getPropValue( PROP_MAT ) );

	font.write( x , y - 1 * lineOffset ,
		"副手攻擊力 : %d" , (int)getActor()->getPropValue( PROP_SAT ) );

	font.write( x , y  - 2 * lineOffset ,
		"總裝甲值    : %d" , (int)getActor()->getPropValue( PROP_DT ) );

	x += 100;

	font.write( x , y -  0 * lineOffset ,
		"力量 : %d" , (int)getActor()->getPropValue( PROP_STR ) );

	font.write( x , y -  1 * lineOffset ,
		"智慧 : %d" , (int)getActor()->getPropValue( PROP_INT ) );

	font.write( x , y  - 2 * lineOffset ,
		"耐力 : %d" , (int)getActor()->getPropValue( PROP_END ) );

	font.write( x , y  - 3 * lineOffset ,
		"敏捷 : %d" , (int)getActor()->getPropValue( PROP_DEX ) );

	font.end();
}

CModelShowPanel::CModelShowPanel( IRenderEntity* comp , Vec2i const& pos , Vec2i const& size , CWidget* parent ) 
	:BaseClass( pos , size , parent )
	,mModel( comp )
{
	CFWorld* world = CUISystem::Get().getCFWorld();
	mViewport = world->createViewport( 0 , 0 , size.x , size.y );
	Material* mat = world->createMaterial();
	mat->addRenderTarget( 0 , 0 , "modelViewTex" , CFly::CF_TEX_FMT_RGB32 , mViewport , false );
	mat->getTextureLayer(0).setFilterMode( CFly::CF_FILTER_POINT );


	UF_CreateSquare3D( mSprite , Vec3D(0,0,0) , size.x , size.y , Vec3D(1,1,1) , mat );

	mSprite->setRenderOption( CFly::CFRO_CULL_FACE , CFly::CF_CULL_NONE );

	CFScene* scene = comp->getRenderNode()->getScene();
	mCamera = scene->createCamera();
	mCamera->setLookAt( Vec3D(0,-100,50) , Vec3D(0,0,50) , Vec3D(0,0,1) );
}

CModelShowPanel::~CModelShowPanel()
{
	if ( mViewport )
		mViewport->release();
	if ( mCamera )
		mCamera->release();
}


void CModelShowPanel::onRender()
{
	CFScene* scene = mCamera->getScene();
	scene->renderNode( mModel->getRenderNode() , Mat4x4::Identity() ,
		mCamera , mViewport , CFly::CFRF_DEFULT | CFly::CFRF_SAVE_RENDER_TARGET );
}

bool CModelShowPanel::onMouseMsg( MouseMsg const& msg )
{
	return getParent()->onMouseMsg( msg );
}
