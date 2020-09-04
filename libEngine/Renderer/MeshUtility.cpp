#include "MeshUtility.h"

#include "RHI/RHICommand.h"
#include "Renderer/Mesh.h"

#include "WindowsHeader.h"
#include "MarcoCommon.h"
#include "FixString.h"
#include "FileSystem.h"
#include "LogSystem.h"

#include "Core/ScopeExit.h"
#include "DataStructure/KDTree.h"
#include "Math/PrimitiveTest.h"
#include "AsyncWork.h"
#include "Serialize/DataStream.h"

#include "NvTriStrip/NvTriStrip.h"
#include "NvTess/nvtess.h"
#include "DirectXSamples/D3D12MeshletGenerator.h"

#include <unordered_map>
#include <memory>
#include <fstream>
#include <algorithm>


namespace
{
	struct InlineMeshlet
	{
		struct PackedTriangle
		{
			uint32 i0 : 10;
			uint32 i1 : 10;
			uint32 i2 : 10;
			uint32 spare : 2;
		};

		std::vector<int>            uniqueVertexIndices;
		std::vector<PackedTriangle> primitiveIndices;
	};


	struct EdgeEntry
	{
		uint32_t   i0;
		uint32_t   i1;
		uint32_t   i2;

		uint32_t   Face;
		EdgeEntry* Next;
	};

	size_t CRCHash(const uint32_t* dwords, uint32_t dwordCount)
	{
		size_t h = 0;

		for (uint32_t i = 0; i < dwordCount; ++i)
		{
			uint32_t highOrd = h & 0xf8000000;
			h = h << 5;
			h = h ^ (highOrd >> 27);
			h = h ^ size_t(dwords[i]);
		}

		return h;
	}

	template <typename T>
	inline size_t Hash(const T& val)
	{
		return std::hash<T>()(val);
	}
}

namespace std
{
	template <> struct hash< Math::Vector3 > { size_t operator()(const Math::Vector3& v) const { return CRCHash(reinterpret_cast<const uint32_t*>(&v), sizeof(v) / 4); } };
}

namespace Render
{
	bool VerifyOpenGLStatus();
	bool gbOptimizeVertexCache = false;


	int* MeshUtility::ConvertToTriangleList(EPrimitive type, void* pIndexData , int numIndices, bool bIntType , std::vector< int >& outConvertBuffer, int& outNumTriangles)
	{
		if( bIntType )
		{
			return ConvertToTriangleListIndices<int32>(type, (int32*)pIndexData, numIndices, outConvertBuffer, outNumTriangles);
		}
		else
		{
			return ConvertToTriangleListIndices<int16>(type, (int16*)pIndexData, numIndices, outConvertBuffer, outNumTriangles);
		}

	}


	void MeshUtility::CalcAABB(VertexElementReader const& positionReader, Vector3& outMin, Vector3& outMax)
	{
		assert(positionReader.getNum() >= 1);
		outMax = outMin = positionReader.get(0);
		for( int i = 1; i < positionReader.getNum(); ++i )
		{
			Vector3 const& pos = positionReader.get(i);
			outMax.max(pos);
			outMin.min(pos);
		}
	}

	bool MeshUtility::BuildDistanceField(Mesh& mesh, DistanceFieldBuildSetting const& setting, DistanceFieldData& outResult)
	{
		if( !mesh.mIndexBuffer.isValid() || !mesh.mVertexBuffer.isValid() )
			return false;

		uint8* pData = (uint8*)( RHILockBuffer( mesh.mVertexBuffer , ELockAccess::ReadOnly)) + mesh.mInputLayoutDesc.getAttributeOffset(Vertex::ATTRIBUTE_POSITION);
		void* pIndexBufferData = RHILockBuffer( mesh.mIndexBuffer , ELockAccess::ReadOnly);
		ON_SCOPE_EXIT
		{
			RHIUnlockBuffer(mesh.mVertexBuffer);
			RHIUnlockBuffer(mesh.mIndexBuffer);
		};
		std::vector<int> tempBuffer;
		int numTriangles;
		int* pIndexData = ConvertToTriangleList(mesh.mType, pIndexBufferData, mesh.mIndexBuffer->getNumElements() , mesh.mIndexBuffer->isIntType(), tempBuffer , numTriangles);
		bool result = false;
		if ( pIndexData )
		{
			result = BuildDistanceField(mesh.makePositionReader(pData), pIndexData, numTriangles, setting, outResult);
		}

		return result;
	}

	bool MeshUtility::BuildDistanceField(VertexElementReader const& positionReader , int* pIndexData, int numTriangles, DistanceFieldBuildSetting const& setting, DistanceFieldData& outResult)
	{
#define USE_KDTREE 1
		Vector3 boundMin;
		Vector3 boundMax;
		CalcAABB(positionReader, boundMin, boundMax);

		boundMax *= setting.boundSizeScale;
		boundMin *= setting.boundSizeScale;
		Vector3 boundSize = boundMax - boundMin;

		float boundMaxDistanceSqr = boundSize.length2();

		Vector3 gridSizeFloat = setting.ResolutionScale * boundSize / setting.gridLength;
		IntVector3 gridSize = IntVector3(Math::CeilToInt(gridSizeFloat.x), Math::CeilToInt(gridSizeFloat.y), Math::CeilToInt(gridSizeFloat.z));
		gridSize = Math::Clamp(gridSize, IntVector3(2, 2, 2), IntVector3(128, 128, 128));
		Vector3 gridLength = (boundSize).div(Vector3(gridSize.x, gridSize.y, gridSize.z));
		outResult.volumeData.resize(gridSize.x * gridSize.y * gridSize.z);

#if USE_KDTREE
		using MyTree = TKDTree<3>;
		MyTree tree;
		{
			int* pIndex = pIndexData;
			for( int i = 0; i < numTriangles; ++i )
			{
				int idx0 = pIndex[0];
				int idx1 = pIndex[1];
				int idx2 = pIndex[2];

				Vector3 const& p0 = positionReader.get(idx0);
				Vector3 const& p1 = positionReader.get(idx1);
				Vector3 const& p2 = positionReader.get(idx2);
				pIndex += 3;
				MyTree::PrimitiveData data;
				data.BBox.min = p0;
				data.BBox.max = p0;
				data.BBox += p1;
				data.BBox += p2;
				data.id = i;
				tree.mDataVec.push_back(data);
			}

			tree.build();
		}
#endif

		Vector3 minCellPos = boundMin + 0.5 * gridLength;
		auto TaskFunc = [positionReader , pIndexData , gridSize , minCellPos , gridLength, &tree , &outResult](int nz , float& maxDistanceSqr)
		{
			for( int ny = 0; ny < gridSize.y; ++ny )
			{
				for( int nx = 0; nx < gridSize.x; ++nx )
				{

					int indexCell = gridSize.x * (gridSize.y * nz + ny) + nx;
					Vector3 p = minCellPos + Vector3(nx, ny, nz).mul(gridLength);

#if USE_KDTREE
					int count = 0;
					float side = 1.0;
					auto DistFunc = [&](MyTree::PrimitiveData const& data, Vector3 const& pos, float minDistSqr)
					{
						++count;
						int* pIndex = pIndexData + 3 * data.id;
						int idx0 = pIndex[0];
						int idx1 = pIndex[1];
						int idx2 = pIndex[2];

						Vector3 const& p0 = positionReader.get(idx0);
						Vector3 const& p1 = positionReader.get(idx1);
						Vector3 const& p2 = positionReader.get(idx2);

						float curSide;
						Vector3 closestPoint = Math::PointToTriangleClosestPoint(pos, p0, p1, p2, curSide);
						float distSqr = Math::DistanceSqure(closestPoint, pos);
						if( distSqr < minDistSqr )
						{
							side = curSide;
						}
						return distSqr;
					};
					float distSqr;
					tree.findNearst(p, DistFunc, distSqr);

					outResult.volumeData[indexCell] = side * Math::Sqrt(distSqr);
#else
					int* pIndex = pIndexData;
					for( int i = 0; i < numTriangles; ++i )
					{
						int idx0 = pIndex[0];
						int idx1 = pIndex[1];
						int idx2 = pIndex[2];

						Vector3 const& p0 = positionReader.get(idx0);
						Vector3 const& p1 = positionReader.get(idx1);
						Vector3 const& p2 = positionReader.get(idx2);

						float side = 1.0;
						Vector3 closestPoint = Math::PointToTriangleClosestPoint(p, p0, p1, p2, side);
						float distSqr = Math::DistanceSqure(closestPoint, p);
						if( i == 0 || Math::Abs(outData[indexCell]) > distSqr )
							outData[indexCell] = Math::Sqrt(distSqr) * side;

						pIndex += 3;
					}
#endif
					if( maxDistanceSqr < Math::Abs(outResult.volumeData[indexCell]) )
						maxDistanceSqr = Math::Abs(outResult.volumeData[indexCell]);
				}
			}
		};

		float maxDistanceSqr = boundMaxDistanceSqr;
		if( setting.WorkTaskPool == nullptr )
		{	
			for( int nz = 0; nz < gridSize.z; ++nz )
			{
				TaskFunc(nz, maxDistanceSqr);
			}
		}
		else
		{
			
			struct MyTask : public IQueuedWork
			{
				MyTask(decltype(TaskFunc)& func , float inDistSqr , int inNz)
					:mFunc(func),maxDistanceSqr(inDistSqr),nz(inNz) {}
				void executeWork() override { mFunc(nz , maxDistanceSqr); }
				void release() override {}
				decltype( TaskFunc )& mFunc;
				float maxDistanceSqr;
				int nz;
			};

			std::vector< std::unique_ptr< MyTask > > allTasks;
			for( int nz = 0; nz < gridSize.z; ++nz )
			{
				auto task = std::make_unique<MyTask>( TaskFunc , boundMaxDistanceSqr , nz );
				setting.WorkTaskPool->addWork(task.get());
				allTasks.push_back(std::move(task));
			}

			setting.WorkTaskPool->waitAllWorkComplete();
			for( auto& ptr : allTasks )
			{
				if( maxDistanceSqr > ptr->maxDistanceSqr )
					maxDistanceSqr = ptr->maxDistanceSqr;
			}
		}


		float maxDistance = Math::Sqrt(maxDistanceSqr);
		for( auto& v : outResult.volumeData )
		{
			v /= maxDistance;
		}

		outResult.boundMax = boundMax;
		outResult.boundMin = boundMin;
		outResult.gridSize = gridSize;
		outResult.maxDistance = maxDistance;


		return true;
	}

	void MeshUtility::BuildTessellationAdjacency(VertexElementReader const& positionReader, int* triIndices, int numTirangle, std::vector<int>& outResult)
	{
		class MyRenderBuffer : public nv::RenderBuffer
		{
		public:
			MyRenderBuffer(VertexElementReader const& positionReader, int* triIndices, int numTirangle , VertexElementReader const* texcoordReader = nullptr)
				:mPositionReader( positionReader )
				,mTexcoordReader( texcoordReader )
			{
				mIb = new nv::IndexBuffer(triIndices, nv::IBT_U32, numTirangle * 3, false);
			}
			nv::Vertex getVertex(unsigned int index) const override
			{
				nv::Vertex v;
				Vector3 pos = mPositionReader.get(index);
				v.pos.x = pos.x;
				v.pos.y = pos.y;
				v.pos.z = pos.z;
				if( mTexcoordReader )
				{
					Vector2 uv = mTexcoordReader->get<Vector2>(index);
					v.uv.x = uv.x;
					v.uv.y = uv.y;
				}
				else
				{
					v.uv.x = 0;
					v.uv.y = 0;
				}
				return v;
			}

			VertexElementReader const& mPositionReader;
			VertexElementReader const* mTexcoordReader;
		};
		MyRenderBuffer renderBuffer( positionReader , triIndices , numTirangle );
		nv::IndexBuffer* outBuffer = nv::tess::buildTessellationBuffer( &renderBuffer, nv::DBM_PnAenOnly , true );
		if( outBuffer )
		{
			outResult.resize(outBuffer->getLength());
			for( int i = 0; i < outBuffer->getLength(); ++i )
			{
				outResult[i] = (*outBuffer)[i];
			}
			delete outBuffer;
		}
	}

	void MeshUtility::BuildVertexAdjacency(VertexElementReader const& positionReader, int* triIndices, int numTirangle, std::vector<int>& outResult)
	{
		std::vector< int > triangleIndexMap;
		struct Vertex
		{
			int   index;
			float z;
		};
		std::vector< Vertex > sortedVertices;
		sortedVertices.resize(positionReader.getNum());
		for( int i = 0; i < positionReader.getNum(); ++i )
		{
			Vector3 const& pos = positionReader.get(i);
			sortedVertices[i].index = i;
			sortedVertices[i].z = pos.z;
		}

		std::sort(sortedVertices.begin(), sortedVertices.end(), [](auto const& a, auto const& b)
		{
			return a.z < b.z;
		});
		triangleIndexMap.resize(positionReader.getNum(), -1);
		for( int i = 0; i < positionReader.getNum(); ++i )
		{
			Vertex const& vi = sortedVertices[i];
			if( triangleIndexMap[vi.index] != -1 )
				continue;

			triangleIndexMap[vi.index] = vi.index;
			Vector3 const& pos0 = positionReader.get(vi.index);
			for( int j = i + 1; j < positionReader.getNum(); ++j )
			{
				Vertex const& vj = sortedVertices[j];
				Vector3 const& pos1 = positionReader.get(vj.index);

				if( Math::Abs(pos0.z - pos1.z) > 1e-6 )
				{
					break;
				}

				if( IsVertexEqual(pos0, pos1) )
				{
					triangleIndexMap[vj.index] = vi.index;
				}
			}
		}

		struct TrianglePair
		{
			int val[2];
			TrianglePair()
			{
				val[0] = val[1] = -1;
			}
		};
		std::unordered_map< uint64, TrianglePair > edgeToTriangle;

		auto MakeEdgeId = [](int idx1, int idx2) -> uint64
		{
			return (idx1 < idx2) ? (((uint64(idx1) << 32)) | uint32(idx2)) : (((uint64(idx2) << 32)) | uint32(idx1));
		};
		auto AddEdgeToTriangle = [&](int idxTriIndices, int idx1, int idx2)
		{
			if( idx1 == idx2 )
				return;
			uint64 edgeId = MakeEdgeId(idx1, idx2);
			auto& triPair = edgeToTriangle[edgeId];

			if( idx1 > idx2 )
			{
				if( triPair.val[0] == -1 )
				{
					triPair.val[0] = idxTriIndices;
				}
				else
				{
					LogWarning(0, "Incomplete Mesh!!");
				}
			}
			else
			{
				if( triPair.val[1] == -1 )
				{
					triPair.val[1] = idxTriIndices;
				}
				else
				{
					LogWarning(0, "Incomplete Mesh!!");
				}
			}
		};

		for( int i = 0; i < numTirangle; ++i )
		{
			int idx0 = triangleIndexMap[triIndices[3 * i + 0]];
			int idx1 = triangleIndexMap[triIndices[3 * i + 1]];
			int idx2 = triangleIndexMap[triIndices[3 * i + 2]];

			AddEdgeToTriangle(3 * i + 2, idx0, idx1);
			AddEdgeToTriangle(3 * i + 0, idx1, idx2);
			AddEdgeToTriangle(3 * i + 1, idx2, idx0);
		}

		int incompleteEdgeCount = 0;
		auto GetAdjIndex = [&](int idxTriIndices, int idx1, int idx2)
		{
			uint64 edgeId = MakeEdgeId(idx1, idx2);
			auto& triPair = edgeToTriangle[edgeId];
			int idxOtherTri;
			if( idx1 > idx2 )
			{
				idxOtherTri = triPair.val[1];
			}
			else
			{
				idxOtherTri = triPair.val[0];

			}
			if( idxOtherTri == -1 )
			{
				++incompleteEdgeCount;
			}
			return (idxOtherTri == -1) ? -1 : triIndices[idxOtherTri];
		};

		if( incompleteEdgeCount > 0 )
		{
			LogWarning(0, "%d Edge not have adj", incompleteEdgeCount);
		}

		outResult.resize(6 * numTirangle);

		int* pInddex = &outResult[0];
		for( int i = 0; i < numTirangle; ++i )
		{
			int idx0 = triIndices[3 * i + 0];
			int idx1 = triIndices[3 * i + 1];
			int idx2 = triIndices[3 * i + 2];

			int idxMapped0 = triangleIndexMap[idx0];
			int idxMapped1 = triangleIndexMap[idx1];
			int idxMapped2 = triangleIndexMap[idx2];

			if( idxMapped0 == idxMapped1 || idxMapped1 == idxMapped2 || idxMapped2 == idxMapped0 )
				continue;

			int adj0 = GetAdjIndex(3 * i + 2, idxMapped0, idxMapped1);
			int adj1 = GetAdjIndex(3 * i + 0, idxMapped1, idxMapped2);
			int adj2 = GetAdjIndex(3 * i + 1, idxMapped2, idxMapped0);

			pInddex[0] = idx0; pInddex[1] = adj0;
			pInddex[2] = idx1; pInddex[3] = adj1;
			pInddex[4] = idx2; pInddex[5] = adj2;

			pInddex += 6;
		}
		outResult.resize(pInddex - &outResult[0]);
	}

	void MeshUtility::OptimizeVertexCache(void* pIndices, int numIndex, bool bIntType)
	{
		NvTriStrip::PrimitiveGroup* groups = nullptr;
		uint32 numGroup = 0;

		NvTriStrip::SetCacheSize(24);
		NvTriStrip::SetListsOnly(true);
		if( bIntType )
		{
			NvTriStrip::GenerateStrips((uint32*)pIndices, numIndex, &groups, &numGroup);
		}
		else
		{
			std::vector<uint32> tempIndices{ (uint16*)pIndices , (uint16*)(pIndices)+numIndex };
			NvTriStrip::GenerateStrips((uint32*)&tempIndices[0], numIndex, &groups, &numGroup);
		}

		if( bIntType )
		{
			uint32* pData = (uint32*)pIndices;
			for( int i = 0; i < numGroup; ++i )
			{
				NvTriStrip::PrimitiveGroup& group = groups[i];
				assert(group.type == NvTriStrip::PT_LIST);
				memcpy(pData, group.indices, group.numIndices * sizeof(uint32));
				pData += group.numIndices;
			}
		}
		else
		{
			uint16* pData = (uint16*)pIndices;
			for( int i = 0; i < numGroup; ++i )
			{
				NvTriStrip::PrimitiveGroup& group = groups[i];
				assert(group.type == NvTriStrip::PT_LIST);
				for( int n = 0; n < group.numIndices; ++n )
				{
					*pData = (uint16)group.indices[n];
					++pData;
				}
			}
		}

		delete[] groups;
	}

	void MeshUtility::FillTriangleListNormalAndTangent(InputLayoutDesc const& desc, void* pVertex, int nV, int* idx, int nIdx)
	{
		FillNormalTangent_TriangleList(desc, pVertex, nV, idx, nIdx);
	}

	void MeshUtility::FillTriangleListTangent(InputLayoutDesc const& desc, void* pVertex, int nV, int* idx, int nIdx)
	{
		FillTangent_TriangleList(desc, pVertex, nV, idx, nIdx);
	}

	void MeshUtility::FillTriangleListNormal(InputLayoutDesc const& desc, void* pVertex, int nV, int* idx, int nIdx, int normalAttrib)
	{
		FillNormal_TriangleList(desc, pVertex, nV, idx, nIdx, normalAttrib);
	}

	bool CompareScores(const std::pair<uint32, float>& a, const std::pair<uint32, float>& b)
	{
		return a.second > b.second;
	}

	Vector3 ComputeNormal(Vector3 tri[])
	{
		Vector3 p0 = tri[0];
		Vector3 p1 = tri[1];
		Vector3 p2 = tri[2];

		Vector3 v01 = p0 - p1;
		Vector3 v02 = p0 - p2;

		return GetNormal(Math::Cross(v01, v02));
	}

	// Compute number of triangle vertices already exist in the meshlet
	template <typename T>
	uint32 ComputeReuse(const InlineMeshlet& meshlet, T(&triIndices)[3])
	{
		uint32 count = 0;

		for (uint32 i = 0; i < static_cast<uint32>(meshlet.uniqueVertexIndices.size()); ++i)
		{
			for (uint32 j = 0; j < 3u; ++j)
			{
				if (meshlet.uniqueVertexIndices[i] == triIndices[j])
				{
					++count;
				}
			}
		}

		return count;
	}

	// Computes a candidacy score based on spatial locality, orientational coherence, and vertex re-use within a meshlet.
	template <typename T>
	float ComputeScore(const InlineMeshlet& meshlet, Vector4 sphere, Vector3 normal, T(&triIndices)[3], Vector3 triVerts[])
	{
		const float reuseWeight = 0.334f;
		const float locWeight = 0.333f;
		const float oriWeight = 0.333f;

		// Vertex reuse
		uint32 reuse = ComputeReuse(meshlet, triIndices);
		float reuseScore = 1 - float(reuse) / 3.0f;

		// Distance from center point
		float maxSq = 0;
		for (uint32 i = 0; i < 3u; ++i)
		{
			Vector3 v = sphere.xyz() - triVerts[i];
			maxSq = Math::Max(maxSq, Math::Dot(v, v));
		}
		float r = sphere.w;
		float r2 = r * r;
		float locScore = Math::LogX(2 , maxSq / r2 + 1);

		// Angle between normal and meshlet cone axis
		Vector3 n = ComputeNormal(triVerts);
		float d = n.dot(normal);
		float oriScore = (-d + 1) / 2.0f;

		float b = reuseWeight * reuseScore + locWeight * locScore + oriWeight * oriScore;

		return b;
	}

	// Determines whether a candidate triangle can be added to a specific meshlet; if it can, does so.
	template <typename T>
	bool AddToMeshlet(uint32 maxVerts, uint32 maxPrims, InlineMeshlet& meshlet, T(&tri)[3])
	{
		// Are we already full of vertices?
		if (meshlet.uniqueVertexIndices.size() == maxVerts)
			return false;

		// Are we full, or can we store an additional primitive?
		if (meshlet.primitiveIndices.size() == maxPrims)
			return false;

		static const uint32 Undef = uint32(-1);
		uint32 indices[3] = { Undef, Undef, Undef };
		uint32 newCount = 3;

		for (uint32 i = 0; i < meshlet.uniqueVertexIndices.size(); ++i)
		{
			for (uint32 j = 0; j < 3; ++j)
			{
				if (meshlet.uniqueVertexIndices[i] == tri[j])
				{
					indices[j] = i;
					--newCount;
				}
			}
		}

		// Will this triangle fit?
		if (meshlet.uniqueVertexIndices.size() + newCount > maxVerts)
			return false;

		// Add unique vertex indices to unique vertex index list
		for (uint32 j = 0; j < 3; ++j)
		{
			if (indices[j] == Undef)
			{
				indices[j] = static_cast<uint32>(meshlet.uniqueVertexIndices.size());
				meshlet.uniqueVertexIndices.push_back(tri[j]);
			}
		}

		// Add the new primitive 
		typename  InlineMeshlet::PackedTriangle prim = {};
		prim.i0 = indices[0];
		prim.i1 = indices[1];
		prim.i2 = indices[2];

		meshlet.primitiveIndices.push_back(prim);

		return true;
	}

	bool IsMeshletFull(uint32 maxVerts, uint32 maxPrims, const InlineMeshlet& meshlet)
	{
		assert(meshlet.uniqueVertexIndices.size() <= maxVerts);
		assert(meshlet.primitiveIndices.size() <= maxPrims);

		return meshlet.uniqueVertexIndices.size() == maxVerts
			|| meshlet.primitiveIndices.size() == maxPrims;
	}


	Vector4 MinimumBoundingSphere(Vector3* points, uint32 count)
	{
		assert(points != nullptr && count != 0);

		// Find the min & max points indices along each axis.
		uint32 minAxis[3] = { 0, 0, 0 };
		uint32 maxAxis[3] = { 0, 0, 0 };

		for (uint32 i = 1; i < count; ++i)
		{
			float* point = (float*)(points + i);

			for (uint32 j = 0; j < 3; ++j)
			{
				float* min = (float*)(&points[minAxis[j]]);
				float* max = (float*)(&points[maxAxis[j]]);

				minAxis[j] = point[j] < min[j] ? i : minAxis[j];
				maxAxis[j] = point[j] > max[j] ? i : maxAxis[j];
			}
		}

		// Find axis with maximum span.
		float distSqMax = 0;
		uint32 axis = 0;

		for (uint32 i = 0; i < 3u; ++i)
		{
			Vector3 min = points[minAxis[i]];
			Vector3 max = points[maxAxis[i]];

			float distSq = (max - min).length2();
			if (distSq > distSqMax)
			{
				distSqMax = distSq;
				axis = i;
			}
		}

		// Calculate an initial starting center point & radius.
		Vector3 p1 = points[minAxis[axis]];
		Vector3 p2 = points[maxAxis[axis]];

		Vector3 center = (p1 + p2) * 0.5f;
		float radius = (p2 - p1).length() * 0.5f;
		float radiusSq = radius * radius;

		// Add all our points to bounding sphere expanding radius & recalculating center point as necessary.
		for (uint32 i = 0; i < count; ++i)
		{
			Vector3 point = points[i];
			float distSq = (point - center).length2();

			if (distSq > radiusSq)
			{
				float dist = Math::Sqrt(distSq);
				float k = (radius / dist) * 0.5f + (0.5f);

				center = center * k + point * (1 - k);
				radius = (radius + dist) * 0.5f;
			}
		}

		return Vector4(center, radius);
	}


	template <typename T>
	void BuildAdjacencyList(
		const T* indices, uint32_t indexCount, VertexElementReader const& positionReader,
		uint32_t* adjacency )
	{
		int vertexCount = positionReader.numVertex;

		const uint32_t triCount = indexCount / 3;
		// Find point reps (unique positions) in the position stream
		// Create a mapping of non-unique vertex indices to point reps
		std::vector<T> pointRep;
		pointRep.resize(vertexCount);

		std::unordered_map<size_t, T> uniquePositionMap;
		uniquePositionMap.reserve(vertexCount);

		for (uint32_t i = 0; i < vertexCount; ++i)
		{
			Vector3 const& position = positionReader.get(i);
			size_t hash = Hash(position);

			auto it = uniquePositionMap.find(hash);
			if (it != uniquePositionMap.end())
			{
				// Position already encountered - reference previous index
				pointRep[i] = it->second;
			}
			else
			{
				// New position found - add to hash table and LUT
				uniquePositionMap.insert(std::make_pair(hash, static_cast<T>(i)));
				pointRep[i] = static_cast<T>(i);
			}
		}

		// Create a linked list of edges for each vertex to determine adjacency
		const uint32_t hashSize = vertexCount / 3;

		std::unique_ptr<EdgeEntry*[]> hashTable(new EdgeEntry*[hashSize]);
		std::unique_ptr<EdgeEntry[]> entries(new EdgeEntry[triCount * 3]);

		std::memset(hashTable.get(), 0, sizeof(EdgeEntry*) * hashSize);
		uint32_t entryIndex = 0;

		for (uint32_t iFace = 0; iFace < triCount; ++iFace)
		{
			uint32_t index = iFace * 3;

			// Create a hash entry in the hash table for each each.
			for (uint32_t iEdge = 0; iEdge < 3; ++iEdge)
			{
				T i0 = pointRep[indices[index + (iEdge % 3)]];
				T i1 = pointRep[indices[index + ((iEdge + 1) % 3)]];
				T i2 = pointRep[indices[index + ((iEdge + 2) % 3)]];

				auto& entry = entries[entryIndex++];
				entry.i0 = i0;
				entry.i1 = i1;
				entry.i2 = i2;

				uint32_t key = entry.i0 % hashSize;

				entry.Next = hashTable[key];
				entry.Face = iFace;

				hashTable[key] = &entry;
			}
		}


		// Initialize the adjacency list
		std::memset(adjacency, uint32_t(-1), indexCount * sizeof(uint32_t));

		for (uint32_t iFace = 0; iFace < triCount; ++iFace)
		{
			uint32_t index = iFace * 3;

			for (uint32_t point = 0; point < 3; ++point)
			{
				if (adjacency[iFace * 3 + point] != uint32_t(-1))
					continue;

				// Look for edges directed in the opposite direction.
				T i0 = pointRep[indices[index + ((point + 1) % 3)]];
				T i1 = pointRep[indices[index + (point % 3)]];
				T i2 = pointRep[indices[index + ((point + 2) % 3)]];

				// Find a face sharing this edge
				uint32_t key = i0 % hashSize;

				EdgeEntry* found = nullptr;
				EdgeEntry* foundPrev = nullptr;

				for (EdgeEntry* current = hashTable[key], *prev = nullptr; current != nullptr; prev = current, current = current->Next)
				{
					if (current->i1 == i1 && current->i0 == i0)
					{
						found = current;
						foundPrev = prev;
						break;
					}
				}

				// Cache this face's normal
				Vector3 n0;
				{
					Vector3 p0 = positionReader.get(i1);
					Vector3 p1 = positionReader.get(i0);
					Vector3 p2 = positionReader.get(i2);

					Vector3 e0 = p0 - p1;
					Vector3 e1 = p1 - p2;

					n0 = GetNormal(Math::Cross(e0, e1));
				}

				// Use face normal dot product to determine best edge-sharing candidate.
				float bestDot = -2.0f;
				for (EdgeEntry* current = found, *prev = foundPrev; current != nullptr; prev = current, current = current->Next)
				{
					if (bestDot == -2.0f || (current->i1 == i1 && current->i0 == i0))
					{
						Vector3 p0 = positionReader.get(current->i0);
						Vector3 p1 = positionReader.get(current->i1);
						Vector3 p2 = positionReader.get(current->i2);

						Vector3 e0 = p0 - p1;
						Vector3 e1 = p1 - p2;

						Vector3 n1 = GetNormal(Math::Cross(e0, e1));

						float dot = Math::Dot(n0, n1);

						if (dot > bestDot)
						{
							found = current;
							foundPrev = prev;
							bestDot = dot;
						}
					}
				}

				// Update hash table and adjacency list
				if (found && found->Face != uint32_t(-1))
				{
					// Erase the found from the hash table linked list.
					if (foundPrev != nullptr)
					{
						foundPrev->Next = found->Next;
					}
					else
					{
						hashTable[key] = found->Next;
					}

					// Update adjacency information
					adjacency[iFace * 3 + point] = found->Face;

					// Search & remove this face from the table linked list
					uint32_t key2 = i1 % hashSize;

					for (EdgeEntry* current = hashTable[key2], *prev = nullptr; current != nullptr; prev = current, current = current->Next)
					{
						if (current->Face == iFace && current->i1 == i0 && current->i0 == i1)
						{
							if (prev != nullptr)
							{
								prev->Next = current->Next;
							}
							else
							{
								hashTable[key2] = current->Next;
							}

							break;
						}
					}

					bool linked = false;
					for (uint32_t point2 = 0; point2 < point; ++point2)
					{
						if (found->Face == adjacency[iFace * 3 + point2])
						{
							linked = true;
							adjacency[iFace * 3 + point] = uint32_t(-1);
							break;
						}
					}

					if (!linked)
					{
						uint32_t edge2 = 0;
						for (; edge2 < 3; ++edge2)
						{
							T k = indices[found->Face * 3 + edge2];
							if (k == uint32_t(-1))
								continue;

							if (pointRep[k] == i0)
								break;
						}

						if (edge2 < 3)
						{
							adjacency[found->Face * 3 + edge2] = iFace;
						}
					}
				}
			}
		}
	}

	bool MeshUtility::Meshletize(int maxVertices, int maxPrims, int* triIndices, int numTriangles, VertexElementReader const& positionReader, std::vector<MeshletData>& outMeshlets, std::vector<uint8>& outUniqueVertexIndices, std::vector<PackagedTriangleIndices>& outPrimitiveIndices)
	{
		using IndexType = uint32;

		std::vector<int> adjacency;

		adjacency.resize(3 * numTriangles);
		BuildAdjacencyList((uint32 const*)triIndices, 3 * numTriangles, positionReader, (uint32*)adjacency.data());

		std::vector<int> adjacencyTemp;
		BuildVertexAdjacency(positionReader, triIndices, numTriangles, adjacencyTemp);
		std::vector<int> adjacencyOld;
		adjacencyOld.resize(3 * numTriangles);
		for (int i = 0; i < adjacency.size(); ++i)
		{
			adjacencyOld[i] = adjacencyTemp[2 * i + 1];
		}

		std::vector< InlineMeshlet > meshletList;
		meshletList.emplace_back();
		auto* curr = &meshletList.back();

		// Bitmask of all triangles in mesh to determine whether a specific one has been added.
		std::vector<bool> checklist;
		checklist.resize(numTriangles);

		std::vector<Vector3> positions;
		std::vector<Vector3> normals;
		std::vector<std::pair<uint32, float>> candidates;
		std::unordered_set<uint32> candidateCheck;

		Vector4 psphere;
		Vector3 normal;

		// Arbitrarily start at triangle zero.
		uint32 triIndex = 0;
		candidates.push_back(std::make_pair(triIndex, 0.0f));
		candidateCheck.insert(triIndex);

		// Continue adding triangles until 
		while (!candidates.empty())
		{
			uint32 index = candidates.back().first;
			candidates.pop_back();

			IndexType tri[3] =
			{
				triIndices[index * 3],
				triIndices[index * 3 + 1],
				triIndices[index * 3 + 2],
			};

			assert(tri[0] < positionReader.numVertex);
			assert(tri[1] < positionReader.numVertex);
			assert(tri[2] < positionReader.numVertex);

			// Try to add triangle to meshlet
			if (AddToMeshlet(maxVertices, maxPrims, *curr, tri))
			{
				// Success! Mark as added.
				checklist[index] = true;

				// Add m_positions & normal to list
				Vector3 points[3] =
				{
					positionReader.get(tri[0]),
					positionReader.get(tri[1]),
					positionReader.get(tri[2]),
				};

				positions.push_back(points[0]);
				positions.push_back(points[1]);
				positions.push_back(points[2]);

				Vector3 normal = ComputeNormal(points);
				normals.push_back(normal);


				// Compute new bounding sphere & normal axis
				psphere = MinimumBoundingSphere(positions.data(), static_cast<uint32>(positions.size()));

				Vector4 nsphere = MinimumBoundingSphere(normals.data(), static_cast<uint32>(normals.size()));
				normal = GetNormal(nsphere.xyz());

				// Find and add all applicable adjacent triangles to candidate list
				const uint32 adjIndex = index * 3;

				int adj[3] =
				{
					adjacency[adjIndex + 1],
					adjacency[adjIndex + 2],
					adjacency[adjIndex + 0],
				};

				for (uint32 i = 0; i < 3u; ++i)
				{
					// Invalid triangle in adjacency slot
					if (adj[i] == -1)
						continue;

					// Already processed triangle
					if (checklist[adj[i]])
						continue;

					// Triangle already in the candidate list
					if (candidateCheck.count(adj[i]))
						continue;

					candidates.push_back(std::make_pair(adj[i], FLT_MAX));
					candidateCheck.insert(adj[i]);
				}

				// Re-score remaining candidate triangles
				for (uint32 i = 0; i < static_cast<uint32>(candidates.size()); ++i)
				{
					uint32 candidate = candidates[i].first;

					IndexType tri2[3] =
					{
						triIndices[candidate * 3],
						triIndices[candidate * 3 + 1],
						triIndices[candidate * 3 + 2],
					};

					assert(tri2[0] < positionReader.numVertex);
					assert(tri2[1] < positionReader.numVertex);
					assert(tri2[2] < positionReader.numVertex);

					Vector3 triVerts[3] =
					{
						positionReader.get(tri2[0]),
						positionReader.get(tri2[1]),
						positionReader.get(tri2[2]),
					};

					candidates[i].second = ComputeScore(*curr, psphere, normal, tri2, triVerts);
				}

				// Determine whether we need to move to the next meshlet.
				if (IsMeshletFull(maxVertices, maxPrims, *curr))
				{
					positions.clear();
					normals.clear();
					candidateCheck.clear();

					// Use one of our existing candidates as the next meshlet seed.
					if (!candidates.empty())
					{
						candidates[0] = candidates.back();
						candidates.resize(1);
						candidateCheck.insert(candidates[0].first);
					}

					meshletList.emplace_back();
					curr = &meshletList.back();
				}
				else
				{
					std::sort(candidates.begin(), candidates.end(),
						[](const std::pair<uint32, float>& a, const std::pair<uint32, float>& b) -> bool
						{
							return a.second > b.second;
						}
					);
				}
			}
			else
			{
				if (candidates.empty())
				{
					positions.clear();
					normals.clear();
					candidateCheck.clear();

					meshletList.emplace_back();
					curr = &meshletList.back();
				}
			}

			// Ran out of candidates; add a new seed candidate to start the next meshlet.
			if (candidates.empty())
			{
				while (triIndex < numTriangles && checklist[triIndex])
					++triIndex;

				if (triIndex == numTriangles)
					break;

				candidates.push_back(std::make_pair(triIndex, 0.0f));
				candidateCheck.insert(triIndex);
			}
		}

		// The last meshlet may have never had any primitives added to it - in which case we want to remove it.
		if (meshletList.back().primitiveIndices.empty())
		{
			meshletList.pop_back();
		}

		uint32 startVertCount = static_cast<uint32>(outUniqueVertexIndices.size()) / sizeof(IndexType);
		uint32 startPrimCount = static_cast<uint32>(outPrimitiveIndices.size());

		uint32 uniqueVertexIndexCount = startVertCount;
		uint32 primitiveIndexCount = startPrimCount;

		// Resize the meshlet output array to hold the newly formed meshlets.
		uint32 meshletCount = static_cast<uint32>(outMeshlets.size());
		outMeshlets.resize(meshletCount + meshletList.size());

		for (uint32 j = 0, dest = meshletCount; j < static_cast<uint32>(meshletList.size()); ++j, ++dest)
		{
			outMeshlets[dest].vertexOffset = uniqueVertexIndexCount;
			outMeshlets[dest].vertexCount = static_cast<uint32>(meshletList[j].uniqueVertexIndices.size());
			uniqueVertexIndexCount += static_cast<uint32>(meshletList[j].uniqueVertexIndices.size());

			outMeshlets[dest].primitveOffset = primitiveIndexCount;
			outMeshlets[dest].primitiveCount = static_cast<uint32>(meshletList[j].primitiveIndices.size());
			primitiveIndexCount += static_cast<uint32>(meshletList[j].primitiveIndices.size());
		}

		// Allocate space for the new data.
		outUniqueVertexIndices.resize(uniqueVertexIndexCount * sizeof(IndexType));
		outPrimitiveIndices.resize(primitiveIndexCount);

		// Copy data from the freshly built meshlets into the output buffers.
		auto vertDest = reinterpret_cast<IndexType*>(outUniqueVertexIndices.data()) + startVertCount;
		auto primDest = reinterpret_cast<uint32*>(outPrimitiveIndices.data()) + startPrimCount;

		for (auto const& meshLit : meshletList)
		{
			std::memcpy(vertDest, meshLit.uniqueVertexIndices.data(), meshLit.uniqueVertexIndices.size() * sizeof(IndexType));
			std::memcpy(primDest, meshLit.primitiveIndices.data(), meshLit.primitiveIndices.size() * sizeof(uint32));

			vertDest += meshLit.uniqueVertexIndices.size();
			primDest += meshLit.primitiveIndices.size();
		}

		return true;
	}

	void MeshUtility::ComputeTangent(uint8* v0, uint8* v1, uint8* v2, int texOffset, Vector3& tangent, Vector3& binormal)
	{
		Vector3& p0 = *reinterpret_cast<Vector3*>(v0);
		Vector3& p1 = *reinterpret_cast<Vector3*>(v1);
		Vector3& p2 = *reinterpret_cast<Vector3*>(v2);

		float* st0 = reinterpret_cast<float*>(v0 + texOffset);
		float* st1 = reinterpret_cast<float*>(v1 + texOffset);
		float* st2 = reinterpret_cast<float*>(v2 + texOffset);

		Vector3 d1 = p1 - p0;
		Vector3 d2 = p2 - p0;
		float s[2];
		s[0] = st1[0] - st0[0];
		s[1] = st2[0] - st0[0];
		float t[2];
		t[0] = st1[1] - st0[1];
		t[1] = st2[1] - st0[1];

		float factor = 1.0f / (s[0] * t[1] - s[1] * t[0]);

		tangent = Math::GetNormal(factor * (t[1] * d1 - t[0] * d2));
		binormal = Math::GetNormal(factor * (s[0] * d2 - s[1] * d1));
	}

	void MeshUtility::FillNormal_TriangleList(InputLayoutDesc const& desc, void* pVertex, int nV, int* idx, int nIdx, int normalAttrib /*= Vertex::ATTRIBUTE_NORMAL*/)
	{
		assert(desc.getAttributeFormat(Vertex::ATTRIBUTE_POSITION) == Vertex::eFloat3);
		assert(desc.getAttributeFormat(normalAttrib) == Vertex::eFloat3);

		int posOffset = desc.getAttributeOffset(Vertex::ATTRIBUTE_POSITION);
		int normalOffset = desc.getAttributeOffset(normalAttrib) - posOffset;
		uint8* pV = (uint8*)(pVertex)+posOffset;

		int numEle = nIdx / 3;
		int vertexSize = desc.getVertexSize();
		int* pCur = idx;

		for (int i = 0; i < numEle; ++i)
		{
			int i1 = pCur[0];
			int i2 = pCur[1];
			int i3 = pCur[2];
			pCur += 3;
			uint8* v1 = pV + i1 * vertexSize;
			uint8* v2 = pV + i2 * vertexSize;
			uint8* v3 = pV + i3 * vertexSize;

			Vector3& p1 = *reinterpret_cast<Vector3*>(v1);
			Vector3& p2 = *reinterpret_cast<Vector3*>(v2);
			Vector3& p3 = *reinterpret_cast<Vector3*>(v3);

			Vector3 normal = (p2 - p1).cross(p3 - p1);
			normal.normalize();
			*reinterpret_cast<Vector3*>(v1 + normalOffset) += normal;
			*reinterpret_cast<Vector3*>(v2 + normalOffset) += normal;
			*reinterpret_cast<Vector3*>(v3 + normalOffset) += normal;
		}

		for (int i = 0; i < nV; ++i)
		{
			uint8* v = pV + i * vertexSize;
			Vector3& normal = *reinterpret_cast<Vector3*>(v + normalOffset);
			normal.normalize();
		}
	}

	void MeshUtility::FillNormalTangent_TriangleList(InputLayoutDesc const& desc, void* pVertex, int nV, int* idx, int nIdx)
	{
		assert(desc.getAttributeFormat(Vertex::ATTRIBUTE_POSITION) == Vertex::eFloat3);
		assert(desc.getAttributeFormat(Vertex::ATTRIBUTE_NORMAL) == Vertex::eFloat3);
		assert(desc.getAttributeFormat(Vertex::ATTRIBUTE_TEXCOORD) == Vertex::eFloat2);
		assert(desc.getAttributeFormat(Vertex::ATTRIBUTE_TANGENT) == Vertex::eFloat4);

		int posOffset = desc.getAttributeOffset(Vertex::ATTRIBUTE_POSITION);
		int texOffset = desc.getAttributeOffset(Vertex::ATTRIBUTE_TEXCOORD) - posOffset;
		int tangentOffset = desc.getAttributeOffset(Vertex::ATTRIBUTE_TANGENT) - posOffset;
		int normalOffset = desc.getAttributeOffset(Vertex::ATTRIBUTE_NORMAL) - posOffset;
		uint8* pV = (uint8*)(pVertex)+posOffset;

		int numEle = nIdx / 3;
		int vertexSize = desc.getVertexSize();
		int* pCur = idx;
		std::vector< Vector3 > binormals(nV, Vector3(0, 0, 0));

		for (int i = 0; i < numEle; ++i)
		{
			int i1 = pCur[0];
			int i2 = pCur[1];
			int i3 = pCur[2];
			pCur += 3;
			uint8* v1 = pV + i1 * vertexSize;
			uint8* v2 = pV + i2 * vertexSize;
			uint8* v3 = pV + i3 * vertexSize;

			Vector3& p1 = *reinterpret_cast<Vector3*>(v1);
			Vector3& p2 = *reinterpret_cast<Vector3*>(v2);
			Vector3& p3 = *reinterpret_cast<Vector3*>(v3);

			Vector3 normal = (p2 - p1).cross(p3 - p1);
			normal.normalize();
			Vector3 tangent, binormal;
			ComputeTangent(v1, v2, v3, texOffset, tangent, binormal);

			*reinterpret_cast<Vector3*>(v1 + tangentOffset) += tangent;
			*reinterpret_cast<Vector3*>(v1 + normalOffset) += normal;
			binormals[i1] += binormal;

			*reinterpret_cast<Vector3*>(v2 + tangentOffset) += tangent;
			*reinterpret_cast<Vector3*>(v2 + normalOffset) += normal;
			binormals[i2] += binormal;

			*reinterpret_cast<Vector3*>(v3 + tangentOffset) += tangent;
			*reinterpret_cast<Vector3*>(v3 + normalOffset) += normal;
			binormals[i3] += binormal;
		}

		for (int i = 0; i < nV; ++i)
		{
			uint8* v = pV + i * vertexSize;
			Vector3& tangent = *reinterpret_cast<Vector3*>(v + tangentOffset);
			Vector3& normal = *reinterpret_cast<Vector3*>(v + normalOffset);

			normal.normalize();
			tangent = tangent - normal * normal.dot(tangent);
			tangent.normalize();
			if (normal.dot(tangent.cross(binormals[i])) > 0)
			{
				tangent[3] = 1;
			}
			else
			{
				tangent[3] = -1;
			}
		}
	}

	void MeshUtility::FillTangent_TriangleList(InputLayoutDesc const& desc, void* pVertex, int nV, int* idx, int nIdx)
	{
		assert(desc.getAttributeFormat(Vertex::ATTRIBUTE_POSITION) == Vertex::eFloat3);
		assert(desc.getAttributeFormat(Vertex::ATTRIBUTE_NORMAL) == Vertex::eFloat3);
		assert(desc.getAttributeFormat(Vertex::ATTRIBUTE_TEXCOORD) == Vertex::eFloat2);
		assert(desc.getAttributeFormat(Vertex::ATTRIBUTE_TANGENT) == Vertex::eFloat4);

		int posOffset = desc.getAttributeOffset(Vertex::ATTRIBUTE_POSITION);
		int texOffset = desc.getAttributeOffset(Vertex::ATTRIBUTE_TEXCOORD) - posOffset;
		int tangentOffset = desc.getAttributeOffset(Vertex::ATTRIBUTE_TANGENT) - posOffset;
		int normalOffset = desc.getAttributeOffset(Vertex::ATTRIBUTE_NORMAL) - posOffset;
		uint8* pV = (uint8*)(pVertex)+posOffset;

		int numEle = nIdx / 3;
		int vertexSize = desc.getVertexSize();
		int* pCur = idx;
		std::vector< Vector3 > binormals(nV, Vector3(0, 0, 0));

		for (int i = 0; i < numEle; ++i)
		{
			int i1 = pCur[0];
			int i2 = pCur[1];
			int i3 = pCur[2];
			pCur += 3;
			uint8* v1 = pV + i1 * vertexSize;
			uint8* v2 = pV + i2 * vertexSize;
			uint8* v3 = pV + i3 * vertexSize;

			Vector3 tangent, binormal;
			ComputeTangent(v1, v2, v3, texOffset, tangent, binormal);

			*reinterpret_cast<Vector3*>(v1 + tangentOffset) += tangent;
			binormals[i1] += binormal;

			*reinterpret_cast<Vector3*>(v2 + tangentOffset) += tangent;
			binormals[i2] += binormal;

			*reinterpret_cast<Vector3*>(v3 + tangentOffset) += tangent;
			binormals[i3] += binormal;
		}

		for (int i = 0; i < nV; ++i)
		{
			uint8* v = pV + i * vertexSize;
			Vector3& tangent = *reinterpret_cast<Vector3*>(v + tangentOffset);
			Vector3& normal = *reinterpret_cast<Vector3*>(v + normalOffset);

			tangent = tangent - normal * (normal.dot(tangent) / normal.length2());
			tangent.normalize();
			if (normal.dot(tangent.cross(binormals[i])) > 0)
			{
				tangent[3] = 1;
			}
			else
			{
				tangent[3] = -1;
			}
		}
	}

	void MeshUtility::FillTangent_QuadList(InputLayoutDesc const& desc, void* pVertex, int nV, int* idx, int nIdx)
	{
		int numEle = nIdx / 4;
		std::vector< int > indices(numEle * 6);
		int* src = idx;
		int* dest = &indices[0];
		for (int i = 0; i < numEle; ++i)
		{
			dest[0] = src[0]; dest[1] = src[1]; dest[2] = src[2];
			dest[3] = src[2]; dest[4] = src[3]; dest[5] = src[0];
			dest += 6;
			src += 4;
		}
		FillTangent_TriangleList(desc, pVertex, nV, &indices[0], indices.size());
	}

	template< class IndexType >
	static int* MeshUtility::ConvertToTriangleListIndices(EPrimitive type, IndexType* data, int numData, std::vector< int >& outConvertBuffer, int& outNumTriangle)
	{
		outNumTriangle = 0;

		switch (type)
		{
		case EPrimitive::TriangleList:
			{
				int numElements = numData / 3;
				if (numElements <= 0)
					return nullptr;

				outNumTriangle = numElements;
				if (sizeof(IndexType) != sizeof(int))
				{
					outConvertBuffer.resize(numData);
					std::copy(data, data + numData, &outConvertBuffer[0]);
					return &outConvertBuffer[0];
				}
				return (int*)data;
			}
			break;
		case EPrimitive::TriangleStrip:
			{
				int numElements = numData - 2;
				if (numElements <= 0)
					return nullptr;

				outNumTriangle = numElements;
				outConvertBuffer.resize(3 * outNumTriangle);
				IndexType* src = data;
				int* dest = &outConvertBuffer[0];
				int idx0 = src[0];
				int idx1 = src[1];
				src += 2;
				for (int i = 0; i < numElements; ++i)
				{
					int idxCur = src[0];
					if (i & 1)
					{
						dest[0] = idx0; dest[1] = idx1; dest[2] = idxCur;
					}
					else
					{
						dest[0] = idx1; dest[1] = idx0; dest[2] = idxCur;
					}
					idx0 = idx1;
					idx1 = idxCur;
					dest += 3;
					src += 1;
				}
			}
			break;
		case EPrimitive::TriangleAdjacency:
			{
				int numElements = numData / 6;
				if (numElements <= 0)
					return nullptr;

				outNumTriangle = numElements;
				outConvertBuffer.resize(3 * outNumTriangle);
				IndexType* src = data;
				int* dest = &outConvertBuffer[0];
				for (int i = 0; i < numElements; ++i)
				{
					dest[0] = src[0]; dest[1] = src[2]; dest[2] = src[4];
					dest += 3;
					src += 6;
				}
			}
			break;
		case EPrimitive::TriangleFan:
			{
				int numElements = numData - 2;
				if (numElements <= 0)
					return nullptr;

				outNumTriangle = numElements;
				outConvertBuffer.resize(3 * outNumTriangle);
				IndexType* src = data;
				int* dest = &outConvertBuffer[0];
				int idxStart = src[0];
				int idx1 = src[0];
				src += 2;
				for (int i = 0; i < numElements; ++i)
				{
					int idxCur = src[0];
					dest[0] = idxStart; dest[1] = idx1; dest[2] = idxCur;
					idx1 = idxCur;
					dest += 3;
					src += 1;
				}
			}
			break;
		case EPrimitive::Quad:
			{
				int numElements = numData / 4;
				if (numElements <= 0)
					return nullptr;

				outNumTriangle = 2 * numElements;
				outConvertBuffer.resize(3 * outNumTriangle);
				IndexType* src = data;
				int* dest = &outConvertBuffer[0];
				for (int i = 0; i < numElements; ++i)
				{
					dest[0] = src[0]; dest[1] = src[1]; dest[2] = src[2];
					dest[3] = src[2]; dest[4] = src[3]; dest[5] = src[0];
					dest += 6;
					src += 4;
				}
				return &outConvertBuffer[0];
			}
			break;
		case EPrimitive::Polygon:
			{
				if (numData < 3)
					return nullptr;

				outNumTriangle = numData - 2;
				outConvertBuffer.resize(3 * outNumTriangle);
				IndexType* src = data;
				int* dest = &outConvertBuffer[0];
				for (int i = 2; i < numData; ++i)
				{
					dest[0] = src[0];
					dest[1] = src[i - 1];
					dest[2] = src[i];
					dest += 3;
				}
				return &outConvertBuffer[0];
			}
			break;
		}
		return nullptr;
	}

	template int* MeshUtility::ConvertToTriangleListIndices<int32>(EPrimitive type, int32* data, int numData, std::vector< int >& outConvertBuffer, int& outNumTriangle);
	template int* MeshUtility::ConvertToTriangleListIndices<int16>(EPrimitive type, int16* data, int numData, std::vector< int >& outConvertBuffer, int& outNumTriangle);

}//namespace Render