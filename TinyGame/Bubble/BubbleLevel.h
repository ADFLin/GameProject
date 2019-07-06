#ifndef BubbleLevel_h__
#define BubbleLevel_h__

#include "TVector2.h"
#include "Math/Vector2.h"
#include "Core/IntegerType.h"

using ::Math::Vector2;

#include <list>
#include <cassert>


#ifndef BIT
#define BIT( n ) ( 1 << ( n ) )
#endif 

namespace Bubble
{

	float const g_BubbleRadius = 15.0f;
	float const g_BubbleDiameter = 2 * g_BubbleRadius;
	int const g_BubbleColorNum = 8;

	enum LinkDir
	{
		LINK_TOP_RIGHT    ,
		LINK_TOP_LEFT     ,
		LINK_RIGHT        ,
		LINK_LEFT         ,
		LINK_BOTTOM_RIGHT ,
		LINK_BOTTOM_LEFT  ,

		NUM_LINK_DIR ,
	};

	class BubbleCell
	{
	private:
		friend class LevelCore;
		static unsigned const ColorMask = 0xff;
		static unsigned const FlagOffset = 4;
	public:
		static unsigned const Block = (unsigned)ColorMask;

		void empty(){ data = 0; }
		int  getColor() const { return color; }
		bool isBlock()  const { return data == Block; }
		bool isEmpty()  const { return data == 0; }
		void setColor( int c ){ assert( !isBlock() ); color = static_cast< unsigned char >( c ); }

		unsigned getFlag() const { return flag; }
		bool     checkFlag( unsigned bit ){ assert( !isBlock() ); return ( flag & bit ) != 0; }
		void     addFlag( unsigned bit )  { assert( !isBlock() ); flag |= ( bit );  }
		void     removeFlag( unsigned bit){ assert( !isBlock() ); flag &= ~( bit ); }

	private:
		friend class LevelCore;
		union
		{
			struct  
			{
				unsigned char color : 4;
				unsigned char flag  : 4;
			};
			unsigned char data;
		};
	};

	enum BubbleState
	{
		BUBBLE_FALL     = BIT(0),
		BUBBLE_POP      = BIT(1),
		BUBBLE_SHOOTING = BIT(2),
		BUBBLE_LOCK     = BIT(3),
	};

	class LevelCore
	{
	public:
		LevelCore();
		~LevelCore();

		void cleanup();

		BubbleCell& getCell( int layer , int nx ){ return mCellData[ layer * mNumCellLayer + nx ];}
		BubbleCell& getCell( int idx ){  return mCellData[idx];  }

		int   calcCellIndex( Vector2 const& pos , int& layer , int& nx );
		int   getLinkCellIndex( int idx , LinkDir dir , bool isEven );
		bool  isEvenLayer( int index ){  return ( index / mNumCellLayer ) % 2 == 0;  }

		Vector2 calcCellCenterPos( int index );
		Vector2 calcCellCenterPos( int layer , int nx );

		int16 const* getLastPopCellIndex() const { return mIndexPopCell; }
		int          getLastPopCellNum()   const { return mNumPopCell; }
		int16 const* getLastFallCellIndex()const { return mIndexFallCell; }
		int          getLastFallCellNum()  const { return mNumFallCell; }

		float        getTopOffset(){ return mTopOffset; }
		void         setTopOffset( float offset ){ mTopOffset = offset;  }
	protected:
		void        setupCell( int width , int layer );
		int         processCollision( Vector2 const& pos , Vector2 const& vel , int color );
		unsigned    lockBubble( int index , int color );
		bool        testFallBubble_R( int index );

		float  mTopOffset;
		int    mNumPopCell;
		int    mNumFallCell;
		int16* mIndexPopCell;
		int16* mIndexFallCell;


		struct PosCache* mPosCache;
	protected:
		int    mNumCellData;
		int    mNumLayer;
		int    mNumCellLayer;
		int    mNumFreeCellLayer;

		BubbleCell* mCellData;
		friend class Scene;

	private:

		enum
		{
			eCheckColor  = BIT(0),
			eMarkPop     = BIT(1),
			eTestFall    = BIT(2),
			eLinkWall    = BIT(3),

		};

		bool checkColFlag( int idx , uint8 bit ){ assert( !getCell( idx ).isBlock() ); return ( mColFlag[idx] & bit ) != 0; }
		void addColFlag  ( int idx , uint8 bit ){ assert( !getCell( idx ).isBlock() ); mColFlag[idx] |=  bit;  }

		uint8*  mColFlag;
	};

	class LevelListener;

	class Level : public LevelCore
	{
	public:
		Level( LevelListener* listener );

		struct Bubble
		{
			Vector2 pos;
			Vector2 vel;
			int   color;
			void  update( float dt ){  pos += dt * vel; }
		};
		void    generateRandomLevel( int maxLayer , int density );
		float   getMaxDepth(){ return mMaxDepth; }

	protected:
		void     update( float dt );
		void     shoot( Bubble* bubble );
		void     rotateRight( float delta );
		unsigned updateShootBubble( Bubble& bubble , float totalTime );
		typedef std::list< Bubble* > BubbleList;

		BubbleList mShootList;
		BubbleList mFallList;


		float mMaxDepth;

		int   mShootBufDelay;

		int   mShootBubbleColor;
		float mShootBubbleSpeed;
		float mShootBubbleTimeStep;

		float mLauncherAngle;
		Vector2 mLauncherDir;
		Vector2 mLauncherPos;

		LevelListener* mListener;
		friend class Scene;
	};

	class LevelListener
	{
	public:
		typedef Level::Bubble Bubble;
		virtual void onRemoveShootBubble( Bubble* bubble ) = 0;
		virtual void onUpdateShootBubble( Bubble* bubble , unsigned result ) = 0;
		virtual void onPopCell( BubbleCell& cell , int index ) = 0;
		virtual void onAddFallCell( BubbleCell& cell , int index ) = 0;
		Level* getLevel(){ return mLevel; }
	protected:
		friend class Level;
		Level* mLevel;
	};

}//namespace Bubble

#endif // BubbleLevel_h__