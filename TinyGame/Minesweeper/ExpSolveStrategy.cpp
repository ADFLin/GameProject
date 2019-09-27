#include "ExpSolveStrategy.h"

#include <cassert>
#include <algorithm>
#include <iostream>

namespace Mine
{
	bool gUseDbgMethod = true;

#define DBG_STOP( expr )\
	if ( expr ){ int _LINE_##StopVar = 0; }

	int const NumNeighborDir = 8;
	int const NumGapDir = 12;

	int const gNeighborOffset[NumNeighborDir][2] =
	{
		{ 1 , 0 },{ 1 , 1 },{ 0 , 1 },{ -1 , 1 },
		{ -1 , 0 },{ -1 , -1 },{ 0 , -1 },{ 1 , -1 },
	};

	int const gGapOffset[NumGapDir][2] = {
		{ 2 , 0 } , { 2 , 1 } , { 2 , -1 } ,
		{ -2 , 0 } ,{ -2 , 1 } ,{ -2 , -1 } ,
		{ 0 , 2 } ,{ 1 , 2 } ,{ -1 , 2 } ,
		{ 0 , -2 } ,{ 1 , -2 } ,{ -1 , -2 } ,
	};


	ExpSolveStrategy::ExpSolveStrategy()
		:mCacheProbs(1)
	{
		mSettingFlag = 0;
		mSolveState = eLogicSolve;
		mNumSolvedBomb = 0;
		mNumOpenCell = 0;
		mLastCheckPos.x = mLastCheckPos.y = -1;
	}

	void ExpSolveStrategy::restoreCell()
	{
		mCacheProbs.resize(1);
		mIdxFreeProb = 0;
		mCheckList.clear();

		mNumOpenCell = 0;
		mNumSolvedBomb = 0;
		int num = mCellSizeX * mCellSizeY;
		for( int i = 0; i < num; ++i )
		{
			CellData& data = mCellData[i];
			data.number = CV_UNPROBLED;
			data.numBomb = 0;
			data.numSolved = 0;
			data.numNeighbor = getNeighbourNum(i % mCellSizeX, i / mCellSizeX);
			data.flag = 0;
			data.idxProb = -1;
		}
	}

	void ExpSolveStrategy::setRule(IMineMap& map)
	{
		mCellSizeX = map.getSizeX();
		mCellSizeY = map.getSizeY();
		mNumTotalBomb = map.getBombNum();
		mCellData.resize(mCellSizeX * mCellSizeY);
	}

	void ExpSolveStrategy::refreshInformation(IMineMap& map, bool beRestart)
	{
		mSolveState = eLogicSolve;

		setRule(map);
		mMineMap = &map;

		restoreCell();

		if( !beRestart )
		{
			CellPos cp;
			cp.idx = 0;
			cp.y = 0;
			for( short& j = cp.y; j < mCellSizeY; ++j )
			{
				cp.x = 0;
				for( short& i = cp.x; i < mCellSizeX; ++i, ++cp.idx )
				{
					int num = map.look(i, j, false);
					if( num != CV_UNPROBLED )
					{
						addCellInformation(cp, num);
					}
				}
			}
		}
	}


	void ExpSolveStrategy::addCellInformation(CellPos const& cp, int num)
	{
		assert(num != CV_UNPROBLED);

		CellData& data = getCellData(cp);
		data.number = num;

		float accProb = 0;

		if( num != CV_FLAG )
		{
			++mNumOpenCell;
			addCheckPos(cp);
			for( int dir = 0; dir < NumNeighborDir; ++dir )
			{
				CellPos ncp;
				if( !makeNeighborPos(cp, ncp, dir) )
					continue;

				addCheckPos(ncp);
				CellData& ndata = getCellData(ncp);
				++ndata.numSolved;

				if( mSolveState == eProbSolve && ndata.number == CV_UNPROBLED &&
				   data.number != 0 )
				{
					bool needSub = (ndata.idxProb != -1);
					ProbInfo& info = fetchProbInfo(ncp);

					if( needSub )
						accProb -= info.prob;
					else
						mSortedProbCells.push_back(&ndata);

					calcCellProb(ncp, info);
					assert(info.num);
					accProb += info.prob;
				}
			}

			if( mSolveState == eProbSolve )
			{
				accProb += (mNumTotalBomb - mNumSolvedBomb) - getOtherProbInfo().prob * getOtherProbInfo().num;
				if( data.idxProb != -1 )
				{
					ProbInfo& info = fetchProbInfo(cp);
					accProb -= info.prob;
					releaseProbInfo(cp);
				}
				updateOtherPropInfo(accProb);
			}
		}
		else
		{
			++mNumSolvedBomb;
			for( int dir = 0; dir < NumNeighborDir; ++dir )
			{
				CellPos ncp;
				if( !makeNeighborPos(cp, ncp, dir) )
					continue;

				addCheckPos(ncp);
				CellData& ndata = getCellData(ncp);
				++ndata.numBomb;
				++ndata.numSolved;
			}
		}

		for( int dir = 0; dir < NumGapDir; ++dir )
		{
			CellPos ncp;
			if( !makeGapPos(cp, ncp, dir) )
				continue;
			addCheckPos(ncp);
		}
	}

	void ExpSolveStrategy::geerateCellProbConstraint()
	{
		for( size_t i = 0; i < mSortedProbCells.size(); ++i )
		{
			CellData* data = mSortedProbCells[i];
			if( data->idxProb == -1 )
				continue;

			ProbInfo& info = mCacheProbs[data->idxProb];
			for( int n = 0; n < info.num; ++n )
			{
				CellData& cdata = mCellData[info.cellCT[n]];
			}
		}
	}
	void ExpSolveStrategy::geerateCellProbSimple()
	{
		float accProb = 0;

		mSortedProbCells.clear();
		mUnknownProbCells.clear();

		CellPos cp;
		cp.idx = 0;
		cp.y = 0;
		for( short& j = cp.y; j < mCellSizeY; ++j )
		{
			cp.x = 0;
			for( short& i = cp.x; i < mCellSizeX; ++i, ++cp.idx )
			{
				CellData& data = getCellData(cp);
				bool releaseProb = true;

				if( data.number == CV_UNPROBLED )
				{
					ProbInfo& info = fetchProbInfo(cp);

					if( calcCellProb(cp, info) != 0 )
					{
						mSortedProbCells.push_back(&data);
						accProb += info.prob;
						releaseProb = false;
					}
					else
					{
						mUnknownProbCells.push_back(&data);
					}
				}

				if( releaseProb )
					releaseProbInfo(cp);
			}
		}
		updateOtherPropInfo(accProb);
	}
	void ExpSolveStrategy::generateCellProb()
	{
		geerateCellProbSimple();
	}

	void ExpSolveStrategy::updateOtherPropInfo(float accProb)
	{
		ProbInfo& info = mCacheProbs.front();
		info.idx = -1;
		info.num = getCellSizeX() * getCellSizeY() - mNumSolvedBomb - mNumOpenCell - mSortedProbCells.size();
		if( info.num )
			info.prob = (mNumTotalBomb - mNumSolvedBomb - accProb) / info.num;
		else
			info.prob = 0;
	}

	int ExpSolveStrategy::calcCellProb(CellPos const& cp, ProbInfo &info)
	{
		int   num = 0;
		float totalProb = 0.0f;
		for( int dir = 0; dir < NumNeighborDir; ++dir )
		{
			CellPos ncp;
			if( !makeNeighborPos(cp, ncp, dir) )
				continue;

			CellData& ndata = getCellData(ncp);
			if( ndata.number <= 0 )
				continue;

			totalProb += float(ndata.number - ndata.numBomb) / (ndata.numNeighbor - ndata.numSolved);
			info.cellCT[num] = ncp.idx;
			++num;
		}

		info.num = num;
		if( num )
			info.prob = totalProb / num;

		return num;
	}

	int ExpSolveStrategy::getNeighbourNum(int cx, int cy)
	{
		static int const numEighbour[] = { 8 , 5 , 3 };
		int side = 0;
		if( cx == 0 || cx == mCellSizeX - 1 )
			++side;
		if( cy == 0 || cy == mCellSizeY - 1 )
			++side;
		return numEighbour[side];
	}

	void ExpSolveStrategy::openCell(CellPos const& cp)
	{
		int num = mMineMap->probe(cp.x, cp.y);
		//if ( num >= 0 )
		{
			scanCellInternal(cp, num);
		}
	}


	void ExpSolveStrategy::openCell(int cx, int cy)
	{
		CellPos cp;
		if( !makeCellPos(cp, cx, cy) )
			return;

		openCell(cp);
	}

	void ExpSolveStrategy::markCell(CellPos const& cp)
	{
		if( mMineMap->mark(cp.x, cp.y) )
		{
			addCellInformation(cp, CV_FLAG);
		}
	}

	bool ExpSolveStrategy::checkPos(int cx, int cy)
	{
		CellPos cp;

		if( !makeCellPos(cp, cx, cy) )
			return false;

		gUseDbgMethod = true;
		bool result = solveLogic(cp);
		gUseDbgMethod = false;

		return result;
	}

	void ExpSolveStrategy::scanCellInternal(CellPos const& cp, int num)
	{
		if( num == CV_UNPROBLED )
		{
			assert("hook error");
			return;
		}

		addCellInformation(cp, num);

		if( num == 0 )
		{
			for( int dir = 0; dir < NumNeighborDir; ++dir )
			{
				CellPos ncp;
				if( !makeNeighborPos(cp, ncp, dir) )
					continue;
				scanCell(ncp);
			}
		}

		if( mSolveState == eProbSolve )
		{
			int swapIndex;

#define FILTER_VECTOR( VECTOR , VAR , EXPR )\
	swapIndex = VECTOR.size() - 1;\
	for( int VAR = 0 ; VAR <= swapIndex ; ++VAR )\
	{\
		if ( EXPR )\
		{\
			std::swap( VECTOR[VAR] , VECTOR[swapIndex] );\
			--swapIndex;\
		}\
	}\
	VECTOR.resize( swapIndex + 1 );

			FILTER_VECTOR(mUnknownProbCells, i, mUnknownProbCells[i]->idxProb != -1)
				FILTER_VECTOR(mSortedProbCells, i, mSortedProbCells[i]->idxProb == -1)

#undef  FILTER_VECTOR
		}

	}

	void ExpSolveStrategy::scanCell(CellPos const& cp)
	{
		if( mCellData[cp.idx].number != CV_UNPROBLED )
			return;

		DBG_STOP(cp.x == 2 && cp.y == 4)
			int num = mMineMap->look(cp.x, cp.y, true);
		scanCellInternal(cp, num);
	}


	void ExpSolveStrategy::print()
	{
		using namespace std;
		int idx = 0;
		for( int j = 0; j < mCellSizeY; ++j )
		{
			cout << "[";
			for( int i = 0; i < mCellSizeX; ++i, ++idx )
			{
				switch( mCellData[idx].number )
				{
				case CV_FLAG:    cout << "M"; break;
				case CV_UNPROBLED: cout << " "; break;
				case 0:           cout << "o"; break;
				default:
					cout << mCellData[idx].number;
				}
			}
			cout << "]" << endl;
		}

		idx = 0;
		for( int j = 0; j < mCellSizeY; ++j )
		{
			cout << "[";
			for( int i = 0; i < mCellSizeX; ++i, ++idx )
			{
				CellData& data = mCellData[idx];
				switch( data.number )
				{
				case CV_FLAG:    cout << "M"; break;
				case CV_UNPROBLED: cout << " "; break;
				case 0:           cout << "o"; break;
				default:
					cout << data.numSolved;
				}
			}
			cout << "]" << endl;
		}
	}

	void ExpSolveStrategy::addCheckPos(CellPos const& cp)
	{
		CellData& data = getCellData(cp);

		if( data.flag & eCheck )
			return;
		// 0 bomb unknown
		if( data.number <= 0 )
			return;
		if( data.numNeighbor == data.numSolved )
			return;

		mCheckList.push_back(cp);
		data.flag |= eCheck;
	}

	bool ExpSolveStrategy::solveLogic(CellPos const& cp)
	{
		CellData& data = getCellData(cp);
		data.flag &= ~eCheck;

		if( data.number == 0 )
			return false;

		if( data.numNeighbor == data.numSolved )
			return false;

		if( execBombClearMethod(cp) )
			return true;
		if( execUniqueMethod(cp) )
			return true;

		for( int dir = 0; dir < NumNeighborDir; ++dir )
		{
			CellPos ncp;
			if( !makeNeighborPos(cp, ncp, dir) )
				continue;

			CellData& ndata = getCellData(ncp);

			if( ndata.number <= 0 )
				continue;

			if( execShareCellMethod(cp, ncp, dir) )
				return true;
		}

		for( int dir = 0; dir < NumGapDir; ++dir )
		{
			CellPos ncp;
			if( !makeGapPos(cp, ncp, dir) )
				continue;

			calcIndex(ncp);
			CellData& ndata = getCellData(ncp);

			if( ndata.number <= 0 )
				continue;

			if( execShareCellMethod(cp, ncp, -dir) )
				return true;
		}
		return false;
	}


	bool ExpSolveStrategy::solveProb()
	{
		int idxSelect = -1;

		ProbInfo const& otherInfo = getOtherProbInfo();

		if( !mSortedProbCells.empty() )
		{
			struct SortCmp
			{
				SortCmp(std::vector< ProbInfo >& probs) : probs(probs) {}

				bool operator ()(CellData* c1, CellData* c2)
				{
					if( c1->idxProb == -1 )
						return true;
					if( c2->idxProb == -1 )
						return false;
					ProbInfo& info1 = probs[c1->idxProb];
					ProbInfo& info2 = probs[c2->idxProb];

					if( info1.prob < info2.prob )
						return true;
					if( info1.prob > info2.prob )
						return false;

					return info1.num > info2.num;
				}
				std::vector< ProbInfo >& probs;
			};

			std::sort(mSortedProbCells.begin(), mSortedProbCells.end(), SortCmp(mCacheProbs));


			ProbInfo& minInfo = mCacheProbs[mSortedProbCells.front()->idxProb];

			if( otherInfo.num == 0 || minInfo.prob <= otherInfo.prob )
			{
				size_t num = 1;
				for( ; num < mSortedProbCells.size(); ++num )
				{
					if( mSortedProbCells[num]->idxProb == -1 )
						break;

					ProbInfo& info = mCacheProbs[mSortedProbCells[num]->idxProb];
					if( info.prob > minInfo.prob || info.num < minInfo.num )
						break;
				}
				CellData* data = mSortedProbCells[rand() % num];
				idxSelect = data - &mCellData[0];

			}
			else if( !mSortedProbCells.empty() )
			{
				float minProp = 1;
				for( size_t i = 0; i < mSortedProbCells.size(); ++i )
				{
					ProbInfo& info = mCacheProbs[mSortedProbCells[i]->idxProb];

					if( info.prob > minProp )
						continue;

					int idx = info.idx;

					CellPos cp;
					cp.x = idx % getCellSizeX();
					cp.y = idx / getCellSizeX();
					cp.idx = idx;

					for( int dir = 0; dir < NumNeighborDir; ++dir )
					{
						CellPos ncp;
						if( !makeNeighborPos(cp, ncp, dir) )
							continue;

						CellData& ndata = getCellData(ncp);
						if( ndata.number == CV_UNPROBLED && ndata.idxProb == -1 )
						{
							minProp = info.prob;
							idxSelect = ncp.idx;
							break;
						}
					}
				}
			}
		}

		if( idxSelect == -1 && mUnknownProbCells.size() > 0 )
		{
			int size = getCellSizeX() * getCellSizeY();
			CellData* data = mUnknownProbCells[rand() % mUnknownProbCells.size()];
			idxSelect = data - &mCellData[0];
		}

		if( idxSelect != -1 )
		{
			CellPos& cp = mPorbSelect;

			cp.x = idxSelect % getCellSizeX();
			cp.y = idxSelect / getCellSizeX();
			cp.idx = idxSelect;
			openCell(cp);
			return true;
		}

		return false;

	}


	bool ExpSolveStrategy::execBombClearMethod(CellPos const& cp)
	{
		CellData& data = getCellData(cp);
		if( data.numBomb != data.number )
			return false;

		//if ( mSettingFlag & ST_FAST_OPEN_CELL )
		//{
		//	try
		//	{
		//		mControler->openNeighberCell( cp.x , cp.y );
		//	}
		//	catch (HookException& )
		//	{

		//	}
		//}

		int count = 0;
		int check = data.numNeighbor - data.numSolved;
		for( int dir = 0; dir < NumNeighborDir; ++dir )
		{
			CellPos ncp;
			if( !makeNeighborPos(cp, ncp, dir) )
				continue;

			CellData& ndata = getCellData(ncp);
			if( ndata.number == CV_UNPROBLED )
			{
				++count;
				//if ( mSettingFlag & ST_FAST_OPEN_CELL )
				//{
				//	scanCell( ncp );
				//}
				//else
				{
					openCell(ncp);
				}
			}
		}

		if( count != check )
		{

		}
		return true;

	}

	bool ExpSolveStrategy::execUniqueMethod(CellPos const& cp)
	{
		CellData& data = getCellData(cp);

		if( data.numNeighbor - data.numSolved != data.number - data.numBomb )
			return false;

		int count = 0;
		int check = data.number - data.numBomb;
		for( int dir = 0; dir < NumNeighborDir; ++dir )
		{
			CellPos ncp;
			if( !makeNeighborPos(cp, ncp, dir) )
				continue;

			CellData& ndata = getCellData(ncp);

			if( ndata.number == CV_UNPROBLED )
			{
				++count;
				markCell(ncp);
			}
		}

		assert(count == check);
		return true;

	}


	bool ExpSolveStrategy::isPosIncluded(CellPos const cpList[], int num, CellPos const& cp)
	{
		for( int i = 0; i < num; ++i )
		{
			if( cpList[i].idx == cp.idx )
				return true;
		}
		return false;
	}

	bool ExpSolveStrategy::testNeighbor(CellPos const& cp1, CellPos const& cp2)
	{
		int dx = cp1.x - cp2.x;
		int dy = cp1.y - cp2.y;

		if( dx > 1 || dx < -1 )
			return false;
		if( dy > 1 || dy < -1 )
			return false;

		return true;
	}
	bool ExpSolveStrategy::execHiddenBombMethod(CellPos const& cp, int skipDir, HiddenParam& param)
	{
		for( int dir = 0; dir < NumNeighborDir; ++dir )
		{
			if( dir == skipDir )
				continue;
			CellPos ncp;
			if( !makeNeighborPos(cp, ncp, dir) )
				continue;

			CellData& ndata = getCellData(ncp);

			if( execHiddenBombInternal(cp, ncp, dir, param) )
				return true;
		}

		for( int dir = 0; dir < NumGapDir; ++dir )
		{
			if( dir == skipDir )
				continue;

			CellPos ncp;
			if( !makeGapPos(cp, ncp, dir) )
				continue;

			CellData& ndata = getCellData(ncp);

			if( execHiddenBombInternal(cp, ncp, -dir, param) )
				return true;
		}
		return false;
	}

	bool ExpSolveStrategy::execHiddenBombInternal(CellPos const& cp, CellPos const& ncp, int ndir, HiddenParam &param)
	{
		if( getCellData(ncp).number <= 0 )
			return false;

		if( isPosIncluded(param.lockPos, param.numLock, ncp) )
			return false;

		CellPos sharePos[NumNeighborDir];
		int numShare = getShareFreeNeighbor(cp, ncp, sharePos);

		int curIdx = 0;
		for( int i = 0; i < numShare; ++i )
		{
			if( isPosIncluded(param.lockPos, param.numLock, sharePos[i]) )
				continue;

			if( i != curIdx )
				sharePos[curIdx] = sharePos[i];
			++curIdx;
		}
		if( curIdx == 0 )
			return false;

		param.numOverlap = numShare - curIdx;
		numShare -= param.numOverlap;

		return execShareCellInternal(cp, ncp, sharePos, numShare, ndir, &param);
	}

	bool ExpSolveStrategy::execShareCellInternal(CellPos const& cp, CellPos const& ncp, CellPos const sharePos[], int numShare, int dir, HiddenParam* param)
	{
		if( numShare == 0 )
			return false;

		CellData& data = getCellData(cp);
		CellData& ndata = getCellData(ncp);

		int bf = data.number - data.numBomb;
		int fc = data.numNeighbor - data.numSolved;

		if( param )
		{
			bf -= param->numBomb;
			fc -= param->numLock;
		}

		int nbf = ndata.number - ndata.numBomb;
		int nfc = ndata.numNeighbor - ndata.numSolved;

		DBG_STOP(cp.idx == 138 && ncp.idx == 168)

			enum Action
		{
			OPEN,
			MARK,
		};
		Action   action;
		CellPos const* rcp = NULL;

		if( !param || param->numOverlap == 0 )
		{
			if( fc - numShare == bf - nbf && bf != nbf )
			{
				action = MARK;
				rcp = &cp;
			}
			else if( nfc - numShare == nbf - bf && bf != nbf )
			{
				action = MARK;
				rcp = &ncp;
			}
		}

		if( !rcp )
		{
			if( nfc == numShare && nbf <= numShare )
			{
				if( bf <= nbf )
				{
					action = OPEN;
					rcp = &cp;
				}
				else if( !param )
				{
					HiddenParam param;
					param.numBomb = nbf;
					param.numLock = numShare;
					param.lockPos = &sharePos[0];
					if( execHiddenBombMethod(cp, dir, param) )
						return true;
				}
			}
			else if( fc == numShare && bf <= numShare )
			{
				if( nbf <= bf )
				{
					action = OPEN;
					rcp = &ncp;
				}
				else if( !param )
				{
					HiddenParam param;
					param.numBomb = bf;
					param.numLock = numShare;
					param.lockPos = &sharePos[0];
					if( execHiddenBombMethod(ncp, (dir + NumNeighborDir / 2) % NumNeighborDir, param) )
						return true;
				}
			}
		}

		if( rcp )
		{
			for( int dir = 0; dir < NumNeighborDir; ++dir )
			{
				CellPos mcp;
				if( !makeNeighborPos(*rcp, mcp, dir) )
					continue;
				if( getCellData(mcp).number != CV_UNPROBLED )
					continue;
				if( isPosIncluded(sharePos, numShare, mcp) )
					continue;
				if( param && isPosIncluded(param->lockPos, param->numLock, mcp) )
					continue;

				switch( action )
				{
				case MARK: markCell(mcp); break;
				case OPEN: openCell(mcp); break;
				}
			}
			return true;
		}

		return false;

	}
	bool ExpSolveStrategy::execShareCellMethod(CellPos const& cp, CellPos const& ncp, int dir)
	{
		CellPos sharePos[NumNeighborDir];
		int numShare = getShareFreeNeighbor(cp, ncp, sharePos);
		return execShareCellInternal(cp, ncp, sharePos, numShare, dir, NULL);
	}

	int ExpSolveStrategy::getShareFreeNeighbor(CellPos const& p1, CellPos const& p2, CellPos share[])
	{
		int result = 0;
		for( int dir = 0; dir < NumNeighborDir; ++dir )
		{
			CellPos& sharePos = share[result];

			if( !makeNeighborPos(p1, sharePos, dir) )
				continue;

			if( sharePos.idx == p2.idx )
				continue;

			if( !testNeighbor(sharePos, p2) )
				continue;

			if( mCellData[sharePos.idx].number != CV_UNPROBLED )
				continue;

			++result;
		}
		return result;
	}

	bool ExpSolveStrategy::makeCellPos(CellPos& cp, int cx, int cy)
	{
		if( !isVaildRange(cx, cy) )
			return false;
		cp.x = cx;
		cp.y = cy;
		calcIndex(cp);
		return true;
	}


	bool ExpSolveStrategy::makeGapPos(CellPos const& cp, CellPos& ncp, int dir)
	{
		return makeCellPos(ncp,
						   cp.x + gGapOffset[dir][0],
						   cp.y + gGapOffset[dir][1]);
	}

	bool ExpSolveStrategy::makeNeighborPos(CellPos const& cp, CellPos& ncp, int dir)
	{
		return makeCellPos(ncp,
						   cp.x + gNeighborOffset[dir][0],
						   cp.y + gNeighborOffset[dir][1]);
	}

	ExpSolveStrategy::ProbInfo& ExpSolveStrategy::fetchProbInfo(CellPos const& cp)
	{
		CellData& data = getCellData(cp);

		ProbInfo* pInfo;
		if( data.idxProb == -1 )
		{
			if( mIdxFreeProb )
			{
				data.idxProb = mIdxFreeProb;
				pInfo = &mCacheProbs[data.idxProb];
				mIdxFreeProb = pInfo->idx;
			}
			else
			{
				data.idxProb = mCacheProbs.size();
				mCacheProbs.push_back(ProbInfo());
				pInfo = &mCacheProbs.back();
			}
			pInfo->idx = cp.idx;
		}
		else
		{
			pInfo = &mCacheProbs[data.idxProb];
		}

		return *pInfo;
	}

	void ExpSolveStrategy::releaseProbInfo(CellPos const& cp)
	{
		CellData& data = getCellData(cp);

		if( data.idxProb == -1 )
			return;

		mCacheProbs[data.idxProb].idx = mIdxFreeProb;
		mIdxFreeProb = data.idxProb;
		data.idxProb = -1;
	}


	bool ExpSolveStrategy::solveStep() /*throw ( HookException )*/
	{
		int numSolved = mNumSolvedBomb + mNumOpenCell;
		if( numSolved == 0 )
		{
			CellPos& cp = mPorbSelect;
			cp.x = rand() % getCellSizeX();
			cp.y = rand() % getCellSizeY();
			calcIndex(cp);
			openCell(cp);
			return true;
		}
		else if( numSolved == getCellSizeX() * getCellSizeY() )
		{
			mCheckList.clear();
			return false;
		}

		bool logicSucceed = false;
		while( !mCheckList.empty() )
		{
			mLastCheckPos = mCheckList.back();
			mCheckList.pop_back();
			try
			{
				logicSucceed = solveLogic(mLastCheckPos);
			}
			catch( ControlException& e )
			{
				mCheckList.push_back(mLastCheckPos);
				throw e;
			}
			if( logicSucceed )
				break;
		}

		if( !logicSucceed )
		{
			if( mSolveState == eLogicSolve )
				generateCellProb();
			mSolveState = eProbSolve;
		}
		else
		{
			mSolveState = eLogicSolve;
		}

		if( mSolveState == eProbSolve )
		{
			//if ( mSettingFlag & ST_DISABLE_PROB_SOLVE )
			//	return false;

			if( solveProb() )
				return true;
		}
		return false;
	}

}//namespace Mine


