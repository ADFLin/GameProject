#include "TinyGamePCH.h"
#include "FreeCellStage.h"

#include "RenderUtility.h"

#include "GameGUISystem.h"
#include "Widget/WidgetUtility.h"

#include "EasingFunction.h"
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
		using BaseClass = GPanel;
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
					GTextCtrl* textCtrl = GUI::CastFast< GTextCtrl >( findChild( UI_SEED_TEXT ) );
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
		mSeed  = 0;
		::Global::GUI().cleanupWidget();
		DevFrame* frame = WidgetUtility::CreateDevFrame();
		frame->addButton( UI_RESTART_GAME , "Restart Game" );
		frame->addButton( UI_NEW_GAME     , "New Game" );
		frame->addButton( UI_CHIOCE_GAME  , "Choice Game" );

		//restart();
		return true; 
	}

	void FreeCellStage::onEnd()
	{

	}

	void FreeCellStage::onUpdate( long time )
	{
		mTweener.update( time );
	}

	void FreeCellStage::onRender( float dFrame )
	{
		IGraphics2D& g = ::Global::GetIGraphics2D();
		g.beginRender();

		g.setBrush( Color3ub( 0 , 125 , 0 ) );
		g.setPen( Color3ub( 0 , 125 , 0 ) );
		g.drawRect( Vec2i(0,0) , ::Global::GetScreenSize() );

		mNumAnim = 0;

		bool const needDrawSelection = mSelectCell && !isStackCell(*mSelectCell);
		if ( needDrawSelection )
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
				if ( mSprites[ idx ].bAnimating && card.getFace() != Card::eACE )
				{
					g.setBrush(Color3ub::White());
					mCardDraw->draw( g , pos , Card( card.getSuit() , card.getFaceRank() - 1 ) );
				}
				drawSprite( g , card );
			}
			pos.x += GCellGap + mCardSize.x;
		}

		for( int i = 0 ; i < SCellNum ; ++i )
		{
			StackCell& cell = getStackCell( i );
			drawSprite( g , cell );
		}

		if (mNumAnim)
		{
			g.setBrush(Color3ub::White());
			for (int i = 0; i < mNumAnim; ++i)
			{
				int idx = mIndexAnim[i];
				mCardDraw->draw(g, Vec2i(mSprites[idx].pos), Card(idx));
			}
		}

		g.endRender();
	}

	void FreeCellStage::drawSelectRect( IGraphics2D& g )
	{
		assert( mSelectCell );
		Vec2i pos = calcCardPos( *mSelectCell );

		if( isStackCell(*mSelectCell) )
		{
			if( mCellLook == mSelectCell )
			{
				pos = mSprites[mSelectCell->getCard().getIndex()].pos;
			}
			else
			{
				pos.y -= SCellCardOffsetY;
			}
		}

		g.setBrush( Color3ub( 255 , 255 , 0 ) );
		RenderUtility::SetPen( g , EColor::Null );
		Vec2i bSize( 3 , 3 );
		g.drawRoundRect( pos - bSize  , mCardSize + 2 * bSize + Vec2i( 1 , 1 ) , Vec2i( 5 , 5 ) );
	}

	void FreeCellStage::drawSprite( IGraphics2D& g , StackCell& cell )
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

	void FreeCellStage::drawSprite( IGraphics2D& g , Card const& card )
	{
		int index = card.getIndex();
		Sprite& spr = mSprites[ index ];
		if ( spr.bAnimating )
		{
			mIndexAnim[ mNumAnim ] = index;
			++mNumAnim;
		}
		else
		{
			g.setBrush(Color3ub::White());
			mCardDraw->draw( g , Vec2i( spr.pos ) , card );
		}
	}

	Cell* FreeCellStage::clickCell( Vec2i const& pos )
	{
		int index;
		index = getGroupCellIndex( pos , SCellStartPos , SCellNum  , SCellGap  , 500 );
		if ( index != INDEX_NONE)
			return &getCell( SCellIndex + index );
		index = getGroupCellIndex( pos , FCellStartPos , FCellNum  , FCellGap  , mCardSize.y );
		if ( index != INDEX_NONE)
			return &getCell( FCellIndex + index );
		index = getGroupCellIndex( pos , GCellStartPos , 4  , GCellGap  , mCardSize.y );
		if ( index != INDEX_NONE)
			return &getCell( GCellIndex + index );

		return nullptr;
	}

	int FreeCellStage::clickCard( StackCell& cell , Vec2i const& pos )
	{
		for( int idx = cell.getCardNum() - 1 ; idx >= 0 ; --idx )
		{
			Card const& card = cell.getCard( idx );
			if ( isInRect( pos , mSprites[ card.getIndex() ].pos , mCardSize ) )
				return idx;
		}
		return INDEX_NONE;
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
		return INDEX_NONE;
	}

	MsgReply FreeCellStage::onMouse( MouseMsg const& msg )
	{
		if ( msg.onLeftDClick() )
		{
			Cell* cell = clickCell( msg.getPos() );
			if ( cell )
			{
				bool haveMoved = false;
				if( isStackCell(*cell) )
				{
					if( tryMoveToGoalCell(static_cast<StackCell&>(*cell)) ||
					    tryMoveToFreeCell(static_cast<StackCell&>(*cell)) )
					{
						haveMoved = true;
					}

				}
				else if( isFreeCell(*cell) )
				{
					if( tryMoveToStackCell(static_cast<FreeCell&>(*cell)) )
					{
						haveMoved = true;
					}
				}

				if ( haveMoved )
				{
					++mMoveStep;
				}
			}
			mSelectCell = nullptr;
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
							mSelectCell = nullptr;
							++mMoveStep;
						}
					}
					else
					{
						mSelectCell = nullptr;
					}
				}
			}
			else
			{
				Cell* cell = clickCell( msg.getPos() );
				if ( cell && !isGoalCell( *cell ) && !cell->isEmpty() )
				{
					mSelectCell = cell;
				}
			}
		}
		else if ( msg.onRightUp() )
		{
#if 0
			if ( mIdxCellLook != -1 )
			{
				StackCell& lookCell = static_cast< StackCell& >( getCell( mIdxCellLook ) );
				for( int i = mIdxCardLook + 1 ; i < lookCell.getCardNum() ; ++i )
				{
					Card const& card = lookCell.getCard( i );
					playAnimation( card , mSprites[ card.getIndex() ].pos - Vector2( 0 , 10 ) );
				}
				mIdxCellLook = -1;
				mIdxCardLook = -1;
			}
#endif
		}
		else if ( msg.onRightDown() )
		{
			Cell* cell = clickCell( msg.getPos() );
			if ( cell && isStackCell( *cell ) )
			{
				StackCell& lookCell = static_cast< StackCell& >(*cell);
				int idx = clickCard( lookCell , msg.getPos() );
				if ( idx != -1 )
				{
					for( int i = idx + 1 ; i < lookCell.getCardNum() ; ++i )
					{
						Card const& card = lookCell.getCard( i );
						playAnimation( card , mSprites[ card.getIndex() ].pos + Vector2( 0 , 15 ) , 0 , ANIM_LOOK );
					}

					mCellLook = cell;
					mIdxCardLook = idx;
				}
			}
		}
		else if ( msg.onRightDClick() )
		{
			undoMove();
			mSelectCell = nullptr;
		}

		return BaseClass::onMouse(msg);
	}

	void FreeCellStage::drawCell( IGraphics2D& g , Vec2i const& pos )
	{
		RenderUtility::SetPen( g , EColor::Black );
		RenderUtility::SetBrush( g , EColor::Null );

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
				ChioceGamePanel* panel = new ChioceGamePanel( UI_ANY , Vec2i(0,0) , nullptr );
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
		FreeCellLevel::setupGame( mSeed );
		mSelectCell = nullptr;
		mMoveStep   = 0;
		mMoveInfoVec.clear();
		mbUndoMove  = false;
		mIdxCardLook = INDEX_NONE;
		mCellLook = nullptr;
		mState = STATE_PLAYING;

		mTweener.clear();

		for( int i = 0 ; i < SCellNum ; ++i )
		{
			StackCell& cell = getStackCell( i );
			for( int n = 0 ; n < cell.getCardNum() ; ++n )
			{
				int idx = cell.getCard( n ).getIndex();
				mSprites[ idx ].pos = SCellStartPos + Vec2i( i * ( SCellGap + mCardSize.x ) , n * SCellCardOffsetY );
				mSprites[ idx ].bAnimating = false;
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
			pos.y += ( static_cast< StackCell& >( cell ).getCardNum() ) * SCellCardOffsetY;
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
			if ( mSprites[ idx ].bAnimating )
				return false;
		}

		Vec2i endPos = calcCardPos( to );

		if ( num > 1 )
		{
			assert( isStackCell( from ) && isStackCell( to ) );

			if ( mSprites[ from.getCard().getIndex() ].bAnimating )
				return false;

			StackCell& fCell = static_cast< StackCell& >( from );
			int idx = fCell.getCardNum() - num;
			float delay = 0;
			for( int i = 0 ; i < num ; ++i )
			{
				bool result = playAnimation( fCell.getCard( idx + i ) , endPos , delay );
				assert( result );
				endPos.y += SCellCardOffsetY;
				delay += 40;
			}
		}
		else 
		{
		   if ( !playAnimation( from.getCard() , endPos ) )
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
		while( beAutoMove && moveToGoalCellAuto() ){}

		if ( mTweener.getActiveNum() == 1 )
		{
			if ( getGoalCardNum() == 52 )
			{
				::Global::GUI().showMessageBox( UI_NEW_GAME , "You Win! Do You Play Again?" , EMessageButton::YesNo );
				return;
			}

			int numPossible = calcPossibleMoveNum();
			if ( numPossible  == 0 )
			{
				::Global::GUI().showMessageBox( UI_NEW_GAME , "You Lose! Do You Give up This Game?" , EMessageButton::YesNo );
			}
			else if( numPossible == 1 )
			{


			}
		}
	}

	bool FreeCellStage::playAnimation( Card const& card , Vec2i const& to , float delay , int animType)
	{
		using MyFun = Easing::IOCubic;
		int idx = card.getIndex();
		Sprite& spr = mSprites[ idx ];

		if ( spr.bAnimating )
			return false;

		switch ( animType )
		{
		case ANIM_MOVEING:
			{
				float time;
				if ( mbUndoMove )
					time = 300;
				else
					time = 150 + 0.4 * sqrt( ( spr.pos - Vector2( to ) ).length2() );

				mTweener.tweenValue< MyFun >( spr.pos , spr.pos , Vector2( to ) ,  time )
					.delay( delay ).finishCallback( std::bind( &FreeCellStage::onAnimFinish , this , idx , animType , mbUndoMove ) );
			}
			break;
		case ANIM_LOOK:
			{
				mTweener.tweenValue< Easing::COCubic >( spr.pos , spr.pos , Vector2( to ) ,  300 )
					.delay( delay ).finishCallback( std::bind( &FreeCellStage::onAnimFinish , this , idx , animType , 0  ) );
			}
			break;
		}

		spr.bAnimating = true;
		return true;
	}

	void FreeCellStage::setupCardDraw(ICardDraw* cardDraw)
	{
		mCardDraw = cardDraw;
		mCardSize = cardDraw->getSize();
		restart();
	}

	void FreeCellStage::onAnimFinish(int idx, int animType, int metaValue)
	{
		Sprite& spr = mSprites[ idx ];
		spr.bAnimating = false;

		switch( animType )
		{
		case ANIM_MOVEING:
			{
				if( metaValue )
				{
					if( mbUndoMove )
						undoMove();
				}
				else
				{
					checkMoveCard( !metaValue );
				}
			}
			break;
		case ANIM_LOOK:
			{

			}
			break;
		default:
			break;
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