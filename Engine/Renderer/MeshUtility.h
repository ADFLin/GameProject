#pragma once
#ifndef MeshUtility_H_DB22DAD5_683A_440F_B891_952ABF13FABD
#define MeshUtility_H_DB22DAD5_683A_440F_B891_952ABF13FABD

#include "Renderer/MeshCommon.h"
#include "RHI/RHICommon.h"

#include "Template/ArrayView.h"

#include <vector>

class QueueThreadPool;
class IStreamSerializer;

namespace Render
{
	class Mesh;

	extern bool gbOptimizeVertexCache;

	struct DistanceFieldBuildSetting
	{
		bool  bTwoSign;
		float boundSizeScale;
		float gridLength;
		float resolutionScale;
		QueueThreadPool* workTaskPool;

		DistanceFieldBuildSetting()
		{
			boundSizeScale = 1.1f;
			bTwoSign = false;
			gridLength = 0.1f;
			resolutionScale = 1.0f;
			workTaskPool = nullptr;
		}
	};

	struct DistanceFieldData
	{
		Vector3    boundMin;
		Vector3    boundMax;
		IntVector3 gridSize;
		float      maxDistance;
		TArray< float > volumeData;
	};

	class MeshUtility
	{
	public:
		static void CalcAABB(VertexElementReader const& positionReader, int numVertices, Vector3& outMin, Vector3& outMax);
		static uint32* ConvertToTriangleList(EPrimitive type, void* pIndexData, int numIndices, bool bIntType, TArray< uint32 >& outConvertBuffer, int& outNumTriangles);
		static bool BuildDistanceField(Mesh& mesh, DistanceFieldBuildSetting const& setting, DistanceFieldData& outResult);
		static bool BuildDistanceField(VertexElementReader const& positionReader, int numVertices, uint32* pIndexData, int numTriangles, DistanceFieldBuildSetting const& setting, DistanceFieldData& outResult);

		static bool IsVertexEqual(Vector3 const& a, Vector3 const& b, float error = 1e-6)
		{
			Vector3 diff = (a - b).abs();
			return diff.x < error && diff.y < error && diff.z < error;
		}

		struct SharedTriangleInfo
		{
			uint32 offset;
			uint32 count;
		};
		static void BuildVertexSharedTriangleInfo(uint32 const* triIndices, int numTirangle, int numVertices, TArray<SharedTriangleInfo>& outInfos, TArray<uint32>& outTriangles);

		static void BuildTessellationAdjacency(VertexElementReader const& positionReader, uint32* triIndices, int numTirangle, TArray<int>& outResult);
		static void BuildVertexAdjacency(VertexElementReader const& positionReader, int numVertices, uint32* triIndices, int numTirangle, TArray<int>& outResult);
		static void OptimizeVertexCache(void* pIndices, int numIndex, bool bIntType);

		static void FillTriangleListNormalAndTangent(InputLayoutDesc const& desc, void* pVertex, int nV, uint32* idx, int nIdx);
		static void FillTriangleListTangent(InputLayoutDesc const& desc, void* pVertex, int nV, uint32* idx, int nIdx);
		static void FillTriangleListNormal(InputLayoutDesc const& desc, void* pVertex, int nV, uint32* idx, int nIdx, int normalAttrib = EVertex::ATTRIBUTE_NORMAL, bool bNeedClear = false);

		static bool Meshletize(
			int maxVertices, int maxPrims, uint32* triIndices, int numTriangles, VertexElementReader const& positionReader, int numVertices,
			TArray<MeshletData>& outMeshlets, TArray<uint8>& outUniqueVertexIndices, TArray<PackagedTriangleIndices>& outPrimitiveIndices);

		static bool Meshletize(
			int maxVertices, int maxPrims, uint32* triIndices, int numTriangles, VertexElementReader const& positionReader, int numVertices, TArrayView< MeshSection const > sections ,
			TArray<MeshletData>& outMeshlets, TArray<uint8>& outUniqueVertexIndices, TArray<PackagedTriangleIndices>& outPrimitiveIndices ,
			TArray< MeshSection >& outSections );

		static void GenerateCullData(
			VertexElementReader const& positionReader, int numVertices, const MeshletData* meshlets, int meshletCount,
			const uint32* uniqueVertexIndices, const PackagedTriangleIndices* primitiveIndices, MeshletCullData* cullData);

		static void ComputeTangent(uint8* v0, uint8* v1, uint8* v2, int texOffset, Vector3& tangent, Vector3& binormal);
		static void ComputeTangent(Vector3 const& v0, Vector2 const& uv0, Vector3 const& v1, Vector2 const& uv1, Vector3 const& v2, Vector2 const& uv2, Vector3& tangent, Vector3& binormal);

		static void FillNormal_TriangleList(InputLayoutDesc const& desc, void* pVertex, int numVerteices, uint32* indices, int numIndices, int normalAttrib = EVertex::ATTRIBUTE_NORMAL, bool bNeedClear = false);
		static void FillNormal_TriangleList(VertexElementReader const& positionReader, VertexElementWriter& normalWriter, int numVerteices, uint32* idx, int numIndices, bool bNeedClear = false, bool bNormalize = true);

		template< class IndexType >
		static uint32* ConvertToTriangleListIndices(EPrimitive type, IndexType* data, int numData, TArray< uint32 >& outConvertBuffer, int& outNumTriangle);

		static void FillNormalTangent_TriangleList(InputLayoutDesc const& desc, void* pVertex, int nV, uint32* idx, int nIdx);

		static void FillTangent_TriangleList(
			VertexElementReader const& positionReader,
			VertexElementReader const& normalReader, 
			VertexElementReader const& uvReader, 
			VertexElementWriter& tangentWriter, 
			int numVertices, uint32* indices, int numIndices, bool bNeedClear = false);

		static void FillTangent_TriangleList(InputLayoutDesc const& desc, void* pVertex, int numVertices, uint32* indices, int numIndices, bool bNeedClear = false);
		static void FillTangent_QuadList(InputLayoutDesc const& desc, void* pVertex, int numVertices, uint32* indices, int numIndices, bool bNeedClear = false);
	};


}//namespace Render

#endif // MeshUtility_H_DB22DAD5_683A_440F_B891_952ABF13FABD
