#include "ProjectPCH.h"
#include "CItemBagPanel.h"

#include "CActor.h"
#include "TItemSystem.h"
#include "UtilityGlobal.h"
#include "CUISystem.h"

CItemBagFrame::Button::Button( unsigned slotPos , Vec2i const& pos , Vec2i const& size  , CWidget* parent )
	:CPlayButton( CBT_ITEMBAG_PANEL , pos , size , parent )
	,m_slotPos( slotPos )
{


}

void CItemBagFrame::Button::onClick()
{
	getActor()->playItemBySlotPos( m_slotPos );
}

CActor* CItemBagFrame::Button::getActor()
{
	return static_cast< CItemBagFrame*>( getParent() )->getActor();
}

void CItemBagFrame::Button::onInputItem( CPlayButton* button )
{
	if ( button->m_cbType == CBT_ITEMBAG_PANEL )
	{
		TItemStorage& IS = getActor()->getItemStorage();
		IS.swapItemSlot( m_slotPos , static_cast< CItemBagFrame::Button*>( button )->m_slotPos );

		PlayInfo info = m_playInfo;
		PlayInfo& bInfo = static_cast< CItemBagFrame::Button*>( button)->m_playInfo;

		setFunction( bInfo.caster , bInfo.name );
		button->setFunction( info.caster ,info.name );
	}
}

void CItemBagFrame::Button::onUpdateUI()
{
	CPlayButton::onUpdateUI();

	TPROFILE("TItemBagButton");

	TItemStorage& IS = getActor()->getItemStorage();
	TItemBase* item = IS.getItem( m_slotPos );
	if ( item )
	{
		setFunction( getActor() , item->getName() );
	}
	else
	{
		setFunction( getActor() , NULL );
	}
}

void CItemBagFrame::Button::onRender()
{
	BaseClass::onRender();
	renderText( CUISystem::Get().getDefultFont() );
}

TItemStorage& CItemBagFrame::Button::getItemStorage()
{
	return getActor()->getItemStorage();
}

void SetItemHelpTip( CToolTipUI& helpTip , TItemBase* item )
{
	char str[64];
	if ( item->isWeapon() )
	{
		sprintf( str , "攻擊值 + %d" , static_cast< TEquipment* >( item )->getEquiVal() );
		helpTip.appendString( str );
	}
	else if ( item->isArmour() )
	{
		sprintf( str , "裝甲值 + %d" , static_cast< TEquipment* >( item )->getEquiVal() );
		helpTip.appendString( str );
	}


	TItemBase::PMInfoVec& pmVec = item->getPropModifyVec();

	static char const* opStr[] = { "+" , "x" , "-" , "/" };

	for ( int i = 0 ; i < pmVec.size() ; ++ i )
	{
		PropModifyInfo& info = pmVec[i];
		int op = info.flag & PF_VALUE_OPERAOTR;
		char const* propName = NULL;

		enum 
		{
			TYPE_INT,
			TYPE_FLOAT,
		};
		int type = TYPE_INT;
		switch ( info.prop )
		{
		case PROP_HP:        propName = "生命值"; break;
		case PROP_MP:        propName = "魔法值"; break;
		case PROP_MAX_HP:    propName = "最大生命值"; break;
		case PROP_MAX_MP:    propName = "最大魔法值"; break;
		case PROP_STR:       propName = "力量"; break;
		case PROP_INT:       propName = "智力"; break;
		case PROP_END:       propName = "耐力"; break;
		case PROP_DEX:       propName = "敏捷"; break;
		case PROP_AT_SPEED:  propName = "攻擊速度"; type = TYPE_FLOAT; break;
		case PROP_MV_SPEED:  propName = "移動速度"; type = TYPE_FLOAT; break;
		}
		if ( propName )
		{
			if ( type == TYPE_FLOAT || op == PF_VOP_MUL || op == PF_VOP_DIV )
			{
				sprintf( str , "%s %s%.3f" , propName , opStr[ op ] , info.val );
				helpTip.appendString( str );
			}
			else
			{
				sprintf( str , "%s %s%d" , propName , opStr[ op ] , (int)info.val );
				helpTip.appendString( str );
			}
		}

	}

	char const* helpStr = item->getHelpString();

	if ( helpStr[0] )
	{
		helpTip.appendString( helpStr );
	}
}

void CItemBagFrame::Button::onSetHelpTip( CToolTipUI& helpTip )
{
	Vec2i pos = getWorldPos();

	helpTip.setOpacity( 0.8 );
	helpTip.appendString( m_playInfo.name );

	TItemBase* item = getItemStorage().getItem( m_slotPos );

	::SetItemHelpTip( helpTip , item );

	helpTip.fitSize();
	helpTip.setPos( Vec2i( pos.x, pos.y - helpTip.getSize().y ) );
}

void CItemBagFrame::Button::renderText( CFont& font )
{
	int num = getItemStorage().getItemNum( m_slotPos );
	if ( num  )
	{
		Vec2i pos = getWorldPos();

		setupUITextDepth();
		//float depth = getDepth() - 5;
		//CFly::ShuFa::setDepth( 1 - ( depth + 10000 ) / 20000 );
		//CFly::ShuFa::setDepth( depth );
		font.setColor( Color4ub(255,255,255,255) );
		font.begin();
		font.write( pos.x + getSize().x - font.width , pos.y + getSize().y - font.height , "%d" , num );
		font.end();
	}
}

CItemBagFrame::CItemBagFrame( CActor* actor , Vec2i const& pos ) 
	:BaseClass( pos , Vec2i( CellSize * RowItemNum + 20  , CellSize * 8 + 20 ) , NULL )
{
	m_actor = actor;

	TItemStorage& IS = m_actor->getItemStorage();

	int d1 = ( CellSize - BoardSize )/2;
	int d2 = ( CellSize - ItemSize )/2;

	CUISystem::Get().setTextureDir( "Data/UI" );
	Texture* tex = CUISystem::Get().fetchTexture( "slot_traits" );


	mSlotBKSpr->setRenderOption( CFly::CFRO_ALPHA_BLENGING , true );

	for ( int i = 0 ; i < IS.getItemSlotNum(); ++i )
	{
		int x = 10 + CellSize * ( i % RowItemNum );
		int y = getSize().y - CellSize * ( i / RowItemNum ) - ( CellSize + 20 );

		mSlotBKSpr->createRectArea( x + d1 , y + d1 , BoardSize , BoardSize , &tex , 1 );
		Button* ui = new Button( i , Vec2i( x + d2 , y + d2 ) , Vec2i( ItemSize , ItemSize ) , this );

		m_buttons.push_back( ui );

		TItemBase* item = IS.getItem( i );
		if ( item )
		{
			ui->setFunction( m_actor , item->getName() );
		}
	}
}

void CItemBagFrame::onRender()
{
	BaseClass::onRender();
	//g_TextFont.start();
	//for( int i = 0 ; i < m_buttons.size() ; ++i )
	//{
	//	m_buttons[i]->render( g_TextFont );
	//}
	//g_TextFont.finish();
}