#pragma once
#ifndef Mesh_H_34CBB38C_714D_4599_9251_35F2EA108F91
#define Mesh_H_34CBB38C_714D_4599_9251_35F2EA108F91

#include "RHI/RHICommon.h"
#include "Renderer/MeshCommon.h"

class IStreamSerializer;

namespace Render
{

	FORCEINLINE bool IsIntType(RHIBuffer* buffer)
	{
		return buffer->getElementSize() == 4;
	}
	class Mesh
	{
	public:

		Mesh();
		~Mesh();

		Mesh(Mesh const& mesh) = default;

		bool createRHIResource(void* pVertex, int nV, void* pIdx, int nIndices, bool bIntIndex);

		template< typename IndexType = uint16 >
		bool createRHIResource(void* pVertex, int nV, IndexType* pIdx = nullptr, int nIndices = 0)
		{
			static_assert(sizeof(IndexType) == sizeof(uint16) || sizeof(IndexType) == sizeof(uint32));
			return createRHIResource(pVertex, nV, pIdx, nIndices, sizeof(IndexType) == sizeof(uint32));
		}
		void releaseRHIResource();
		void draw(RHICommandList& commandList);
		void draw(RHICommandList& commandList, LinearColor const& color);
		void drawPositionOnly(RHICommandList& commandList);

		void drawAdj(RHICommandList& commandList);
		void drawAdj(RHICommandList& commandList, LinearColor const& color);
		void drawTessellation(RHICommandList& commandList, int patchPointCount, bool bUseAdjBuffer = false);
		void drawSection(RHICommandList& commandList, int idx);


		void setupColorOverride(LinearColor const& color);
		void drawWithColorInternal(RHICommandList& commandList, EPrimitive type, int idxStart, int num, RHIBuffer* indexBuffer);
		void drawInternal(RHICommandList& commandList, EPrimitive type, int idxStart, int num, RHIBuffer* indexBuffer);

		bool buildMeshlet(int maxVertices, int maxPrims, TArray<MeshletData>& outMeshlets, TArray<uint8>& outUniqueIndices, TArray<PackagedTriangleIndices>& outPrimitiveIndices, TArray<MeshletCullData>* outCullDataList = nullptr);


		VertexElementReader makeAttributeReader(uint8 const* pData, EVertex::Attribute attribute)
		{
			VertexElementReader result;
			result.vertexDataStride = mInputLayoutDesc.getVertexSize();
			result.pVertexData = pData + mInputLayoutDesc.getAttributeOffset(attribute);
			return result;
		}
		VertexElementReader makePositionReader(uint8 const* pData)
		{
			return makeAttributeReader(pData, EVertex::ATTRIBUTE_POSITION);
		}


		VertexElementReader makeUVReader(uint8 const* pData, int index = 0)
		{
			return makeAttributeReader(pData, EVertex::Attribute(EVertex::ATTRIBUTE_TEXCOORD + index));
		}

		VertexElementWriter makeAttributeWriter(uint8* pData, EVertex::Attribute attribute)
		{
			VertexElementWriter result;
			result.vertexDataStride = mInputLayoutDesc.getVertexSize();
			result.pVertexData = pData + mInputLayoutDesc.getAttributeOffset(attribute);
			return result;
		}

		VertexElementWriter makePositionWriter(uint8* pData)
		{
			return makeAttributeWriter(pData, EVertex::ATTRIBUTE_POSITION);
		}

		bool generateVertexAdjacency();
		bool generateTessellationAdjacency();
		bool generateAdjacencyInternal(EAdjacencyType type, RHIBufferRef& outIndexBuffer);

		bool save(IStreamSerializer& serializer);
		bool load(IStreamSerializer& serializer);

		int getVertexCount() const { return mVertexBuffer->getNumElements(); }

		Mesh& operator = (Mesh&& mesh)
		{
			mType = mesh.mType;

#define MOVE_MEMBER(MEMBER)\
	MEMBER = std::move(mesh.MEMBER);

			MOVE_MEMBER(mInputLayoutDesc);
			MOVE_MEMBER(mInputLayout);
			MOVE_MEMBER(mInputLayoutPositionOnly);
			MOVE_MEMBER(mInputLayoutOverwriteColor);
			MOVE_MEMBER(mVertexBuffer);
			MOVE_MEMBER(mColorBuffer);
			MOVE_MEMBER(mIndexBuffer);
			MOVE_MEMBER(mVertexAdjIndexBuffer);
			MOVE_MEMBER(mTessAdjIndexBuffer);
			MOVE_MEMBER(mSections);
			return *this;
		}

		EPrimitive          mType;
		InputLayoutDesc     mInputLayoutDesc;
		RHIInputLayoutRef   mInputLayout;
		RHIInputLayoutRef   mInputLayoutOverwriteColor;
		RHIInputLayoutRef   mInputLayoutPositionOnly;
		RHIBufferRef  mVertexBuffer;
		RHIBufferRef  mColorBuffer;
		RHIBufferRef  mIndexBuffer;
		RHIBufferRef  mVertexAdjIndexBuffer;
		RHIBufferRef  mTessAdjIndexBuffer;

		TArray< MeshSection > mSections;
	};



}

#endif // Mesh_H_34CBB38C_714D_4599_9251_35F2EA108F91