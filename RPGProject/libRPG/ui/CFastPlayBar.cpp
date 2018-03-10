#include "ProjectPCH.h"
#include "CFastPlayBar.h"

#include "CUISystem.h"

#include "CActor.h"

#include "UtilityGlobal.h"
#include "TItemSystem.h"
#include "TSkillBook.h"

Vec2i const CFastPlayBar::Size( CellSize * FPButtonNum , CellSize );

CFastPlayBar::Button::Button( int index , Vec2i const& pos , Vec2i const& size , CFastPlayBar* parent ) 
	:BaseClass( CBT_FAST_PLAY_BAR , pos , size , parent )
	,m_index( index )
{

}


void CFastPlayBar::Button::onUpdateUI()
{
	CPlayButton::onUpdateUI();

	if ( m_playInfo.type == PT_ITEM )
	{
		int num = m_playInfo.caster->getItemStorage().queryItemNum( UG_GetItemID(m_playInfo.name) );

		Color4ub color( 255, 255 , 255 );
		if ( num == 0 )
		{
			color = Color4ub( 128 , 128 , 128 );
		}
		mSprite->setRectColor( 0 , color );
	}
	else if ( m_playInfo.type == PT_SKILL )
	{
		TSkillBook& book = m_playInfo.caster->getSkillBook();
		int castMP = book.getSkillInfo( m_playInfo.name )->castMP;

		Color4ub color( 255, 255 , 255 );
		if ( castMP > m_playInfo.caster->getMP() )
		{
			color = Color4ub( 128 , 128 , 128 );
		}

		mSprite->setRectColor( 0 , color );
		
	}
}

void CFastPlayBar::Button::onInputItem( CPlayButton* button )
{
	if ( button->m_cbType == CBT_FAST_PLAY_BAR ||
		 button->m_cbType == CBT_ITEMBAG_PANEL ||
		 button->m_cbType == CBT_SKILL_PANEL )
	{

		PlayInfo& info = button->m_playInfo;
		setFunction( info.caster , info.name );

		if ( button->m_cbType == CBT_FAST_PLAY_BAR )
			button->setFunction( NULL , NULL );
	}
}

void CFastPlayBar::Button::onMouse( bool beIn )
{
	BaseClass::onMouse( beIn );

	if ( beIn )
	{
		Vec2i pos = getWorldPos();

		CToolTipUI*  tooltip = CUISystem::Get().getHelpTip();
		tooltip->setPos( Vec2i( pos.x, pos.y + getSize().y ) );

	}
}

void CFastPlayBar::Button::onRender()
{
	CFont& font = CUISystem::Get().getDefultFont(); 

	Vec2i pos = getWorldPos();
	setupUITextDepth();

	font.begin();
	font.write( pos.x + getSize().x - font.width , pos.y  , "%d" ,m_index );
	font.end();
}

CFastPlayBar::CFastPlayBar( Vec2i const& pos , CWidget* parent ) 
	:CWidget( pos , Size , parent )
{
	int d1 = ( CellSize - BoardSize )/2;
	int d2 = ( CellSize - ItemSize )/2;

	CUISystem::Get().setTextureDir( "Data/UI" );
	Texture* tex = CUISystem::Get().fetchTexture( "inventory" );

	mSprite->setRenderOption( CFly::CFRO_ALPHA_BLENGING , true );

	for( int i = 0 ; i < FPButtonNum ; ++i )
	{
		int x = CellSize * i;
		int y = 0;

		mSprite->createRectArea( x + d1 , y + d1 , BoardSize , BoardSize , &tex , 1  );

		button[i] = new CFastPlayBar::Button( ( i + 1 ) % 10 ,
			Vec2i( x + d2 , y + d2 ) , Vec2i( ItemSize , ItemSize )  , this );
	}
}

void CFastPlayBar::playButton( int index )
{
	if ( button[index] )
		button[index]->play();
}

void CFastPlayBar::onRender()
{
	BaseClass::onRender();
}

void CFastPlayBar::setButtonFun( int index , CActor* actor , char const* name )
{
	getButton(index)->setFunction( actor , name );
}
