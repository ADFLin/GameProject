#include "TinyGamePCH.h"
#include "GoCore.h"

#include "SocketBuffer.h"
#include "FileSystem.h"
#include "StringParse.h"
#include "Core/FNV1a.h"
#include "Core/StringConv.h"

#include <cassert>
#include <iostream>
#include <fstream>

namespace GoCore
{

	static int const gDirOffset[4][2] =
	{
		1,0,  -1,0,  0,-1,  0,1,
	};


	void BoardBase::setupDataEdge(int dataSize)
	{
		mData[0] = mData[dataSize - 1] = EDGE_MARK;
		dataSize -= 2;

		auto pData = mData.get() + 1;
		for (int i = getDataSizeX() - 1; i < dataSize; i += getDataSizeX())
		{
			pData[i] = EDGE_MARK;
		}
		{

			int offsetTop = dataSize - getDataSizeX();
			char* top = pData;
			char* bottom = pData + offsetTop;
			for (int i = 0; i < getDataSizeX(); ++i)
			{
				*(top++) = *(bottom++) = EDGE_MARK;
			}
		}
	}
}

namespace Go
{

	Board::Board()
	{
	}

	Board::~Board()
	{
	}

	void Board::copy(Board const& other)
	{
		if( mSize != other.mSize )
		{
			setup(other.mSize, false);
		}
		std::copy(other.mLinkIndex.get(), other.mLinkIndex.get() + getDataSize(), mLinkIndex.get());
		std::copy(other.mData.get(), other.mData.get() + getDataSize(), mData.get());
	}

	void Board::setup(int size, bool bClear)
	{
		if (mSize == size)
			return;

		if ( mSize < size )
		{
			mSize = size;
			int dataSize = getDataSize();
			mData = std::make_unique<char[]>( dataSize );
			mLinkIndex.reset( new LinkType[ dataSize ] );	
		}
		else
		{
			mSize = size;
		}

		initIndexOffset();
		if (bClear)
		{
			clear();
		}
		else
		{
			int dataSize = getDataSize();
			setupDataEdge(dataSize);
		}
	}

	void Board::clear()
	{
		int dataSize = getDataSize();
		std::fill_n( mLinkIndex.get(), dataSize, 0);
		std::fill_n(mData.get(), dataSize, (char)EStoneColor::Empty);
		setupDataEdge(dataSize);
	}

	int Board::getCaptureCount( int x , int y ) const
	{
		assert( getData( x , y ) != EStoneColor::Empty );
		return getCaptureCount( getDataIndex( x, y ) );
	}

	int Board::getCaptureCount( int index ) const
	{
		assert( getData( index ) != EStoneColor::Empty );
		int idxRoot = getLinkRoot( index );
		return -mLinkIndex[ idxRoot ];
	}

	void Board::putStone( int idx, DataType color )
	{
		assert( color != EStoneColor::Empty );
		assert( mData[idx] == EStoneColor::Empty );
		mData[ idx ] = color;

		mCacheIdxConRoot = idx;
		mLinkIndex[ idx ] = 0;

		for( int i = 0 ; i < NumDir ; ++i )
		{
			int idxCon = calcLinkIndex( idx , i );
			char data = getData( idxCon );

			if ( data == EDGE_MARK )
				continue;

			if ( data == EStoneColor::Empty )
			{
				addRootCaptureCount( mCacheIdxConRoot , 1 );
			}
			else
			{
				int idxRoot = getLinkRoot( idxCon );
				if ( data == color && mCacheIdxConRoot != idxRoot )
				{
					linkRoot( idxRoot );
				}
				else
				{
					addRootCaptureCount( idxRoot , -1 );
				}
			}
		}
	}

	int Board::fillLinkedStone( Pos const& p , DataType color )
	{
		mCacheColorR = color;
		assert( color != EStoneColor::Empty );
		int idx = p.toIndex();
		return fillLinkedStone_R( idx );
	}

	int Board::fillLinkedStone_R( int idx )
	{
		char data = getData( idx );
		if ( data != EStoneColor::Empty )
			return 0;

		putStone( idx , mCacheColorR );

		int result = 1;
		for( int i = 0 ; i < Board::NumDir ; ++i )
		{
			int idxCon = calcLinkIndex( idx , i );
			result += fillLinkedStone_R( idxCon );
		}

		return result;
	}

	void Board::linkRoot( int idxRoot )
	{
		assert( idxRoot != mCacheIdxConRoot );

		int life1 = mLinkIndex[ mCacheIdxConRoot ];
		int life2 = mLinkIndex[ idxRoot ];

		int totalLive = life1 + life2 + 1;
		if ( life1 >= life2 )
		{
			mLinkIndex[ mCacheIdxConRoot ] = idxRoot;
			mLinkIndex[ idxRoot ] = totalLive;
			mCacheIdxConRoot = idxRoot;
		}
		else
		{
			mLinkIndex[ idxRoot ] = mCacheIdxConRoot;
			mLinkIndex[ mCacheIdxConRoot ] = totalLive;
		}
	}

	void Board::removeStone( Pos const& p )
	{
		int  idx = p.toIndex();
		char data = getData( idx );
		assert( data != EStoneColor::Empty );

		mData[ idx ] = EStoneColor::Empty;

		for( int dir = 0 ; dir < Board::NumDir ; ++dir )
		{
			int idxCon = calcLinkIndex( idx , dir );
			char dataCon = mData[ idxCon ];

			//if( dataCon & VISITED_MARK )
				//continue;
			if ( dataCon == EStoneColor::Black || dataCon == EStoneColor::White )
			{
				if( dataCon != data )
				{
					addRootCaptureCount(getLinkRoot(idxCon), 1);
				}
				else
				{
					relink(idxCon);
				}
			}
		}

		for( int dir = 0 ; dir < Board::NumDir ; ++dir )
		{
			int idxCon = calcLinkIndex( idx , dir );
			removeVisitedMark_R( idxCon );
		}
	}



	int Board::getLinkRoot( int idx ) const
	{
#if 0
		int result = idx;
		while( mLinkIndex[result] > 0 )
		{
			result = mLinkIndex[result];
		}
		//mLinkIndex[idx] = result;
		return result;
#else
		if( mLinkIndex[idx] <= 0 )
			return idx;

		int prev = idx;
		int result = mLinkIndex[idx];
		int next;
		while(  ( next = mLinkIndex[result] ) > 0 )
		{
			mLinkIndex[prev] = next;
			prev = result;
			result = next;
		}

		mLinkIndex[idx] = result;
		return result;
#endif
	}

	int Board::getLinkToRootDist(int idx) const
	{
		int result = 0;
		while( mLinkIndex[idx] > 0 )
		{
			++result;
			idx = mLinkIndex[idx];
		}
		return result;
	}

	void Board::addRootCaptureCount(int idxRoot, int val)
	{
		assert( getLinkRoot( idxRoot ) == idxRoot );
		mLinkIndex[ idxRoot ] -= val;
		assert(mLinkIndex[idxRoot] <= 0);
	}

	int Board::getLiberty_R(int idx) const
	{
		//#TODO
		DataType data = getData(idx);
		if( data == EDGE_MARK )
			return 0;

		return 0;
	}


	int Board::captureLinkedStone(Pos const& p)
	{
		mCacheColorR = getData(p);
		assert(mCacheColorR != EStoneColor::Empty);
		return captureLinkedStone_R(p.toIndex());
	}

	int Board::captureLinkedStone_R(int idx)
	{
		DataType data = getData( idx );

		if ( data != mCacheColorR )
		{
			if( data == EDGE_MARK || data == EStoneColor::Empty )
			{

			}
			else
			{
				int idxRoot = getLinkRoot(idx);
				addRootCaptureCount(idxRoot, 1);
			}
			return 0;
		}

		mData[ idx ] = EStoneColor::Empty;

		int result = 1;
		for( int dir = 0 ; dir < Board::NumDir ; ++dir )
			result += captureLinkedStone_R( calcLinkIndex( idx , dir ) );

		return result;
	}

	int Board::peekCaptureStone(Pos const& p, unsigned& outDirMask ) const
	{
		mCacheColorR = EStoneColor::Opposite(getData(p));
		assert(mCacheColorR != EStoneColor::Empty);

		int numCapture = 0;
		for( int dir = 0; dir < Board::NumDir; ++dir )
		{
			Pos posCon;
			if( !getLinkPos(p, dir, posCon) )
				continue;

			DataType dataCon = getData(posCon);
			//if ( dataCon & VISITED_MARK )
				//continue;

			if( dataCon != mCacheColorR )
				continue;

			if( getCaptureCount(posCon) != 0 )
				continue;

			numCapture += peekCaptureConStone(posCon);
			outDirMask |= (1 << dir);
		}

		for( int dir = 0; dir < Board::NumDir; ++dir )
		{
			if( BIT(dir) & outDirMask )
			{
				removeVisitedMark_R(calcLinkIndex(p.toIndex(), dir));
			}		
		}
		return numCapture;
	}

	int Board::peekCaptureConStone(Pos const& p) const
	{
		int idx = p.toIndex();
		DataType data = getData(idx);
		assert(data == mCacheColorR);
		assert((data & VISITED_MARK) == 0);
		mData[idx] |= VISITED_MARK;
		int result = 1;
		for( int dir = 0; dir < Board::NumDir; ++dir )
			result += peekCaptureLinkedStone_R(calcLinkIndex(idx, dir));

		return result;
	}

	int Board::peekCaptureLinkedStone_R(int idx) const
	{
		if( mData[idx] != mCacheColorR )
			return 0;

		mData[idx] |= VISITED_MARK;

		int result = 1;
		for( int dir = 0; dir < Board::NumDir; ++dir )
			result += peekCaptureLinkedStone_R(calcLinkIndex(idx, dir));

		return result;
	}

	int Board::relink(int idx)
	{
		DataType data = getData( idx );
		assert( data != EStoneColor::Empty && data != EDGE_MARK );
		assert( (data & VISITED_MARK) == 0 );
		mData[ idx ] |= VISITED_MARK;
		mCacheIdxConRoot = idx;
		mCacheColorR = DataType( data );

		int life = 0;
		for( int dir = 0 ; dir < Board::NumDir ; ++dir )
		{
			life += relink_R( calcLinkIndex( idx , dir ) );
		}
		mLinkIndex[ idx ] = -life;
		return life;
	}

	int Board::relink_R( int idx )
	{
		DataType data = mData[ idx ];

		if ( data == EStoneColor::Empty )
			return 1;

		if ( data != mCacheColorR )
			return 0;

		mData[ idx ] |= VISITED_MARK;
		mLinkIndex[ idx ] = mCacheIdxConRoot;

		int life = 0;
		for( int dir = 0 ; dir < Board::NumDir ; ++dir )
			life += relink_R( calcLinkIndex( idx , dir ) );

		return life;
	}

	void Board::removeVisitedMark_R( int idx ) const
	{
		if ( mData[ idx ] & VISITED_MARK )
		{
			mData[ idx ] &= ~VISITED_MARK;
			for( int dir = 0 ; dir < Board::NumDir ; ++dir )
				removeVisitedMark_R( calcLinkIndex( idx , dir ) );
		}
	}

	bool Board::getLinkPos( Pos const& pos , int dir , Pos& result ) const
	{
		int idx = calcLinkIndex( pos.toIndex() , dir );
		
		if ( getData( idx ) == EDGE_MARK )
			return false;

		result = Pos(idx);
		return true;
	}

	Game::Game()
	{

	}

	void Game::setup( int size )
	{
		mSetting.boardSize = size;
		doRestart( false );
	}

	void Game::restart()
	{
		doRestart( true );
	}

	void Game::doRestart(bool beClearBoard , bool bClearStepHistory)
	{
		if (mSetting.boardSize != mBoard.getSize())
		{
			mBoard.setup(mSetting.boardSize, beClearBoard);
		}
		else
		{
			if (beClearBoard)
				mBoard.clear();
		}

		if( bClearStepHistory )
			mStepHistory.clear();

		mCurrentStep  = 0;
		
		mNumBlackCaptured = 0;
		mNumWhiteCaptured = 0;

		mSimpleKOStates.clear();

		mNextPlayColor = mSetting.bBlackFirst ? EStoneColor::Black : EStoneColor::White;
		if( mSetting.numHandicap && mSetting.bFixedHandicap )
		{
			VisitFixedHandicapPositions( mBoard.getSize(), mSetting.numHandicap,
				[&]( int x , int y )
				{
					mBoard.putStone(mBoard.getPos(x, y), EStoneColor::Black);
				}
			);
		}
	}

	bool Game::canPlay(int x, int y) const
	{
		if( !mBoard.checkRange(x, y) )
			return false;

		Pos pos = mBoard.getPos(x, y);

		if( mBoard.getData(pos) != EStoneColor::Empty )
			return false;

		mBoard.putStone(pos, mNextPlayColor);

		unsigned bitDir = 0;
		int numCapture = const_cast<Game&>(*this).captureStone(pos, bitDir);
		bool result = true;
		if (numCapture == 0)
		{
			if (mBoard.getCaptureCount(pos) == 0)
			{
				result = false;
			}
		}
		else
		{
			KOState koState = calcKOState(pos);
			if (isKOStateReached(pos, mNextPlayColor, koState))
			{
				result = false;
			}

			for (int dir = 0; dir < Board::NumDir; ++dir)
			{
				if (bitDir & BIT(dir))
				{
					Pos linkPos;
					bool isOK = mBoard.getLinkPos(pos, dir, linkPos);
					assert(isOK);
					mBoard.fillLinkedStone(linkPos, EStoneColor::Opposite(mNextPlayColor));
				}
			}
		}

		mBoard.removeStone(pos);
		return result;
	}

	bool Game::addStone(int x, int y, DataType color)
	{
		if( !mBoard.checkRange(x, y) )
			return false;

		Pos pos = mBoard.getPos(x, y);
		addStoneInternal(pos, color, false);
		return true;
	}

	bool Game::playStone(int x, int y)
	{
		if( !mBoard.checkRange(x, y) )
			return false;

		Pos pos = mBoard.getPos(x, y);
		return playStone(pos);
	}

	void Game::addStoneInternal(Pos const& pos, DataType color, bool bReviewing)
	{
		DataType curColor = mBoard.getData(pos);

		if(  curColor != color )
		{
			if (curColor != EStoneColor::Empty)
				mBoard.removeStone(pos);
			if ( color != EStoneColor::Empty )
				mBoard.putStone(pos, color);
		}
		
		if( !bReviewing )
		{
			StepInfo info;
			info.colorAdded = color;
			info.bPlay = 0;
			info.idxPos = pos.toIndex();
			mStepHistory.push_back(info);
		}
		++mCurrentStep;
	}


	bool Game::playStoneInternal( Pos const& pos , bool bReviewing )
	{
		if ( mBoard.getData( pos ) != EStoneColor::Empty )
			return false;

		mBoard.putStone( pos , mNextPlayColor );

		unsigned bitDir = 0;
		int numCapture = captureStone( pos , bitDir );
		KOState koState = calcKOState(pos);

		bool result = true;
		if ( numCapture == 0 )
		{
			if ( mBoard.getCaptureCount( pos ) == 0 )
			{
				mBoard.removeStone( pos );
				return false;
			}
		}
		else
		{
			if ( isKOStateReached(pos , mNextPlayColor , koState ) )
			{
				for (int dir = 0; dir < Board::NumDir; ++dir)
				{
					if (bitDir & BIT(dir))
					{
						Pos linkPos;
						bool isOK = mBoard.getLinkPos(pos, dir, linkPos);
						assert(isOK);
						mBoard.fillLinkedStone(linkPos, EStoneColor::Opposite(mNextPlayColor));
					}
				}
				mBoard.removeStone( pos );
				return false;
			}
		}

		addKOState(mNextPlayColor, &pos ,  numCapture, koState);

		if ( !bReviewing )
		{
			StepInfo info;
			info.captureDirMask = bitDir;
			info.idxPos = pos.toIndex();
			info.bPlay = 1;
			mStepHistory.push_back(info);
		}

		++mCurrentStep;
		if ( mNextPlayColor == EStoneColor::Black )
		{
			mNumWhiteCaptured += numCapture;
			mNextPlayColor = EStoneColor::White;
		}
		else
		{
			mNumBlackCaptured += numCapture;
			mNextPlayColor = EStoneColor::Black;
		}
		return true;
	}

	void Game::playPassInternal( bool bReviewing )
	{
		addKOState(mNextPlayColor, nullptr , 0 , KOState::Invalid());

		if (!bReviewing)
		{
			StepInfo info;
			info.idxPos = -1;
			info.captureDirMask = 0;
			info.bPlay = 1;
			mStepHistory.push_back(info);
		}

		++mCurrentStep;
		if( mNextPlayColor == EStoneColor::Black )
		{
			mNextPlayColor = EStoneColor::White;
		}
		else
		{
			mNextPlayColor = EStoneColor::Black;
		}
	}

	int Game::captureStone( Board::Pos const& pos , unsigned& bitDir )
	{
		DataType dataCapture = EStoneColor::Opposite( mBoard.getData( pos ) );

		int numCapture = 0;
		for( int dir = 0 ; dir < Board::NumDir ; ++dir )
		{
			Pos posCon;
			if ( !mBoard.getLinkPos( pos , dir , posCon ) )
				continue;

			DataType dataCon = mBoard.getData( posCon );

			if ( dataCon != dataCapture )
				continue;

			if ( mBoard.getCaptureCount( posCon ) != 0 )
				continue;

			numCapture += mBoard.captureLinkedStone( posCon );
			bitDir |= ( 1 << dir );
		}

		return numCapture;
	}

	bool Game::undoInternal( bool bReviewing )
	{
		if( mCurrentStep == 0 )
			return false;

		StepInfo& step = mStepHistory[ mCurrentStep - 1];

		if( step.bPlay )
		{
			DataType color = EStoneColor::Opposite(mNextPlayColor);

			if( step.idxPos != INDEX_NONE)
			{
				Board::Pos posRemove = mBoard.getPos(step.idxPos);

				if( step.captureDirMask )
				{
					int num = 0;
					for( int dir = 0; dir < Board::NumDir; ++dir )
					{
						if( step.captureDirMask & BIT(dir) )
						{
							Board::Pos p;
							if( mBoard.getLinkPos(posRemove, dir, p) )
							{
								num += mBoard.fillLinkedStone(p, mNextPlayColor);
							}
							else
							{


							}
							
						}
					}

					if( color == EStoneColor::Black )
					{
						mNumWhiteCaptured -= num;
					}
					else
					{
						mNumBlackCaptured -= num;
					}
				}

				mBoard.removeStone(posRemove);
				removeKOState(color, &posRemove );
			}
			else
			{
				removeKOState(color, nullptr );
			}
			mNextPlayColor = color;
		}
		else
		{
			Pos posRemove = mBoard.getPos(step.idxPos);
			mBoard.removeStone(posRemove);
		}

		--mCurrentStep;
		if ( !bReviewing )
			mStepHistory.pop_back();

		return true;
	}

	void Game::copy(Game const& other)
	{
#define ASSIGN_VAR( VAR ) VAR = other.VAR;
		mBoard.copy(other.mBoard);
		ASSIGN_VAR(mSetting);
		ASSIGN_VAR(mCurrentStep);
		ASSIGN_VAR(mNumBlackCaptured);
		ASSIGN_VAR(mNumWhiteCaptured);
		ASSIGN_VAR(mSimpleKOStates);
		ASSIGN_VAR(mNextPlayColor);
		ASSIGN_VAR(mStepHistory);
#undef ASSIGN_VAR
	}

	static void EmitOp(IGameCopier& copier, Board const& board,  Game::StepInfo const& info , int& color )
	{
		if( info.bPlay )
		{
			if( info.idxPos == -1 )
			{
				copier.emitPlayPass(color);
			}
			else
			{
				int coord[2];
				board.getPosCoord(info.idxPos, coord);
				copier.emitPlayStone(coord[0], coord[1], color);
				color = EStoneColor::Opposite(color);
			}
		}
		else
		{
			int coord[2];
			board.getPosCoord(info.idxPos, coord);
			copier.emitAddStone(coord[0], coord[1], info.colorAdded);
		}
	}
	void Game::synchronizeState(IGameCopier& copier, bool bReviewOnly) const
	{
		copier.emitSetup(mSetting);

		int color = mSetting.bBlackFirst ? EStoneColor::Black : EStoneColor::White;
		int indexEnd = (bReviewOnly) ? mCurrentStep : mStepHistory.size();
		for( int i = 0; i < indexEnd; ++i )
		{
			auto const& info = mStepHistory[i];
			EmitOp(copier, mBoard, info, color);
		}
	}

	void Game::synchronizeStateKeep(IGameCopier& copier , int startStep, bool bReviewOnly) const
	{
		int color = mNextPlayColor;

		int playCount = 0;
		if( startStep < mCurrentStep )
		{
			for( int i = startStep; i < mCurrentStep; ++i )
			{
				if( mStepHistory[i].bPlay )
					++playCount;
			}
		}
		else if( startStep > mCurrentStep ) //review state
		{
			if( bReviewOnly )
			{
				for( int i = startStep; i > mCurrentStep; --i )
				{
					copier.emitUndo();
				}
				return;
			}

			if( startStep >= mStepHistory.size() )
			{
				LogError("Can't sync game state : Error start Step!!");
				return;
			}
			for( int i = startStep; i > mCurrentStep; --i )
			{
				if( mStepHistory[i].bPlay )
					++playCount;
			}
		}

		if( playCount % 2 )
		{
			color = EStoneColor::Opposite(color);
		}

		int indexEnd = (bReviewOnly) ? mCurrentStep : mStepHistory.size();
		for( int i = startStep; i < indexEnd; ++i )
		{
			auto const& info = mStepHistory[i];
			EmitOp(copier, mBoard, info, color);
		}
	}

	void Game::print(int x, int y)
	{
		using namespace std;
		static char const* dstr[] = 
		{
			"┌","┬","┐",
			"├","┼","┤",
			"└","┴","┘",
		};

		int size = mBoard.getSize();

		for( int j = 0 ; j < size ; ++ j )
		{
			for( int i = 0 ; i < size ; ++ i )
			{
				if ( i == x && j == y )
				{
					cout << "⊕";
					continue;
				}

				switch ( mBoard.getData( i , j ) )
				{
				case EStoneColor::Black: cout << "○" ; break;
				case EStoneColor::White: cout << "●" ; break;
				case EStoneColor::Empty:
					{
						int index = 0;
						if ( i != 0 )
						{ 
							index += ( i != ( size - 1 ) ) ? 1 : 2;
						}
						if ( j != 0 )
						{
							index += 3 * ( ( j != ( size - 1 ) ) ? 1 : 2 );
						}
						cout << dstr[ index ];
					}
					break;
				}
			}
			cout << endl;
		}

		for ( int dir = 0 ; dir < Board::NumDir ; ++dir )
		{
			int nx = x + gDirOffset[dir][0];
			int ny = y + gDirOffset[dir][1];

			if ( !mBoard.checkRange( nx , ny ) )
				continue;
			DataType nType = mBoard.getData( nx , ny );
			if ( nType == EStoneColor::Empty )
				continue;
			int life = mBoard.getCaptureCount( nx , ny ) ;
			std::cout << "dir =" << dir << " life = "<< life << std::endl;
		}
	}


	bool Game::isKOStateReached(Pos const& pos, DataType playColor, KOState const& koState) const
	{
		if (koState == KOState::Invalid())
			return false;

		if (mSimpleKOStates.size() < 2)
			return false;

		return koState == mSimpleKOStates[mSimpleKOStates.size() - 1];
	}

	Game::KOState Game::calcKOState(Pos const& pos) const
	{
		return{ pos.toIndex() , 0 };
	}

	void Game::addKOState(DataType playColor, Pos const* pos,  int numCapture, KOState const& koState)
	{
		mSimpleKOStates.push_back(koState);
	}

	void Game::removeKOState(DataType playColor , Pos const* pos)
	{
		mSimpleKOStates.pop_back();
	}

	void Game::takeData(SocketBuffer& buffer)
	{
		unsigned size = mStepHistory.size();
		buffer.fill( size );

		for(Go::Game::StepInfo & info : mStepHistory)
		{
			buffer.fill( info.idxPos );
		}
	}

	bool Game::restoreData( SocketBuffer& buffer )
	{
		mStepHistory.clear();

		restart();

		unsigned size;
		buffer.take( size );

		for( int i = 0 ; i < size ; ++i )
		{
			int idxPos;
			buffer.take( idxPos );
			if ( idxPos == -1 )
			{
				playPass();
			}
			else if ( playStone( mBoard.getPos( idxPos ) ) )
			{
				return false;
			}
		}
		return true;
	}

	bool Game::saveSGF( char const* path , GameDescription const* description ) const
	{
		std::ofstream fs( path );
		if ( !fs.is_open() )
			return false;

		fs << "(;";
		fs << "FF[4]";
		fs << "CA[UTF-8]";

		if( description )
		{
			fs << "PB[" << description->blackPlayer << ']';
			fs << "PW[" << description->whitePlayer << ']';
			fs << "DT[" << description->date << ']';
			if ( !description->mathResult.empty() )
				fs << "RE[" << description->mathResult << ']';
		}

		fs << "SZ[" << mBoard.getSize() << ']';
		fs << "KM[" << mSetting.komi << ']';

		bool bBlack = mSetting.bBlackFirst;

		for( StepInfo const& step : mStepHistory )
		{
			if ( step.bPlay )
			{
				fs << ';' << ((bBlack) ? 'B' : 'W');
				if( step.idxPos != -1 )
				{
					int coord[2];
					mBoard.getPosCoord(step.idxPos, coord);
					fs << '[' << char('a' + coord[0]) << char('a' + coord[1]) << ']';
				}
				else
				{
					fs << "[tt]";
				}
				bBlack = !bBlack;
			}
			else
			{
				fs << ';' << ((step.colorAdded == EStoneColor::Black) ? 'AB' : 'AW');

				int coord[2];
				mBoard.getPosCoord(step.idxPos, coord);
				fs << '[' << char('a' + coord[0]) << char('a' + coord[1]) << ']';
			}
		}

		fs << ')';
		return true;
	}

	PlayVertex Game::getStepPos(int step) const
	{
		if( 0 > step || step >= mStepHistory.size()  )
			return PlayVertex::Undefiend();

		StepInfo const& info = mStepHistory[step];

		if( info.idxPos == -1 )
			return PlayVertex::Pass();
		int outPos[2];
		mBoard.getPosCoord(info.idxPos, outPos);

		return PlayVertex::OnBoard(outPos[0], outPos[1]);
	}

	int Game::getLastPassCount() const
	{
		int count = 0;
		for( int i = mStepHistory.size() - 1; i >= 0; --i )
		{
			if( mStepHistory[i].idxPos >= 0 )
				break;

			++count;
		}
		return count;
	}

	bool Game::isReviewing() const
	{
		return mCurrentStep < mStepHistory.size();
	}

	int Game::reviewBeginStep()
	{
		int result = mCurrentStep;
		doRestart(true, false);
		return result;
	}

	int Game::reviewPrevSetp(int numStep /*= 1*/)
	{
		int result = 0;
		for( int i = 0; i < numStep; ++i )
		{
			if( !undoInternal(true) )
				break;

			++result;
		}
		return result;
	}

	void Game::advanceStepFromHistory()
	{
		assert(mCurrentStep < mStepHistory.size());
		StepInfo const& info = mStepHistory[mCurrentStep];
		if( info.bPlay )
		{
			if( info.idxPos == -1 )
			{
				playPassInternal(true);
			}
			else
			{
				playStoneInternal(mBoard.getPos(info.idxPos), true);
			}
		}
		else
		{
			addStoneInternal(mBoard.getPos(info.idxPos), info.colorAdded, true);
		}
	}

	int Game::reviewNextStep(int numStep /*= 1*/)
	{
		int result = 0;
		for( int i = 0; i < numStep; ++i )
		{
			if( mCurrentStep >= mStepHistory.size() )
				break;
			advanceStepFromHistory();
			++result;
		}
		return result;
	}

	int Game::reviewLastStep()
	{
		int result = 0;
		while( mCurrentStep < mStepHistory.size() )
		{
			advanceStepFromHistory();
			++result;
		}
		return result;
	}


	struct BranchInfo
	{
		int parent;
	};


	static constexpr uint32 HashValue(StringView str)
	{
		return FNV1a::MakeStringHash<uint32>(str.data(), str.length()) & 0xff;
	}

	bool IGameCopier::LoadSGF(char const*path, IGameCopier& copier, GameDescription* des )
	{
		std::vector<uint8> text;
		if (!FFileUtility::LoadToBuffer(path, text , true))
		{
			return false;
		}


		int step = -1;
		TArray< int > branchStack;
		Tokenizer tokenizer((char const*)text.data() , " \r\n\t","[]();" );

		GameSetting setting;

		StringView token;

		int fileFormat = 0;

		for(;;)
		{
			auto tokenType = tokenizer.take(token);

			if (tokenType == EStringToken::None)
			{
				break;
			}
			else if (tokenType == EStringToken::Delim)
			{
				switch (token[0])
				{
				case ';':
					{
						if (step == 0)
						{
							copier.emitSetup(setting);
						}
						++step;
					}
					break;
				case '(':
					{
						branchStack.push_back(step);
					}
					break;
				case ')':
					{
						//we don't care branch so return
						return true;
						branchStack.pop_back();
					}
					break;
				}

			}
			else
			{
				InlineString<32> command =(const char *)token.toCString();
				uint32 hashCommand = HashValue(token);

				if (!tokenizer.takeChar('['))
				{
					return false;
				}

				for(;;)
				{
					if (!tokenizer.takeUntil(']', token))
					{
						return false;
					}
					auto PlayStone = [&](int color)
					{
						int x = token[0] - 'a';
						int y = token[1] - 'a';
						if (x == 19 && y == 19)
						{
							copier.emitPlayPass(color);
						}
						else
						{
							copier.emitPlayStone(x, y, color);
						}			
					};
					auto AddStone = [&](int color)
					{
						int x = token[0] - 'a';
						int y = token[1] - 'a';
						copier.emitAddStone(x, y, color);
					};

					LogDevMsg(0, "%s : %s", command.c_str(), (const char *)token.toCString());
#define STR_HASH(STR) HashValue(STR)
					switch (hashCommand)
					{
					case STR_HASH("SZ"):
						setting.boardSize = token.toValue<int>();
						break;
					case STR_HASH("HA"):
						setting.numHandicap = token.toValue<int>();
						setting.bFixedHandicap = false;
						setting.bBlackFirst = setting.numHandicap == 0;
						break;
					case STR_HASH("KM"):
						setting.komi = token.toValue<int>() / 100.0f;
						break;
					case STR_HASH("AB"):
						{
							if (token.size() != 2)
							{
								return false;
							}

							AddStone(EStoneColor::Black);
						}
						break;
					case STR_HASH("AW"):
						{
							if (token.size() != 2)
							{
								return false;
							}
							AddStone(EStoneColor::White);
						}
						break;
					case STR_HASH("AE"):
						{
							if (token.size() != 2)
							{
								return false;
							}
							AddStone(EStoneColor::Empty);
						}
						break;
					case STR_HASH("B"):
						{
							if (token.size() != 2)
							{
								return false;
							}

							PlayStone(EStoneColor::Black);
						}
						break;
					case STR_HASH("W"):
						{
							if (token.size() != 2)
							{
								return false;
							}
							PlayStone(EStoneColor::White);
						}
						break;
					case STR_HASH("GM"): //Game Type
						{
							int gameType = token.toValue<int>();
							if (gameType != 1)
							{
								return false;
							}
						}
						break;
					case STR_HASH("GN"): //Game Name
						if (des)
						{
							des->gameName = token.toStdString();
						}
						break;
					case STR_HASH("DT"): //date
						if (des)
						{
							des->date = token.toStdString();
						}
						break;
					case STR_HASH("PB"): //black player
						if (des)
						{
							des->blackPlayer = token.toStdString();
						}
						break;
					case STR_HASH("WB"): //white player
						if (des)
						{
							des->whitePlayer = token.toStdString();
						}
						break;
					case STR_HASH("AP"):
						break;
					case STR_HASH("RU"): //rule
						break;
					case STR_HASH("C"): //comment
						break;
					case STR_HASH("RE"): 
						break;
					case STR_HASH("RL"):
						break;
					case STR_HASH("TC"): //Territory count
						break;
					case STR_HASH("TM"): //Time limit
						break;
					case STR_HASH("TT"):
						break;
					case STR_HASH("FF"):
						fileFormat = token.toValue<int>();
						break;

					}
#undef STR_HASH
					if (!tokenizer.takeChar('['))
					{
						break;
					}
				}
			}
		}


		return true;
	}



}//namespace Go