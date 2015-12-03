#include "TinyGamePCH.h"
#include "HoldemScene.h"

#include "GameWidget.h"
#include "GamePlayer.h"
#include "GameGUISystem.h"

#include "CardDraw.h"

#include <string>

namespace Poker { namespace Holdem {

	static Vec2i const tableCenterPos = Vec2i( gDefaultScreenWidth , gDefaultScreenHeight ) / 2;

	static int const offsetDownA   = 100;
	static int const offsetDownB   = 200;
	static int const offsetRightA  = 150;
	static int const offsetRightB  =  offsetRightA * 2;

	static Vec2i const SlotOffset[ MaxPlayerNum ] = 
	{
		Vec2i( offsetRightA , -offsetDownB ) , 
		Vec2i( offsetRightB , -offsetDownA ) , 
		Vec2i( offsetRightB ,  offsetDownA ) , 
		Vec2i( offsetRightA ,  offsetDownB ) , 
		Vec2i( 0 , offsetDownB ) , 
		Vec2i( -offsetRightA ,  offsetDownB ) , 
		Vec2i( -offsetRightB ,  offsetDownA ) , 
		Vec2i( -offsetRightB , -offsetDownA ) , 
		Vec2i( -offsetRightA , -offsetDownB ) , 
	};


	BetPanelBase::BetPanelBase( int id , Vec2i const& pos , GWidget* parent ) 
		:BaseClass( id , pos , Vec2i( 300 , 100 ) , parent )
	{
		GButton* button;
		Vec2i size( 80 , 20 );
		int len = size.x + 5;
		button = new GButton( UI_CALL , Vec2i( 10 , 10 ) , size , this );
		button->setTitle( "Call" );
		button = new GButton( UI_FOLD , Vec2i( 10 + len  , 10), size , this );
		button->setTitle( "Fold" );
		button = new GButton( UI_ALL_IN , Vec2i( 10 + 2*len , 10  )  , size , this );
		button->setTitle( "All In" );
		button = new GButton( UI_RISE , Vec2i( 10 , 10 + 30 ) , size , this );
		button->setTitle( "Rise" );

		GSlider* slider = new GSlider( UI_RISE_MONEY , Vec2i( 10 + len , 10 + 30 ) , size.x , true , this );
		GTextCtrl* ctrl = new GTextCtrl( UI_RISE_MONEY_TEXT , Vec2i( 10 + 2 * len , 10 + 30 ) , size.x , this );
	}

	void BetPanelBase::doRefreshWidget( LevelBase& level , int pos )
	{
		if ( pos != -1 )
		{
			SlotInfo& info = level.getSlotInfo( pos );
			bool beE = info.ownMoney + info.betMoney[ level.getBetStep() ] > level.getMaxBetMoney() ;
			findChild( UI_CALL )->enable( beE );
			findChild( UI_RISE )->enable( beE );
			//findChild( UI_ALL_IN )->enable( true );
		}
	}

	bool BetPanelBase::onChildEvent( int event , int id , GWidget* ui )
	{
		switch ( id )
		{
		case UI_RISE_MONEY_TEXT:
			switch( event )
			{
			case EVT_TEXTCTRL_CHANGE:
				char const* str = GUI::castFast< GTextCtrl* >( ui )->getValue();
				if ( str )
				{
					char* end;
					int money = strtol( str , &end , 10 );
					if ( money > 0 && *end == '\0' )
					{
						setRiseMoney( money );
					}
				}
				return false;
			}
			break;
		case UI_RISE_MONEY:
			{
				int money = GUI::castFast< GSlider* >( ui )->getValue();
				setRiseMoney( money );
			}
			return false;
		}

		return true;
	}

	void BetPanelBase::setRiseMoney( int money )
	{

	}

	class BetPanel : public BetPanelBase
	{
		typedef BetPanelBase BaseClass;
	public:
		BetPanel( int id , Vec2i const& pos , GWidget* parent )
			:BaseClass( id , pos , parent ){}

		bool onChildEvent( int event , int id , GWidget* ui )
		{
			switch( id )
			{
			case UI_FOLD:
				mScene->betRequest( BET_FOLD );
				break;
			case UI_ALL_IN:
				mScene->betRequest( BET_ALL_IN );
				break;
			case UI_RISE:
				mScene->betRequest( BET_RAISE , mScene->getLevel().getRule().bigBlind );
				break;
			case UI_CALL:
				mScene->betRequest( BET_CALL );
				break;
			case UI_RISE_MONEY:
				break;
			}
			return BaseClass::onChildEvent( event , id , ui );
		}

		void refreshWidget()
		{
			ClientLevel& level = mScene->getLevel();
			doRefreshWidget( level , level.getPlayerPos() );
		}

		void setScene( Scene& scene )
		{
			mScene = &scene;
		}

		Scene* mScene;
	};


	class SlotPanel : public GPanel
	{
		typedef GPanel BaseClass;
	public:
		SlotPanel( int id , Vec2i const& pos , GWidget* parent )
			:BaseClass( id , pos , Size , parent )
		{

		}
		
		static Vec2i const Size; 
		int slotPos;
	};

	Vec2i const SlotPanel::Size( 120 , 60 ); 

	Scene::Scene( ClientLevel& level , IPlayerManager& playerMgr ) 
		:mLevel( &level )
		,mPlayerMgr( &playerMgr )
		,mPanel( NULL )
	{
		mLevel->setListener( this );
		mCardDraw = ICardDraw::create( ICardDraw::eWin7 );
	}

	Scene::~Scene()
	{
		mLevel->setListener( NULL );
		mCardDraw->release();
	}

	void Scene::render( Graphics2D& g )
	{
		for( int i = 0 ; i < MaxPlayerNum ; ++i )
		{
			SlotInfo const& info = mLevel->getSlotInfo( i );
			drawPlayerState( g , Vec2i( 20 , 20 + 30 * i ) , i , info );
			drawSlot( g , tableCenterPos + SlotOffset[i] , info );
		}
		
		FixString< 64 > str;
		char const* suit[] = { "C" ,"D" , "H" ,"S" };
		int cardNum = mLevel->getCommunityCardNum();
		for( int i = 0 ; i < cardNum ; ++i )
		{
			Card const& card = mLevel->getCommunityCard( i );
			str += "[";
			str += suit[ card.getSuit() ];
			str += Card::toString( card.getFace() );
			str += "]";

			mCardDraw->draw( g , tableCenterPos + Vec2i( i * 50 , -10 ) , card );
		}
		g.drawText( Vec2i( 200 , 200 ) , str.c_str() );

		if ( getLevel().getPlayerPos() != -1 )
		{
			str.clear();
			for ( int i = 0 ; i < PocketCardNum ; ++i )
			{
				Card const& card = mLevel->getPoketCard( i );
				str += "[";
				str += suit[ card.getSuit() ];
				str += Card::toString( card.getFace() );
				str += "]";
			}
			g.drawText( Vec2i( 200 , 200 + 20 ) , str.c_str() );
		}

		GWidget* ui = ::Global::getGUI().getManager().getMouseUI();
		if ( ui )
		{
			FixString< 128 > str;
			str.format( "( %d , %d )" , ui->getPos().x , ui->getPos().y );
			g.drawText( Vec2i( ::Global::getDrawEngine()->getScreenWidth() , 10 ) , str );
		}
	}

	void Scene::drawPlayerState( Graphics2D& g , Vec2i const& pos , int posSlot , SlotInfo const& info )
	{
		FixString<64> str;

		g.setTextColor( 255 , 0 , 0 );
		switch( info.state )
		{
		case SLOT_WAIT_NEXT:
		case SLOT_PLAY:
			{
				char const* betState[] = { "None" , "Fold" , "Call" , "Raise" , "All In" };

				GamePlayer* player = mPlayerMgr->getPlayer( info.playerId );
				char const* name = "unknown";
				if ( player )
				{
					name = player->getName();
				}
				str.format( "%d-%s %d " , posSlot , name , info.ownMoney );
				g.drawText( pos , str );
				str.format( "%s %d %d %d %d" , betState[ info.betType ] , info.betMoney[0] , info.betMoney[1] , info.betMoney[2] , info.betMoney[3] );
				g.drawText( pos + Vec2i( 0 , 15 ) , str );

				if ( mLevel->getButtonPos() == posSlot )
				{
					g.drawText( pos - Vec2i( 15 , 0 ) , "(B)" );
				}
			}
			break;
		case SLOT_EMPTY:
			str.format( "%d-(Empty)" , posSlot );
			g.drawText( pos , str );
			break;
		}
	}


	void Scene::setupWidget()
	{
		mPanel = new BetPanel( UI_ANY , Vec2i( 100 , 100 ) , NULL );
		mPanel->enable( false );
		mPanel->setScene( *this );


		for( int i = 0 ; i < MaxPlayerNum ; ++i )
		{
			SlotPanel* panel = new SlotPanel( UI_ANY , tableCenterPos + SlotOffset[i] + Vec2i( 0 , 50 ) - SlotPanel::Size / 2 , NULL );
			panel->slotPos = i;
			panel->setRenderCallback( RenderCallBack::create( this , &Scene::drawSlotPanel ) );
			::Global::getGUI().addWidget( panel );
		}
		::Global::getGUI().addWidget( mPanel );
	}

	void Scene::onBetCall( int slot )
	{
		mPanel->enable( slot == mLevel->getPlayerPos() );
		mPanel->show( slot == mLevel->getPlayerPos() );
		mPanel->refreshWidget();
	}

	void Scene::drawSlotPanel( GWidget* widget )
	{
		Graphics2D& g = ::Global::getGraphics2D();

		SlotPanel* panel = GUI::castFast< SlotPanel* >( widget );
		SlotInfo& info = getLevel().getSlotInfo( panel->slotPos );
		Vec2i worldPos = widget->getWorldPos();

		if ( info.state != SLOT_EMPTY )
		{
			GamePlayer* player = mPlayerMgr->getPlayer( info.playerId );
			if ( player )
			{
				g.drawText( worldPos , Vec2i( SlotPanel::Size.x , 20 ) , player->getName() );
			}

			FixString< 64 > str;
			str.format( "%d" , info.ownMoney );
			g.drawText( worldPos + Vec2i( 0 , 13 ) , Vec2i( SlotPanel::Size.x , 20 ) , str );
		}
	}

	void Scene::drawSlot( Graphics2D& g , Vec2i const& pos , SlotInfo const& info )
	{
		if ( info.state == SLOT_EMPTY )
			return;

		if ( getLevel().getBetStep() != STEP_NO_BET  )
		{
			if ( info.betType != BET_FOLD )
			{
				Vec2i cardPos = pos - mCardDraw->getSize() / 2;
				if ( info.pos == getLevel().getPlayerPos() )
				{
					for ( int i = 0 ; i < PocketCardNum ; ++i )
					{
						Card const& card = mLevel->getPoketCard( i );
						mCardDraw->draw( g , cardPos + Vec2i( 20 * i , 0 ) , card );
					}
				}
				else
				{
					for ( int i = 0 ; i < PocketCardNum ; ++i )
					{
						Card const& card = mLevel->getPoketCard( i );
						mCardDraw->drawCardBack( g , cardPos + Vec2i( 20 * i , 0 ) );
					}
				}
			}
		}
	}

}//namespace Holdem
}//namespace Poker
