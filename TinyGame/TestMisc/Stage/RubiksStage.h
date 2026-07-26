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

	//     U2
	//  L4 F0 R1 B3
	//     D5
	//  0 1 2
	//  7 8 3
	//  6 5 4  
	enum FaceDir
	{
		FaceFront = 0,
		FaceRight = 1,
		FaceUp    = 2,
		FaceBack  = 3,
		FaceLeft  = 4,
		FaceDown  = 5,

		CountFace = 6,
	};

	FORCEINLINE bool IsOppositeFace(FaceDir a, FaceDir b)
	{
		return ( a % 3 ) == ( b % 3 );
	}

	int const CubeBlockSize = 3;
	int const CubeFaceEdgeNum = 4;

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
		struct MoveCache
		{
			bool  bInitialized = false;
			uint8 cornerDest[2][CountFace][8];
			uint8 edgeDest[2][CountFace][12];
			uint8 cornerOri[2][CountFace][8][3];
			uint8 edgeOri[2][CountFace][12][2];
		};
		struct SymmetryTransform
		{
			int8 axis[3];
			int8 sign[3];
		};
		struct SymmetryCache
		{
			bool  bInitialized = false;
			uint8 cornerDest[48][8];
			uint8 edgeDest[48][12];
			uint8 cornerValue[48][8][8][3];
			uint8 edgeValue[48][12][12][2];
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

		bool isLess( CubeState const& other ) const
		{
			if ( cornerKey != other.cornerKey )
				return cornerKey < other.cornerKey;
			return edgeKey < other.edgeKey;
		}

		void setEmptyState()
		{
			cornerKey = 0;
			edgeKey = 0;
			updateHash();
		}

		void setGoalState()
		{
			cornerKey = 0;
			edgeKey = 0;
			for ( uint8 i = 0 ; i < 8 ; ++i )
			{
				setCorner(i, i, 0);
			}
			for ( uint8 i = 0 ; i < 12 ; ++i )
			{
				setEdge(i, i, 0);
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
				getCorner(desc.pos, piece, ori);
				FaceDir colorFace = GetCornerPieceFaces()[piece][Mod(desc.slot - ori, 3)];
				return GetColorValue(colorFace);
			}

			uint8 piece;
			uint8 ori;
			getEdge(desc.pos, piece, ori);
			FaceDir colorFace = GetEdgePieceFaces()[piece][Mod(desc.slot - ori, 2)];
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
				oldState.getCorner(i, piece, ori);
				newState.setCorner(dest, piece, cache.cornerOri[invIndex][dir][i][ori]);
			}
			for ( int i = 0 ; i < 12 ; ++i )
			{
				uint8 dest = cache.edgeDest[invIndex][dir][i];
				uint8 piece;
				uint8 ori;
				oldState.getEdge(i, piece, ori);
				newState.setEdge(dest, piece, cache.edgeOri[invIndex][dir][i][ori]);
			}
			newState.updateHash();
		}

		static uint64 GetFieldMask(int bits, int index)
		{
			return ((uint64(1) << bits) - 1) << (bits * index);
		}

		void getCorner(uint8 index, uint8& outPiece, uint8& outOri) const
		{
			uint64 value = (cornerKey >> (5 * index)) & 0x1f;
			outPiece = value & 0x7;
			outOri = (value >> 3) & 0x3;
		}

		void setCorner(uint8 index, uint8 piece, uint8 ori)
		{
			uint64 value = (piece & 0x7) | ((uint64(ori & 0x3)) << 3);
			cornerKey = (cornerKey & ~GetFieldMask(5, index)) | (value << (5 * index));
		}

		void getEdge(uint8 index, uint8& outPiece, uint8& outOri) const
		{
			uint64 value = (edgeKey >> (5 * index)) & 0x1f;
			outPiece = value & 0xf;
			outOri = (value >> 4) & 0x1;
		}

		void setEdge(uint8 index, uint8 piece, uint8 ori)
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

		void buildSymmetryCanonicalState(CubeState& outState) const
		{
			CubeState testState;
			BuildSymmetryState(0, testState);
			outState = testState;

			for ( int i = 1 ; i < GetSymmetryTransformCount() ; ++i )
			{
				BuildSymmetryState(i, testState);
				if ( testState.isLess(outState) )
					outState = testState;
			}
			outState.updateHash();
		}

		static int Mod(int value, int mod)
		{
			int result = value % mod;
			return ( result < 0 ) ? ( result + mod ) : result;
		}

		static uint32 GetColorValue(FaceDir face)
		{
			return ( face != CountFace ) ? uint32(face) + 1 : 0;
		}

		static Coord GetFaceCoord(FaceDir face)
		{
			switch ( face )
			{
			case FaceUp:    return { 0, 0, 1 };
			case FaceRight: return { 1, 0, 0 };
			case FaceFront: return { 0, 1, 0 };
			case FaceDown:  return { 0, 0,-1 };
			case FaceLeft:  return {-1, 0, 0 };
			case FaceBack:  return { 0,-1, 0 };
			default:        return { 0, 0, 0 };
			}
		}

		static FaceDir GetFaceFromCoord(Coord coord)
		{
			if ( coord.x == 1 ) return FaceRight;
			if ( coord.x == -1 ) return FaceLeft;
			if ( coord.y == 1 ) return FaceFront;
			if ( coord.y == -1 ) return FaceBack;
			if ( coord.z == 1 ) return FaceUp;
			if ( coord.z == -1 ) return FaceDown;
			return CountFace;
		}

		static Coord TransformCoord(SymmetryTransform const& transform, Coord coord)
		{
			int8 values[3] = { coord.x, coord.y, coord.z };
			return Coord
			{
				int8(transform.sign[0] * values[transform.axis[0]]),
				int8(transform.sign[1] * values[transform.axis[1]]),
				int8(transform.sign[2] * values[transform.axis[2]])
			};
		}

		static FaceDir TransformFace(SymmetryTransform const& transform, FaceDir face)
		{
			return GetFaceFromCoord(TransformCoord(transform, GetFaceCoord(face)));
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
			case FaceFront: axis = 1; axisSign = -1; break;
			case FaceBack:  axis = 1; axisSign = 1; break;
			case FaceUp:    axis = 2; axisSign = -1; break;
			case FaceDown:  axis = 2; axisSign = 1; break;
			}

			int quarterTurn = ( bInverse ? 1 : -1 ) * axisSign;
			return RotateAroundPositiveAxis(axis, quarterTurn, coord);
		}

		static FaceDir RotateFace(FaceDir dir, bool bInverse, FaceDir face)
		{
			return GetFaceFromCoord(RotateCoord(dir, bInverse, GetFaceCoord(face)));
		}

		static bool IsOnLayer(FaceDir dir, Coord coord)
		{
			switch ( dir )
			{
			case FaceFront: return coord.y == 1;
			case FaceBack:  return coord.y == -1;
			case FaceRight: return coord.x == 1;
			case FaceLeft:  return coord.x == -1;
			case FaceUp:    return coord.z == 1;
			case FaceDown:  return coord.z == -1;
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
				if ( row == 2 && col == 1 ) return { 1, 0, false };
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
				{ 1, 1, 1 }, { -1, 1, 1 }, { -1,-1, 1 }, { 1,-1, 1 },
				{ 1, 1,-1 }, { -1, 1,-1 }, { -1,-1,-1 }, { 1,-1,-1 },
			};
			return Data;
		}

		static Coord const (&GetEdgeCoords())[12]
		{
			static Coord const Data[12] =
			{
				{ 1, 0, 1 }, { 0, 1, 1 }, { -1, 0, 1 }, { 0,-1, 1 },
				{ 1, 0,-1 }, { 0, 1,-1 }, { -1, 0,-1 }, { 0,-1,-1 },
				{ 1, 1, 0 }, { -1, 1, 0 }, { -1,-1, 0 }, { 1,-1, 0 },
			};
			return Data;
		}

		static FaceDir const (&GetCornerPieceFaces())[8][3]
		{
			static FaceDir const Data[8][3] =
			{
				{ FaceUp, FaceRight, FaceFront }, { FaceUp, FaceFront, FaceLeft }, { FaceUp, FaceLeft, FaceBack }, { FaceUp, FaceBack, FaceRight },
				{ FaceDown, FaceFront, FaceRight }, { FaceDown, FaceLeft, FaceFront }, { FaceDown, FaceBack, FaceLeft }, { FaceDown, FaceRight, FaceBack },
			};
			return Data;
		}

		static FaceDir const (&GetEdgePieceFaces())[12][2]
		{
			static FaceDir const Data[12][2] =
			{
				{ FaceUp, FaceRight }, { FaceUp, FaceFront }, { FaceUp, FaceLeft }, { FaceUp, FaceBack },
				{ FaceDown, FaceRight }, { FaceDown, FaceFront }, { FaceDown, FaceLeft }, { FaceDown, FaceBack },
				{ FaceFront, FaceRight }, { FaceFront, FaceLeft }, { FaceBack, FaceLeft }, { FaceBack, FaceRight },
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
							FaceDir sourceOutwardFace = GetCornerPosFaces()[i][ori];
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
							FaceDir sourceOutwardFace = GetEdgePosFaces()[i][ori];
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

		static FaceDir const (&GetCornerPosFaces())[8][3]
		{
			return GetCornerPieceFaces();
		}

		static FaceDir const (&GetEdgePosFaces())[12][2]
		{
			return GetEdgePieceFaces();
		}

		static SymmetryCache& GetSymmetryCache()
		{
			static SymmetryCache cache;
			if ( cache.bInitialized )
				return cache;

			auto const& cornerCoords = GetCornerCoords();
			auto const& edgeCoords = GetEdgeCoords();
			auto const& cornerPosFaces = GetCornerPosFaces();
			auto const& edgePosFaces = GetEdgePosFaces();

			for ( int transformIndex = 0 ; transformIndex < GetSymmetryTransformCount() ; ++transformIndex )
			{
				SymmetryTransform const& transform = GetSymmetryTransform(transformIndex);

				for ( uint8 pos = 0 ; pos < 8 ; ++pos )
				{
					int destIndex = FindCornerPos(TransformCoord(transform, cornerCoords[pos]));
					assert(destIndex != INDEX_NONE);
					uint8 dest = uint8(destIndex);
					cache.cornerDest[transformIndex][pos] = dest;

					for ( uint8 piece = 0 ; piece < 8 ; ++piece )
					{
						for ( uint8 ori = 0 ; ori < 3 ; ++ori )
						{
							FaceDir colors[3] = { CountFace, CountFace, CountFace };
							for ( uint8 slot = 0 ; slot < 3 ; ++slot )
							{
								FaceDir outwardFace = TransformFace(transform, cornerPosFaces[pos][slot]);
								FaceDir colorFace = TransformFace(transform, GetCornerPieceFaces()[piece][Mod(slot - ori, 3)]);
								for ( uint8 destSlot = 0 ; destSlot < 3 ; ++destSlot )
								{
									if ( cornerPosFaces[dest][destSlot] == outwardFace )
									{
										colors[destSlot] = colorFace;
										break;
									}
								}
							}

							uint8 newPiece = 0;
							uint8 newOri = 0;
							FindCornerPieceFromColors(colors, newPiece, newOri);
							cache.cornerValue[transformIndex][pos][piece][ori] = newPiece | (newOri << 3);
						}
					}
				}

				for ( uint8 pos = 0 ; pos < 12 ; ++pos )
				{
					int destIndex = FindEdgePos(TransformCoord(transform, edgeCoords[pos]));
					assert(destIndex != INDEX_NONE);
					uint8 dest = uint8(destIndex);
					cache.edgeDest[transformIndex][pos] = dest;

					for ( uint8 piece = 0 ; piece < 12 ; ++piece )
					{
						for ( uint8 ori = 0 ; ori < 2 ; ++ori )
						{
							FaceDir colors[2] = { CountFace, CountFace };
							for ( uint8 slot = 0 ; slot < 2 ; ++slot )
							{
								FaceDir outwardFace = TransformFace(transform, edgePosFaces[pos][slot]);
								FaceDir colorFace = TransformFace(transform, GetEdgePieceFaces()[piece][Mod(slot - ori, 2)]);
								for ( uint8 destSlot = 0 ; destSlot < 2 ; ++destSlot )
								{
									if ( edgePosFaces[dest][destSlot] == outwardFace )
									{
										colors[destSlot] = colorFace;
										break;
									}
								}
							}

							uint8 newPiece = 0;
							uint8 newOri = 0;
							FindEdgePieceFromColors(colors, newPiece, newOri);
							cache.edgeValue[transformIndex][pos][piece][ori] = newPiece | (newOri << 4);
						}
					}
				}
			}

			cache.bInitialized = true;
			return cache;
		}

		void BuildSymmetryState(int transformIndex, CubeState& outState) const
		{
			SymmetryCache& cache = GetSymmetryCache();
			outState.cornerKey = 0;
			outState.edgeKey = 0;

			for ( uint8 i = 0 ; i < 8 ; ++i )
			{
				uint8 piece;
				uint8 ori;
				getCorner(i, piece, ori);
				uint8 dest = cache.cornerDest[transformIndex][i];
				uint64 value = cache.cornerValue[transformIndex][i][piece][ori];
				outState.cornerKey |= value << (5 * dest);
			}

			for ( uint8 i = 0 ; i < 12 ; ++i )
			{
				uint8 piece;
				uint8 ori;
				getEdge(i, piece, ori);
				uint8 dest = cache.edgeDest[transformIndex][i];
				uint64 value = cache.edgeValue[transformIndex][i][piece][ori];
				outState.edgeKey |= value << (5 * dest);
			}
		}

		void BuildSymmetryState(SymmetryTransform const& transform, CubeState& outState) const
		{
			outState.setEmptyState();

			auto const& cornerCoords = GetCornerCoords();
			auto const& edgeCoords = GetEdgeCoords();
			auto const& cornerPosFaces = GetCornerPosFaces();
			auto const& edgePosFaces = GetEdgePosFaces();

			for ( uint8 i = 0 ; i < 8 ; ++i )
			{
				uint8 piece;
				uint8 ori;
				getCorner(i, piece, ori);

				int dest = FindCornerPos(TransformCoord(transform, cornerCoords[i]));
				assert(dest != INDEX_NONE);

				FaceDir colors[3] = { CountFace, CountFace, CountFace };
				for ( uint8 slot = 0 ; slot < 3 ; ++slot )
				{
					FaceDir outwardFace = TransformFace(transform, cornerPosFaces[i][slot]);
					FaceDir colorFace = TransformFace(transform, GetCornerPieceFaces()[piece][Mod(slot - ori, 3)]);
					for ( uint8 destSlot = 0 ; destSlot < 3 ; ++destSlot )
					{
						if ( cornerPosFaces[dest][destSlot] == outwardFace )
						{
							colors[destSlot] = colorFace;
							break;
						}
					}
				}

				uint8 newPiece = 0;
				uint8 newOri = 0;
				FindCornerPieceFromColors(colors, newPiece, newOri);
				outState.setCorner(dest, newPiece, newOri);
			}

			for ( uint8 i = 0 ; i < 12 ; ++i )
			{
				uint8 piece;
				uint8 ori;
				getEdge(i, piece, ori);

				int dest = FindEdgePos(TransformCoord(transform, edgeCoords[i]));
				assert(dest != INDEX_NONE);

				FaceDir colors[2] = { CountFace, CountFace };
				for ( uint8 slot = 0 ; slot < 2 ; ++slot )
				{
					FaceDir outwardFace = TransformFace(transform, edgePosFaces[i][slot]);
					FaceDir colorFace = TransformFace(transform, GetEdgePieceFaces()[piece][Mod(slot - ori, 2)]);
					for ( uint8 destSlot = 0 ; destSlot < 2 ; ++destSlot )
					{
						if ( edgePosFaces[dest][destSlot] == outwardFace )
						{
							colors[destSlot] = colorFace;
							break;
						}
					}
				}

				uint8 newPiece = 0;
				uint8 newOri = 0;
				FindEdgePieceFromColors(colors, newPiece, newOri);
				outState.setEdge(dest, newPiece, newOri);
			}

			outState.updateHash();
		}

		static void FindCornerPieceFromColors(FaceDir const colors[3], uint8& outPiece, uint8& outOri)
		{
			auto const& pieceFaces = GetCornerPieceFaces();
			for ( uint8 piece = 0 ; piece < 8 ; ++piece )
			{
				for ( uint8 ori = 0 ; ori < 3 ; ++ori )
				{
					if ( pieceFaces[piece][Mod(0 - ori, 3)] == colors[0] &&
					     pieceFaces[piece][Mod(1 - ori, 3)] == colors[1] &&
					     pieceFaces[piece][Mod(2 - ori, 3)] == colors[2] )
					{
						outPiece = piece;
						outOri = ori;
						return;
					}
				}
			}
			assert(false);
		}

		static void FindEdgePieceFromColors(FaceDir const colors[2], uint8& outPiece, uint8& outOri)
		{
			auto const& pieceFaces = GetEdgePieceFaces();
			for ( uint8 piece = 0 ; piece < 12 ; ++piece )
			{
				for ( uint8 ori = 0 ; ori < 2 ; ++ori )
				{
					if ( pieceFaces[piece][Mod(0 - ori, 2)] == colors[0] &&
					     pieceFaces[piece][Mod(1 - ori, 2)] == colors[1] )
					{
						outPiece = piece;
						outOri = ori;
						return;
					}
				}
			}
			assert(false);
		}

		static int GetSymmetryTransformCount()
		{
			return 48;
		}

		static SymmetryTransform const& GetSymmetryTransform(int index)
		{
			static bool bInitialized = false;
			static SymmetryTransform transforms[48];
			if ( !bInitialized )
			{
				static int8 const Permutations[6][3] =
				{
					{ 0, 1, 2 }, { 0, 2, 1 }, { 1, 0, 2 },
					{ 1, 2, 0 }, { 2, 0, 1 }, { 2, 1, 0 },
				};

				int outIndex = 0;
				for ( int p = 0 ; p < 6 ; ++p )
				{
					for ( int signMask = 0 ; signMask < 8 ; ++signMask )
					{
						for ( int axis = 0 ; axis < 3 ; ++axis )
						{
							transforms[outIndex].axis[axis] = Permutations[p][axis];
							transforms[outIndex].sign[axis] = (signMask & (1 << axis)) ? -1 : 1;
						}
						++outIndex;
					}
				}
				bInitialized = true;
			}
			return transforms[index];
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
		Solver()
			: mbRunning(false)
			, mGoalTableDepth(-1)
		{
		}

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
		void buildGoalDepthTable(int depth);
		bool isInGoalDepthTable(CubeState const& canonicalState) const;

		struct StateNode
		{
			StateNode* parent;
			CubeState  state;
			CubeState  canonicalState;
			FaceDir    rotation;
			bool       bInverse;
			int        depth;
		};
		int generateNextNodes(StateNode* node , StateNode* nextNodes[]);
		static bool ShouldPruneMove(StateNode const* node, FaceDir dir, bool bInverse);
		static bool ShouldPruneMove(FaceDir prevDir, bool bPrevInverse, bool bHavePrevMove, FaceDir dir, bool bInverse);
		static int GetFacePairOrder(FaceDir dir);


		bool mbRunning;
		ConditionVariable mRequestFindCond;
		ConditionVariable mUncheckCond;
		Mutex mRequestFindMutex;
		Mutex mUncheckMutex;
		std::deque< StateNode* > mRequestFindNodes;
		std::deque< StateNode* > mUncheckNodes;
		TArray< StateNode* > mAllocNodes;
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
		struct StateValueEqual
		{
			bool operator ()( CubeState const& a , CubeState const& b ) const
			{
				return a.isEqual(b);
			}
		};
		struct StateValueHash
		{
			std::size_t operator()( CubeState const& a ) const
			{
				return a.hashValue;
			}
		};

		typedef std::unordered_set< CubeState* , StateHash , StateEqual > StateSet;
		typedef std::unordered_set< CubeState , StateValueHash , StateValueEqual > StateValueSet;
		StateSet mCheckedStates;
		StateValueSet mGoalDepthStates;
		CubeState mGoalTableFinalCanonical;
		int mGoalTableDepth;

		CubeState mInitState;
		CubeState mFinalState;
	};

	class FastSolver
	{
	public:
		static int const COCount = 2187;
		static int const EOCount = 2048;
		static int const SliceCount = 495;
		static int const Perm8Count = 40320;
		static int const Perm4Count = 24;
		static int const MoveCount = CountFace * 3;
		static int const Phase2MoveCount = 10;

		struct SearchNode
		{
			FaceDir  rotation;
			uint8    power;
		};
		struct IDASearchContext
		{
			int nodeCount = 0;
			TArray< SearchNode > path;
		};
		struct CoordState
		{
			uint16 co;
			uint16 eo;
			uint16 slice;
			uint16 cp;
			uint16 udEdgePerm;
			uint16 slicePerm;
		};

		struct CoordMoveTables
		{
			uint16 co[COCount][MoveCount];
			uint16 eo[EOCount][MoveCount];
			uint16 slice[SliceCount][MoveCount];
			uint16 cp[Perm8Count][Phase2MoveCount];
			uint16 udEdgePerm[Perm8Count][Phase2MoveCount];
			uint16 slicePerm[Perm4Count][Phase2MoveCount];
			int phase2MoveToMove[Phase2MoveCount];
		};
		struct PruningTables
		{
			int8 cornerOri[COCount];
			int8 edgeOri[EOCount];
			int8 slice[SliceCount];
			int8 cornerPerm[Perm8Count];
			int8 udEdgePerm[Perm8Count];
			int8 slicePerm[Perm4Count];
		};

		bool run();
		bool runTwoPhase();
		bool searchPhase1(CoordState const& state, int depth, int bound, FaceDir prevDir, bool bHavePrevMove, IDASearchContext& context);
		bool searchPhase2(CoordState const& state, int depth, int bound, FaceDir prevDir, bool bHavePrevMove, IDASearchContext& context);

		static int GetFacePairOrder(FaceDir dir);
		static bool ShouldPruneMove(FaceDir prevDir, bool bHavePrevMove, FaceDir dir);
		static bool IsPhase2Move(FaceDir dir, uint8 power);
		static bool IsPhase1Goal(CoordState const& state);
		static bool ApplyMove(CubeState const& state, FaceDir dir, uint8 power, CubeState& outState);
		static PruningTables& GetPruningTables();
		static CoordMoveTables& GetCoordMoveTables();
		static CoordState MakeCoordState(CubeState const& state);
		static CoordState ApplyCoordPhase2Move(CoordState const& state, int phase2MoveIndex);
		static FaceDir GetMoveFace(int moveIndex);
		static uint8 GetMovePower(int moveIndex);
		static int GetMoveIndex(FaceDir dir, uint8 power);
		static void BuildCoordPruningTable(int8* table, int tableSize, uint16 startCoord, uint16 const* moveTable, int moveStride, int numMoves);
		static int CalcCornerOriCoord(CubeState const& state);
		static int CalcEdgeOriCoord(CubeState const& state);
		static int CalcSliceCoord(CubeState const& state);
		static int CalcCornerPermCoord(CubeState const& state);
		static int CalcUDEdgePermCoord(CubeState const& state);
		static int CalcSlicePermCoord(CubeState const& state);
		static int CalcCombinationRank(uint32 mask, int numBits, int chooseBits);
		static int CalcPermutationRank(uint8 const* values, int numValues);
		static void BuildPermutationFromRank(int rank, int numValues, uint8* outValues);
		static uint32 BuildCombinationMaskFromRank(int rank, int numBits, int chooseBits);
		static void BuildCornerOriState(int coord, CubeState& outState);
		static void BuildEdgeOriState(int coord, CubeState& outState);
		static void BuildSliceState(int coord, CubeState& outState);
		static void BuildCornerPermState(int coord, CubeState& outState);
		static void BuildUDEdgePermState(int coord, CubeState& outState);
		static void BuildSlicePermState(int coord, CubeState& outState);
		static int CalcPhase1Heuristic(CoordState const& state);
		static int CalcPhase2Heuristic(CoordState const& state);

		CubeState mInitState;
		CubeState mFinalState;
		TArray< SearchNode > mSolution;
	};

	static int BlockColor[] = 
	{
		EColor::Black ,
		EColor::Red ,
		EColor::Blue ,
		EColor::White ,
		EColor::Orange ,
		EColor::Green ,
		EColor::Yellow ,
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
		Vec2i(1,1) , Vec2i(2,1) , Vec2i(1,0) , Vec2i(3,1) , Vec2i(0,1) , Vec2i(1,2) ,
	};

	class TestStage : public StageBase
		            , public IGameRenderSetup
	{
		typedef StageBase BaseClass;
	public:
		struct RotationAnim
		{
			bool    bPlaying;
			FaceDir stateDir;
			FaceDir renderDir;
			bool    bInverse;
			float   time;
			float   duration;
		};
		struct QueuedMove
		{
			FaceDir dir;
			bool    bInverse;
		};

		TestStage(){}
		virtual bool onInit()
		{
			::Global::GUI().cleanupWidget();
			setupDevFrame();
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
			mState[0].setGoalState();
			mState[1].setGoalState();
			mAnim.bPlaying = false;
			mAnim.stateDir = FaceFront;
			mAnim.renderDir = FaceFront;
			mAnim.time = 0;
			mAnim.duration = 0.22f;
			mCameraYaw = Math::DegToRad(42.0f);
			mCameraPitch = Math::DegToRad(43.7f);
			mCameraDistance = 11.9f;
			mbDraggingCamera = false;
			mLastMousePos = Vec2i(0, 0);
			mSolutionMoves.clear();
			mSolutionMoveIndex = 0;
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
						CubeOperator::RotateInv( mState[idxCur] , mAnim.stateDir , mState[idxNext] );
					else
						CubeOperator::Rotate( mState[idxCur] , mAnim.stateDir , mState[idxNext] );
					idxCur = idxNext;
					playNextSolutionMove();
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
			float cosPitch = Math::Cos(mCameraPitch);
			Vector3 camPos = lookPos + mCameraDistance * Vector3(
				Math::Cos(mCameraYaw) * cosPitch,
				Math::Sin(mCameraYaw) * cosPitch,
				Math::Sin(mCameraPitch));
			Matrix4 viewMatrix = LookAtMatrix(camPos, lookPos - camPos, Vector3(0, 0, 1));

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
			if ( msg.onRightDown() )
			{
				mbDraggingCamera = true;
				mLastMousePos = msg.getPos();
				return MsgReply::Handled();
			}

			if ( msg.onRightUp() )
			{
				mbDraggingCamera = false;
				return MsgReply::Handled();
			}

			if ( mbDraggingCamera && msg.isRightDown() && msg.onMoving() )
			{
				Vec2i delta = msg.getPos() - mLastMousePos;
				mLastMousePos = msg.getPos();

				float constexpr RotateSpeed = 0.008f;
				mCameraYaw -= delta.x * RotateSpeed;
				mCameraPitch = Math::Clamp(mCameraPitch + delta.y * RotateSpeed, Math::DegToRad(-82.0f), Math::DegToRad(82.0f));
				return MsgReply::Handled();
			}

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
						solveCurrentCube();
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

			mSolutionMoves.clear();
			mSolutionMoveIndex = 0;
			startRotationAnim(dir, bInverse);
		}

		void startRotationAnim(FaceDir dir, bool bInverse)
		{
			mAnim.bPlaying = true;
			mAnim.stateDir = dir;
			mAnim.renderDir = dir;
			mAnim.bInverse = bInverse;
			mAnim.time = 0;
		}

		void setupDevFrame()
		{
			DevFrame* frame = WidgetUtility::CreateDevFrame();
			frame->addButton("Scramble", [this](int event, GWidget*) -> bool
			{
				scrambleCurrentCube();
				return false;
			});
			frame->addButton("Fast Solve", [this](int event, GWidget*) -> bool
			{
				solveCurrentCube();
				return false;
			});
		}

		void solveCurrentCube()
		{
			if ( mAnim.bPlaying )
				return;

			fastSolver.mInitState = mState[idxCur];
			fastSolver.mFinalState.setGoalState();
			if ( fastSolver.run() )
				applySolution(fastSolver.mSolution);
		}

		void applySolution(TArray< FastSolver::SearchNode > const& solution)
		{
			mSolutionMoves.clear();
			mSolutionMoveIndex = 0;

			for ( FastSolver::SearchNode const& move : solution )
			{
				if ( move.power == 3 )
				{
					mSolutionMoves.push_back({ move.rotation, true });
				}
				else
				{
					for ( uint8 i = 0 ; i < move.power ; ++i )
					{
						mSolutionMoves.push_back({ move.rotation, false });
					}
				}
			}

			playNextSolutionMove();
		}

		void playNextSolutionMove()
		{
			if ( mAnim.bPlaying )
				return;

			if ( mSolutionMoveIndex >= mSolutionMoves.size() )
				return;

			QueuedMove const& move = mSolutionMoves[mSolutionMoveIndex++];
			startRotationAnim(move.dir, move.bInverse);
		}

		void scrambleCurrentCube()
		{
			if ( mAnim.bPlaying )
				return;

			static int const ScrambleStepCount = 30;
			FaceDir prevDir = CountFace;
			for ( int i = 0 ; i < ScrambleStepCount ; ++i )
			{
				FaceDir dir = FaceDir(::Global::Random() % CountFace);
				if ( dir == prevDir )
					dir = FaceDir((uint32(dir) + 1 + ::Global::Random() % (CountFace - 1)) % CountFace);

				bool bInverse = (::Global::Random() & 1) != 0;
				int idxNext = 1 - idxCur;
				if ( bInverse )
					CubeOperator::RotateInv(mState[idxCur], dir, mState[idxNext]);
				else
					CubeOperator::Rotate(mState[idxCur], dir, mState[idxNext]);

				idxCur = idxNext;
				prevDir = dir;
			}
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
			case FaceFront: return Vector3(0, 1, 0);
			case FaceBack:  return Vector3(0, -1, 0);
			case FaceRight: return Vector3(1, 0, 0);
			case FaceLeft:  return Vector3(-1, 0, 0);
			case FaceUp:    return Vector3(0, 0, 1);
			case FaceDown:  return Vector3(0, 0, -1);
			default:        return Vector3(0, 1, 0);
			}
		}

		static bool IsLayerCubie(FaceDir dir, int x, int y, int z)
		{
			switch ( dir )
			{
			case FaceFront: return y == 2;
			case FaceBack:  return y == 0;
			case FaceRight: return x == 2;
			case FaceLeft:  return x == 0;
			case FaceUp:    return z == 2;
			case FaceDown:  return z == 0;
			default:        return false;
			}
		}

		static void GetFaceCellCoord(FaceDir dir, int row, int col, int& outX, int& outY, int& outZ)
		{
			CubeState::Coord coord;
			if ( row == 1 && col == 1 )
			{
				coord = CubeState::GetFaceCoord(dir);
			}
			else
			{
				CubeState::FaceletDesc desc = CubeState::GetFaceletDesc(dir, row, col);
				coord = desc.bCorner ? CubeState::GetCornerCoords()[desc.pos] : CubeState::GetEdgeCoords()[desc.pos];
			}

			outX = coord.x + 1;
			outY = coord.y + 1;
			outZ = coord.z + 1;
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
			case FaceRight:
			case FaceLeft:
				return Vector3(1, 0, 0);
			case FaceFront:
			case FaceBack:
				return Vector3(0, 1, 0);
			case FaceUp:
			case FaceDown:
				return Vector3(0, 0, 1);
			default:
				return Vector3(0, 1, 0);
			}
		}

		static int GetRotationAxisSign(FaceDir dir)
		{
			switch ( dir )
			{
			case FaceRight: return 1;
			case FaceLeft:  return -1;
			case FaceFront: return -1;
			case FaceBack:  return 1;
			case FaceUp:    return -1;
			case FaceDown:  return 1;
			default:        return 1;
			}
		}

		Vector3 GetLayerPivot(FaceDir dir) const
		{
			return GetFaceNormal(dir) * CubeSpacing;
		}

		float GetAnimationAngle(FaceDir dir) const
		{
			if ( !mAnim.bPlaying || mAnim.duration <= 0 )
				return 0;

			float alpha = Math::Clamp(mAnim.time / mAnim.duration, 0.0f, 1.0f);
			alpha = alpha * alpha * (3.0f - 2.0f * alpha);
			float sign = float(( mAnim.bInverse ? 1 : -1 ) * GetRotationAxisSign(dir));
			return sign * alpha * 0.5f * Math::PI;
		}

		Vector3 GetCubieMinPos(int x, int y, int z) const
		{
			Vector3 center((x - 1) * CubeSpacing, (y - 1) * CubeSpacing, (z - 1) * CubeSpacing);
			return center - Vector3(0.5f * CubieSize, 0.5f * CubieSize, 0.5f * CubieSize);
		}

		void SetupLayerRotation(int x, int y, int z)
		{
			if ( !( mAnim.bPlaying && IsLayerCubie(mAnim.renderDir, x, y, z) ) )
				return;

			Vector3 pivot = GetLayerPivot(mAnim.renderDir);
			mStack.translate(pivot);
			mStack.rotate(Quaternion::Rotate(GetRotationAxis(mAnim.renderDir), GetAnimationAngle(mAnim.renderDir)));
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
				stickerPos += Vector3(StickerInset, CubieSize + StickerLift, StickerInset);
				stickerSize = Vector3(StickerExtent, StickerThickness, StickerExtent);
				break;
			case FaceBack:
				stickerPos += Vector3(StickerInset, -StickerThickness - StickerLift, StickerInset);
				stickerSize = Vector3(StickerExtent, StickerThickness, StickerExtent);
				break;
			case FaceRight:
				stickerPos += Vector3(CubieSize + StickerLift, StickerInset, StickerInset);
				stickerSize = Vector3(StickerThickness, StickerExtent, StickerExtent);
				break;
			case FaceLeft:
				stickerPos += Vector3(-StickerThickness - StickerLift, StickerInset, StickerInset);
				stickerSize = Vector3(StickerThickness, StickerExtent, StickerExtent);
				break;
			case FaceUp:
				stickerPos += Vector3(StickerInset, StickerInset, CubieSize + StickerLift);
				stickerSize = Vector3(StickerExtent, StickerExtent, StickerThickness);
				break;
			case FaceDown:
				stickerPos += Vector3(StickerInset, StickerInset, -StickerThickness - StickerLift);
				stickerSize = Vector3(StickerExtent, StickerExtent, StickerThickness);
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

		static float constexpr CubeSpacing = 1.0f;
		static float constexpr CubieSize = 1.0f;
		static float constexpr StickerInset = 0.10f;
		static float constexpr StickerThickness = 0.02f;
		static float constexpr StickerLift = 0.002f;
		static float constexpr StickerExtent = CubieSize - 2 * StickerInset;

		Solver solver;
		FastSolver fastSolver;

		CubeState mState[2];
		int  idxCur;
		bool bInvRotation;
		RotationAnim mAnim;
		float mCameraYaw = Math::DegToRad(42.0f);
		float mCameraPitch = Math::DegToRad(43.7f);
		float mCameraDistance = 11.9f;
		bool mbDraggingCamera = false;
		Vec2i mLastMousePos;
		TArray< QueuedMove > mSolutionMoves;
		int mSolutionMoveIndex = 0;
		Render::TTransformStack< true > mStack;
		Render::Mesh mCube;
	};


}//namespace Rubiks

#endif // RubiksStage_h__c781c03d_43f7_4882_b367_96b003d27fbb
