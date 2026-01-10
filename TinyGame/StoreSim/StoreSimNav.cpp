#include "StoreSimNav.h"

#if 1
#include "Async/Coroutines.h"
#include "Misc/DebugDraw.h"
#include "RenderUtility.h"
#define DEBUG_YEILD() CO_YEILD(nullptr)
#define DEBUG_POINT(V, C) DrawDebugPoint(V, C, 5);
#define DEBUG_LINE(V1, V2, C) DrawDebugLine(V1, V2, C, 2);
#define DEBUG_TEXT(V, TEXT, C) DrawDebugText(V, TEXT, C);
#else
#define DEBUG_YEILD()
#define DEBUG_POINT(V, C)
#define DEBUG_LINE(V1, V2)
#define DEBUG_TEXT(V, TEXT, C)
#endif

namespace StoreSim
{
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


	bool NavArea::sortVertices()
	{
		CHECK(!vertices.empty());
		CHECK(vertices.size() % 2 == 0);
		TArray< Vector2 > sorted;
		sorted.push_back(vertices[0]);
		Vector2 curPos = vertices[1];

		int num = vertices.size();
		if (num > 2)
		{
			vertices[1] = vertices[num - 1];
			vertices[0] = vertices[num - 2];
		}
		num -= 2;
		while (num > 0)
		{
			bool bFound = false;
			for (int i = 0; i < num; i += 2)
			{
				if (Bsp2D::IsEqual(curPos, vertices[i]))
				{
					sorted.push_back(curPos);
					curPos = vertices[i + 1];

					if (num > i + 2)
					{
						vertices[i + 1] = vertices[num - 1];
						vertices[i + 0] = vertices[num - 2];
					}
					num -= 2;
					bFound = true;
					break;
				}
			}

			if (!bFound)
			{
				vertices.clear();
				return false;
			}
		}

		vertices = std::move(sorted);
		return true;
	}

	bool PathFinder::findPath(int indexFrom, Vector2 const& fromPos, int indexTo, Vector2 const& toPos, bool bUsePortalClip /*= true*/)
	{
		FindState state;
		state.area = &mMesh->areas[indexFrom];
		state.entrance = INDEX_NONE;
		state.posIndex = INDEX_NONE;
		state.pos = fromPos;

		mGoalPos = toPos;
		mGoalArea = &mMesh->areas[indexTo];

		mPath.clear();
		SearchResult searchResult;
		if (!search(state, searchResult))
		{
			return false;
		}

		searchResult.globalNode->child = nullptr;
		this->constructPath(searchResult.globalNode, [](NodeType* node)
		{
			if (node->parent)
			{
				node->parent->child = node;
			}
		});

		DEBUG_POINT(fromPos, Color3f(1, 0, 0));

		if (bUsePortalClip)
		{
			mPath.push_back(fromPos);
			auto GetPortal = [&](NodeType* node) -> Portal
			{
				auto const& link = mMesh->links[node->state.entrance];
				DEBUG_POINT(link.pos, Color3f(0, 0, 1));
				auto area = node->state.area;
				int index = (area == link.area[0]) ? link.vertex[0] : link.vertex[1];
				Portal portal;
				portal.from = area->vertices[index];
				portal.to = area->vertices[(index + 1) % area->vertices.size()];
				return portal;
			};

			auto curNode = searchResult.startNode;

			curNode = curNode->child;
			Vector2 viewPos = fromPos;
			TArray< Portal > portalQueue;

			while (curNode)
			{
				if (!(curNode->state.entrance != INDEX_NONE))
					break;

				Portal portal = GetPortal(curNode);
				DEBUG_LINE(portal.from, portal.to, Color3f(1, 1, 0));

				portalQueue.push_back(portal);

				curNode = curNode->child;
			}

			Portal portal;
			int indexQueue = 0;
			int indexEnd = 0;
			while (indexQueue < portalQueue.size())
			{
				portal = portalQueue[indexQueue];

				int indexClip = indexQueue + 1;
				for (; indexClip < portalQueue.size(); ++indexClip)
				{
					Portal const& portalClip = portalQueue[indexClip];
					float t[2];
					int side = PortalView::Clip(portal, viewPos, portalClip, t);

					if (side != 0)
					{
						viewPos = (side > 0) ? portal.to : portal.from;
						mPath.push_back(viewPos);
						break;
					}
				}

				if (indexClip == portalQueue.size())
					break;

				++indexQueue;
			}

			while (indexQueue < portalQueue.size())
			{
				portal = portalQueue[indexQueue];
				int indexClip = indexQueue + 1;
				for (; indexClip < portalQueue.size(); ++indexClip)
				{
					Portal const& portalClip = portalQueue[indexClip];
					float t[2];
					int side = PortalView::Clip(portal, viewPos, portalClip, t);
					if (side != 0)
					{
						viewPos = (side > 0) ? portal.to : portal.from;
						mPath.push_back(viewPos);
						break;
					}
				}
				if (indexClip != portalQueue.size())
				{
					++indexQueue;
					continue;
				}

				float t;
				int side = PortalView::CheckSide(portal, viewPos, toPos, t);
				if (side == 0)
					break;

				if (indexQueue + 1 != portalQueue.size())
				{
					portal = portalQueue[indexQueue];
					float t[2];
					PortalView::Clip(portal, viewPos, portalQueue[portalQueue.size() - 1], t);
				}

				viewPos = (side > 0) ? portal.to : portal.from;
				mPath.push_back(viewPos);
				++indexQueue;
			}

			mPath.push_back(viewPos);
			++indexQueue;
		}
		else
		{
			buildPath(searchResult.globalNode);
		}

		mPath.push_back(toPos);
		DEBUG_POINT(toPos, Color3f(0, 1, 0));
		return true;
	}

	float CrossProduct(const Vector2& p1, const Vector2& p2,
		const Vector2& p3)
	{
		return (p2.x - p1.x) * (p3.y - p1.y)
			- (p2.y - p1.y) * (p3.x - p1.x);
	}
	bool IsInsidePolygon2(Vector2 const& p, Vector2 const v[], int nV)
	{
		int windingNumber = 0;

		// Iterate through each edge of the polygon
		for (int i = 0; i < nV; i++) 
		{
			Vector2 p1 = v[i];
			// Next vertex in the polygon
			Vector2 p2 = v[(i + 1) % nV];

			// Calculate the cross product to determine winding
			// direction
			if (p1.y <= p.y) 
			{
				if (p2.y > p.y && CrossProduct(p1, p2, p) > 0)
				{
					windingNumber++;
				}
			}
			else 
			{
				if (p2.y <= p.y && CrossProduct(p1, p2, p) < 0)
				{
					windingNumber--;
				}
			}
		}
		// Return the winding number
		return windingNumber != 0;

		for (int32 i = 0; i < nV; ++i)
		{
			float dot = Math::Dot(v[i] - v[0], p - v[i]);
			if (dot < 0.0f)
			{
				return true;
			}
		}
		return false;
	}

	void NavManager::buildMesh(TArrayView< Bsp2D::PolyArea* > areaList)
	{
		mTree.build(areaList.data(), areaList.size(), mBoundMin, mBoundMax);

		for (auto& leaf : mTree.mLeaves)
		{
			AABB bound; bound.invalidate();

			for (auto indexEdge : leaf.edges)
			{
				auto const& edge = mTree.mEdges[indexEdge];
				bound.addPoint(edge.v[0]);
				bound.addPoint(edge.v[1]);
			}

			leaf.debugPos = bound.getCenter();
		}

		mBuilder.mTree = &mTree;
		mPathFinder.mMesh = &mNavMesh;

		mBuilder.build(mBoundMin, mBoundMax);
		mNavMesh.clear();
		mNavMesh.areas.resize(mTree.mLeaves.size());

		for (int indexArea = 0; indexArea < mNavMesh.areas.size(); ++indexArea)
		{
			auto& area = mNavMesh.areas[indexArea];
			auto& leaf = mTree.mLeaves[indexArea];
			
			area.bVaild = false;
			uint32 tag = ~uint32(indexArea);

			for (auto indexEdge : leaf.edges)
			{
				auto const& edge = mTree.mEdges[indexEdge];
				area.vertices.push_back(edge.v[0]);
				area.vertices.push_back(edge.v[1]);
			}
		}

		for (auto* splane : mBuilder.mSPlanes)
		{
			for (auto const& portal : splane->portals)
			{
				if (Bsp2D::Tree::IsLeafTag(portal.frontTag))
				{
					auto& areaFront = mNavMesh.areas[~portal.frontTag];
					areaFront.bVaild = true;
					areaFront.vertices.push_back(portal.v[0]);
					areaFront.vertices.push_back(portal.v[1]);
				}
				if (Bsp2D::Tree::IsLeafTag(portal.backTag))
				{
					auto& areaBack = mNavMesh.areas[~portal.backTag];
					areaBack.bVaild = true;
					areaBack.vertices.push_back(portal.v[1]);
					areaBack.vertices.push_back(portal.v[0]);
				}
			}
		}
		for (int indexArea = 0; indexArea < mNavMesh.areas.size(); ++indexArea)
		{
			auto& area = mNavMesh.areas[indexArea];

			if ( !area.bVaild )
				continue;

#if 0
			if (indexArea == 12)
			{
				for (int i = 0; i < area.vertices.size(); i += 2)
				{
					Math::Plane2D plane = Math::Plane2D::FromPosition(area.vertices[i], area.vertices[i + 1]);
					Vector2 off = (0.02 * i / 2) * plane.getNormal();
					DrawDebugLine(area.vertices[i ] + off, area.vertices[i +1] + off, Color3f(1, 0, 0), 2);
				}
			}
#endif
			if (area.vertices.size() > 0 && !area.sortVertices())
			{
				area.bVaild = false;
				LogWarning(0, "Area %d vertices error", indexArea);

			}
			auto& leaf = mTree.mLeaves[indexArea];

			area.center = Math::CalcPolygonCentroid(area.vertices);
			leaf.debugPos = area.center;

			for (int indexCheck = 0; indexCheck < areaList.size(); ++indexCheck)
			{
				auto const& areaCheck = *areaList[indexCheck];
				if (IsInsidePolygon2(area.center, areaCheck.mVertices.data(), areaCheck.mVertices.size()))
				{
					area.bVaild = false;
					break;
				}
			}

		}

		for (auto* splane : mBuilder.mSPlanes)
		{
			for (auto const& portal : splane->portals)
			{
				if (Bsp2D::Tree::IsLeafTag(portal.frontTag) == false || Bsp2D::Tree::IsLeafTag(portal.backTag) == false)
					continue;

				auto& areaFront = mNavMesh.areas[~portal.frontTag];
				auto& areaBack = mNavMesh.areas[~portal.backTag];

				if ( !areaFront.bVaild || !areaBack.bVaild )
					continue;

				CHECK(areaFront.bVaild && areaBack.bVaild);

				NavLink link;
				link.pos = 0.5 * (portal.v[0] + portal.v[1]);
				link.area[0] = &areaFront;
				link.area[1] = &areaBack;
				link.vertex[0] = areaFront.vertices.findIndexPred([&portal](Vector2 const& pos)
				{
					return Bsp2D::IsEqual(pos, portal.v[0]);
				});
				link.vertex[1] = areaBack.vertices.findIndexPred([&portal](Vector2 const& pos)
				{
					return Bsp2D::IsEqual(pos, portal.v[1]);
				});

				if (link.vertex[0] == INDEX_NONE || link.vertex[1] == INDEX_NONE)
				{
					continue;
				}
				int indexLink = mNavMesh.links.size();
				mNavMesh.links.push_back(link);
				areaFront.links.push_back(indexLink);
				areaBack.links.push_back(indexLink);
			}
		}
	}

	bool NavManager::findPath(int indexFrom, Vector2 const& fromPos, int indexTo, Vector2 const& toPos, TArray<Vector2>& outPath)
	{
		Bsp2D::Tree::ColInfo colInfo;
		if (!mTree.segmentTest(fromPos, toPos, colInfo))
		{
			outPath.clear();
			outPath.push_back(fromPos);
			outPath.push_back(toPos);
			return true;
		}

		if (!mPathFinder.findPath(indexFrom, fromPos, indexTo, toPos, bUsePortalClip))
			return false;

		outPath = std::move(mPathFinder.mPath);

		if (bOptimizePath)
		{
			int checkCount = 0;
#if 0
			int length = (int)outPath.size() - 2;
			for (int indexCur = 0; indexCur < length; ++indexCur)
			{
				int offset = outPath.size() - indexCur - 1;
				while ( offset > 1 )
				{
					int indexTest = indexCur + offset;
					++checkCount;
					if (mTree.segmentTest(outPath[indexCur], outPath[indexTest], colInfo))
					{
						offset = (offset + 1)/2;
					}
					else
					{
						outPath.erase(outPath.begin() + (indexCur + 1), outPath.begin() + indexTest);
						int numRemoved = offset - 1;
						length -= numRemoved;
						CHECK(length == (int)outPath.size() - 2);
						break;
					}
				}
			}
#else
			for (int indexStart = 0; indexStart < outPath.size() - 2; ++indexStart)
			{
				int indexTest = indexStart + 2;
				for (; indexTest < outPath.size(); ++indexTest)
				{
					++checkCount;
					if (mTree.segmentTest(outPath[indexStart], outPath[indexTest], colInfo))
						break;
				}
				if (indexTest != indexStart + 2)
				{
					outPath.erase(outPath.begin() + (indexStart + 1), outPath.begin() + (indexTest - 1));
				}
			}
#endif
			LogMsg("checkCount = %d", checkCount);
		}
		return true;
	}

	bool NavManager::findPath(Vector2 const& fromPos, Vector2 const& toPos, TArray<Vector2>& outPath)
	{
		int indexFrom = getAreaIndex(fromPos);
		if (indexFrom == INDEX_NONE)
			return false;
		int indexTo = getAreaIndex(toPos);
		if (indexTo == INDEX_NONE)
			return false;
		return findPath(indexFrom, fromPos, indexTo, toPos, outPath);
	}

	int NavManager::getAreaIndex(Vector2 const& pos)
	{
		auto node = mTree.getLeaf(pos);
		if (node)
		{
			return ~node->tag;
		}
		return INDEX_NONE;
	}

}//namespace StoreSim

