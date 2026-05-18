#ifndef RubiksStage_h__c781c03d_43f7_4882_b367_96b003d27fbb
#define RubiksStage_h__c781c03d_43f7_4882_b367_96b003d27fbb

#include "Core/IntegerType.h"

#include "StageBase.h"
#include "GameGlobal.h"
#include "GameRenderSetup.h"
#include "RenderUtility.h"
#include "GameGUISystem.h"
#include "GameWidgetID.h"

#include "Widget/WidgetUtility.h"
#include "Math/Base.h"
#include "Math/TVector3.h"
#include "Math/Vector3.h"
#include "RHI/RHIGlobalResource.h"
#include "RHI/SimpleRenderState.h"
#include "Renderer/MeshBuild.h"

#include "CppVersion.h"
#include "FastDelegate/FastDelegate.h"
#include <unordered_set>
#include <deque>
#include <vector>

#include "PlatformThread.h"

namespace Rubiks
{
	using namespace Render;

	//     U1
	//  L4 F0 R2 B5
	//     D3
	//  0 1 2
	//  7 8 3
	//  6 5 4  
	enum FaceDir
	{
		FaceFront = 0,
		FaceUp    = 1,
		FaceRight = 2,
		FaceDown  = 3,
		FaceLeft  = 4,
		FaceBack  = 5,

		CountFace = 6,
	};

	int const CubeBlockSize = 3;
	int const CubeFaceEdgeNum = 4;

	struct EdgeLinkParam
	{
		FaceDir face;
		int     dataOffset;
	};

	struct FaceLinkParam
	{
		FaceDir invFace;
		EdgeLinkParam edges[4];
	};

	FaceLinkParam const gFaceLinkParam[CountFace] = 
	{
		/*F*/{ FaceBack , FaceUp    , 6 , FaceRight , 0  ,  FaceDown , 2  , FaceLeft , 4  } ,
		/*U*/{ FaceDown , FaceBack  , 2 , FaceRight , 2  , FaceFront , 2  , FaceLeft , 2 } ,
		/*R*/{ FaceLeft , FaceUp    , 4 , FaceBack , 0  , FaceDown , 4  , FaceFront , 4 } ,
		/*D*/{ FaceUp   , FaceFront , 6 , FaceRight , 6  , FaceBack , 6 , FaceLeft , 6 } ,
		/*L*/{ FaceRight, FaceUp    , 0 , FaceFront , 0  , FaceDown , 0  , FaceBack , 4 } ,
		/*B*/{ FaceFront, FaceUp    , 2 , FaceLeft , 0  , FaceDown , 6  , FaceRight , 4 } ,
	};

	struct CubeState
	{
		struct FaceletDesc
		{
			uint8 pos;
			uint8 slot;
			uint8 bCorner;
		};
		struct Coord
		{
			int8 x;
			int8 y;
			int8 z;
		};
		enum CubieFace : uint8
		{
			CF_U,
			CF_R,
			CF_F,
			CF_D,
			CF_L,
			CF_B,
			CF_None = 0xff,
		};
		struct MoveCache
		{
			bool  bInitialized = false;
			uint8 cornerDest[2][CountFace][8];
			uint8 edgeDest[2][CountFace][12];
			uint8 cornerOri[2][CountFace][8][3];
			uint8 edgeOri[2][CountFace][12][2];
		};

		uint64 cornerKey;
		uint64 edgeKey;
		std::size_t hashValue;

		void updateHash()
		{
			hashValue = std::hash<uint64>{}(cornerKey ^ (edgeKey + 0x9e3779b97f4a7c15ull + (cornerKey << 6) + (cornerKey >> 2)));
		}

		bool isEqual( CubeState const& other ) const
		{
			return cornerKey == other.cornerKey && edgeKey == other.edgeKey;
		}

		void setEmptyState()
		{
			cornerKey = 0;
			edgeKey = 0;
			updateHash();
		}
		void setGlobalState()
		{
			cornerKey = 0;
			edgeKey = 0;
			for ( uint8 i = 0 ; i < 8 ; ++i )
			{
				SetCorner(i, i, 0);
			}
			for ( uint8 i = 0 ; i < 12 ; ++i )
			{
				SetEdge(i, i, 0);
			}
			updateHash();
		}

		uint32 getBlockValue( FaceDir dir , uint32 index ) const
		{
			if ( index == 8 )
				return uint32(dir) + 1;

			int row, col;
			GetRowColFromIndex(index, row, col);
			FaceletDesc desc = GetFaceletDesc(dir, row, col);
			if ( desc.bCorner )
			{
				uint8 piece;
				uint8 ori;
				GetCorner(desc.pos, piece, ori);
				CubieFace colorFace = GetCornerPieceFaces()[piece][Mod(desc.slot - ori, 3)];
				return GetColorValue(colorFace);
			}

			uint8 piece;
			uint8 ori;
			GetEdge(desc.pos, piece, ori);
			CubieFace colorFace = GetEdgePieceFaces()[piece][Mod(desc.slot - ori, 2)];
			return GetColorValue(colorFace);
		}

		static void ApplyMove(CubeState const& oldState, FaceDir dir, bool bInverse, CubeState& newState)
		{
			MoveCache& cache = GetMoveCache();
			int invIndex = bInverse ? 1 : 0;

			for ( int i = 0 ; i < 8 ; ++i )
			{
				uint8 dest = cache.cornerDest[invIndex][dir][i];
				uint8 piece;
				uint8 ori;
				oldState.GetCorner(i, piece, ori);
				newState.SetCorner(dest, piece, cache.cornerOri[invIndex][dir][i][ori]);
			}
			for ( int i = 0 ; i < 12 ; ++i )
			{
				uint8 dest = cache.edgeDest[invIndex][dir][i];
				uint8 piece;
				uint8 ori;
				oldState.GetEdge(i, piece, ori);
				newState.SetEdge(dest, piece, cache.edgeOri[invIndex][dir][i][ori]);
			}
			newState.updateHash();
		}

		static uint64 GetFieldMask(int bits, int index)
		{
			return ((uint64(1) << bits) - 1) << (bits * index);
		}

		void GetCorner(uint8 index, uint8& outPiece, uint8& outOri) const
		{
			uint64 value = (cornerKey >> (5 * index)) & 0x1f;
			outPiece = value & 0x7;
			outOri = (value >> 3) & 0x3;
		}

		void SetCorner(uint8 index, uint8 piece, uint8 ori)
		{
			uint64 value = (piece & 0x7) | ((uint64(ori & 0x3)) << 3);
			cornerKey = (cornerKey & ~GetFieldMask(5, index)) | (value << (5 * index));
		}

		void GetEdge(uint8 index, uint8& outPiece, uint8& outOri) const
		{
			uint64 value = (edgeKey >> (5 * index)) & 0x1f;
			outPiece = value & 0xf;
			outOri = (value >> 4) & 0x1;
		}

		void SetEdge(uint8 index, uint8 piece, uint8 ori)
		{
			uint64 value = (piece & 0xf) | ((uint64(ori & 0x1)) << 4);
			edgeKey = (edgeKey & ~GetFieldMask(5, index)) | (value << (5 * index));
		}

		static MoveCache& GetMoveCache()
		{
			static MoveCache cache;
			if ( !cache.bInitialized )
			{
				BuildMoveCache(cache);
				cache.bInitialized = true;
			}
			return cache;
		}

		static int Mod(int value, int mod)
		{
			int result = value % mod;
			return ( result < 0 ) ? ( result + mod ) : result;
		}

		static CubieFace ToCubieFace(FaceDir dir)
		{
			switch ( dir )
			{
			case FaceFront: return CF_F;
			case FaceUp:    return CF_U;
			case FaceRight: return CF_R;
			case FaceDown:  return CF_D;
			case FaceLeft:  return CF_L;
			case FaceBack:  return CF_B;
			default:        return CF_None;
			}
		}

		static uint32 GetColorValue(CubieFace face)
		{
			switch ( face )
			{
			case CF_F: return FaceFront + 1;
			case CF_U: return FaceUp + 1;
			case CF_R: return FaceRight + 1;
			case CF_D: return FaceDown + 1;
			case CF_L: return FaceLeft + 1;
			case CF_B: return FaceBack + 1;
			default:   return 0;
			}
		}

		static Coord GetFaceCoord(CubieFace face)
		{
			switch ( face )
			{
			case CF_U: return { 0, 1, 0 };
			case CF_R: return { 1, 0, 0 };
			case CF_F: return { 0, 0, 1 };
			case CF_D: return { 0,-1, 0 };
			case CF_L: return {-1, 0, 0 };
			case CF_B: return { 0, 0,-1 };
			default:   return { 0, 0, 0 };
			}
		}

		static CubieFace GetFaceFromCoord(Coord coord)
		{
			if ( coord.x == 1 ) return CF_R;
			if ( coord.x == -1 ) return CF_L;
			if ( coord.y == 1 ) return CF_U;
			if ( coord.y == -1 ) return CF_D;
			if ( coord.z == 1 ) return CF_F;
			if ( coord.z == -1 ) return CF_B;
			return CF_None;
		}

		static Coord RotateAroundPositiveAxis(int axis, int quarterTurn, Coord coord)
		{
			switch ( axis )
			{
			case 0:
				return ( quarterTurn > 0 ) ? Coord{ coord.x, int8(-coord.z), int8(coord.y) } : Coord{ coord.x, int8(coord.z), int8(-coord.y) };
			case 1:
				return ( quarterTurn > 0 ) ? Coord{ int8(coord.z), coord.y, int8(-coord.x) } : Coord{ int8(-coord.z), coord.y, int8(coord.x) };
			default:
				return ( quarterTurn > 0 ) ? Coord{ int8(-coord.y), int8(coord.x), coord.z } : Coord{ int8(coord.y), int8(-coord.x), coord.z };
			}
		}

		static Coord RotateCoord(FaceDir dir, bool bInverse, Coord coord)
		{
			int axis = 2;
			int axisSign = 1;
			switch ( dir )
			{
			case FaceRight: axis = 0; axisSign = 1; break;
			case FaceLeft:  axis = 0; axisSign = -1; break;
			case FaceUp:    axis = 1; axisSign = -1; break;
			case FaceDown:  axis = 1; axisSign = 1; break;
			case FaceFront: axis = 2; axisSign = -1; break;
			case FaceBack:  axis = 2; axisSign = 1; break;
			}

			int quarterTurn = ( bInverse ? 1 : -1 ) * axisSign;
			return RotateAroundPositiveAxis(axis, quarterTurn, coord);
		}

		static CubieFace RotateFace(FaceDir dir, bool bInverse, CubieFace face)
		{
			return GetFaceFromCoord(RotateCoord(dir, bInverse, GetFaceCoord(face)));
		}

		static bool IsOnLayer(FaceDir dir, Coord coord)
		{
			switch ( dir )
			{
			case FaceFront: return coord.z == 1;
			case FaceBack:  return coord.z == -1;
			case FaceRight: return coord.x == 1;
			case FaceLeft:  return coord.x == -1;
			case FaceUp:    return coord.y == 1;
			case FaceDown:  return coord.y == -1;
			default:        return false;
			}
		}

		static int FindCornerPos(Coord coord)
		{
			auto const& coords = GetCornerCoords();
			for ( int i = 0 ; i < 8 ; ++i )
			{
				if ( coords[i].x == coord.x && coords[i].y == coord.y && coords[i].z == coord.z )
					return i;
			}
			return INDEX_NONE;
		}

		static int FindEdgePos(Coord coord)
		{
			auto const& coords = GetEdgeCoords();
			for ( int i = 0 ; i < 12 ; ++i )
			{
				if ( coords[i].x == coord.x && coords[i].y == coord.y && coords[i].z == coord.z )
					return i;
			}
			return INDEX_NONE;
		}

		static void GetRowColFromIndex(uint32 index, int& outRow, int& outCol)
		{
			static uint8 const IndexToRowCol[9][2] =
			{
				{ 0, 0 }, { 0, 1 }, { 0, 2 },
				{ 1, 2 }, { 2, 2 }, { 2, 1 },
				{ 2, 0 }, { 1, 0 }, { 1, 1 },
			};
			outRow = IndexToRowCol[index][0];
			outCol = IndexToRowCol[index][1];
		}

		static FaceletDesc GetFaceletDesc(FaceDir dir, int row, int col)
		{
			switch ( dir )
			{
			case FaceUp:
				if ( row == 0 && col == 0 ) return { 2, 0, true };
				if ( row == 0 && col == 1 ) return { 3, 0, false };
				if ( row == 0 && col == 2 ) return { 3, 0, true };
				if ( row == 1 && col == 0 ) return { 2, 0, false };
				if ( row == 1 && col == 2 ) return { 0, 0, false };
				if ( row == 2 && col == 0 ) return { 1, 0, true };
				if ( row == 2 && col == 1 ) return { 0, 0, false };
				return { 0, 0, true };
			case FaceFront:
				if ( row == 0 && col == 0 ) return { 1, 1, true };
				if ( row == 0 && col == 1 ) return { 1, 1, false };
				if ( row == 0 && col == 2 ) return { 0, 2, true };
				if ( row == 1 && col == 0 ) return { 9, 0, false };
				if ( row == 1 && col == 2 ) return { 8, 0, false };
				if ( row == 2 && col == 0 ) return { 5, 2, true };
				if ( row == 2 && col == 1 ) return { 5, 1, false };
				return { 4, 1, true };
			case FaceRight:
				if ( row == 0 && col == 0 ) return { 0, 1, true };
				if ( row == 0 && col == 1 ) return { 0, 1, false };
				if ( row == 0 && col == 2 ) return { 3, 2, true };
				if ( row == 1 && col == 0 ) return { 8, 1, false };
				if ( row == 1 && col == 2 ) return { 11, 1, false };
				if ( row == 2 && col == 0 ) return { 4, 2, true };
				if ( row == 2 && col == 1 ) return { 4, 1, false };
				return { 7, 1, true };
			case FaceDown:
				if ( row == 0 && col == 0 ) return { 5, 0, true };
				if ( row == 0 && col == 1 ) return { 5, 0, false };
				if ( row == 0 && col == 2 ) return { 4, 0, true };
				if ( row == 1 && col == 0 ) return { 6, 0, false };
				if ( row == 1 && col == 2 ) return { 4, 0, false };
				if ( row == 2 && col == 0 ) return { 6, 0, true };
				if ( row == 2 && col == 1 ) return { 7, 0, false };
				return { 7, 0, true };
			case FaceLeft:
				if ( row == 0 && col == 0 ) return { 2, 1, true };
				if ( row == 0 && col == 1 ) return { 2, 1, false };
				if ( row == 0 && col == 2 ) return { 1, 2, true };
				if ( row == 1 && col == 0 ) return { 10, 1, false };
				if ( row == 1 && col == 2 ) return { 9, 1, false };
				if ( row == 2 && col == 0 ) return { 6, 2, true };
				if ( row == 2 && col == 1 ) return { 6, 1, false };
				return { 5, 1, true };
			case FaceBack:
				if ( row == 0 && col == 0 ) return { 3, 1, true };
				if ( row == 0 && col == 1 ) return { 3, 1, false };
				if ( row == 0 && col == 2 ) return { 2, 2, true };
				if ( row == 1 && col == 0 ) return { 11, 0, false };
				if ( row == 1 && col == 2 ) return { 10, 0, false };
				if ( row == 2 && col == 0 ) return { 7, 2, true };
				if ( row == 2 && col == 1 ) return { 7, 1, false };
				return { 6, 1, true };
			default:
				return { 0, 0, false };
			}
		}

		static Coord const (&GetCornerCoords())[8]
		{
			static Coord const Data[8] =
			{
				{ 1, 1, 1 }, { -1, 1, 1 }, { -1, 1,-1 }, { 1, 1,-1 },
				{ 1,-1, 1 }, { -1,-1, 1 }, { -1,-1,-1 }, { 1,-1,-1 },
			};
			return Data;
		}

		static Coord const (&GetEdgeCoords())[12]
		{
			static Coord const Data[12] =
			{
				{ 1, 1, 0 }, { 0, 1, 1 }, { -1, 1, 0 }, { 0, 1,-1 },
				{ 1,-1, 0 }, { 0,-1, 1 }, { -1,-1, 0 }, { 0,-1,-1 },
				{ 1, 0, 1 }, { -1, 0, 1 }, { -1, 0,-1 }, { 1, 0,-1 },
			};
			return Data;
		}

		static CubieFace const (&GetCornerPieceFaces())[8][3]
		{
			static CubieFace const Data[8][3] =
			{
				{ CF_U, CF_R, CF_F }, { CF_U, CF_F, CF_L }, { CF_U, CF_L, CF_B }, { CF_U, CF_B, CF_R },
				{ CF_D, CF_F, CF_R }, { CF_D, CF_L, CF_F }, { CF_D, CF_B, CF_L }, { CF_D, CF_R, CF_B },
			};
			return Data;
		}

		static CubieFace const (&GetEdgePieceFaces())[12][2]
		{
			static CubieFace const Data[12][2] =
			{
				{ CF_U, CF_R }, { CF_U, CF_F }, { CF_U, CF_L }, { CF_U, CF_B },
				{ CF_D, CF_R }, { CF_D, CF_F }, { CF_D, CF_L }, { CF_D, CF_B },
				{ CF_F, CF_R }, { CF_F, CF_L }, { CF_B, CF_L }, { CF_B, CF_R },
			};
			return Data;
		}

		static void BuildMoveCache(MoveCache& cache)
		{
			auto const& cornerCoords = GetCornerCoords();
			auto const& edgeCoords = GetEdgeCoords();

			for ( int inv = 0 ; inv < 2 ; ++inv )
			{
				for ( int move = 0 ; move < CountFace ; ++move )
				{
					for ( int i = 0 ; i < 8 ; ++i )
					{
						Coord srcCoord = cornerCoords[i];
						int dest = i;
						if ( IsOnLayer(FaceDir(move), srcCoord) )
							dest = FindCornerPos(RotateCoord(FaceDir(move), inv != 0, srcCoord));
						cache.cornerDest[inv][move][i] = dest;

						for ( int ori = 0 ; ori < 3 ; ++ori )
						{
							CubieFace sourceOutwardFace = GetCornerPosFaces()[i][ori];
							if ( IsOnLayer(FaceDir(move), srcCoord) )
								sourceOutwardFace = RotateFace(FaceDir(move), inv != 0, sourceOutwardFace);

							uint8 newOri = 0;
							for ( uint8 slot = 0 ; slot < 3 ; ++slot )
							{
								if ( GetCornerPosFaces()[dest][slot] == sourceOutwardFace )
								{
									newOri = slot;
									break;
								}
							}
							cache.cornerOri[inv][move][i][ori] = newOri;
						}
					}

					for ( int i = 0 ; i < 12 ; ++i )
					{
						Coord srcCoord = edgeCoords[i];
						int dest = i;
						if ( IsOnLayer(FaceDir(move), srcCoord) )
							dest = FindEdgePos(RotateCoord(FaceDir(move), inv != 0, srcCoord));
						cache.edgeDest[inv][move][i] = dest;

						for ( int ori = 0 ; ori < 2 ; ++ori )
						{
							CubieFace sourceOutwardFace = GetEdgePosFaces()[i][ori];
							if ( IsOnLayer(FaceDir(move), srcCoord) )
								sourceOutwardFace = RotateFace(FaceDir(move), inv != 0, sourceOutwardFace);

							uint8 newOri = 0;
							for ( uint8 slot = 0 ; slot < 2 ; ++slot )
							{
								if ( GetEdgePosFaces()[dest][slot] == sourceOutwardFace )
								{
									newOri = slot;
									break;
								}
							}
							cache.edgeOri[inv][move][i][ori] = newOri;
						}
					}
				}
			}
		}

		static CubieFace const (&GetCornerPosFaces())[8][3]
		{
			return GetCornerPieceFaces();
		}

		static CubieFace const (&GetEdgePosFaces())[12][2]
		{
			return GetEdgePieceFaces();
		}
	};

	class CubeOperator
	{
	public:
		static void Rotate( CubeState const& oldState , FaceDir dir , CubeState& newState )
		{
			CubeState::ApplyMove(oldState, dir, false, newState);
		}
		static void RotateInv( CubeState const& oldState , FaceDir dir , CubeState& newState )
		{
			CubeState::ApplyMove(oldState, dir, true, newState);
		}
	};


	class IDA
	{
	public:
		void proc( int depth )
		{

			if( depth > maxDepth )
				return;
		}
		int maxDepth;
	};

	class Solver
	{
	public:

		void run();

		void term();
		bool haveReauestFind()
		{
			return !mRequestFindNodes.empty() && mbRunning;
		}
		bool haveUncheck()
		{
			return !mUncheckNodes.empty() && mbRunning;
		}
		void run_FindThread();



		void solveSuccess()
		{
			term();
		}
		void solveFail()
		{
			term();
		}

		void cleanup();

		struct StateNode
		{
			StateNode* parent;
			CubeState  state;
			FaceDir    rotation;
			bool       bInverse;
		};
		void generateNextNodes(StateNode* node , StateNode* nextNodes[]);


		bool mbRunning;
		ConditionVariable mRequestFindCond;
		ConditionVariable mUncheckCond;
		Mutex mRequestFindMutex;
		Mutex mUncheckMutex;
		std::deque< StateNode* > mRequestFindNodes;
		std::deque< StateNode* > mUncheckNodes;
		std::vector< StateNode* > mAllocNodes;
		struct StateEqual
		{
			bool operator ()( CubeState const* a , CubeState const* b ) const
			{
				return a->isEqual( *b );
			}
		};
		struct StateHash
		{
			std::size_t operator()( CubeState const* a ) const
			{
				return a->hashValue;
			}
		};

		typedef std::unordered_set< CubeState* , StateHash , StateEqual > StateSet;
		StateSet mCheckedStates;

		CubeState mInitState;
		CubeState mFinalState;
	};

	static int BlockColor[] = 
	{
		EColor::Black , EColor::Red , EColor::White , EColor::Blue , EColor::Yellow , EColor::Green , EColor::Orange ,
	};
	int const FaceGridIndexMap[CubeBlockSize][CubeBlockSize] =
	{
		{ 0 , 1 , 2 },
		{ 7 , 8 , 3 },
		{ 6 , 5 , 4 },
	};
	int const BlockLen = 20;
	Vec2i const BlockSize = Vec2i( BlockLen  , BlockLen  );
	Vec2i const BlockOffset[] = 
	{
		Vec2i(0,0) , Vec2i(1,0) , Vec2i(2,0) ,
		Vec2i(2,1) , Vec2i(2,2) , Vec2i(1,2) ,
		Vec2i(0,2) , Vec2i(0,1) , Vec2i(1,1) , 
	};
	Vec2i const FaceOffset[] =
	{
		Vec2i(1,1) , Vec2i(1,0) , Vec2i(2,1) , Vec2i(1,2) , Vec2i(0,1) , Vec2i(3,1) ,
	};

	class TestStage : public StageBase
		            , public IGameRenderSetup
	{
		typedef StageBase BaseClass;
	public:
		struct RotationAnim
		{
			bool    bPlaying;
			FaceDir dir;
			bool    bInverse;
			float   time;
			float   duration;
		};

		TestStage(){}
		virtual bool onInit()
		{
			::Global::GUI().cleanupWidget();
			restart();
			return true;
		}

		virtual void onEnd()
		{
		}

		ERenderSystem getDefaultRenderSystem() override
		{
			return ERenderSystem::OpenGL;
		}

		bool setupRenderResource(ERenderSystem systemName) override
		{
			return FMeshBuild::CubeOffset(mCube, 0.5f, Vector3(0.5f, 0.5f, 0.5f));
		}

		void preShutdownRenderSystem(bool bReInit = false) override
		{
			mCube.releaseRHIResource();
		}



		void restart()
		{
			bInvRotation = false;
			idxCur = 0;
			mState[0].setGlobalState();
			mState[1].setGlobalState();
			mAnim.bPlaying = false;
			mAnim.time = 0;
			mAnim.duration = 0.22f;
		}

		virtual void onUpdate(GameTimeSpan deltaTime)
		{
			BaseClass::onUpdate(deltaTime);
			if ( mAnim.bPlaying )
			{
				mAnim.time += deltaTime.value;
				if ( mAnim.time >= mAnim.duration )
				{
					mAnim.bPlaying = false;
					mAnim.time = 0;

					int idxNext = 1 - idxCur;
					if ( mAnim.bInverse )
						CubeOperator::RotateInv( mState[idxCur] , mAnim.dir , mState[idxNext] );
					else
						CubeOperator::Rotate( mState[idxCur] , mAnim.dir , mState[idxNext] );
					idxCur = idxNext;
				}
			}
		}

		void onRender(float dFrame)
		{
			RHICommandList& commandList = RHICommandList::GetImmediateList();
			RHISetFrameBuffer(commandList, nullptr);
			RHIClearRenderTargets(commandList, EClearBits::Color | EClearBits::Depth, &LinearColor(0.18f, 0.22f, 0.28f, 1.0f), 1);

			Vec2i screenSize = ::Global::GetScreenSize();
			RHISetViewport(commandList, 0, 0, screenSize.x, screenSize.y);

			float aspect = float(screenSize.x) / screenSize.y;
			Matrix4 projectionMatrix = PerspectiveMatrix(Math::DegToRad(42.0f), aspect, 0.01f, 100.0f);
			Vector3 lookPos = Vector3(0, 0, 0);
			Vector3 camPos = Vector3(-6.3f, -8.2f, 5.8f);
			Matrix4 viewMatrix = LookAtMatrix(camPos, lookPos - camPos, Vector3(0, 1, 0));

			RHISetDepthStencilState(commandList, TStaticDepthStencilState<>::GetRHI());
			RHISetBlendState(commandList, TStaticBlendState<>::GetRHI());
			RHISetRasterizerState(commandList, TStaticRasterizerState<ECullMode::Back, EFillMode::Solid>::GetRHI());

			mStack.set(viewMatrix * AdjustProjectionMatrixForRHI(projectionMatrix));
			drawCubeRHI(commandList, mState[idxCur]);

			IGraphics2D& g = Global::GetIGraphics2D();
			drawCube(g, Vec2i(40, 40), mState[idxCur]);
			if ( mAnim.bPlaying )
			{
				drawCube(g, Vec2i(40, 290), mState[idxCur]);
			}
		}

		MsgReply onMouse(MouseMsg const& msg) override
		{
			return BaseClass::onMouse(msg);
		}

		MsgReply onKey(KeyMsg const& msg) override
		{
			if (msg.isDown())
			{
				if ( mAnim.bPlaying )
					return BaseClass::onKey(msg);

				switch (msg.getCode())
				{

				case EKeyCode::R: restart(); break;
				case EKeyCode::A: rotateCube(FaceLeft, bInvRotation); break;
				case EKeyCode::S: rotateCube(FaceFront, bInvRotation); break;
				case EKeyCode::D: rotateCube(FaceRight, bInvRotation); break;
				case EKeyCode::F: rotateCube(FaceBack, bInvRotation); break;
				case EKeyCode::W: rotateCube(FaceUp, bInvRotation); break;
				case EKeyCode::X: rotateCube(FaceDown, bInvRotation); break;
				case EKeyCode::E: bInvRotation = !bInvRotation; break;
				case EKeyCode::P:
					{
						solver.mInitState = mState[idxCur];
						solver.mFinalState.setGlobalState();
						solver.run();
					}
					break;
				}
			}
			return BaseClass::onKey(msg);
		}

		void rotateCube( FaceDir dir , bool bInverse = false )
		{
			if ( mAnim.bPlaying )
				return;

			mAnim.bPlaying = true;
			mAnim.dir = dir;
			mAnim.bInverse = bInverse;
			mAnim.time = 0;
		}

		void drawCube( IGraphics2D& g , Vec2i const& pos , CubeState const& state )
		{
			for ( int i = 0 ; i < CountFace ; ++i )
			{
				drawFace( g , pos + ( CubeBlockSize * BlockLen ) * FaceOffset[i] , state , FaceDir(i) );
			}
		}

		void drawFace( IGraphics2D& g , Vec2i const& pos , CubeState const& state , FaceDir dir )
		{
			RenderUtility::SetPen(g, EColor::Black);
			for ( int i = 0 ; i < CubeBlockSize * CubeBlockSize ; ++i )
			{
				RenderUtility::SetBrush(g, BlockColor[state.getBlockValue(dir, i)]);
				g.drawRect(pos + BlockLen * BlockOffset[i], BlockSize);
			}
		}

		static Vector3 GetFaceNormal(FaceDir dir)
		{
			switch ( dir )
			{
			case FaceFront: return Vector3(0, 0, 1);
			case FaceBack:  return Vector3(0, 0, -1);
			case FaceRight: return Vector3(-1, 0, 0);
			case FaceLeft:  return Vector3(1, 0, 0);
			case FaceUp:    return Vector3(0, 1, 0);
			case FaceDown:  return Vector3(0, -1, 0);
			default:        return Vector3(0, 0, 1);
			}
		}

		static bool IsLayerCubie(FaceDir dir, int x, int y, int z)
		{
			switch ( dir )
			{
			case FaceFront: return z == 2;
			case FaceBack:  return z == 0;
			case FaceRight: return x == 0;
			case FaceLeft:  return x == 2;
			case FaceUp:    return y == 2;
			case FaceDown:  return y == 0;
			default:        return false;
			}
		}

		static void GetFaceCellCoord(FaceDir dir, int row, int col, int& outX, int& outY, int& outZ)
		{
			switch ( dir )
			{
			case FaceFront: outX = 2 - col; outY = 2 - row; outZ = 2;       break;
			case FaceBack:  outX = col;     outY = 2 - row; outZ = 0;       break;
			case FaceRight: outX = 0;       outY = 2 - row; outZ = 2 - col; break;
			case FaceLeft:  outX = 2;       outY = 2 - row; outZ = col;     break;
			case FaceUp:    outX = 2 - col; outY = 2;       outZ = row;     break;
			case FaceDown:  outX = 2 - col; outY = 0;       outZ = 2 - row; break;
			default:        outX = 2 - col; outY = 2 - row; outZ = 2;       break;
			}
		}

		static LinearColor ToLinearColor(int colorId)
		{
			Color3f color(RenderUtility::GetColor(colorId));
			return LinearColor(color.r, color.g, color.b, 1.0f);
		}

		Vector3 GetRotationAxis(FaceDir dir) const
		{
			switch ( dir )
			{
			case FaceRight: return Vector3(1, 0, 0);
			case FaceLeft:  return Vector3(-1, 0, 0);
			default:        return GetFaceNormal(dir);
			}
		}

		Vector3 GetLayerPivot(FaceDir dir) const
		{
			return GetFaceNormal(dir) * CubeSpacing;
		}

		float GetAnimationAngle() const
		{
			if ( !mAnim.bPlaying || mAnim.duration <= 0 )
				return 0;

			float alpha = Math::Clamp(mAnim.time / mAnim.duration, 0.0f, 1.0f);
			alpha = alpha * alpha * (3.0f - 2.0f * alpha);
			float sign = mAnim.bInverse ? 1.0f : -1.0f;
			return sign * alpha * 0.5f * Math::PI;
		}

		Vector3 GetCubieMinPos(int x, int y, int z) const
		{
			Vector3 center((x - 1) * CubeSpacing, (y - 1) * CubeSpacing, (z - 1) * CubeSpacing);
			return center - Vector3(0.5f * CubieSize, 0.5f * CubieSize, 0.5f * CubieSize);
		}

		void SetupLayerRotation(int x, int y, int z)
		{
			if ( !( mAnim.bPlaying && IsLayerCubie(mAnim.dir, x, y, z) ) )
				return;

			Vector3 pivot = GetLayerPivot(mAnim.dir);
			mStack.translate(pivot);
			mStack.rotate(Quaternion::Rotate(GetRotationAxis(mAnim.dir), GetAnimationAngle()));
			mStack.translate(-pivot);
		}

		void DrawScaledCube(RHICommandList& commandList, Vector3 const& pos, Vector3 const& size, LinearColor const& color)
		{
			mStack.push();
			mStack.translate(pos);
			mStack.scale(size);
			RHISetFixedShaderPipelineState(commandList, mStack.get(), color);
			mCube.draw(commandList);
			mStack.pop();
		}

		void DrawCubieBase(RHICommandList& commandList, int x, int y, int z)
		{
			mStack.push();
			SetupLayerRotation(x, y, z);
			DrawScaledCube(commandList, GetCubieMinPos(x, y, z), Vector3(CubieSize, CubieSize, CubieSize), LinearColor(0, 0, 0, 1));
			mStack.pop();
		}

		void DrawSticker(RHICommandList& commandList, CubeState const& state, FaceDir dir, int row, int col)
		{
			int x, y, z;
			GetFaceCellCoord(dir, row, col, x, y, z);

			Vector3 stickerPos = GetCubieMinPos(x, y, z);
			Vector3 stickerSize(StickerExtent, StickerExtent, StickerExtent);
			switch ( dir )
			{
			case FaceFront:
				stickerPos += Vector3(StickerInset, StickerInset, CubieSize + StickerLift);
				stickerSize = Vector3(StickerExtent, StickerExtent, StickerThickness);
				break;
			case FaceBack:
				stickerPos += Vector3(StickerInset, StickerInset, -StickerThickness - StickerLift);
				stickerSize = Vector3(StickerExtent, StickerExtent, StickerThickness);
				break;
			case FaceRight:
				stickerPos += Vector3(-StickerThickness - StickerLift, StickerInset, StickerInset);
				stickerSize = Vector3(StickerThickness, StickerExtent, StickerExtent);
				break;
			case FaceLeft:
				stickerPos += Vector3(CubieSize + StickerLift, StickerInset, StickerInset);
				stickerSize = Vector3(StickerThickness, StickerExtent, StickerExtent);
				break;
			case FaceUp:
				stickerPos += Vector3(StickerInset, CubieSize + StickerLift, StickerInset);
				stickerSize = Vector3(StickerExtent, StickerThickness, StickerExtent);
				break;
			case FaceDown:
				stickerPos += Vector3(StickerInset, -StickerThickness - StickerLift, StickerInset);
				stickerSize = Vector3(StickerExtent, StickerThickness, StickerExtent);
				break;
			default:
				break;
			}

			int index = FaceGridIndexMap[row][col];
			LinearColor color = ToLinearColor(BlockColor[state.getBlockValue(dir, index)]);

			mStack.push();
			SetupLayerRotation(x, y, z);
			DrawScaledCube(commandList, stickerPos, stickerSize, color);
			mStack.pop();
		}

		void drawCubeRHI(RHICommandList& commandList, CubeState const& state)
		{
			for ( int z = 0; z < CubeBlockSize; ++z )
			{
				for ( int y = 0; y < CubeBlockSize; ++y )
				{
					for ( int x = 0; x < CubeBlockSize; ++x )
					{
						DrawCubieBase(commandList, x, y, z);
					}
				}
			}

			for ( int face = 0; face < CountFace; ++face )
			{
				for ( int row = 0; row < CubeBlockSize; ++row )
				{
					for ( int col = 0; col < CubeBlockSize; ++col )
					{
						DrawSticker(commandList, state, FaceDir(face), row, col);
					}
				}
			}
		}

		static float constexpr CubeSpacing = 1.02f;
		static float constexpr CubieSize = 0.96f;
		static float constexpr StickerInset = 0.10f;
		static float constexpr StickerThickness = 0.02f;
		static float constexpr StickerLift = 0.01f;
		static float constexpr StickerExtent = CubieSize - 2 * StickerInset;

		Solver solver;

		CubeState mState[2];
		int  idxCur;
		bool bInvRotation;
		RotationAnim mAnim;
		Render::TTransformStack< true > mStack;
		Render::Mesh mCube;
	};


}//namespace Rubiks

#endif // RubiksStage_h__c781c03d_43f7_4882_b367_96b003d27fbb
