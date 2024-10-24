#include "StoreSimNav.h"


namespace StoreSim
{

	bool NavArea::sortPolygon()
	{
		CHECK(!polygon.empty());
		CHECK(polygon.size() % 2 == 0);
		TArray< Vector2 > sorted;
		sorted.push_back(polygon[0]);
		Vector2 curPos = polygon[1];

		int num = polygon.size();
		if (num > 2)
		{
			polygon[1] = polygon[num - 1];
			polygon[0] = polygon[num - 2];
		}
		num -= 2;
		while (num > 0)
		{
			bool bFound = false;
			for (int i = 0; i < num; i += 2)
			{
				if (Bsp2D::IsEqual(curPos, polygon[i]))
				{
					sorted.push_back(curPos);
					curPos = polygon[i + 1];

					if (num > i + 2)
					{
						polygon[i + 1] = polygon[num - 1];
						polygon[i + 0] = polygon[num - 2];
					}
					num -= 2;
					bFound = true;
					break;
				}
			}

			if (!bFound)
			{
				polygon.clear();
				return false;
			}
		}

		polygon = std::move(sorted);
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
		SreachResult sreachResult;
		if (!sreach(state, sreachResult))
		{
			return false;
		}

		sreachResult.globalNode->child = nullptr;
		this->constructPath(sreachResult.globalNode, [](NodeType* node)
		{
			if (node->parent)
			{
				node->parent->child = node;
			}
		});


		mPath.push_back(fromPos);
		DEBUG_POINT(fromPos, Color3f(1, 0, 0));

		if (bUsePortalClip)
		{
			auto GetPortal = [&](NodeType* node) -> Portal
			{
				auto const& link = mMesh->links[node->state.entrance];
				DEBUG_POINT(link.pos, Color3f(0, 0, 1));
				auto area = node->state.area;
				int index = (area == link.area[0]) ? link.polygon[0] : link.polygon[1];
				Portal portal;
				portal.from = area->polygon[index];
				portal.to = area->polygon[(index + 1) % area->polygon.size()];
				return portal;
			};

			auto curNode = sreachResult.startNode;

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
			buildPath(sreachResult.globalNode);
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
			uint32 tag = ~uint32(indexArea);

			for (auto indexEdge : leaf.edges)
			{
				auto const& edge = mTree.mEdges[indexEdge];
				area.polygon.push_back(edge.v[0]);
				area.polygon.push_back(edge.v[1]);
			}
		}

		for (auto* splane : mBuilder.mSPlanes)
		{
			for (auto const& portal : splane->portals)
			{
				if (Bsp2D::Tree::IsLeafTag(portal.frontTag))
				{
					auto& areaFront = mNavMesh.areas[~portal.frontTag];
					areaFront.polygon.push_back(portal.v[0]);
					areaFront.polygon.push_back(portal.v[1]);
				}
				if (Bsp2D::Tree::IsLeafTag(portal.backTag))
				{
					auto& areaBack = mNavMesh.areas[~portal.backTag];
					areaBack.polygon.push_back(portal.v[1]);
					areaBack.polygon.push_back(portal.v[0]);
				}
			}
		}
		for (int indexArea = 0; indexArea < mNavMesh.areas.size(); ++indexArea)
		{
			auto& area = mNavMesh.areas[indexArea];

			if (indexArea == 12 && 0)
			{
				for (int i = 0; i < area.polygon.size(); i += 2)
				{
					Math::Plane2D plane = Math::Plane2D::FromPosition(area.polygon[i], area.polygon[i + 1]);
					Vector2 off = (0.02 * i / 2) * plane.getNormal();
					DrawDebugLine(area.polygon[i ] + off, area.polygon[i +1] + off, Color3f(1, 0, 0), 2);
				}

				int i = 1;
			}

			area.bVaild = true;
			if (area.polygon.size() > 0 && !area.sortPolygon())
			{
				area.bVaild = false;
				LogWarning(0, "Area %d polygon error", indexArea);

			}
			auto& leaf = mTree.mLeaves[indexArea];

			area.center = Math::CalcPolygonCentroid(area.polygon);
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
				link.polygon[0] = areaFront.polygon.findIndexPred([&portal](Vector2 const& pos)
				{
					return Bsp2D::IsEqual(pos, portal.v[0]);
				});
				link.polygon[1] = areaBack.polygon.findIndexPred([&portal](Vector2 const& pos)
				{
					return Bsp2D::IsEqual(pos, portal.v[1]);
				});

				if (link.polygon[0] == INDEX_NONE || link.polygon[1] == INDEX_NONE)
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
			for (int indexStart = 0; indexStart < outPath.size() - 2; ++indexStart)
			{
				int indexTest = indexStart + 2;
				for (; indexTest < outPath.size(); ++indexTest)
				{
					if (mTree.segmentTest(outPath[indexStart], outPath[indexTest], colInfo))
						break;
				}
				if (indexTest != indexStart + 2)
				{
					outPath.erase(outPath.begin() + (indexStart + 1), outPath.begin() + (indexTest - 1));
				}
			}
		}
		return true;
	}

}//namespace StoreSim

