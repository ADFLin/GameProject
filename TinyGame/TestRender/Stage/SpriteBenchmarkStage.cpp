#include "Stage/TestRenderStageBase.h"
#include "RHI/RHIGraphics2D.h"
#include "RHI/RHIUtility.h"
#include "RHI/GpuProfiler.h"
#include "SystemPlatform.h"

namespace Render
{
	class SpriteBenchmarkStage : public TestRenderStageBase
	{
		using BaseClass = TestRenderStageBase;
	public:
		bool onInit() override
		{
			if (!BaseClass::onInit())
				return false;

			::Global::GUI().cleanupWidget();
			mSampleFrameCount = 0;
			mSampleAccumMs = 0.0f;
			mAvgFrameMs = 0.0f;
			mLastTick = SystemPlatform::GetTickCount();
			mCpuSampleCount = 0;
			mCpuSampleAccumMs = 0.0f;
			mCpuAvgMs = 0.0f;
			mCpuMinMs = 999999.0f;
			mCpuMaxMs = 0.0f;
			mGpuSampleCount = 0;
			mGpuSampleAccumMs = 0.0f;
			mGpuAvgMs = 0.0f;
			mGpuMinMs = 999999.0f;
			mGpuMaxMs = 0.0f;
			return true;
		}

		ERenderSystem getDefaultRenderSystem() override
		{
			return ERenderSystem::D3D11;
		}

		bool setupRenderResource(ERenderSystem systemName) override
		{
			VERIFY_RETURN_FALSE(BaseClass::setupRenderResource(systemName));
			mTexture = RHIUtility::LoadTexture2DFromFile("Texture/UVChecker.png", TextureLoadOption().FlipV());
			return true;
		}

		void preShutdownRenderSystem(bool bReInit = false) override
		{
			mTexture.release();
			BaseClass::preShutdownRenderSystem(bReInit);
		}

		void onRender(float dFrame) override
		{
			initializeRenderState();

			int64 tick = SystemPlatform::GetTickCount();
			float frameMs = float(tick - mLastTick);
			mLastTick = tick;
			if (frameMs < 0.0f)
				frameMs = 0.0f;
			if (frameMs == 0.0f && dFrame > 0.0f)
				frameMs = 1000.0f * dFrame;

			mSampleAccumMs += frameMs;
			++mSampleFrameCount;
			if (mSampleFrameCount >= 30)
			{
				mAvgFrameMs = mSampleAccumMs / float(mSampleFrameCount);
				mSampleAccumMs = 0.0f;
				mSampleFrameCount = 0;
			}

			RHIGraphics2D& g = ::Global::GetRHIGraphics2D();
			g.beginRender();

			if (mTexture)
			{
				int64 cpuStartTick = SystemPlatform::GetTickCount();

				GPU_PROFILE("Sprite Render");
				g.setTexture(*mTexture);
				IntVector2 screenSize = ::Global::GetScreenSize();
				float spriteSize = float(mSpriteSize);
				int32 cols = Math::Max(1, int32(screenSize.x / spriteSize));
				int32 rows = Math::Max(1, int32(screenSize.y / spriteSize));
				int32 drawCount = mSpriteCount;

				for (int32 i = 0; i < drawCount; ++i)
				{
					int32 cx = i % cols;
					int32 cy = (i / cols) % rows;
					Vector2 pos(float(cx) * spriteSize, float(cy) * spriteSize);
					g.drawTexture(pos, Vector2(spriteSize, spriteSize), Color4f::White());
				}

				g.flush();

				float cpuMs = float(SystemPlatform::GetTickCount() - cpuStartTick);
				mCpuSampleAccumMs += cpuMs;
				++mCpuSampleCount;
				if (cpuMs < mCpuMinMs)
					mCpuMinMs = cpuMs;
				if (cpuMs > mCpuMaxMs)
					mCpuMaxMs = cpuMs;
				mCpuAvgMs = mCpuSampleAccumMs / float(mCpuSampleCount);
			}

			{
				GpuProfileReadScope readScope;
				if (readScope.isLocked())
				{
					for (int i = 0; i < GpuProfiler::Get().getSampleNum(); ++i)
					{
						GpuProfileSample* sample = GpuProfiler::Get().getSample(i);
						if (!sample)
							continue;
						if (sample->name == "Sprite Render" && sample->time >= 0.0f)
						{
							float gpuMs = sample->time;
							mGpuSampleAccumMs += gpuMs;
							++mGpuSampleCount;
							if (gpuMs < mGpuMinMs)
								mGpuMinMs = gpuMs;
							if (gpuMs > mGpuMaxMs)
								mGpuMaxMs = gpuMs;
							mGpuAvgMs = mGpuSampleAccumMs / float(mGpuSampleCount);
							break;
						}
					}
				}
			}


			Vec2i panelPos(6, 6);
			Vec2i panelSize(760, 148);
			g.beginBlend(panelPos, panelSize, 0.75f);
			RenderUtility::SetPen(g, EColor::Null);
			RenderUtility::SetBrush(g, EColor::Black);
			g.drawRoundRect(panelPos, panelSize, Vector2(6, 6));
			g.endBlend();

			RenderUtility::SetFont(g, FONT_S12);
			g.setTextColor(Color3f(1, 1, 0));

			InlineString<256> str;
			str.format("Sprite Benchmark");
			g.drawText(Vector2(10, 10), str.c_str());

			str.format("Sprites : %d  Size : %d", mSpriteCount, mSpriteSize);
			g.drawText(Vector2(10, 30), str.c_str());

			float fps = (mAvgFrameMs > 0.0f) ? (1000.0f / mAvgFrameMs) : 0.0f;
			str.format("Avg Frame : %.3f ms (%.1f FPS)", mAvgFrameMs, fps);
			g.drawText(Vector2(10, 50), str.c_str());

			str.format("[Up/Down] Sprite +/- 2000   [Left/Right] Size +/- 2   [R] Reset");
			g.drawText(Vector2(10, 70), str.c_str());

			str.format("CPU(Sprite Loop) Min/Max/Avg : %.3f / %.3f / %.3f ms", mCpuMinMs, mCpuMaxMs, mCpuAvgMs);
			g.drawText(Vector2(10, 90), str.c_str());

			str.format("GPU(Sprite Render) Min/Max/Avg : %.3f / %.3f / %.3f ms", mGpuMinMs, mGpuMaxMs, mGpuAvgMs);
			g.drawText(Vector2(10, 110), str.c_str());

			str.format("Compare by rebuilding with USE_SPRITE_RENDER = 0 / 1");
			g.drawText(Vector2(10, 130), str.c_str());

			g.endRender();
		}

		MsgReply onKey(KeyMsg const& msg) override
		{
			if (!msg.isDown())
				return BaseClass::onKey(msg);

			switch (msg.getCode())
			{
			case EKeyCode::Up:
				mSpriteCount += 2000;
				return MsgReply::Handled();
			case EKeyCode::Down:
				mSpriteCount = Math::Max(1000, mSpriteCount - 2000);
				return MsgReply::Handled();
			case EKeyCode::Left:
				mSpriteSize = Math::Max(4, mSpriteSize - 2);
				return MsgReply::Handled();
			case EKeyCode::Right:
				mSpriteSize = Math::Min(128, mSpriteSize + 2);
				return MsgReply::Handled();
			case EKeyCode::R:
				mSampleFrameCount = 0;
				mSampleAccumMs = 0.0f;
				mAvgFrameMs = 0.0f;
				mLastTick = SystemPlatform::GetTickCount();
				mCpuSampleCount = 0;
				mCpuSampleAccumMs = 0.0f;
				mCpuAvgMs = 0.0f;
				mCpuMinMs = 999999.0f;
				mCpuMaxMs = 0.0f;
				mGpuSampleCount = 0;
				mGpuSampleAccumMs = 0.0f;
				mGpuAvgMs = 0.0f;
				mGpuMinMs = 999999.0f;
				mGpuMaxMs = 0.0f;
				return MsgReply::Handled();
			}

			return BaseClass::onKey(msg);
		}

	private:
		RHITexture2DRef mTexture;
		int32 mSpriteCount = 40000;
		int32 mSpriteSize = 64;

		int32 mSampleFrameCount = 0;
		float mSampleAccumMs = 0.0f;
		float mAvgFrameMs = 0.0f;
		int64 mLastTick = 0;

		int32 mCpuSampleCount = 0;
		float mCpuSampleAccumMs = 0.0f;
		float mCpuAvgMs = 0.0f;
		float mCpuMinMs = 999999.0f;
		float mCpuMaxMs = 0.0f;

		int32 mGpuSampleCount = 0;
		float mGpuSampleAccumMs = 0.0f;
		float mGpuAvgMs = 0.0f;
		float mGpuMinMs = 999999.0f;
		float mGpuMaxMs = 0.0f;
	};

	REGISTER_STAGE_ENTRY("Sprite Benchmark", SpriteBenchmarkStage, EExecGroup::FeatureDev, "Render|Benchmark");
}
