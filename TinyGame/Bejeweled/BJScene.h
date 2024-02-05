#ifndef BJScene_h__
#define BJScene_h__

#include "BJLevel.h"

#include "Tween.h"
#include "RHI/RHICommon.h"
#include "RHI/RHICommand.h"

class MouseMsg;

namespace Bejeweled
{
	using namespace Render;

	namespace ETextureId
	{
		enum Type
		{
			GemAdd,
			BigStar,
			Selector,
			HyperGem,
			Rock,
			//UI
			Frame,
			ScorePod,

			COUNT,
		};
	}

	class Scene : public Level::Listener
	{
	public:
		Scene();

		static long const TimeSwapGem    = 200;
		static long const TimeDestroyGem = 200;
		static long const TimeFillGem    = 300;
		static int const CellLength = 84;
		static int const BoardSize = Level::BoardSize;

		bool initializeRHI();

		void  restart();
		void  tick();

		void  evoluteState();

		void  updateFrame(int frame);
		void  render( IGraphics2D& g );
		void  renderDebug(IGraphics2D& g);
		void  renderRHI(RHIGraphics2D& g);

		void  swapGem( int idx1 , int idx2 );

		void  setBoardPos( Vec2i const& pos ){ mBoardPos = pos; }
		Vec2i getCellPos( int idx )
		{
			return Vec2i( idx % Level::BoardSize , idx / Level::BoardSize );
		}

		bool bAutoPlay = false;
		bool bDrawDebugForce = false;

		//FIXME : move to GameControllor
	public:
		MsgReply procMouseMsg( MouseMsg const& msg );
		MsgReply procKey( KeyMsg const& msg );
	
	public:
		enum ControlMode
		{
			CM_CHOICE_GEM1 ,
			CM_CHOICE_GEM2 ,
		};
		ControlMode mCtrlMode;
		int         mIndexSwapCell[2];

		Level&        getLevel() { return mLevel; }

	private:
		//Level::listener
		void          onDestroyGem( int idx , GemData type );
		void          onMoveDownGem( int idxFrom , int idxTo );
		void          onFillGem( int idx , GemData type );

		void          addGemAnim( int idx , Vec2i const& from , Vec2i const& to , long time );
		void          removeAllGemAnim();
		void          updateAnim( long dt );

		template< typename TFunc >
		static void  VisitPosibleSwapPair(Level& level , TFunc&& func)
		{
			for (int i = 0; i < Level::BoardStorageSize; ++i)
			{
				int idx2;
				idx2 = Level::getAdjoinedIndex(i, Level::eRIGHT);
				if ((i / BoardSize) == (idx2 / BoardSize))
				{
					if (level.testSwapPairHaveRemove(i, idx2))
						func(i, idx2);
				}
				idx2 = Level::getAdjoinedIndex(i, Level::eDOWN);
				if (idx2 < Level::BoardStorageSize)
				{
					if (level.testSwapPairHaveRemove(i, idx2))
						func(i, idx2);
				}
			}
		}

		ActionContext mActionContext;

		struct GemFadeOut
		{
			GemData type;
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
			Vec2i offset = pos - mBoardPos;
			Vec2i result;
			result.x = Math::ToTileValue(offset.x, CellLength);
			result.y = Math::ToTileValue(offset.y, CellLength);
			return result;
		}

		void drawGem( IGraphics2D& g , Vector2 const& pos, GemData type);
		void drawGem( RHIGraphics2D& g, Vector2 const& pos, GemData type, int layer);
		Tween::GroupTweener< float > mTweener;
		int         mTurnCount;

		Vec2i       mBoardPos;
		Vec2i       mMoveCell;
		bool        mIsOver;
		Level       mLevel;
		int         mHoveredIndex = INDEX_NONE;

		static constexpr int GemImageCount = 7;

		RHITexture2DRef mTexGems[GemImageCount];
		RHITexture2DRef mTexMap[ETextureId::COUNT];

		bool loadBackground();
		TArray< RHITexture2DRef > mBGTexturs;
		int mIndexBG = 0;
	};


}//namespace Bejeweled
#endif // BJScene_h__
