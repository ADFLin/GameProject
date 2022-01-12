#include "ProjectPCH.h"
#include "CPlayButton.h"

#include "UtilityGlobal.h"
#include "UtilityFlyFun.h"

#include "TItemSystem.h"
#include "TSkill.h"
#include "TSkillBook.h"

#include "CActor.h"
#include "EventType.h"

#include "CUISystem.h"

#include "CFScene.h"
#include "CFBuffer.h"



Sprite*      CPlayButton::sDragIcon = nullptr;

Vec2i const CPlayButton::Size( 50 , 50 );


CPlayButton::CPlayButton( CBType type , Vec2i const& pos , Vec2i const& size , CWidget* parent ) 
	:CButtonUI( pos , size , parent )
	,m_mask( size , mSprite->getScene() )
{
	m_cbType = type;
	mSprite->setRenderOption( CFly::CFRO_ALPHA_BLENGING , TRUE );

	mIdSprRect = CF_FAIL_ID;

	m_mask.spr->setParent( mSprite );

	if ( !sDragIcon )
	{
		sDragIcon = CUISystem::Get().createSprite();
		sDragIcon->setRenderOption( CFly::CFRO_ALPHA_BLENGING , true );
	}

}

void CPlayButton::setFunction( CActor* actor , char const* name )
{
	m_playInfo.caster = actor;
	PlayType type = getPlayType( name );


	if ( m_playInfo.name != name || m_playInfo.type != type )
	{
		m_playInfo.name   = name;
		m_playInfo.type   = type;

		if ( name == NULL || 
			 m_playInfo.type == PT_ITEM ||
			 m_playInfo.type == PT_SKILL )
		{
			m_mask.setConnectEvent( actor , name );
			checkColdDown();
		}

		mSprite->removeRectArea( mIdSprRect );
		if ( name )
		{
			mIdSprRect = setupPlayIcon( mSprite );
		}
	}
}

MsgReply CPlayButton::onMouseMsg( MouseMsg const& msg )
{
	if ( msg.onLeftDown() )
	{
		if ( m_playInfo.type == PT_NULL )
			return MsgReply::Handled();

		sDragIcon->removeRectArea( 0 );
		setupPlayIcon( sDragIcon );
		sDragIcon->show( true );
		
		Vec3D pos = mSprite->getWorldPosition();
		pos[2] = -8000;
		sDragIcon->setWorldPosition( pos );

		CUISystem::Get().captureMouse( this );
	}
	else if ( msg.onLeftUp() )
	{
		CUISystem::Get().releaseMouse();
		sDragIcon->show( false );

		CWidget* ui = CUISystem::Get().hitTest( msg.getPos() );

		if ( ui != this )
		{
			if ( ui )
			{
				CPlayButton* playbutton = dynamic_cast< CPlayButton* >( ui ) ;
				if ( playbutton )
					playbutton->onInputItem( this );
			}
			else
			{
				if ( m_cbType ==  CBT_FAST_PLAY_BAR )
					setFunction( NULL , NULL );
			}
			// skip OnClick
			return MsgReply::Handled();
		}
	}

	if ( msg.isLeftDown() && msg.isDraging() )
	{
		if ( sDragIcon->isShow() )
		{
			Vec2i const& size = getSize();
			MouseMsg const& msg = CUISystem::Get().getLastMouseMsg();
			sDragIcon->setWorldPosition( Vec3D( msg.x() - size.x/2 , msg.y() - size.y/2 , -8000 ) );
		}
	}

	CButtonUI::onMouseMsg( msg );

	return MsgReply::Handled();
}


CPlayButton::PlayType CPlayButton::getPlayType( char const* name )
{
	if ( !name )
		return PT_NULL;

	if ( UG_QuerySkill( name , 1 ) )
		return PT_SKILL;

	TItemBase* item = UG_GetItem( UG_GetItemID( name ) );

	if ( item )
	{
		if ( item->isEquipment() )
			return PT_EQUIP;
		else 
			return PT_ITEM;
	}
	return PT_NULL;
}

void CPlayButton::onMouse( bool beIn )
{
	TPROFILE("PlayButton");
	if ( beIn )
	{
		if ( m_playInfo.type == PT_NULL )
			return;

		//TToolTipUI&  tooltip = TToolTipUI::getHelpTip();
		//tooltip.clearString();
		//onSetHelpTip( tooltip );
		//tooltip.show( true );
	}
	else
	{
		//TToolTipUI&  tooltip = TToolTipUI::getHelpTip();
		//tooltip.show( false );
	}
}

void CPlayButton::onSetHelpTip( CToolTipUI& helpTip )
{
	Vec3D pos = mSprite->getWorldPosition();

	//helpTip.setOpacity( 0.8 );
	//helpTip.appendString( m_playInfo.name );
	//helpTip.fitSize();
	//helpTip.setPos( TVec2D( pos.x(), pos.y() - helpTip.getSize().y ) );
}

void CPlayButton::checkColdDown()
{
	CActor* actor =  m_playInfo.caster;
	char const* name = m_playInfo.name;

	PlayType type = getPlayType( name );
	if ( type == PT_ITEM )
	{
		float time = actor->getCDManager().getCDTime( 
			m_playInfo.name , CD_ITEM );
		if ( time > 0 )
		{
			TItemBase* item = UG_GetItem( UG_GetItemID( name ) );
			m_mask.setState( time , item->getCDTime() );
		}
	}
	else if ( type == PT_SKILL )
	{
		float time = actor->getCDManager().getCDTime( 
			m_playInfo.name , CD_SKILL );
		if ( time > 0 )
		{
			TSkillBook& book = m_playInfo.caster->getSkillBook();
			float cdTime  = book.getSkillInfo( name )->cdTime;
			m_mask.setState( time , cdTime );
		}
	}
}

int CPlayButton::setupPlayIcon( Sprite* spr )
{
	char const* texName = nullptr;
	switch( m_playInfo.type )
	{
	case PT_ITEM: case  PT_EQUIP:
		{
			TItemBase* item = UG_GetItem( m_playInfo.name );
			if ( !item )
				return CF_FAIL_ID;
			texName = TItemManager::Get().getIconName( item->getIconID() );
			CUISystem::Get().setTextureDir( ITEM_ICON_DIR );
		}
		break;
	case PT_SKILL:
		{
			texName = TSkillLibrary::Get().getIconName( m_playInfo.name );
			CUISystem::Get().setTextureDir( SKILL_ICON_DIR );
		}
	}

	if ( !texName )
		return CF_FAIL_ID;

	Texture* tex[1];
	tex[0] = CUISystem::Get().fetchTexture( texName , nullptr );

	if ( !tex[0] )
		return CF_FAIL_ID;

	Vec2i size = getSize();
	return spr->createRectArea( 0 , 0 , size.x , size.y , tex , 1 , 0.0f , nullptr , CFly::CF_FILTER_ANISOTROPIC );

}

void CPlayButton::onShow( bool beS )
{
	BassClass::onShow( beS );
	m_mask.spr->show( beS );
	m_mask.needShow = beS;
}



void CPlayButton::PlayInfo::doPlay()
{
	if ( !name )
		return;

	switch( type )
	{
	case PT_ITEM:  caster->useItem( name ); break;
	case PT_EQUIP: caster->equip( name );   break;
	case PT_SKILL: caster->castSkill( name ); break;
	}
}

CDMask::CDMask( Vec2i const& size , CFScene* scene )
	:size( size )
	,needShow( true )
{

	spr = scene->createObject( nullptr );
	
	spr->setOpacity( 0.5f );
	//spr->createPlane( NULL , 100 , 100 , Vec3D(1,0,0) );
	spr->setLocalPosition( Vec3D(0,0,10) );
	spr->enableVisibleTest( false );
	spr->setRenderOption( CFly::CFRO_CULL_FACE , CFly::CF_CULL_NONE );

	int geom = createMask( spr , nullptr );

	CFly::MeshBase* shape = spr->getElement( geom )->getMesh();
	mGoemBuf = shape->getVertexElement( CFly::CFV_XYZ , mGoemOffset );
}


void CDMask::restore( float totalTime )
{
	spr->show( needShow );
	restoreMaskVertex();
	float factor = ( 8 / 2) * g_GlobalVal.frameTime / totalTime;
	dx = factor * size.x;
	dy = factor * size.y;
	mIdxProcess = 1;
	mTotalTime = totalTime;
	reTime = 0;
}

void CDMask::update()
{
	if ( mIdxProcess > 8)
		return;

	char* data = reinterpret_cast< char* >( mGoemBuf->lock() );
	if ( !data )
		return;
	data += mGoemOffset;
	float* vi = reinterpret_cast< float* >( data + mIdxProcess * mGoemBuf->getVertexSize() );

	switch ( mIdxProcess )
	{
	case 1: vi[0] += dx; break;
	case 2: vi[1] -= dy; break;
	case 3: vi[1] -= dy; break;
	case 4: vi[0] -= dx; break;
	case 5: vi[0] -= dx; break;
	case 6: vi[1] += dy; break;
	case 7: vi[1] += dy; break;
	case 8: vi[0] += dx; break;
	}

	reTime += g_GlobalVal.frameTime;
	if ( reTime > mTotalTime / 8 * mIdxProcess )
	{
		float* v0 = reinterpret_cast< float* >( data );
		vi[0] = v0[0];
		vi[1] = v0[1];
		++mIdxProcess;
	}

	mGoemBuf->unlock();
}

void CDMask::restoreMaskVertex()
{
	int dx = size.x / 2;
	int dy = size.y / 2;

	CFly::Locker< CFly::VertexBuffer > locker( mGoemBuf );

	char* data = static_cast< char* >( locker.getData() );
	if ( !data )
		return;

	data += mGoemOffset;
	float* dbg = reinterpret_cast< float*>( data );

#define SET_VERTEX( x , y )\
	{\
		float* v = reinterpret_cast< float*>( data );\
		v[0] = x; v[1] = y; \
		data += mGoemBuf->getVertexSize();\
	}
	
	SET_VERTEX( dx , dy );
	SET_VERTEX( dx , size.y );
	SET_VERTEX( size.x , size.y );
	SET_VERTEX( size.x , dy );
	SET_VERTEX( size.x , 0 );
	SET_VERTEX( dx     , 0 );
	SET_VERTEX( 0 , 0 );
	SET_VERTEX( 0 , dy );
	SET_VERTEX( 0 , size.y );
	SET_VERTEX( dx , size.y );

#undef SET_VERTEX
}

int CDMask::createMask( CFObject* obj , Material* mat )
{
	float z = 1;
	Vec3D color( 0.2 ,0.2 ,0.2 );

	//mat.SetOpacity( 0.1 );

	float v[ 10 * 8 ];
	float* vtx = v;
	
	vtx = UF_SetXYZ_RGB( vtx , Vec3D( 0 , 0 , z), color );
	vtx = UF_SetXYZ_RGB( vtx , Vec3D( 0 , 0 , z), color );
	vtx = UF_SetXYZ_RGB( vtx , Vec3D( 0 , 0 , z), color );
	vtx = UF_SetXYZ_RGB( vtx , Vec3D( 0 , 0 , z), color );
	vtx = UF_SetXYZ_RGB( vtx , Vec3D( 0 , 0 , z), color );
	vtx = UF_SetXYZ_RGB( vtx , Vec3D( 0 , 0 , z), color );
	vtx = UF_SetXYZ_RGB( vtx , Vec3D( 0 , 0 , z), color );
	vtx = UF_SetXYZ_RGB( vtx , Vec3D( 0 , 0 , z), color );
	vtx = UF_SetXYZ_RGB( vtx , Vec3D( 0 , 0 , z), color );
	vtx = UF_SetXYZ_RGB( vtx , Vec3D( 0 , 0 , z), color );

	int tri[8 * 3] =
	{
		2,1,0, 3,2,0, 4,3,0, 5,4,0, 
		6,5,0, 7,6,0, 8,7,0, 9,8,0,
	};
	return obj->createIndexedTriangle( mat , CFly::CFVT_XYZ_CF1 , v , 10 , tri , 8  );
}

//  Mask Vertex Index
//       8--9/1--2
//	     | \ | / |
//		 |  \|/  |
//		 7---0---3		
//       |  /|\  |
//       | / | \ |
//       6---5---4

void CDMask::setState( float time , float chCDTime )
{
	//spr.Show( isShow );
	restore( chCDTime );
	reTime = chCDTime  - time;

	mIdxProcess = int(reTime * 8 / chCDTime ) + 1;

	CFly::Locker< CFly::VertexBuffer > locker( mGoemBuf );
	char* data = static_cast< char* >( locker.getData() );
	if ( !data )
		return;

	data += mGoemOffset;


	float* v = reinterpret_cast< float*>( data );
	float* vi;
	char* temp = data;
	for( int i = 1 ; i < mIdxProcess ; ++i  )
	{
		temp += mGoemBuf->getVertexSize();
		vi = reinterpret_cast< float* >( temp );
		vi[0] = v[0];
		vi[1] = v[1];
	}

	temp += mGoemBuf->getVertexSize();
	vi = reinterpret_cast< float* >( data + mIdxProcess * mGoemBuf->getVertexSize() );

	float t1 = reTime - ( mIdxProcess - 1 ) * chCDTime / 8; 

	if ( (mIdxProcess / 2) % 2 == 0 )
	{
		if ( mIdxProcess / 2 == 2 )
			vi[0] -= dx * t1 / g_GlobalVal.frameTime;
		else
			vi[0] += dx * t1 / g_GlobalVal.frameTime;
	}
	else
	{
		if ( mIdxProcess / 2 == 1 )
			vi[1] -= dy * t1 / g_GlobalVal.frameTime;
		else
			vi[1] += dy * t1 / g_GlobalVal.frameTime;
	}
}


void CDMask::setConnectEvent( CActor* actor , char const* name )
{
	if ( mName != name )
	{
		spr->show( false );
	}
	mName = name;


	//TEventSystem::instance().disconnectEvent( this , EVT_PLAY_CD_START );
	if ( name )
	{
		UG_ConnectEvent( EVT_PLAY_CD_START ,  actor->getRefID() , EventCallBack( this , &CDMask::startCD )  );
	}
}

void CDMask::startCD( TEvent const& event )
{
	CDInfo* info = reinterpret_cast< CDInfo* >( event.data );

	if ( info->name == mName )
	{
		restore( info->cdTime );
	}
}

CDMask::~CDMask()
{
	spr->release();
}
