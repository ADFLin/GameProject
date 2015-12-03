#include "ProjectPCH.h"
#include "CSkillFrame.h"

#include "CUISystem.h"

#include "CActor.h"

#include "TSkillbook.h"
#include "TItemSystem.h"



Vec2i const CSkillFrame::Size( 250 , 400 );

CSkillButton::CSkillButton(  Vec2i const& pos , Vec2i const& size , CSkillFrame* parent ) 
	:CPlayButton( CBT_SKILL_PANEL , pos , size , parent )
{

}


CActor* CSkillButton::getActor()
{
	return static_cast< CSkillFrame* >( getParent() )->getActor();
}

CSkillFrame::CSkillFrame( CActor* actor , Vec2i const& pos )
	:CSlotFrameUI( pos , Size , nullptr )
{
	m_actor = actor;
	m_book  = &actor->getSkillBook();

	char const* skillName[128];
	int numSkill = m_book->getAllSkillName( skillName , ARRAY_SIZE( skillName ) );

	int d1 = ( CellSize - BoardSize )/2;
	int d2 = ( CellSize - ItemSize )/2;

	CUISystem::getInstance().setTextureDir( "Data/UI" );
	Texture* tex = CUISystem::getInstance().fetchTexture( "inventory" );

	mSlotBKSpr->setRenderOption( CFly::CFRO_ALPHA_BLENGING , true );
	
	for (int i = 0 ; i < numSkill ; ++i )
	{
		int x = 10;
		int y = getSize().y - CellSize - 30 - CellSize *  i;

		mSlotBKSpr->createRectArea( x + d1 , y + d1 , BoardSize , BoardSize , &tex , 1  );

		CSkillButton* button = new CSkillButton( 
			Vec2i( x + d2 , y + d2 ) , Vec2i( ItemSize , ItemSize )  , this );
		button->setFunction( getActor() ,  skillName[i]  );

		skButtonVec.push_back( button );
	}
}

void CSkillFrame::playButton( int index )
{
	if ( getButton( index ) )
		getButton( index )->play();
}

void CSkillFrame::onRender()
{
	BaseClass::onRender();

	Vec3D pos = mSprite->getWorldPosition();

	int x = pos.x + 70;
	int y = pos.y + getSize().y - CellSize ;

	CFont& font = CUISystem::getInstance().getDefultFont();

	setupUITextDepth();

	font.begin();
	for ( int i = 0 ; i < skButtonVec.size() ; ++i )
	{
		CPlayButton* button = skButtonVec[i];
		char const* fullName = TSkillLibrary::getInstance().getFullName( button->getPlayName() );
		font.write( x , y - i * CellSize ,  fullName );
	}
	font.end();
}
