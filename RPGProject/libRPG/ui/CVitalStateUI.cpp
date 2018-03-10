#include "ProjectPCH.h"
#include "CVitalStateUI.h"

#include "CFWorld.h"

#include "AbilityProp.h"

#include "CBloodBar.h"
#include "UtilityFlyFun.h"
#include "CUISystem.h"

CVitalStateUI::CVitalStateUI( Vec2i const& pos ,Thinkable* thinkable , AbilityProp* comp  ) 
	:CWidget( pos , Vec2i( 273 , 105 ) , NULL )
{
	mPropComp = comp;

	CUISystem::Get().setTextureDir( "Data/UI" );

	mSprite->createRectArea( 0 , 0, 273 , 104 , "player_vitals" , 1 , 0 );
	mSprite->setRenderOption( CFly::CFRO_ALPHA_BLENGING , TRUE );
	mSprite->setRenderOption( CFly::CFRO_CULL_FACE , CFly::CF_CULL_NONE );

	m_HPBar = new CBloodBar( thinkable , mSprite , mPropComp->getPropValue( PROP_MAX_HP ), Vec3D( 98 - 12 , 54  , 0.01 ) , Vec2i( BloodBarLength + 18 , 15 ), Color4f(1,0,0) );
	m_HPBar->setLife( mPropComp->getHP() , false );

	m_MPBar = new CBloodBar( thinkable,  mSprite , mPropComp->getPropValue( PROP_MAX_MP ), Vec3D(97, 43 , 0.01 ) , Vec2i( BloodBarLength  , 10 ), Color4f(0.1,0.1,1) );
	m_MPBar->setLife( mPropComp->getMP() , false );

	shufa = CUISystem::Get().getCFWorld()->createShuFa( "·s²Ó©úÅé" , 12 , true , false );

}

void CVitalStateUI::onUpdateUI()
{
	m_HPBar->setMaxVal( mPropComp->getPropValue( PROP_MAX_HP ) );
	m_MPBar->setMaxVal( mPropComp->getPropValue( PROP_MAX_MP ) );
	m_HPBar->setLife( mPropComp->getHP() , true );
	m_MPBar->setLife( mPropComp->getMP() , true );
}


void CVitalStateUI::onRender()
{
	BaseClass::onRender();
	Vec3D pos = mSprite->getWorldPosition();
	
	int offset;
	char str[64];
	int life = m_HPBar->getLife();

	setupUITextDepth();
	shufa->begin();
	sprintf( str , "%d / %d " , (int)m_HPBar->getLife() , (int)mPropComp->getPropValue( PROP_MAX_HP ) );
	offset = ( BloodBarLength - strlen( str ) * 6 )/ 2;
	shufa->write( str , pos.x + 98 + offset , ( pos.y + 50 + 12 ), Color4ub( 255 ,255 , 255 , 255 ) );
	//sprintf( str , "%d / %d " , (int)m_MPBar->getLife() , (int)mPropComp->getPropValue( PROP_MAX_MP ) );
	//offset = ( BloodBarLength - strlen( str ) * 6 )/ 2;
	//shufa->write( str , pos.x + 98 + offset , ( pos.y + 39 + 12 ), Color4ub( 255 ,255 , 255 , 255 ) );
	shufa->end();

}
