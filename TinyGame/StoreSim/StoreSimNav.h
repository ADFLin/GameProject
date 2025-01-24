#pragma once
#ifndef StoreSimNav_H_2943C778_CFDE_4511_9CA4_0AC3BBC851CE
#define StoreSimNav_H_2943C778_CFDE_4511_9CA4_0AC3BBC851CE

#include "StoreSimCore.h"

#include "Math/GeometryPrimitive.h"
#include "Collision2D/SATSolver.h"
#include "Collision2D/Bsp2D.h"
#include "Algo/AStar.h"


namespace StoreSim
{
	using AABB = Math::TAABBox<Vector2>;

	struct NavArea
	{
		bool bVaild;
		TArray< int > links;
		TArray< Vector2 > vertices;
		Vector2 center;

		bool sortVertices();
	};


	struct NavLink
	{
		Vector2  pos;
		NavArea* area[2];
		int      vertex[2];
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
				int indexPolygon = link.vertex[index];
				state.area = link.area[index];
				state.entrance = indexLink;
				state.pos = link.pos;
				state.posIndex = 0;
				addSreachNode(state, aNode);
				state.pos = state.area->vertices[indexPolygon];
				state.posIndex = 1;
				addSreachNode(state, aNode);
				state.pos = state.area->vertices[(indexPolygon + 1) % state.area->vertices.size()];
				state.posIndex = 2;
				addSreachNode(state, aNode);
			}
		}


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
		bool findPath(Vector2 const& fromPos, Vector2 const& toPos, TArray<Vector2>& outPath);

		bool bOptimizePath = false;
		bool bUsePortalClip = false;

		bool findPath(int indexFrom, Vector2 const& fromPos, int indexTo, Vector2 const& toPos, TArray<Vector2>& outPath);

		void cleanup()
		{
			mTree.clear();
			mBuilder.cleanup();
		}


		int getAreaIndex(Vector2 const& pos);

		NavMesh mNavMesh;
		Bsp2D::Tree mTree;
		Bsp2D::PortalBuilder mBuilder;
		PathFinder mPathFinder;

	};
}
#endif // StoreSimNav_H_2943C778_CFDE_4511_9CA4_0AC3BBC851CE
