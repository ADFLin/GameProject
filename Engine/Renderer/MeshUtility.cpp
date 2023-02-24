#include "MeshUtility.h"

#include "RHI/RHICommand.h"
#include "Renderer/Mesh.h"

#include "WindowsHeader.h"
#include "MarcoCommon.h"
#include "InlineString.h"
#include "FileSystem.h"
#include "LogSystem.h"

#include "Core/ScopeGuard.h"
#include "Core/Memory.h"
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


#define DF_BUILD_USE_KDTREE 1

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

		TArray<int>            uniqueVertexIndices;
		TArray<PackedTriangle> primitiveIndices;
	};


	struct EdgeEntry
	{
		uint32   i0;
		uint32   i1;
		uint32   i2;

		uint32   Face;
		EdgeEntry* Next;
	};

	uint32 CRCHash(const uint32* dwords, uint32 dwordCount)
	{
		uint32 h = 0;

		for (uint32 i = 0; i < dwordCount; ++i)
		{
			uint32 highOrd = h & 0xf8000000;
			h = h << 5;
			h = h ^ (highOrd >> 27);
			h = h ^ uint32(dwords[i]);
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
	template <> struct hash< Math::Vector3 > { size_t operator()(const Math::Vector3& v) const { return CRCHash(reinterpret_cast<const uint32*>(&v), sizeof(v) / 4); } };
}

namespace Render
{
	bool VerifyOpenGLStatus();
	bool gbOptimizeVertexCache = false;


	uint32* MeshUtility::ConvertToTriangleList(EPrimitive type, void* pIndexData , int numIndices, bool bIntType , TArray< uint32 >& outConvertBuffer, int& outNumTriangles)
	{
		if( bIntType )
		{
			return ConvertToTriangleListIndices<uint32>(type, (uint32*)pIndexData, numIndices, outConvertBuffer, outNumTriangles);
		}
		else
		{
			return ConvertToTriangleListIndices<uint16>(type, (uint16*)pIndexData, numIndices, outConvertBuffer, outNumTriangles);
		}
	}


	void MeshUtility::CalcAABB(VertexElementReader const& positionReader, int numVertices, Vector3& outMin, Vector3& outMax)
	{
		assert(numVertices >= 1);
		outMax = outMin = positionReader.get(0);
		for( int i = 1; i < numVertices; ++i )
		{
			Vector3 const& pos = positionReader.get(i);
			outMax.setMax(pos);
			outMin.setMin(pos);
		}
	}

	bool MeshUtility::BuildDistanceField(Mesh& mesh, DistanceFieldBuildSetting const& setting, DistanceFieldData& outResult)
	{
		if( !mesh.mIndexBuffer.isValid() || !mesh.mVertexBuffer.isValid() )
			return false;

		uint8* pData = (uint8*)( RHILockBuffer( mesh.mVertexBuffer , ELockAccess::ReadOnly)) + mesh.mInputLayoutDesc.getAttributeOffset(EVertex::ATTRIBUTE_POSITION);
		void* pIndexBufferData = RHILockBuffer( mesh.mIndexBuffer , ELockAccess::ReadOnly);
		ON_SCOPE_EXIT
		{
			RHIUnlockBuffer(mesh.mVertexBuffer);
			RHIUnlockBuffer(mesh.mIndexBuffer);
		};
		TArray<uint32> tempBuffer;
		int numTriangles;
		uint32* pIndexData = ConvertToTriangleList(mesh.mType, pIndexBufferData, mesh.mIndexBuffer->getNumElements() , IsIntType(mesh.mIndexBuffer), tempBuffer, numTriangles);
		bool result = false;
		if ( pIndexData )
		{
			result = BuildDistanceField(mesh.makePositionReader(pData), mesh.getVertexCount(), pIndexData, numTriangles, setting, outResult);
		}

		return result;
	}

	bool MeshUtility::BuildDistanceField(VertexElementReader const& positionReader, int numVertices, uint32* pIndexData, int numTriangles, DistanceFieldBuildSetting const& setting, DistanceFieldData& outResult)
	{

		Vector3 boundMin;
		Vector3 boundMax;
		CalcAABB(positionReader, numVertices, boundMin, boundMax);

		boundMax *= setting.boundSizeScale;
		boundMin *= setting.boundSizeScale;
		Vector3 boundSize = boundMax - boundMin;

		float boundMaxDistanceSqr = boundSize.length2();

		Vector3 gridSizeFloat = setting.resolutionScale * boundSize / setting.gridLength;
		IntVector3 gridSize = IntVector3(Math::CeilToInt(gridSizeFloat.x), Math::CeilToInt(gridSizeFloat.y), Math::CeilToInt(gridSizeFloat.z));
		gridSize = Math::Clamp(gridSize, IntVector3(2, 2, 2), IntVector3(128, 128, 128));
		Vector3 gridLength = (boundSize).div(Vector3(gridSize.x, gridSize.y, gridSize.z));
		outResult.volumeData.resize(gridSize.x * gridSize.y * gridSize.z);

#if DF_BUILD_USE_KDTREE
		using MyTree = TKDTree<3>;
		MyTree tree;
		{
			uint32* pIndex = pIndexData;
			for( int i = 0; i < numTriangles; ++i )
			{
				uint32 idx0 = pIndex[0];
				uint32 idx1 = pIndex[1];
				uint32 idx2 = pIndex[2];

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

#if DF_BUILD_USE_KDTREE
					int count = 0;
					float side = 1.0;
					auto DistFunc = [&](MyTree::PrimitiveData const& data, Vector3 const& pos, float minDistSqr)
					{
						++count;
						uint32* pIndex = pIndexData + 3 * data.id;
						uint32 idx0 = pIndex[0];
						uint32 idx1 = pIndex[1];
						uint32 idx2 = pIndex[2];

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
		if( setting.workTaskPool == nullptr )
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

			TArray< std::unique_ptr< MyTask > > allTasks;
			for( int nz = 0; nz < gridSize.z; ++nz )
			{
				auto task = std::make_unique<MyTask>( TaskFunc , boundMaxDistanceSqr , nz );
				setting.workTaskPool->addWork(task.get());
				allTasks.push_back(std::move(task));
			}

			setting.workTaskPool->waitAllWorkComplete();
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

	void MeshUtility::BuildTessellationAdjacency(VertexElementReader const& positionReader, uint32* triIndices, int numTirangle, TArray<int>& outResult)
	{
		class MyRenderBuffer : public nv::RenderBuffer
		{
		public:
			MyRenderBuffer(VertexElementReader const& positionReader, uint32* triIndices, int numTirangle , VertexElementReader const* texcoordReader = nullptr)
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

	void MeshUtility::BuildVertexAdjacency(VertexElementReader const& positionReader, int numVertices, uint32* triIndices, int numTirangle, TArray<int>& outResult)
	{
		TArray< int > triangleIndexMap;
		struct Vertex
		{
			int   index;
			float z;
		};
		TArray< Vertex > sortedVertices;
		sortedVertices.resize(numVertices);
		for( int i = 0; i < numVertices; ++i )
		{
			Vector3 const& pos = positionReader.get(i);
			sortedVertices[i].index = i;
			sortedVertices[i].z = pos.z;
		}

		std::sort(sortedVertices.begin(), sortedVertices.end(), [](auto const& a, auto const& b)
		{
			return a.z < b.z;
		});
		triangleIndexMap.resize(numVertices, -1);
		for( int i = 0; i < numVertices; ++i )
		{
			Vertex const& vi = sortedVertices[i];
			if( triangleIndexMap[vi.index] != -1 )
				continue;

			triangleIndexMap[vi.index] = vi.index;
			Vector3 const& pos0 = positionReader.get(vi.index);
			for( int j = i + 1; j < numVertices; ++j )
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
			TArray<uint32> tempIndices{ (uint16*)pIndices , (uint16*)(pIndices)+numIndex };
			NvTriStrip::GenerateStrips((uint32*)&tempIndices[0], numIndex, &groups, &numGroup);
		}

		if( bIntType )
		{
			uint32* pData = (uint32*)pIndices;
			for( int i = 0; i < numGroup; ++i )
			{
				NvTriStrip::PrimitiveGroup& group = groups[i];
				assert(group.type == NvTriStrip::PT_LIST);
				FMemory::Copy(pData, group.indices, group.numIndices * sizeof(uint32));
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

	void MeshUtility::FillTriangleListNormalAndTangent(InputLayoutDesc const& desc, void* pVertex, int nV, uint32* idx, int nIdx)
	{
		FillNormalTangent_TriangleList(desc, pVertex, nV, idx, nIdx);
	}

	void MeshUtility::FillTriangleListTangent(InputLayoutDesc const& desc, void* pVertex, int nV, uint32* idx, int nIdx)
	{
		FillTangent_TriangleList(desc, pVertex, nV, idx, nIdx);
	}

	void MeshUtility::FillTriangleListNormal(InputLayoutDesc const& desc, void* pVertex, int nV, uint32* idx, int nIdx, int normalAttrib, bool bNeedClear)
	{
		FillNormal_TriangleList(desc, pVertex, nV, idx, nIdx, normalAttrib, bNeedClear);
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
	void BuildAdjacencyList(const T* indices, uint32 indexCount, VertexElementReader const& positionReader, int numVertices, uint32* adjacency )
	{
		const uint32 triCount = indexCount / 3;
		// Find point reps (unique positions) in the position stream
		// Create a mapping of non-unique vertex indices to point reps
		TArray<T> pointRep;
		pointRep.resize(numVertices);

		std::unordered_map<size_t, T> uniquePositionMap;
		uniquePositionMap.reserve(numVertices);

		for (uint32 i = 0; i < numVertices; ++i)
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
		const uint32 hashSize = numVertices / 3;

		std::unique_ptr<EdgeEntry*[]> hashTable(new EdgeEntry*[hashSize]);
		std::unique_ptr<EdgeEntry[]> entries(new EdgeEntry[triCount * 3]);

		std::memset(hashTable.get(), 0, sizeof(EdgeEntry*) * hashSize);
		uint32 entryIndex = 0;

		for (uint32 iFace = 0; iFace < triCount; ++iFace)
		{
			uint32 index = iFace * 3;

			// Create a hash entry in the hash table for each each.
			for (uint32 iEdge = 0; iEdge < 3; ++iEdge)
			{
				T i0 = pointRep[indices[index + (iEdge % 3)]];
				T i1 = pointRep[indices[index + ((iEdge + 1) % 3)]];
				T i2 = pointRep[indices[index + ((iEdge + 2) % 3)]];

				auto& entry = entries[entryIndex++];
				entry.i0 = i0;
				entry.i1 = i1;
				entry.i2 = i2;

				uint32 key = entry.i0 % hashSize;

				entry.Next = hashTable[key];
				entry.Face = iFace;

				hashTable[key] = &entry;
			}
		}


		// Initialize the adjacency list
		std::memset(adjacency, uint32(-1), indexCount * sizeof(uint32));

		for (uint32 iFace = 0; iFace < triCount; ++iFace)
		{
			uint32 index = iFace * 3;

			for (uint32 point = 0; point < 3; ++point)
			{
				if (adjacency[iFace * 3 + point] != uint32(-1))
					continue;

				// Look for edges directed in the opposite direction.
				T i0 = pointRep[indices[index + ((point + 1) % 3)]];
				T i1 = pointRep[indices[index + (point % 3)]];
				T i2 = pointRep[indices[index + ((point + 2) % 3)]];

				// Find a face sharing this edge
				uint32 key = i0 % hashSize;

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
				if (found && found->Face != uint32(-1))
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
					uint32 key2 = i1 % hashSize;

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
					for (uint32 point2 = 0; point2 < point; ++point2)
					{
						if (found->Face == adjacency[iFace * 3 + point2])
						{
							linked = true;
							adjacency[iFace * 3 + point] = uint32(-1);
							break;
						}
					}

					if (!linked)
					{
						uint32 edge2 = 0;
						for (; edge2 < 3; ++edge2)
						{
							T k = indices[found->Face * 3 + edge2];
							if (k == uint32(-1))
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

	bool MeshletizeInternal(int maxVertices, int maxPrims, uint32* triIndices, int numTriangles, VertexElementReader const& positionReader, int numVertices, TArray< InlineMeshlet >& outMeshletList)
	{

		using IndexType = uint32;

		TArray<uint32> adjacency;
		adjacency.resize(3 * numTriangles);
		BuildAdjacencyList((uint32 const*)triIndices, 3 * numTriangles, positionReader, numVertices, adjacency.data());

		outMeshletList.emplace_back();
		auto* curr = &outMeshletList.back();

		// Bitmask of all triangles in mesh to determine whether a specific one has been added.
		TArray<bool> checklist;
		checklist.resize(numTriangles);

		TArray<Vector3> positions;
		TArray<Vector3> normals;
		TArray<std::pair<uint32, float>> candidates;
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

			CHECK(tri[0] < numVertices);
			CHECK(tri[1] < numVertices);
			CHECK(tri[2] < numVertices);

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

					CHECK(tri2[0] < numVertices);
					CHECK(tri2[1] < numVertices);
					CHECK(tri2[2] < numVertices);

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

					outMeshletList.emplace_back();
					curr = &outMeshletList.back();
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

					outMeshletList.emplace_back();
					curr = &outMeshletList.back();
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
		if (outMeshletList.back().primitiveIndices.empty())
		{
			outMeshletList.pop_back();
		}

		return true;
	}

	void ApplyMeshletData(TArray< InlineMeshlet > const& meshletList, TArray<MeshletData>& outMeshlets, TArray<uint8>& outUniqueVertexIndices, TArray<PackagedTriangleIndices>& outPrimitiveIndices)
	{
		using IndexType = uint32;

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

		for (auto const& meshlet : meshletList)
		{
			FMemory::Copy(vertDest, meshlet.uniqueVertexIndices.data(), meshlet.uniqueVertexIndices.size() * sizeof(IndexType));
			FMemory::Copy(primDest, meshlet.primitiveIndices.data(), meshlet.primitiveIndices.size() * sizeof(uint32));

			vertDest += meshlet.uniqueVertexIndices.size();
			primDest += meshlet.primitiveIndices.size();
		}
	}

	bool MeshUtility::Meshletize(int maxVertices, int maxPrims, uint32* triIndices, int numTriangles, VertexElementReader const& positionReader, int numVertices, TArray<MeshletData>& outMeshlets, TArray<uint8>& outUniqueVertexIndices, TArray<PackagedTriangleIndices>& outPrimitiveIndices)
	{
		using IndexType = uint32;

		TArray< InlineMeshlet > meshletList;
		MeshletizeInternal(maxVertices, maxPrims, triIndices, numTriangles, positionReader, numVertices, meshletList);
		ApplyMeshletData(meshletList, outMeshlets, outUniqueVertexIndices, outPrimitiveIndices);

		return true;
	}


	bool MeshUtility::Meshletize(int maxVertices, int maxPrims, uint32* triIndices, int numTriangles, VertexElementReader const& positionReader, int numVertices, TArrayView< MeshSection const > sections, TArray<MeshletData>& outMeshlets, TArray<uint8>& outUniqueVertexIndices, TArray<PackagedTriangleIndices>& outPrimitiveIndices, TArray< MeshSection >& outSections)
	{
		for (int indexSection = 0; indexSection < sections.size(); ++indexSection)
		{
			MeshSection const& section = sections[indexSection];

			TArray< InlineMeshlet > meshletList;
			MeshletizeInternal(maxVertices, maxPrims, triIndices + section.indexStart, section.count / 3, positionReader, numVertices, meshletList);
			ApplyMeshletData(meshletList, outMeshlets, outUniqueVertexIndices, outPrimitiveIndices);

			MeshSection outSection;
			outSection.count = meshletList.size();
			outSection.indexStart = outMeshlets.size();
			outSections.push_back(outSection);
		}

		return true;
	}

	Vector3 Clamp01(Vector3 const& v)
	{
		return Vector3(
			Math::Clamp<float>(v.x, 0, 1),
			Math::Clamp<float>(v.y, 0, 1),
			Math::Clamp<float>(v.z, 0, 1));
	}

	inline Vector3 QuantizeSNorm(Vector3 const& value)
	{
		return (Clamp01(value) * 0.5f + Vector3(0.5f)) * 255.0f;
	}

	inline Vector3 QuantizeUNorm(Vector3 const& value)
	{
		return Clamp01(value) * 255.0f;
	}


	void MeshUtility::GenerateCullData(VertexElementReader const& positionReader, int numVertices, MeshletData const* meshlets, int meshletCount, uint32 const* uniqueVertexIndices, const PackagedTriangleIndices* primitiveIndices, MeshletCullData* cullData)
	{
#define CNORM_WIND_CW 0x4

		DWORD flags = 0;

		Vector3 vertices[256];
		Vector3 normals[256];

		for (uint32 mi = 0; mi < meshletCount; ++mi)
		{
			auto& m = meshlets[mi];
			auto& c = cullData[mi];

			// Cache vertices
			for (uint32 i = 0; i < m.vertexCount; ++i)
			{
				uint32 vIndex = uniqueVertexIndices[m.vertexOffset + i];

				CHECK(vIndex < numVertices);
				vertices[i] = positionReader[vIndex];
			}

			// Generate primitive normals & cache
			for (uint32 i = 0; i < m.primitiveCount; ++i)
			{
				auto primitive = primitiveIndices[m.primitveOffset + i];

				Vector3 triangle[3]
				{
					vertices[primitive.i0],
					vertices[primitive.i1],
					vertices[primitive.i2],
				};

				Vector3 p10 = triangle[1] - triangle[0];
				Vector3 p20 = triangle[2] - triangle[0];
				Vector3 n = GetNormal(Math::Cross(p10, p20));

				normals[i] = (flags & CNORM_WIND_CW) ? -n : n;
			}

			// Calculate spatial bounds
			Vector4 positionBounds = MinimumBoundingSphere(vertices, m.vertexCount);
			c.boundingSphere = positionBounds;

			// Calculate the normal cone
			// 1. Normalized center point of minimum bounding sphere of unit normals == conic axis
			Vector4 normalBounds = MinimumBoundingSphere(normals, m.primitiveCount);

			// 2. Calculate dot product of all normals to conic axis, selecting minimum
			Vector3 axis = GetNormal(normalBounds.xyz());

			float minDot = 1;
			for (uint32 i = 0; i < m.primitiveCount; ++i)
			{
				float dot = Math::Dot(axis, normals[i]);
				minDot = Math::Min(minDot, dot);
			}

			if (minDot < 0.1f)
			{
				// Degenerate cone
#if MESH_USE_PACKED_CONE
				c.normalCone[0] = 127;
				c.normalCone[1] = 127;
				c.normalCone[2] = 127;
				c.normalCone[3] = 255;
#else
				c.normalCone[0] = 0;
				c.normalCone[1] = 0;
				c.normalCone[2] = 0;
				c.normalCone[3] = 1;

#endif
				continue;
			}

			// Find the point on center-t*axis ray that lies in negative half-space of all triangles
			float maxt = 0;

			for (uint32 i = 0; i < m.primitiveCount; ++i)
			{
				auto primitive = primitiveIndices[m.primitveOffset + i];

				uint32 indices[3]
				{
					primitive.i0,
					primitive.i1,
					primitive.i2,
				};

				Vector3 triangle[3]
				{
					vertices[indices[0]],
					vertices[indices[1]],
					vertices[indices[2]],
				};

				Vector3 c = positionBounds.xyz() - triangle[0];

				Vector3 n = normals[i];
				float dc = Math::Dot(c, n);
				float dn = Math::Dot(axis, n);

				// dn should be larger than mindp cutoff above
				CHECK(dn > 0.0f);
				float t = dc / dn;

				maxt = (t > maxt) ? t : maxt;
			}

			// cone apex should be in the negative half-space of all cluster triangles by construction
			c.ApexOffset = maxt;

			// cos(a) for normal cone is minDot; we need to add 90 degrees on both sides and invert the cone
			// which gives us -cos(a+90) = -(-sin(a)) = sin(a) = sqrt(1 - cos^2(a))
			float coneCutoff = Math::Sqrt(1 - minDot * minDot);

#if MESH_USE_PACKED_CONE

			// 3. Quantize to uint8
			Vector3 quantized = QuantizeSNorm(axis);
			c.normalCone[0] = (uint8)quantized.x;
			c.normalCone[1] = (uint8)quantized.y;
			c.normalCone[2] = (uint8)quantized.z;

			Vector3 error = ((quantized / 255.0f) * 2.0f - Vector3(1)) - axis;
			float totalError = Math::Abs(error.x) + Math::Abs(error.y) + Math::Abs(error.z);

			quantized = QuantizeUNorm(Vector3( coneCutoff + totalError ));
			quantized.x = Math::Min(quantized.x + 1, 255.0f);
			c.normalCone[3] = (uint8_t)quantized.x;

#else
			c.normalCone[0] = axis.x;
			c.normalCone[1] = axis.y;
			c.normalCone[2] = axis.z;
			c.normalCone[3] = coneCutoff;
#endif
		}

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

	void MeshUtility::ComputeTangent(Vector3 const& v0, Vector2 const& uv0, Vector3 const& v1, Vector2 const& uv1, Vector3 const& v2, Vector2 const& uv2, Vector3& tangent, Vector3& binormal)
	{
		Vector3 d1 = v1 - v0;
		Vector3 d2 = v2 - v0;
		float s[2];
		s[0] = uv1[0] - uv0[0];
		s[1] = uv2[0] - uv0[0];
		float t[2];
		t[0] = uv1[1] - uv0[1];
		t[1] = uv2[1] - uv0[1];

		float factor = 1.0f / (s[0] * t[1] - s[1] * t[0]);

		tangent = Math::GetNormal(factor * (t[1] * d1 - t[0] * d2));
		binormal = Math::GetNormal(factor * (s[0] * d2 - s[1] * d1));
	}

	VertexElementReader MakeReader(InputLayoutDesc const& desc, void const* pData, EVertex::Attribute attribute)
	{
		VertexElementReader result;
		auto element = desc.findElementByAttribute(attribute);
		CHECK(element);
		result.pVertexData = static_cast<uint8 const*>(pData) + element->offset;
		result.vertexDataStride = desc.getVertexSize(element->streamIndex);
		return result;
	}
	VertexElementWriter MakeWriter(InputLayoutDesc const& desc, void* pData, EVertex::Attribute attribute)
	{
		VertexElementWriter result;
		auto element = desc.findElementByAttribute(attribute);
		CHECK(element);
		result.pVertexData = static_cast<uint8*>(pData) + element->offset;
		result.vertexDataStride = desc.getVertexSize(element->streamIndex);
		return result;
	}

	void MeshUtility::FillNormal_TriangleList(InputLayoutDesc const& desc, void* pVertex, int numVerteices, uint32* indices, int numIndices, int normalAttrib, bool bNeedClear)
	{
		CHECK(desc.getAttributeFormat(EVertex::ATTRIBUTE_POSITION) == EVertex::Float3);
		CHECK(desc.getAttributeFormat(normalAttrib) == EVertex::Float3);

		FillNormal_TriangleList(MakeReader(desc, pVertex, EVertex::ATTRIBUTE_POSITION), MakeWriter(desc, pVertex, EVertex::Attribute (normalAttrib)), numVerteices, indices, numIndices, bNeedClear);
	}

	void MeshUtility::FillNormal_TriangleList(VertexElementReader const& positionReader , VertexElementWriter& normalWriter, int numVerteices, uint32* indices, int numIndices , bool bNeedClear)
	{
		int numTriangles = numIndices / 3;

		if ( bNeedClear )
		{
			for (int i = 0; i < numVerteices; ++i)
			{
				normalWriter[i] = Vector3::Zero();
			}
		}

		uint32* pCur = indices;
		for (int i = 0; i < numTriangles; ++i)
		{
			uint32 i0 = pCur[0];
			uint32 i1 = pCur[1];
			uint32 i2 = pCur[2];
			pCur += 3;

			Vector3 const& p0 = positionReader[i0];
			Vector3 const& p1 = positionReader[i1];
			Vector3 const& p2 = positionReader[i2];

			Vector3 normal = (p1 - p0).cross(p2 - p0);
			normal.normalize();

			normalWriter[i0] += normal;
			normalWriter[i1] += normal;
			normalWriter[i2] += normal;
		}

		for (int i = 0; i < numVerteices; ++i)
		{
			normalWriter[i].normalize();
		}
	}

	void MeshUtility::FillNormalTangent_TriangleList(InputLayoutDesc const& desc, void* pVertex, int nV, uint32* idx, int nIdx)
	{
		assert(desc.getAttributeFormat(EVertex::ATTRIBUTE_POSITION) == EVertex::Float3);
		assert(desc.getAttributeFormat(EVertex::ATTRIBUTE_NORMAL) == EVertex::Float3);
		assert(desc.getAttributeFormat(EVertex::ATTRIBUTE_TEXCOORD) == EVertex::Float2);
		assert(desc.getAttributeFormat(EVertex::ATTRIBUTE_TANGENT) == EVertex::Float4);

		int posOffset = desc.getAttributeOffset(EVertex::ATTRIBUTE_POSITION);
		int texOffset = desc.getAttributeOffset(EVertex::ATTRIBUTE_TEXCOORD) - posOffset;
		int tangentOffset = desc.getAttributeOffset(EVertex::ATTRIBUTE_TANGENT) - posOffset;
		int normalOffset = desc.getAttributeOffset(EVertex::ATTRIBUTE_NORMAL) - posOffset;
		uint8* pV = (uint8*)(pVertex)+posOffset;

		int numEle = nIdx / 3;
		int vertexSize = desc.getVertexSize();
		uint32* pCur = idx;
		TArray< Vector3 > binormals(nV, Vector3(0, 0, 0));

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

	void MeshUtility::FillTangent_TriangleList(InputLayoutDesc const& desc, void* pVertex, int numVertices, uint32* indices, int numIndices, bool bNeedClear)
	{
		assert(desc.getAttributeFormat(EVertex::ATTRIBUTE_POSITION) == EVertex::Float3);
		assert(desc.getAttributeFormat(EVertex::ATTRIBUTE_NORMAL) == EVertex::Float3);
		assert(desc.getAttributeFormat(EVertex::ATTRIBUTE_TEXCOORD) == EVertex::Float2);
		assert(desc.getAttributeFormat(EVertex::ATTRIBUTE_TANGENT) == EVertex::Float4);

		FillTangent_TriangleList(
			MakeReader(desc, pVertex, EVertex::ATTRIBUTE_POSITION),
			MakeReader(desc, pVertex, EVertex::ATTRIBUTE_NORMAL),
			MakeReader(desc, pVertex, EVertex::ATTRIBUTE_TEXCOORD),
			MakeWriter(desc, pVertex, EVertex::ATTRIBUTE_TANGENT),
			numVertices , indices , numIndices, bNeedClear
		);
	}

	void MeshUtility::FillTangent_TriangleList(
		VertexElementReader const& positionReader, 
		VertexElementReader const& normalReader, 
		VertexElementReader const& uvReader, 
		VertexElementWriter& tangentWriter, int numVertices, uint32* indices, int numIndices, bool bNeedClear)
	{

		int numTriangles = numIndices / 3;

		TArray< Vector3 > binormals(numVertices, Vector3(0, 0, 0));
		if (bNeedClear)
		{
			for (int i = 0; i < numVertices; ++i)
			{
				tangentWriter.get< Vector4 >(i) = Vector4::Zero();
			}
		}


		uint32* pCur = indices;
		for (int i = 0; i < numTriangles; ++i)
		{
			uint32 i0 = pCur[0];
			uint32 i1 = pCur[1];
			uint32 i2 = pCur[2];
			pCur += 3;
			Vector3 const& v0 = positionReader[i0];
			Vector3 const& v1 = positionReader[i1];
			Vector3 const& v2 = positionReader[i2];

			Vector3 tangent, binormal;
			ComputeTangent(
				v0, uvReader.get<Vector2>(i0), 
				v1, uvReader.get<Vector2>(i1),
				v2, uvReader.get<Vector2>(i2), tangent, binormal);

			tangentWriter.get< Vector3 >(i0) += tangent;
			binormals[i0] += binormal;

			tangentWriter.get< Vector3 >(i1) += tangent;
			binormals[i1] += binormal;

			tangentWriter.get< Vector3 >(i2) += tangent;
			binormals[i2] += binormal;
		}

		for (int i = 0; i < numVertices; ++i)
		{
			Vector3& tangent = tangentWriter.get< Vector3 >(i);
			Vector3 const& normal = normalReader[i];

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

	void MeshUtility::FillTangent_QuadList(InputLayoutDesc const& desc, void* pVertex, int numVertices, uint32* indices, int numIndices, bool bNeedClear)
	{
		int numElements = numIndices / 4;
		TArray< uint32 > tempIndices(numElements * 6);
		uint32* src = indices;
		uint32* dest = &tempIndices[0];
		for (int i = 0; i < numElements; ++i)
		{
			dest[0] = src[0]; dest[1] = src[1]; dest[2] = src[2];
			dest[3] = src[2]; dest[4] = src[3]; dest[5] = src[0];
			dest += 6;
			src += 4;
		}
		FillTangent_TriangleList(desc, pVertex, numVertices, &tempIndices[0], tempIndices.size(), bNeedClear);
	}

	template< class IndexType >
	static uint32* MeshUtility::ConvertToTriangleListIndices(EPrimitive type, IndexType* data, int numData, TArray< uint32 >& outConvertBuffer, int& outNumTriangle)
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
				if (sizeof(IndexType) != sizeof(uint32))
				{
					outConvertBuffer.resize(numData);
					std::copy(data, data + numData, &outConvertBuffer[0]);
					return &outConvertBuffer[0];
				}
				return (uint32*)data;
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
				uint32* dest = &outConvertBuffer[0];
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
				return &outConvertBuffer[0];
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
				uint32* dest = &outConvertBuffer[0];
				for (int i = 0; i < numElements; ++i)
				{
					dest[0] = src[0]; dest[1] = src[2]; dest[2] = src[4];
					dest += 3;
					src += 6;
				}
				return &outConvertBuffer[0];
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
				uint32* dest = &outConvertBuffer[0];
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
				return &outConvertBuffer[0];
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
				uint32* dest = &outConvertBuffer[0];
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
				uint32* dest = &outConvertBuffer[0];
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

	template uint32* MeshUtility::ConvertToTriangleListIndices<uint32>(EPrimitive type, uint32* data, int numData, TArray< uint32 >& outConvertBuffer, int& outNumTriangle);
	template uint32* MeshUtility::ConvertToTriangleListIndices<uint16>(EPrimitive type, uint16* data, int numData, TArray< uint32 >& outConvertBuffer, int& outNumTriangle);

}//namespace Render