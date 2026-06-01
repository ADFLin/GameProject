#include "Stage/TestRenderStageBase.h"

#include "RHI/RHIGraphics2D.h"
#include "RHI/RHICommand.h"
#include "RHI/DrawUtility.h"

#include "RenderUtility.h"
#include "WindowsPlatform.h"

#include <cstring>

namespace Render
{
	namespace
	{
		constexpr int OverlayWidth = 640;
		constexpr int OverlayHeight = 300;

		class OverlayWindow : public WinFrameT< OverlayWindow >
		{
		public:
			DWORD getWinStyle() { return WS_POPUP | WS_VISIBLE; }
			DWORD getWinExtStyle() { return WS_EX_LAYERED | WS_EX_TOPMOST | WS_EX_TOOLWINDOW; }
			LPTSTR getWinClassName() { return TEXT("DesktopOverlayTestWindow"); }

			bool setupWindow(bool fullscreen, unsigned colorBits)
			{
				sActiveWindow = this;
				SetWindowPos(getHWnd(), HWND_TOPMOST, 120, 120, getWidth(), getHeight(), SWP_SHOWWINDOW | SWP_NOACTIVATE);
				return true;
			}

			void destoryWindow()
			{
				if (sActiveWindow == this)
				{
					sActiveWindow = nullptr;
				}
			}

			void setHitTestData(TArray< uint8 > const* data)
			{
				mHitTestData = data;
			}

			bool hitTestDrawnPixel(POINT screenPos) const
			{
				if (mHitTestData == nullptr || mHitTestData->empty())
					return false;

				POINT clientPos = screenPos;
				ScreenToClient(getHWnd(), &clientPos);
				if (clientPos.x < 0 || clientPos.y < 0 || clientPos.x >= OverlayWidth || clientPos.y >= OverlayHeight)
					return false;

				int index = 4 * (clientPos.y * OverlayWidth + clientPos.x);
				uint8 alpha = (*mHitTestData)[index + 3];
				return alpha > 16;
			}

			void beginDrag(POINT clientPos)
			{
				POINT screenPos = clientPos;
				ClientToScreen(getHWnd(), &screenPos);
				if (!hitTestDrawnPixel(screenPos))
					return;

				RECT rect;
				GetWindowRect(getHWnd(), &rect);
				mDragOffset.x = screenPos.x - rect.left;
				mDragOffset.y = screenPos.y - rect.top;
				mbDragging = true;
				SetCapture(getHWnd());
			}

			void updateDrag()
			{
				if (!mbDragging)
					return;

				if ((GetKeyState(VK_LBUTTON) & 0x8000) == 0)
				{
					endDrag();
					return;
				}

				POINT screenPos;
				GetCursorPos(&screenPos);
				SetWindowPos(getHWnd(), HWND_TOPMOST,
				             screenPos.x - mDragOffset.x,
				             screenPos.y - mDragOffset.y,
				             0, 0,
				             SWP_NOSIZE | SWP_NOACTIVATE);
			}

			void endDrag()
			{
				if (mbDragging)
				{
					mbDragging = false;
					if (GetCapture() == getHWnd())
					{
						ReleaseCapture();
					}
				}
			}

			static LRESULT CALLBACK StaticWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
			{
				switch (message)
				{
				case WM_CLOSE:
					ShowWindow(hWnd, SW_HIDE);
					return 0;
				case WM_ERASEBKGND:
					return 1;
				case WM_NCHITTEST:
					if (sActiveWindow)
					{
						POINT screenPos = { int(short(LOWORD(lParam))), int(short(HIWORD(lParam))) };
						return sActiveWindow->hitTestDrawnPixel(screenPos) ? HTCLIENT : HTTRANSPARENT;
					}
					return HTTRANSPARENT;
				case WM_LBUTTONDOWN:
					if (sActiveWindow)
					{
						POINT clientPos = { int(short(LOWORD(lParam))), int(short(HIWORD(lParam))) };
						sActiveWindow->beginDrag(clientPos);
						return 0;
					}
					break;
				case WM_MOUSEMOVE:
					if (sActiveWindow)
					{
						sActiveWindow->updateDrag();
						return 0;
					}
					break;
				case WM_LBUTTONUP:
					if (sActiveWindow)
					{
						sActiveWindow->endDrag();
						return 0;
					}
					break;
				case WM_CAPTURECHANGED:
					if (sActiveWindow)
					{
						sActiveWindow->mbDragging = false;
					}
					break;
				}
				return DefWindowProc(hWnd, message, wParam, lParam);
			}

			static OverlayWindow* sActiveWindow;
			TArray< uint8 > const* mHitTestData = nullptr;
			bool mbDragging = false;
			POINT mDragOffset = { 0, 0 };
		};

		OverlayWindow* OverlayWindow::sActiveWindow = nullptr;

		Vector2 MakeCirclePoint(Vector2 const& center, float radius, float angle)
		{
			return center + radius * Vector2(Math::Cos(angle), Math::Sin(angle));
		}
	}

	class DesktopOverlayTestStage : public TestRenderStageBase
	{
		using BaseClass = TestRenderStageBase;
	public:
		bool onInit() override
		{
			if (!BaseClass::onInit())
				return false;

			mOverlayWindow.create(TEXT("DesktopOverlay"), OverlayWidth, OverlayHeight, OverlayWindow::StaticWndProc);

			BITMAPINFO info = {};
			info.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
			info.bmiHeader.biWidth = OverlayWidth;
			info.bmiHeader.biHeight = -OverlayHeight;
			info.bmiHeader.biPlanes = 1;
			info.bmiHeader.biBitCount = 32;
			info.bmiHeader.biCompression = BI_RGB;

			HDC screenDC = GetDC(nullptr);
			mBitmapDC.initialize(screenDC, &info, &mBitampDataPtr);
			ReleaseDC(nullptr, screenDC);

			::Global::GUI().cleanupWidget();
			return true;
		}

		void onEnd() override
		{
			mOverlayWindow.destroy();
			mBitmapDC.release();
			mBitampDataPtr = nullptr;

			BaseClass::onEnd();
		}

		bool setupRenderResource(ERenderSystem systemName) override
		{
			VERIFY_RETURN_FALSE(BaseClass::setupRenderResource(systemName));

			mOverlayTexture = RHICreateTexture2D(ETexture::BGRA8, OverlayWidth, OverlayHeight, 1, 1, TCF_RenderTarget | TCF_CreateSRV);
			VERIFY_RETURN_FALSE(mOverlayTexture.isValid());

			mOverlayFrameBuffer = RHICreateFrameBuffer();
			VERIFY_RETURN_FALSE(mOverlayFrameBuffer.isValid());
			mOverlayFrameBuffer->setTexture(0, *mOverlayTexture);

			mOverlayGraphics.setViewportSize(OverlayWidth, OverlayHeight);
			mOverlayGraphics.initializeRHI();
			return true;
		}

		void preShutdownRenderSystem(bool bReInit) override
		{
			mOverlayGraphics.releaseRHI();
			mOverlayFrameBuffer.release();
			mOverlayTexture.release();
			BaseClass::preShutdownRenderSystem(bReInit);
		}

		void onUpdate(GameTimeSpan deltaTime) override
		{
			BaseClass::onUpdate(deltaTime);
			mTime += float(deltaTime);
		}

		void onRender(float dFrame) override
		{
			renderOverlayTexture();
			blitOverlayWindow();

			RHICommandList& commandList = RHICommandList::GetImmediateList();
			RHISetFrameBuffer(commandList, nullptr);

			Vec2i screenSize = ::Global::GetScreenSize();
			RHIClearRenderTargets(commandList, EClearBits::All, &LinearColor(0.05, 0.06, 0.08, 1), 1);
			RHISetViewport(commandList, 0, 0, screenSize.x, screenSize.y);

			Matrix4 projectMatrix = AdjustProjectionMatrixForRHI(OrthoMatrix(0, screenSize.x, screenSize.y, 0, -1, 1));
			DrawUtility::DrawTexture(commandList, projectMatrix, *mOverlayTexture, Vec2i(24, 24), Vec2i(OverlayWidth, OverlayHeight));
		}

	private:
		void renderOverlayTexture()
		{
			RHICommandList& commandList = RHICommandList::GetImmediateList();

			RHISetFrameBuffer(commandList, mOverlayFrameBuffer);
			RHIClearRenderTargets(commandList, EClearBits::All, &LinearColor(0, 0, 0, 0), 1);
			RHISetViewport(commandList, 0, 0, OverlayWidth, OverlayHeight);
			RHISetDepthStencilState(commandList, StaticDepthDisableState::GetRHI());
			RHISetRasterizerState(commandList, TStaticRasterizerState< ECullMode::None >::GetRHI());
			RHISetBlendState(commandList, TStaticBlendState<>::GetRHI());

			RHIGraphics2D& g = mOverlayGraphics;
			g.beginRender();

			g.setPen(Color3ub(80, 120, 170), 2);
			g.setBrush(Color3ub(16, 28, 42));
			//g.drawRect(Vector2(0, 0), Vector2(OverlayWidth, OverlayHeight));

			float pulse = 0.5f + 0.5f * Math::Sin(1.7f * mTime);
			Vector2 circlePos(130 + 70 * Math::Sin(1.3f * mTime), 100 + 35 * Math::Cos(1.9f * mTime));
			g.setPen(Color3ub(255, 245, 180), 3);
			g.setBrush(Color3ub(80, uint8(160 + 70 * pulse), 255));
			g.drawCircle(circlePos, 34 + 10 * pulse);

			Vector2 rectPos(330 + 90 * Math::Cos(0.8f * mTime), 80 + 30 * Math::Sin(1.1f * mTime));
			g.pushXForm();
			g.translateXForm(rectPos.x, rectPos.y);
			g.rotateXForm(mTime);
			g.setPen(Color3ub(255, 120, 90), 2);
			g.setBrush(Color3ub(220, 70, 70));
			g.drawRect(Vector2(-45, -28), Vector2(90, 56));
			g.popXForm();

			Vector2 tri[3];
			float baseAngle = -0.7f * mTime;
			for (int i = 0; i < 3; ++i)
			{
				tri[i] = MakeCirclePoint(Vector2(500, 185), 48, baseAngle + 2.0f * Math::PI * i / 3.0f);
			}
			g.setPen(Color3ub(70, 255, 180), 3);
			g.setBrush(Color3ub(40, 180, 120));
			g.drawPolygon(tri, 3);

			Vector2 wave[32];
			for (int i = 0; i < ARRAY_SIZE(wave); ++i)
			{
				float x = 35.0f + i * 18.0f;
				float y = 235.0f + 22.0f * Math::Sin(0.11f * x + 2.8f * mTime);
				wave[i] = Vector2(x, y);
			}
			g.setPen(Color3ub(190, 210, 255), 2);
			g.drawLineStrip(wave, ARRAY_SIZE(wave));

			g.endRender();

			RHIFlushCommand(commandList);
			RHIReadTexture(*mOverlayTexture, ETexture::BGRA8, 0, mReadbackData);
			mOverlayWindow.setHitTestData(&mReadbackData);
		}

		void blitOverlayWindow()
		{
			if (mOverlayWindow.getHWnd() == nullptr || mReadbackData.empty())
				return;

			RECT rect;
			GetWindowRect(mOverlayWindow.getHWnd(), &rect);
			HDC screenDC = GetDC(nullptr);

			FMemory::Copy(mBitampDataPtr, mReadbackData.data(), mReadbackData.size());

			POINT dstPos = { rect.left, rect.top };
			POINT srcPos = { 0, 0 };
			SIZE size = { OverlayWidth, OverlayHeight };
			BLENDFUNCTION blend = {};
			blend.BlendOp = AC_SRC_OVER;
			blend.SourceConstantAlpha = 255;
			blend.AlphaFormat = AC_SRC_ALPHA;

			UpdateLayeredWindow(mOverlayWindow.getHWnd(), screenDC, &dstPos, &size, mBitmapDC.getHandle(), &srcPos, 0, &blend, ULW_ALPHA);

			ReleaseDC(nullptr, screenDC);
		}

		OverlayWindow mOverlayWindow;
		BitmapDC      mBitmapDC;
		void*         mBitampDataPtr = nullptr;

		RHIGraphics2D mOverlayGraphics;
		RHITexture2DRef mOverlayTexture;
		RHIFrameBufferRef mOverlayFrameBuffer;
		TArray< uint8 > mReadbackData;
		float mTime = 0.0f;
	};

	REGISTER_STAGE_ENTRY("Desktop Overlay Test", DesktopOverlayTestStage, EExecGroup::GraphicsTest, "Render|RHI");
}
