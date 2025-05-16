#include "RenderData.h"

namespace CB
{
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

	void RenderData::create(int numVertices, int numAlign, int numIndex, bool bUseNormal)
	{
		mbNormalOwned = bUseNormal;
		mVertexNum = numVertices;
		mVertexSize = sizeof(float) * (7 + (bUseNormal ? 3 : 0));
		mVertexBuffer.resize(Math::AlignUp(mVertexNum, numAlign) * mVertexSize);
		mIndexBuffer.resize(numIndex);
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

}//namespace CB