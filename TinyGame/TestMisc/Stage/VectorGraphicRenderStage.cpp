
#include "Stage/TestStageHeader.h"
#include "GameRenderSetup.h"
#include "RHI/RHICommand.h"
#include "RHI/RHIGraphics2D.h"
#include "RHI/Font.h"
#include "Math/GeometryPrimitive.h"
#include "Renderer/RenderTransform2D.h"
#include "FileSystem.h"

#include <cstring>
#include "ProfileSystem.h"

#include "Renderer/TrueType.h"

namespace VGR
{
	using namespace Render;

	class TestStage : public StageBase
	                , public IGameRenderSetup
	{
		using BaseClass = StageBase;
	public:
		TestStage() {}

		TArray< CurveSegment > mSegments;

		bool onInit() override
		{
			if (!BaseClass::onInit())
				return false;
			::Global::GUI().cleanupWidget();

			if (!mTTFLoader.load("C:/Windows/Fonts/arial.ttf"))
				return false;

			TrueTypeFileLoader::FontMetrics const& metrics = mTTFLoader.getFontMetrics();
			if (metrics.unitsPerEm == 0)
				return false;


			provider.initialize("arial-ttf", mTTFLoader, 48, 4);
			drawer.initialize(provider);

			auto frame = WidgetUtility::CreateDevFrame();
			FWidgetProperty::Bind(frame->addSlider("Size") , mSettings.size , 1 , 20, [&](float value)
			{
				rasterize(mSettings);
			});

			restart();
			return true;
		}


		TrueTypeCharDataProvider provider;

		FontDrawer drawer;


		void onEnd() override
		{
			BaseClass::onEnd();
		}

		void restart() 
		{
			mSegments.clear();

			if (loadFontGlyph())
			{

			}
			else
			{
				mSegments.resize(8);
				{
					CurveSegment& segment = mSegments[0];
					segment.start = Vector2(100, 200);
					segment.control = Vector2(100, 100);
					segment.end = Vector2(200, 100);
				}
				{
					CurveSegment& segment = mSegments[1];
					segment.start = Vector2(200, 100);
					segment.control = Vector2(300, 100);
					segment.end = Vector2(300, 200);
				}
				{
					CurveSegment& segment = mSegments[2];
					segment.start = Vector2(300, 200);
					segment.control = Vector2(300, 300);
					segment.end = Vector2(200, 300);
				}
				{
					CurveSegment& segment = mSegments[3];
					segment.start = Vector2(200, 300);
					segment.control = Vector2(100, 300);
					segment.end = Vector2(100, 200);
				}

				Vector2 offset = Vector2(50, 50);
				{
					CurveSegment& segment = mSegments[4];
					segment.start = Vector2(100, 150) + offset;
					segment.control = Vector2(100, 100) + offset;
					segment.end = Vector2(150, 100) + offset;
				}
				{
					CurveSegment& segment = mSegments[5];
					segment.start = Vector2(150, 100) + offset;
					segment.control = Vector2(150, 250);
					segment.end = Vector2(200, 150) + offset;
				}
				{
					CurveSegment& segment = mSegments[6];
					segment.start = Vector2(200, 150) + offset;
					segment.control = Vector2(300, 300);
					segment.end = Vector2(150, 200) + offset;
				}
				{
					CurveSegment& segment = mSegments[7];
					segment.start = Vector2(150, 200) + offset;
					segment.control = Vector2(100, 300);
					segment.end = Vector2(100, 150) + offset;
				}

				mSettings.bound.min = Vector2(50, 50);
				mSettings.bound.max = Vector2(350, 350);
			}



			rasterize(mSettings);
		}

		void appendGlyphSegments(TrueTypeFileLoader::GlyphData const& glyph, Vector2 const& offset, float scale, TArray< CurveSegment >& outSegments)
		{
			for (CurveSegment const& srcSegment : glyph.segments)
			{
				auto TransformPos = [&](Vector2 const& pos)
				{
					return Vector2(offset.x + scale * pos.x, offset.y - scale * pos.y);
				};

				CurveSegment segment;
				segment.start = TransformPos(srcSegment.start);
				segment.control = TransformPos(srcSegment.control);
				segment.end = TransformPos(srcSegment.end);
				outSegments.push_back(segment);
			}
		}

		bool loadFontGlyph()
		{
			auto glyph = getGlyhData('R');
			if ( glyph == nullptr)
				return false;

			TrueTypeFileLoader::FontMetrics const& metrics = mTTFLoader.getFontMetrics();
			float scale = 220.0f / metrics.unitsPerEm;
			Vector2 offset = Vector2(100, 320);

			mSegments.reserve(glyph->segments.size());
			appendGlyphSegments(*glyph, offset, scale, mSegments);

			if (mSegments.empty())
				return false;

			Math::TAABBox< Vector2 > bound;
			bound.min = offset + scale * Vector2(glyph->bound.min.x, -glyph->bound.max.y);
			bound.max = offset + scale * Vector2(glyph->bound.max.x, -glyph->bound.min.y);

			mSettings.bound = bound;
			//Vector2 padding(20, 20);
			//mSettings.bound.min -= padding;
			//mSettings.bound.max += padding;
			return true;
		}

		TrueTypeFileLoader::GlyphData* getGlyhData(uint32 c)
		{
			auto iter = mGlyphMap.find(c);
			if (iter != mGlyphMap.end())
				return &iter->second;

			TrueTypeFileLoader::GlyphData glyph;
			if (!mTTFLoader.loadGlyph(c, glyph))
				return nullptr;


			int segmentCount = glyph.segments.size();
			for (int i = 0; i < segmentCount; ++i)
			{
				CurveSegment other;
				if (glyph.segments[i].trySplitYMonotonic(other))
				{
					glyph.segments.push_back(other);
				}
			}
			auto& pair = mGlyphMap.emplace(uint32(c), std::move(glyph));
			return &pair.first->second;
		}

		void drawText(RHIGraphics2D& g, float size, char const* text)
		{
			TrueTypeFileLoader::FontMetrics const& metrics = mTTFLoader.getFontMetrics();
			float scale = size / metrics.unitsPerEm;

			Vector2 textPos = Vector2::Zero();

			g.scaleXForm(scale, -scale);
			for (char const* ch = text; *ch; ++ch)
			{
				TrueTypeFileLoader::GlyphData* glyph = getGlyhData(*ch);

				if (glyph->bHasOutline)
				{
					g.pushXForm();
					g.translateXForm(textPos.x, textPos.y);
					for (auto& segment : glyph->segments)
					{
						draw(g, segment, 24);
					}
					g.popXForm();
				}

				textPos.x += glyph->advanceWidth;
			}
		}

		std::unordered_map< uint32, TrueTypeFileLoader::GlyphData > mGlyphMap;
		TrueTypeFileLoader mTTFLoader;
		RHITexture2DRef mFontTexture;


		void drawDebug(RHIGraphics2D& g, CurveSegment const& segment)
		{
			RenderUtility::SetBrush(g, EColor::Null);

			Vector2 size = Vector2(4, 4);
			RenderUtility::SetPen(g, EColor::Red);
			g.drawRect(segment.start - 0.5 * size, size);
			g.drawRect(segment.end - 0.5 * size, size);
			RenderUtility::SetPen(g, EColor::Blue);
			g.drawRect(segment.control - 0.5 * size, size);
		}

		void draw(RHIGraphics2D& g, CurveSegment const& segment, int num)
		{
			float delta = 1.0f / num;
			Vector2 p0 = segment.get(0);

			float t = 0;
			for (int i = 0; i < num; ++i)
			{
				t += delta;
				Vector2 p1 = segment.get(t);
				g.drawLine(p0, p1);
				p0 = p1;
			}
		}


		void onUpdate(GameTimeSpan deltaTime) override
		{
			BaseClass::onUpdate(deltaTime);
		}

		void onRender(float dFrame) override
		{
			RHIGraphics2D& g = Global::GetRHIGraphics2D();
			auto& commandList = RHICommandList::GetImmediateList();
			RHISetFrameBuffer(commandList, nullptr);
			RHIClearRenderTargets(commandList, EClearBits::All, &LinearColor(0.2, 0.2, 0.2, 1), 1);

			g.beginRender();

			RenderUtility::SetBrush(g, EColor::Purple);
			for (auto const& pos : mRasterDebugPosList)
			{
				Vector2 size = 0.5 * Vector2(mSettings.size, mSettings.size);
				RenderUtility::SetPen(g, EColor::Null);
				g.drawRect(pos - 0.5 * size, size);
			}

			for (auto& segment : mSegments)
			{
				RenderUtility::SetPen(g, EColor::Yellow);
				draw(g, segment, 32);
				drawDebug(g, segment);
			}



			RenderUtility::SetBrush(g, EColor::Null);
			for (auto const& pos : mDebugPosList)
			{
				Vector2 size = Vector2(4, 4);
				RenderUtility::SetPen(g, EColor::Green);
				g.drawRect(pos - 0.5 * size, size);
			}

			g.pushXForm();
			char const* text = "CurveSegment Text";
			g.translateXForm(80, 520);
			drawText(g, 72, text);
			g.popXForm();


			RenderUtility::SetBrush(g, EColor::White);
			g.setBlendState(ESimpleBlendMode::Translucent);
			g.setBlendAlpha(1.0f);
			g.drawTexture(*mFontTexture, Vector2(300,150), Vector2(200,200));



			g.setFont(drawer);
			g.drawText(Vector2(0, 400), L"ABCDEFG@");


			RenderUtility::SetFont(g, FONT_S24);
			g.drawText(Vector2(0,0), L"ABC¤j®a¦n");



			g.endRender();
		}


		TArray< Vector2 > mDebugPosList;
		TArray< Vector2 > mRasterDebugPosList;
		TArray< CurveSegment > mTextSegments;

		RasterizeSettings mSettings;

		void rasterize(RasterizeSettings const& settings)
		{
			mRasterDebugPosList.clear();
			Rasterize(settings, mSegments, [this, &settings](int y, int xStart, int xEnd)
			{
				float yPos = settings.bound.min.y + (float(y) + 0.5f) * settings.size;
				for (int x = xStart; x <= xEnd; ++x)
				{
					float xPos = settings.bound.min.x + (float(x) + 0.5f) * settings.size;
					mRasterDebugPosList.push_back(Vector2(xPos, yPos));
				}
			});


			Vec2i texSize;
			Vector2 boundSize = settings.bound.getSize();
			texSize.x = Math::CeilToInt(boundSize.x / settings.size);
			texSize.y = Math::CeilToInt(boundSize.y / settings.size);


			TArray< Color4ub > fontData;
			fontData.resize(texSize.x * texSize.y, Color4ub(0,0,0,0));
			RasterizeSettings texSettings = settings;
			int const sampleFactor = 4;
			texSettings.size /= sampleFactor;

			{
				TIME_SCOPE("Rasterize");
				Rasterize(texSettings, mSegments, [&](int sampleY, int sampleXStart, int sampleXEnd)
				{
					int texY = sampleY / sampleFactor;
					if (texY < 0 || texY >= texSize.y)
						return;

					VisitSampleSpanCoverage(sampleXStart, sampleXEnd, sampleFactor, texSize.x, [&](int texX, int coverage)
					{
						fontData[texX + texY * texSize.x].r += uint8(coverage);
					});
				});
			}

			float factor = 1.0f / (sampleFactor * sampleFactor);
			for (auto& c : fontData)
			{
				uint8 gray = uint8(255.0f * (float(c.r) * factor));
				c = Color4ub(255, 255, 255, gray);
			}

			mFontTexture = RHICreateTexture2D(TextureDesc::Type2D(ETexture::RGBA8, texSize.x, texSize.y), fontData.data());
		}



		MsgReply onMouse(MouseMsg const& msg) override
		{
			if (msg.onLeftDown())
			{
				Vector2 pos = msg.getPos();
				mDebugPosList.clear();

				for (auto& segment : mSegments)
				{
					auto AddPosConditional = [&](float t)
					{
						if (0 <= t && t <= 1.0f)
						{
							mDebugPosList.push_back(segment.get(t));
						}
					};

					float outT[2];
					if (segment.findIntersection(pos.y, outT))
					{		
						AddPosConditional(outT[0]);
						if (Math::Abs(outT[0] - outT[1]) > 1e-5)
						{
							AddPosConditional(outT[1]);
						}
					}
				}
			}
			return BaseClass::onMouse(msg);
		}

		MsgReply onKey(KeyMsg const& msg) override
		{
			if (msg.isDown())
			{
				switch (msg.getCode())
				{
				case EKeyCode::R: restart(); break;
				}
			}
			return BaseClass::onKey(msg);
		}

		bool onWidgetEvent(int event, int id, GWidget* ui) override
		{
			switch (id)
			{
			default:
				break;
			}

			return BaseClass::onWidgetEvent(event, id, ui);
		}
	protected:
	};

	REGISTER_STAGE_ENTRY("VGR Test", TestStage, EExecGroup::MiscTest);
}
