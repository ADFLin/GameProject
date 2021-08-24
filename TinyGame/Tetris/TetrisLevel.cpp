#include "TetrisPCH.h"
#include "TetrisLevel.h"
#include "GameGlobal.h"

#include "ColorName.h"

namespace Tetris
{
	int const Level::GravityUnit           = 256;
	int const LevelRule::DefaultTimeClearLayer = 40 * gDefaultTickTime;
	int const LevelRule::DefaultTimeLockPiece  = 500;
	int const LevelRule::DefaultTimeEntryDelay = 12 * gDefaultTickTime;

	int const TsClassicMapSizeX = 10;
	int const TsClassicMapSizeY = 20;

	static uint8 const* GetRotationTransform(int size, int time)
	{
		assert(size == 3 || size == 4);
		static uint8 const gRotate3x3[4 * 9] =
		{
			0,1,2, 3,4,5, 6,7,8,
			2,5,8, 1,4,7, 0,3,6,
			8,7,6, 5,4,3, 2,1,0,
			6,3,0, 7,4,1, 8,5,2,
		};

		static uint8 const gRotate4x4[4 * 16] =
		{
			0, 1, 2, 3,   4, 5, 6, 7,   8, 9,10,11, 12,13,14,15,
			3, 7,11,15,   2, 6,10,14,   1, 5, 9,13,   0, 4, 8,12,
			15,14,13,12,  11,10, 9, 8,  7, 6, 5, 4,   3, 2, 1, 0,
			12, 8, 4, 0,  13, 9, 5, 1,  14,10, 6, 2,  15,11, 7, 3,
		};
		uint8 const* curTrans = (size == 3) ? gRotate3x3 : gRotate4x4;
		curTrans += size *size*time;

		return curTrans;
	}

	static PieceTemplate gClassicTemplateData[] = 
	{
		4 , 2 , EColor::Red    , ARRAY2BIT4( 0010 , 0010 , 0010 , 0010 ) ,
		4 , 1 , EColor::Yellow , ARRAY2BIT4( 0000 , 0110 , 0110 , 0000 ) ,
		3 , 2 , EColor::Green  , ARRAY2BIT3( 100 , 110 , 010 ) ,
		3 , 2 , EColor::Purple , ARRAY2BIT3( 001 , 011 , 010 ) ,
		3 , 4 , EColor::Cyan   , ARRAY2BIT3( 010 , 110 , 010 ) ,
		3 , 4 , EColor::Blue   , ARRAY2BIT3( 110 , 010 , 010 ) ,
		3 , 4 , EColor::Orange , ARRAY2BIT3( 011 , 010 , 010 ) ,
	};
	PieceTemplateSet gClassicTemplateSet = PieceTemplateSet( gClassicTemplateData , sizeof(gClassicTemplateData) / sizeof(PieceTemplate));

	static PieceTemplate sEmptyTemp = { 3 ,1 , EColor::Red , 0 };
	static Piece         sEmptyPiece;

	void PieceBlock::transformPos( uint8 const* trans, int size )
	{
		index = trans[ index ];
		x = index % size;
		y = index / size;
	}

	void PieceBlock::setPos( int idx , int size )
	{
		index = idx;
		x = index % size;
		y = index / size;
	}

	void Piece::setTemplate( PieceTemplate& temp )
	{
		mTemplate = &temp;

		assert( getRotationSize() == 3 || getRotationSize() == 4 );
		int len = getRotationSize() * getRotationSize();

		mNumBlock = 0;
		for( int i = 0 ; i < len ; ++i )
		{
			if ( getTemplate().blockData & ( 1 << i ) )
			{
				PieceBlock& block = getBlockInternal( mNumBlock );
				block.setPos( i , getRotationSize() );
				block.type = getTemplate().baseColor;

				++mNumBlock;
			}
		}
	}

	void Piece::rotate( int time )
	{
		int dirNum = getDirectionNum();
		time = time % dirNum;

		if ( time < 0 )
			time += dirNum;

		if ( dirNum == 2 )
		{
			if ( mDirection == 1 && time == 1 )
				time = 3;
			//else if ( mDir == 0 && time == -1 )
				//time = 1;
		}

		mDirection = ( mDirection + time + dirNum ) % dirNum;

		int mapSize = getRotationSize();
		if ( mapSize )
		{
			uint8 const* curTrans = GetRotationTransform(mapSize, time);
			for( int i = 0; i < getBlockNum(); ++i )
			{
				PieceBlock& block = getBlockInternal(i);
				block.transformPos(curTrans, mapSize);
			}
		}
	}

	Piece::Piece()
	{
		mDirection   = 0;
		setTemplate( sEmptyTemp );
	}

	void Piece::getBoundRect(int x[] , int y[])
	{
		x[0] = y[0] =  100;
		x[1] = y[1] = -100;

		for( int i = 0 ; i < getBlockNum() ; ++i )
		{
			PieceBlock const& block = getBlock(i);
			if ( block.x < x[0] ) x[0] = block.x;
			else if ( block.x > x[1] ) x[1] = block.x;

			if ( block.y < y[0] ) y[0] = block.y;
			else if ( block.y > y[1] ) y[1] = block.y;
		}
	}

	PieceTemplateSet::PieceTemplateSet( PieceTemplate temp[] , int num )
	{
		mTemps = temp;
		mNum   = num;
	}

	PieceTemplateSet& PieceTemplateSet::GetClassic()
	{
		return gClassicTemplateSet;
	}

	BlockStorage::BlockStorage( int sizeX , int sizeY )
		:mSizeX( sizeX )
		,mSizeY( sizeY )

	{
		storeData();
		clearBlock();
	}

	BlockStorage::~BlockStorage()
	{
		cleanupData();
	}

	void BlockStorage::storeData()
	{
		int extendMapSizeY = getExtendSizeY();
		mLayerMap.resize( extendMapSizeY );
		mBlockStorage.reset( new  BlockType[ mSizeX * extendMapSizeY ] );
		for( int i = 0 ; i < extendMapSizeY ; ++i )
		{
			mLayerMap[i].blocks = mBlockStorage.get() + i * mSizeX;
		}

		mConMap.resize( mSizeY );
		int* cons = new int[ mSizeX * mSizeY ];
		for( int i = 0 ; i < mSizeY ; ++i )
		{
			mConMap[i] = cons + i * mSizeX;
		}
	}

	void BlockStorage::cleanupData()
	{
		delete [] mConMap[0];
		mBlockStorage.clear();
	}

	void BlockStorage::reset(int sizeX , int sizeY )
	{
		if ( sizeX != mSizeX || sizeY != mSizeY )
		{
			cleanupData();
			mSizeX = sizeX;
			mSizeY = sizeY;
			storeData();
		}
		clearBlock();
	}

	void BlockStorage::clearBlock()
	{
		int extendMapSizeY = getExtendSizeY();
		BlockType* blocks = mBlockStorage.get();
		memset( blocks , 0 , sizeof( BlockType ) * extendMapSizeY * mSizeX );
		for( int i = 0 ; i < extendMapSizeY ; ++i )
		{
			mLayerMap[i].filledCount = 0;
		}
	}

	unsigned BlockStorage::testCollision( Piece& piece , int cx, int cy )
	{
		int result = 0;

		for( int i = 0 ; i < piece.getBlockNum(); ++i )
		{
			PieceBlock const& block = piece.getBlock(i);

			int xPos = cx + block.getX();
			int yPos = cy + block.getY();

			unsigned hitFlag = 0;

			if ( xPos < 0 )
				hitFlag |= HIT_LEFT_SIDE;
			else if ( xPos >= mSizeX )
				hitFlag |= HIT_RIGHT_SIDE;

			if ( yPos < 0 )
				hitFlag |= HIT_BOTTOM_SIDE;

			if ( hitFlag == 0 && getBlock( xPos , yPos ) )
				hitFlag |= HIT_BLOCK;

			result |= hitFlag;
		}

		return result;
	}

	void BlockStorage::markPiece( Piece& piece , int x , int y )
	{
		for( int i = 0 ; i < piece.getBlockNum(); ++i )
		{
			PieceBlock const& block = piece.getBlock(i);

			int xPos = x + block.getX();
			int yPos = y + block.getY();

			if ( isValidRange( xPos , yPos ) )
			{
				setBlock( xPos , yPos , block.getType() );
			}
		}
	}

	int BlockStorage::movePiece( Piece& piece , int& x , int& y  , int ox , int oy )
	{
		int xPosNew = x + ox;
		int yPosNew = y + oy;

		unsigned hit = testCollision( piece , xPosNew , yPosNew );

		if ( !hit )
		{
			x = xPosNew;
			y = yPosNew;
		}

		return hit;
	}

	void BlockStorage::removeLayer( int ys[] , int num )
	{
		int extendMapSizeY = getExtendSizeY();
		static int const MaxRemoveLayerNum = 10;
		assert( num <= MaxRemoveLayerNum );

		BlockType* savedBlocks[ MaxRemoveLayerNum ];
		for( int i = 0 ; i < num ; ++i )
		{
			int y = ys[i];
			assert( i - 1 < 0 || ys[i-1] > ys[i] );

			savedBlocks[i] = mLayerMap[y].blocks;
			mLayerMap[y].blocks = nullptr;
		}

		int yFill = ys[ num - 1 ];
		int maxY = extendMapSizeY - 1;
		for( int y = ys[ num - 1 ] + 1; y <= maxY ; ++y )
		{
			if ( mLayerMap[y].blocks == nullptr )
				continue;

			mLayerMap[yFill] = mLayerMap[y]; 
			++yFill;
		}
		for( int i = 0 ; i < num ; ++i )
		{
			Layer& layer = mLayerMap[yFill];
			layer.blocks = savedBlocks[i];
			layer.filledCount = 0;
			std::fill_n(layer.blocks, mSizeX, 0);
			++yFill;
		}
	}

	void BlockStorage::removeLayer(int y)
	{
		int extendMapSizeY = getExtendSizeY();

		BlockType* savedBlock = mLayerMap[y].blocks;
		Layer cLayer = mLayerMap[y];
		for( int j = y ; j < extendMapSizeY - 1; ++j )
			mLayerMap[j] = mLayerMap[j+1];

		Layer& layer = mLayerMap[extendMapSizeY - 1];
		layer.blocks = savedBlock;
		layer.filledCount = 0;
		std::fill_n(layer.blocks, mSizeX, 0);
	}

	bool BlockStorage::isLayerFilled( int y )
	{
		return mLayerMap[y].filledCount == mSizeX;
	}

	int BlockStorage::scanFilledLayer( int yMax , int yMin , int removeLayer[] )
	{
		int n = 0; 

		for( int y = yMax; y >= yMin ; --y )
		{
			if ( !isSafeRange(0,y) )
				continue;
			if ( !isLayerFilled( y ) )
				continue;
	
			removeLayer[ n++ ] = y;
		}
		removeLayer[n] = -1;
		return n;
	}

	int BlockStorage::rotatePiece( Piece& piece , int& x , int& y , bool beCW )
	{
		piece.rotate( beCW ? -1 : 1 );

		unsigned hit = testCollision( piece , x , y );

		if ( hit )
		{
			if( hit == HIT_BLOCK && testCollision(piece, x, y + 1) == 0 )
			{
				y += 1;
				return hit;
			}
			if( hit == HIT_LEFT_SIDE && testCollision(piece, x + 1, y) == 0 )
			{
				x += 1;
				return hit;
			}
			if( hit == HIT_RIGHT_SIDE && testCollision(piece, x - 1, y) == 0 )
			{
				x -= 1;
				return hit;
			}

			piece.rotate(beCW ? 1 : -1);
		}

		return hit;
	}

	int BlockStorage::scanConnect( int cx , int cy  )
	{
		int* temp = mConMap[0];
		for (int i = 0; i <  mSizeX * mSizeY ; ++i)
			temp[i] = 0;

		if ( !isSafeRange( cx , cy ) )
			return 0;

		BlockType color = Piece::Color( getBlock( cx , cy ) );
		return scanConnectRecursive( cx , cy , color );
	}

	int BlockStorage::scanConnectRecursive( int cx , int cy , short color  )
	{
		assert( isSafeRange( cx , cy ) );

		if ( mConMap[cy][cx] )
			return 0;

		if ( Piece::Color( getBlock( cx , cy ) )!= color )
			return 0;

		int num = 0;
		int total = 0;

		mConMap[cy][cx] = 1;

#define CHECK_CON_NUM( PX , PY )\
		{\
			int nx = ( PX );\
			int ny = ( PY );\
			if ( isSafeRange( nx , ny ) )\
			{\
				total += scanConnectRecursive( nx , ny, color );\
				if ( Piece::Color( getBlock( nx , ny ) ) == color )\
				++num;\
			}\
		}

		CHECK_CON_NUM( cx + 1 , cy );
		CHECK_CON_NUM( cx - 1 , cy );
		CHECK_CON_NUM( cx , cy + 1 );
		CHECK_CON_NUM( cx , cy - 1 );

#undef CHECK_CON_NUM

		return total + (( num <= 2 ) ? 1 : 0 );
	}

	int BlockStorage::removeConnect()
	{
		int removeNum = 0;

		for ( int i = 0 ; i < mSizeX ; ++i )
		{
			int idx = 0;
			for ( int j = 0 ; j < mSizeY ; ++j )
			{
				if( mConMap[j][i] )
				{
					emptyBlock( i , j );
					++removeNum;
				}
			}
		}

		return removeNum;
	}

	bool BlockStorage::addLayer( int y , unsigned leakBit , BlockType block )
	{
		assert( y < mSizeY );

		int extendMapSizeY = getExtendSizeY();

		if ( !mLayerMap[extendMapSizeY - 1].isEmpty() )
			return false;

		Layer layer = mLayerMap[extendMapSizeY - 1];
		int filledCount = 0;
		for( int i = 0; i < mSizeX; ++i )
		{
			if( leakBit & BIT(i) )
				continue;

			layer.blocks[i] = block;
			++filledCount;
		}
		layer.filledCount = filledCount;

		for( int j = extendMapSizeY - 1 ; j > y ; --j )
			mLayerMap[j] = mLayerMap[j-1];
		mLayerMap[y] = layer;

		return true;
	}

	void BlockStorage::setBlock( int x , int y , BlockType val )
	{
		assert( val );
		
		Layer& layer = mLayerMap[y];
		layer.blocks[x] = val;
		++layer.filledCount;
	}

	void BlockStorage::emptyBlock( int x , int y )
	{
		Layer& layer = mLayerMap[y];
		assert(layer.blocks[x]);
		layer.blocks[x] = 0;
		--layer.filledCount;
	}

	void BlockStorage::fixPiecePos( Piece& piece , int& cx , int& cy , int dy )
	{
		int y = cy + dy;

		int j = y;
		for( ; j >= cy ; --j )
		{
			if ( testCollision( piece , cx , y ) )
				break;
		}

		cy = j + 1;
		//FIXME
	}

	Level::Level( EventListener* listener )
		:mStorage( TsClassicMapSizeX , TsClassicMapSizeY )
	{
		mState      = LVS_OVER;
		mListener   = listener;
		mUserData   = NULL;
	}

	void Level::setMovePiece( Piece& piece , int x , int y )
	{
		mMovePiece = &piece;
		mXPosMP = x;
		mYPosMP = y;
	}

	void Level::restart()
	{
		mStorage.clearBlock();

		for( int i = 0 ; i < NumStoragePiece ; ++i )
		{
			produceRandomPiece( mPiece[i] );
		}

		mHoldPiece = &sEmptyPiece;
		mMovePiece = &sEmptyPiece;

		mIndexPiece = 0;

		mState = LVS_NORMAL;
		mStateDuration = 0;
		mTimeDuration = 0;
		mTiggerLockPiece = 0;

		mTotalLayerRemove = 0;
		mTotalPieceUse    = 0;

		changeNextPiece();
	}

	void Level::update( long time )
	{
		if ( getState() != LVS_OVER )
			mTimeDuration += time;

		mStateDuration += time;

		switch ( getState() )
		{
		case LVS_NORMAL:
			{
				mTiggerLockPiece += time;
				mCountMove += time * gDefaultGameFPS;
				//long countG = ( 1000 * GravityUnit ) / getGravityValue();
				//assert( countCurG );
				while( mCountMove >= mCountGravity )
				{
					LevelState nextState = doMoveDown( true );
					if ( nextState == LVS_NORMAL )
					{
						mCountMove -= mCountGravity;
					}
					else
					{
						mCountMove = 0;
						changeState( nextState );
					}
				}
			}
			break;
		case LVS_REMOVE_MAPLINE:
			if ( mStateDuration > getClearLayerTime() )
			{
				mStorage.removeLayer( mRemoveLayer , mRemoveLayerNum );

				mTotalLayerRemove += mRemoveLayerNum;
				mListener->onRemoveLayer( this , mRemoveLayer , mRemoveLayerNum );
				changeNextPiece();
				changeState( LVS_NORMAL );
			}
			break;
		case LVS_REMOVE_CONNECT:
			if ( mStateDuration > getClearLayerTime() )
			{
				int n = mStorage.removeConnect();
				changeNextPiece();
				changeState( LVS_NORMAL );
			}
			break;
		case LVS_ENTRY_DELAY:
			if ( mStateDuration > getEntryDelayTime() )
			{
				changeNextPiece();
				changeState( LVS_NORMAL );
			}
			break;
		case LVS_OVER:
			break;
		}
	}

	void Level::produceRandomPiece( Piece& piece )
	{
		piece.resetRotation();
		PieceTemplateSet* set = LevelRule::getPieceTemplateSet();
		set->setTemplate( Global::RandomNet() % set->getTemplateNum() , piece );
		int rotateTime = Global::RandomNet() % piece.getDirectionNum();
		piece.rotate( rotateTime );
	}

	void Level::moveDown()
	{
		if ( mState != LVS_NORMAL )
			return;

		LevelState nextState = doMoveDown( false );
		changeState( nextState );
	}

	void Level::fallPiece()
	{
		if ( mState != LVS_NORMAL )
			return;

		LevelState nextState;
		do
		{
			nextState = doMoveDown( false );
		}
		while ( nextState == LVS_NORMAL );
		changeState( nextState );
	}

	int Level::calcFallPosY()
	{
		Piece& curPiece = getMovePiece();

		int x = mXPosMP;
		int y = mYPosMP;

		int hit;
		do 
		{
			hit = mStorage.movePiece( curPiece , x , y , 0 , -1 );

		} while ( !hit );

		return y;
	}

	LevelState Level::doMoveDown( bool beGravity )
	{
		if ( !beGravity )
			mCountMove = 0;

		Piece& curPiece = getMovePiece();
		int hit = mStorage.movePiece( curPiece , mXPosMP , mYPosMP , 0 , -1 );

		if ( hit )
		{
			if ( hit & ( BlockStorage::HIT_BLOCK | BlockStorage::HIT_BOTTOM_SIDE ) )
			{
				if ( !beGravity )
					mTiggerLockPiece += getLockPieceTime() / 3;

				if ( mTiggerLockPiece >= getLockPieceTime() )
				{
					mStorage.markPiece( curPiece , mXPosMP , mYPosMP );

					int extendMapSizeY = mStorage.getExtendSizeY();
					//check over
					for( int y = mStorage.getSizeY() ; y < extendMapSizeY ; ++y )
					{
						if ( !mStorage.isEmptyLayer( y ) )
							return LVS_OVER;
					}

					int numLayer = mStorage.scanFilledLayer( mYPosMP + getMovePiece().getRotationSize() - 1 , mYPosMP ,  mRemoveLayer );
					assert( numLayer <= MaxRemoveLayerNum );

					++mTotalPieceUse;
					mTiggerLockPiece = 0;

					mRemoveLayerNum = numLayer;

					if ( numLayer )
						return LVS_REMOVE_MAPLINE;
#if 0
					int tx = mXPosMP + getMovePiece().getBlock(0).x;
					int ty = mYPosMP + getMovePiece().getBlock(0).y;
					int numCon = mStorage.scanConnect( tx , ty , mConMap );
					if ( numCon  >= 10 )
						return LVS_REMOVE_CONNECT;
#endif

					mListener->onMarkPiece( this ,mRemoveLayer , mRemoveLayerNum );
					return LVS_ENTRY_DELAY;
				}
			}
		}
		else
		{
			mTiggerLockPiece = 0;
		}

		return LVS_NORMAL;
	}


	void Level::changeNextPiece()
	{
		produceRandomPiece( mPiece[ mIndexPiece ] );

		Piece& piece = getNextPiece();
		mIndexPiece = (int)( &piece - mPiece );

		int yMin = 100;

		for( int i = 0 ; i < piece.getBlockNum() ; ++i )
		{
			PieceBlock const& block = piece.getBlock(i);
			if ( block.getY() < yMin )
				yMin = block.getY();
		}

		setMovePiece( piece, ( mStorage.getSizeX() - piece.getRotationSize() )/ 2,  mStorage.getSizeY() - yMin  );

		mCountMove = 0;
		mListener->onChangePiece( this );
	}

	void Level::changeState( LevelState state )
	{
		if ( mState == state )
			return;
		mState = state;
		mStateDuration = 0;
	}

	Piece& Level::getNextPiece( int idx /*= 0 */ )
	{
		assert( idx < NumStoragePiece );
		int nextIdx = ( mIndexPiece + idx + 1 );

		int idxEnd = mIndexPiece + idx + 1;

		for( int i = mIndexPiece + 1 ; i <= idxEnd ; ++i )
		{
			int idxTest = i;
			if ( idxTest >= NumStoragePiece )
				idxTest -= NumStoragePiece;
			if ( mPiece + idxTest == mHoldPiece )
			{
				++nextIdx;
				break;
			}
		}

		if ( nextIdx >= NumStoragePiece )
			nextIdx -= NumStoragePiece;

		return *(mPiece + nextIdx);
	}

	void Level::holdPiece()
	{
		Piece& piece = getNextPiece();
		if ( mHoldPiece != &sEmptyPiece )
		{
			using std::swap;
			swap( *mHoldPiece , piece );
		}
		else
		{
			mHoldPiece = &piece;
		}
	}

	void Level::setGravityValue( int value )
	{
		mGravityValue = value;
		mCountGravity = ( 1000 * GravityUnit ) / value;
		assert( mCountGravity );
	}

	void Level::resetMap(int sizeX , int sizeY)
	{
		mStorage.reset( sizeX , sizeY );

		Piece& piece = getMovePiece();

		int sx[2];
		int sy[2];
		piece.getBoundRect( sx , sy );

		if ( mXPosMP + sx[0] < 0 )
			mXPosMP = -sx[0];
		else if ( mXPosMP + sx[1] >= getBlockStorage().getSizeX() )
			mXPosMP = getBlockStorage().getSizeX() - sx[1] - 1;

		if ( mYPosMP + sy[0] < 0 )
			mYPosMP = -sy[0];
		else if ( mYPosMP + sy[1] >= getBlockStorage().getSizeY() )
			mYPosMP = getBlockStorage().getSizeY() - sy[1] - 1;
	}


	LevelRule::LevelRule()
	{
		setDefault();
	}

	void LevelRule::setDefault()
	{
		mPieceTempSet = &PieceTemplateSet::GetClassic();

		mTimeLockPiece = DefaultTimeLockPiece;
		mTimeClearLayer = DefaultTimeClearLayer;
		mTimeEntryDelay = DefaultTimeEntryDelay;
	}

}//namespace Tetris
