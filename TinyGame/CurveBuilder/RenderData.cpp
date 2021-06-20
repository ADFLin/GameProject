#include "RenderData.h"

namespace CB
{
	RenderData::RenderData()
		:mVertexNum(0)
		,mVertexSize(0)
		,mbNormalOwned(false)
	{

	}

	RenderData::RenderData(int numVertices, int numIndex, bool useNormal)
	{
		create(numVertices, numIndex, useNormal);
	}

	RenderData::~RenderData()
	{
		release();
	}

	void RenderData::create(int numVertices, int numIndex, bool bUseNormal)
	{
		mbNormalOwned = bUseNormal;
		mVertexNum = numVertices;
		mVertexSize = sizeof(float) * (7 + (bUseNormal ? 3 : 0));
		mVertexBuffer.resize(mVertexSize * mVertexNum);
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
	}

}//namespace CB