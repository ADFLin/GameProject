#include "RenderData.h"

#include "RHI/RHICommand.h"

namespace CB
{

	using namespace Render;

	RenderData::RenderData()
		:mVertexNum(0)
		,mVertexSize(0)
		,mbNormalOwned(false)
	{

	}

	RenderData::~RenderData()
	{
		release();
	}

	void RenderData::create(int numVertices, int numAlign, int numIndex, bool bUseNormal, bool bUseResource)
	{
		mbNormalOwned = bUseNormal;
		mVertexNum = numVertices;
		mVertexSize = sizeof(float) * (7 + (bUseNormal ? 3 : 0));
		mIndexNum = numIndex;

		if (bUseResource)
		{
			if (resource == nullptr)
			{
				resource = new RenderResource;
			}

			resource->vertexBuffer = RHICreateVertexBuffer(mVertexSize, Math::AlignUp(mVertexNum, numAlign), BCF_CpuAccessRead | BCF_CpuAccessWrite);
			resource->indexBuffer = RHICreateIndexBuffer(numIndex, true, BCF_CpuAccessWrite);
		}
		else
		{
			mVertexBuffer.resize(Math::AlignUp(mVertexNum, numAlign) * mVertexSize);
			mIndexBuffer.resize(numIndex);

			mVertexDataPtr = mVertexBuffer.data();
			mIndexDataPtr = mIndexBuffer.data();
		}
	}

	void RenderData::release()
	{
		mVertexBuffer.clear();
		mVertexBuffer.shrink_to_fit();
		mIndexBuffer.clear();
		mIndexBuffer.shrink_to_fit();

		mVertexNum = 0;
		mVertexSize = 0;
		mbNormalOwned = false;

		delete resource;
		resource = nullptr;
	}

	void RenderData::lockVertexResource()
	{
		if ( resource == nullptr )
			return;

		mVertexDataPtr = (uint8*)RHILockBuffer(resource->vertexBuffer, ELockAccess::ReadWrite);
	}

	void RenderData::unlockVertexResource()
	{
		if (resource == nullptr)
			return;


		RHIUnlockBuffer(resource->vertexBuffer);
		mVertexDataPtr = nullptr;
	}

}//namespace CB