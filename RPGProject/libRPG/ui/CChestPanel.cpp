#include "ProjectPCH.h"
#include "CChestPanel.h"

#include "CUISystem.h"

#include "TItemSystem.h"
#include "CActor.h"


CChestPanel::CChestPanel( CActor* actor , Vec2i const& pos )
	:BaseClass( pos , Vec2i( CellSize * RowItemNum + 20  , CellSize * 4 + 40 ) , NULL )
{
	m_actor = actor;
	m_items = NULL;

	int d1 = ( CellSize - BoardSize )/2;
	int d2 = ( CellSize - ItemSize )/2;

	CUISystem::Get().setTextureDir( "Data/UI" );
	Texture* tex = CUISystem::Get().fetchTexture( "slot_traits" );

	for ( int i = 0 ; i < MaxItemNum ; ++i )
	{
		int x = 10 + CellSize * ( i % RowItemNum );
		int y = getSize().y - CellSize * ( i / RowItemNum ) - ( CellSize + 20 );

		mSlotBKSpr->createRectArea( x , y , BoardSize , BoardSize , &tex , 1  );

		Button* ui = new Button( i ,
			Vec2i( x + d2 , y + d2 ) , Vec2i( ItemSize , ItemSize ) , this );

		m_buttons.push_back( ui );
	}

}

void CChestPanel::setItemStorage( TItemStorage* items )
{
	m_items = items;
	for( int i = 0 ; i < MaxItemNum ; ++i  )
	{
		if ( items )
		{
			TItemBase* item = items->getItem( i );
			if ( item )
			{
				m_buttons[i]->setFunction( m_actor , item->getName() );
			}
			else
			{
				m_buttons[i]->setFunction( m_actor , NULL );
			}
		}
		else
		{
			m_buttons[i]->setFunction( m_actor , NULL );
		}
	}
}

bool CChestPanel::takeItem( int slotPos )
{
	if ( !m_items )
		return false;

	TItemStorage::ItemSlot* slot = m_items->getItemSlot( slotPos );

	if ( !slot )
		return false;

	m_actor->addItem( slot->itemID  , slot->num  );
	m_items->removeItemSlot( slotPos );
	return true;
}

TItemBase* CChestPanel::getItem( int pos )
{
	return m_items->getItem( pos );
}

CChestPanel::Button::Button( unsigned slotPos , Vec2i const& pos , Vec2i const& size , CChestPanel* parent ) 
	:BaseClass( CBT_CHEST_PANEL , pos , size , parent )
	,m_slotPos( slotPos )
{

}

void CChestPanel::Button::onClick()
{
	static_cast< CChestPanel* >( getParent() )->takeItem( m_slotPos );
	setFunction( NULL , NULL );
}

void CChestPanel::Button::onSetHelpTip( CToolTipUI& helpTip )
{
	Vec2i pos = getWorldPos();

	helpTip.setOpacity( 0.8 );
	helpTip.appendString( m_playInfo.name );

	TItemBase* item = static_cast< CChestPanel* >( getParent() )->getItem( m_slotPos );
	::SetItemHelpTip( helpTip , item );

	helpTip.fitSize();
	helpTip.setPos( Vec2i( pos.x, pos.y - helpTip.getSize().y ) );
}
