#include "ProjectPCH.h"
#include "CStoreFrame.h"

#include "CNPCBase.h"

#include "TItemSystem.h"
#include "AICondition.h"

#include "EventType.h"
#include "UtilityGlobal.h"

#include "CUISystem.h"

//#include "TAudioPlayer.h"

static CStoreFrame* g_storePanel;
CStoreFrame* getStorePanel()
{
	return g_storePanel;
}


CStoreButton::CStoreButton( Vec2i const& pos , Vec2i const& size , CWidget* parent ) 
	:CPlayButton( CBT_STORE_PANEL , pos , size , parent )
{

}

void CStoreButton::onClick()
{
	static_cast< CStoreFrame* >( getParent() )->buyItem( m_slotPos );
}

void CStoreButton::onSetHelpTip( CToolTipUI& helpTip )
{
	Vec3D pos = mSprite->getWorldPosition();

	helpTip.setOpacity( 0.8 );
	helpTip.appendString( m_playInfo.name );

	static_cast< CStoreFrame*>( getParent() )->setHelpTip( m_slotPos , helpTip );

	helpTip.fitSize();
	helpTip.setPos( Vec2i( pos.x, pos.y - helpTip.getSize().y ) );

}

CStoreFrame::CStoreFrame( Vec2i const& pos ) 
	:CSlotFrameUI( pos , Vec2i( ( CellSize + ItemStrLen ) * RowItemNum + 20  , 
	                           CellSize * 6 + 20 ) , NULL )
{
	g_storePanel = this;

	m_seller = NULL;
	m_buyer  = NULL;

	m_totalPage = 0;
	m_page = 0;

	int btnSize = 25;
	Vec2i size = getSize();

	{
		char const* dir = "Data/UI/miniMap";
		char const* texName[] = { "up" , "up_press" , "up" };
		CSimpleButton* button  = new CSimpleButton( dir , &texName[0] , 
			Vec2i( size.x - btnSize - 30 ,  10 ) , 
			Vec2i( btnSize , btnSize ) , this  );

		UG_ConnectEvent( EVT_UI_BUTTON_CLICK , button->getSprite()->getEntityID() , 
			EventCallBack( this , &CStoreFrame::pageUp ) );
	}

	{
		char const* dir = "Data/UI/miniMap";
		char const* texName[] = { "down" , "down_press" , "down" };
		CSimpleButton* button  = new CSimpleButton( dir , &texName[0] , 
			Vec2i( size.x - 2 * btnSize - 30 ,  10 ) ,
			Vec2i( btnSize , btnSize ) , this  );

		UG_ConnectEvent( EVT_UI_BUTTON_CLICK , button->getSprite()->getEntityID() ,
			EventCallBack( this, &CStoreFrame::pageDown ) );
	}

	int d1 = ( CellSize - BoardSize )/2;
	int d2 = ( CellSize - ItemSize )/2;

	CUISystem::Get().setTextureDir( "Data/UI" );
	Texture* tex = CUISystem::Get().fetchTexture( "slot_traits" );

	mSlotBKSpr->setRenderOption( CFly::CFRO_ALPHA_BLENGING , true );

	for ( int i = 0 ; i < PageShowNum ; ++i )
	{
		int x = 10 + ( CellSize + ItemStrLen ) * ( i % RowItemNum );
		int y = getSize().y - CellSize * ( i / RowItemNum ) - ( CellSize + 20 );

		mSlotBKSpr->createRectArea( x + d1 , y + d1 , BoardSize , BoardSize , &tex , 1  );
		m_buttons[i] = new CStoreButton( 
			Vec2i( x + d2 , y + d2 ) , Vec2i( ItemSize , ItemSize ) , this );
	}
}

CStoreFrame::~CStoreFrame()
{
	UG_DisconnectEvent( EVT_UI_BUTTON_CLICK , EventCallBack( this , &CStoreFrame::pageUp ) );
	UG_DisconnectEvent( EVT_UI_BUTTON_CLICK , EventCallBack( this, &CStoreFrame::pageDown ) );
}

void CStoreFrame::refreshGoods( int page )
{
	m_page = page;

	int showIndex = page * PageShowNum;
	if ( m_type == SELL_ITEMS )
	{
		TItemStorage& storage = m_seller->getItemStorage();

		for ( int i = showIndex ; i < showIndex + PageShowNum  ; ++i )
		{
			TItemBase* item;
			if (  i < storage.getItemSlotNum() &&
				 ( item = storage.getItem(i)  ) )
			{
				m_buttons[ i - showIndex ]->setSlotPos( i );
				m_buttons[ i - showIndex ]->setFunction( m_buyer , item->getName() );
			}
			else
			{
				m_buttons[ i - showIndex ]->setSlotPos( i );
				m_buttons[ i - showIndex ]->setFunction( m_buyer , NULL );
			}
		}
	}
	else if ( m_type == SELL_SKILL )
	{
		assert( 0 );
	}
}

void CStoreFrame::onShow( bool beS )
{
	BaseClass::onShow( beS );

	if ( !beS && m_seller )
	{
		m_seller->addCondition( CDT_TRADE_END );
	}
	
}

void CStoreFrame::startTrade( SellType type , CNPCBase* seller , 
							  CActor* buyer )
{
	m_buyer  = buyer;
	m_seller = seller;
	m_type = type;

	if ( m_type == SELL_ITEMS )
	{
		TItemStorage& storage = m_seller->getItemStorage();
		m_totalPage = storage.getUsedSlotNum() / PageShowNum;
		if ( storage.getUsedSlotNum() % PageShowNum )
			++m_totalPage; 
	}
	else if ( m_type == SELL_SKILL )
	{
		assert( 0 );
	}

	refreshGoods( 0 ); 
	show( true );
}

bool CStoreFrame::buyItem( int pos )
{
	if ( m_type == SELL_ITEMS )
	{
		TItemStorage& storage = m_seller->getItemStorage();

		TItemBase* item = storage.getItem( pos );

		if ( item == NULL )
			return false;

		if ( m_buyer->getMoney() < item->getBuyCast() )
		{
			//MessgeUI( 255 , 0 , 0 ,"你不夠錢買這項物品" );
			//TAudioPlayer::instance().play( "wrong" , 1.0 );
			return false;
		}

		if ( !m_buyer->getItemStorage().haveEmptySolt( 1 ))
		{
			//MessgeUI( 255 , 0 , 0 , "你的包包沒有足夠的空間" );
			//TAudioPlayer::instance().play( "wrong" , 1.0 );
			return false;
		}

		m_buyer->spendMoney( item->getBuyCast() );
		m_buyer->addItem( item->getID() , 1 );

		//TAudioPlayer::instance().play( "buyItem" , 1.0 );

		return true;
	}

	return false;
}

void CStoreFrame::onRender()
{
	if ( !m_seller || !m_buyer )
		return;

	int showIndex = m_page * PageShowNum;


	Vec3D pos = mSprite->getWorldPosition();

	int x = pos.x + 70;
	int y = pos.y + getSize().y - CellSize ;

	CFont& font = CUISystem::Get().getDefultFont();

	setupUITextDepth();

	font.begin();

	if ( m_type == SELL_ITEMS )
	{
		TItemStorage& storage = m_seller->getItemStorage();

		for ( int i = 0 ; i < PageShowNum  ; ++i )
		{
			if (  i + showIndex >= storage.getItemSlotNum() )
				continue;

			TItemBase*  item = storage.getItem( i + showIndex );
			if ( item )
			{
				Vec3D pos = mSprite->getWorldPosition();;

				int x = pos.x + 10  + ( CellSize + ItemStrLen ) * ( i % RowItemNum ) + CellSize ;
				int y = pos.y + getSize().y - CellSize * ( i / RowItemNum ) - ( CellSize + 20 ) + CellSize - 15 ;

				font.write( x , y  ,  item->getName() );
				font.write( x + ItemStrLen - 40 , y - 20, "%5d G" , item->getBuyCast() );
			}
		}
	}

	int money = (int) m_buyer->getMoney() ;

	font.write( pos.x + getSize().x - 200 , pos.y + 30, "金錢 : %5d G" , money );
	font.write( pos.x + getSize().x - 200 , pos.y + 10, "第%d / %d頁 " , m_page + 1 , m_totalPage );

	font.end();

}

void CStoreFrame::pageUp( TEvent const& event )
{
	int page = TMin( m_page + 2 , m_totalPage );
	--page;
	if ( page != m_page )
		refreshGoods( page );
}

void CStoreFrame::pageDown( TEvent const& event )
{
	int page = TMax( m_page - 1 , 0 );
	if ( page != m_page )
		refreshGoods( page );
}

void CStoreFrame::setHelpTip( int pos , CToolTipUI& helpTip )
{
	if ( m_type == SELL_ITEMS )
	{
		TItemStorage& storage = m_seller->getItemStorage();
		TItemBase* item = storage.getItem( pos );
		::SetItemHelpTip(  helpTip , item );
	}
}
