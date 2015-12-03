#include "BubblePCH.h"
#include "BubbleLevel.h"

#include "GameGlobal.h"

#include <cmath>

#define  SIN_60 ( 1.73205081f * 0.5f )

namespace Bubble
{
	int  const  g_MinBubbleDestroyNum = 2;
	float const g_MinCollisionDistance = g_BubbleDiameter - 0.5f;

	struct PosCache
	{
		Vec2f* data;
		int    refCount;
		int    numCellLayer;
	};

	PosCache* gCurPosCache = NULL;

	LevelCore::LevelCore()
	{
		mCellData      = NULL;
		mIndexPopCell  = NULL;
		mIndexFallCell = NULL;
		mColFlag       = NULL;
		mPosCache      = NULL;

		mNumPopCell  = 0;
		mNumFallCell = 0;
		mTopOffset   = 0.0f;
	}

	LevelCore::~LevelCore()
	{
		cleanup();
	}


	void LevelCore::setupCell( int width , int layer )
	{
		cleanup();

		mNumLayer = layer;
		mNumFreeCellLayer = width;
		mNumCellLayer     = width + 1;

		int num = mNumLayer * mNumCellLayer;
		mNumCellData = num;

		mIndexPopCell  = new short[ num ];
		mIndexFallCell = new short[ num ];
		mColFlag       = new unsigned char[ num ];

		mCellData = new BubbleCell[ num ];

		memset( mCellData , 0 , sizeof( BubbleCell ) * num );

		for( int i = 0 ; i < mNumCellLayer ; ++i )
			mCellData[i].data = BubbleCell::Block;

		for( int layer = 0 ; layer < mNumLayer ; ++layer )
		{
			int idx = layer * mNumCellLayer;
			mCellData[ idx ].data = BubbleCell::Block;
			if ( layer % 2  == 0 )
				mCellData[ idx + mNumCellLayer - 1 ].data = BubbleCell::Block;
		}

		if ( gCurPosCache == NULL || mNumCellLayer != gCurPosCache->numCellLayer )
		{
			gCurPosCache = new PosCache;
			gCurPosCache->numCellLayer = mNumCellLayer;
			gCurPosCache->refCount     = 0;
			gCurPosCache->data     = new Vec2f[ mNumCellData ];

			for( int i = 0 ; i < mNumCellData ; ++i )
			{
				int layer = i / mNumCellLayer;
				int nx    = i % mNumCellLayer;
				gCurPosCache->data[i].x = nx * g_BubbleDiameter - (layer % 2) * g_BubbleRadius;
				gCurPosCache->data[i].y = SIN_60 * float( layer - 1 ) * g_BubbleDiameter + g_BubbleRadius;
			}
		}

		mPosCache = gCurPosCache;
		mPosCache->refCount += 1;
	}

	void LevelCore::cleanup()
	{
		delete [] mCellData;
		mCellData = NULL;
		delete [] mIndexPopCell;
		mIndexPopCell = NULL;
		delete [] mIndexFallCell;
		mIndexFallCell = NULL;
		delete [] mColFlag;
		mColFlag = NULL;

		mNumLayer = 0;
		mNumFreeCellLayer = 0;
		mNumCellLayer     = 0;
		mNumCellData = 0;

		if ( mPosCache )
		{
			mPosCache->refCount -= 1;
			if ( mPosCache->refCount == 0 )
			{
				delete [] mPosCache->data;
				delete mPosCache;
			}
			mPosCache = NULL;
		}
	}


	Vec2f LevelCore::calcCellCenterPos( int layer , int nx )
	{
		return calcCellCenterPos( layer * mNumCellLayer + nx );
		//return Vec2f( nx * g_BubbleDiameter - (layer % 2) * g_BubbleRadius ,
		//	SIN_60 * float( layer - 1 ) * g_BubbleDiameter + g_BubbleRadius + mTopOffset );
	}

	Vec2f LevelCore::calcCellCenterPos( int index )
	{
		Vec2f out = mPosCache->data[index];
		out.y += mTopOffset;
		return out;
		//return calcCellCenterPos( index / mNumCellLayer , index % mNumCellLayer );
	}

	int LevelCore::calcCellIndex( Vec2f const& pos  , int& layer , int& nx )
	{
		assert( 0 <= pos.x && pos.x < mNumFreeCellLayer * g_BubbleDiameter );
		assert( 0 <= pos.y && pos.y < mNumLayer * g_BubbleDiameter );

		layer = int( float( pos.y - mTopOffset )  / ( g_BubbleDiameter * SIN_60 ) ) + 1;

		float x = pos.x + g_BubbleRadius;
		if ( layer % 2 )
			x += g_BubbleRadius;

		nx = int( x / g_BubbleDiameter );

		return layer * mNumCellLayer + nx;
	}

	int LevelCore::processCollision( Vec2f const& pos , Vec2f const& vel , int color )
	{
		int layer,nx;
		int index = calcCellIndex( pos , layer , nx );

		if ( layer >= mNumLayer - 1 )
			return -1;


		BubbleCell& cell = getCell( index );

		if ( cell.isBlock() )
			return -1;

		// In special condition , two bubbles are the same position and which one has locked early
		//	assert( cell.isEmpty() );
		if (  !cell.isEmpty() )
			return -1;


		Vec2f cellPos = calcCellCenterPos( layer , nx );

		if ( layer == 1  )
		{
			float const g_MinTopLayerLockDistance = 0.5f * g_BubbleRadius;
			Vec2f offset = pos - cellPos;
			if ( offset.length2() <  g_MinTopLayerLockDistance * g_MinTopLayerLockDistance ||
				vel.y > 0 )
				return index;
		}

		bool isEven = ( layer % 2 ) == 0;

		float dist2Test = g_MinCollisionDistance * g_MinCollisionDistance;
		float idxTest   = -1;
		for( int i = 0 ; i < NUM_LINK_DIR ; ++i )
		{
			int linkIdx = getLinkCellIndex( index , LinkDir(i) , isEven );

			BubbleCell& linkCell = getCell( linkIdx );

			if ( linkCell.isEmpty() || linkCell.isBlock() )
				continue;

			Vec2f linkCellPos = calcCellCenterPos( linkIdx );

			float dist2 = ( linkCellPos - pos ).length2();
			
			if ( dist2 < dist2Test )
			{
				idxTest = index;
				dist2Test = dist2;
			}
		}

		return idxTest;
	}

	int LevelCore::getLinkCellIndex( int idx , LinkDir dir , bool isEven )
	{
		assert( isEvenLayer( idx ) == isEven );
		assert( !getCell( idx ).isBlock() );

		switch ( dir )
		{
		case LINK_TOP_RIGHT:    return idx     - mNumCellLayer + int( isEven );
		case LINK_TOP_LEFT:     return idx - 1 - mNumCellLayer + int( isEven );
		case LINK_RIGHT:        return idx + 1;
		case LINK_LEFT:         return idx - 1;
		case LINK_BOTTOM_RIGHT: return idx     + mNumCellLayer + int( isEven );
		case LINK_BOTTOM_LEFT:  return idx - 1 + mNumCellLayer + int( isEven );
		}

		assert( 0 );
		return idx;
	}

	unsigned LevelCore::lockBubble( int index , int color )
	{
		BubbleCell& cell = getCell( index );
		assert( !cell.isBlock() );

		cell.setColor( color );

		memset( mColFlag , 0 , mNumCellData * sizeof( mColFlag[0]) );

		addColFlag( index , eCheckColor | eMarkPop );

		unsigned result = BUBBLE_LOCK;

		mIndexPopCell[0] = index;

		short* idxCur   = mIndexPopCell;
		short* idxPop   = mIndexPopCell;
		short* idxCheck = mIndexPopCell + mNumCellData;

		do
		{
			bool isEven = isEvenLayer( *idxCur );

			for( int i = 0 ; i < NUM_LINK_DIR ; ++i )
			{
				int linkIdx = getLinkCellIndex( *idxCur , LinkDir(i) , isEven );

				BubbleCell& linkCell = getCell( linkIdx );

				if ( linkCell.isBlock() || linkCell.isEmpty() ||
					checkColFlag( linkIdx , eCheckColor ) )
					continue;

				if ( linkCell.getColor() == color )
				{
					*( ++idxPop ) = linkIdx;
					addColFlag( linkIdx , eCheckColor | eMarkPop );
				}
				else
				{
					*( --idxCheck ) = linkIdx;
					addColFlag( linkIdx , eCheckColor );
				}
			}
			++idxCur;
		}
		while( idxPop >= idxCur );

		mNumPopCell = int( idxPop - mIndexPopCell ) + 1;

		if ( mNumPopCell <= g_MinBubbleDestroyNum )
		{
			mNumPopCell = 0;
			return result;
		}

		result |= BUBBLE_POP;

		short* idxEnd = mIndexPopCell + mNumCellData;

		int tempNumFallCell;
		tempNumFallCell = mNumFallCell = 0;

		while( idxCheck != idxEnd )
		{
			if ( testFallBubble_R( *(idxCheck++) ) )
				tempNumFallCell = mNumFallCell;
			else
				mNumFallCell = tempNumFallCell;
		}

		if ( mNumFallCell )
			result |= BUBBLE_FALL;

		return result;
	}

	bool LevelCore::testFallBubble_R( int index )
	{
		BubbleCell& cell = getCell( index );

		if ( checkColFlag( index , eLinkWall ) )
			return false;
		if ( checkColFlag( index , eTestFall | eMarkPop ) )
			return true;

		assert( !( cell.isEmpty() || cell.isBlock() ) );

		mIndexFallCell[ mNumFallCell++ ] = index;
		addColFlag( index , eTestFall );

		bool isEven = isEvenLayer( index );

		for( int i = 0 ; i < NUM_LINK_DIR ; ++i )
		{
			int linkIdx = getLinkCellIndex( index , LinkDir(i) , isEven );
			BubbleCell& linkCell = getCell( linkIdx );

			if ( linkCell.isEmpty() )
				continue;

			//odd layer always link wall when linking one block;
			// ( isBlock && !isEven ) ||
			// (!isBlock && !testFallBubble )
			if ( ( linkCell.isBlock() ? !isEven  : !testFallBubble_R( linkIdx )  ) )
			{
				addColFlag( index , eLinkWall );
				return false;
			}
		}
		return true;
	}

	Level::Level( LevelListener* listener )
	{
		mListener = listener;
		mListener->mLevel = this;

		int num = 10;
		mLauncherPos = Vec2f( num * g_BubbleDiameter / 2 , 15 * g_BubbleDiameter );

		mShootBubbleColor = Global::RandomNet() % g_BubbleColorNum + 1;
		mShootBubbleSpeed = 800.0f;
		mShootBubbleTimeStep = g_BubbleDiameter / ( gDefaultGameFPS * mShootBubbleSpeed );

		mLauncherAngle = 0;


		mMaxDepth = mLauncherPos.y + g_BubbleDiameter;
		setupCell( num , (int)(mMaxDepth / g_BubbleDiameter) + 5 );


		roteRight( 0 );
	}

	void Level::update( float dt )
	{

		for( BubbleList::iterator iter = mShootList.begin();
			iter != mShootList.end() ; )
		{
			Bubble* bubble = *iter;

			unsigned result = updateShootBubble( *bubble , dt );

			mListener->onUpdateShootBubble( bubble , result );

			if ( result & BUBBLE_FALL )
			{
				short const* index = getLastFallCellIndex();
				for( int i = 0 ; i < mNumFallCell; ++i )
				{
					int idx = index[i] ;
					BubbleCell& cell = getCell( idx );
					mListener->onAddFallCell( cell , idx );
					cell.empty();
				}
			}

			if ( result & BUBBLE_POP )
			{
				short const* index = getLastPopCellIndex();
				for( int i = 0 ; i < mNumPopCell ; ++i )
				{
					int idx = index[i] ;
					BubbleCell& cell = getCell( idx );
					mListener->onPopCell( cell , idx );
					cell.empty();
				}
			}

			if ( result != BUBBLE_SHOOTING || bubble->pos.y > mMaxDepth )
			{
				iter = mShootList.erase( iter );
				mListener->onRemoveShootBubble( bubble );
			}
			else
			{	
				++iter;
			}
		}
	}

	unsigned Level::updateShootBubble( Bubble& bubble , float totalTime )
	{
		bool done = false;

		unsigned result = BUBBLE_SHOOTING;
		int rightSide = int( mNumFreeCellLayer * g_BubbleDiameter - g_BubbleRadius );

		while( !done )
		{
			if ( totalTime > mShootBubbleTimeStep )
			{
				totalTime -= mShootBubbleTimeStep;
				bubble.update( mShootBubbleTimeStep );
			}
			else
			{
				done = true;
				bubble.update( totalTime );
			}

			if ( bubble.pos.x < g_BubbleRadius )
			{
				bubble.pos.x = 2 * g_BubbleRadius - bubble.pos.x;
				//bubble.pos.x = g_BubbleRadius;

				if ( bubble.vel.x < 0 )
					bubble.vel.x = -bubble.vel.x;
			}
			else if ( bubble.pos.x > rightSide )
			{
				bubble.pos.x = 2 * rightSide - bubble.pos.x;
				//bubble.pos.x = (float)rightSide;

				if ( bubble.vel.x > 0 )
					bubble.vel.x = -bubble.vel.x;
			}

			if ( bubble.pos.y < g_BubbleRadius )
			{
				bubble.pos.y = g_BubbleRadius;
				if ( bubble.vel.y < 0 )
					bubble.vel.y = -bubble.vel.y;
			}


			int index = processCollision( bubble.pos , bubble.vel , bubble.color );

			if ( index != -1 )
			{
				result = lockBubble( index , bubble.color );
			}

			if ( result != BUBBLE_SHOOTING )
				break;
		}
		return result;
	}

	void Level::shoot( Bubble* bubble )
	{
		bubble->vel   = mShootBubbleSpeed * mLauncherDir;
		bubble->pos   = mLauncherPos;
		bubble->color = mShootBubbleColor;
		mShootList.push_back( bubble );

		mShootBubbleColor = Global::RandomNet() % g_BubbleColorNum + 1;
	}

	void Level::roteRight( float delta )
	{
		static float const MaxBarrelAngle = DEG2RAD( 87 );
		mLauncherAngle += delta;
		if ( mLauncherAngle > MaxBarrelAngle )
			mLauncherAngle = MaxBarrelAngle;
		else if ( mLauncherAngle < -MaxBarrelAngle )
			mLauncherAngle = -MaxBarrelAngle;

		mLauncherDir.setValue( -sin(mLauncherAngle) , -cos( mLauncherAngle ) );
	}

	void Level::generateRandomLevel( int maxLayer , int density )
	{
		for( int layer = 1 ; layer < maxLayer ; ++layer )
		{
			for( int i = 1 ; i < mNumCellLayer ; ++i )
			{

			}
		}
	}

}//namespace Bubble