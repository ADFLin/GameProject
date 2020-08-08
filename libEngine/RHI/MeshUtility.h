#pragma once
#ifndef MeshUtility_H_DB22DAD5_683A_440F_B891_952ABF13FABD
#define MeshUtility_H_DB22DAD5_683A_440F_B891_952ABF13FABD

#include "RHI/RHICommon.h"

#include "MarcoCommon.h"

#include <string>

class QueueThreadPool;
class IStreamSerializer;

namespace Render
{
	extern bool gbOptimizeVertexCache;

	struct OBJMaterialInfo
	{
		char const* name;

		Vector3 ambient;
		Vector3 diffuse;
		Vector3 specular;
		Vector3 transmittance;
		Vector3 emission;
		float shininess;
		float ior;
		float dissolve;
		int   illum;
		char const* ambientTextureName;
		char const* diffuseTextureName;
		char const* specularTextureName;
		char const* normalTextureName;
		std::vector < std::pair< char const*, char const* > > unknownParameters;

		char const* findParam(char const* name) const
		{
			for( auto& pair : unknownParameters )
			{
				if( strcmp(name, pair.first) == 0 )
					return pair.second;
			}
			return nullptr;
		}
	};

	class OBJMaterialBuildListener
	{
	public:
		virtual void build(int idxSection, OBJMaterialInfo const* mat) = 0;
	};


	struct VertexElementReader
	{
		template< class T = Vector3 >
		T const& get(int idx) const
		{
			return *(T const*)(pVertexData + idx * vertexDataStride);
		}
		int32  getNum() const { return numVertex; }
		uint8 const* pVertexData;
		uint32 vertexDataStride;
		int32  numVertex;
	};


	enum class EAdjacencyType
	{
		Vertex,
		Tessellation,
	};

	struct MeshSection
	{
		int start;
		int num;
	};

	class Mesh
	{
	public:

		Mesh();
		~Mesh();

		bool createRHIResource(void* pVertex, int nV, void* pIdx = nullptr, int nIndices = 0, bool bIntIndex = false);
		void releaseRHIResource();
		void draw(RHICommandList& commandList);
		void draw(RHICommandList& commandList, LinearColor const& color);

		void drawAdj(RHICommandList& commandList);
		void drawAdj(RHICommandList& commandList, LinearColor const& color);
		void drawTessellation(RHICommandList& commandList, int patchPointCount, bool bUseAdjBuffer = false);
		void drawSection(RHICommandList& commandList, int idx);


		void setupColorOverride(LinearColor const& color);
		void drawWithColorInternal(RHICommandList& commandList, EPrimitive type, int idxStart, int num, RHIIndexBuffer* indexBuffer);
		void drawInternal(RHICommandList& commandList, EPrimitive type, int idxStart, int num, RHIIndexBuffer* indexBuffer);

		VertexElementReader makePositionReader(uint8 const* pData)
		{
			VertexElementReader result;
			result.numVertex = mVertexBuffer->getNumElements();
			result.vertexDataStride = mVertexBuffer->getElementSize();
			result.pVertexData = pData + mInputLayoutDesc.getAttributeOffset(Vertex::ATTRIBUTE_POSITION);
			return result;
		}

		VertexElementReader makeUVReader(uint8 const* pData , int index = 0 )
		{
			VertexElementReader result;
			result.numVertex = mVertexBuffer->getNumElements();
			result.vertexDataStride = mVertexBuffer->getElementSize();
			result.pVertexData = pData + mInputLayoutDesc.getAttributeOffset(Vertex::ATTRIBUTE_TEXCOORD + index);
			return result;
		}


		bool generateVertexAdjacency();
		bool generateTessellationAdjacency();
		bool generateAdjacencyInternal(EAdjacencyType type, RHIIndexBufferRef& outIndexBuffer);

		bool save(IStreamSerializer& serializer);
		bool load(IStreamSerializer& serializer);

		EPrimitive          mType;
		InputLayoutDesc     mInputLayoutDesc;
		RHIInputLayoutRef   mInputLayout;
		RHIInputLayoutRef   mInputLayoutOverwriteColor;
		RHIVertexBufferRef  mVertexBuffer;
		RHIVertexBufferRef  mColorBuffer;
		RHIIndexBufferRef   mIndexBuffer;
		RHIIndexBufferRef   mVertexAdjIndexBuffer;
		RHIIndexBufferRef   mTessAdjIndexBuffer;

		std::vector< MeshSection > mSections;
	};

	class MeshBuild
	{
	public:
		static bool Tile( Mesh& mesh , int tileSize , float len , bool bHaveSkirt = true );
		static bool UVSphere( Mesh& mesh , float radius , int rings, int sectors);
		static bool IcoSphere( Mesh& mesh , float radius , int numDiv );
		static bool SkyBox( Mesh& mesh );
		static bool CubeShare(Mesh& mesh, float halfLen = 1.0f);
		static bool CubeOffset(Mesh& mesh, float halfLen, Vector3 const& offset);
		static bool Cube(Mesh& mesh, float halfLen = 1.0f);
		
		static bool Cone(Mesh& mesh, float height, int numSide);
		static bool Doughnut(Mesh& mesh , float radius , float ringRadius , int rings , int sectors);
		static bool PlaneZ( Mesh& mesh , float len, float texFactor );
		static bool Plane( Mesh& mesh , Vector3 const& offset , Vector3 const& normal , Vector3 const& dirY , Vector2 const& size, float texFactor);

		static bool SimpleSkin(Mesh& mesh, float width, float height, int nx, int ny);

		static bool LightSphere(Mesh& mesh);
		static bool LightCone(Mesh& mesh);

		struct MeshData
		{
			float* position;
			int    numPosition;
			float* normal;
			int    numNormal;
			int*   indices;
			int    numIndex;
		};
		static bool ObjFileMesh(Mesh& mesh, MeshData& data);



		static bool LoadObjectFile(
			Mesh& mesh, char const* path , 
			Matrix4* pTransform = nullptr , 
			OBJMaterialBuildListener* listener = nullptr ,
			int * skip = nullptr );

	};

	struct DistanceFieldBuildSetting
	{
		bool  bTwoSign;
		float boundSizeScale;
		float gridLength;
		float ResolutionScale;
		QueueThreadPool* WorkTaskPool;

		DistanceFieldBuildSetting()
		{
			boundSizeScale = 1.1;
			bTwoSign = false;
			gridLength = 0.1;
			ResolutionScale = 1.0f;
			WorkTaskPool = nullptr;
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
		static int* ConvertToTriangleList(EPrimitive type, void* pIndexData, int numIndices ,bool bIntType, std::vector< int >& outConvertBuffer, int& outNumTriangles);
		static bool BuildDistanceField(Mesh& mesh, DistanceFieldBuildSetting const& setting , DistanceFieldData& outResult);
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
	};


}//namespace Render

#endif // MeshUtility_H_DB22DAD5_683A_440F_B891_952ABF13FABD
