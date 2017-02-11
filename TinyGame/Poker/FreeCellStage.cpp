#include "TinyGamePCH.h"
#include "FreeCellStage.h"

#include "DrawEngine.h"
#include "RenderUtility.h"

#include "GameGUISystem.h"
#include "WidgetUtility.h"

#include "EasingFun.h"
#include <functional>

namespace Poker
{

	inline bool isInRect( Vec2i const& pos , Vec2i const& rPos , Vec2i const& rSize )
	{
		return rPos.x <= pos.x && pos.x < rPos.x + rSize.x &&
		       rPos.y <= pos.y && pos.y < rPos.y + rSize.y;
	}

	int const SCellCardOffsetY = 25;
	int const GCellGap = 13;
	int const FCellGap = GCellGap;
	int const SCellGap = 10;

	Vec2i const FCellStartPos( 30 , 50 );
	Vec2i const GCellStartPos( 440 , 50 );
	Vec2i const SCellStartPos( 70 , 170 );


	class ChioceGamePanel : public GPanel
	{
		typedef GPanel BaseClass;
	public:
		ChioceGamePanel( int id , Vec2i const& pos , GWidget* parent )
			:GPanel( id , pos , Vec2i( 200 , 60 ) , parent )
		{

			GTextCtrl* textCtrl = new GTextCtrl( UI_SEED_TEXT , Vec2i( 10 , 10 ) , 180 , this );
			GButton* button;
			Vec2i btnSize( 80 , 20 );

			button = new GButton( UI_CANCEL , Vec2i( 15 , 35 ) , btnSize , this ); 
			button->setTitle( "Cancel" );

			button = new GButton( UI_OK , Vec2i( 20 + btnSize.x , 35 ) , btnSize , this ); 
			button->setTitle( "OK" );
		}

		enum
		{
			UI_OK  = BaseClass::NEXT_UI_ID,
			UI_CANCEL ,
			UI_SEED_TEXT ,
		};
		bool onChildEvent( int event , int id , GWidget* ui )
		{  
			switch( id )
			{
			case UI_OK:
				{
					GTextCtrl* textCtrl = GUI::castFast< GTextCtrl* >( findChild( UI_SEED_TEXT ) );
					int seed = atoi( textCtrl->getValue() );
					if ( seed )
					{
						stage->mSeed = seed;
						stage->restart();
						destroy();
					}
				}
				break;
			case UI_CANCEL:
				{
					destroy();
				}
				break;
			}
			return true; 
		}
		FreeCellStage* stage;
	};

	bool FreeCellStage::onInit()
	{ 
		mCardDraw = ICardDraw::create( ICardDraw::eWin7 );
		mCardSize = mCardDraw->getSize();

		mSeed  = 0;

		::Global::GUI().cleanupWidget();
		DevFrame* frame = WidgetUtility::createDevFrame();
		frame->addButton( UI_RESTART_GAME , "Restart Game" );
		frame->addButton( UI_NEW_GAME     , "New Game" );
		frame->addButton( UI_CHIOCE_GAME  , "Choice Game" );

		restart();

		return true; 
	}

	void FreeCellStage::onEnd()
	{
		mCardDraw->release();
	}

	void FreeCellStage::onUpdate( long time )
	{
		mTweener.update( time );
	}

	void FreeCellStage::onRender( float dFrame )
	{
		Graphics2D& g = Global::getGraphics2D();

		g.setBrush( ColorKey3( 0 , 125 , 0 ) );
		g.setPen( ColorKey3( 0 , 125 , 0 ) );
		g.drawRect( Vec2i(0,0) , Global::getDrawEngine()->getScreenSize() );

		mNumAnim = 0;

		if ( mSelectCell && !isStackCell( *mSelectCell ) )
		{
			drawSelectRect(g);
		}

		Vec2i pos;
		pos = FCellStartPos;
		for( int i = 0 ; i < FCellNum ; ++i )
		{
			Cell& cell = getCell( FCellIndex + i );
			drawCell( g , pos );
			if ( !cell.isEmpty() )
			{
				Card card = cell.getCard();
				drawSprite( g , card );
			}
			pos.x += FCellGap + mCardSize.x;
		}

		pos = GCellStartPos;
		for( int i = 0 ; i < 4 ; ++i )
		{
			Cell& cell = getCell( GCellIndex + i );
			drawCell( g , pos );
			if ( !cell.isEmpty() )
			{
				Card card = cell.getCard();
				int idx = card.getIndex();
				if ( mSprites[ idx ].beAnim && card.getFace() != Card::eACE )
				{
					mCardDraw->draw( g , pos , Card( card.getSuit() , card.getFaceRank() - 1 ) );
				}
				drawSprite( g , card );
			}
			pos.x += GCellGap + mCardSize.x;
		}

		for( int i = 0 ; i < SCellNum ; ++i )
		{
			SCell& cell = getSCell( i );
			drawSprite( g , cell );
		}

		for( int i = 0 ; i < mNumAnim ; ++i )
		{
			int idx = mIndexAnim[i];
			mCardDraw->draw( g , Vec2i( mSprites[ idx ].pos ) , Card( idx ) );
		}

	}

	void FreeCellStage::drawSelectRect( Graphics2D &g )
	{
		assert( mSelectCell );
		Vec2i pos = calcCardPos( *mSelectCell );

		if ( isStackCell( *mSelectCell ) )
			pos.y -= SCellCardOffsetY;

		g.setBrush( ColorKey3( 255 , 255 , 0 ) );
		RenderUtility::setPen( g , Color::eNull );
		Vec2i bSize( 3 , 3 );
		g.drawRoundRect( pos - bSize  , mCardSize + 2 * bSize + Vec2i( 1 , 1 ) , Vec2i( 5 , 5 ) );
	}

	void FreeCellStage::drawSprite( Graphics2D& g , SCell& cell )
	{
		int num = cell.getCardNum();
		for( int i = 0 ; i < num - 1 ; ++i )
		{
			Card card = cell.getCard( i );
			drawSprite( g , card );
		}
		if ( num )
		{
			if ( mSelectCell == &cell )
			{
				drawSelectRect( g );
			}
			drawSprite( g , cell.getCard( num - 1 ) );
		}
	}

	void FreeCellStage::drawSprite( Graphics2D& g , Card const& card )
	{
		int index = card.getIndex();
		Sprite& spr = mSprites[ index ];
		if ( spr.beAnim )
		{
			mIndexAnim[ mNumAnim ] = index;
			++mNumAnim;
		}
		else
		{
			mCardDraw->draw( g , Vec2i( spr.pos ) , card );
		}
	}

	Cell* FreeCellStage::clickCell( Vec2i const& pos )
	{
		int index;
		index = getGroupCellIndex( pos , SCellStartPos , SCellNum  , SCellGap  , 500 );
		if ( index != -1 )
			return &getCell( SCellIndex + index );
		index = getGroupCellIndex( pos , FCellStartPos , FCellNum  , FCellGap  , mCardSize.y );
		if ( index != -1 )
			return &getCell( FCellIndex + index );
		index = getGroupCellIndex( pos , GCellStartPos , 4  , GCellGap  , mCardSize.y );
		if ( index != -1 )
			return &getCell( GCellIndex + index );

		return NULL;
	}

	int FreeCellStage::clickCard( SCell& cell , Vec2i const& pos )
	{
		for( int idx = cell.getCardNum() - 1 ; idx >= 0 ; --idx )
		{
			Card const& card = cell.getCard( idx );
			if ( isInRect( pos , mSprites[ card.getIndex() ].pos , mCardSize ) )
				return idx;
		}
		return -1;
	}

	int FreeCellStage::getGroupCellIndex( Vec2i const& pos , Vec2i const& sPos , int numCell , int gap , int height )
	{
		Vec2i size = Vec2i( mCardSize.x * numCell + gap * ( numCell - 1 ) , height );
		if ( isInRect( pos , sPos , size ) )
		{
			int tx = ( pos.x - sPos.x );
			if ( tx % ( mCardSize.x + gap ) < mCardSize.x )
				return  tx / ( mCardSize.x + gap );
		}
		return -1;
	}

	bool FreeCellStage::onMouse( MouseMsg const& msg )
	{
		if ( msg.onLeftDClick() )
		{
			Cell* cell = clickCell( msg.getPos() );
			if ( cell )
			{
				if ( ( isStackCell( *cell ) && tryMoveToFCell( static_cast< SCell& >( *cell ) ) ) ||
					 ( isFreeCell( *cell ) && tryMoveToSCell( static_cast< FCell& >( *cell ) ) ) )
				{
					++mMoveStep;
				}
			}
			mSelectCell = NULL;
		}
		else if ( msg.onLeftDown() )
		{
			if ( mSelectCell )
			{
				Cell* to = clickCell( msg.getPos() );
				if ( to )
				{
					if ( mSelectCell != to )
					{
						if ( tryMoveCard( *mSelectCell , *to ) )
						{
							mSelectCell = NULL;
							++mMoveStep;
						}
					}
					else
					{
						mSelectCell = NULL;
					}
				}
			}
			else
			{
				Cell* cell = clickCell( msg.getPos() );
				if ( cell && cell->getType() != Cell::eGOAL && !cell->isEmpty() )
				{
					mSelectCell = cell;
				}
			}
		}
		else if ( msg.onRightUp() )
		{
			if ( mIdxCellLook != -1 )
			{
				SCell& lookCell = static_cast< SCell& >( getCell( mIdxCellLook ) );
				for( int i = mIdxCardLook + 1 ; i < lookCell.getCardNum() ; ++i )
				{
					Card const& card = lookCell.getCard( i );
					addAnim( card , mSprites[ card.getIndex() ].pos - Vec2f( 0 , 10 ) );
				}
				mIdxCellLook = -1;
				mIdxCardLook = -1;
			}
		}
		else if ( msg.onRightDown() )
		{
			Cell* cell = clickCell( msg.getPos() );
			if ( cell && isStackCell( *cell ) )
			{
				SCell& lookCell = static_cast< SCell& >(*cell);
				int idx = clickCard( lookCell , msg.getPos() );
				if ( idx != -1 )
				{
					for( int i = idx + 1 ; i < lookCell.getCardNum() ; ++i )
					{
						Card const& card = lookCell.getCard( i );
						addAnim( card , mSprites[ card.getIndex() ].pos + Vec2f( 0 , 15 ) , 0 , ANIM_LOOK );
					}
					//mIdxCellLook = cell->getIndex();
					//mIdxCardLook = idx;
				}
			}
		}
		else if ( msg.onRightDClick() )
		{
			undoMove();
			mSelectCell = NULL;
		}
		return false;
	}

	void FreeCellStage::drawCell( Graphics2D& g , Vec2i const& pos )
	{
		RenderUtility::setPen( g , Color::eBlack );
		RenderUtility::setBrush( g , Color::eNull );

		Vec2i border( 3 , 3 );
		g.drawRoundRect( pos - border , mCardSize + 2 * border , Vec2i( 8 , 8 ) );
	}

	bool FreeCellStage::onWidgetEvent( int event , int id , GWidget* ui )
	{
		switch( id )
		{
		case UI_RESTART_GAME:
			restart();
			return false;
		case UI_CHIOCE_GAME:
			{
				ChioceGamePanel* panel = new ChioceGamePanel( UI_ANY , Vec2i(0,0) , NULL );
				Vec2i pos = GUISystem::calcScreenCenterPos( panel->getSize() );
				panel->stage = this;
				panel->setPos( pos );
				::Global::GUI().addWidget( panel );
				panel->doModal();
			}
			return false;
		case UI_NEW_GAME:
			if ( event == EVT_BOX_YES )
			{
				newGame();
				return false;
			}
			else if ( event == EVT_BOX_NO )
			{

			}
			else
			{
				newGame();
				return false;
			}
			break;
		}
		return true;
	}

	void FreeCellStage::restart()
	{
		FreeCell::setupGame( mSeed );
		mSelectCell = NULL;
		mMoveStep   = 0;
		mMoveInfoVec.clear();
		mbUndoMove  = false;
		mIdxCardLook = -1;
		mIdxCellLook = -1;
		mState = STATE_PLAYING;

		mTweener.clear();

		for( int i = 0 ; i < SCellNum ; ++i )
		{
			SCell& cell = getSCell( i );
			for( int n = 0 ; n < cell.getCardNum() ; ++n )
			{
				int idx = cell.getCard( n ).getIndex();
				mSprites[ idx ].pos = SCellStartPos + Vec2i( i * ( SCellGap + mCardSize.x ) , n * SCellCardOffsetY );
				mSprites[ idx ].beAnim = false;
			}
		}
	}

	Vec2i FreeCellStage::calcCardPos( Cell& cell )
	{
		Vec2i pos; 
		int idx;
		int gap;
		switch( cell.getType() )
		{
		case Cell::eFREE: 
			pos = FCellStartPos; idx = FCellIndex; gap = FCellGap; 
			break;
		case Cell::eGOAL: 
			pos = GCellStartPos; idx = GCellIndex; gap = GCellGap; 
			break;
		case Cell::eSTACK: 
			pos = SCellStartPos; idx = SCellIndex; gap = SCellGap;
			pos.y += ( static_cast< SCell& >( cell ).getCardNum() ) * SCellCardOffsetY;
			break;
		}
		pos.x += ( cell.getIndex() - idx ) * ( gap + mCardSize.x );

		return pos;
	}

	bool FreeCellStage::preMoveCard( Cell& from , Cell& to , int num )
	{
		if ( !mbUndoMove && !to.isEmpty() )
		{
			int idx = to.getCard().getIndex();
			if ( mSprites[ idx ].beAnim )
				return false;
		}

		Vec2i endPos = calcCardPos( to );

		if ( num > 1 )
		{
			assert( isStackCell( from ) && isStackCell( to ) );

			if ( mSprites[ from.getCard().getIndex() ].beAnim )
				return false;

			SCell& fCell = static_cast< SCell& >( from );
			int idx = fCell.getCardNum() - num;
			float delay = 0;
			for( int i = 0 ; i < num ; ++i )
			{
				bool result = addAnim( fCell.getCard( idx + i ) , endPos , delay );
				assert( result );
				endPos.y += SCellCardOffsetY;
				delay += 40;
			}
		}
		else 
		{
		   if ( !addAnim( from.getCard() , endPos ) )
			   return false;
		}

		if ( !mbUndoMove )
		{
			MoveInfo info;
			info.from = from.getIndex();
			info.to   = to.getIndex();
			info.num  = num;
			info.step = mMoveStep + 1;
			mMoveInfoVec.push_back( info );
		}
		return true;
	}

	void FreeCellStage::checkMoveCard( bool beAutoMove )
	{
		while( beAutoMove && autoMoveToGCell() ){}

		if ( mTweener.getActiveNum() == 1 )
		{
			if ( getGCardNum() == 52 )
			{
				::Global::GUI().showMessageBox( UI_NEW_GAME , "You Win! Do You Play Again?" , GMB_YESNO );
				return;
			}

			int numPossible = calcPossibleMoveNum();
			if ( numPossible  == 0 )
			{
				::Global::GUI().showMessageBox( UI_NEW_GAME , "You Lose! Do You Give up This Game?" , GMB_YESNO );
			}
			else if( numPossible == 1 )
			{


			}
		}
	}

	bool FreeCellStage::addAnim( Card const& card , Vec2i const& to , float delay , int type )
	{
		typedef Easing::IOCubic MyFun;
		int idx = card.getIndex();
		Sprite& spr = mSprites[ idx ];

		if ( spr.beAnim )
			return false;

		switch ( type )
		{
		case ANIM_MOVEING:
			{
				float time;
				if ( mbUndoMove )
					time = 300;
				else
					time = 150 + 0.4 * sqrt( ( spr.pos - Vec2f( to ) ).length2() );

				mTweener.tweenValue< MyFun >( spr.pos , spr.pos , Vec2f( to ) ,  time )
					.delay( delay ).finishCallback( std::tr1::bind( &FreeCellStage::onAnimFinish , this , idx , mbUndoMove ) );
			}
			break;
		case ANIM_LOOK:
			{
				mTweener.tweenValue< Easing::COCubic >( spr.pos , spr.pos , Vec2f( to ) ,  300 )
					.delay( delay ).finishCallback( std::tr1::bind( &FreeCellStage::onAnimFinish , this , idx , true ) );
			}
			break;
		}

		spr.beAnim = true;
		return true;
	}

	void FreeCellStage::onAnimFinish( int idx , bool bUndoMove )
	{
		Sprite& spr = mSprites[ idx ];
		spr.beAnim = false;
		if ( bUndoMove )
		{
			if ( mbUndoMove )
				undoMove();
		}
		else
		{
			checkMoveCard( !bUndoMove );
		}
	}

	void FreeCellStage::newGame()
	{
		mSeed = (int)generateRandSeed();
		restart();
	}

	void FreeCellStage::undoMove()
	{
		mbUndoMove = true; 

		bool isOk = true;

		while ( !mMoveInfoVec.empty() )
		{
			MoveInfo& info = mMoveInfoVec.back();

			if ( info.step < mMoveStep )
				break;

			if ( !processMoveCard( getCell( info.to ) , getCell( info.from ) , info.num ) )
			{
				isOk = false;
				break;
			}
			mMoveInfoVec.pop_back();
		}

		if ( isOk )
		{
			--mMoveStep;
			mbUndoMove = false;
		}
	}

}//namespace Poker