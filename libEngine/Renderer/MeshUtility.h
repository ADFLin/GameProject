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
			boundSizeScale = 1.1;
			bTwoSign = false;
			gridLength = 0.1;
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
		std::vector< float > volumeData;
	};

	class MeshUtility
	{
	public:
		static void CalcAABB(VertexElementReader const& positionReader, Vector3& outMin, Vector3& outMax);
		static int* ConvertToTriangleList(EPrimitive type, void* pIndexData, int numIndices, bool bIntType, std::vector< int >& outConvertBuffer, int& outNumTriangles);
		static bool BuildDistanceField(Mesh& mesh, DistanceFieldBuildSetting const& setting, DistanceFieldData& outResult);
		static bool BuildDistanceField(VertexElementReader const& positionReader, int* pIndexData, int numTriangles, DistanceFieldBuildSetting const& setting, DistanceFieldData& outResult);

		static bool IsVertexEqual(Vector3 const& a, Vector3 const& b, float error = 1e-6)
		{
			Vector3 diff = (a - b).abs();
			return diff.x < error && diff.y < error && diff.z < error;
		}

		static void BuildTessellationAdjacency(VertexElementReader const& positionReader, int* triIndices, int numTirangle, std::vector<int>& outResult);
		static void BuildVertexAdjacency(VertexElementReader const& positionReader, int* triIndices, int numTirangle, std::vector<int>& outResult);
		static void OptimizeVertexCache(void* pIndices, int numIndex, bool bIntType);

		static void FillTriangleListNormalAndTangent(InputLayoutDesc const& desc, void* pVertex, int nV, int* idx, int nIdx);
		static void FillTriangleListTangent(InputLayoutDesc const& desc, void* pVertex, int nV, int* idx, int nIdx);
		static void FillTriangleListNormal(InputLayoutDesc const& desc, void* pVertex, int nV, int* idx, int nIdx, int normalAttrib = Vertex::ATTRIBUTE_NORMAL);

		static bool Meshletize(
			int maxVertices, int maxPrims, int* triIndices, int numTriangles, VertexElementReader const& positionReader, 
			std::vector<MeshletData>& outMeshlets, std::vector<uint8>& outUniqueVertexIndices, std::vector<PackagedTriangleIndices>& outPrimitiveIndices);

		static bool Meshletize(
			int maxVertices, int maxPrims, int* triIndices, int numTriangles, VertexElementReader const& positionReader, TArrayView< MeshSection const > sections ,
			std::vector<MeshletData>& outMeshlets, std::vector<uint8>& outUniqueVertexIndices, std::vector<PackagedTriangleIndices>& outPrimitiveIndices , 
			std::vector< MeshSection >& outSections );

		static void GenerateCullData(
			VertexElementReader const& positionReader, const MeshletData* meshlets, int meshletCount, 
			const uint32* uniqueVertexIndices, const PackagedTriangleIndices* primitiveIndices, MeshletCullData* cullData);

		static void ComputeTangent(uint8* v0, uint8* v1, uint8* v2, int texOffset, Vector3& tangent, Vector3& binormal);

		static void FillNormal_TriangleList(InputLayoutDesc const& desc, void* pVertex, int nV, int* idx, int nIdx, int normalAttrib = Vertex::ATTRIBUTE_NORMAL);


		template< class IndexType >
		static int* ConvertToTriangleListIndices(EPrimitive type, IndexType* data, int numData, std::vector< int >& outConvertBuffer, int& outNumTriangle);

		static void FillNormalTangent_TriangleList(InputLayoutDesc const& desc, void* pVertex, int nV, int* idx, int nIdx);

		static void FillTangent_TriangleList(InputLayoutDesc const& desc, void* pVertex, int nV, int* idx, int nIdx);

		static void FillTangent_QuadList(InputLayoutDesc const& desc, void* pVertex, int nV, int* idx, int nIdx);
	};


}//namespace Render

#endif // MeshUtility_H_DB22DAD5_683A_440F_B891_952ABF13FABD
