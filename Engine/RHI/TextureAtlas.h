#pragma once
#ifndef TextureAtlas_H_991594F1_1108_4346_B634_187B185F2B8D
#define TextureAtlas_H_991594F1_1108_4346_B634_187B185F2B8D

#include "RHICommon.h"
#include "Misc/ImageMergeHelper.h"

namespace Render
{

	class TextureAtlas
	{
	public:
		TextureAtlas();

		bool initialize(ETexture::Format format, int w, int h, int border = 0);
		void finalize();

		int  addImageFile(char const* path);
		bool addImageFile(int id , char const* path);
		int  addImage(int w, int h, ETexture::Format format, void* data, int dataImageWidth = 0);
		bool addImage(int id, int w, int h, ETexture::Format format, void* data, int dataImageWidth = 0)
		{
			return addImageInteranl(id, w, h, format, data, dataImageWidth);
		}

		bool addImageInteranl(int id, int w, int h, ETexture::Format format, void* data, int dataImageWidth);
		void getRectUV(int id, Vector2& outMin, Vector2& outMax) const;

		void getRectUVChecked(int id, Vector2& outMin, Vector2& outMax) const;
		void getRectUVSizeChecked(int id, Vector2& outPos, Vector2& outSize) const;
		bool getRectSize(int id, IntVector2& outSize ) const;
		IntVector2 getRectSizeChecked(int id) const;



		RHITexture2D& getTexture() { return *mTexture; }
		int  getTextureNum() const { return mNextImageId; }

		float calcUsageAreaRatio();


		int              mBorder;
		int              mNextImageId;
		RHITexture2DRef  mTexture;
		Vector2          mInvSize;
		ImageMergeHelper mHelper;
	};

}//namespace Render

#endif // TextureAtlas_H_991594F1_1108_4346_B634_187B185F2B8D
