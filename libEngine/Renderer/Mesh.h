#pragma once
#ifndef Mesh_H_34CBB38C_714D_4599_9251_35F2EA108F91
#define Mesh_H_34CBB38C_714D_4599_9251_35F2EA108F91

#include "RHI/RHICommon.h"
#include "Renderer/MeshCommon.h"

class IStreamSerializer;

namespace Render
{

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

		bool buildMeshlet(int maxVertices, int maxPrims, std::vector<MeshletData>& outMeshlets, std::vector<uint8>& outUniqueIndices, std::vector<PackagedTriangleIndices>& outPrimitiveIndices, std::vector<MeshletCullData>* outCullDataList = nullptr);


		VertexElementReader makeAttributeReader(uint8 const* pData, Vertex::Attribute attribute)
		{
			VertexElementReader result;
			result.numVertex = mVertexBuffer->getNumElements();
			result.vertexDataStride = mVertexBuffer->getElementSize();
			result.pVertexData = pData + mInputLayoutDesc.getAttributeOffset(attribute);
			return result;
		}
		VertexElementReader makePositionReader(uint8 const* pData)
		{
			return makeAttributeReader(pData, Vertex::ATTRIBUTE_POSITION);
		}

		VertexElementReader makeUVReader(uint8 const* pData, int index = 0)
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



}

#endif // Mesh_H_34CBB38C_714D_4599_9251_35F2EA108F91