#include "TinyGamePCH.h"
#include "Big2Scene.h"

#include "Big2Level.h"

#include "DrawEngine.h"
#include "RenderUtility.h"

#include "GameGUISystem.h"
#include "GamePlayer.h"

#include "EasingFun.h"
#include "resource.h"



namespace Poker { namespace Big2 {

	class CardButton : public GUI::ButtonT< CardButton >
	{
		typedef GUI::ButtonT< CardButton > BaseClass;
	public:
		CardButton( Scene* scene , int idx )
			:BaseClass( Vec2i(0,0) , scene->getCardSize() , NULL )
		{
			setUserData( intptr_t( scene ) );
			index = idx;
			show( false );
		}

		static Vec2i calcPos( int index )
		{
			return Vec2i( 100 + index * 15 , 100 ); 
		}

		void reset( Card const& c )
		{
			card  = c;
			beSelected = false;
			setPos( calcPos( index ) );
			show( true );
		}

		Card card;
		bool beSelected;
		int  index;

		Scene& getScene(){ return *static_cast<Scene*>( (void*)getUserData() );  }

		
		void  onRender()
		{
			getScene().getCardDraw().draw( ::Global::GetGraphics2D() , getWorldPos() , card );
		}
		void  onClick()
		{
			//beSelected = getScene().toggleCardSelect( index , beSelected );

			if ( beSelected )
				setPos( calcPos( index ) + Vec2i( 0 , -10 ) );
			else
				setPos( calcPos( index ) );
		}
	};



	class ActionButton : public GUI::ButtonT< ActionButton >
	{
		typedef GUI::ButtonT< ActionButton > BaseClass;
	public:
		enum Action
		{
			eSINGLE    ,
			eONE_PAIR  ,
			eSTRAIGHT  ,
			eFULL_HOUSE ,
			eFOUR_OF_KIND ,
			eSTRAIGHT_FLUSH ,

			eFLUSH     ,

			ePASS         ,
			eSHOW_CARD    ,
			eCLEAR_SELECT ,
		};
		ActionButton( Scene* scene , Action action  , Vec2i const& pos , Vec2i const& size , GWidget* parent = NULL )
			:BaseClass( pos , size , parent )
			,mAction( action )
		{
			setUserData( intptr_t(scene) );
		}


		Action mAction;

		Scene& getScene(){ return *static_cast<Scene*>( (void*)getUserData() );  }
		void   onClick()
		{
			switch( mAction )
			{
			case eSINGLE:         getScene().nextCombination( CG_SINGLE ); break;
			case eONE_PAIR:       getScene().nextCombination( CG_ONE_PAIR ); break;
			case eFLUSH:          getScene().nextCombination( CG_FLUSH ); break;
			case eFULL_HOUSE:     getScene().nextCombination( CG_FULL_HOUSE ); break;
			case eSTRAIGHT:       getScene().nextCombination( CG_STRAIGHT ); break;
			case eFOUR_OF_KIND:   getScene().nextCombination( CG_FOUR_OF_KIND ); break;
			case eSTRAIGHT_FLUSH: getScene().nextCombination( CG_STRAIGHT_FLUSH ); break;
			case eSHOW_CARD:      getScene().showCard(); break;
			case ePASS:           getScene().getLevel().passCard(); break;
			case eCLEAR_SELECT:   getScene().clearSelect();
			}
		}
		void onRender()
		{
			IGraphics2D& g = ::Global::GetIGraphics2D();
			Vec2i pos  = getWorldPos();
			Vec2i size = getSize();

			WidgetColor color;
			color.setName(EColor::Blue);
			GWidget::getRenderer().drawButton( g , pos  , size , getButtonState() , color, isEnable() );

			char const* str = "??";
			if ( getButtonState() == BS_PRESS )
				pos += Vec2i(2,2);

			switch( mAction )
			{
			case eSINGLE:         str = "Single";    break;
			case eONE_PAIR:       str = "One Pair";  break;
			case eFULL_HOUSE:     str = "Full House";break;
			case eSTRAIGHT:       str = "Straight";  break;
			case eFLUSH:          str = "Flush";     break;
			case eFOUR_OF_KIND:   str = "Four Of Kind";   break;
			case eSTRAIGHT_FLUSH: str = "Straight Flush"; break;
			case eSHOW_CARD:      str = "Show";   break;
			case ePASS:           str = "Pass";   break;
			case eCLEAR_SELECT:   str = "Clear";  break;
			}
			RenderUtility::SetFont( g , FONT_S8 );
			g.drawText( pos , size , str );
		}
	};
	Vec2i const gTablePosOffset[] = { Vec2i( 0, 8 ) , Vec2i( 20 , 0 ) , Vec2i( 0 , -8 ) , Vec2i( -20 , 0 ) };
	int const TimeDealCard = 30;


	Scene::Scene( ClientLevel& level , IPlayerManager& playerMgr ) 
		:mLevel( &level )
		,mPlayerManager( &playerMgr )
	{
		level.setListener( *this );

		mCardDraw = ICardDraw::Create( ICardDraw::eWin7 );
		mCardSize = mCardDraw->getSize();

		mShowCardBmp.initialize( ::Global::GetGraphics2D().getTargetDC() , 600 , 400 );
	}

	Scene::~Scene()
	{
		mCardDraw->release();
	}


	void Scene::setupUI()
	{

		Vec2i sSize = ::Global::GetDrawEngine()->getScreenSize();

		CardListUI::setScene( *this );
		mOwnCardsUI = new CardListUI( getLevel().getOwnCards() , Vec2i( sSize.x / 2 , sSize.y - mCardSize.y - 75 ) , NULL );
		mOwnCardsUI->onChangeIndexSelected.bind( this , &Scene::onListCardSelect );
		mOwnCardsUI->show( false );
		::Global::GUI().addWidget( mOwnCardsUI );

		int numCol = 6;
		Vec2i size( 80 , 20 );
		int offset = size.x + 3;
		Vec2i pos = Vec2i( sSize.x / 2 - ( numCol * offset )/ 2 , sSize.y - size.y - 30 );
		ActionButton* button;
		

#define  ACTION_BUTTON( ID )\
	button = new ActionButton( this , ID , pos , size , NULL );\
	mButton[ ID ] = button;\
	::Global::GUI().addWidget( button );\
	pos.x += offset;

		ACTION_BUTTON( ActionButton::eSTRAIGHT_FLUSH );
		ACTION_BUTTON( ActionButton::eFOUR_OF_KIND );
		ACTION_BUTTON( ActionButton::eFULL_HOUSE );
		//ACTION_BUTTON( ActionButton::eFLUSH );
		ACTION_BUTTON( ActionButton::eSTRAIGHT );
		ACTION_BUTTON( ActionButton::eONE_PAIR );
		ACTION_BUTTON( ActionButton::eSINGLE );
		
		numCol = 3;
		pos.x = sSize.x / 2 - ( numCol * offset )/ 2;
		pos.y += size.y + 3;
		ACTION_BUTTON( ActionButton::eCLEAR_SELECT );
		ACTION_BUTTON( ActionButton::ePASS );
		ACTION_BUTTON( ActionButton::eSHOW_CARD );

#undef ACTION_BUTTON

		changeState( eDEAL_CARD );
	}

	void Scene::refreshUI( bool haveChange )
	{
		bool beE = getLevel().getNextShowSlot() == getLevel().getPlayerStatus().slotId;
		mButton[ ActionButton::ePASS ]->enable( beE && getLevel().getLastShowSlot() != -1 );
		mButton[ ActionButton::eSHOW_CARD ]->enable( beE );

		if ( haveChange )
		{
			mButton[ ActionButton::eONE_PAIR ]->enable( mHelper.haveGroup( CG_ONE_PAIR ) );
			mButton[ ActionButton::eSTRAIGHT ]->enable( mHelper.haveGroup( CG_STRAIGHT ));
			//mActions[ ActionButton::eFLUSH ]->enable( mHelper.haveGroup( CG_FLUSH ) );
			mButton[ ActionButton::eFULL_HOUSE ]->enable( mHelper.haveGroup( CG_FULL_HOUSE ) );
			mButton[ ActionButton::eFOUR_OF_KIND ]->enable( mHelper.haveGroup( CG_FOUR_OF_KIND ) );
			mButton[ ActionButton::eSTRAIGHT_FLUSH ]->enable( mHelper.haveGroup( CG_STRAIGHT_FLUSH ) );
		}
	}



	void Scene::render( Graphics2D& g )
	{
		Vec2i sSize = ::Global::GetDrawEngine()->getScreenSize();

		int yOffset = 50;
		Vec2i gap( 8 , 8 );
		RenderUtility::SetPen( g , EColor::Black );
		g.setBrush( Color3ub( 100 , 100 , 100 ) );
		g.drawRect( Vec2i(0,0) , sSize );
		g.setBrush( Color3ub( 128 , 64 , 64 ) );
		g.drawRoundRect( gap , sSize - 2 * gap - Vec2i( 0 , yOffset ) , Vec2i( 50 , 50 ) );
		Vec2i gap2( 30 , 30 );
		g.setBrush( Color3ub( 0 , 180 , 10 ) );
		g.drawRoundRect( gap + gap2 , sSize - 2 * ( gap + gap2 ) - Vec2i( 0 , yOffset ) , Vec2i( 70 , 70 ) );

		switch( mState )
		{
		case eDEAL_CARD:
			{
				int numCard = 52;
				if ( mStateTime < TimeDealCard * 52 )
				{
					numCard = ( mStateTime ) / TimeDealCard;
				}

				Vec2i    pos;
				TablePos tPos;
				tPos = TPOS_FRONT;
				pos = Vec2i( sSize.x / 2 , 15 + mCardSize.y  ) ;
				drawOtherPlayer( g , pos , tPos , ( numCard + 2 ) / 4  , false );

				tPos = TPOS_LEFT;
				pos = Vec2i( mCardSize.y + 10 , sSize.y / 2 - yOffset  );
				drawOtherPlayer( g , pos , tPos , ( numCard + 1 ) / 4 , false );

				tPos = TPOS_RIGHT;
				pos =  Vec2i( sSize.x - ( mCardSize.y + 10 ) , sSize.y / 2 - yOffset );
				drawOtherPlayer( g , pos , tPos , ( numCard + 3 ) / 4 , false );

				int num = numCard / 4;
				pos = mOwnCardsUI->getBasePos() - Vec2i( ( ( 13 - 1 ) * CardListUI::NextOffset + mCardSize.x ) / 2 , -CardListUI::SelectOffect );
				for( int i = 0 ; i < num ; ++i )
				{
					mCardDraw->draw( g , pos , Card( int( mCards[i] ) ) );
					pos.x += CardListUI::NextOffset;
				}

				if ( mStateTime > 52 * TimeDealCard + 1000 )
					changeState( ePLAYING );
			}
			break;
		case ePLAYING:
			{
				{
					mShowCardBmp.bitBltTransparent( g.getRenderDC() , RGB(0,255,0) , 
						( sSize.x - mShowCardBmp.getWidth() ) / 2 , 
						( sSize.y - mShowCardBmp.getHeight() - yOffset ) / 2  );
					TrickInfo const* info = getLevel().getLastShowCards();
					if ( info )
					{
						Vec2i pos = Vec2i( sSize.x / 2 , ( sSize.y - yOffset ) / 2 );
						drawLastShowCard( g , pos , true );
					}
				}

				{
					int      slotId;
					Vec2i    pos;
					TablePos tPos;

					bool bDrawBack = ( mState == ePLAYING );

					tPos = TPOS_FRONT;
					pos = Vec2i( sSize.x / 2 , 15 + mCardSize.y );
					slotId = getLevel().getSlotId( tPos );
					drawOtherPlayer( g , pos , tPos , getLevel().getSlotCardNum( slotId ) , true );

					tPos = TPOS_LEFT;
					pos = Vec2i( mCardSize.y + 10 , sSize.y / 2 - yOffset );
					slotId = getLevel().getSlotId( tPos );
					drawOtherPlayer( g , pos , tPos , getLevel().getSlotCardNum( slotId ) , true );

					tPos = TPOS_RIGHT;
					pos =  Vec2i( sSize.x - ( mCardSize.y + 10 ) , sSize.y / 2 - yOffset );
					slotId = getLevel().getSlotId( tPos );
					drawOtherPlayer( g , pos , tPos , getLevel().getSlotCardNum( slotId ) , true );
				}

				{
					RenderUtility::SetBrush( g , EColor::Red );

					TablePos tPos = getLevel().getTablePos( getLevel().getNextShowSlot() );
					g.drawCircle( ( sSize - Vec2i( 0 , yOffset ) ) / 2 + 14 * gTablePosOffset[ tPos ]  , 10 );
				}
				{
					Vec2i pos(0,0);
					for ( int i = 0 ; i < PlayerNum ; ++i )
					{
						drawSlotStatus( g , pos  , i );
						pos.y += 100;
					}
				}
			}
			break;
		case eROUND_END:
			{
				int slotId;
				Vec2i    pos;
				TablePos tPos;

				bool bDrawBack = ( mState == ePLAYING );

				tPos = TPOS_FRONT;
				pos = Vec2i( sSize.x / 2 , 15 + mCardSize.y );
				slotId = getLevel().getSlotId( tPos );
				drawOtherPlayer( g , pos , tPos , getLevel().getSlotCardNum( slotId ) , true , false );

				tPos = TPOS_LEFT;
				pos = Vec2i( mCardSize.y + 10 , sSize.y / 2 - yOffset );
				slotId = getLevel().getSlotId( tPos );
				drawOtherPlayer( g , pos , tPos , getLevel().getSlotCardNum( slotId ) , true , false );

				tPos = TPOS_RIGHT;
				pos =  Vec2i( sSize.x - ( mCardSize.y + 10 ) , sSize.y / 2 - yOffset );
				slotId = getLevel().getSlotId( tPos );
				drawOtherPlayer( g , pos , tPos , getLevel().getSlotCardNum( slotId ) , true , false );	
			}
			break;
		}
	}

	void Scene::updateFrame( int frame )
	{
		mStateTime += gDefaultTickTime * frame;
		mTweener.update( gDefaultTickTime * frame );
	}

	void Scene::drawOtherPlayer( Graphics2D& g , Vec2i const& pos , TablePos tPos , int numCard , bool beCenterPos , bool bDrawBack )
	{
		g.beginXForm();
		g.translateXForm( pos.x , pos.y );
		switch( tPos )
		{
		case TPOS_LEFT:   g.rotateXForm( 90.0f * PI / 180.0f ); break;
		case TPOS_FRONT:  g.rotateXForm( 180.0f * PI / 180.0f ); break;
		case TPOS_RIGHT:  g.rotateXForm( 270.0f * PI / 180.0f ); break;
		}
		int offset = 15;
		Vec2i lPos;
		lPos.x = -( (( beCenterPos ) ? numCard : 13 ) * offset + mCardSize.x ) / 2;
		lPos.y = 0;
		if ( bDrawBack )
		{
			for( int i = 0 ; i < numCard ; ++i )
			{
				mCardDraw->drawCardBack( g , lPos );
				lPos.x += 15;
			}
		}
		else
		{
			int id = getLevel().getSlotId( tPos );
			for( int i = 0 ; i < numCard ; ++i )
			{
				mCardDraw->draw( g , lPos , Card( mCards[ mIdxCard[id] + i ] ) );
				lPos.x += 15;
			}
		}
		g.finishXForm();
	}


	void Scene::drawLastShowCard( Graphics2D& g , Vec2i const& pos , bool beFoucs  )
	{
		TrickInfo const* info = getLevel().getLastShowCards();
		if ( !info )
			return;

		TablePos tPos = getLevel().getTablePos( getLevel().getLastShowSlot() );
		float angle;
		switch( tPos )
		{
		case TPOS_ME:     angle = 0.0f; break;
		case TPOS_LEFT:   angle = 90.0f * PI / 180.0f; break;
		case TPOS_FRONT:  angle = 180.0f * PI / 180.0f; break; 
		case TPOS_RIGHT:  angle = 270.0f * PI / 180.0f; break;
		}
		drawShowCard( g , pos + Vec2i( mShowCardOffset[ tPos ] ) + 5 * gTablePosOffset[ tPos ] , angle , *info , beFoucs );

	}

	void Scene::drawShowCard( Graphics2D& g , Vec2i const& pos , float angle ,  TrickInfo const& info , bool beFoucs  )
	{
		int offset = 20;

		Vec2i renderPos = pos ;

		if ( beFoucs )
		{
			g.setBrush( Color3ub( 255 , 255 , 100 ) );
			g.setPen( Color3ub( 255 , 255 , 100 ) );
		}

		g.beginXForm();
		g.translateXForm( pos.x , pos.y );
		g.rotateXForm( angle );
	
		Vec2i lPos = Vec2i( -( ( info.num - 1 ) * offset + mCardSize.x ) / 2 ,  -mCardSize.y / 2  );
		Vec2i shell( 3 , 3 );
		for( int i = 0 ; i < info.num ; ++i )
		{
			g.pushXForm();

			Vec2i tPos =  lPos + mShowCardTrans[i].offset;
			g.translateXForm( tPos.x , tPos.y );
			//g.transRotate( 0.05 * i );
			if ( beFoucs )
			{
				g.beginBlend( -shell , mCardSize + 2 * shell , 0.8f );
				g.drawRoundRect( -shell , mCardSize + 2 * shell , Vec2i(3,3) );
				g.endBlend();
			}
			mCardDraw->draw( g , Vec2i(0,0) , info.card[i] );
			lPos.x += offset;

			g.popXForm();
		}

		g.finishXForm();
	}

	void Scene::onRoundInit( SDRoundInit const& initData )
	{
		CardDeck& cards = getLevel().getOwnCards();
		mOwnCardsUI->refreshCards();
		mHelper.parse( &cards[0] , cards.size() );
		mRestIterator = true;

		Graphics2D g( mShowCardBmp.getDC() );
		g.setBrush( Color3ub( 0 , 255 , 0 ) );
		g.drawRect( Vec2i(-1,-1) , Vec2i( mShowCardBmp.getWidth() + 2 , mShowCardBmp.getHeight() + 2 ) );

		refreshUI( true );
		std::copy( initData.card , initData.card + 13 , mCards );

		mDealCardNum = 0;
		changeState( eDEAL_CARD );
	}

	void Scene::onGameInit()
	{

	}


	void Scene::onSlotTurn( int slotId , bool beShow )
	{
		if( beShow )
		{
			TrickInfo const* info = getLevel().getLastShowCards();
			TablePos tPos = getLevel().getTablePos( getLevel().getLastShowSlot() );

			Vec2i from;
			switch( tPos )
			{
			case TPOS_ME:   from = Vec2i( 0 , 100 ); break;
			case TPOS_FRONT: from = Vec2i( 0 , -100 ); break;
			case TPOS_LEFT : from = Vec2i( -200 , 0 ); break;
			case TPOS_RIGHT: from = Vec2i( 200 , 0 ); break;
			}
			Vector2 to;
			to.x = ( ::Global::Random() % 50 ) - 25;
			to.y = ( ::Global::Random() % 50 ) - 25;
			mTweener.tweenValue< Easing::OExpo >( mShowCardOffset[ tPos ] , Vector2( from ) , to , 500 )
				.finishCallback( std::tr1::bind( &Scene::drawLastCardOnBitmap , this ) );
			for( int i = 0 ; i < info->num ; ++i )
			{
				mShowCardTrans[i].offset.x = ( ::Global::Random() % 5 ) - 2;
				mShowCardTrans[i].offset.y = ( ::Global::Random() % 10 ) - 5;
			}
		}

		if ( slotId == getLevel().getPlayerStatus().slotId )
		{
			if ( beShow )
			{
				mOwnCardsUI->removeSelected();
				CardDeck& cards = getLevel().getOwnCards();
				mHelper.parse( &cards[0] , cards.size() );
				mRestIterator = true;
			}
			refreshUI( beShow );
			
		}
		else
		{
			refreshUI( false );
		}
	}

	void Scene::onNewCycle()
	{
		refreshUI( false );
	}

	void Scene::onRoundEnd( SDRoundEnd const& endData )
	{
		int num = 0;
		for ( int i = 0 ; i < PlayerNum ; ++i )
		{
			mIdxCard[i] = num;
			num += endData.numCard[i];
		}
		std::copy( endData.card , endData.card + num , mCards );

		changeState( eROUND_END );
	}

	void Scene::showCard()
	{
		int num;
		int* indexSelect = mOwnCardsUI->getSelcetIndex( num );
		getLevel().showCard( indexSelect , num );
	}

	void Scene::onListCardSelect( int* index , int num )
	{
		TrickInfo info;
		CardDeck& ownCards = getLevel().getOwnCards();
		mButton[ ActionButton::eSHOW_CARD ]->enable( 
			TrickUtility::checkCard( &ownCards[0] , ownCards.size() , index , num , info ) );
	}

	void Scene::nextCombination( CardGroup group )
	{
		static bool haveIter = false;
		if ( mRestIterator || mIterator.getGroup() != group )
		{
			mIterator = mHelper.getIterator( group );
			haveIter = mIterator.is();
			mRestIterator = false;
		}
		else if ( haveIter )
		{
			mIterator.goNext();
			if ( !mIterator.is() )
				mIterator.reset();

		}
		if ( haveIter && mIterator.is() )
		{
			int  num;
			int* pIndex = mIterator.getIndex( num );
			mOwnCardsUI->selectIndex( pIndex , num );
		}
	}

	void Scene::changeState( RenderState state )
	{
		mState = state;
		mStateTime = 0;
		switch( mState )
		{
		case eDEAL_CARD:
			mOwnCardsUI->show( false );
			break;
		case ePLAYING: 
			mOwnCardsUI->show( true );
			break;
		case eROUND_END: 
			mOwnCardsUI->show( false );
			break;
		}
	}

	void Scene::clearSelect()
	{
		mOwnCardsUI->clearSelect();
		mIterator.reset();
	}

	void Scene::drawLastCardOnBitmap()
	{
		Graphics2D g( mShowCardBmp.getDC() );
		drawLastShowCard( g , Vec2i( mShowCardBmp.getWidth() / 2 , mShowCardBmp.getHeight() / 2 ) , false );
	}

	void Scene::drawSlotStatus( Graphics2D& g , Vec2i const& pos , int slotId )
	{
		Vec2i renderPos = pos;
		SlotStatus& status = getLevel().getSlotStatus( slotId );
		GamePlayer* player = mPlayerManager->getPlayer( status.playerId );

		FixString< 128 > str;
		RenderUtility::SetFont( g , FONT_S10 );
		g.drawText( renderPos , player->getName() );
		renderPos.y += 12;
		str.format( "Money = %d" , status.money );
		g.drawText( renderPos , str );
		renderPos.y += 12;
		str.format( "Card Num = %d" , getLevel().getSlotCardNum( slotId ) );
		g.drawText( renderPos , str );
		renderPos.y += 12;
	}

	Vec2i      CardListUI::msCardSize;
	ICardDraw* CardListUI::msCardDraw = NULL;

	Scene* CardListUI::msScene = NULL;

	CardListUI::CardListUI( CardDeck& cards , Vec2i const& pos , GWidget* parent ) 
		:BaseClass( pos , Vec2i(0,0) , parent )
	{
		mBasePos = pos;
		mHaveAnim = false;
		std::fill_n( mCardSelectOffset , 13 , 0 );
		mNumSelected = 0;
		setClientCards( cards );
		setRerouteMouseMsgUnhandled();
	}

	void CardListUI::setClientCards( CardDeck& cards )
	{
		mClinetCards = &cards;
		refreshCards();
	}

	void CardListUI::refreshCards()
	{
		setPos( mBasePos - Vec2i( ( ( mClinetCards->size() - 1 ) * NextOffset + msCardSize.x  ) / 2 , 0 ) );
		setSize( Vec2i( msCardSize.x + NextOffset * ( mClinetCards->size() - 1 ) , msCardSize.y + SelectOffect )  );
		clearSelect();

		mCurCardNum = mClinetCards->size();
	}

	void CardListUI::onRender()
	{
		Vec2i pos = getWorldPos();

		if ( mHaveAnim )
		{
			for( int i = 0 ; i < (int)mClinetCards->size() ; ++i )
			{
				msCardDraw->draw( ::Global::GetGraphics2D() , pos + Vec2i( mCardPosOffset + mCardPos[i] , SelectOffect )  , (*mClinetCards)[i] );
			}
		}
		else
		{
			for( int i = 0 ; i < (int)mClinetCards->size() ; ++i )
			{
				msCardDraw->draw( ::Global::GetGraphics2D() , pos + Vec2i( 0 , SelectOffect - (int)mCardSelectOffset[i]  ) , (*mClinetCards)[i] );
				pos.x += NextOffset;
			}
		}

	}

	bool CardListUI::onMouseMsg( MouseMsg const& msg )
	{
		if ( mHaveAnim || mClinetCards->size() == 0 )
		{
			return true;
		}

		if ( msg.onLeftDown() || msg.onLeftDClick() )
		{
			Vec2i localPos = msg.getPos() - getWorldPos();
			int index = localPos.x / NextOffset;
			if ( index >= mClinetCards->size() )
				index = mClinetCards->size() - 1;
			if ( mbeSelected[ index ] )
			{
				if ( localPos.y > msCardSize.y )
					return false;
			}
			else
			{
				if ( localPos.y < SelectOffect )
				{
					return true;
				}
			}
			mbeSelected[ index ] = toggleCardSelect( index , mbeSelected[ index ] );

			if ( onChangeIndexSelected )
				onChangeIndexSelected( mIndexSelected , mNumSelected );

			return false;
		}
		else if ( msg.onRightDown() )
		{

		}

		return false;
	}

	void CardListUI::setScene( Scene& scene )
	{
		msScene    = &scene;
		msCardDraw = &scene.getCardDraw();
		msCardSize = scene.getCardSize();
	}

	void CardListUI::clearSelect()
	{
		for( int i = 0 ; i < mNumSelected ; ++i )
		{
			addUnselectAnim( mIndexSelected[i] );
		}
		std::fill_n( mbeSelected , 13 , false );
		mNumSelected = 0;
	}

	int* CardListUI::getSelcetIndex( int& num )
	{
		num = mNumSelected;
		return mIndexSelected;
	}

	bool CardListUI::toggleCardSelect( int index , bool beSelected )
	{
		if ( beSelected )
		{
			for( int i = 0 ; i < mNumSelected ; ++i )
			{
				if ( mIndexSelected[i] != index )
					continue;
				addUnselectAnim( index );
				if ( i != mNumSelected - 1 )
				{
					mIndexSelected[i] = mIndexSelected[ mNumSelected - 1 ];
				}
				mNumSelected -= 1;
				
				return false;
			}
		}

		if ( mNumSelected == MaxNumSelected )
			return false;

		addSelectAnim( index );
		mIndexSelected[ mNumSelected ] = index;
		mNumSelected += 1;
		
		return true;
	}

	void CardListUI::selectIndex( int pIndex[] , int num )
	{
		bool prevSelect[13];
		std::copy( mbeSelected , mbeSelected + mClinetCards->size() , prevSelect );

		std::fill_n( mbeSelected , 13 , false );
		mNumSelected = num;
		for( int i = 0 ; i < num ; ++i )
		{
			int idx = pIndex[i];
			mIndexSelected[i] = idx;
			mbeSelected[ idx ] = true;
			if ( !prevSelect[ idx ] )
				addSelectAnim( idx );
		}
		for( int idx = 0 ; idx < mClinetCards->size() ; ++idx )
		{
			if ( prevSelect[idx] && !mbeSelected[idx] )
				addUnselectAnim( idx );
		}

		if ( onChangeIndexSelected )
			onChangeIndexSelected( mIndexSelected , mNumSelected );
	}

	void CardListUI::removeSelected()
	{
		int const AnimTime = 500;

		if ( mNumSelected )
		{
			std::sort( mIndexSelected , mIndexSelected + mNumSelected );
			int count = 0;
			for( int i = 0 ; i < mCurCardNum ; ++i )
			{
				if( i >= mClinetCards->size() )
					break;

				if ( count < mNumSelected && mIndexSelected[ count ] == i )
					++count;

				if ( count )
				{
					msScene->getTweener().tweenValue< Easing::IOCubic >( mCardPos[i] , float( ( i + count ) * NextOffset ) , float( i * NextOffset ) , AnimTime );
				}
				else
				{
					mCardPos[i] = float( i * NextOffset );
				}
			}
			if ( count )
			{
				msScene->getTweener().tweenValue< Easing::IOCubic >( mCardPosOffset , 0.0f , float( ( mCurCardNum - mClinetCards->size() ) * NextOffset * 0.5f ) , AnimTime )
					.finishCallback( std::tr1::bind( &CardListUI::animFinish , this ) );
				mHaveAnim = true;
			}
		}

		mNumSelected = 0;
		std::fill_n( mCardSelectOffset , 13 , 0 );
	}

	void CardListUI::animFinish()
	{
		refreshCards();
		mHaveAnim = false;
	}

	void CardListUI::addSelectAnim( int index )
	{
		msScene->getTweener().tweenValue< Easing::OQuart >( mCardSelectOffset[ index ] , 0.0f , float( SelectOffect )  , 100 );
	}

	void CardListUI::addUnselectAnim( int index )
	{
		msScene->getTweener().tweenValue< Easing::OQuart >( mCardSelectOffset[ index ]  , float( SelectOffect ) , 0.0f , 100 );
	}

}//namespace Big2
}//namespace Poker