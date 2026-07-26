#include "RubiksStage.h"
#include "StageRegister.h"
#include "ProfileSystem.h"
#include "SystemPlatform.h"

namespace Rubiks
{

	void Solver::run()
	{
		TIME_SCOPE("Solve");

		if ( mInitState.isEqual( mFinalState ) )
			return;

		cleanup();
		buildGoalDepthTable(5);
		CubeState initCanonical;
		mInitState.buildSymmetryCanonicalState(initCanonical);
		if ( isInGoalDepthTable(initCanonical) )
			return;

		class FindRunable : public RunnableThreadT< FindRunable >
		{
		public:
			unsigned run()
			{
				solver->run_FindThread();
				return 0;
			}
			void exit(){ delete this; }
			Solver* solver;
		};

		mbRunning = true;
		FindRunable* findWork = new FindRunable;
		findWork->solver = this;
		findWork->start();

		{
			StateNode* node = new StateNode;
			node->state = mInitState;
			node->state.buildSymmetryCanonicalState(node->canonicalState);
			node->parent = nullptr;
			node->depth = 0;
			Mutex::Locker locker( mRequestFindMutex );
			mRequestFindNodes.push_back( node );
			mRequestFindCond.notifyOne();
		}

		int logDepth = INDEX_NONE;
		int64 depthStartTime = 0;
		int depthTotalCount = 0;
		int depthAcceptedCount = 0;
		int depthDuplicateCount = 0;

		auto FlushDepthLog = [&]()
		{
			if ( logDepth == INDEX_NONE )
				return;

			int64 elapsedTime = SystemPlatform::GetTickCount() - depthStartTime;
			LogMsg("Rubiks Solver Depth %d : time = %lld ms , total = %d , accepted = %d , duplicate = %d",
			       logDepth, elapsedTime, depthTotalCount, depthAcceptedCount, depthDuplicateCount);
		};

		for(;;)
		{
			StateNode* node = nullptr;
			{
				Mutex::Locker locker(mUncheckMutex);
				mUncheckCond.wait(locker , fastdelegate::FastDelegate< bool () >( this , &Solver::haveUncheck ) );

				if ( mbRunning == false )
					break;

				node = mUncheckNodes.front();
				mUncheckNodes.pop_front();
			}

			if ( node == nullptr )
				break;

			if ( node->depth != logDepth )
			{
				FlushDepthLog();
				logDepth = node->depth;
				depthStartTime = SystemPlatform::GetTickCount();
				depthTotalCount = 0;
				depthAcceptedCount = 0;
				depthDuplicateCount = 0;
			}

			++depthTotalCount;

			if ( mCheckedStates.find( &node->canonicalState ) == mCheckedStates.end() )
			{
				if ( mFinalState.isEqual( node->state ) || isInGoalDepthTable(node->canonicalState) )
				{
					++depthAcceptedCount;
					FlushDepthLog();
					solveSuccess();
					return;
				}
				mCheckedStates.insert( &node->canonicalState );
				++depthAcceptedCount;
				{
					Mutex::Locker locker( mRequestFindMutex );
					mRequestFindNodes.push_back( node );
					mRequestFindCond.notifyOne();
				}
			}
			else
			{
				++depthDuplicateCount;
				int i = 1;

			}
		}

		FlushDepthLog();
		findWork->join();
	}

	void Solver::term()
	{
		Mutex::Locker locker( mRequestFindMutex );
		Mutex::Locker locker2( mUncheckMutex );
		mbRunning = false;

		mRequestFindCond.notifyAll();
		mUncheckCond.notifyAll();
	}

	void Solver::run_FindThread()
	{
		for(;;)
		{
			StateNode* node;
			{
				Mutex::Locker locker(mRequestFindMutex);
				mRequestFindCond.wait(locker, fastdelegate::FastDelegate< bool () >( this , &Solver::haveReauestFind ) );

				if ( mbRunning == false )
				{
					return;
				}
				node = mRequestFindNodes.front();
				mRequestFindNodes.pop_front();
			}
			assert( node );

			int const NewStateNum = 2 * CountFace;
			StateNode* newStates[ NewStateNum ];
			int numNewStates = generateNextNodes( node , newStates );
			{

				Mutex::Locker locker(mUncheckMutex);
				mUncheckNodes.insert(mUncheckNodes.end(), newStates, newStates + numNewStates);
				mUncheckCond.notifyOne();
			}
		}
	}
	
	void Solver::cleanup()
	{
		mRequestFindNodes.clear();
		mUncheckNodes.clear();
		for( int i = 0 ; i < mAllocNodes.size() ; ++i )
		{
			delete mAllocNodes[i];
		}
		mAllocNodes.clear();
		mCheckedStates.clear();
	}

	bool Solver::isInGoalDepthTable(CubeState const& canonicalState) const
	{
		return mGoalDepthStates.find(canonicalState) != mGoalDepthStates.end();
	}

	void Solver::buildGoalDepthTable(int depth)
	{
		TIME_SCOPE("BuildGoalDepthTable");

		CubeState finalCanonical;
		mFinalState.buildSymmetryCanonicalState(finalCanonical);
		if ( mGoalTableDepth == depth && finalCanonical.isEqual(mGoalTableFinalCanonical) )
			return;

		mGoalDepthStates.clear();
		mGoalTableDepth = depth;
		mGoalTableFinalCanonical = finalCanonical;

		struct GoalNode
		{
			CubeState state;
			FaceDir rotation;
			bool bInverse;
			bool bHaveMove;
		};

		TArray< GoalNode > frontier;
		TArray< GoalNode > nextFrontier;
		GoalNode rootNode;
		rootNode.state = mFinalState;
		rootNode.rotation = FaceFront;
		rootNode.bInverse = false;
		rootNode.bHaveMove = false;
		frontier.push_back(rootNode);
		mGoalDepthStates.insert(finalCanonical);

		for ( int curDepth = 0 ; curDepth < depth ; ++curDepth )
		{
			int64 depthStartTime = SystemPlatform::GetTickCount();
			int numInputStates = frontier.size();

			nextFrontier.clear();
			for ( GoalNode const& node : frontier )
			{
				for ( int face = 0 ; face < CountFace ; ++face )
				{
					FaceDir dir = FaceDir(face);
					CubeState nextState;
					CubeState canonicalState;

					if ( !ShouldPruneMove(node.rotation, node.bInverse, node.bHaveMove, dir, false) )
					{
						CubeOperator::Rotate(node.state, dir, nextState);
						nextState.buildSymmetryCanonicalState(canonicalState);
						if ( mGoalDepthStates.insert(canonicalState).second )
						{
							GoalNode nextNode;
							nextNode.state = nextState;
							nextNode.rotation = dir;
							nextNode.bInverse = false;
							nextNode.bHaveMove = true;
							nextFrontier.push_back(nextNode);
						}
					}

					if ( !ShouldPruneMove(node.rotation, node.bInverse, node.bHaveMove, dir, true) )
					{
						CubeOperator::RotateInv(node.state, dir, nextState);
						nextState.buildSymmetryCanonicalState(canonicalState);
						if ( mGoalDepthStates.insert(canonicalState).second )
						{
							GoalNode nextNode;
							nextNode.state = nextState;
							nextNode.rotation = dir;
							nextNode.bInverse = true;
							nextNode.bHaveMove = true;
							nextFrontier.push_back(nextNode);
						}
					}
				}
			}
			frontier.swap(nextFrontier);
			LogMsg("Rubiks Solver Goal Depth %d : time = %lld ms , input = %d , new = %d , total = %d",
			       curDepth + 1,
			       SystemPlatform::GetTickCount() - depthStartTime,
			       numInputStates,
			       int(frontier.size()),
			       int(mGoalDepthStates.size()));
			if ( frontier.empty() )
				break;
		}
	}

	int Solver::GetFacePairOrder(FaceDir dir)
	{
		return dir / 3;
	}

	bool Solver::ShouldPruneMove(StateNode const* node, FaceDir dir, bool bInverse)
	{
		return ShouldPruneMove(node->rotation, node->bInverse, node->parent != nullptr, dir, bInverse);
	}

	bool Solver::ShouldPruneMove(FaceDir prevDir, bool bPrevInverse, bool bHavePrevMove, FaceDir dir, bool bInverse)
	{
		if ( !bHavePrevMove )
			return false;

		if ( prevDir == dir && bPrevInverse != bInverse )
			return true;

		if ( IsOppositeFace(prevDir, dir) && GetFacePairOrder(prevDir) > GetFacePairOrder(dir) )
			return true;

		return false;
	}

	int Solver::generateNextNodes(StateNode* node , StateNode* nextNodes[])
	{
		int numNodes = 0;
		for( int i = 0 ; i < CountFace ; ++i)
		{
			FaceDir dir = FaceDir(i);
			if ( !ShouldPruneMove(node, dir, false) )
			{
				StateNode* n1 = new StateNode;
				mAllocNodes.push_back(n1);
				nextNodes[numNodes++] = n1;
				n1->rotation = dir;
				n1->bInverse = false;
				n1->parent = node;
				n1->depth = node->depth + 1;
				CubeOperator::Rotate( node->state , dir , n1->state );
				n1->state.buildSymmetryCanonicalState(n1->canonicalState);
			}

			if ( !ShouldPruneMove(node, dir, true) )
			{
				StateNode* n2 = new StateNode;
				mAllocNodes.push_back(n2);
				nextNodes[numNodes++] = n2;
				n2->rotation = dir;
				n2->bInverse = true;
				n2->parent = node;
				n2->depth = node->depth + 1;
				CubeOperator::RotateInv( node->state , dir , n2->state );
				n2->state.buildSymmetryCanonicalState(n2->canonicalState);
			}
		}
		return numNodes;
	}

	bool FastSolver::run()
	{
		TIME_SCOPE("FastSolve");
		mSolution.clear();

		return runTwoPhase();
	}

	bool FastSolver::runTwoPhase()
	{
		TIME_SCOPE("RubiksTwoPhase");

		if ( mInitState.isEqual(mFinalState) )
		{
			LogMsg("Rubiks FastSolver TwoPhase : already solved");
			return true;
		}

		GetPruningTables();
		GetCoordMoveTables();

		int const MaxPhase1Depth = 12;
		CoordState initState = MakeCoordState(mInitState);

		int phase1StartBound = CalcPhase1Heuristic(initState);
		for ( int phase1Bound = phase1StartBound ; phase1Bound <= MaxPhase1Depth ; ++phase1Bound )
		{
			IDASearchContext phase1Context;
			phase1Context.path.reserve(phase1Bound);
			int64 startTime = SystemPlatform::GetTickCount();
			if ( !searchPhase1(initState, 0, phase1Bound, FaceFront, false, phase1Context) )
			{
				LogMsg("Rubiks FastSolver Phase1 Depth %d : time = %lld ms , nodes = %d",
				       phase1Bound, SystemPlatform::GetTickCount() - startTime, phase1Context.nodeCount);
				continue;
			}

			LogMsg("Rubiks FastSolver Phase1 Depth %d : time = %lld ms , nodes = %d , solved = 1",
			       phase1Bound, SystemPlatform::GetTickCount() - startTime, phase1Context.nodeCount);
			return true;
		}

		LogMsg("Rubiks FastSolver TwoPhase : failed");
		return false;
	}

	bool FastSolver::searchPhase1(CoordState const& state, int depth, int bound, FaceDir prevDir, bool bHavePrevMove, IDASearchContext& context)
	{
		if ( IsPhase1Goal(state) )
		{
			CubeState phase1Cube = mInitState;
			CubeState nextCube;
			for ( SearchNode const& move : context.path )
			{
				ApplyMove(phase1Cube, move.rotation, move.power, nextCube);
				phase1Cube = nextCube;
			}
			CoordState phase2StartState = MakeCoordState(phase1Cube);

			int const MaxPhase2Depth = 18;
			int phase2StartBound = CalcPhase2Heuristic(phase2StartState);
			for ( int phase2Bound = phase2StartBound ; phase2Bound <= MaxPhase2Depth ; ++phase2Bound )
			{
				IDASearchContext phase2Context;
				phase2Context.path.reserve(phase2Bound);
				int64 phase2StartTime = SystemPlatform::GetTickCount();
				FaceDir phase2PrevDir = context.path.empty() ? FaceFront : context.path.back().rotation;
				bool bHavePhase2PrevMove = !context.path.empty();
				if ( searchPhase2(phase2StartState, 0, phase2Bound, phase2PrevDir, bHavePhase2PrevMove, phase2Context) )
				{
					LogMsg("Rubiks FastSolver Phase2 Depth %d : time = %lld ms , nodes = %d , solved = 1",
					       phase2Bound, SystemPlatform::GetTickCount() - phase2StartTime, phase2Context.nodeCount);
					mSolution = context.path;
					for ( SearchNode const& move : phase2Context.path )
					{
						mSolution.push_back(move);
					}
					LogMsg("Rubiks FastSolver TwoPhase : solved , phase1 = %d , phase2 = %d , total = %d",
					       int(context.path.size()), int(phase2Context.path.size()), int(mSolution.size()));
					return true;
				}
			}

			return false;
		}

		int h = CalcPhase1Heuristic(state);
		if ( depth + h > bound )
			return false;

		if ( depth >= bound )
			return false;

		++context.nodeCount;

		for ( int face = 0 ; face < CountFace ; ++face )
		{
			FaceDir dir = FaceDir(face);
			if ( ShouldPruneMove(prevDir, bHavePrevMove, dir) )
				continue;

			CoordMoveTables& moveTables = GetCoordMoveTables();
			for ( uint8 power = 1 ; power <= 3 ; ++power )
			{
				int moveIndex = GetMoveIndex(dir, power);
				CoordState nextState;
				nextState.co = moveTables.co[state.co][moveIndex];
				nextState.eo = moveTables.eo[state.eo][moveIndex];
				nextState.slice = moveTables.slice[state.slice][moveIndex];
				nextState.cp = 0;
				nextState.udEdgePerm = 0;
				nextState.slicePerm = 0;

				SearchNode move;
				move.rotation = dir;
				move.power = power;
				context.path.push_back(move);

				if ( searchPhase1(nextState, depth + 1, bound, dir, true, context) )
					return true;

				context.path.pop_back();
			}
		}

		return false;
	}

	bool FastSolver::searchPhase2(CoordState const& state, int depth, int bound, FaceDir prevDir, bool bHavePrevMove, IDASearchContext& context)
	{
		if ( state.cp == 0 && state.udEdgePerm == 0 && state.slicePerm == 0 )
			return true;

		int h = CalcPhase2Heuristic(state);
		if ( depth + h > bound )
			return false;

		if ( depth >= bound )
			return false;

		++context.nodeCount;

		CoordMoveTables& moveTables = GetCoordMoveTables();
		for ( int phase2MoveIndex = 0 ; phase2MoveIndex < Phase2MoveCount ; ++phase2MoveIndex )
		{
			int moveIndex = moveTables.phase2MoveToMove[phase2MoveIndex];
			FaceDir dir = GetMoveFace(moveIndex);
			if ( ShouldPruneMove(prevDir, bHavePrevMove, dir) )
				continue;

			uint8 power = GetMovePower(moveIndex);
			CoordState nextState = ApplyCoordPhase2Move(state, phase2MoveIndex);

			SearchNode move;
			move.rotation = dir;
			move.power = power;
			context.path.push_back(move);

			if ( searchPhase2(nextState, depth + 1, bound, dir, true, context) )
				return true;

			context.path.pop_back();
		}

		return false;
	}


	int FastSolver::GetFacePairOrder(FaceDir dir)
	{
		return dir / 3;
	}

	bool FastSolver::ShouldPruneMove(FaceDir prevDir, bool bHavePrevMove, FaceDir dir)
	{
		if ( !bHavePrevMove )
			return false;

		if ( prevDir == dir )
			return true;

		if ( IsOppositeFace(prevDir, dir) && GetFacePairOrder(prevDir) > GetFacePairOrder(dir) )
			return true;

		return false;
	}

	bool FastSolver::IsPhase2Move(FaceDir dir, uint8 power)
	{
		if ( dir == FaceUp || dir == FaceDown )
			return true;

		return power == 2;
	}

	bool FastSolver::IsPhase1Goal(CoordState const& state)
	{
		struct StaticLocal
		{
			StaticLocal()
			{
				CubeState solved;
				solved.setGoalState();
				SolvedSliceCoord = CalcSliceCoord(solved);
			}

			int SolvedSliceCoord;
		};
		static StaticLocal instance;
		return state.co == 0 &&
		       state.eo == 0 &&
		       state.slice == instance.SolvedSliceCoord;
	}

	bool FastSolver::ApplyMove(CubeState const& state, FaceDir dir, uint8 power, CubeState& outState)
	{
		CubeState tempState;
		CubeOperator::Rotate(state, dir, outState);
		for ( uint8 i = 1 ; i < power ; ++i )
		{
			CubeOperator::Rotate(outState, dir, tempState);
			outState = tempState;
		}
		return true;
	}

	FastSolver::CoordMoveTables& FastSolver::GetCoordMoveTables()
	{
		struct StaticLocal
		{
			StaticLocal()
			{
				int64 startTime = SystemPlatform::GetTickCount();
				int phase2Index = 0;
				for (int face = 0; face < CountFace; ++face)
				{
					FaceDir dir = FaceDir(face);
					for (uint8 power = 1; power <= 3; ++power)
					{
						if (IsPhase2Move(dir, power))
							tables.phase2MoveToMove[phase2Index++] = GetMoveIndex(dir, power);
					}
				}

				for (int coord = 0; coord < COCount; ++coord)
				{
					CubeState state;
					BuildCornerOriState(coord, state);
					for (int move = 0; move < MoveCount; ++move)
					{
						CubeState nextState;
						ApplyMove(state, GetMoveFace(move), GetMovePower(move), nextState);
						tables.co[coord][move] = uint16(CalcCornerOriCoord(nextState));
					}
				}

				for (int coord = 0; coord < EOCount; ++coord)
				{
					CubeState state;
					BuildEdgeOriState(coord, state);
					for (int move = 0; move < MoveCount; ++move)
					{
						CubeState nextState;
						ApplyMove(state, GetMoveFace(move), GetMovePower(move), nextState);
						tables.eo[coord][move] = uint16(CalcEdgeOriCoord(nextState));
					}
				}

				for (int coord = 0; coord < SliceCount; ++coord)
				{
					CubeState state;
					BuildSliceState(coord, state);
					for (int move = 0; move < MoveCount; ++move)
					{
						CubeState nextState;
						ApplyMove(state, GetMoveFace(move), GetMovePower(move), nextState);
						tables.slice[coord][move] = uint16(CalcSliceCoord(nextState));
					}
				}

				for (int coord = 0; coord < Perm8Count; ++coord)
				{
					CubeState state;
					BuildCornerPermState(coord, state);
					for (int move = 0; move < Phase2MoveCount; ++move)
					{
						CubeState nextState;
						int moveIndex = tables.phase2MoveToMove[move];
						ApplyMove(state, GetMoveFace(moveIndex), GetMovePower(moveIndex), nextState);
						tables.cp[coord][move] = uint16(CalcCornerPermCoord(nextState));
					}

					BuildUDEdgePermState(coord, state);
					for (int move = 0; move < Phase2MoveCount; ++move)
					{
						CubeState nextState;
						int moveIndex = tables.phase2MoveToMove[move];
						ApplyMove(state, GetMoveFace(moveIndex), GetMovePower(moveIndex), nextState);
						tables.udEdgePerm[coord][move] = uint16(CalcUDEdgePermCoord(nextState));
					}
				}

				for (int coord = 0; coord < Perm4Count; ++coord)
				{
					CubeState state;
					BuildSlicePermState(coord, state);
					for (int move = 0; move < Phase2MoveCount; ++move)
					{
						CubeState nextState;
						int moveIndex = tables.phase2MoveToMove[move];
						ApplyMove(state, GetMoveFace(moveIndex), GetMovePower(moveIndex), nextState);
						tables.slicePerm[coord][move] = uint16(CalcSlicePermCoord(nextState));
					}
				}
				LogMsg("Rubiks FastSolver CoordMoveTable : time = %lld ms", SystemPlatform::GetTickCount() - startTime);
			}
			CoordMoveTables tables;
		};

		static StaticLocal instance;
		return instance.tables;
	}

	FastSolver::CoordState FastSolver::MakeCoordState(CubeState const& state)
	{
		CoordState result;
		result.co = uint16(CalcCornerOriCoord(state));
		result.eo = uint16(CalcEdgeOriCoord(state));
		result.slice = uint16(CalcSliceCoord(state));

		if ( IsPhase1Goal(result) )
		{
			result.cp = uint16(CalcCornerPermCoord(state));
			result.udEdgePerm = uint16(CalcUDEdgePermCoord(state));
			result.slicePerm = uint16(CalcSlicePermCoord(state));
		}
		else
		{
			result.cp = 0;
			result.udEdgePerm = 0;
			result.slicePerm = 0;
		}
		return result;
	}

	FastSolver::CoordState FastSolver::ApplyCoordPhase2Move(CoordState const& state, int phase2MoveIndex)
	{
		CoordMoveTables& tables = GetCoordMoveTables();
		int moveIndex = tables.phase2MoveToMove[phase2MoveIndex];
		CoordState result;
		result.co = tables.co[state.co][moveIndex];
		result.eo = tables.eo[state.eo][moveIndex];
		result.slice = tables.slice[state.slice][moveIndex];
		result.cp = tables.cp[state.cp][phase2MoveIndex];
		result.udEdgePerm = tables.udEdgePerm[state.udEdgePerm][phase2MoveIndex];
		result.slicePerm = tables.slicePerm[state.slicePerm][phase2MoveIndex];
		return result;
	}

	FaceDir FastSolver::GetMoveFace(int moveIndex)
	{
		return FaceDir(moveIndex / 3);
	}

	uint8 FastSolver::GetMovePower(int moveIndex)
	{
		return uint8(moveIndex % 3 + 1);
	}

	int FastSolver::GetMoveIndex(FaceDir dir, uint8 power)
	{
		return int(dir) * 3 + int(power - 1);
	}

	FastSolver::PruningTables& FastSolver::GetPruningTables()
	{
		struct StaticLocal
		{
			StaticLocal()
			{
				int64 startTime = SystemPlatform::GetTickCount();
				CoordMoveTables& moveTables = GetCoordMoveTables();
				CubeState solved;
				solved.setGoalState();
				BuildCoordPruningTable(tables.cornerOri, COCount, 0, &moveTables.co[0][0], MoveCount, MoveCount);
				BuildCoordPruningTable(tables.edgeOri, EOCount, 0, &moveTables.eo[0][0], MoveCount, MoveCount);
				BuildCoordPruningTable(tables.slice, SliceCount, uint16(CalcSliceCoord(solved)), &moveTables.slice[0][0], MoveCount, MoveCount);
				BuildCoordPruningTable(tables.cornerPerm, Perm8Count, 0, &moveTables.cp[0][0], Phase2MoveCount, Phase2MoveCount);
				BuildCoordPruningTable(tables.udEdgePerm, Perm8Count, 0, &moveTables.udEdgePerm[0][0], Phase2MoveCount, Phase2MoveCount);
				BuildCoordPruningTable(tables.slicePerm, Perm4Count, 0, &moveTables.slicePerm[0][0], Phase2MoveCount, Phase2MoveCount);
				LogMsg("Rubiks FastSolver PruningTable : time = %lld ms", SystemPlatform::GetTickCount() - startTime);
			}
			PruningTables tables;
		};

		static StaticLocal instance;
		return instance.tables;
	}

	void FastSolver::BuildCoordPruningTable(int8* table, int tableSize, uint16 startCoord, uint16 const* moveTable, int moveStride, int numMoves)
	{
		for ( int i = 0 ; i < tableSize ; ++i )
			table[i] = -1;

		TArray< uint16 > frontier;
		TArray< uint16 > nextFrontier;
		table[startCoord] = 0;
		frontier.push_back(startCoord);

		int depth = 0;
		int numVisited = 1;
		while ( numVisited < tableSize && !frontier.empty() )
		{
			nextFrontier.clear();
			for ( uint16 coord : frontier )
			{
				uint16 const* row = moveTable + coord * moveStride;
				for ( int move = 0 ; move < numMoves ; ++move )
				{
					uint16 nextCoord = row[move];
					if ( table[nextCoord] == -1 )
					{
						table[nextCoord] = int8(depth + 1);
						nextFrontier.push_back(nextCoord);
						++numVisited;
					}
				}
			}
			frontier.swap(nextFrontier);
			++depth;
		}
	}

	int FastSolver::CalcCornerOriCoord(CubeState const& state)
	{
		int coord = 0;
		int factor = 1;
		for ( uint8 i = 0 ; i < 7 ; ++i )
		{
			uint8 piece;
			uint8 ori;
			state.getCorner(i, piece, ori);
			coord += ori * factor;
			factor *= 3;
		}
		return coord;
	}

	int FastSolver::CalcEdgeOriCoord(CubeState const& state)
	{
		int coord = 0;
		for ( uint8 i = 0 ; i < 11 ; ++i )
		{
			uint8 piece;
			uint8 ori;
			state.getEdge(i, piece, ori);
			coord |= int(ori) << i;
		}
		return coord;
	}

	int FastSolver::CalcSliceCoord(CubeState const& state)
	{
		uint32 mask = 0;
		for ( uint8 i = 0 ; i < 12 ; ++i )
		{
			uint8 piece;
			uint8 ori;
			state.getEdge(i, piece, ori);
			if ( piece >= 8 )
				mask |= 1 << i;
		}
		return CalcCombinationRank(mask, 12, 4);
	}

	int FastSolver::CalcCornerPermCoord(CubeState const& state)
	{
		uint8 values[8];
		for ( uint8 i = 0 ; i < 8 ; ++i )
		{
			uint8 piece;
			uint8 ori;
			state.getCorner(i, piece, ori);
			values[i] = piece;
		}
		return CalcPermutationRank(values, 8);
	}

	int FastSolver::CalcUDEdgePermCoord(CubeState const& state)
	{
		uint8 values[8];
		for ( uint8 i = 0 ; i < 8 ; ++i )
		{
			uint8 piece;
			uint8 ori;
			state.getEdge(i, piece, ori);
			values[i] = piece;
		}
		return CalcPermutationRank(values, 8);
	}

	int FastSolver::CalcSlicePermCoord(CubeState const& state)
	{
		uint8 values[4];
		for ( uint8 i = 0 ; i < 4 ; ++i )
		{
			uint8 piece;
			uint8 ori;
			state.getEdge(i + 8, piece, ori);
			values[i] = piece - 8;
		}
		return CalcPermutationRank(values, 4);
	}

	int FastSolver::CalcCombinationRank(uint32 mask, int numBits, int chooseBits)
	{
		struct StaticLocal
		{
			StaticLocal()
			{
				for (int i = 0; i < (1 << 12); ++i)
					rankTable[i] = INDEX_NONE;

				int rank = 0;
				for (int value = 0; value < (1 << 12); ++value)
				{
					int count = 0;
					for (int bit = 0; bit < 12; ++bit)
					{
						if (value & (1 << bit))
							++count;
					}
					if (count == 4)
						rankTable[value] = rank++;
				}
			}
			int rankTable[1 << 12];
		};

		static StaticLocal instance;
		return instance.rankTable[mask & ((1 << numBits) - 1)];
	}

	int FastSolver::CalcPermutationRank(uint8 const* values, int numValues)
	{
		int rank = 0;
		for ( int i = 0 ; i < numValues ; ++i )
		{
			int lessCount = 0;
			for ( int j = i + 1 ; j < numValues ; ++j )
			{
				if ( values[j] < values[i] )
					++lessCount;
			}
			rank = rank * (numValues - i) + lessCount;
		}
		return rank;
	}

	void FastSolver::BuildPermutationFromRank(int rank, int numValues, uint8* outValues)
	{
		static int const Factorial[13] =
		{
			1, 1, 2, 6, 24, 120, 720, 5040, 40320, 362880, 3628800, 39916800, 479001600
		};
		uint8 items[12];
		for ( int i = 0 ; i < numValues ; ++i )
			items[i] = uint8(i);

		for ( int i = 0 ; i < numValues ; ++i )
		{
			int factor = Factorial[numValues - 1 - i];
			int index = ( factor > 0 ) ? ( rank / factor ) : 0;
			rank %= factor;
			outValues[i] = items[index];
			for ( int j = index ; j + 1 < numValues - i ; ++j )
				items[j] = items[j + 1];
		}
	}

	uint32 FastSolver::BuildCombinationMaskFromRank(int rank, int numBits, int chooseBits)
	{
		int curRank = 0;
		for ( uint32 mask = 0 ; mask < (uint32(1) << numBits) ; ++mask )
		{
			int count = 0;
			for ( int bit = 0 ; bit < numBits ; ++bit )
			{
				if ( mask & (uint32(1) << bit) )
					++count;
			}
			if ( count == chooseBits )
			{
				if ( curRank == rank )
					return mask;
				++curRank;
			}
		}
		return 0;
	}

	void FastSolver::BuildCornerOriState(int coord, CubeState& outState)
	{
		outState.setGoalState();
		int sumOri = 0;
		for ( uint8 i = 0 ; i < 7 ; ++i )
		{
			uint8 piece;
			uint8 oldOri;
			outState.getCorner(i, piece, oldOri);
			uint8 ori = coord % 3;
			coord /= 3;
			outState.setCorner(i, piece, ori);
			sumOri += ori;
		}
		uint8 piece;
		uint8 oldOri;
		outState.getCorner(7, piece, oldOri);
		outState.setCorner(7, piece, uint8((3 - sumOri % 3) % 3));
		outState.updateHash();
	}

	void FastSolver::BuildEdgeOriState(int coord, CubeState& outState)
	{
		outState.setGoalState();
		int sumOri = 0;
		for ( uint8 i = 0 ; i < 11 ; ++i )
		{
			uint8 piece;
			uint8 oldOri;
			outState.getEdge(i, piece, oldOri);
			uint8 ori = (coord >> i) & 1;
			outState.setEdge(i, piece, ori);
			sumOri += ori;
		}
		uint8 piece;
		uint8 oldOri;
		outState.getEdge(11, piece, oldOri);
		outState.setEdge(11, piece, uint8(sumOri & 1));
		outState.updateHash();
	}

	void FastSolver::BuildSliceState(int coord, CubeState& outState)
	{
		outState.setGoalState();
		uint32 mask = BuildCombinationMaskFromRank(coord, 12, 4);
		uint8 nextUDEdge = 0;
		uint8 nextSliceEdge = 8;
		for ( uint8 i = 0 ; i < 12 ; ++i )
		{
			if ( mask & (uint32(1) << i) )
				outState.setEdge(i, nextSliceEdge++, 0);
			else
				outState.setEdge(i, nextUDEdge++, 0);
		}
		outState.updateHash();
	}

	void FastSolver::BuildCornerPermState(int coord, CubeState& outState)
	{
		outState.setGoalState();
		uint8 values[8];
		BuildPermutationFromRank(coord, 8, values);
		for ( uint8 i = 0 ; i < 8 ; ++i )
			outState.setCorner(i, values[i], 0);
		outState.updateHash();
	}

	void FastSolver::BuildUDEdgePermState(int coord, CubeState& outState)
	{
		outState.setGoalState();
		uint8 values[8];
		BuildPermutationFromRank(coord, 8, values);
		for ( uint8 i = 0 ; i < 8 ; ++i )
			outState.setEdge(i, values[i], 0);
		for ( uint8 i = 8 ; i < 12 ; ++i )
			outState.setEdge(i, i, 0);
		outState.updateHash();
	}

	void FastSolver::BuildSlicePermState(int coord, CubeState& outState)
	{
		outState.setGoalState();
		uint8 values[4];
		BuildPermutationFromRank(coord, 4, values);
		for ( uint8 i = 0 ; i < 4 ; ++i )
			outState.setEdge(i + 8, values[i] + 8, 0);
		outState.updateHash();
	}

	int FastSolver::CalcPhase1Heuristic(CoordState const& state)
	{
		PruningTables& tables = GetPruningTables();
		int hCO = tables.cornerOri[state.co];
		int hEO = tables.edgeOri[state.eo];
		int hSlice = tables.slice[state.slice];
		int h = hCO;
		if ( h < hEO )
			h = hEO;
		if ( h < hSlice )
			h = hSlice;
		return h;
	}

	int FastSolver::CalcPhase2Heuristic(CoordState const& state)
	{
		PruningTables& tables = GetPruningTables();
		int hCP = tables.cornerPerm[state.cp];
		int hUDEP = tables.udEdgePerm[state.udEdgePerm];
		int hSP = tables.slicePerm[state.slicePerm];
		int h = hCP;
		if ( h < hUDEP )
			h = hUDEP;
		if ( h < hSP )
			h = hSP;
		return h;
	}

}//namespace Rubiks


REGISTER_STAGE_ENTRY("Rubiks Test", Rubiks::TestStage, EExecGroup::Dev4);
