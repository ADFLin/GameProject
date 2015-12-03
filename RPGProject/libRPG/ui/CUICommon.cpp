#include "ProjectPCH.h"
#include "CUICommon.h"

#include "CUISystem.h"

#include "EventType.h"
#include "UtilityGlobal.h"


#define PANEL_LB "PANEL_LB"
#define PANEL_LT "PANEL_LT"
#define PANEL_RB "PANEL_RB"
#define PANEL_RT "PANEL_RT"
#define PANEL_L  "PANEL_L"
#define PANEL_R  "PANEL_R"
#define PANEL_T  "PANEL_T"
#define PANEL_B  "PANEL_B"
#define PANEL_C  "PANEL_C"

float const CWidget::MaxLevelDepth = 10000;

void refreshChildrenDepth( CUI::Core* parent , float delta )
{
	float depth = 1.0f;
	CWidget* ui = parent->getChild();
	while( ui )
	{
		ui->setLocalDepth( -depth );
		depth += delta;
		ui = parent->nextChild( ui );
	}
}


class UITextRender : public CFly::RenderListener
{
public:
	virtual void postRender( SceneNode* obj )
	{
		CWidget* widget = reinterpret_cast< CWidget* >( obj->getUserData() );
		widget->onRender();
	}
};

static UITextRender gUITextRender;

CWidget::CWidget( Vec2i const& pos , Vec2i const& size , CWidget* parent ) 
	:TUICore< CWidget >( pos , size , nullptr )
{
	if ( parent )
	{
		mSprite = CUISystem::getInstance().createSprite( parent->getSprite() );
	}
	else
	{
		mSprite = CUISystem::getInstance().createSprite( nullptr );
	}

	mSprite->setUserData( this );
	//mSprite->setRenderListener( &gUITextRender );

	if ( parent )
		parent->addChild( this );
}

CWidget::~CWidget()
{
	mSprite->release();
}

void CWidget::onLink()
{
	Vec2i pos = getPos();
	float depth = calcDepthOffset() * getOrder();
	mSprite->setLocalPosition( Vec3D( pos.x , pos.y , 0 ) );
	setLocalDepth( -depth );
	//float depth = 0;


	//CWidget* child = getChild();
	//while( child )
	//{
	//	Vec3D wPos = child->getSprite()->getWorldPosition();
	//	wPos.z;
	//	child->getChild( child );
	//}
}

void CWidget::setOpacity( float val , bool bothChildren /*= true */ )
{
	mSprite->setOpacity( val );
	onSetOpactity( val );

	if ( bothChildren )
	{
		CWidget* ui = getChild();
		while ( ui )
		{
			ui->setOpacity( val , true );
			ui = nextChild( ui );
		}
	}
}

void CWidget::updatePos()
{

}

void CWidget::setupUITextDepth()
{
	float depth = getDepth() - 0.01;
	CFly::ShuFa::setTextDepth( depth );
}

void CWidget::onChangePos( Vec2i const& pos , bool beLocal )
{
	Vec3D sPos = mSprite->getLocalPosition();
	sPos.x = pos.x;
	sPos.y = pos.y;
	mSprite->setLocalPosition( sPos );
}

void CWidget::onShow( bool beS )
{
	mSprite->show( beS );
}

void CWidget::onRender()
{
	//CFont& font = CUISystem::getInstance().getDefultFont();
	//Vec3D wPos = mSprite->getWorldPosition();

	//setupUITextDepth();
	//font.begin();
	//font.setColor( Color4ub( 125 , 255 , 125 ) );
	//font.write( wPos.x , wPos.y ,"%.3f" , wPos.z );
	//font.end();
}


void CWidget::onChangeChildrenOrder()
{
	float delta = calcDepthOffset();
	delta /= (float)MaxChildNum;
	refreshChildrenDepth( this , delta );
}

void CWidget::setLocalDepth( float depth )
{
	Vec3D pos = mSprite->getLocalPosition();
	pos.z = depth;
	mSprite->setLocalPosition( pos );
}


float CWidget::getDepth()
{
	return mSprite->getWorldPosition().z;
}

float CWidget::calcDepthOffset()
{
	int level = getLevel();
	float depth = (float)MaxLevelDepth / MaxChildNum;
	do
	{
		depth /= ( float )MaxChildNum ;
		--level;
	}
	while( level );
	return depth;
}

bool CWidget::onMouseMsg( MouseMsg const& msg )
{
	return true;
}

void CWidget::onChangeOrder()
{
	if ( getParent() == getManager()->getRoot() )
	{
		float delta = calcDepthOffset();
		refreshChildrenDepth( getParent() , delta );
	}
}

CSimpleButton::CSimpleButton( char const* dir , char const** picName , Vec2i const& pos , Vec2i const& size , CWidget* parent ) 
	:CButtonUI( pos , size , parent )
{
	Texture* tex[ BS_NUM_STATE ];

	CUISystem::getInstance().setTextureDir( dir );
	for( int i = 0 ; i < BS_NUM_STATE ; ++ i )
	{
		tex[i] = CUISystem::getInstance().fetchTexture( picName[i] );
	}
	mIdButtonRect = mSprite->createRectArea( 0 , 0 , size.x , size.y , tex , BS_NUM_STATE );
	mSprite->setRenderOption( CFly::CFRO_ALPHA_BLENGING , TRUE );

}

void CSimpleButton::onChangeState( ButtonState state )
{
	mSprite->setRectTextureSlot( mIdButtonRect , state );
}

CCloseButton::CCloseButton( Vec2i const& pos , CWidget* parent ) 
	:CButtonUI( pos , Vec2i( closeSize , closeSize ) , parent )
{
	Texture* tex = CUISystem::getInstance().fetchTexture( "PANEL_X" );
	mSprite->createRectArea( 0 , 0 ,closeSize , closeSize , &tex , 1 );

	mSprite->setRenderOption( CFly::CFRO_ALPHA_BLENGING , TRUE );
}

PanelTemplateInfo CFrameUI::DefultTemplate = 
{
	36 , 36 , 36 ,

	"Data/UI/Panel" ,

	"panel_corner_lower_left" ,
	"panel_side_left" ,
	"panel_corner_upper_left" ,

	"panel_side_bottom" ,
	"panel_center_bg" ,
	"panel_side_top" ,

	"panel_corner_lower_right" ,
	"panel_side_right" ,
	"panel_corner_upper_right" ,
};

CFrameUI::CFrameUI( Vec2i const& pos , Vec2i const& size  , CWidget* parent )
	:CPanelUI( DefultTemplate , pos , size , parent )
{
	m_closeButton = new CCloseButton( Vec2i( size.x - 20 , 20 - CCloseButton::closeSize ) , this  );
}


bool CFrameUI::onMouseMsg( MouseMsg const& msg )
{
	CWidget::onMouseMsg( msg );

	static int x , y;
	if ( msg.onLeftDown() )
	{
		x = msg.x();
		y = msg.y();

		setTop();
		getManager()->captureMouse( this );
	}
	else if ( msg.onLeftUp() )
	{
		getManager()->releaseMouse();
	}
	if ( msg.isLeftDown() && msg.isDraging() )
	{
		Vec2i pos = getPos() +Vec2i( msg.x() - x , msg.y() - y );
		setPos( pos );
		//mSprite->translate( CFly::CFTO_GLOBAL ,Vec3D( msg.x() - x , msg.y() - y , 0 ) );
		//updatePos();
		x = msg.x();
		y = msg.y();
	}
	return false;
}


CPanelUI::CPanelUI( PanelTemplateInfo& info , Vec2i const& pos , Vec2i const& size , CWidget* parent ) 
	:CUI::Panel< CPanelUI >( pos , size , parent )
{
	initTemplate( info );
}

void CPanelUI::initTemplate( PanelTemplateInfo& info )
{
	Vec2i const& size = getSize();
	CUISystem::getInstance().setTextureDir( info.path );

	Texture* tex[1];

#define SET_PANEL_BK( pos  , ox , oy , w , h )\
	tex[0] = CUISystem::getInstance().fetchTexture( info.texName[ pos ] );\
	mIdBKSprRect[ pos ] = mSprite->createRectArea( ox , oy , w , h , tex , 1  );

	SET_PANEL_BK( PANEL_LB_CORNER , 
		0 , size.y - info.cornerLength , 
		info.cornerLength ,
		info.cornerLength );

	SET_PANEL_BK( PANEL_L_SIDE , 
		0 , info.cornerLength ,
		info.sideWidth ,
		TMax( 1 , size.y - 2 * info.cornerLength ) );

	SET_PANEL_BK( PANEL_LT_CORNER , 
		0 , 0 ,
		info.cornerLength ,
		info.cornerLength );

	SET_PANEL_BK( PANEL_B_SIDE , 
		info.cornerLength , size.y - info.cornerLength , 
		TMax( 1 , size.x - 2 * info.cornerLength ) ,
		info.sideWidth );

	SET_PANEL_BK( PANEL_CENTER , 
		info.cornerLength , info.cornerLength ,
		size.x - 2 * info.sideWidth ,
		size.y - 2 * info.sideWidth );

	SET_PANEL_BK( PANEL_T_SIDE , 
		info.cornerLength , 0 ,
		TMax( 1 , size.x - 2 * info.cornerLength ) ,
		info.sideWidth );

	SET_PANEL_BK( PANEL_RB_CORNER  ,
		size.x - info.cornerLength , size.y - info.cornerLength , 
		info.cornerLength ,
		info.cornerLength );

	SET_PANEL_BK( PANEL_R_SIDE , 
		size.x - info.cornerLength , info.cornerLength ,
		info.sideWidth ,
		TMax( 1 , size.y - 2 * info.cornerLength ) );

	SET_PANEL_BK( PANEL_RT_CORNER ,
		size.x - info.cornerLength , 0 ,
		info.cornerLength ,
		info.cornerLength );

#undef SET_PANEL_BK
}


void CPanelUI::onShow( bool beS )
{
	BaseClass::onShow( beS );
}

CToolTipUI::CToolTipUI( Vec2i const& pos , Vec2i const& size , CWidget* parent ) 
	:CPanelUI( DefultTemplate , pos , size, parent )
{
	maxLineFontNum = 0;
	lineNum = 0;
}

PanelTemplateInfo CToolTipUI::DefultTemplate = 
{
	19 , 50 , 3 ,

	"Data/UI/Tooltip" ,
	"basepanel_bottomleft" ,
	"basepanel_midleft" ,
	"basepanel_topleft" ,

	"basepanel_bottommid" ,
	"basepanel_center" ,
	"basepanel_topmid" ,

	"basepanel_bottomright" ,
	"basepanel_midright" ,
	"basepanel_topright" ,
};

void CToolTipUI::drawText()
{
	Vec3D pos = mSprite->getWorldPosition();
	CFont& font = CUISystem::getInstance().getDefultFont();

	font.setColor( Color4ub( 255  , 255 , 0 , 255 ) );
	font.write( 
		pos.x + 15 ,
		pos.y + getSize().y - font.height / 3 - DefultTemplate.sideWidth  ,
		m_text.c_str() );
}


void CToolTipUI::onRender()
{
	drawText();
}

void CToolTipUI::appendString( char const* str )
{
	char const* testStr = str;
	
	while ( *testStr != '\0')
	{
		int num = 0;
		while ( *testStr != '\n' && *testStr != '\0' )
		{
			++testStr;
			++num;
		}
		maxLineFontNum = TMax( maxLineFontNum , num );
	}


	m_text += '\n';
	m_text += str;

	++lineNum;
}

void CToolTipUI::resize( Vec2i const& size )
{
	for( int i = 0 ; i < PANEL_POS_NUM ; ++i )
		mSprite->removeRectArea( mIdBKSprRect[i] );

	initTemplate( DefultTemplate );
}

void CToolTipUI::fitSize()
{
	CFont& font = CUISystem::getInstance().getDefultFont();

	int sx = maxLineFontNum * font.width + 30;
	int sy = lineNum * font.height + 20;

	resize( Vec2i( sx , sy ) );
}


void CToolTipUI::clearString()
{
	maxLineFontNum = 0;
	lineNum = 0;
	m_text.clear();
}

CSlotFrameUI::CSlotFrameUI( Vec2i const& pos , Vec2i const& size , CWidget* parent ) 
	:CFrameUI( pos , size, parent )
{
	mSlotBKSpr = CUISystem::getInstance().createSprite( mSprite );
	mSlotBKSpr->setLocalPosition( Vec3D(0,0,-0.01) );
	mSlotBKSpr->setParent( mSprite );
}

CSlotFrameUI::~CSlotFrameUI()
{
	mSlotBKSpr->release();
}

void CSlotFrameUI::onShow( bool beS )
{
	BassClass::onShow( beS );
	mSlotBKSpr->show( beS );
}




//void  MessgeUI( BYTE r, BYTE g , BYTE b , const char* str , ... )
//{
//
//}




void CButtonUI::onClick()
{
	TEvent event( EVT_UI_BUTTON_CLICK , getID() , this , this );
	UG_SendEvent( event );
}
