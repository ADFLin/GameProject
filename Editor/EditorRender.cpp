#include "EditorRender.h"

#include "RHI/RHICommand.h"
#include "RHI/TextureAtlas.h"
#include "RHI/Font.h"
#include "Platform/Windows/ComUtility.h"

#include "ImGui/backends/imgui_impl_dx11.h"
#include "ImGui/backends/imgui_impl_dx12.h"
#include "ImGui/backends/imgui_impl_opengl3.h"

#pragma comment(lib , "DXGI.lib")
#pragma comment(lib , "dxguid.lib")

#include <D3D11.h>
#include <D3D12.h>
#pragma comment(lib , "D3D11.lib")
#pragma comment(lib , "D3D12.lib")

#include "RHI/D3D11Command.h"
#include "RHI/D3D12Command.h"
#include "RHI/OpenGLCommand.h"

using namespace Render;


#define ERROR_MSG_GENERATE( HR , CODE , FILE , LINE )\
	LogWarning(1, "ErrorCode = 0x%x File = %s Line = %s %s ", HR , FILE, #LINE, #CODE)

#define VERIFY_D3D_RESULT_INNER( FILE , LINE , CODE ,ERRORCODE )\
	{ HRESULT hResult = CODE; if( FAILED(hResult) ){ ERROR_MSG_GENERATE( hResult , CODE, FILE, LINE ); ERRORCODE } }

#define VERIFY_D3D_RESULT( CODE , ERRORCODE ) VERIFY_D3D_RESULT_INNER( __FILE__ , __LINE__ , CODE , ERRORCODE )
#define VERIFY_D3D_RESULT_RETURN_FALSE( CODE ) VERIFY_D3D_RESULT_INNER( __FILE__ , __LINE__ , CODE , return false; )



class EditorRendererD3D11 : public IEditorRenderer
{
public:

	class WindowRenderData : public IEditorWindowRenderData
	{
	public:
		bool initRenderResource(EditorWindow& window, ID3D11Device* device)
		{

			HRESULT hr;
			TComPtr<IDXGIFactory> factory;
			hr = CreateDXGIFactory(IID_PPV_ARGS(&factory));

			UINT quality = 0;
			DXGI_SWAP_CHAIN_DESC swapChainDesc; ZeroMemory(&swapChainDesc, sizeof(swapChainDesc));
			swapChainDesc.OutputWindow = window.getHWnd();
			swapChainDesc.Windowed = true;
			swapChainDesc.SampleDesc.Count = 1;
			swapChainDesc.SampleDesc.Quality = quality;
			swapChainDesc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
			swapChainDesc.BufferDesc.Width = window.getWidth();
			swapChainDesc.BufferDesc.Height = window.getHeight();
			swapChainDesc.BufferCount = 2;
			swapChainDesc.BufferUsage = DXGI_USAGE_BACK_BUFFER | DXGI_USAGE_RENDER_TARGET_OUTPUT;
			swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;
			swapChainDesc.Flags = 0;
			VERIFY_D3D_RESULT_RETURN_FALSE(factory->CreateSwapChain(device, &swapChainDesc, &mSwapChain));

			createRenderTarget(device);

			return true;
		}

		bool createRenderTarget(ID3D11Device* device)
		{
			ID3D11Texture2D* pBackBuffer;
			mSwapChain->GetBuffer(0, IID_PPV_ARGS(&pBackBuffer));
			if (pBackBuffer)
			{
				device->CreateRenderTargetView(pBackBuffer, NULL, &mRenderTargetView);
				pBackBuffer->Release();
			}
			else
			{
				LogWarning(0, "Can't Get BackBuffer");
				return false;
			}


			return true;
		}

		void cleanupRenderTarget()
		{
			mRenderTargetView.reset();
		}

		TComPtr< IDXGISwapChain > mSwapChain;
		TComPtr< ID3D11RenderTargetView > mRenderTargetView;

	};
	virtual bool initialize(EditorWindow& mainWindow)
	{
		mDevice = static_cast<D3D11System*>(GRHISystem)->mDevice;
		mDeviceContext = static_cast<D3D11System*>(GRHISystem)->mDeviceContext;

		VERIFY_RETURN_FALSE(ImGui_ImplWin32_Init(mainWindow.getHWnd()));
		VERIFY_RETURN_FALSE(ImGui_ImplDX11_Init(mDevice, mDeviceContext));
		VERIFY_RETURN_FALSE(initializeWindowRenderData(mainWindow));

		return true;
	}
	virtual void shutdown()
	{
		ImGui_ImplDX11_Shutdown();
		ImGui_ImplWin32_Shutdown();
	}

	void beginFrame()
	{
		ImGui_ImplDX11_NewFrame();
		ImGui_ImplWin32_NewFrame();
	}
	TComPtr< ID3D11Device > mDevice;
	TComPtr< ID3D11DeviceContext > mDeviceContext;


	ID3D11RenderTargetView* mSavedRenderTarget[8];
	ID3D11DepthStencilView* mSavedDepth;

	bool initializeWindowRenderData(EditorWindow& window) override
	{
		auto renderData = std::make_unique<WindowRenderData>();
		VERIFY_RETURN_FALSE(renderData->initRenderResource(window, mDevice));
		window.mRenderData = std::move(renderData);
		return true;
	}

	void renderWindow(EditorWindow& window)
	{
		WindowRenderData* renderData = window.getRenderData<WindowRenderData>();

		const float clear_color_with_alpha[4] = { 0,0,0,1 };
		mDeviceContext->OMSetRenderTargets(1, &renderData->mRenderTargetView, NULL);
		mDeviceContext->ClearRenderTargetView(renderData->mRenderTargetView, clear_color_with_alpha);
		ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
		renderData->mSwapChain->Present(1, 0);
	}

	void notifyWindowResize(EditorWindow& window, int width, int height) override
	{
		WindowRenderData* renderData = window.getRenderData<WindowRenderData>();
		if (renderData->mSwapChain.isValid())
		{
			renderData->cleanupRenderTarget();
			renderData->mSwapChain->ResizeBuffers(0, width, height, DXGI_FORMAT_UNKNOWN, 0);
			renderData->createRenderTarget(mDevice);
		}
	}
};

class EditorRendererD3D12 : public IEditorRenderer
{
public:

#if 0
	class WindowRenderData : public IEditorWindowRenderData
	{
	public:
		bool initRenderResource(EditorWindow& window, ID3D12DeviceRHI* device)
		{
			HRESULT hr;
			TComPtr<IDXGIFactory> factory;
			hr = CreateDXGIFactory(IID_PPV_ARGS(&factory));

			UINT quality = 0;
			DXGI_SWAP_CHAIN_DESC1 swapChainDesc; 
			ZeroMemory(&swapChainDesc, sizeof(swapChainDesc));
			swapChainDesc.OutputWindow = window.getHWnd();
			swapChainDesc.Windowed = true;
			swapChainDesc.SampleDesc.Count = 1;
			swapChainDesc.SampleDesc.Quality = quality;
			swapChainDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
			swapChainDesc.Width = window.getWidth();
			swapChainDesc.Height = window.getHeight();
			swapChainDesc.BufferCount = NUM_BACK_BUFFERS;
			swapChainDesc.BufferUsage = DXGI_USAGE_BACK_BUFFER | DXGI_USAGE_RENDER_TARGET_OUTPUT;
			swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;
			swapChainDesc.Flags = 0;
			VERIFY_D3D_RESULT_RETURN_FALSE(factory->CreateSwapChain(device, &swapChainDesc, &mSwapChain));

			createRenderTarget(device);

			return true;
		}

		bool createRenderTarget(ID3D12DeviceRHI* device)
		{

			return true;
		}

		void cleanupRenderTarget()
		{

		}
		struct FrameContext
		{
			ID3D12CommandAllocator* CommandAllocator;
			UINT64                  FenceValue;
		};

		static int const             NUM_BACK_BUFFERS = 3;
		// Data
		int const                    NUM_FRAMES_IN_FLIGHT = 3;
		FrameContext                 g_frameContext[NUM_BACK_BUFFERS] = {};
		UINT                         g_frameIndex = 0;
		ID3D12Device*                g_pd3dDevice = nullptr;
		ID3D12DescriptorHeap*        g_pd3dRtvDescHeap = nullptr;
		ID3D12DescriptorHeap*        g_pd3dSrvDescHeap = nullptr;
		ID3D12CommandQueue*          g_pd3dCommandQueue = nullptr;
		ID3D12GraphicsCommandList*   g_pd3dCommandList = nullptr;
		ID3D12Fence*                 g_fence = nullptr;
		HANDLE                       g_fenceEvent = nullptr;
		UINT64                       g_fenceLastSignaledValue = 0;
		IDXGISwapChain3*             g_pSwapChain = nullptr;
		HANDLE                       g_hSwapChainWaitableObject = nullptr;
		ID3D12Resource*              g_mainRenderTargetResource[NUM_BACK_BUFFERS] = {};
		D3D12_CPU_DESCRIPTOR_HANDLE  g_mainRenderTargetDescriptor[NUM_BACK_BUFFERS] = {};

		TComPtr< IDXGISwapChain3 > mSwapChain;


	};
	virtual bool initialize(EditorWindow& mainWindow)
	{
		mDevice = static_cast<D3D12System*>(GRHISystem)->mDevice;

		VERIFY_RETURN_FALSE(ImGui_ImplWin32_Init(mainWindow.getHWnd()));
		//VERIFY_RETURN_FALSE(ImGui_ImplDX11_Init(mDevice, mDeviceContext));
		VERIFY_RETURN_FALSE(initializeWindowRenderData(mainWindow));

		return true;
	}
	virtual void shutdown()
	{
		ImGui_ImplDX12_Shutdown();
		ImGui_ImplWin32_Shutdown();
	}

	void beginFrame()
	{
		ImGui_ImplDX12_NewFrame();
		ImGui_ImplWin32_NewFrame();
	}
	TComPtr< ID3D12DeviceRHI > mDevice;


	bool initializeWindowRenderData(EditorWindow& window) override
	{
		auto renderData = std::make_unique<WindowRenderData>();
		VERIFY_RETURN_FALSE(renderData->initRenderResource(window, mDevice));
		window.mRenderData = std::move(renderData);
		return true;
	}

	void renderWindow(EditorWindow& window)
	{
		WindowRenderData* renderData = window.getRenderData<WindowRenderData>();

		const float clear_color_with_alpha[4] = { 0,0,0,1 };


		//ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData());
		renderData->mSwapChain->Present(1, 0);
	}

	void notifyWindowResize(EditorWindow& window, int width, int height) override
	{
		WindowRenderData* renderData = window.getRenderData<WindowRenderData>();
		if (renderData->mSwapChain.isValid())
		{
			renderData->cleanupRenderTarget();
			renderData->mSwapChain->ResizeBuffers(0, width, height, DXGI_FORMAT_UNKNOWN, 0);
			renderData->createRenderTarget(mDevice);
		}
	}
#endif
};


class EditorRendererOpenGL : public IEditorRenderer
{
public:

	class WindowRenderData : public IEditorWindowRenderData
	{
	public:
		bool initRenderResource(EditorWindow& window)
		{
			WGLPixelFormat format;
			mContext.init(window.getHDC(), format, true);
			if (!wglShareLists(static_cast<OpenGLSystem*>(GRHISystem)->mGLContext.getHandle(), mContext.getHandle()))
			{
			}
			return true;
		}


		WindowsGLContext mContext;
	};

	virtual bool initialize(EditorWindow& mainWindow)
	{
		VERIFY_RETURN_FALSE(ImGui_ImplWin32_Init(mainWindow.getHWnd()));
		VERIFY_RETURN_FALSE(ImGui_ImplOpenGL3_Init());
		VERIFY_RETURN_FALSE(initializeWindowRenderData(mainWindow));

		return true;
	}
	virtual void shutdown()
	{
		ImGui_ImplOpenGL3_Shutdown();
		ImGui_ImplWin32_Shutdown();
	}

	void beginFrame()
	{
		ImGui_ImplOpenGL3_NewFrame();
		ImGui_ImplWin32_NewFrame();
	}

	bool initializeWindowRenderData(EditorWindow& window) override
	{
		auto renderData = std::make_unique<WindowRenderData>();
		VERIFY_RETURN_FALSE(renderData->initRenderResource(window));
		window.mRenderData = std::move(renderData);
		return true;
	}

	void renderWindow(EditorWindow& window)
	{
		WindowRenderData* renderData = window.getRenderData<WindowRenderData>();

		renderData->mContext.makeCurrent();
		glBindFramebuffer(GL_FRAMEBUFFER, 0);
		glClearColor(0,0,0,1);
		glClear(GL_COLOR_BUFFER_BIT);
		ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
		renderData->mContext.swapBuffer();
	}

	void notifyWindowResize(EditorWindow& window, int width, int height) override
	{
		WindowRenderData* renderData = window.getRenderData<WindowRenderData>();
	}
};

IEditorRenderer* IEditorRenderer::Create()
{
	switch (GRHISystem->getName())
	{
	case RHISystemName::D3D11:
		return new EditorRendererD3D11;
	case RHISystemName::D3D12:
		return new EditorRendererD3D12;
	case RHISystemName::OpenGL:
		return new EditorRendererOpenGL;
	default:
		LogWarning(0, "Editor can't supported %s", Literal::ToString(GRHISystem->getName()));
		return false;
	}
}
