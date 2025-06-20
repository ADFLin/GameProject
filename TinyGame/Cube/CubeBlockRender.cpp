#include "CubePCH.h"
#include "CubeBlockRender.h"

#include "CubeBlock.h"
#include "CubeBlockType.h"

#include "CubeMesh.h"
#include "ProfileSystem.h"

#include <unordered_map>

namespace Cube
{

	static uint32 TexPosToMatId(uint32 x, uint32 y)
	{
		return x + 64 * y;
	}


	void BlockRenderer::draw(Vec3i const& offset, BlockId id )
	{
		Block* block = Block::Get( id );
		unsigned faceMask = block->calcRenderFaceMask( *mBlockAccess , mBasePos + offset);
		if ( !faceMask )
			return;

		setVertexOffset(Vec3f(mBasePos + offset));

		switch( id )
		{
		case BLOCK_BASE:
			drawSimple(faceMask, TexPosToMatId(11,0));
			break;
		case BLOCK_ROCK:
			drawSimple(faceMask, TexPosToMatId(22, 7));
			break;
		case BLOCK_DIRT:
		default:
			drawUnknown(faceMask);
		}
	}

	void BlockRenderer::drawSimple(unsigned faceMask, uint32 matId)
	{
		setColor(mDebugColor);
		for (int face = 0; face < FaceSide::COUNT; ++face)
		{
			if (faceMask & BIT(face))
			{
				addBlockFace(FaceSide(face), matId);
			}
		}
	}

	void BlockRenderer::drawUnknown(unsigned faceMask)
	{
		setColor(mDebugColor);
		//mesh.setColor( 0 , 255 , 0 );
		static uint32 const materialIdMap[] =
		{
			TexPosToMatId(28,7),
			TexPosToMatId(28,7),
			TexPosToMatId(28,7),
			TexPosToMatId(28,7),
			TexPosToMatId(29,15),
			TexPosToMatId(24,2),
		};

		for (int face = 0; face < FaceSide::COUNT; ++face)
		{
			if (faceMask & BIT(face))
			{
				int matId = materialIdMap[face];
				if ( GetFaceAxis(FaceSide(face)) != 2 && (faceMask & BIT(FaceSide::FACE_Z)) == 0 )
				{
					matId = TexPosToMatId(4, 15);
				}
				addBlockFace(FaceSide(face), matId);
			}
		}
	}

#define USE_QUAD 1


	void BlockRenderer::addBlockFace(FaceSide face, uint32 matId)
	{
#if USE_QUAD
		BlockSurfaceQuad quad;
		quad.face = face;
		quad.offset = mVertexOffset;
		quad.matId = matId;
		mQuads.push_back(quad);
#else
		uint32 indexBase = mVertices.size();
		for (int i = 0; i < 4; ++i)
		{
			BlockVertex v;
			v.face = face;
			v.pos = mVertexOffset + GetFaceVertices(face)[i];
			v.matId = matId;

			mVertices.push_back(v);
			bound += v.pos;
		}
		addBlockQuad(indexBase + 0, indexBase + 1, indexBase + 2, indexBase + 3);
#endif
	}

	void BlockRenderer::addMeshVertex(Vec3f const& pos)
	{
		mCurVertex.pos = mVertexOffset + pos;
		mMesh->mVertices.push_back(mCurVertex);
		bound += mCurVertex.pos;
	}

	void BlockRenderer::addMeshVertex(float x, float y, float z)
	{
		addMeshVertex(Vec3f(x, y, z));
	}

	void BlockRenderer::setColor(uint8 r, uint8 g, uint8 b)
	{
		mCurVertex.color[0] = r;
		mCurVertex.color[1] = g;
		mCurVertex.color[2] = b;
		mCurVertex.color[3] = 255;
	}

	void BlockRenderer::addMeshTriangle(int i1, int i2, int i3)
	{
		mMesh->mIndices.push_back(i1);
		mMesh->mIndices.push_back(i2);
		mMesh->mIndices.push_back(i3);
	}

	void BlockRenderer::addMeshQuad(int i1, int i2, int i3, int i4)
	{
		addMeshTriangle(i1, i2, i3);
		addMeshTriangle(i1, i3, i4);
	}

	using Math::Vector3;
	Vector3 GetSafeNormal(Vector3 const& v)
	{
		Vector3 result = v;
		if (result.normalize() == 0.0)
			return Vector3::Zero();

		return result;
	}

	bool IsNearlyZero(Vector3 const& v)
	{
		float const ZeroError = 1e-4;
		return Math::Abs(v.x) < ZeroError && Math::Abs(v.y) < ZeroError && Math::Abs(v.z) < ZeroError;
	}


	struct Edge
	{
		uint32 vertexId;
		Edge*  prev;
		Edge*  next;
		union
		{
			bool   bNeedCheck;
			struct
			{
				Vector3 p;
				bool    isEar;
				float   angle;
			};
		};
	};

	class TriangulateAlgorithm
	{
	public:


		using PartitionVertex = Edge;


		Vector3 mNoraml;
		TArray< PartitionVertex > mVertices;
		PartitionVertex* mHead;

		bool IsConvex(Vector3 const& p1, Vector3 const& p2, Vector3 const& p3)
		{
			Vector3 n = Math::Cross(p2 - p1, p3 - p1);
			return Math::Dot(mNoraml, n) > 0;
		}

		bool IsInside(Vector3 const& p1, Vector3 const& p2, Vector3 const& p3, Vector3 const& p)
		{
#if 0
			float coords[3];
			if (!Math::Barycentric(p, p1, p2, p3, coords))
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
				for( auto vertex = mHead; vertex->next != mHead; vertex = vertex->next)
				{
					CHECK(vertex->prev != nullptr);
#if 0
					if (IsNearlyZero(vertex->p - v.p) ||
						IsNearlyZero(vertex->p - v1->p) ||
						IsNearlyZero(vertex->p - v3->p))
						continue;
#else
					if (vertex == &v)
						continue;
					if (vertex == v1)
						continue;
					if (vertex == v3)
						continue;
#endif

					if (IsInside(v1->p, v.p, v3->p, vertex->p))
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
		bool run(int numVertices, uint8* verticesData, int dataStride, int indexOffset, int* pIndices, TArray< uint32 >& outTriList)
		{
			if (numVertices < 3)
				return false;

			if (numVertices == 3)
			{
				if (pIndices)
				{
					outTriList.push_back(indexOffset + pIndices[0]);
					outTriList.push_back(indexOffset + pIndices[1]);
					outTriList.push_back(indexOffset + pIndices[2]);
				}
				else
				{
					outTriList.push_back(indexOffset + 0);
					outTriList.push_back(indexOffset + 1);
					outTriList.push_back(indexOffset + 2);
				}
				return true;
			}

			mVertices.resize(numVertices);
			PartitionVertex* prev = &mVertices[numVertices - 1];
			for (int i = 0; i < numVertices; ++i)
			{
				int idx = indexOffset;
				if (pIndices)
					idx += pIndices[i];
				else
					idx += i;

				PartitionVertex& v = mVertices[i];
				v.vertexId = idx;
				v.p = *reinterpret_cast<Vector3*>(verticesData + idx * dataStride);
				v.prev = prev;
				v.next = mVertices.data() + (i + 1);
				prev = &v;
			}
			prev->next = mVertices.data();

			mHead = &mVertices[0];
			return runInternal(numVertices, outTriList);
		}

		bool run(PartitionVertex* head, int numVertices, TArray< uint32 >& outTriList)
		{
			if (numVertices < 3)
				return false;

			mHead = head;
			if (numVertices == 3)
			{
				outTriList.push_back(head->prev->vertexId);
				outTriList.push_back(head->vertexId);
				outTriList.push_back(head->next->vertexId);
				return true;
			}
			return runInternal(numVertices, outTriList);
		}

		bool runInternal(int numVertices, TArray< uint32 >& outTriList)
		{

			for(auto vertex = mHead; vertex->next != mHead; vertex = vertex->next )
			{
				updateVertex(*vertex);
			}

			for (int i = 0; i < numVertices - 3; ++i)
			{
				PartitionVertex* ear = mHead;
				//find the most extruded ear

				for (auto vertex = mHead->next; vertex->next != mHead; vertex = vertex->next)
				{
					CHECK(vertex->prev != nullptr);
					if (vertex->angle > ear->angle)
					{
						ear = vertex;
					}
				}

				CHECK(ear);

				outTriList.push_back(ear->prev->vertexId);
				outTriList.push_back(ear->vertexId);
				outTriList.push_back(ear->next->vertexId);

				ear->prev->next = ear->next;
				ear->next->prev = ear->prev;
				ear->prev = nullptr;
				if (ear == mHead)
				{
					mHead = mHead->next;
				}

				if (i == numVertices - 4)
					break;

				updateVertex(*ear->prev);
				updateVertex(*ear->next);
			}

			outTriList.push_back(mHead->prev->vertexId);
			outTriList.push_back(mHead->vertexId);
			outTriList.push_back(mHead->next->vertexId);
			return true;
		}

	};


	extern double GTriTime;
	uint64 MakeKey(uint32 a, uint32 b)
	{
		return (uint64(a) << 32) | uint64(b);
	}

	void BlockRenderer::mergeBlockPrimitives()
	{
		TArray< Edge > edges;
		std::unordered_map< BlockVertex, uint32, MemberFuncHasher > vertexIdMap;

		edges.resize(mQuads.size() * 4);
		Edge* pEdge = edges.data();
		for (auto const& quad : mQuads)
		{
			uint32 quadIndices[4];
			for( int i = 0 ; i < 4 ; ++i )
			{
				BlockVertex v;
				v.face = quad.face;
				v.pos = quad.offset + GetFaceVertices(quad.face)[i];
				v.matId = quad.matId;

				auto iter = vertexIdMap.find(v);
				if (iter != vertexIdMap.end())
				{
					quadIndices[i] = iter->second;
				}
				else
				{
					quadIndices[i] = mVertices.size();
					mVertices.push_back(v);
					vertexIdMap.emplace(v , quadIndices[i]);
					bound += v.pos;
				}
			}

			Edge& e0 = pEdge[0];
			Edge& e1 = pEdge[1];
			Edge& e2 = pEdge[2];
			Edge& e3 = pEdge[3];
			e0.bNeedCheck = false;
			e0.vertexId = quadIndices[0];
			e0.prev = &e3;
			e0.next = &e1;

			e1.bNeedCheck = false;
			e1.vertexId = quadIndices[1];
			e1.prev = &e0;
			e1.next = &e2;

			e2.bNeedCheck = false;
			e2.vertexId = quadIndices[2];
			e2.prev = &e1;
			e2.next = &e3;

			e3.bNeedCheck = false;
			e3.vertexId = quadIndices[3];
			e3.prev = &e2;
			e3.next = &e0;
			pEdge += 4;
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

			Vector3 d1 = GetPosition(edge.next->vertexId) - GetPosition(edge.vertexId);
			Vector3 d2 = GetPosition(edgeNext.next->vertexId) - GetPosition(edgeNext.vertexId);

			float l1 = d1.normalize();
			float l2 = d2.normalize();

			float dot = d1.dot(d2);

			if (1 - Math::Abs(dot) > 1e-4)
				return false;

			auto iter = edgePairMap.find(MakeKey(edge.next->vertexId, edge.vertexId));
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

			auto iter = edgePairMap.find(MakeKey(edge.vertexId, edge.next->vertexId));
			if (iter == edgePairMap.end() || !TryRemoveEdge(edge, *iter->second))
			{
				edgePairMap[MakeKey(edge.next->vertexId, edge.vertexId)] = &edge;
			}
			else
			{
				bHadRemoveEdge = true;
			}
		}

		if (!bHadRemoveEdge)
		{
			mIndices.reserve(6 * mQuads.size());
			Edge* pEdge = edges.data();
			for( int i = 0 ; i < mQuads.size(); ++i)
			{
				Edge& e0 = pEdge[0];
				Edge& e1 = pEdge[1];
				Edge& e2 = pEdge[2];
				Edge& e3 = pEdge[3];
				addBlockQuad(e0.vertexId, e1.vertexId, e2.vertexId, e3.vertexId);
				pEdge += 4;
			}
			return;
		}

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

				auto iter = edgePairMap.find(MakeKey(edge.vertexId, edge.next->vertexId));
				if (iter == edgePairMap.end() || !TryRemoveEdge(edge, *iter->second))
				{
					edgePairMap[MakeKey(edge.next->vertexId, edge.vertexId)] = &edge;
				}
			}
		}


		TArray< int > vertexIdRemap;
		vertexIdRemap.resize((int)mVertices.size(), INDEX_NONE);
		TArray< BlockVertex > verticesNew;
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
			algo.mNoraml = GetFaceNoraml( FaceSide(mVertices[edge.vertexId].face) );
			Edge* curEdge = &edge;
#if 1
			do
			{
				auto const& v = mVertices[curEdge->vertexId];
				if (vertexIdRemap[curEdge->vertexId] == INDEX_NONE)
				{
					vertexIdRemap[curEdge->vertexId] = verticesNew.size();
					verticesNew.push_back(v);
				}
				curEdge->vertexId = vertexIdRemap[curEdge->vertexId];
				++count;
				//curEdge->prev = nullptr;
				curEdge->p = v.pos;
				curEdge = curEdge->next;

			} while (curEdge != &edge);

			{
				TIME_SCOPE(GTriTime);
				algo.run(&edge, count, indicesNew);
				algo.mHead->prev->prev = nullptr;
				algo.mHead->next->prev = nullptr;
				algo.mHead->prev = nullptr;
			}
#else
			do
			{
				verticesNew.push_back(mVertices[curEdge->vertexId]);
				++count;
				curEdge->prev = nullptr;
				curEdge = curEdge->next;
			} while (curEdge != &edge);

			{
				TIME_SCOPE(GTriTime);
				algo.run(count, (uint8*)(verticesNew.data()), sizeof(BlockVertex), indexOffset, nullptr, indicesNew);
			}
#endif
		}

		mVertices = std::move(verticesNew);
		mIndices = std::move(indicesNew);
	}

	void BlockRenderer::fillBlockPrimitivesToMesh()
	{
		uint32 baseIndex = mMesh->mVertices.size();
		mMesh->mVertices.resize(baseIndex + mVertices.size());

		Vec3f faceUVVectorMap[FaceSide::COUNT][2] =
		{
			{ Vec3f(0,1,0), Vec3f(0,0,-1) },
			{ Vec3f(0,1,0), Vec3f(0,0,-1) },
			{ Vec3f(1,0,0), Vec3f(0,0,-1) },
			{ Vec3f(1,0,0), Vec3f(0,0,-1) },
			{ Vec3f(1,0,0), Vec3f(0,1,0) },
			{ Vec3f(1,0,0), Vec3f(0,1,0) },
		};

		Mesh::Vertex* pDest = mMesh->mVertices.data() + baseIndex;
		for (int i = 0; i < mVertices.size(); ++i)
		{
			BlockVertex const& src = mVertices[i];
			Mesh::Vertex& dest = pDest[i];
			dest.pos = src.pos;
			dest.uv = Vec2f(faceUVVectorMap[src.face][0].dot(src.pos), faceUVVectorMap[src.face][1].dot(src.pos));
			dest.normal = GetFaceNoraml(FaceSide(src.face));
			dest.color = Color4ub(255,255,255,255);
			dest.meta = src.matId;
		}

		int indexOffset = mMesh->mIndices.size();
		mMesh->mIndices.resize(mMesh->mIndices.size() + mIndices.size());
		uint32* pDestIndices = mMesh->mIndices.data() + indexOffset;
		for (int i = 0; i < mIndices.size(); ++i)
		{
			pDestIndices[i] = mIndices[i] + baseIndex;
		}
	}

}//namespace Cube