#include "TextureAtlas.h"

#include "stb/stb_image.h"

#include "Image/ImageData.h"
#include "RHICommand.h"

namespace Render
{

	TextureAtlas::TextureAtlas()
	{
		mBorder = 0;
		mNextImageId = 0;
	}

	bool TextureAtlas::initialize(Texture::Format format, int w, int h, int border /*= 0*/)
	{
		mBorder = border;
		mTexture = RHICreateTexture2D( format , w , h );
		if( !mTexture.isValid() )
			return false;
		mHelper.init(w, h);
		return true;
	}

	void TextureAtlas::finalize()
	{
		mTexture.release();
		mHelper.clear();
	}

	int TextureAtlas::addImageFile(char const* path)
	{
		ImageData imageData;
		if (!imageData.load(path , false , false))
			return false;

		int result = -1;

		//#TODO
		switch(imageData.numComponent)
		{
		case 3:
			result = addImage(imageData.width, imageData.height, Texture::eRGB8, imageData.data);
			break;
		case 4:
			result = addImage(imageData.width, imageData.height, Texture::eRGBA8, imageData.data);
			break;
		}
		//glGenerateMipmap( texType);
		return result;
	}

	bool TextureAtlas::addImageFile(int id, char const* path)
	{
		int w;
		int h;
		int comp;
		unsigned char* image = stbi_load(path, &w, &h, &comp, STBI_default);

		if( !image )
			return false;

		int result = -1;

		Texture::Format format = Texture::eRGBA8;
		//#TODO
		switch( comp )
		{
		case 3:
			format = Texture::eRGB8;
			break;
		case 4:
			format = Texture::eRGBA8;
			break;
		default:
			return false;
		}

		if( !addImageInteranl(id, w, h, format, image, 0) )
			return false;

		if( mNextImageId <= id )
			mNextImageId = id + 1;

		return true;
	}

	int TextureAtlas::addImage(int w, int h, Texture::Format format, void* data, int dataImageWidth)
	{
		if( !addImageInteranl(mNextImageId, w, h, format, data, dataImageWidth) )
			return -1;

		int result = mNextImageId;
		++mNextImageId;
		return result;
	}

	bool TextureAtlas::addImageInteranl(int id , int w, int h, Texture::Format format, void* data, int dataImageWidth)
	{
		if( !mHelper.addImage(id, w + 2 * mBorder, h + 2 * mBorder) )
			return false;

		auto rect = mHelper.getNode(id)->rect;

		if( dataImageWidth )
		{
			mTexture->update(rect.x + mBorder, rect.y + mBorder, w, h, format, dataImageWidth, data);
		}
		else
		{
			mTexture->update(rect.x + mBorder, rect.y + mBorder, w, h, format, data);
		}

		return true;
	}

	void TextureAtlas::getRectUV(int id, Vector2& outMin, Vector2& outMax) const
	{
		auto* node = mHelper.getNode(id);
		if (node)
		{
			auto rect = node->rect;
			outMin.x = float(rect.x + mBorder) / mTexture->getSizeX();
			outMin.y = float(rect.y + mBorder) / mTexture->getSizeY();
			outMax.x = float(rect.x + rect.w - mBorder) / mTexture->getSizeX();
			outMax.y = float(rect.y + rect.h - mBorder) / mTexture->getSizeY();
		}
	}

	void TextureAtlas::getRectUVChecked(int id, Vector2& outMin, Vector2& outMax) const
	{
		auto* node = mHelper.getNode(id);
		assert(node);
		auto const& rect = node->rect;
		outMin.x = float(rect.x + mBorder) / mTexture->getSizeX();
		outMin.y = float(rect.y + mBorder) / mTexture->getSizeY();
		outMax.x = float(rect.x + rect.w - mBorder) / mTexture->getSizeX();
		outMax.y = float(rect.y + rect.h - mBorder) / mTexture->getSizeY();
	}

	bool TextureAtlas::getRectSize(int id, IntVector2& outSize) const
	{
		auto* node = mHelper.getNode(id);
		if (node == nullptr)
			return false;
		auto const& rect = node->rect;
		return IntVector2(rect.w, rect.h);
	}

	IntVector2 TextureAtlas::getRectSizeChecked(int id) const
	{
		auto* node = mHelper.getNode(id);
		assert(node);
		auto const& rect = node->rect;
		return IntVector2(rect.w, rect.h);
	}

	float TextureAtlas::calcUsageAreaRatio()
	{
		return float(mHelper.calcUsageArea()) / mHelper.getTotalArea();
	}

}//namespace Render