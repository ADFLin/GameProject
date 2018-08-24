#include "Font.h"

#include "PlatformConfig.h"

#if SYS_PLATFORM_WIN
#include "WindowsHeader.h"
#include "BitmapDC.h"
#endif //SYS_PLATFORM_WIN

namespace RenderGL
{
#if SYS_PLATFORM_WIN
	class GDIFontCharDataProvider : public ICharDataProvider
	{
	public:
		bool initialize( HDC hDC, char const* faceName, int size, bool bBold = true, bool bUnderLine = false);

		static void CopyImage(uint8* dest, int w, int h, int pixeSize, uint8* src, int pixelStride)
		{
			int copySize = pixeSize * w;
			int dataStride = pixelStride * pixeSize;
			for( int i = 0; i < h; ++i )
			{
				std::copy(src, src + copySize, dest);
				src += dataStride;
				dest += copySize;
			}
		}

		virtual bool getCharData(uint32 charWord, CharImageData& data);

		int      getFontHeight() const { return textureDC.getHeight(); }
		int      mSize;
		BitmapDC textureDC;
		uint8*   pDataTexture = nullptr;
		HFONT    hFont = NULL;
	};

	bool GDIFontCharDataProvider::initialize(HDC hDC , char const* faceName, int size, bool bBold /*= true*/, bool bUnderLine /*= false*/)
	{
		int fontHeight = MulDiv(size, GetDeviceCaps(hDC, LOGPIXELSY), 72);

		hFont = ::CreateFontA(
			-fontHeight,				        // Height Of Font
			0,								// Width Of Font
			0,								// Angle Of Escapement
			0,								// Orientation Angle
			bBold ? FW_BOLD : FW_NORMAL,	// Font Weight
			FALSE,							// Italic
			bUnderLine,							// Underline
			FALSE,							// Strikeout
			DEFAULT_CHARSET,			    // Character Set Identifier
			OUT_TT_PRECIS,					// Output Precision
			CLIP_DEFAULT_PRECIS,			// Clipping Precision
			ANTIALIASED_QUALITY,			// Output Quality
			FF_DONTCARE | DEFAULT_PITCH,    // Family And Pitch
			faceName);			            // Font Name

		if( hFont == NULL )
			return false;

		TEXTMETRIC tm;
		HGDIOBJ hOldFont = ::SelectObject(hDC, hFont);
		::GetTextMetrics(hDC, &tm);
		::SelectObject(hDC, hOldFont);

		BITMAPINFO& bmpInfo = *(BITMAPINFO*)(alloca(sizeof(BITMAPINFOHEADER) + sizeof(RGBQUAD) * 0));
		bmpInfo.bmiHeader.biSize = sizeof(bmpInfo.bmiHeader);
		bmpInfo.bmiHeader.biWidth = tm.tmMaxCharWidth;
		bmpInfo.bmiHeader.biHeight = -fontHeight;
		bmpInfo.bmiHeader.biPlanes = 1;
		bmpInfo.bmiHeader.biBitCount = 32;
		bmpInfo.bmiHeader.biCompression = BI_RGB;
		bmpInfo.bmiHeader.biXPelsPerMeter = 0;
		bmpInfo.bmiHeader.biYPelsPerMeter = 0;
		bmpInfo.bmiHeader.biSizeImage = 0;
		if( !textureDC.initialize(hDC, &bmpInfo, (void**)&pDataTexture) )
			return false;


		::SelectObject(textureDC, GetStockObject(BLACK_PEN));
		::SelectObject(textureDC, GetStockObject(BLACK_BRUSH));
		::SetBkMode(textureDC, TRANSPARENT);
		::SelectObject(textureDC, hFont);
		::SetTextColor(textureDC, RGB(255, 255, 255));
		mSize = size;

		return true;
	}

	bool GDIFontCharDataProvider::getCharData(uint32 charWord, CharImageData& data)
	{
		
		wchar_t charW = charWord;
#if 0
		WORD index = 0;
		int numConv = ::GetGlyphIndicesW(textureDC, &charW, 1, &index, GGI_MARK_NONEXISTING_GLYPHS);
		if( numConv == GDI_ERROR || index == 0xffff )
		{
			int i = 1;
		}
#endif

		ABCFLOAT abcFloat;
		::GetCharABCWidthsFloatW(textureDC, charW, charW, &abcFloat);
		::Rectangle(textureDC, 0, 0, textureDC.getWidth(), textureDC.getHeight());
		RECT rect;
		rect.left = -(int)abcFloat.abcfA;
		rect.right = (int)abcFloat.abcfB;
		rect.top = 0;
		rect.bottom = textureDC.getHeight();
		::DrawTextW(textureDC, &charW, 1, &rect, DT_LEFT | DT_TOP);

		data.width = (int)abcFloat.abcfB;
		data.height = textureDC.getHeight();
		data.kerning = abcFloat.abcfA;
		data.advance = abcFloat.abcfB + abcFloat.abcfC;
		data.imageWidth = data.width;
		data.imageHeight = textureDC.getHeight();
		data.pixelSize = 4;
		data.imageData.resize(data.imageWidth * data.imageHeight * data.pixelSize);
		CopyImage(&data.imageData[0], data.imageWidth, data.imageHeight, data.pixelSize, pDataTexture, textureDC.getWidth());

		return true;
	}
#endif //SYS_PLATFORM_WIN
	ICharDataProvider* ICharDataProvider::Create(FontFaceInfo const& fontFace)
	{
#if SYS_PLATFORM_WIN
		GDIFontCharDataProvider* result = new GDIFontCharDataProvider;
		if( !result->initialize( FontCharCache::Get().hDC , fontFace.name.c_str(), fontFace.size , fontFace.bBold , fontFace.bUnderLine ) )
		{
			delete result;
			return nullptr;
		}
		return result;
#else
#error No Impl!
#endif
	}

#if CORE_SHARE_CODE
	FontCharCache& FontCharCache::Get()
	{
		static FontCharCache sInstance;
		return sInstance;
	}
#endif //CORE_SHARE_CODE

	bool FontCharCache::initialize()
	{
		if( !bInitialized )
		{
			if( !mTextAtlas.initialize(Texture::eRGBA8, 1024, 1024, 1) )
				return false;

			{
				GL_BIND_LOCK_OBJECT(mTextAtlas.getTexture());
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_A, GL_RED);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_G, GL_RED);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_B, GL_RED);
			}

			for( auto& pair : mCharDataSetMap )
			{
				pair.second->mUsedTextAtlas = &mTextAtlas;
			}

			bInitialized = true;
		}

		return true;
	}

	void FontCharCache::finalize()
	{
		releaseRHI();
		for( auto& pair : mCharDataSetMap )
		{
			delete pair.second;
		}
		mCharDataSetMap.clear();
	}

	void FontCharCache::releaseRHI()
	{
		mTextAtlas.finalize();
		for( auto& pair : mCharDataSetMap )
		{
			pair.second->clearRHIResource();
		}
		bInitialized = false;
	}

	CharDataSet* FontCharCache::getCharDataSet(FontFaceInfo const& fontFace)
	{
		auto iter = mCharDataSetMap.find(fontFace);
		if( iter == mCharDataSetMap.end() )
		{
			CharDataSet* dataSet = new CharDataSet;
			if( !dataSet->initialize(fontFace) )
			{
				LogWarning(0, "Can't Init Char Data Set");
				delete dataSet;
				return nullptr;
			}
			dataSet->mUsedTextAtlas = &mTextAtlas;
			mCharDataSetMap.insert({ fontFace , dataSet });
			return dataSet;
		}
		return iter->second;
	}

	bool CharDataSet::initialize(FontFaceInfo const& font)
	{
		mProvider = ICharDataProvider::Create(font);
		if( mProvider == nullptr )
			return false;

		return true;
	}

	void CharDataSet::clearRHIResource()
	{
		mCharMap.clear();
		mUsedTextAtlas = nullptr;
	}

	CharDataSet::CharData const& CharDataSet::findOrAddChar(uint32 charWord)
	{
		auto iter = mCharMap.find(charWord);
		if( iter != mCharMap.end() )
			return iter->second;

		CharImageData imageData;
		mProvider->getCharData(charWord, imageData);

		CharData& charData = mCharMap[charWord];
		charData.width = imageData.width;
		charData.height = imageData.height;
		if( imageData.width != imageData.imageWidth )
		{
			charData.atlasId = mUsedTextAtlas->addImage(imageData.width, imageData.height, Texture::eRGBA8, &imageData.imageData[0], imageData.imageWidth);
		}
		else
		{
			charData.atlasId = mUsedTextAtlas->addImage(imageData.width, imageData.height, Texture::eRGBA8, &imageData.imageData[0]);
		}

		charData.advance = imageData.advance;
		charData.kerning = imageData.kerning;
		mUsedTextAtlas->getRectUV(charData.atlasId, charData.uvMin, charData.uvMax);

		return charData;
	}


	FontDrawer::FontDrawer()
	{
		mSize = 0;
	}


	FontDrawer::~FontDrawer()
	{
		cleanup();
	}

	void FontDrawer::draw(Vector2 const& pos, char const* str)
	{
		assert(isValid());
		if( str == nullptr )
			return;

		std::wstring text = FCString::CharToWChar(str);
		drawImpl(pos, text.c_str());
	}

	void FontDrawer::draw(Vector2 const& pos, wchar_t const* str)
	{
		assert(isValid());
		if( str == nullptr )
			return;
		drawImpl(pos, str);
	}

	Vector2 FontDrawer::calcTextExtent(wchar_t const* str)
	{
		assert(isValid());

		if( str == nullptr )
			return Vector2(0, 0);

		Vector2 result = Vector2(0, 0);
		bool bPrevSpace = false;
		bool bStartChar = true;

		float curPosX = 0;
		float curPosY = 0;
		while( *str != 0 )
		{
			wchar_t c = *(str++);

			if( false && c == L'\n' )
			{
				result.x = std::max(result.x, curPosX);
				curPosX = 0;
				result.y += curPosY + 2;
				bStartChar = true;
				continue;
			}

			CharDataSet::CharData const& data = mCharDataSet->findOrAddChar(c);

			if( !(bPrevSpace || bStartChar) )
				curPosX += data.kerning;

			curPosY = std::max(curPosY, float(data.height));
			curPosX += data.advance;
			bStartChar = false;
			if( c == iswspace(c) )
			{
				bPrevSpace = true;
			}
		}

		result.x = std::max(result.x, curPosX);
		result.y += curPosY;
		return result;
	}

	Vector2 FontDrawer::calcTextExtent(char const* str)
	{
		assert(isValid());
		if( str == nullptr )
			return Vector2(0, 0);
		std::wstring text = FCString::CharToWChar(str);
		return calcTextExtent(text.c_str());
	}

	void FontDrawer::drawImpl(Vector2 const& pos, wchar_t const* str)
	{
		mBuffer.clear();
		Vector2 curPos = pos;
		bool bPrevSpace = false;
		bool bStartChar = true;

		while ( *str != 0 )
		{
			wchar_t c = *(str++);
			if( false && c == L'\n' )
			{
				curPos.x = pos.x;
				curPos.y += mCharDataSet->getFontHeight() + 2;
				bStartChar = true;
				continue;
			}

			CharDataSet::CharData const& data = mCharDataSet->findOrAddChar(c);

			if( !(bPrevSpace || bStartChar) )
				curPos.x += data.kerning;

			addQuad(curPos, Vector2(data.width, data.height), data.uvMin, data.uvMax);
			curPos.x += data.advance;
			bStartChar = false;
			if( c == iswspace(c) )
			{
				bPrevSpace = true;
			}
		}

		if( !mBuffer.empty() )
		{
			RHISetBlendState(TStaticBlendState< CWM_RGBA, Blend::eSrcAlpha, Blend::eOneMinusSrcAlpha >::GetRHI());
			{
				glEnable(GL_TEXTURE_2D);
				GL_BIND_LOCK_OBJECT(mCharDataSet->getTexture());
				TRenderRT< RTVF_XY_T2 >::Draw(PrimitiveType::Quad, &mBuffer[0], mBuffer.size());
				glDisable(GL_TEXTURE_2D);
			}
			RHISetBlendState(TStaticBlendState<>::GetRHI());
		}
	}

	void FontDrawer::addQuad(Vector2 const& pos, Vector2 const& size, Vector2 const& uvMin, Vector2 const& uvMax)
	{
		Vector2 posMax = pos + size;
		mBuffer.push_back({ pos , uvMin });
		mBuffer.push_back({ Vector2(posMax.x , pos.y) , Vector2(uvMax.x , uvMin.y) });
		mBuffer.push_back({ posMax , uvMax });
		mBuffer.push_back({ Vector2(pos.x , posMax.y) , Vector2(uvMin.x , uvMax.y) });
	}

	bool FontDrawer::initialize(FontFaceInfo const& fontFace)
	{
		if( !FontCharCache::Get().initialize() )
			return false;

		mCharDataSet = FontCharCache::Get().getCharDataSet(fontFace);
		if( mCharDataSet == nullptr )
			return false;

		mSize = fontFace.size;
		return true;
	}

	void FontDrawer::cleanup()
	{
		mCharDataSet = nullptr;
		mSize = 0;
	}

}//namespace RenderGL

