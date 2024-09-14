#include "Voronoi.h"

#include "Math/GeometryPrimitive.h"
#include "TemplateMisc.h"
#include "Math/PrimitiveTest.h"

#define SHOW_DEBUG 0
#define DO_CHECK 0

#if SHOW_DEBUG
#include "Misc/DebugDraw.h"
#include "Async/Coroutines.h"
#endif

namespace Voronoi
{
#define INDEX_FREE 0xfeefeefe

	void DelaunaySolver::solve(TArray< Vector2 > const& posList)
	{
		//Bowyer¡VWatson algorithm
		triangulation.clear();
		edges.clear();

		if (posList.size() < 3)
			return;

		Math::TAABBox<Vector2> bound;
		bound.invalidate();
		for (auto pos : posList)
		{
			bound.addPoint(pos);
		}

		Vector2 boundCenter = bound.getCenter();
		Vector2 boundSize = bound.getSize();
		float len = 100 * Math::Max(boundSize.x, boundSize.y);
		Vector2 superTriVertex[3] =
		{
			boundCenter + Vector2(-len,-len),
			boundCenter + Vector2(0,len),
			boundCenter + Vector2(len,-len),
		};

		auto GetPosition = [&](int index) -> Vector2 const&
		{
			if (index < 0)
			{
				return superTriVertex[(-index) - 1];
			}
			return posList[index];
		};

#if USE_TRIANGLE_CONNECTIVITY
		
		int indexTriFree = INDEX_NONE;
		int indexEdgeFree = INDEX_NONE;

		auto AddEdge = [&](int i0, int i1) -> std::tuple<int , Edge*>
		{
			Edge* edge;
			int edgeId;
			if (indexEdgeFree != INDEX_NONE)
			{
				edgeId = indexEdgeFree;
				edge = &edges[edgeId];
				indexEdgeFree = edge->indices[0];
			}
			else
			{
				edgeId = edges.size();
				edge = edges.addUninitialized();
			}
			edge->indices[0] = i0;
			edge->indices[1] = i1;

			return { edgeId , edge };
		};

		auto DestroyEdge = [&](Edge& edge)
		{
			int edgeID = &edge - edges.data();
			edge.indices[0] = indexEdgeFree;
			edge.indices[1] = INDEX_FREE;
			indexEdgeFree = edgeID;
		};

#endif

		auto IsInsideCircumcircle = [&](Triangle const& tri, Vector2 const& p) -> bool
		{
#if USE_CIRCUCIRCLE_CACHE
			return (p - tri.pos).length2() < tri.radiusSquare;
#else
			return Math::IsInsideCircumcircle(GetPosition(tri.indices[0]), GetPosition(tri.indices[1]), GetPosition(tri.indices[2]), p);
#endif
		};

		auto AddTriangle = [&](int i0, int i1, int i2) -> Triangle*
		{
			Vector2 const& a = GetPosition(i0);
			Vector2 const& b = GetPosition(i1);
			Vector2 const& c = GetPosition(i2);
#if !USE_CIRCUCIRCLE_CACHE
			if ((b - a).cross(c - a) <= 0)
			{
				std::swap(i1, i2);
			}
#endif

			Triangle* tri;
#if USE_TRIANGLE_CONNECTIVITY
			if (indexTriFree != INDEX_NONE)
			{
				tri = &triangulation[indexTriFree];
				indexTriFree = tri->indices[0];
			}
			else
			{
				tri = triangulation.addUninitialized();
			}
			tri->indexCheck = 0;
#else
			tri = triangulation.addUninitialized();
#endif
			tri->indices[0] = i0;
			tri->indices[1] = i1;
			tri->indices[2] = i2;
#if USE_CIRCUCIRCLE_CACHE
			tri->pos = Math::GetCircumcirclePoint(a, b, c);
			tri->radiusSquare = (tri->pos - a).length2();
#endif
			return tri;
		};


		auto DestoryTriangle = [&](int indexTri)
		{
#if USE_TRIANGLE_CONNECTIVITY
			Triangle& tri = triangulation[indexTri];
			tri.indices[0] = indexTriFree;
			tri.indices[1] = INDEX_FREE;
			indexTriFree = indexTri;
#else
			triangulation.removeIndexSwap(indexTri);
#endif
		};

		{
			static int const indices[] = { -1, -2, -3 ,-1 };

			for (int indexEdge = 0; indexEdge < 3; ++indexEdge)
			{
				int i0 = indices[indexEdge];
				int i1 = indices[indexEdge + 1];
#if USE_TRIANGLE_CONNECTIVITY
				auto tri = AddTriangle(i0, i1, 0);
				
				auto[edgeID, edge] = AddEdge(i0, i1);
				tri->edgeIDs[2] = edgeID;
				edge->triIDs[0] = tri - triangulation.data();
				edge->triIDs[1] = INDEX_NONE;
#else
				AddTriangle(i0, i1, 0);
#endif
			}

#if USE_TRIANGLE_CONNECTIVITY
			int indexTriPrev = 2;
			for (int indexTri = 0; indexTri < 3; ++indexTri)
			{
				int i0 = indices[indexTri];
				auto& tri = triangulation[indexTri];
				auto& triPrev = triangulation[indexTriPrev];

				auto [edgeID, edge] = AddEdge(i0, 0);
				tri.edgeIDs[1] = edgeID;
				triPrev.edgeIDs[0] = edgeID;
				edge->triIDs[0] = indexTri;
				edge->triIDs[1] = indexTriPrev;
	
				indexTriPrev = indexTri;
			}
#endif
		}

#if SHOW_DEBUG
		auto DebugDraw = [&]()
		{
			InlineString<> str;
			for (int i = 0; i < posList.size(); ++i)
			{
				str.format("%d", i);
				DrawDebugText(posList[i] + Vector2(3,3), str, Color3f(0.2, 0.8, 0));
			}
#if USE_TRIANGLE_CONNECTIVITY
			for (int i = 0; i < triangulation.size(); ++i)
			{
				Triangle const& tri = triangulation[i];
				if (tri.indices[1] == INDEX_FREE)
					continue;
				Vector2 p0 = GetPosition(tri.indices[0]);
				Vector2 p1 = GetPosition(tri.indices[1]);
				Vector2 p2 = GetPosition(tri.indices[2]);
				DrawDebugLine(p0, p1, Color3f(0, 1, 1), 1);
				DrawDebugLine(p1, p2, Color3f(0, 1, 1), 1);
				DrawDebugLine(p2, p0, Color3f(0, 1, 1), 1);
			}

			for (int i = 0; i < edges.size(); ++i)
			{
				Edge const& edge = edges[i];
				if (edge.indices[1] == INDEX_FREE)
					continue;

				Vector2 p0 = GetPosition(edge.indices[0]);
				Vector2 p1 = GetPosition(edge.indices[1]);
				Vector2 pos;
				if (edge.indices[0] < 0 || edge.indices[1] < 0)
				{
					if (edge.indices[0] < 0)
						pos = Math::LinearLerp(p0, p1, 0.999);
					else
						pos = Math::LinearLerp(p1, p0, 0.999);
				}
				else
				{
					pos = 0.5 * (p0 + p1);
				}

				str.format("%d %d", edge.triIDs[0], edge.triIDs[1]);
				DrawDebugText(pos, str, Color3f(0.2, 0, 0.8));
				DrawDebugLine(p0, p1, Color3f(1, 1, 0), 1);
			}
#endif
		};


		bool bDebugBreak = posList.size() == 4;

		if (bDebugBreak)
		{
			DebugDraw();
			CO_YEILD(nullptr);
		}

#endif

		TArray<int> badTriangleIndices;
		badTriangleIndices.reserve(posList.size() * 5 / 3);
#if USE_TRIANGLE_CONNECTIVITY
		TArray<int> polyTriangleIndices;
		polyTriangleIndices.reserve(posList.size() * 5 / 3);
#endif
		for (int index = 1; index < posList.size(); ++index)
		{
			auto const& pos = posList[index];
			badTriangleIndices.clear();
#if USE_TRIANGLE_CONNECTIVITY
			polyTriangleIndices.clear();
#if 1
			for (int indexTri = 0; indexTri < triangulation.size(); ++indexTri)
			{
				auto& tri = triangulation[indexTri];
				if (tri.indices[1] != INDEX_FREE)
				{
					polyTriangleIndices.push_back(indexTri);
					break;
				}
			}
			CHECK(!polyTriangleIndices.empty());
			do
			{
				int indexTri = polyTriangleIndices.back();
				polyTriangleIndices.pop_back();

				auto& tri = triangulation[indexTri];
				if (Math::Abs(tri.indexCheck) == index)
					continue;

				if (IsInsideCircumcircle(tri, pos))
				{
					badTriangleIndices.push_back(indexTri);
					tri.indexCheck = index;

					polyTriangleIndices.clear();
					for (int i = 0; i < 3; ++i)
					{
						Edge const& edge = edges[tri.edgeIDs[i]];
						int indexTriLink = edge.getOtherTriID(indexTri);
						if (indexTriLink != INDEX_NONE)
						{
							polyTriangleIndices.push_back(indexTriLink);
						}
					}
					break;
				}
				else
				{
					tri.indexCheck = -index;
					for (int i = 0; i < 3; ++i)
					{
						Edge const& edge = edges[tri.edgeIDs[i]];
						int indexTriLink = edge.getOtherTriID(indexTri);
						if (indexTriLink == INDEX_NONE)
							continue;

						if (Math::Abs(triangulation[indexTriLink].indexCheck) == index)
							continue;

						Vector2 const& p0 = GetPosition(edge.indices[0]);
						Vector2 const& p1 = GetPosition(edge.indices[1]);
						Vector2 const& p2 = GetPosition(edge.getOtherIndex(tri));
						Vector2 d1 = p1 - p0;
						Vector2 d2 = p2 - p0;
						Vector2 dir = Math::TripleProduct(d2, d1, d1);
						if (dir.dot(pos - p0) >= 0)
						{
							polyTriangleIndices.push_back(indexTriLink);
						}
					}
				}
			}
			while (!polyTriangleIndices.empty());

			while (!polyTriangleIndices.empty())
			{
				int indexTri = polyTriangleIndices.back();
				polyTriangleIndices.pop_back();

				auto& tri = triangulation[indexTri];
				if (Math::Abs(tri.indexCheck) == index)
					continue;

				if (IsInsideCircumcircle(tri, pos))
				{
					badTriangleIndices.push_back(indexTri);
					tri.indexCheck = index;

					for (int i = 0; i < 3; ++i)
					{
						Edge const& edge = edges[tri.edgeIDs[i]];
						int indexTriLink = edge.getOtherTriID(indexTri);
						if (indexTriLink != INDEX_NONE)
						{
							polyTriangleIndices.push_back(indexTriLink);
						}
					}
				}
				else
				{
					tri.indexCheck = -index;
				}
			}
#else
			for (int indexTri = 0; indexTri < triangulation.size(); ++indexTri)
			{
				auto& tri = triangulation[indexTri];
				if (tri.indices[1] == INDEX_FREE)
					continue;

				if (IsInsideCircumcircle(tri, pos))
				{
					badTriangleIndices.push_back(indexTri);
					tri.indexCheck = index;
				}
			}
#endif
#else
			for (int indexTri = 0; indexTri < triangulation.size(); ++indexTri)
			{
				auto& tri = triangulation[indexTri];
				if (IsInsideCircumcircle(tri, pos))
				{
					badTriangleIndices.push_back(indexTri);
				}
			}
#endif

			for (int i = 0; i < badTriangleIndices.size(); ++i)
			{
				auto indexTri = badTriangleIndices[i];
				for (int indexEdge = 0; indexEdge < 3; ++indexEdge)
				{
					auto& tri = triangulation[indexTri];
#if USE_TRIANGLE_CONNECTIVITY
					auto edgeID = tri.edgeIDs[indexEdge];
					if (edgeID == INDEX_NONE)
						continue;

					Edge& edge = edges[edgeID];
					int indexTriLink = edge.getOtherTriID(indexTri);
					if (indexTriLink != INDEX_NONE)
					{
						Triangle& triLink = triangulation[indexTriLink];

						//Remove shared edge in Polygon
						if (triLink.indexCheck == index)
						{
							tri.replace(edgeID, INDEX_NONE);
							triLink.replace(edgeID, INDEX_NONE);
							DestroyEdge(edge);
							continue;
						}
					}

					auto triPoly = AddTriangle(edge.indices[0], edge.indices[1], index);
					triPoly->edgeIDs[0] = INDEX_NONE;
					triPoly->edgeIDs[1] = INDEX_NONE;
					triPoly->edgeIDs[2] = edgeID;
					int indexTriPoly = triPoly - triangulation.data();
					edge.replace(indexTri, indexTriPoly);
					polyTriangleIndices.push_back(indexTriPoly);				
#else
					int i0 = tri.indices[indexEdge];
					int i1 = tri.indices[(indexEdge + 1) % 3];
					for (int j = 0; j < badTriangleIndices.size(); ++j)
					{
						int indexTriOther = badTriangleIndices[j];
						if (indexTri == indexTriOther)
							continue;
						auto& triOther = triangulation[indexTriOther];
#if 1
						bool bShared =
							(triOther.indices[0] == i0 || triOther.indices[1] == i0 || triOther.indices[2] == i0) &&
							(triOther.indices[0] == i1 || triOther.indices[1] == i1 || triOther.indices[2] == i1);
#else
						bool bShared =
							(triOther.indices[0] == i0 && (triOther.indices[1] == i1 || triOther.indices[2] == i1)) ||
							(triOther.indices[1] == i0 && (triOther.indices[2] == i1 || triOther.indices[0] == i1)) ||
							(triOther.indices[2] == i0 && (triOther.indices[0] == i1 || triOther.indices[1] == i1));
#endif
						if (bShared)
						{
							goto FoundEdge;
						}
					}
					AddTriangle(i0, i1, index);
				FoundEdge:
					;
#endif
				}
			}

#if USE_TRIANGLE_CONNECTIVITY

			// connect polygon vertex to inner vertex
			for (int i = 0; i < polyTriangleIndices.size(); ++i)
			{
				auto indexTri = polyTriangleIndices[i];
				auto& tri = triangulation[indexTri];

				auto AddSharedEdge = [&](int indexEdge)
				{
					for (int j = i + 1; j < polyTriangleIndices.size(); ++j)
					{
						auto indexTriCheck = polyTriangleIndices[j];
						auto& triCheck = triangulation[indexTriCheck];
						int indexEdgeCheck = INDEX_NONE;

						int indexShared = tri.indices[1 - indexEdge];
						if (indexShared == triCheck.indices[0])
							indexEdgeCheck = 1;
						else if (indexShared == triCheck.indices[1])
							indexEdgeCheck = 0;

						if (indexEdgeCheck != INDEX_NONE)
						{
							auto [edgeID, edge] = AddEdge(indexShared, index);
							tri.edgeIDs[indexEdge] = edgeID;
							triCheck.edgeIDs[indexEdgeCheck] = edgeID;
							edge->triIDs[0] = indexTri;
							edge->triIDs[1] = indexTriCheck;
							return;
						}
					}
				};

				if (tri.edgeIDs[0] == INDEX_NONE)
				{	
					AddSharedEdge(0);
				}
				if (tri.edgeIDs[1] == INDEX_NONE)
				{
					AddSharedEdge(1);
				}

				CHECK(tri.edgeIDs[0] != INDEX_NONE && tri.edgeIDs[1] != INDEX_NONE);
			}
#endif

			for (int indexTri : MakeReverseView(badTriangleIndices))
			{
				DestoryTriangle(indexTri);
			}
#if USE_TRIANGLE_CONNECTIVITY
#if DO_CHECK
			for (int i = 0; i < triangulation.size(); ++i)
			{
				Triangle const& tri = triangulation[i];
				if (tri.indices[1] == INDEX_FREE)
					continue;

				CHECK(tri.edgeIDs[0] >= 0 && tri.edgeIDs[1] >= 0 && tri.edgeIDs[2] >= 0);
				CHECK(edges[tri.edgeIDs[0]].contain(i));
				CHECK(edges[tri.edgeIDs[1]].contain(i));
				CHECK(edges[tri.edgeIDs[2]].contain(i));
			}

			for (int i = 0; i < edges.size(); ++i)
			{
				Edge const& edge = edges[i];
				if (edge.indices[1] == INDEX_FREE)
					continue;

				if (edge.triIDs[0] != INDEX_NONE)
				{
					CHECK(triangulation[edge.triIDs[0]].contain(i));
					CHECK(edge.checkVaild(triangulation[edge.triIDs[0]]));
				}
				if (edge.triIDs[1] != INDEX_NONE)
				{
					CHECK(triangulation[edge.triIDs[1]].contain(i));
					CHECK(edge.checkVaild(triangulation[edge.triIDs[1]]));
				}
			}

#endif
#endif

#if SHOW_DEBUG
			if (bDebugBreak)
			{
				DebugDraw();
				CO_YEILD(nullptr);
			}
#endif
		}

		for (int indexTri = 0; indexTri < triangulation.size(); )
		{
			auto const& tri = triangulation[indexTri];
			if (tri.indices[0] < 0 || tri.indices[1] < 0 || tri.indices[2] < 0)
			{
#if USE_TRIANGLE_CONNECTIVITY && USE_EDGE_INFO
				if (tri.indices[1] != INDEX_FREE)
				{
					for (int i = 0; i < 3; ++i)
					{
						Edge& edge = edges[tri.edgeIDs[i]];
						edge.replace(indexTri, INDEX_NONE);
					}
				}

				int indexTriMove = triangulation.size() - 1;
				if (indexTri != indexTriMove)
				{			
					auto& triMove = triangulation[indexTriMove];
					if (triMove.indices[1] != INDEX_FREE)
					{
						for (int i = 0; i < 3; ++i)
						{
							Edge& edge = edges[triMove.edgeIDs[i]];
							edge.replace(indexTriMove, indexTri);
						}
					}
				}
#endif
				triangulation.removeIndexSwap(indexTri);
			}
			else
			{
				++indexTri;
			}
		}
	}


}////namespace Voronoi
