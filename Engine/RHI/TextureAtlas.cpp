#include "TextureAtlas.h"

#include "Image/ImageData.h"
#include "RHICommand.h"

namespace Render
{

	TextureAtlas::TextureAtlas()
	{
		mBorder = 0;
		mNextImageId = 0;
	}

	bool TextureAtlas::initialize(ETexture::Format format, int w, int h, int border /*= 0*/)
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
		mNextImageId = 0;
	}

	int TextureAtlas::addImageFile(char const* path)
	{
		ImageData imageData;
		if (!imageData.load(path))
			return false;

		int result = INDEX_NONE;

		//#TODO
		switch(imageData.numComponent)
		{
		case 3:
			result = addImage(imageData.width, imageData.height, ETexture::RGB8, imageData.data);
			break;
		case 4:
			result = addImage(imageData.width, imageData.height, ETexture::RGBA8, imageData.data);
			break;
		}
		//glGenerateMipmap( texType);
		return result;
	}

	bool TextureAtlas::addImageFile(int id, char const* path)
	{
		ImageData imageData;
		if (!imageData.load(path))
			return false;

		int result = -1;
		ETexture::Format format = ETexture::RGBA8;
		//#TODO
		switch( imageData.numComponent )
		{
		case 1:
			
			break;
		case 3:
			format = ETexture::RGB8;
			break;
		case 4:
			format = ETexture::RGBA8;
			break;
		default:
			return false;
		}

		if( !addImageInteranl(id, imageData.width, imageData.height, format, imageData.data, 0) )
			return false;

		if( mNextImageId <= id )
			mNextImageId = id + 1;

		return true;
	}

	int TextureAtlas::addImage(int w, int h, ETexture::Format format, void* data, int dataImageWidth)
	{
		int result = mNextImageId;
		if( !addImageInteranl(result, w, h, format, data, dataImageWidth) )
			return INDEX_NONE;

		++mNextImageId;
		return result;
	}

	bool TextureAtlas::addImageInteranl(int id , int w, int h, ETexture::Format format, void* data, int dataImageWidth)
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
		outSize = IntVector2(rect.w - 2 * mBorder, rect.h - 2 * mBorder);
		return true;
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