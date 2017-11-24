#ifndef BJScene_h__
#define BJScene_h__

#include "BJLevel.h"

#include "Tween.h"

class MouseMsg;

namespace Bejeweled
{

	class Scene : public Level::Listener
	{
	public:
		Scene();

		static long const TimeSwapGem    = 200;
		static long const TimeDestroyGem = 200;
		static long const TimeFillGem    = 300;
		static int const CellLength = 50;
		static int const BoardSize = Level::BoardSize;

		void  restart();
		void  tick();
		void  updateFrame( int frame );
		void  render( Graphics2D& g );

		void  swapGem( int idx1 , int idx2 );

		void  setBoardPos( Vec2i const& pos ){ mBoardPos = pos; }
		Vec2i getCellPos( int idx )
		{
			return Vec2i( idx % Level::BoardSize , idx / Level::BoardSize );
		}

		//FIXME : move to GameControllor
	public:
		bool procMouseMsg( MouseMsg const& msg );
		bool procKey( unsigned key , bool isDown );
	private:
		enum ControlMode
		{
			CM_CHOICE_GEM1 ,
			CM_CHOICE_GEM2 ,
		};
		ControlMode mCtrlMode;
		int         mIndexSwapCell[2];

	private:
		//Level::listener
		DestroyMethod onFindSameGems( int idxStart , int num , bool beV );
		void          onDestroyGem( int idx , GemType type );
		void          onMoveDownGem( int idxFrom , int idxTo );
		void          onFillGem( int idx , GemType type );

		Level&        getLevel(){  return mLevel;  }

		void          addGemAnim( int idx , Vec2i const& from , Vec2i const& to , long time );
		void          removeAllGemAnim();
		void          updateAnim( long dt );

		struct GemFadeOut
		{
			int  type;
			int  index;
		};

		typedef TVector2< float > Vector2;
		struct GemMove
		{
			int   index;
			Vector2 pos;
			Vector2 vel;
			long  time;
		};

		struct IndexPair
		{
			int idx1 , idx2;
		};

		typedef std::vector< IndexPair > IndexPairVec;
		IndexPairVec mPosibleSwapPairVec;
		void addPosibleSwapPair( int idx1 , int idx2 );
		void removeAllPosibleSwapPair();

		void checkAllPosibleSwapPair();

		enum
		{
			RF_RENDER_ANIM  = BIT(0) ,
			RF_POSIBLE_SWAP = BIT(1),
		};
		unsigned mGemRenderFlag[ Level::BoardStorageSize ];
		int      mNumFill[ BoardSize ];

		typedef std::vector< GemMove > GemMoveVec;
		typedef std::vector< GemFadeOut > GemFadeOutVec;
		GemMoveVec    mGemMoveVec;
		GemFadeOutVec mGemFadeOutVec;
		float         mFadeOutAlpha;

		enum State
		{
			eChoiceSwapGem ,
			eSwapGem       ,
			eRestoreGem    ,
			eFillGem       ,
			eDestroyGem    ,
		};

		State      mState;
		long       mStateTime;

		void changeState( State state );


		Vec2i getCellPos( Vec2i const& pos )
		{
			return ( pos - mBoardPos ) / CellLength;
		}

		void drawGem( Graphics2D& g , Vec2i const& pos , int type );

		Tween::GroupTweener< float > mTweener;

		Vec2i       mBoardPos;
		Vec2i       mMoveCell;
		bool        mIsOver;
		Level       mLevel;
	};


}//namespace Bejeweled
#endif // BJScene_h__
