#include "TextureAtlas.h"

#include "stb/stb_image.h"

#include "RHICommand.h"

namespace RenderGL
{

	TextureAtlas::TextureAtlas()
	{
		mBorder = 0;
		mNextImageId = 0;
	}

	bool TextureAtlas::create(Texture::Format format, int w, int h, int border /*= 0*/)
	{
		mBorder = border;
		mTexture = RHICreateTexture2D( format , w , h );
		if( !mTexture.isValid() )
			return false;
		mHelper.init(w, h);
		return true;
	}

	int TextureAtlas::addImageFile(char const* path)
	{
		int w;
		int h;
		int comp;
		unsigned char* image = stbi_load(path, &w, &h, &comp, STBI_default);

		if( !image )
			return -1;

		int result = -1;

		//#TODO
		switch( comp )
		{
		case 3:
			result = addImage(w, h, Texture::eRGB8, image);
			break;
		case 4:
			result = addImage(w, h, Texture::eRGBA8, image);
			break;
		}
		//glGenerateMipmap( texType);
		stbi_image_free(image);
		return result;
	}

	int TextureAtlas::addImage(int w, int h, Texture::Format format, void* data, int pixelStride)
	{
		if( !mHelper.addImage(mNextImageId, w + 2 * mBorder, h + 2 * mBorder) )
			return -1;

		auto rect = mHelper.getNode(mNextImageId)->rect;

		if( pixelStride )
		{
			mTexture->update(rect.x + mBorder, rect.y + mBorder, w, h, format, pixelStride, data);
		}
		else
		{
			mTexture->update(rect.x + mBorder, rect.y + mBorder, w, h, format, data);
		}
		
		int result = mNextImageId;
		++mNextImageId;
		return result;
	}

	void TextureAtlas::getRectUV(int id, Vector2& outMin, Vector2& outMax) const
	{
		auto rect = mHelper.getNode(id)->rect;
		outMin.x = float(rect.x + mBorder) / mTexture->getSizeX();
		outMin.y = float(rect.y + mBorder) / mTexture->getSizeY();
		outMax.x = float(rect.x + rect.w - mBorder) / mTexture->getSizeX();
		outMax.y = float(rect.y + rect.h - mBorder) / mTexture->getSizeY();
	}

}//namespace RenderGL