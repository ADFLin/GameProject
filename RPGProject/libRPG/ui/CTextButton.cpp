#include "ProjectPCH.h"
#include "CTextButton.h"

#include "CUISystem.h"

TextButtonTemplateInfo CTextButton::DefultTemplate[ BS_NUM_STATE + 1 ] = 
{
	"textbutton_left_normal",
	"textbutton_mid_normal",
	"textbutton_right_normal",

	"textbutton_left_pressed",
	"textbutton_mid_pressed",
	"textbutton_right_pressed",

	"textbutton_left_highlighted",
	"textbutton_mid_highlighted",
	"textbutton_right_highlighted",

	"textbutton_left_ghosted",
	"textbutton_mid_ghosted",
	"textbutton_right_ghosted",


};

CTextButton::CTextButton( char const* text , int width , Vec2i const& pos , CWidget* parent ) 
	:CButtonUI( pos , Vec2i( width , Hight ) , parent )
	,m_text( text )
{

	Texture* texLeft[4];
	Texture* texMid[4];
	Texture* texRight[4];

	CUISystem::Get().setTextureDir("Data/UI/Textbutton");

	for( int i = 0 ; i < BS_NUM_STATE + 1; ++i )
	{
		texLeft[i] = CUISystem::Get().fetchTexture( DefultTemplate[i].left );
		texMid[i] = CUISystem::Get().fetchTexture( DefultTemplate[i].middle );
		texRight[i] = CUISystem::Get().fetchTexture( DefultTemplate[i].right );
	}

	
	mIdBtnImage[0] = mSprite->createRectArea( 0 , 0 , Hight , Hight , texLeft , 4 );
	mIdBtnImage[1] = mSprite->createRectArea( Hight - 1 , 0 , TMax( 0 , width -  2 * Hight + 2 ) , Hight , texMid , 4 , 0.0f , NULL , CFly::CF_FILTER_POINT , CFly::STM_WARP , CFly::STM_FILL );
	mIdBtnImage[2] = mSprite->createRectArea( width - Hight  , 0 , Hight , Hight , texRight , 4  );

	mSprite->setRenderOption( CFly::CFRO_ALPHA_BLENGING , true );

	for( int i = 0 ; i < 3 ; ++i )
		mSprite->setRectTextureSlot( mIdBtnImage[i] , BS_NORMAL );


}
void CTextButton::onChangeState( ButtonState state )
{
	for( int i = 0 ; i < 3 ; ++i )
		mSprite->setRectTextureSlot( mIdBtnImage[i] , state );
}



void CTextButton::onRender()
{
	BaseClass::onRender();

	Vec3D pos = mSprite->getWorldPosition();

	CFont& font = CUISystem::Get().getDefultFont();

	int offset = ( getSize().x - m_text.length() * 6 )/ 2;

	int x = pos.x + offset;
	int y = pos.y + 6;
	switch( getButtonState() )
	{
	case BS_PRESS:
		font.setColor( Color4ub(  255 , 20 , 10 , 255 ) );
		x -= 1; y += 1;
		break;
	case BS_NORMAL:
		font.setColor( Color4ub( 255 , 255 , 255 , 255 ) );
		break;
	case BS_HIGHLIGHT:
		font.setColor( Color4ub( 255 , 255 , 0 , 255 ) );
		break;
	}

	setupUITextDepth();

	font.begin();
	font.write( x , y , m_text.c_str() );
	font.end();
}

