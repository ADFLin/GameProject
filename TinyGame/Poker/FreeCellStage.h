#ifndef FreeCellStage_h__
#define FreeCellStage_h__

#include "StageBase.h"
#include "FreeCell.h"
#include "CardDraw.h"
#include "Tween.h"

#include "GameRenderSetup.h"

namespace Poker
{

	class FreeCellStage : public StageBase
		                , private FreeCellLevel
		                , public  ICardResourceSetup
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
		MsgReply onMouse( MouseMsg const& msg ) override;
		bool onWidgetEvent( int event , int id , GWidget* ui ) override;

		bool preMoveCard( Cell& from , Cell& to , int num );
		void checkMoveCard( bool beAutoMove );

		Vec2i calcCardPos( Cell& cell );
		struct Sprite
		{
			Vector2 pos;
			bool    bAnimating;
		};

		void undoMove();
		void onAnimFinish(int idx, int animType, int metaValue);
		int  clickCard(StackCell& cell, Vec2i const& pos);
		void drawCell( IGraphics2D& g , Vec2i const& pos );
		void drawSprite( IGraphics2D& g , StackCell& cell );
		void drawSprite( IGraphics2D& g , Card const& card );
		void drawSelectRect( IGraphics2D& g );

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
		bool       playAnimation(  Card const& card , Vec2i const& to , float delay = 0.0f , int animType = ANIM_MOVEING );
		Cell*      mSelectCell;
		ICardDraw* mCardDraw = nullptr;
		Vec2i      mCardSize;
		typedef Tween::GroupTweener< float > Tweener;
		Tweener    mTweener;
		int        mSeed;
		bool       mbShowAnimation;
		Cell*      mCellLook;
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

		//ICardResourceSetup
		void setupCardDraw(ICardDraw* cardDraw) override;

	};

}//namespace Poker

#endif // FreeCellStage_h__
