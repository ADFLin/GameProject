#include "Mesh.h"

#include "Renderer/MeshUtility.h"
#include "RHI/RHICommand.h"

#include "Core/ScopeExit.h"
#include "Serialize/DataStream.h"

namespace Render
{
	Mesh::Mesh()
	{
		mType = EPrimitive::TriangleList;
	}

	Mesh::~Mesh()
	{
		releaseRHIResource();
	}

	bool Mesh::createRHIResource(void* pVertex, int nV, void* pIdx, int nIndices, bool bIntIndex)
	{
		{
			mInputLayout = RHICreateInputLayout(mInputLayoutDesc);
			if (!mInputLayout.isValid())
				return false;
		}

		if (nV)
		{
			mVertexBuffer = RHICreateVertexBuffer(mInputLayoutDesc.getVertexSize(), nV, BCF_DefalutValue, pVertex);
			if (!mVertexBuffer.isValid())
				return false;
		}
		else
		{
			mVertexBuffer = nullptr;
		}


		if (nIndices)
		{
			if (gbOptimizeVertexCache && mType == EPrimitive::TriangleList)
			{
				MeshUtility::OptimizeVertexCache(pIdx, nIndices, bIntIndex);
			}

			mIndexBuffer = RHICreateIndexBuffer(nIndices, bIntIndex, BCF_DefalutValue, pIdx);
			if (!mIndexBuffer.isValid())
				return false;
		}
		else
		{
			mIndexBuffer = nullptr;
		}

		return true;
	}

	void Mesh::releaseRHIResource()
	{
		mInputLayoutDesc.clear();
		mInputLayout.release();
		mInputLayoutOverwriteColor.release();
		mVertexBuffer.release();
		mColorBuffer.release();
		mIndexBuffer.release();
		mVertexAdjIndexBuffer.release();
		mTessAdjIndexBuffer.release();
	}

	void Mesh::draw(RHICommandList& commandList)
	{
		if (mVertexBuffer == nullptr)
			return;

		drawInternal(commandList, mType, 0, (mIndexBuffer) ? mIndexBuffer->getNumElements() : mVertexBuffer->getNumElements(), mIndexBuffer);
	}

	void Mesh::draw(RHICommandList& commandList, LinearColor const& color)
	{
		if (mVertexBuffer == nullptr)
			return;

		setupColorOverride(color);
		drawWithColorInternal(commandList, mType, 0, (mIndexBuffer) ? mIndexBuffer->getNumElements() : mVertexBuffer->getNumElements(), mIndexBuffer);
	}

	void Mesh::drawAdj(RHICommandList& commandList)
	{
		if (mVertexBuffer == nullptr || mVertexAdjIndexBuffer == nullptr)
			return;

		drawInternal(commandList, EPrimitive::TriangleAdjacency, 0, mVertexAdjIndexBuffer->getNumElements(), mVertexAdjIndexBuffer);
	}

	void Mesh::drawAdj(RHICommandList& commandList, LinearColor const& color)
	{
		if (mVertexBuffer == nullptr || mVertexAdjIndexBuffer == nullptr)
			return;

		setupColorOverride(color);
		drawWithColorInternal(commandList, EPrimitive::TriangleAdjacency, 0, mVertexAdjIndexBuffer->getNumElements(), mVertexAdjIndexBuffer);
	}

	void Mesh::drawTessellation(RHICommandList& commandList, int patchPointCount, bool bUseAdjBuffer)
	{
		if (mVertexBuffer == nullptr)
			return;

		RHIIndexBuffer* indexBuffer = bUseAdjBuffer ? mTessAdjIndexBuffer : mIndexBuffer;
		if (indexBuffer == nullptr)
			return;

		drawInternal(commandList, EPrimitive((int)EPrimitive::PatchPoint1 + patchPointCount), 0, indexBuffer->getNumElements(), indexBuffer);
	}


	void Mesh::drawSection(RHICommandList& commandList, int idx)
	{
		if (mVertexBuffer == nullptr)
			return;
		MeshSection& section = mSections[idx];
		drawInternal(commandList, mType, section.indexStart, section.count, mIndexBuffer);
	}

	void Mesh::drawInternal(RHICommandList& commandList, EPrimitive type, int idxStart, int num, RHIIndexBuffer* indexBuffer)
	{
		assert(mVertexBuffer != nullptr);
		InputStreamInfo inputStream;
		inputStream.buffer = mVertexBuffer;
		RHISetInputStream(commandList, mInputLayout, &inputStream, 1);

		if (indexBuffer)
		{
			RHISetIndexBuffer(commandList, indexBuffer);
			RHIDrawIndexedPrimitive(commandList, type, idxStart, num);
		}
		else
		{
			RHIDrawPrimitive(commandList, type, idxStart, num);
		}
	}

	bool Mesh::buildMeshlet(int maxVertices, int maxPrims, std::vector<MeshletData>& outMeshlets, std::vector<uint8>& outUniqueVertexIndices, std::vector<PackagedTriangleIndices>& outPrimitiveIndices, std::vector<MeshletCullData>* outCullDataList)
	{

		uint8* pVertex = (uint8*)RHILockBuffer(mVertexBuffer, ELockAccess::ReadOnly);
		uint8* pIndex = (uint8*)RHILockBuffer(mIndexBuffer, ELockAccess::ReadOnly);
		ON_SCOPE_EXIT
		{
			RHIUnlockBuffer(mVertexBuffer);
			RHIUnlockBuffer(mIndexBuffer);
		};


		if (pVertex == nullptr || pIndex == nullptr)
			return false;

		std::vector< uint32 > tempBuffer;
		int numTriangles = 0;
		uint32* pIndexData = MeshUtility::ConvertToTriangleList(mType, pIndex, mIndexBuffer->getNumElements(), mIndexBuffer->isIntType(), tempBuffer, numTriangles);
		if (pIndexData == nullptr)
			return false;

		auto positionReader = makePositionReader(pVertex);
		bool result = MeshUtility::Meshletize(maxVertices, maxPrims, pIndexData, numTriangles, positionReader, getVertexCount(), outMeshlets, outUniqueVertexIndices, outPrimitiveIndices);

		if (result && outCullDataList)
		{
			outCullDataList->resize(outMeshlets.size());
			MeshUtility::GenerateCullData(positionReader, getVertexCount(), outMeshlets.data(), outMeshlets.size(), (uint32 const*)outUniqueVertexIndices.data(), outPrimitiveIndices.data(), outCullDataList->data());
		}

		return result;
	}

	void Mesh::setupColorOverride(LinearColor const& color)
	{
		if (!mInputLayoutOverwriteColor.isValid())
		{
			InputLayoutDesc desc = mInputLayoutDesc;
			desc.setElementUnusable(EVertex::ATTRIBUTE_COLOR);
			desc.addElement(1, EVertex::ATTRIBUTE_COLOR, EVertex::Float4);
			mInputLayoutOverwriteColor = RHICreateInputLayout(desc);
			mColorBuffer = RHICreateVertexBuffer(sizeof(LinearColor), 1/* mVertexBuffer->getNumElements()*/, BCF_DefalutValue | BCF_CpuAccessWrite);
		}

		LinearColor* pColor = (LinearColor*)RHILockBuffer(mColorBuffer, ELockAccess::WriteDiscard);
		for (int i = 0; i < 1/*mVertexBuffer->getNumElements()*/; ++i)
		{
			*pColor = color;
			++pColor;
		}
		RHIUnlockBuffer(mColorBuffer);
	}

	void Mesh::drawWithColorInternal(RHICommandList& commandList, EPrimitive type, int idxStart, int num, RHIIndexBuffer* indexBuffer)
	{
		assert(mVertexBuffer != nullptr);
		InputStreamInfo inputStreams[2];
		inputStreams[0].buffer = mVertexBuffer;
		inputStreams[1].buffer = mColorBuffer;
		inputStreams[1].stride = 0;
		RHISetInputStream(commandList, mInputLayoutOverwriteColor, inputStreams, 2);

		if (indexBuffer)
		{
			RHISetIndexBuffer(commandList, indexBuffer);
			RHIDrawIndexedPrimitive(commandList, type, idxStart, num);
		}
		else
		{
			RHIDrawPrimitive(commandList, type, idxStart, num);
		}
	}

	bool Mesh::generateVertexAdjacency()
	{
		return generateAdjacencyInternal(EAdjacencyType::Vertex, mVertexAdjIndexBuffer);
	}

	bool Mesh::generateTessellationAdjacency()
	{
		return generateAdjacencyInternal(EAdjacencyType::Tessellation, mTessAdjIndexBuffer);
	}

	bool Mesh::generateAdjacencyInternal(EAdjacencyType type, RHIIndexBufferRef& outIndexBuffer)
	{
		if (outIndexBuffer.isValid())
			return true;

		if (!mIndexBuffer.isValid() || !mVertexBuffer.isValid())
		{
			return false;
		}

		uint8* pVertex = (uint8*)RHILockBuffer(mVertexBuffer, ELockAccess::ReadOnly);
		uint8* pIndex = (uint8*)RHILockBuffer(mIndexBuffer, ELockAccess::ReadOnly);
		ON_SCOPE_EXIT
		{
			RHIUnlockBuffer(mVertexBuffer);
			RHIUnlockBuffer(mIndexBuffer);
		};
		if (pVertex == nullptr || pIndex == nullptr)
			return false;

		std::vector< uint32 > tempBuffer;
		int numTriangles = 0;
		uint32* pIndexData = MeshUtility::ConvertToTriangleList(mType, pIndex, mIndexBuffer->getNumElements(), mIndexBuffer->isIntType(), tempBuffer, numTriangles);

		if (pIndexData == nullptr)
			return false;

		std::vector< int > adjIndices;

		switch (type)
		{
		case EAdjacencyType::Vertex:
			MeshUtility::BuildVertexAdjacency(makePositionReader(pVertex), getVertexCount(), pIndexData, numTriangles, adjIndices);
			break;
		case EAdjacencyType::Tessellation:
			MeshUtility::BuildTessellationAdjacency(makePositionReader(pVertex), pIndexData, numTriangles, adjIndices);
			break;
		}

		outIndexBuffer = RHICreateIndexBuffer(adjIndices.size(), true, 0, &adjIndices[0]);
		return outIndexBuffer.isValid();
	}

	bool Mesh::save(IStreamSerializer& serializer)
	{
		serializer << (uint8)mType;
		uint8* pVertex = (uint8*)RHILockBuffer(mVertexBuffer, ELockAccess::ReadOnly);
		uint32 vertexDataSize = mVertexBuffer->getSize();
		uint8* pIndex = (uint8*)RHILockBuffer(mIndexBuffer, ELockAccess::ReadOnly);
		uint32 indexDataSize = mIndexBuffer->getSize();
		ON_SCOPE_EXIT
		{
			RHIUnlockBuffer(mVertexBuffer);
			RHIUnlockBuffer(mIndexBuffer);
		};

		if (pVertex == nullptr || pIndex == nullptr)
			return false;

		bool bIntType = mIndexBuffer->isIntType();
		serializer << vertexDataSize;
		serializer.write(pVertex, vertexDataSize);
		serializer << bIntType;
		serializer << indexDataSize;
		serializer.write(pIndex, indexDataSize);

		serializer << mInputLayoutDesc;
		serializer << mSections;
		return true;
	}

	bool Mesh::load(IStreamSerializer& serializer)
	{
		uint8 type;
		serializer >> type;
		mType = EPrimitive(type);
		std::vector<uint8> vertexData;
		std::vector<uint8> indexData;

		uint32 vertexDataSize;
		serializer >> vertexDataSize;
		if (vertexDataSize)
		{
			vertexData.resize(vertexDataSize);
			serializer.read(vertexData.data(), vertexDataSize);
		}
		uint32 indexDataSize;
		bool bIntType;
		serializer >> bIntType;
		serializer >> indexDataSize;
		if (indexDataSize)
		{
			indexData.resize(indexDataSize);
			serializer.read(indexData.data(), indexDataSize);
		}

		serializer >> mInputLayoutDesc;
		serializer >> mSections;
		if (!createRHIResource(vertexData.data(), vertexDataSize / mInputLayoutDesc.getVertexSize(),
			indexData.data(), bIntType ? indexDataSize / sizeof(uint32) : indexDataSize / sizeof(uint16), bIntType))
		{
			return false;
		}
		return true;

	}


}