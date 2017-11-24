#ifndef FreeCellStage_h__
#define FreeCellStage_h__

#include "StageBase.h"
#include "FreeCell.h"
#include "CardDraw.h"
#include "Tween.h"

namespace Poker
{

	class FreeCellStage : public StageBase
		                , private FreeCellLevel
	{
		typedef StageBase BaseClass;
	public:
		void newGame();
		void restart();

		virtual bool onInit();
		virtual void onEnd();
		virtual void onUpdate( long time );


		virtual void onRender( float dFrame );

		enum
		{
			UI_NEW_GAME = BaseClass::NEXT_UI_ID ,
			UI_CHIOCE_GAME ,
			NEXT_UI_ID ,
		};


		Cell* clickCell( Vec2i const& pos );

		int getGroupCellIndex( Vec2i const& pos , Vec2i const& sPos , int numCell , int gap , int height );
		virtual bool onMouse( MouseMsg const& msg );
		virtual bool onWidgetEvent( int event , int id , GWidget* ui );

		bool preMoveCard( Cell& from , Cell& to , int num );
		void checkMoveCard( bool beAutoMove );

		Vec2i calcCardPos( Cell& cell );
		struct Sprite
		{
			Vector2 pos;
			bool  beAnim;
		};

		void undoMove();
		void onAnimFinish( int idx , bool bUndoMove );
		int  clickCard( StackCell& cell , Vec2i const& pos );
		void drawCell( Graphics2D& g , Vec2i const& pos );
		void drawSprite( Graphics2D& g , StackCell& cell );
		void drawSprite( Graphics2D& g , Card const& card );
		void drawSelectRect( Graphics2D& g );

		int        mNumAnim;
		int        mIndexAnim[ 52 ];
		Sprite     mSprites[ 52 ];

		enum
		{
			ANIM_MOVEING ,
			ANIM_LOOK ,
		};

		enum State
		{
			STATE_DEAL ,
			STATE_MOVE_CARD ,
			STATE_PLAYING ,
			STATE_END ,
		};
		State      mState;
		bool       playAnim(  Card const& card , Vec2i const& to , float delay = 0.0f , int type = ANIM_MOVEING );
		Cell*      mSelectCell;
		ICardDraw* mCardDraw;
		Vec2i      mCardSize;
		typedef Tween::GroupTweener< float > Tweener;
		Tweener    mTweener;
		int        mSeed;
		bool       mShowAnim;
		int        mIdxCellLook;
		int        mIdxCardLook;

		struct MoveInfo
		{
			int  step;
			int  from;
			int  to;
			int  num;
		};
		int  mMoveStep;
		bool mbUndoMove;
		std::vector< MoveInfo > mMoveInfoVec;
	};




}//namespace Poker

#endif // FreeCellStage_h__
