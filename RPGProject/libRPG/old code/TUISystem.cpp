#include "TUISystem.h"

#include "UtilityFlyFun.h"
#include "TEventSystem.h"
#include "EventType.h"
#include "TActor.h"
#include "TActorTemplate.h"
#include "TCombatSystem.h"
#include "TEntityManager.h"
#include "TGame.h"
#include "TGameStage.h"
#include "profile.h"
#include "TEventSystem.h"
#include "UtilityGlobal.h"
#include "TPlayButton.h"

TFont   g_TextFont;

int const TUIBase::MaxLevelDepth = 7000;
int const TUIBase::MaxChildNum   = 100;

TSprite::TSprite( FnSprite& obj ) 
	:v( 0,0,0)
	,pos(0,0,0)
{
	setTemp( true );
	m_obj.Object(obj.Object());
	is3DPos = false;
}

TSprite::~TSprite()
{
	m_obj.RemoveAllGeometry();
	TUISystem::destorySprite( m_obj );
}

void TSprite::setDestoryTime( float time )
{
	int index = addThink( (fnThink) &TSprite::destoryThink );
	setNextThink( index , time );
}

void TSprite::destoryThink()
{
	setEntityState( EF_DESTORY );
}

void TSprite::updateFrame()
{
	pos += v * g_GlobalVal.frameTime;

	if ( is3DPos )
	{
		Vec3D tPos = g_Game->computeScreenPos( pos );
		tPos[1] = g_ScreenHeight - tPos[1]; 

		m_obj.SetPosition( tPos );
	}
	else
		m_obj.SetPosition( pos );
}

void TSprite::changeOpacityThink()
{
	float op = m_obj.GetOpacity();

	op += offsetOpacity;

	if ( ( offsetOpacity > 0 && op > endOpacity ) ||
		 ( offsetOpacity < 0 && op < endOpacity ) )
	{
		op = endOpacity;
	}
	else
	{
		setCurContextNextThink( g_GlobalVal.nextTime );
	}

	m_obj.SetOpacity( op );
}

void TSprite::setOpacityChange( float os , float oe , float dt )
{
	//
	if ( os == 1.0f )
		os = 0.99999999f;

	m_obj.SetOpacity( os );
	offsetOpacity = ( oe - os ) / ( dt / g_GlobalVal.frameTime );
	endOpacity = oe;

	int index = addThink( (fnThink)&TSprite::changeOpacityThink );
	setNextThink( index , g_GlobalVal.nextTime );
}

void TSprite::setPosition( Vec3D const& val , bool be3D )
{
	pos = val;
	is3DPos = be3D;
}

TNumUICreator::TNumUICreator(FnWorld& world)
{
	init( world );
}

TNumUICreator::~TNumUICreator()
{
	UF_DestoryObject( saveMatObj );
	FnWorld world;
	world.Object( bgScene.GetWorld() );
	world.DeleteScene( bgScene.Object() );
}

void TNumUICreator::init( FnWorld& world )
{
	world.SetTexturePath("Data/UI/string");
	bgScene.Object( world.CreateScene(0) );

	char* texName = "num";
	numMatID = world.CreateMaterial( NULL , NULL , NULL , 1.0f, NULL );
	FnMaterial mat; mat.Object( numMatID );

	TEXTUREid tID = mat.AddTexture( 0 , 0 , texName , FALSE );

	saveMatObj =  UF_CreateUIObject(bgScene);
	createNumGeom( &saveMatObj , Vec3D(0,0,0) , 0 ,0 );
}

void TNumUICreator::createNumGeom( FnObject* obj, Vec3D const& pos , int digi , int numColor )
{
	Vec3D color( 1 , 1 , 1 );
	float v[ 8 * 4 ];

	int w = DigiWidth;
	int h = DigiHeight;

	float du = 1.0f / 10;
	float dv = 1.0f / 5;
	float su = digi * du;
	float sv = numColor * dv;

	float* vtx = v;
	vtx = UF_SetXYZ_RGB( vtx , pos                      , color , su      ,  sv  );
	vtx = UF_SetXYZ_RGB( vtx , pos + Vec3D( w , 0 , 0 ) , color , su + du ,  sv  );
	vtx = UF_SetXYZ_RGB( vtx , pos + Vec3D( w , h , 0 ) , color , su + du ,  sv + dv );
	vtx = UF_SetXYZ_RGB( vtx , pos + Vec3D( 0 , h , 0 ) , color , su      ,  sv + dv );

	int texLen = 2;
	WORD tri[6];

	tri[0] = 0; tri[1] = 1; tri[2] = 3;
	tri[3] = 1; tri[4] = 2; tri[5] = 3;

	obj->IndexedTriangle( XYZ_RGB , numMatID , 1, 4, 1 , &texLen , v , 2, tri, FALSE );
}

FnSprite TNumUICreator::create( int val , int color )
{
	FnSprite obj;
	obj.Object( TUISystem::createSprite() );
	int i = 0;
	int temp = val;
	//get digi number
	while( temp /= 10 ){ ++i; }

	do
	{
		int digi = val % 10 ;
		createNumGeom( &obj , Vec3D( ( DigiWidth - 4 )*i , 0 , 0 ) , digi , color );
		--i;
	}
	while( val /= 10 );
	return obj;
}

TUISystem::TUISystem()
	:numCreator(NULL)
	,m_focusUI(NULL)
	,m_MouseUI(NULL)
	,m_root()
{
	setGlobal( true );
	TEntityManager::instance().addEntity( this );
	listenEvent( EVT_COMBAT_DAMAGE , EVENT_ANY_ID , FnMemEvent( &TUISystem::showDamageNum ) );
	listenEvent( EVT_LEVEL_UP , EVENT_ANY_ID , FnMemEvent( &TUISystem::showLevelUp ) );
}


void TUISystem::init( FnWorld& world )
{
	backScene.Object( world.CreateScene(1) );
	frontScene.Object( world.CreateScene(1) );

	backScene.SetSpriteWorldSize( g_ScreenWidth , g_ScreenHeight , 100000 );
	frontScene.SetSpriteWorldSize( g_ScreenWidth , g_ScreenHeight , 100000 );
	numCreator = new TNumUICreator( world );

	g_TextFont.init( TSTR("·s²Ó©úÅé") , 12 , true , false );
	g_TextFont.setColor( 255 , 255 , 255 , 255 );
}

bool TUISystem::OnMessage( MouseMsg& msg )
{
	TPROFILE("UI System");

	m_mouseMsg = msg;
	m_mouseMsg.setPos( TVec2D( msg.x() , g_ScreenHeight - msg.y() ) );

	TUIBase* ui = hitTest( m_mouseMsg.getPos() );

	{
		TPROFILE("Msg Process");

		if (  msg.OnLeftDown() )
		{
			if ( m_focusUI != ui )
			{
				if ( m_focusUI )
					m_focusUI->OnFocus( false );

				if ( ui )
					ui->OnFocus( true );

				m_focusUI = ui;
			}
		}
		else if ( msg.OnLeftUp() )
		{
			TEvent event( EVT_UI_MOUSE_LEFTUP , EVENT_ANY_ID , this , ui );
			UG_SendEvent( event );
		}

		if ( msg.OnMoving() )
		{
			if ( m_MouseUI != ui )
			{
				if ( m_MouseUI )
					m_MouseUI->OnMouse( false );
				
				if ( ui )
					ui->OnMouse( true );

				m_MouseUI = ui;
			}
		}

		if ( msg.OnDraging() )
		{
			TEvent event( EVT_UI_MOUSE_DRAG , EVENT_ANY_ID , this , ui );
			UG_SendEvent( event );

			if ( TPlayButton::s_dragButton )
				return false;
		}

		if ( ui )
		{
			if ( msg.OnDraging() && ui != m_focusUI )
			{
				return false;
			}
			return ui->OnMessage( m_mouseMsg );

			//while ( bool still = ui->OnMessage( m_mouseMsg ) )
			//{
			//	ui = ui->getParent();
			//	if ( !still || ui == getRoot() )
			//		return still;
			//}
		}
	}

	return true;
}

TUIBase* TUISystem::hitTest( TVec2D const& testPos )
{
	TPROFILE("UI Hit Test");
	TUIBase* ui = m_root.m_child;
	while ( ui )
	{
		if ( ui->isEnable() && ui->isShow() &&
			  testPointInRect( testPos , ui->m_rect ) )
			return ui->hitTestChildren( testPos - ui->getPos() );

		ui = ui->m_next;
	}
	return NULL;
}

void TUISystem::updateUI( TUIBase* ui )
{
	while ( ui )
	{
		if ( ui->isEnable() )
			ui->OnUpdateUI();
		
		updateUI( ui->getChildren() );
		ui = ui->m_next;
	}
}

void TUISystem::updateUI()
{
	TPROFILE("UI System");

	{
		TPROFILE("UpdateUI");
		updateUI( m_root.m_child );
	}
}


void TUISystem::render( FnViewport& viewport )
{
	TPROFILE("UI Render");

	viewport.Render2D( backScene.Object() , FALSE , TRUE );

	for ( int i = numRootChlid - 1 ; i >= 0 ; --i )
		viewport.Render2D( sceneID[i] , FALSE , TRUE );

	viewport.Render2D( frontScene.Object() , FALSE , TRUE );

}

void TUISystem::render( TUIBase* ui )
{
	while ( ui )
	{
		if ( ui->isShow() )
			ui->OnRender();

		render( ui->getChildren() );
		ui = ui->m_next;
	}
}


void TUISystem::setTextureDir( char const* path )
{
	g_Game->getWorld().SetTexturePath( (char*) path );
}

void TUISystem::showDamageNum()
{
	TEventSystem& evtSys = TEventSystem::instance();
	assert( evtSys.isProcessInFun( this , (FnMemEvent)&TUISystem::showDamageNum ) );

	TEvent event = evtSys.getCurEvent();

	DamageInfo* info = (DamageInfo*) event.data;

	if ( TActor* actor = TActor::upCast( info->defender )  )
	{
		if ( actor->getStateBit() & AS_DIE )
			return;

		int color = 0;
		switch( actor->getTeam() )
		{
		case TEAM_MONSTER:  color = 0; break;
		case TEAM_PLAYER:   color = 2; break;
		case TEAM_VILLAGER: color = 1; break;
		}
		
		FnSprite obj = numCreator->create( info->damageResult , color );
		obj.SetRenderOption( ALPHA , TRUE );

		TSprite* sprite = new TSprite( obj );
		sprite->use3DPos( true );
		sprite->setVelocity( Vec3D( 0, 0 ,30 ) );
		sprite->setPosition(actor->getPosition()  , true );
		sprite->setDestoryTime( g_GlobalVal.curtime + 2.0 );
		sprite->setOpacityChange( 1.0f , 0 , 2 );
		sprite->updateFrame();


		obj.Show( TRUE );
		TEntityManager::instance().addEntity( sprite );
	}
}

TSprite* TUISystem::createScreenMask( Vec3D const& color )
{
	FnSprite obj; obj.Object( createSprite() );

	UF_CreateSquare3D( &obj , Vec3D( 0 , 0 , 0 ) , g_ScreenWidth , g_ScreenHeight , (float*)&color , FAILED_MATERIAL_ID );
	obj.SetRenderOption( ALPHA , TRUE );
	obj.ChangeScene( frontScene.Object() );

	TSprite* sprite = new TSprite( obj );
	TEntityManager::instance().addEntity( sprite );

	return sprite;

}
void TUISystem::showLevelUp()
{
	TEventSystem& evtSys = TEventSystem::instance();
	assert( evtSys.isProcessInFun( this , (FnMemEvent)&TUISystem::showLevelUp ) );

	TEvent event = evtSys.getCurEvent();

	TActor* actor = (TActor*) event.data;
	if ( actor )
	{
		FnSprite obj; obj.Object( createSprite() );

		setTextureDir( "Data/UI/string" );
		FnMaterial mat = UF_CreateMaterial();
		mat.AddTexture( 0 , 0, "levelup" , FALSE );

		int w = 200;
		int h = 50;

		UF_CreateSquare3D( &obj , Vec3D( -w/2 , -h/2 ,0 ) , w , h , Vec3D(1,1,1) , mat.Object() );
		obj.SetRenderOption( ALPHA , TRUE );

		TSprite* sprite = new TSprite( obj );
		sprite->use3DPos( true );
		sprite->setVelocity( Vec3D( 0, 0 , 20 ) );
		sprite->setPosition( actor->getPosition() + Vec3D(0,0,20)  , true );
		sprite->setDestoryTime( g_GlobalVal.curtime + 5.0 );
		sprite->setOpacityChange( 1.0f , 0 , 3 );
		sprite->updateFrame();

		TEntityManager::instance().addEntity( sprite );
	}

}

void TUISystem::destorySprite( FnSprite& spr )
{
	FnScene scene;
	scene.Object( spr.GetScene() );
	scene.DeleteSprite( spr.Object() );
	spr.Object( FAILED_ID );
}

OBJECTid TUISystem::createSprite()
{
	return instance().backScene.CreateSprite();
}

SCENEid TUISystem::createNewUIScene()
{
	FnWorld world;
	world.Object( backScene.GetWorld() );
	
	SCENEid id = world.CreateScene( 2 );

	FnScene scene;
	scene.Object( id );
	scene.SetSpriteWorldSize( g_ScreenWidth , g_ScreenHeight , 2000 );
	scene.SetCurrentRenderGroup( 0 );

	return id;
}

void TUISystem::sortUIRenderIndex()
{
	TUIBase* root = getRoot();
	TUIBase* ui = NULL;

	numRootChlid = 0;
	while ( ui = root->getChildren(ui) )
	{
		FnSprite spr;
		spr.Object( UPCAST_PTR( TSpriteUI , ui )->getFlyID() );
		sceneID[ numRootChlid++] = spr.GetScene();
	}
}

void TUISystem::destoryUI( TUIBase* ui )
{
	if ( ui == NULL )
		return;

	if ( m_focusUI == ui )
		m_focusUI = NULL;

	if ( m_MouseUI == ui )
		m_MouseUI = NULL;

	TUIBase* child;
	while ( child = ui->getChildren() )
	{
		destoryUI( child );
	}

	ui->getParent()->removeChild( ui );
	delete ui;
}

void TUISystem::destoryAllUI()
{
	TUIBase* child;
	while ( child = getRoot()->getChildren() )
	{
		destoryUI( child );
	}
}

void TUISystem::changetScreenSize( int x , int y )
{
	frontScene.SetSpriteWorldSize( x , y , 10000 );
	backScene.SetSpriteWorldSize( x , y , 10000 );
	for( int i = 0 ; i < numRootChlid ; ++i )
	{
		FnScene scene;
		scene.Object( sceneID[i] );
		scene.SetSpriteWorldSize( x , y , 10000 );
	}
}

TUIBase::TUIBase( TVec2D const& pos , TVec2D const& size , TUIBase* parent ) 
	:m_parent(NULL)
	,m_rect( pos , size )
	,m_next(NULL)
	,m_child(NULL)
	,m_enable( true )
	,m_show( true )
	,m_numChild(0)
{
	if ( parent )
	{
		if ( parent != UI_NO_PARENT )
			parent->addChild( this );
	}
	else
	{
		TUISystem::instance().addUI( this );
	}
}


TUIBase::TUIBase()
	:m_parent(NULL)
	,m_next(NULL)
	,m_child(NULL)
	,m_rect( TVec2D(0,0) , TVec2D(0,0))
	,m_enable( true )
	,m_show( true )
	,m_numChild(0)
{

}


TUIBase* TUIBase::hitTestChildren( TVec2D const& testPos )
{
	TUIBase* testUI = m_child;
	while( testUI )
	{
		if ( testPointInRect( testPos , testUI->m_rect ) )
		{
			TVec2D tPos = testPos - m_rect.min; 
			return testUI->hitTestChildren( tPos );
		}
		testUI = testUI->m_next; 
	}
	return this;
}

TUIBase* TUIBase::getChildren( TUIBase* startUi /*= NULL */ )
{
	if ( startUi )
		return startUi->m_next;
	return m_child;
}

void TUIBase::removeChild( TUIBase* ui )
{
	ui->m_parent = NULL;

	if ( m_child == ui  )
	{
		m_child = ui->m_next;
		--m_numChild;
	}
	else
	{
		TUIBase* temp = m_child;
		while( temp )
		{
			if ( temp->m_next == ui )
			{
				temp->m_next = ui->m_next;
				--m_numChild;
				break;
			}
		}
	}
}

TUIBase::~TUIBase()
{
	if ( m_parent)
		m_parent->removeChild( this );
}

bool TUIBase::isFocus()
{
	return TUISystem::instance().getFocusUI() == this;
}

void TUIBase::show( bool beS )
{
	if ( isShow() == beS )
		return;

	OnShow( beS );

	m_show = beS;

	TUIBase* ui = m_child;
	while ( ui )
	{
		ui->show( beS );
		ui = ui->m_next;
	}
}

void TUIBase::OnShow( bool beS )
{

}

int TUIBase::getLevel()
{
	TUIBase* ui = m_parent;
	int result = 0;
	while ( ui ){ ++result; ui = ui->m_parent; }
	return result;
}

int TUIBase::getLayer()
{
	assert( m_parent );

	TUIBase* ui = m_parent->m_child;
	int result = 1;
	while ( ui )
	{
		if ( ui == this )
			return result;

		++result;
		ui = ui->m_next;
	}

	assert( "Not In Parent Node");
	return result;
}

float TUIBase::computeDepth()
{
	int level = getLevel();
	float depth = MaxLevelDepth;
	do{ depth /= MaxChildNum; }while ( --level );

	assert( m_parent );

	float result = depth * ( m_parent->getChildrenNum() - getLayer() + 1 );
	
	return TMax( result , 3.0f ) ; 
}

void TUIBase::addChild( TUIBase* ui )
{
	if ( ui->m_parent )
		ui->m_parent->removeChild( ui );

	assert( ui->m_parent == NULL );

	ui->m_next = this->m_child;
	ui->m_parent = this;
	this->m_child = ui;
	++m_numChild;
}

TVec2D TUIBase::getWorldPos()
{
	TVec2D pos = getPos();
	TUIBase* ui = m_parent;
	while ( ui != TUISystem::instance().getRoot() )
	{
		pos += ui->getPos();
		ui = ui->m_parent;
	}

	return pos;
}

void TUIBase::setPos( TVec2D const& pos )
{
	TVec2D offset = pos - m_rect.min;

	m_rect.min = pos;
	m_rect.max += offset;

	OnChangePos( pos , true );
}

TUIBase* TUIBase::getPrev()
{
	TUIBase* ui = m_parent->m_child;

	if ( ui == this )
		return NULL;

	while ( ui )
	{
		if ( ui->m_next == this )
			return ui;
		ui = ui->m_next;
	}

	TASSERT( 0 , "Error Link UI" );
	return NULL;
}






void TFont::init( char const* fontClass , int size , bool beB , bool beI )
{
	shufa.Object( UF_GetWorld().CreateShuFa( (char*)fontClass, size , beB , beI ) );
	width  = size / 2;
	height = size ;
}

void TFont::start()
{
	shufa.Begin( UF_GetScreenViewport().Object() );
}

int TFont::write( int x , int y , char const* format , ... )
{
	va_list argptr;
	va_start( argptr, format );
	vsprintf( str , format , argptr );
	va_end( argptr );

	return shufa.Write( str , x , g_ScreenHeight - ( y + height ) , cr , cg , cb , ca  );
}

int TFont::write( int x , int y , int maxPixel , char const* format , ... )
{
	va_list argptr;
	va_start( argptr, format );
	vsprintf( str , format , argptr );
	va_end( argptr );

	return shufa.Write( str , x , g_ScreenHeight - ( y + height ) , cr , cg , cb , ca , TRUE , maxPixel );
}