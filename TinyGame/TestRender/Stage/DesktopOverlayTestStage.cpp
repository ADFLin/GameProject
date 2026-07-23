#include "Stage/TestRenderStageBase.h"

#include "RHI/RHIGraphics2D.h"
#include "RHI/RHICommand.h"
#include "RHI/DrawUtility.h"
#include "RHI/GlobalShader.h"
#include "RHI/ShaderManager.h"

#include "RenderUtility.h"
#include "WindowsPlatform.h"

#include <cstring>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include "RHI/D3D11Common.h"
#include "RHI/D3D11Command.h"
#include "ProfileSystem.h"

#include <dxgi1_2.h>
#include <dcomp.h>

#pragma comment(lib, "dcomp.lib")

#ifndef WDA_EXCLUDEFROMCAPTURE
#define WDA_EXCLUDEFROMCAPTURE 0x00000011
#endif

namespace Render
{
	namespace
	{
		class DesktopCaptureSource
		{
		public:
			virtual ~DesktopCaptureSource() = default;
			virtual bool capture(RECT const& screenRect, TArray<uint8>& outBGRAData) = 0;
		};

		class GDIDesktopCaptureSource : public DesktopCaptureSource
		{
		public:
			~GDIDesktopCaptureSource()
			{
				release();
			}

			bool capture(RECT const& screenRect, TArray<uint8>& outBGRAData) override
			{
				int width = screenRect.right - screenRect.left;
				int height = screenRect.bottom - screenRect.top;
				if (width <= 0 || height <= 0 || !prepare(width, height))
					return false;

				HDC desktopDC = GetDC(nullptr);
				if (desktopDC == nullptr)
					return false;

				BOOL result = BitBlt(mCaptureDC, 0, 0, width, height, desktopDC,
					screenRect.left, screenRect.top, SRCCOPY);
				ReleaseDC(nullptr, desktopDC);
				if (!result)
					return false;

				outBGRAData.resize(4 * width * height);
				FMemory::Copy(outBGRAData.data(), mBitmapData, outBGRAData.size());
				return true;
			}

		private:
			bool prepare(int width, int height)
			{
				if (mCaptureDC != nullptr && mWidth == width && mHeight == height)
					return true;

				release();
				mCaptureDC = CreateCompatibleDC(nullptr);
				if (mCaptureDC == nullptr)
					return false;

				BITMAPINFO info = {};
				info.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
				info.bmiHeader.biWidth = width;
				info.bmiHeader.biHeight = -height;
				info.bmiHeader.biPlanes = 1;
				info.bmiHeader.biBitCount = 32;
				info.bmiHeader.biCompression = BI_RGB;
				mBitmap = CreateDIBSection(mCaptureDC, &info, DIB_RGB_COLORS, &mBitmapData, nullptr, 0);
				if (mBitmap == nullptr)
				{
					release();
					return false;
				}

				mOldBitmap = SelectObject(mCaptureDC, mBitmap);
				mWidth = width;
				mHeight = height;
				return true;
			}

			void release()
			{
				if (mCaptureDC && mOldBitmap)
					SelectObject(mCaptureDC, mOldBitmap);
				if (mBitmap)
					DeleteObject(mBitmap);
				if (mCaptureDC)
					DeleteDC(mCaptureDC);
				mCaptureDC = nullptr;
				mBitmap = nullptr;
				mOldBitmap = nullptr;
				mBitmapData = nullptr;
				mWidth = mHeight = 0;
			}

			HDC mCaptureDC = nullptr;
			HBITMAP mBitmap = nullptr;
			HGDIOBJ mOldBitmap = nullptr;
			void* mBitmapData = nullptr;
			int mWidth = 0;
			int mHeight = 0;
		};

		class D3D11DesktopDuplicationSource : public DesktopCaptureSource
		{
		public:
			~D3D11DesktopDuplicationSource()
			{
				releaseOutput();
			}

			bool capture(RECT const& screenRect, TArray<uint8>& outBGRAData) override
			{
				if (GRHISystem == nullptr || GRHISystem->getName() != RHISystemName::D3D11)
					return false;

				if (!contains(mOutputRect, screenRect) && !initializeOutput(screenRect))
					return false;

				DXGI_OUTDUPL_FRAME_INFO frameInfo = {};
				TComPtr<IDXGIResource> desktopResource;
				HRESULT hr = mDuplication->AcquireNextFrame(0, &frameInfo, &desktopResource);
				if (hr == DXGI_ERROR_ACCESS_LOST)
				{
					releaseOutput();
					initializeOutput(screenRect);
					return false;
				}
				if (hr != DXGI_ERROR_WAIT_TIMEOUT && FAILED(hr))
					return false;

				if (SUCCEEDED(hr))
				{
					TComPtr<ID3D11Texture2D> desktopTexture;
					HRESULT textureResult = desktopResource->QueryInterface(IID_PPV_ARGS(&desktopTexture));
					if (SUCCEEDED(textureResult))
					{
						mContext->CopyResource(mStagingTexture, desktopTexture);
						mbHasFrame = true;
					}
					mDuplication->ReleaseFrame();
					if (FAILED(textureResult))
						return false;
				}

				if (!mbHasFrame)
					return false;

				D3D11_MAPPED_SUBRESOURCE mapped = {};
				hr = mContext->Map(mStagingTexture, 0, D3D11_MAP_READ, 0, &mapped);
				if (FAILED(hr))
					return false;

				int width = screenRect.right - screenRect.left;
				int height = screenRect.bottom - screenRect.top;
				int sourceX = screenRect.left - mOutputRect.left;
				int sourceY = screenRect.top - mOutputRect.top;
				outBGRAData.resize(4 * width * height);
				for (int y = 0; y < height; ++y)
				{
					uint8 const* source = static_cast<uint8 const*>(mapped.pData) +
						(sourceY + y) * mapped.RowPitch + 4 * sourceX;
					FMemory::Copy(outBGRAData.data() + 4 * width * y, source, 4 * width);
				}
				mContext->Unmap(mStagingTexture, 0);
				return true;
			}

		private:
			static bool contains(RECT const& outer, RECT const& inner)
			{
				return inner.left >= outer.left && inner.top >= outer.top &&
					inner.right <= outer.right && inner.bottom <= outer.bottom;
			}

			bool initializeOutput(RECT const& screenRect)
			{
				releaseOutput();
				auto* system = static_cast<D3D11System*>(GRHISystem);
				mDevice = system->mDevice;
				mContext = system->mDeviceContextImmdiate;

				TComPtr<IDXGIDevice> dxgiDevice;
				if (FAILED(mDevice->QueryInterface(IID_PPV_ARGS(&dxgiDevice))))
					return false;
				TComPtr<IDXGIAdapter> adapter;
				if (FAILED(dxgiDevice->GetAdapter(&adapter)))
					return false;

				for (UINT index = 0; ; ++index)
				{
					TComPtr<IDXGIOutput> output;
					HRESULT hr = adapter->EnumOutputs(index, &output);
					if (hr == DXGI_ERROR_NOT_FOUND)
						break;
					if (FAILED(hr))
						continue;

					DXGI_OUTPUT_DESC outputDesc = {};
					if (FAILED(output->GetDesc(&outputDesc)) || !contains(outputDesc.DesktopCoordinates, screenRect))
						continue;
					// Rotation support can be added when portrait monitors are needed.
					if (outputDesc.Rotation != DXGI_MODE_ROTATION_IDENTITY)
						return false;

					TComPtr<IDXGIOutput1> output1;
					if (FAILED(output->QueryInterface(IID_PPV_ARGS(&output1))) ||
						FAILED(output1->DuplicateOutput(mDevice, &mDuplication)))
						return false;

					mOutputRect = outputDesc.DesktopCoordinates;
					D3D11_TEXTURE2D_DESC stagingDesc = {};
					stagingDesc.Width = mOutputRect.right - mOutputRect.left;
					stagingDesc.Height = mOutputRect.bottom - mOutputRect.top;
					stagingDesc.MipLevels = 1;
					stagingDesc.ArraySize = 1;
					stagingDesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
					stagingDesc.SampleDesc.Count = 1;
					stagingDesc.Usage = D3D11_USAGE_STAGING;
					stagingDesc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
					if (FAILED(mDevice->CreateTexture2D(&stagingDesc, nullptr, &mStagingTexture)))
					{
						releaseOutput();
						return false;
					}
					return true;
				}
				return false;
			}

			void releaseOutput()
			{
				mDuplication.reset();
				mStagingTexture.reset();
				mContext.reset();
				mDevice.reset();
				mOutputRect = {};
				mbHasFrame = false;
			}

			TComPtr<ID3D11Device> mDevice;
			TComPtr<ID3D11DeviceContext> mContext;
			TComPtr<IDXGIOutputDuplication> mDuplication;
			TComPtr<ID3D11Texture2D> mStagingTexture;
			RECT mOutputRect = {};
			bool mbHasFrame = false;
		};

		class OverlayBackgroundProgram : public GlobalShaderProgram
		{
			DECLARE_SHADER_PROGRAM(OverlayBackgroundProgram, Global);
		public:
			static char const* GetShaderFileName() { return "Shader/Test/DesktopOverlay"; }
			static TArrayView<ShaderEntryInfo const> GetShaderEntries()
			{
				static ShaderEntryInfo const entries[] =
				{
					{ EShader::Vertex, SHADER_ENTRY(ScreenVS) },
					{ EShader::Pixel, SHADER_ENTRY(BackgroundEffectPS) },
				};
				return entries;
			}
			void bindParameters(ShaderParameterMap const& parameterMap) override
			{
				BIND_TEXTURE_PARAM(parameterMap, BackgroundTexture);
				BIND_SHADER_PARAM(parameterMap, TextureSizeInv);
			}
			DEFINE_TEXTURE_PARAM(BackgroundTexture);
			DEFINE_SHADER_PARAM(TextureSizeInv);
		};

		IMPLEMENT_SHADER_PROGRAM(OverlayBackgroundProgram);

		class OverlayWindow : public WinFrameT< OverlayWindow >
		{
		public:
			DWORD getWinStyle() { return WS_POPUP | WS_VISIBLE; }
			DWORD getWinExtStyle() { return WS_EX_TOPMOST | WS_EX_TOOLWINDOW | WS_EX_NOACTIVATE | WS_EX_NOREDIRECTIONBITMAP; }
			LPTSTR getWinClassName() { return "DesktopOverlayWindow"; }

			bool setupWindow(bool fullscreen, unsigned colorBits)
			{
				BITMAPINFO info = {};
				info.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
				info.bmiHeader.biWidth = getWidth();
				info.bmiHeader.biHeight = -getHeight();
				info.bmiHeader.biPlanes = 1;
				info.bmiHeader.biBitCount = 32;
				info.bmiHeader.biCompression = BI_RGB;
				mBitmapDC.initialize(getHDC(), &info, (void**)&mBitampDataPtr);
				return true;
			}

			void onWinodwPrevDestory()
			{
				releaseComposition();
				mBitmapDC.release();
				mBitampDataPtr = nullptr;
			}

			bool initializeComposition()
			{
				releaseComposition();
				if (GRHISystem == nullptr || GRHISystem->getName() != RHISystemName::D3D11)
					return false;

				auto* system = static_cast<D3D11System*>(GRHISystem);
				mD3DDevice = system->mDevice;
				mD3DContext = system->mDeviceContextImmdiate;

				TComPtr<IDXGIDevice> dxgiDevice;
				VERIFY_D3D_RESULT_RETURN_FALSE(mD3DDevice->QueryInterface(IID_PPV_ARGS(&dxgiDevice)));
				TComPtr<IDXGIAdapter> adapter;
				VERIFY_D3D_RESULT_RETURN_FALSE(dxgiDevice->GetAdapter(&adapter));
				TComPtr<IDXGIFactory2> factory;
				VERIFY_D3D_RESULT_RETURN_FALSE(adapter->GetParent(IID_PPV_ARGS(&factory)));

				DXGI_SWAP_CHAIN_DESC1 swapChainDesc = {};
				swapChainDesc.Width = getWidth();
				swapChainDesc.Height = getHeight();
				swapChainDesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
				swapChainDesc.SampleDesc.Count = 1;
				swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
				swapChainDesc.BufferCount = 2;
				swapChainDesc.Scaling = DXGI_SCALING_STRETCH;
				swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL;
				swapChainDesc.AlphaMode = DXGI_ALPHA_MODE_PREMULTIPLIED;
			VERIFY_D3D_RESULT_RETURN_FALSE(factory->CreateSwapChainForComposition(
				mD3DDevice, &swapChainDesc, nullptr, &mCompositionSwapChain));

			VERIFY_D3D_RESULT_RETURN_FALSE(DCompositionCreateDevice(
				dxgiDevice, IID_PPV_ARGS(&mCompositionDevice)));
			VERIFY_D3D_RESULT_RETURN_FALSE(mCompositionDevice->CreateTargetForHwnd(
				getHWnd(), TRUE, &mCompositionTarget));
			VERIFY_D3D_RESULT_RETURN_FALSE(mCompositionDevice->CreateVisual(&mCompositionVisual));
			VERIFY_D3D_RESULT_RETURN_FALSE(mCompositionVisual->SetContent(mCompositionSwapChain));
			VERIFY_D3D_RESULT_RETURN_FALSE(mCompositionDevice->CreateEffectGroup(&mCompositionEffectGroup));
			VERIFY_D3D_RESULT_RETURN_FALSE(mCompositionVisual->SetEffect(mCompositionEffectGroup));
			VERIFY_D3D_RESULT_RETURN_FALSE(mCompositionTarget->SetRoot(mCompositionVisual));
			VERIFY_D3D_RESULT_RETURN_FALSE(mCompositionDevice->Commit());
			return true;
		}

			void releaseComposition()
			{
				mCompositionEffectGroup.reset();
				mCompositionVisual.reset();
				mCompositionTarget.reset();
				mCompositionDevice.reset();
				mCompositionSwapChain.reset();
				mD3DContext.reset();
				mD3DDevice.reset();
				mbDisplayAffinityConfigured = false;
				mDisplayAffinityAttemptCount = 0;
			}

			bool present(RHITexture2D& texture)
			{
				if (!mCompositionSwapChain || !mD3DContext)
					return false;

				TComPtr<ID3D11Texture2D> backBuffer;
				VERIFY_D3D_RESULT_RETURN_FALSE(mCompositionSwapChain->GetBuffer(0, IID_PPV_ARGS(&backBuffer)));
				mD3DContext->CopyResource(backBuffer, D3D11Cast::GetResource(texture));
				VERIFY_D3D_RESULT_RETURN_FALSE(mCompositionSwapChain->Present(0, 0));

				if (!mbDisplayAffinityConfigured && mDisplayAffinityAttemptCount < 3)
				{
					++mDisplayAffinityAttemptCount;
					HWND overlayHWnd = getHWnd();
					BOOL setResult = SetWindowDisplayAffinity(overlayHWnd, WDA_EXCLUDEFROMCAPTURE);
					DWORD setError = setResult ? ERROR_SUCCESS : GetLastError();
					DWORD affinity = WDA_NONE;
					BOOL getResult = GetWindowDisplayAffinity(overlayHWnd, &affinity);
					DWORD getError = getResult ? ERROR_SUCCESS : GetLastError();
					LogMsg("DirectComposition overlay affinity: hwnd = %p, setResult = %d, setError = %u, getResult = %d, value = 0x%08X, getError = %u",
						static_cast<void*>(overlayHWnd), int(setResult), setError, int(getResult), affinity, getError);
					mbDisplayAffinityConfigured = setResult && affinity == WDA_EXCLUDEFROMCAPTURE;
				}
				return true;
			}

			bool hitTestDrawnPixel(POINT screenPos) const
			{
				if (mBitampDataPtr == nullptr)
					return false;

				POINT clientPos = screenPos;
				ScreenToClient(getHWnd(), &clientPos);
				if (clientPos.x < 0 || clientPos.y < 0 || clientPos.x >= mWidth || clientPos.y >= mHeight)
					return false;

				int index = 4 * (clientPos.y * mWidth + clientPos.x);
				uint8 alpha = mBitampDataPtr[index + 3];
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
				SetWindowPos(getHWnd(), NULL, screenPos.x - mDragOffset.x, screenPos.y - mDragOffset.y, 0, 0, SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE);
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

			void updateLayer(float alpha)
			{
				if (mCompositionEffectGroup && mCompositionDevice)
				{
					mCompositionEffectGroup->SetOpacity(alpha);
					mCompositionDevice->Commit();
				}
			}

			static LRESULT CALLBACK StaticWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
			{
				if (message == WM_CREATE)
				{
					CREATESTRUCT* ps = (CREATESTRUCT*)lParam;
					SetWindowLongPtr(hWnd, GWLP_USERDATA, (LONG)ps->lpCreateParams);
				}

				OverlayWindow* window = (OverlayWindow*)GetWindowLongPtr(hWnd, GWLP_USERDATA);
				switch (message)
				{
				case WM_CLOSE:
					ShowWindow(hWnd, SW_HIDE);
					return 0;
				case WM_ERASEBKGND:
					return 1;
				case WM_MOUSEACTIVATE:
					return MA_NOACTIVATE;
				case WM_NCHITTEST:
					if (window)
					{
						POINT screenPos = { int(short(LOWORD(lParam))), int(short(HIWORD(lParam))) };
						return window->hitTestDrawnPixel(screenPos) ? HTCLIENT : HTTRANSPARENT;
					}
					return HTTRANSPARENT;
				case WM_LBUTTONDOWN:
					if (window)
					{
						POINT clientPos = { int(short(LOWORD(lParam))), int(short(HIWORD(lParam))) };
						window->beginDrag(clientPos);
						return 0;
					}
					break;
				case WM_MOUSEMOVE:
					if (window)
					{
						window->updateDrag();
						return 0;
					}
					break;
				case WM_LBUTTONUP:
					if (window)
					{
						window->endDrag();
						return 0;
					}
					break;
				case WM_CAPTURECHANGED:
					if (window)
					{
						window->mbDragging = false;
					}
					break;
				}
				return DefWindowProc(hWnd, message, wParam, lParam);
			}

			BitmapDC mBitmapDC;
			uint8* mBitampDataPtr = nullptr;
			TComPtr<ID3D11Device> mD3DDevice;
			TComPtr<ID3D11DeviceContext> mD3DContext;
			TComPtr<IDXGISwapChain1> mCompositionSwapChain;
			TComPtr<IDCompositionDevice> mCompositionDevice;
			TComPtr<IDCompositionTarget> mCompositionTarget;
			TComPtr<IDCompositionVisual> mCompositionVisual;
			TComPtr<IDCompositionEffectGroup> mCompositionEffectGroup;

			bool mbDragging = false;
			bool mbDisplayAffinityConfigured = false;
			int mDisplayAffinityAttemptCount = 0;
			POINT mDragOffset = { 0, 0 };
		};

		struct OverlayArea
		{
			int id = 0;
			OverlayWindow window;
			Vec2i size;
			RHITexture2DRef   texture;
			RHIFrameBufferRef frameBuffer;
			RHITexture2DRef   backgroundTexture;
			bool bUseBackgroundEffect = false;
			TArray< uint8 >   captureData;
			TArray< uint8 >   readbackData;
			std::function< void(RHICommandList&, OverlayArea const&) > renderFunc;
		};


		struct OverlayAreaDesc
		{
			TCHAR const* title;
			Vec2i position;
			Vec2i size;
			std::function< void(RHICommandList&, OverlayArea const&) > renderFunc;
			bool bUseBackgroundEffect = false;
		};

		class OverlayManager
		{
		public:
			~OverlayManager()
			{
				cleanup();
			}

			int addArea(OverlayAreaDesc const& desc)
			{
				std::unique_ptr< OverlayArea > area(new OverlayArea);

				area->size = desc.size;
				area->renderFunc = desc.renderFunc;
				area->bUseBackgroundEffect = desc.bUseBackgroundEffect;

				if (!area->window.create(desc.title, desc.size.x, desc.size.y, OverlayWindow::StaticWndProc))
					return INDEX_NONE;

				int id = mNextId++;
				area->id = id;
				area->window.setPosition(desc.position);
				if (bRenderResourceInitialized)
				{
					initializeRenderResource(*area);
				}
				mAreas.push_back(std::move(area));
				return id;
			}

			bool removeArea(int id)
			{
				for (std::unique_ptr< OverlayArea >& area : mAreas)
				{
					if (area->id != id)
						continue;

					area->window.destroy();
					mAreas.remove(area);
					return true;
				}
				return false;
			}


			bool initializeRenderResource(OverlayArea& area)
			{
				VERIFY_RETURN_FALSE(area.window.initializeComposition());

				area.texture = RHICreateTexture2D(ETexture::BGRA8, area.size.x, area.size.y, 1, 1, TCF_RenderTarget | TCF_CreateSRV);
				VERIFY_RETURN_FALSE(area.texture.isValid());

				area.frameBuffer = RHICreateFrameBuffer();
				VERIFY_RETURN_FALSE(area.frameBuffer.isValid());
				area.frameBuffer->setTexture(0, *area.texture);
				if (area.bUseBackgroundEffect)
				{
					area.backgroundTexture = RHICreateTexture2D(ETexture::BGRA8, area.size.x, area.size.y, 1, 1, TCF_CreateSRV);
					VERIFY_RETURN_FALSE(area.backgroundTexture.isValid());
				}

				return true;
			}

			bool setupRenderResource()
			{
				VERIFY_RETURN_FALSE(mBackgroundProgram = ShaderManager::Get().getGlobalShaderT<OverlayBackgroundProgram>());
				bRenderResourceInitialized = true;

				for (std::unique_ptr< OverlayArea >& area : mAreas)
				{
					if (!initializeRenderResource(*area))
					{
						return false;
					}
				}
				return true;
			}


			void render(RHICommandList& commandList)
			{
				for (std::unique_ptr< OverlayArea >& area : mAreas)
				{
					if (area->bUseBackgroundEffect)
					{
						RECT rect;
						HWND overlayHWnd = area->window.getHWnd();
						GetWindowRect(overlayHWnd, &rect);
						if (mCaptureSource->capture(rect, area->captureData))
						{
							RHIUpdateTexture(commandList, *area->backgroundTexture, 0, 0,
								area->size.x, area->size.y, area->captureData.data(), 0, area->size.x);
						}
					}

					RHISetFrameBuffer(commandList, area->frameBuffer);
					RHIClearRenderTargets(commandList, EClearBits::All, &LinearColor(0.0f, 0.0f, 0.0f, 0.0f), 1);
					RHISetViewport(commandList, 0, 0, float(area->size.x), float(area->size.y));

					if (area->bUseBackgroundEffect && mBackgroundProgram)
					{
						RHISetDepthStencilState(commandList, StaticDepthDisableState::GetRHI());
						RHISetRasterizerState(commandList, TStaticRasterizerState<ECullMode::None>::GetRHI());
						RHISetBlendState(commandList, TStaticBlendState<>::GetRHI());
						RHISetShaderProgram(commandList, mBackgroundProgram->getRHI());
						auto& sampler = TStaticSamplerState<ESampler::Bilinear, ESampler::Clamp, ESampler::Clamp>::GetRHI();
						SET_SHADER_TEXTURE_AND_SAMPLER(commandList, *mBackgroundProgram, BackgroundTexture, *area->backgroundTexture, sampler);
						SET_SHADER_PARAM(commandList, *mBackgroundProgram, TextureSizeInv, Vector2(1.0f / area->size.x, 1.0f / area->size.y));
						DrawUtility::ScreenRect(commandList);
					}

					area->renderFunc(commandList, *area);

					RHIFlushCommand(commandList);
					area->window.present(*area->texture);

					// Kept temporarily for alpha-based hit testing. Presentation itself
					// now stays entirely on the GPU through DirectComposition.
					RHIReadTexture(*area->texture, ETexture::BGRA8, 0, area->readbackData);
					FMemory::Copy(area->window.mBitampDataPtr, area->readbackData.data(), area->readbackData.size());
				}
			}

			void releaseRenderResource()
			{
				for (std::unique_ptr< OverlayArea >& area : mAreas)
				{
					area->window.releaseComposition();
					area->frameBuffer.release();
					area->texture.release();
					area->backgroundTexture.release();
					area->captureData.clear();
					area->readbackData.clear();
				}
				bRenderResourceInitialized = false;
				mBackgroundProgram = nullptr;
			}

			void updateWindows(float alpha)
			{
				for (std::unique_ptr< OverlayArea >& area : mAreas)
				{
					area->window.updateLayer(alpha);
				}
			}

			void cleanup()
			{
				for (std::unique_ptr< OverlayArea >& area : mAreas)
				{
					area->window.destroy();
				}
				mAreas.clear();
			}

			TArray< std::unique_ptr< OverlayArea > >& getAreas() { return mAreas; }
			TArray< std::unique_ptr< OverlayArea > > const& getAreas() const { return mAreas; }

		private:
			bool bRenderResourceInitialized = false;
			int  mNextId = 0;
			TArray< std::unique_ptr< OverlayArea > > mAreas;
			std::unique_ptr<DesktopCaptureSource> mCaptureSource = std::make_unique<D3D11DesktopDuplicationSource>();
			OverlayBackgroundProgram* mBackgroundProgram = nullptr;
		};

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


			mOverlayManager.addArea({ TEXT("OverlayCircle"), Vec2i(120, 120), Vec2i(260, 180),
				[this](RHICommandList& commandList, OverlayArea const& area)
				{
					renderOverlayArea(commandList, area, 0);
				}, true}
			);
			mOverlayManager.addArea({ TEXT("OverlayBox"), Vec2i(440, 170), Vec2i(300, 190),
				[this](RHICommandList& commandList, OverlayArea const& area)
				{
					renderOverlayArea(commandList, area, 1);
				}}
			);

			mOverlayManager.addArea({ TEXT("OverlayWave"), Vec2i(260, 420), Vec2i(520, 130),
				[this](RHICommandList& commandList, OverlayArea const& area)
				{
					renderOverlayArea(commandList, area, 2);
				}}
			);

			::Global::GUI().cleanupWidget();

			auto frame = WidgetUtility::CreateDevFrame();

			frame->addSlider("Alpha", mOverlayAlpha, 0.0f, 1.0f);
			return true;
		}

		void onEnd() override
		{
			mOverlayManager.cleanup();
			BaseClass::onEnd();
		}

		bool setupRenderResource(ERenderSystem systemName) override
		{
			VERIFY_RETURN_FALSE(BaseClass::setupRenderResource(systemName));
			VERIFY_RETURN_FALSE(mOverlayManager.setupRenderResource());
			return true;
		}

		void preShutdownRenderSystem(bool bReInit) override
		{
			mOverlayManager.releaseRenderResource();
			BaseClass::preShutdownRenderSystem(bReInit);
		}

		void onUpdate(GameTimeSpan deltaTime) override
		{
			BaseClass::onUpdate(deltaTime);
			mTime += float(deltaTime);
		}

		void onRender(float dFrame) override
		{
			renderOverlayAreas();
			mOverlayManager.updateWindows(mOverlayAlpha);

			RHICommandList& commandList = RHICommandList::GetImmediateList();
			RHISetFrameBuffer(commandList, nullptr);

			Vec2i screenSize = ::Global::GetScreenSize();
			RHIClearRenderTargets(commandList, EClearBits::All, &LinearColor(0.05f, 0.06f, 0.08f, 1.0f), 1);
			RHISetViewport(commandList, 0, 0, float(screenSize.x), float(screenSize.y));

			RHIGraphics2D& g = ::Global::GetRHIGraphics2D();

			g.beginRender();
			g.setBrush(Color3ub(255, 255, 255));

			Vec2i previewPos(24, 24);
			for (std::unique_ptr< OverlayArea > const& area : mOverlayManager.getAreas())
			{
				g.drawTexture(*area->texture, previewPos, area->size);
				if (area->backgroundTexture.isValid())
				{
					Vec2i capturePreviewPos(previewPos.x + area->size.x + 16, previewPos.y);
					g.drawTexture(*area->backgroundTexture, capturePreviewPos, area->size);
				}
				previewPos.y += area->size.y + 16;
			}

			g.endRender();
		}

		MsgReply onKey(KeyMsg const& msg)
		{
			if ( !msg.isDown() )
				return MsgReply::Unhandled();

			switch (msg.getCode())
			{
			case EKeyCode::Z:
				mOverlayManager.removeArea(0);
				break;
			}
			return MsgReply::Unhandled();
		}
	private:
		void renderOverlayAreas()
		{
			RHICommandList& commandList = RHICommandList::GetImmediateList();
			mOverlayManager.render(commandList);

			Vec2i screenSize = ::Global::GetScreenSize();
			::Global::GetRHIGraphics2D().setViewportSize(screenSize.x, screenSize.y);
		}

		void renderOverlayArea(RHICommandList& commandList, OverlayArea const& area, int contentId)
		{
			RHIGraphics2D& g = ::Global::GetRHIGraphics2D();

			g.setViewportSize(area.size.x, area.size.y);
			g.beginRender();

			switch (contentId)
			{
			case 0:
				drawCircleArea(g, area.size);
				break;
			case 1:
				drawBoxArea(g, area.size);
				break;
			case 2:
				drawWaveArea(g, area.size);
				break;
			default:
				break;
			}

			g.endRender();
		}

		void drawCircleArea(RHIGraphics2D& g, Vec2i const& size)
		{
			float pulse = 0.5f;
			Vector2 circlePos(90.0f, 82.0f);
			g.setPen(Color3ub(255, 245, 180), 3);
			g.setBrush(Color3ub(80, uint8(160 + 70 * pulse), 255));
			g.drawCircle(circlePos, 34.0f + 10.0f * pulse);

			Vector2 tri[3];
			float baseAngle = 0.0f;
			for (int i = 0; i < 3; ++i)
			{
				tri[i] = MakeCirclePoint(Vector2(float(size.x - 68), float(size.y - 58)), 36.0f, baseAngle + 2.0f * Math::PI * float(i) / 3.0f);
			}
			g.setPen(Color3ub(70, 255, 180), 3);
			g.setBrush(Color3ub(40, 180, 120));
			g.drawPolygon(tri, 3);
		}

		void drawBoxArea(RHIGraphics2D& g, Vec2i const& size)
		{
			Vector2 rectPos(size.x * 0.5f + 80.0f * Math::Cos(0.8f * mTime), size.y * 0.5f + 30.0f * Math::Sin(1.1f * mTime));
			g.pushXForm();
			g.translateXForm(rectPos.x, rectPos.y);
			g.rotateXForm(mTime);
			g.setPen(Color3ub(255, 120, 90), 2);
			g.setBrush(Color3ub(220, 70, 70));
			g.drawRect(Vector2(-45, -28), Vector2(90, 56));
			g.popXForm();
		}

		void drawWaveArea(RHIGraphics2D& g, Vec2i const& size)
		{
			Vector2 wave[32];
			for (int i = 0; i < ARRAY_SIZE(wave); ++i)
			{
				float x = 20.0f + float(i) * float(size.x - 40) / float(ARRAY_SIZE(wave) - 1);
				float y = 65.0f + 22.0f * Math::Sin(0.05f * x + 2.8f * mTime);
				wave[i] = Vector2(x, y);
			}
			g.setPen(Color3ub(190, 210, 255), 2);
			g.drawLineStrip(wave, int(ARRAY_SIZE(wave)));
		}

		OverlayManager mOverlayManager;
		float mOverlayAlpha = 1.0f;
		float mTime = 0.0f;
	};

	REGISTER_STAGE_ENTRY("Desktop Overlay Test", DesktopOverlayTestStage, EExecGroup::GraphicsTest, "Render|RHI");
}
