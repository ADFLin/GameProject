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
		struct PartitionVertex
		{
			Vector3 p;
			int32   idx;
			bool    isEar;
			bool    isActive;
			bool    isConvex;
			float   angle;

			PartitionVertex* prev;
			PartitionVertex* next;
		};


		float   mNormalFactor;
		Vector3 mNoraml;

		bool IsConvex(Vector3 const& p1, Vector3 const& p2, Vector3 const& p3)
		{
			Vector3 n = Math::Cross(p2 - p1, p3 - p1);
			return Math::Dot(mNoraml, n) > 0;
		}

		bool IsInside(Vector3 const& p1, Vector3 const& p2, Vector3 const& p3, Vector3 const& p)
		{
			if (IsConvex(p1, p, p2)) return false;
			if (IsConvex(p2, p, p3)) return false;
			if (IsConvex(p3, p, p1)) return false;
			return true;
		}

		void UpdateVertex(PartitionVertex& v, PartitionVertex vertices[], int32 numvertices)
		{
			PartitionVertex *v1 = v.prev;
			PartitionVertex *v3 = v.next;

			v.isConvex = IsConvex(v1->p, v.p, v3->p);

			Vector3 vec1 = GetSafeNormal(v1->p - v.p);
			Vector3 vec3 = GetSafeNormal(v3->p - v.p);
			v.angle = vec1.dot(vec3);

			if (v.isConvex)
			{
				v.isEar = true;
				for (int i = 0; i < numvertices; i++)
				{
					if (IsNearlyZero(vertices[i].p - v.p) ||
						IsNearlyZero(vertices[i].p - v1->p) ||
						IsNearlyZero(vertices[i].p - v3->p))
						continue;

					if (IsInside(v1->p, v.p, v3->p, vertices[i].p))
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

			TArray< PartitionVertex > vertices;
			vertices.resize(numVertices);

			for (int i = 0; i < numVertices; i++)
			{
				int idx = indexOffset;
				if (pIndices)
					idx += pIndices[i];
				else
					idx += i;

				PartitionVertex& v = vertices[i];
				v.idx = idx;
				v.isActive = true;
				v.p = *reinterpret_cast<Vector3*>(verticesData + idx * dataStride);
				if (i == (numVertices - 1))
					v.next = &(vertices[0]);
				else
					v.next = &(vertices[i + 1]);
				if (i == 0)
					v.prev = &(vertices[numVertices - 1]);
				else
					v.prev = &(vertices[i - 1]);
			}

			for (int i = 0; i < numVertices; i++)
			{
				UpdateVertex(vertices[i], &vertices[0], numVertices);
			}


			for (int i = 0; i < numVertices - 3; i++)
			{
				PartitionVertex* ear = nullptr;
				//find the most extruded ear
				for (int j = 0; j < numVertices; j++)
				{
					if (!vertices[j].isActive)
						continue;
					if (!vertices[j].isEar)
						continue;

					if (ear == nullptr)
					{
						ear = &(vertices[j]);
					}
					else
					{
						if (vertices[j].angle > ear->angle)
						{
							ear = &(vertices[j]);
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

				ear->isActive = false;
				ear->prev->next = ear->next;
				ear->next->prev = ear->prev;

				if (i == numVertices - 4)
					break;

				UpdateVertex(*ear->prev, &vertices[0], numVertices);
				UpdateVertex(*ear->next, &vertices[0], numVertices);
			}

			for (int i = 0; i < numVertices; i++)
			{
				if (vertices[i].isActive)
				{
					triList.push_back(vertices[i].prev->idx);
					triList.push_back(vertices[i].idx);
					triList.push_back(vertices[i].next->idx);
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
			int links[2];
			uint32 verticeIds[2];
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
			CHECK(edge.links[0] != INDEX_NONE);

			auto& edgeNext = edges[edge.links[1]];
			CHECK(edge.verticeIds[1] == edgeNext.verticeIds[0]);
			auto& edgePrev = edges[edge.links[0]];
			CHECK(edge.verticeIds[0] == edgePrev.verticeIds[1]);
		};

		edges.resize(mIndices.size());
		for (int i = 0; i < mIndices.size(); i += 3)
		{
			uint32 i0 = RemapVertexId(mIndices[i + 0]);
			uint32 i1 = RemapVertexId(mIndices[i + 1]);
			uint32 i2 = RemapVertexId(mIndices[i + 2]);

			Edge& e0 = edges[i];
			e0.bNeedCheck = false;
			e0.links[0] = i + 2;
			e0.links[1] = i + 1;
			e0.verticeIds[0] = i0;
			e0.verticeIds[1] = i1;

			Edge& e1 = edges[i + 1];
			e1.bNeedCheck = false;
			e1.links[0] = i;
			e1.links[1] = i + 2;
			e1.verticeIds[0] = i1;
			e1.verticeIds[1] = i2;

			Edge& e2 = edges[i + 2];
			e2.bNeedCheck = false;
			e2.links[0] = i + 1;
			e2.links[1] = i;
			e2.verticeIds[0] = i2;
			e2.verticeIds[1] = i0;
		}

		auto GetPosition = [&](uint32 vertexId)
		{
			return mVertices[vertexId].pos;
		};

		auto ConnectLink = [&](int index1, int index2)
		{
			auto& e1 = edges[index1];
			auto& e2 = edges[index2];
			e1.links[1] = index2;
			e2.links[0] = index1;
		};

		std::unordered_map< uint64, int > edgePairMap;
		edgePairMap.reserve(edges.size());

		bool bHadMergeEdge = false;
		auto TryMergeEdge = [&](int indexEdge)
		{
			Edge& edge = edges[indexEdge];
			Edge& edgeNext = edges[edge.links[1]];
			CHECK(edge.verticeIds[1] == edgeNext.verticeIds[0]);

			Vector3 d1 = GetPosition(edge.verticeIds[1]) - GetPosition(edge.verticeIds[0]);
			Vector3 d2 = GetPosition(edgeNext.verticeIds[1]) - GetPosition(edgeNext.verticeIds[0]);

			float l1 = d1.normalize();
			float l2 = d2.normalize();

			float dot = d1.dot(d2);

			if (1 - Math::Abs(dot) > 1e-4 )
				return false;

			auto iter = edgePairMap.find(MakeKey(edge.verticeIds[1], edge.verticeIds[0]));
			if (iter != edgePairMap.end())
			{
				edgePairMap.erase(iter);
			}

			edge.verticeIds[1] = edgeNext.verticeIds[1];
			edge.bNeedCheck = true;
			ConnectLink(indexEdge, edgeNext.links[1]);

#if 0
			CheckVaild(indexEdge);
			CheckVaild(edgeNext.links[1]);
#endif

			edgeNext.links[0] = INDEX_NONE;

			bHadMergeEdge = true;
			return true;
		};


		auto TryRemoveEdge = [&](int i, int j) -> bool
		{
			auto& ei = edges[i];
			auto& ej = edges[j];

			if (ej.links[0] == INDEX_NONE)
				return false;

			CHECK(ei.verticeIds[0] == ej.verticeIds[1] &&
				  ei.verticeIds[1] == ej.verticeIds[0]);

			ConnectLink(ei.links[0], ej.links[1]);
			ConnectLink(ej.links[0], ei.links[1]);

#if 0
			CheckVaild(ei.links[0]);
			CheckVaild(ei.links[1]);
			CheckVaild(ej.links[0]);
			CheckVaild(ej.links[1]);
#endif

			TryMergeEdge(ei.links[0]);
			TryMergeEdge(ej.links[0]);

			ei.links[0] = INDEX_NONE;
			ej.links[0] = INDEX_NONE;

			return true;
		};


		bool bHadRemoveEdge = false;
		for (int i = 0; i < edges.size(); ++i)
		{
			if (edges[i].links[0] == INDEX_NONE)
				continue;
			
			auto iter = edgePairMap.find(MakeKey(edges[i].verticeIds[0], edges[i].verticeIds[1]));
			if (iter == edgePairMap.end() || !TryRemoveEdge(i , iter->second))
			{
				edgePairMap[MakeKey(edges[i].verticeIds[1], edges[i].verticeIds[0])] = i;
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
				if (edges[i].bNeedCheck == false)
					continue;
				edges[i].bNeedCheck = false;

				if (edges[i].links[0] == INDEX_NONE)
					continue;

				auto iter = edgePairMap.find(MakeKey(edges[i].verticeIds[0], edges[i].verticeIds[1]));
				if (iter == edgePairMap.end() || !TryRemoveEdge(i, iter->second))
				{
					edgePairMap[MakeKey(edges[i].verticeIds[1], edges[i].verticeIds[0])] = i;
				}
			}
		}

		TArray< Vertex > verticesNew;
		TArray< uint32 > indicesNew;
		for (int i = 0; i < edges.size(); ++i)
		{
			if ( edges[i].links[0] == INDEX_NONE )
				continue;

			int count = 0;
			int indexOffset = verticesNew.size();
			int indexEdge = i;

			TriangulateAlgorithm algo;
			algo.mNoraml = mVertices[edges[i].verticeIds[0]].normal;
			do
			{	auto& edge = edges[indexEdge];

				verticesNew.push_back(mVertices[edge.verticeIds[0]]);
				++count;
				edge.links[0] = INDEX_NONE;
				indexEdge = edge.links[1];
			}
			while( indexEdge != i);

			{
				TIME_SCOPE(GTriTime);
				algo.run(count, (uint8*)(verticesNew.data()), sizeof(Vertex), indicesNew, indexOffset, nullptr);
			}
		}

		mVertices = std::move(verticesNew);
		mIndices = std::move(indicesNew);
	}

}//namespace Cube
