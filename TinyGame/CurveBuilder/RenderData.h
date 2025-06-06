#pragma once
#ifndef RenderData_H_8A5058DB_EAFA_4050_B401_AD89A5C32202
#define RenderData_H_8A5058DB_EAFA_4050_B401_AD89A5C32202

#include "Base.h"
#include "Color.h"
#include "DataStructure/Array.h"
#include "RHI/RHICommon.h"

namespace CB
{

#define USE_SHARE_TRIANGLE_INFO 1
#define USE_COMPACTED_SHARE_TRIANGLE_INFO 1

	struct RenderResource
	{
		Render::RHIBufferRef vertexBuffer;
		Render::RHIBufferRef indexBuffer;
#if USE_SHARE_TRIANGLE_INFO
		Render::RHIBufferRef sharedTriangleInfoBuffer;
		Render::RHIBufferRef triangleIdBuffer;
#endif
	};

	class RenderData
	{
	public:
		RenderData();
		~RenderData();

		void        create(int numVertices, int numAlign, int numIndex, bool bUseNormal, bool bUseResource = false);
		void        release();

		uint8*      getVertexData() { return mVertexDataPtr; }
		int         getPositionOffset()  const { return 0 * sizeof(float); }
		int         getColorOffset() const { return 3 * sizeof(float); }
		int         getNormalOffset() const { return mbNormalOwned ? (7 * sizeof(float)) : INDEX_NONE; }

		int         getVertexNum() const { return mVertexNum; }
		int         getVertexSize() const {  return mVertexSize;  }

		int         getIndexNum() const { return mIndexNum; }
		uint32*     getIndexData() { return mIndexDataPtr; }

		void       setCachedDataSize(int size){ mCachedBuffer.resize(size);}
		uint8*     getCachedData(){ return mCachedBuffer.data(); }


		void lockVertexResource();
		void unlockVertexResource();


		void releaseResource()
		{
			delete resource;
			resource = nullptr;
		}


		class RenderResource* resource = nullptr;

	private:

		friend class ShapeMeshBuilder;

		uint8*      mVertexDataPtr = nullptr;
		uint32*     mIndexDataPtr = nullptr;

		int         mVertexNum;
		int         mVertexSize;
		int         mIndexNum;
		bool        mbNormalOwned;

		TArray< uint8 >  mCachedBuffer;
		TArray< uint8 >  mVertexBuffer;
		TArray< uint32 > mIndexBuffer;
	};


}//namespace CB


#endif // RenderData_H_8A5058DB_EAFA_4050_B401_AD89A5C32202
