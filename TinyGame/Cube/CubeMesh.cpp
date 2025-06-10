#include "CubePCH.h"
#include "CubeMesh.h"

#include "WindowsHeader.h"
#include <gl/GL.h>
#include <gl/GLU.h>
#include "ProfileSystem.h"

namespace Cube
{

	Mesh::Mesh()
	{
		mVertices.reserve( 4 * 1000 );
		mIndices.reserve( 3 * 2 * 1000 );
		mIndexBase = 0;
		mVertexOffset.setValue( 0 , 0 , 0 );
		bound.invalidate();
	}

	void Mesh::addVertex( Vec3f const& pos )
	{
		mCurVertex.pos = mVertexOffset + pos;
		mVertices.push_back(mCurVertex);
		bound += mCurVertex.pos;
	}

	void Mesh::addVertex( float x , float y , float z )
	{
		addVertex(Vec3f(x,y,z));
	}

	void Mesh::setIndexBase( int index )
	{
		mIndexBase = index;
	}

	void Mesh::addTriangle( int o1 , int o2 , int o3 )
	{
		mIndices.push_back( mIndexBase + o1 );
		mIndices.push_back( mIndexBase + o2 );
		mIndices.push_back( mIndexBase + o3 );
	}

	void Mesh::addQuad( int o1 , int o2 , int o3 , int o4 )
	{
		addTriangle( o1 , o2 , o3 );
		addTriangle( o1 , o3 , o4 );
	}

	void Mesh::setColor( uint8 r , uint8 g , uint8 b )
	{
		mCurVertex.color[0] = r;
		mCurVertex.color[1] = g;
		mCurVertex.color[2] = b;
		mCurVertex.color[3] = 255;
	}

	using Math::Vector3;
	Vector3 GetSafeNormal(Vector3 const& v)
	{
		Vector3 result = v;
		if ( result.normalize() == 0.0 )
			return Vector3::Zero();

		return result;
	}

	bool IsNearlyZero(Vector3 const& v)
	{
		float const ZeroError = 1e-4;
		return Math::Abs(v.x) < ZeroError && Math::Abs(v.y) < ZeroError && Math::Abs(v.z) < ZeroError;
	}


	class TriangulateAlgorithm
	{
	public:

		struct Edge
		{
			bool bNeedCheck;
			uint32 verticeId;
			Edge* prev;
			Edge* next;
		};

		struct PartitionVertex
		{
			Vector3 p;
			int32   idx;
			bool    isEar;
			float   angle;

			PartitionVertex* prev;
			PartitionVertex* next;
		};

		Vector3 mNoraml;
		TArray< PartitionVertex > mVertices;

		bool IsConvex(Vector3 const& p1, Vector3 const& p2, Vector3 const& p3)
		{
			Vector3 n = Math::Cross(p2 - p1, p3 - p1);
			return Math::Dot(mNoraml, n) > 0;
		}

		bool IsInside(Vector3 const& p1, Vector3 const& p2, Vector3 const& p3, Vector3 const& p)
		{
#if 0
			float coords[3];
			if ( !Math::Barycentric(p, p1, p2, p3, coords) )
				return false;
#else

			if (IsConvex(p1, p, p2)) return false;
			if (IsConvex(p2, p, p3)) return false;
			if (IsConvex(p3, p, p1)) return false;
#endif
			return true;
		}

		void updateVertex(PartitionVertex& v)
		{
			PartitionVertex *v1 = v.prev;
			PartitionVertex *v3 = v.next;

			bool isConvex = IsConvex(v1->p, v.p, v3->p);

			Vector3 vec1 = GetSafeNormal(v1->p - v.p);
			Vector3 vec3 = GetSafeNormal(v3->p - v.p);
			v.angle = vec1.dot(vec3);

			if (isConvex)
			{
				v.isEar = true;
				for (int i = 0; i < mVertices.size(); i++)
				{
					if (mVertices[i].prev == nullptr)
						continue;
#if 0
					if (IsNearlyZero(mVertices[i].p - v.p) ||
						IsNearlyZero(mVertices[i].p - v1->p) ||
						IsNearlyZero(mVertices[i].p - v3->p))
						continue;
#else
					if (i == &v - mVertices.data())
						continue;
					if (i == v1 - mVertices.data())
						continue;
					if (i == v3 - mVertices.data())
						continue;
#endif

					if (IsInside(v1->p, v.p, v3->p, mVertices[i].p))
					{
						v.isEar = false;
						break;
					}
				}
			}
			else
			{
				v.isEar = false;
			}
		}


		bool run(int numVertices, uint8* verticesData, int dataStride, TArray< uint32 >& triList, int indexOffset, int* pIndices)
		{
			if (numVertices < 3)
				return false;

			if (numVertices == 3)
			{
				if (pIndices)
				{
					triList.push_back(indexOffset + pIndices[0]);
					triList.push_back(indexOffset + pIndices[1]);
					triList.push_back(indexOffset + pIndices[2]);
				}
				else
				{
					triList.push_back(indexOffset + 0);
					triList.push_back(indexOffset + 1);
					triList.push_back(indexOffset + 2);
				}
				return true;
			}


			mVertices.resize(numVertices);
			PartitionVertex* prev = &mVertices[numVertices - 1];
			for (int i = 0; i < numVertices; i++)
			{
				int idx = indexOffset;
				if (pIndices)
					idx += pIndices[i];
				else
					idx += i;

				PartitionVertex& v = mVertices[i];
				v.idx = idx;
				v.p = *reinterpret_cast<Vector3*>(verticesData + idx * dataStride);
				v.prev = prev;
				v.next = mVertices.data() + (i + 1);
				prev = &v;
			}
			prev->next = mVertices.data();
			

			for (int i = 0; i < numVertices; i++)
			{
				updateVertex(mVertices[i]);
			}


			for (int i = 0; i < numVertices - 3; i++)
			{
				PartitionVertex* ear = nullptr;
				//find the most extruded ear
				for (int j = 0; j < numVertices; j++)
				{
					if (mVertices[j].prev == nullptr)
						continue;
					if (!mVertices[j].isEar)
						continue;

					if (ear == nullptr)
					{
						ear = &(mVertices[j]);
					}
					else
					{
						if (mVertices[j].angle > ear->angle)
						{
							ear = &(mVertices[j]);
						}
					}
				}
				if (ear == nullptr)
				{
					return false;
				}

				triList.push_back(ear->prev->idx);
				triList.push_back(ear->idx);
				triList.push_back(ear->next->idx);

				ear->prev->next = ear->next;
				ear->next->prev = ear->prev;
				ear->prev = nullptr;
				if (i == numVertices - 4)
					break;

				updateVertex(*ear->prev);
				updateVertex(*ear->next);
			}

			for (int i = 0; i < numVertices; i++)
			{
				if (mVertices[i].prev)
				{
					triList.push_back(mVertices[i].prev->idx);
					triList.push_back(mVertices[i].idx);
					triList.push_back(mVertices[i].next->idx);
					break;
				}
			}

			return true;
		}

	};


	extern double GTriTime;
	uint64 MakeKey(uint32 a, uint32 b)
	{
		return (uint64(a) << 32) | uint64(b);
	}

	void Mesh::merge()
	{
		struct Edge
		{
			bool bNeedCheck;
			uint32 verticeId;
			Edge* prev;
			Edge* next;
		};

		std::vector< Edge > edges;
		std::vector<uint32> vertexIdRemap;
		vertexIdRemap.resize( mVertices.size() );
		std::unordered_map< Vertex, uint32, MemberFuncHasher > vertexIdMap;
		for (uint32 i = 0; i < mVertices.size(); ++i)
		{
			auto const& v = mVertices[i];
			auto iter = vertexIdMap.find(v);
			if (iter != vertexIdMap.end())
			{
				vertexIdRemap[i] = iter->second;
			}
			else
			{
				vertexIdRemap[i] = i;
				vertexIdMap[v] = i;
			}
		}

		auto RemapVertexId = [&](uint32 vertexId)
		{
			return vertexIdRemap[vertexId];
		};

		auto CheckVaild = [&]( int indexEdge)
		{
			auto& edge = edges[indexEdge];
			CHECK(edge.prev != nullptr);
		};

		edges.resize(mIndices.size());
		for (int i = 0; i < mIndices.size(); i += 3)
		{
			uint32 i0 = RemapVertexId(mIndices[i + 0]);
			uint32 i1 = RemapVertexId(mIndices[i + 1]);
			uint32 i2 = RemapVertexId(mIndices[i + 2]);

			Edge& e0 = edges[i];
			Edge& e1 = edges[i + 1];
			Edge& e2 = edges[i + 2];
			e0.bNeedCheck = false;
			e0.verticeId = i0;
			e0.prev = &e2;
			e0.next = &e1;

			e1.bNeedCheck = false;
			e1.verticeId = i1;
			e1.prev = &e0;
			e1.next = &e2;

			e2.bNeedCheck = false;
			e2.verticeId = i2;
			e2.prev = &e1;
			e2.next = &e0;
		}

		auto GetPosition = [&](uint32 vertexId)
		{
			return mVertices[vertexId].pos;
		};

		auto ConnectLink = [&](Edge& e1, Edge& e2)
		{
			e1.next = &e2;
			e2.prev = &e1;
		};

		std::unordered_map< uint64, Edge* > edgePairMap;
		edgePairMap.reserve(edges.size());

		bool bHadMergeEdge = false;
		auto TryMergeEdge = [&](Edge& edge)
		{
			Edge& edgeNext = *edge.next;

			Vector3 d1 = GetPosition(edge.next->verticeId) - GetPosition(edge.verticeId);
			Vector3 d2 = GetPosition(edgeNext.next->verticeId) - GetPosition(edgeNext.verticeId);

			float l1 = d1.normalize();
			float l2 = d2.normalize();

			float dot = d1.dot(d2);

			if (1 - Math::Abs(dot) > 1e-4 )
				return false;

			auto iter = edgePairMap.find(MakeKey(edge.next->verticeId, edge.verticeId));
			if (iter != edgePairMap.end())
			{
				edgePairMap.erase(iter);
			}

			edge.bNeedCheck = true;
			ConnectLink(edge, *edgeNext.next);

#if 0
			CheckVaild(edge);
			CheckVaild(*edgeNext->next);
#endif

			edgeNext.prev = nullptr;

			bHadMergeEdge = true;
			return true;
		};


		auto TryRemoveEdge = [&](auto& ei, auto& ej) -> bool
		{
			CHECK(&ei != &ej);
			if (ej.prev == nullptr)
				return false;

			ConnectLink(*ei.prev, *ej.next);
			ConnectLink(*ej.prev, *ei.next);

#if 0
			CheckVaild(*ei.prev);
			CheckVaild(*ei.next);
			CheckVaild(*ej.prev);
			CheckVaild(*ej.next);
#endif

			TryMergeEdge(*ei.prev);
			TryMergeEdge(*ej.prev);

			ei.prev = nullptr;
			ej.prev = nullptr;

			return true;
		};


		bool bHadRemoveEdge = false;
		for (int i = 0; i < edges.size(); ++i)
		{
			auto& edge = edges[i];
			if (edge.prev == nullptr)
				continue;
			
			auto iter = edgePairMap.find(MakeKey(edge.verticeId, edge.next->verticeId));
			if (iter == edgePairMap.end() || !TryRemoveEdge(edge , *iter->second))
			{
				edgePairMap[MakeKey(edge.next->verticeId, edge.verticeId)] = &edge;
			}
			else
			{
				bHadRemoveEdge = true;
			}
		}

		if (!bHadRemoveEdge)
			return;


		while (bHadMergeEdge)
		{
			bHadMergeEdge = false;

			for (int i = 0; i < edges.size(); ++i)
			{
				auto& edge = edges[i];
				if (edge.bNeedCheck == false)
					continue;
				edge.bNeedCheck = false;

				if (edge.prev == nullptr)
					continue;

				auto iter = edgePairMap.find(MakeKey(edge.verticeId, edge.next->verticeId));
				if (iter == edgePairMap.end() || !TryRemoveEdge(edge, *iter->second))
				{
					edgePairMap[MakeKey(edge.next->verticeId, edge.verticeId)] = &edge;
				}
			}
		}

#if 0
		std::unordered_multimap<uint32, int> vertexEdgeMap;

		for (int i = 0; i < edges.size(); ++i)
		{
			if (edges[i].prev == nullptr)
				continue;

			vertexEdgeMap.emplace(edges[i].verticeIds[0], i);
			vertexEdgeMap.emplace(edges[i].verticeIds[1], i);
		}

		for (int i = 0; i < edges.size(); ++i)
		{
			if (edges[i].prev == nullptr)
				continue;

			vertexEdgeMap.find(edges[i].verticeIds[0]);





		}
#endif	


		TArray< Vertex > verticesNew;
		TArray< uint32 > indicesNew;
		for (int i = 0; i < edges.size(); ++i)
		{
			auto& edge = edges[i];
			if (edge.prev == nullptr)
				continue;

			int count = 0;
			int indexOffset = verticesNew.size();
			int indexEdge = i;

			TriangulateAlgorithm algo;
			algo.mNoraml = mVertices[edge.verticeId].normal;
			Edge* curEdge = &edge;
			do 
			{
				verticesNew.push_back(mVertices[curEdge->verticeId]);
				++count;
				curEdge->prev = nullptr;
				curEdge = curEdge->next;
			}
			while(curEdge != &edge);

			{
				TIME_SCOPE(GTriTime);
				algo.run(count, (uint8*)(verticesNew.data()), sizeof(Vertex), indicesNew, indexOffset, nullptr);
			}
		}

		mVertices = std::move(verticesNew);
		mIndices = std::move(indicesNew);
	}

}//namespace Cube
