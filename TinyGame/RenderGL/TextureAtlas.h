#pragma once
#ifndef TextureAtlas_H_991594F1_1108_4346_B634_187B185F2B8D
#define TextureAtlas_H_991594F1_1108_4346_B634_187B185F2B8D

#include "RenderGL/GLCommon.h"

#include "Misc/ImageMergeHelper.h"

namespace RenderGL
{

	class TextureAtlas
	{
	public:
		TextureAtlas();

		bool create(Texture::Format format, int w, int h, int border = 0);

		int addImageFile(char const* path);
		int addImage(int w, int h, Texture::Format format, void* data);

		void getRectUV(int id, Vector2& outMin, Vector2& outMax);

		RHITexture2D& getTexture() { return mTexture; }
		int getTextureNum() { return mNextImageId; }

		int              mBorder;
		int              mNextImageId;
		RHITexture2D     mTexture;
		ImageMergeHelper mHelper;
	};

}//namespace RenderGL

#endif // TextureAtlas_H_991594F1_1108_4346_B634_187B185F2B8D
