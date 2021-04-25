#include "TestStageHeader.h"

#include "BitmapDC.h"

#include "RHI/RHIGraphics2D.h"

#include "RHI/Font.h"
#include "GameRenderSetup.h"
#include "FileSystem.h"


using namespace Render;

class FontDrawTestStage : public StageBase
	                    , public IGameRenderSetup
{
	using BaseClass = StageBase;
public:
	FontDrawTestStage() {}


	RHITexture2DRef mTexture;


	virtual void configRenderSystem(ERenderSystem systenName, RenderSystemConfigs& systemConfigs) override
	{
		systemConfigs.numSamples = 1;
	}


	std::vector<char> textBuffer;

	bool onInit() override
	{
		if( !BaseClass::onInit() )
			return false;

		VERIFY_RETURN_FALSE(FileUtility::LoadToBuffer("Subtitle.txt", textBuffer));

		VERIFY_RETURN_FALSE(FontCharCache::Get().initialize());

		{
			FontFaceInfo fontFace;
			fontFace.name = "微軟正黑體";
			fontFace.size = 8;
			VERIFY_RETURN_FALSE(mCharDataSet = FontCharCache::Get().getCharDataSet(fontFace));
		}
		{
			FontFaceInfo fontFace;
			fontFace.name = "微軟正黑體";
			fontFace.size = 1000;
			VERIFY_RETURN_FALSE( mBigCharDataSet = FontCharCache::Get().getCharDataSet(fontFace) );

			CharImageData data;
			VERIFY_RETURN_FALSE( mBigCharDataSet->mProvider->getCharData(L'冏', data) );

			int i = 1;
		}


		Color4f colors[] =
		{
			Color4f(0,0,0) ,Color4f(1,0,0) ,
			Color4f(0,1,0) ,Color4f(1,1,1) ,
		};
		VERIFY_RETURN_FALSE( mTexture = RHICreateTexture2D(ETexture::FloatRGBA, 2, 2, 1, 1, BCF_DefalutValue, colors) );

		charData = mCharDataSet->findOrAddChar(L'籖');
		charData = mCharDataSet->findOrAddChar(L'H');

		::Global::GUI().cleanupWidget();
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
	void drawText( RHICommandList& commandList, Vec2i const& pos, wchar_t const* str)
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
				curPos.y += mCharDataSet->getFontHeight() + 2;
				bStartChar = true;
				continue;
			}

			CharDataSet::CharData const& data = mCharDataSet->findOrAddChar(c);

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
			RHISetBlendState(commandList, TStaticBlendState< CWM_RGBA , EBlend::SrcAlpha, EBlend::OneMinusSrcAlpha >::GetRHI());
			{
				glEnable(GL_TEXTURE_2D);
				GL_SCOPED_BIND_OBJECT(mCharDataSet->getTexture());
				TRenderRT< RTVF_XY_T2 >::Draw(commandList, EPrimitive::Quad, &mBuffer[0], mBuffer.size());
				glDisable(GL_TEXTURE_2D);
			}
			RHISetBlendState(commandList, TStaticBlendState<>::GetRHI());
		}
			
	}

	CharDataSet* mCharDataSet;
	CharDataSet* mBigCharDataSet;
	CharDataSet::CharData charData;

	void onEnd() override
	{
		BaseClass::onEnd();
	}

	void restart() {}
	void tick() {}
	void updateFrame(int frame) {}

	void onUpdate(long time) override
	{
		BaseClass::onUpdate(time);

		int frame = time / gDefaultTickTime;
		for( int i = 0; i < frame; ++i )
			tick();

		updateFrame(frame);
	}

	void onRender(float dFrame) override
	{
		RHIGraphics2D& g = ::Global::GetRHIGraphics2D();
		RHICommandList& commandList = RHICommandList::GetImmediateList();
		g.beginRender();

		RHIClearRenderTargets(commandList, EClearBits::Color, &LinearColor(1, 1, 0.2, 1), 1);

		DrawUtility::DrawTexture(commandList, g.getBaseTransform(), mCharDataSet->getTexture(), Vec2i(0, 0), Vec2i(1024, 1024));
		g.restoreRenderState();

		wchar_t const* str =
			L"作詞：陳宏宇作曲：G.E.M. 編曲：Lupo Groinig 監製：Lupo Groinig\n"
			"你對愛並不了解　誤會愛的分類\n"
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
#if 1
		glColor4f(1, 0.5, 0, 1);
		drawText(commandList, Vec2i(100, 50), str);

#else
		g.drawText(Vec2i(0, 0), str);
#endif

		g.beginClip(Vec2i(50, 50), Vec2i(100, 100));
		g.setBrush(Color3f(1, 0, 0));
		g.drawRect(Vec2i(0, 0), ::Global::GetScreenSize());
		g.endClip();

		glColor3f(1, 1, 1);
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
		//TRenderRT<RTVF_XY>::DrawIndexed(commandList, EPrimitive::TriangleList, vertices, 4, indices, 6);
		//DrawUtility::DrawTexture(commandList, *mTexture, TStaticSamplerState< Sampler::eBilinear , Sampler::eClamp , Sampler::eClamp >::GetRHI() , Vec2i(0, 0), Vec2i(200, 200));
		g.endRender();

	}

	bool onMouse(MouseMsg const& msg) override
	{
		if( !BaseClass::onMouse(msg) )
			return false;
		return true;
	}

	bool onKey(KeyMsg const& msg) override
	{
		if( !msg.isDown())
			return false;
		switch(msg.getCode())
		{
		case EKeyCode::R: restart(); break;
		case EKeyCode::F5: glEnable(GL_MULTISAMPLE); break;
		case EKeyCode::F6: glDisable(GL_MULTISAMPLE); break;
		}
		return false;
	}

	bool onWidgetEvent(int event, int id, GWidget* ui) override
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

REGISTER_STAGE("Font Draw", FontDrawTestStage, EStageGroup::FeatureDev, "Render");