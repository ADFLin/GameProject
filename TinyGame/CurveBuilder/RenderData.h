#pragma once
#ifndef RenderData_H_8A5058DB_EAFA_4050_B401_AD89A5C32202
#define RenderData_H_8A5058DB_EAFA_4050_B401_AD89A5C32202

#include "Base.h"
#include "Color.h"
#include "DataStructure/Array.h"

namespace CB
{

	class RenderData
	{
	public:
		RenderData();
		RenderData(int numVertices, int numIndex, bool useNormal);
		~RenderData();

		void        create(int numVertices, int numIndex, bool bUseNormal);
		void        release();

		uint8*      getVertexData() { return (mVertexBuffer.empty()) ? nullptr : &mVertexBuffer[0]; }
		int         getPositionOffset()  const { return 0 * sizeof(float); }
		int         getColorOffset() const { return 3 * sizeof(float); }
		int         getNormalOffset() const { return mbNormalOwned ? (7 * sizeof(float)) : INDEX_NONE; }

		int         getVertexNum() const { return mVertexNum; }
		int         getVertexSize() const {  return mVertexSize;  }

		int         getIndexNum() const { return mIndexBuffer.size(); }
		uint32*     getIndexData() { return mIndexBuffer.empty() ? nullptr : &mIndexBuffer[0]; }

	private:


		int         mVertexNum;
		int         mVertexSize;
		bool        mbNormalOwned;

		TArray< uint8 > mVertexBuffer;
		TArray< uint32 > mIndexBuffer;
	};


}//namespace CB


#endif // RenderData_H_8A5058DB_EAFA_4050_B401_AD89A5C32202
