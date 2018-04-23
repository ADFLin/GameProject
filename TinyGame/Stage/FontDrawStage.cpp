#include "TestStageHeader.h"

#include "BitmapDC.h"

#include "GLGraphics2D.h"

#include "RenderGL/TextureAtlas.h"
#include "RenderGL/DrawUtility.h"
#include <unordered_map>

namespace RenderGL
{
	struct CharImageData
	{
		int    width;
		int    height;
		float  kerning;
		float  advance;
		int    imageWidth;
		int    imageHeight;
		int    pixelByte;
		std::vector<uint8> imageData;
	};

	class ICharDataProvider
	{
	public:
		~ICharDataProvider(){}
		virtual bool getCharData(uint32 charWord, CharImageData& data) = 0;
	};


	class CharDataSet
	{
	public:
		struct CharData
		{
			int     width;
			int     height;
			float   kerning;
			float   advance;
			int     atlasId;
			Vector2 uvMin;
			Vector2 uvMax;
		};

		CharData const& findOrAddChar(uint32 charWord, ICharDataProvider& provider)
		{
			auto iter = mCharMap.find(charWord);
			if( iter != mCharMap.end() )
				return iter->second;

			CharImageData imageData;
			provider.getCharData(charWord, imageData);

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

		uint32        UsedAtlasId;
		TextureAtlas* mUsedTextAtlas;
		std::unordered_map< uint32, CharData > mCharMap;
	};

	struct FontInfo
	{
		int  size;
		bool bBold;
		bool bUnderLine;
	};

	class GDIFontCharDataProvider : public ICharDataProvider
	{
	public:
		bool initialize(char const* faceName , int size , bool bBold = true , bool bUnderLine = false)
		{

			int height = (int)(fabs((float)10 * size *GetDeviceCaps(NULL, LOGPIXELSY) / 72) / 10.0 + 0.5);

			hFont = ::CreateFontA(
				-height,				        // Height Of Font
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

			static HDC tempDC = NULL;
			if( tempDC == NULL )
			{
				tempDC = ::CreateCompatibleDC(NULL);
			}

			TEXTMETRIC tm;
			HGDIOBJ hOldFont = ::SelectObject(tempDC, hFont);
			::GetTextMetrics(tempDC, &tm);
			::SelectObject(tempDC, hOldFont);


			BITMAPINFO bmpInfo;
			bmpInfo.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
			bmpInfo.bmiHeader.biWidth = 2 * size;
			bmpInfo.bmiHeader.biHeight = -tm.tmHeight;
			bmpInfo.bmiHeader.biPlanes = 1;
			bmpInfo.bmiHeader.biBitCount = 32;
			bmpInfo.bmiHeader.biCompression = BI_RGB;
			bmpInfo.bmiHeader.biXPelsPerMeter = 0;
			bmpInfo.bmiHeader.biYPelsPerMeter = 0;
			bmpInfo.bmiHeader.biSizeImage = 0;
			if( !textureDC.initialize(NULL, &bmpInfo, (void**)&pDataTexture) )
				return false;


			::SelectObject(textureDC, GetStockObject(BLACK_PEN));
			::SelectObject(textureDC, GetStockObject(BLACK_BRUSH));
			::SetBkMode(textureDC, TRANSPARENT);
			::SelectObject(textureDC, hFont);
			::SetTextColor(textureDC, RGB(255, 255, 255));
			mSize = size;
			
			return true;

		}

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

		virtual bool getCharData(uint32 charWord, CharImageData& data)
		{
			ABCFLOAT abcFloat;
			::GetCharABCWidthsFloatW(textureDC , charWord, charWord, &abcFloat);
			::Rectangle(textureDC , 0, 0, textureDC.getWidth(), textureDC.getHeight());
			RECT rect;
			rect.left = -(int)abcFloat.abcfA;
			rect.right = (int)abcFloat.abcfB;
			rect.top = 0;
			rect.bottom = textureDC.getHeight();
			wchar_t charW = charWord;
			::DrawTextW(textureDC , &charW, 1, &rect, DT_LEFT | DT_TOP );

			data.width = (int)abcFloat.abcfB;
			data.height = textureDC.getHeight();
			data.kerning = abcFloat.abcfA;
			data.advance = abcFloat.abcfB + abcFloat.abcfC;
			data.imageWidth = data.width;
			data.imageHeight = textureDC.getHeight();
			data.pixelByte = 4;
			data.imageData.resize(data.imageWidth * data.imageHeight * data.pixelByte);
			CopyImage( &data.imageData[0] , data.imageWidth  , data.imageHeight , data.pixelByte, pDataTexture ,  textureDC.getWidth() );

			return true;
		}

		int      getFontHeight() const { return textureDC.getHeight(); }
		int      mSize;
		BitmapDC textureDC;
		uint8*   pDataTexture = nullptr;
		HFONT    hFont = NULL;
	};

}

using namespace RenderGL;

class FontDrawTestStage : public StageBase
{
	typedef StageBase BaseClass;
public:
	FontDrawTestStage() {}


	virtual bool onInit()
	{
		if( !BaseClass::onInit() )
			return false;

		::Global::GUI().cleanupWidget();

		if( !::Global::getDrawEngine()->startOpenGL(true , 4) )
			return false;

		if( !provider.initialize("微軟正黑體", 12) )
			return false;

		if( !mFontTextureAtlas.create(Texture::eRGB8, 1024, 1024, 1) )
			return false;

		mCharDataSet.mUsedTextAtlas = &mFontTextureAtlas;

		charData = mCharDataSet.findOrAddChar(L'大', provider);
		charData = mCharDataSet.findOrAddChar(L'H', provider);


		restart();
		return true;
	}

	struct TextVertex
	{
		Vector2 pos;
		Vector2 uv;
	};

	std::vector< TextVertex > mBuffer;
	void addQuad(Vector2 const& pos, Vector2 const& size, Vector2 const& uvMin, Vector2 const& uvMax)
	{
		Vector2 posMax = pos + size;
		mBuffer.push_back({ pos , uvMin });
		mBuffer.push_back({ Vector2( posMax.x , pos.y ) , Vector2( uvMax.x , uvMin.y) });
		mBuffer.push_back({ posMax , uvMax });
		mBuffer.push_back({ Vector2(pos.x , posMax.y) , Vector2(uvMin.x , uvMax.y) });
	}
	void drawText(Vec2i const& pos, wchar_t const* str)
	{
		mBuffer.clear();
		Vector2 curPos = pos;

		bool bPrevSpace = false;
		bool bStartChar = true;
		while( *str != 0 )
		{
			wchar_t c = *(str++);
			if( c == L'\n' )
			{
				curPos.x = pos.x;
				curPos.y += provider.getFontHeight() + 2;
				bStartChar = true;
				continue;
			}

			CharDataSet::CharData const& data = mCharDataSet.findOrAddChar(c, provider);

			if ( !( bPrevSpace || bStartChar ) )
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
			glEnable(GL_TEXTURE_2D);
			glEnable(GL_BLEND);
			glBlendFunc(GL_SRC_COLOR, GL_ONE_MINUS_SRC_COLOR);
			{
				GL_BIND_LOCK_OBJECT(mFontTextureAtlas.getTexture());
				glColor3f(1, 1, 1);
				TRenderRT< RTVF_XY_T2 >::Draw(PrimitiveType::eQuad, &mBuffer[0], mBuffer.size());
			}
			glDisable(GL_BLEND);
			glDisable(GL_TEXTURE_2D);

		}
			
	}

	TextureAtlas mFontTextureAtlas;
	GDIFontCharDataProvider provider;
	CharDataSet mCharDataSet;
	CharDataSet::CharData charData;

	virtual void onEnd()
	{
		BaseClass::onEnd();
	}

	void restart() {}
	void tick() {}
	void updateFrame(int frame) {}

	virtual void onUpdate(long time)
	{
		BaseClass::onUpdate(time);

		int frame = time / gDefaultTickTime;
		for( int i = 0; i < frame; ++i )
			tick();

		updateFrame(frame);
	}

	void onRender(float dFrame)
	{
		GLGraphics2D& g = ::Global::getGLGraphics2D();
		g.beginRender();

		glClear(GL_COLOR_BUFFER_BIT);

		DrawUtility::DrawTexture(mFontTextureAtlas.getTexture(), Vec2i(0, 0), Vec2i(400, 400));

		wchar_t const* str =
			L"作詞：陳宏宇作曲：G.E.M. 編曲：Lupo Groinig 監製：Lupo Groinig\n"
			"你對愛並不了解　誤會愛的分類\n"
			"TATaTA\n" 
			"What have you done? What have you done?\n"
			"把最珍貴的玫瑰　當作荒野薔薇\n"
			"相信了你的承諾　讓愛蒙蔽我的眼\n"
			"塞納河的光線　照不亮心的那邊　背叛的終點\n"
			"線索己那麼明顯　你的愛是欺騙\n"
			"我在一千零一夜　守著錯誤的籖\n"
			"真相似毒刺漫延　心被扎得出血\n"
			"你和我的愛之間　主角是背叛　虛偽的表演\n"
			"在塞納河上的玫瑰　它的養份是我的淚\n"
			"花瓣的顏色愈妖艷　我郤愈憔悴\n"
			"發現了自己信仰的依賴　逐漸瓦解\n"
			"永遠在瞬間被毀滅\n"
			"線索己那麼明顯　你的愛是欺騙\n"
			"我在一千零一夜　守著錯誤的籖\n"
			"真相似毒刺漫延　心被扎得出血\n"
			"痛與愛難分難解　因為我已經為你　忘了我自己\n"
			"在塞納河上的玫瑰　它的養份是我的淚\n"
			"花瓣的顏色愈妖艷　我郤愈憔悴\n"
			"它緩緩落下的露水　從來沒人在意\n"
			"哪一顆是我的眼淚\n"
			"愛　會讓我痛\n"
			"我　卻不懂恨\n"
			"在塞納河上的玫瑰　它的養份是我的淚\n"
			"原來在最美的時刻　要讓它心碎\n"
			"終於讓自己殘存的愛情全部揉碎\n"
			"淹沒在塞納河水\n"
			"你對愛並不了解　愛會把你忽略\n"
			"遍地野生的薔薇　不如玫瑰珍貴\n"
			"承諾要灰飛煙滅　誰還能被愛紀念\n"
			"凋謝最紅的玫瑰　眼淚化作塞納河水\n";

		drawText(Vec2i(100, 50), str);

		glColor3f(1, 0, 0);
		Vector2 vertices[] =
		{
			Vector2(100 , 100),
			Vector2(200 , 100),
			Vector2(200 , 300),
			Vector2(100 , 200),
		};
		int indices[] =
		{
			0,1,2,0,2,3,
		};
		TRenderRT<RTVF_XY>::DrawIndexed(PrimitiveType::eTriangleList, vertices, 4, indices, 6);


		g.beginClip(Vec2i(50, 50), Vec2i(100, 100));
		g.setBrush(Color3f(1, 0, 0));
		g.drawRect(Vec2i(0, 0), Global::getDrawEngine()->getScreenSize());
		g.endClip();

		g.endRender();

	}

	bool onMouse(MouseMsg const& msg)
	{
		if( !BaseClass::onMouse(msg) )
			return false;
		return true;
	}

	bool onKey(unsigned key, bool isDown)
	{
		if( !isDown )
			return false;
		switch( key )
		{
		case Keyboard::eR: restart(); break;
		case Keyboard::eF5: glEnable(GL_MULTISAMPLE); break;
		case Keyboard::eF6: glDisable(GL_MULTISAMPLE); break;
		}
		return false;
	}

	virtual bool onWidgetEvent(int event, int id, GWidget* ui) override
	{
		switch( id )
		{
		default:
			break;
		}

		return BaseClass::onWidgetEvent(event, id, ui);
	}
protected:
};

REGISTER_STAGE("Font Draw", FontDrawTestStage, EStageGroup::FeatureDev);