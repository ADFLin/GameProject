#pragma once
#ifndef StoreSimNav_H_2943C778_CFDE_4511_9CA4_0AC3BBC851CE
#define StoreSimNav_H_2943C778_CFDE_4511_9CA4_0AC3BBC851CE

#include "StoreSimCore.h"

#include "Math/GeometryPrimitive.h"
#include "Collision2D/SATSolver.h"
#include "Collision2D/Bsp2D.h"
#include "Algo/AStar.h"
#if 1
#include "Async/Coroutines.h"
#include "Misc/DebugDraw.h"
#include "RenderUtility.h"
#define DEBUG_BREAK() CO_YEILD(nullptr)
#define DEBUG_POINT(V, C) DrawDebugPoint(V, C, 5);
#define DEBUG_LINE(V1, V2, C) DrawDebugLine(V1, V2, C, 2);
#define DEBUG_TEXT(V, TEXT, C) DrawDebugText(V, TEXT, C);
#else
#define DEBUG_BREAK()
#define DEBUG_POINT(V, C)
#define DEBUG_LINE(V1, V2)
#define DEBUG_TEXT(V, TEXT, C)
#endif



namespace StoreSim
{
	using AABB = Math::TAABBox<Vector2>;

	struct NavArea
	{
		bool bVaild;
		TArray< int > links;
		TArray< Vector2 > polygon;
		Vector2 center;

		bool sortPolygon();
	};


	struct NavLink
	{
		Vector2  pos;
		NavArea* area[2];
		int      polygon[2];
	};
	struct NavMesh
	{
		TArray< NavArea > areas;
		TArray< NavLink > links;

		void clear()
		{
			areas.clear();
			links.clear();
		}
	};


	struct FindState
	{
		NavArea* area;
		int entrance;
		int posIndex;
		Vector2  pos;
	};
	struct AStarNode : AStar::NodeBaseT< AStarNode, FindState, float >
	{
		AStarNode* child = nullptr;
	};
	class PathFinder : public AStar::AStarT< PathFinder, AStarNode >
	{
	public:
		typedef FindState StateType;
		typedef AStarNode NodeType;

		ScoreType calcHeuristic(StateType& state)
		{
			return Math::Distance(state.pos, mGoalPos);
		}
		ScoreType calcDistance(StateType& a, StateType& b)
		{
			return Math::Distance(a.pos, b.pos);
		}

		bool      isEqual(StateType& a, StateType& b)
		{
			return a.area == b.area && a.entrance == b.entrance && a.posIndex == b.posIndex;
		}
		bool      isGoal(StateType& state)
		{
			return state.area == mGoalArea;
		}

		//  call addSreachNode for all possible next state
		void      processNeighborNode(NodeType& aNode)
		{
			auto area = aNode.state.area;
			for (int indexLink : area->links)
			{
				if (indexLink == aNode.state.entrance)
					continue;

				auto const& link = mMesh->links[indexLink];
				CHECK(link.area[0] == area || link.area[1] == area);
				FindState state;
				int index = (link.area[0] == area) ? 1 : 0;
				int indexPolygon = link.polygon[index];
				state.area = link.area[index];
				state.entrance = indexLink;
				state.pos = link.pos;
				state.posIndex = 0;
				addSreachNode(state, aNode);
				state.pos = state.area->polygon[indexPolygon];
				state.posIndex = 1;
				addSreachNode(state, aNode);
				state.pos = state.area->polygon[(indexPolygon + 1) % state.area->polygon.size()];
				state.posIndex = 2;
				addSreachNode(state, aNode);
			}
		}

		struct Portal
		{
			Vector2 from;
			Vector2 to;
			int clipCount = 0;

			Vector2 getPos(float t)
			{
				return Math::LinearLerp(from, to, t);
			}
		};

		struct PortalView
		{
			static int CheckSide(Portal const& portal, Vector2 const& pos, Vector2 const& posTarget, float& outT)
			{
				if (Math::LineSegmentTest(pos, posTarget, portal.from, portal.to, outT))
				{
					return 0;
				}
				else
				{
					if (outT > 1)
						return 1;
					else
						return -1;
				}
			}

			static int Clip(Portal& portal, Vector2 const& pos, Portal const& clipPortal, float outT[2])
			{
				float tFrom;
				int sideFrom = CheckSide(portal, pos, clipPortal.from, tFrom);

				float tTo;
				int sideTo = CheckSide(portal, pos, clipPortal.to, tTo);

				outT[0] = tFrom;
				outT[1] = tTo;
#if 0
				DEBUG_LINE(pos, clipPortal.from, Color3f(1, 1, 0));
				DEBUG_POINT(portal.getPos(tFrom), Color3f(0, 1, 1));
				DEBUG_LINE(pos, clipPortal.to, Color3f(1, 1, 0));
				DEBUG_POINT(portal.getPos(tTo), Color3f(0, 1, 1));
#endif

				if (sideFrom + sideTo) //LL 0X X0 RR
				{
					if (sideFrom == sideTo) //LL RR
					{
						return sideFrom;
					}
					else if (sideFrom == 0) // 0X
					{
						if (sideTo > 1)
							portal.to = portal.getPos(tFrom);
						else
							portal.from = portal.getPos(tFrom);

						portal.clipCount += 1;
						DEBUG_LINE(portal.from, portal.to, RenderUtility::GetColor(portal.clipCount));

					}
					else //X0
					{
						CHECK(sideTo == 0);
						if (sideFrom > 1)
							portal.from = portal.getPos(tTo);
						else
							portal.to = portal.getPos(tTo);

						portal.clipCount += 1;
						DEBUG_LINE(portal.from, portal.to, RenderUtility::GetColor(portal.clipCount));
					}

				}
				else //LR 00
				{
					if (sideFrom == 0) // 00
					{
						CHECK(sideTo == 0);
						Vector2 temp = portal.getPos(tFrom);
						portal.to = portal.getPos(tTo);
						portal.from = temp;

						portal.clipCount += 1;
						DEBUG_LINE(portal.from, portal.to, RenderUtility::GetColor(portal.clipCount));
					}
					else // LR
					{

					}
				}
				return 0;
			}
		};
		bool findPath(int indexFrom, Vector2 const& fromPos, int indexTo, Vector2 const& toPos, bool bUsePortalClip = true);

		void buildPath(NodeType* node)
		{
			if (node->parent)
			{
				buildPath(node->parent);
			}
			mPath.push_back(node->state.pos);
		};

		TArray< Vector2 > mPath;

		NavMesh* mMesh;
		Vector2  mGoalPos;
		NavArea* mGoalArea;
	};

	class NavManager
	{
	public:

		Vector2 mBoundMin = Vector2(0, 0);
		Vector2 mBoundMax = Vector2(20, 20);


		void buildMesh(TArrayView< Bsp2D::PolyArea* > areaList);
		bool findPath(Vector2 const& fromPos, Vector2 const& toPos, TArray<Vector2>& outPath)
		{
			int indexFrom = getAreaIndex(fromPos);
			if (indexFrom == INDEX_NONE)
				return false;
			int indexTo = getAreaIndex(toPos);
			if (indexTo == INDEX_NONE)
				return false;
			return findPath(indexFrom, fromPos, indexTo, toPos, outPath);
		}

		bool bOptimizePath = false;
		bool bUsePortalClip = false;

		bool findPath(int indexFrom, Vector2 const& fromPos, int indexTo, Vector2 const& toPos, TArray<Vector2>& outPath);

		void cleanup()
		{
			mTree.clear();
			mBuilder.cleanup();
		}


		int getAreaIndex(Vector2 const& pos)
		{
			auto node = mTree.getLeaf(pos);
			if (node)
			{
				return ~node->tag;
			}
			return INDEX_NONE;
		}

		NavMesh mNavMesh;
		Bsp2D::Tree mTree;
		Bsp2D::PortalBuilder mBuilder;
		PathFinder mPathFinder;

	};
}
#endif // StoreSimNav_H_2943C778_CFDE_4511_9CA4_0AC3BBC851CE
