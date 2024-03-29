#ifndef Big2Scene_h__
#define Big2Scene_h__

#include "PokerBase.h"

#include "Big2Level.h"
#include "Big2Utility.h"

#include "GameWidget.h"
#include "Tween.h"
#include "CardDraw.h"

#include "FastDelegate/FastDelegate.h"
class Graphics2D;
class IPlayerManager;

namespace Big2 {

	class ClientLevel;
	class Scene;

	class CardListUI : public GWidget
	{
		typedef GWidget BaseClass;
	public:
		static int const SelectOffect = 10;
		static int const NextOffset   = 20;
		CardListUI( CardDeck& cards , Vec2i const& pos , GWidget* parent );
		void  setClientCards( CardDeck& cards );
		void  refreshCards();
		void  removeSelected();
		int*  getSelcetIndex( int& num );

		void  onRender();
		MsgReply  onMouseMsg(MouseMsg const& msg);

		void   selectIndex( int pIndex[] , int num );
		void   clearSelect();

		typedef fastdelegate::FastDelegate< void ( int* , int ) > Func;
		Func  onChangeIndexSelected;

		Vec2i  getBasePos(){ return mBasePos; }

		static void setScene( Scene& scene );

		Vec2i      mBasePos;
	private:
		static Vec2i      msCardSize;
		static ICardDraw* msCardDraw;
		static Scene*     msScene;

		void   animFinish();
		bool   toggleCardSelect( int index , bool beSelected );
		void   addSelectAnim( int index );
		void   addUnselectAnim( int index );

		int        mCurCardNum;
		bool       mHaveAnim;
		float      mCardPosOffset;
		float      mCardSelectOffset[ PlayerCardNum ];
		float      mCardPos[ PlayerCardNum ];

		CardDeck* mClinetCards;
		bool       mbSelectedMap[ PlayerCardNum ];
		static int const MaxNumSelected = PlayerCardNum;
		int        mIndexSelected[ MaxNumSelected ];
		int        mNumSelected;
	};

	class CardButton;
	class ActionButton;

	class Scene : public ClientLevel::Listener
	{
	public:
		Scene( ClientLevel& level , IPlayerManager& playerMgr );
		~Scene();

		void         setupUI();
		ICardDraw&   getCardDraw(){ return *mCardDraw; }
		ClientLevel& getLevel(){ return *mLevel; }

		typedef Tween::GroupTweener< float > Tweener;
		Tweener&     getTweener(){ return mTweener;  }

		void         showCard();
		void         clearSelect();
		void         nextCombination( CardGroup group );
		Vec2i const& getCardSize(){ return mCardSize; }
		void         setupCardDraw(ICardDraw* cardDraw);
		void         render( IGraphics2D& g );
		void         updateFrame( int frame );
		
		
		void         onGameInit();
		void         onRoundInit( SDRoundInit const& initData );
		void         onSlotTurn( int slotId , bool beShow );
		void         onNewCycle();
		void         onRoundEnd( SDRoundEnd const& endData );

		void         onListCardSelect( int* index , int num );

		void         refreshUI( bool haveChange );

		void         drawOtherPlayer( IGraphics2D& g , Vec2i const& pos , TablePos tPos , int numCard , bool beCenterPos = true , bool bDrawBack = true );
		void         drawShowCard( IGraphics2D& g , Vec2i const& pos , float angle ,  TrickInfo const& info , bool beFoucs );
		void         drawLastShowCard( IGraphics2D& g , Vec2i const& pos , bool beFoucs );
		void         drawSlotStatus( IGraphics2D& g , Vec2i const& pos , int slotId );

		enum RenderState
		{
			eDEAL_CARD ,
			ePLAYING ,
			eROUND_END ,
			eGAME_OVER ,
		};

		void         changeState( RenderState state );
		void         drawLastCardOnBitmap();
		
		IPlayerManager*   mPlayerManager;

		int               mStateTime;
		RenderState       mState;
		BitmapDC          mShowCardBmp;
		Vector2           mShowCardOffset[4];
		struct CardTrans
		{
			Vec2i offset;
		};
		CardTrans         mShowCardTrans[5];
		int               mDealCardNum;
		Vector2           mDealCardPos[4];

		int               mIdxCard[4];
		char              mCards[52];
		TrickIterator     mIterator;
		bool              mRestIterator;
		TrickHelper       mHelper;
		Vec2i             mCardSize;
		ICardDraw*        mCardDraw;
		ClientLevel*      mLevel;
		CardListUI*       mOwnCardsUI;
		ActionButton*     mButton[10];
		Tweener           mTweener;
	};

}//namespace Big2

#endif // Big2Scene_h__
