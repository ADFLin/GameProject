#include "EditorRender.h"

#include "RHI/RHICommand.h"
#include "RHI/TextureAtlas.h"
#include "RHI/Font.h"
#include "Platform/Windows/ComUtility.h"

#include "ImGui/backends/imgui_impl_dx11.h"
#include "ImGui/backends/imgui_impl_dx12.h"
#include "ImGui/backends/imgui_impl_opengl3.h"
#include "ImGui/backends/imgui_impl_vulkan.h"

#pragma comment(lib , "DXGI.lib")
#pragma comment(lib , "dxguid.lib")

#include <D3D11.h>
#include <D3D12.h>
#pragma comment(lib , "D3D11.lib")
#pragma comment(lib , "D3D12.lib")

#define USE_VULKAN 1

#include "RHI/D3D11Command.h"
#include "RHI/D3D12Command.h"
#include "RHI/OpenGLCommand.h"
#if USE_VULKAN
#include "RHI/VulkanCommand.h"
#pragma comment(lib , "vulkan-1.lib")
#endif
#include "Renderer/RenderThread.h"

using namespace Render;

IEditorRenderer* GEditorRenderer = nullptr;


#define ERROR_MSG_GENERATE( HR , CODE , FILE , LINE )\
	LogWarning(1, "ErrorCode = 0x%x File = %s Line = %s %s ", HR , FILE, #LINE, #CODE)

#define VERIFY_D3D_RESULT_INNER( FILE , LINE , CODE ,ERRORCODE )\
	{ HRESULT hResult = CODE; if( FAILED(hResult) ){ ERROR_MSG_GENERATE( hResult , CODE, FILE, LINE ); ERRORCODE } }

#define VERIFY_D3D_RESULT( CODE , ERRORCODE ) VERIFY_D3D_RESULT_INNER( __FILE__ , __LINE__ , CODE , ERRORCODE )
#define VERIFY_D3D_RESULT_RETURN_FALSE( CODE ) VERIFY_D3D_RESULT_INNER( __FILE__ , __LINE__ , CODE , return false; )


static void(*GDefaultImGuiRenderer_RenderWindow)(ImGuiViewport* viewport, void* render_arg) = nullptr;
static void ImGui_Renderer_RenderWindow_Hook(ImGuiViewport* viewport, void* render_arg)
{
	if (RenderThread::IsRunning())
	{
		auto* snapshot = ImDrawDataSnapshot::Copy(viewport->DrawData);
		if (snapshot)
		{
			RenderThread::AddCommand("ImGui::Renderer_RenderWindow", [viewport, snapshot]()
			{
				ImDrawData* originalData = viewport->DrawData;
				viewport->DrawData = &snapshot->data;
				if (GDefaultImGuiRenderer_RenderWindow)
				{
					GDefaultImGuiRenderer_RenderWindow(viewport, nullptr);
				}
				viewport->DrawData = originalData;
				delete snapshot;
			});
		}
	}
	else if (GDefaultImGuiRenderer_RenderWindow)
	{
		GDefaultImGuiRenderer_RenderWindow(viewport, render_arg);
	}
}

static void(*GDefaultImGuiRenderer_CreateWindow)(ImGuiViewport* viewport) = nullptr;
static void ImGui_Renderer_CreateWindow_Hook(ImGuiViewport* viewport)
{
	if (RenderThread::IsRunning())
	{
		RenderThread::AddCommand("ImGui::Renderer_CreateWindow", [viewport]()
		{
			if (GDefaultImGuiRenderer_CreateWindow)
				GDefaultImGuiRenderer_CreateWindow(viewport);
		});
		//RenderThread::FlushCommands();
	}
	else if (GDefaultImGuiRenderer_CreateWindow)
	{
		GDefaultImGuiRenderer_CreateWindow(viewport);
	}
}

static void(*GDefaultImGuiRenderer_DestroyWindow)(ImGuiViewport* viewport) = nullptr;
static void ImGui_Renderer_DestroyWindow_Hook(ImGuiViewport* viewport)
{
	if (RenderThread::IsRunning())
	{
		RenderThread::AddCommand("ImGui::Renderer_DestroyWindow", [viewport]()
		{
			if (GDefaultImGuiRenderer_DestroyWindow)
				GDefaultImGuiRenderer_DestroyWindow(viewport);
		});
		//RenderThread::FlushCommands();
	}
	else if (GDefaultImGuiRenderer_DestroyWindow)
	{
		GDefaultImGuiRenderer_DestroyWindow(viewport);
	}
}

static void(*GDefaultImGuiRenderer_SetWindowSize)(ImGuiViewport* viewport, ImVec2 size) = nullptr;
static void ImGui_Renderer_SetWindowSize_Hook(ImGuiViewport* viewport, ImVec2 size)
{
	if (RenderThread::IsRunning())
	{
		RenderThread::AddCommand("ImGui::Renderer_SetWindowSize", [viewport, size]()
		{
			if (GDefaultImGuiRenderer_SetWindowSize)
				GDefaultImGuiRenderer_SetWindowSize(viewport, size);
		});
		//RenderThread::FlushCommands();
	}
	else if (GDefaultImGuiRenderer_SetWindowSize)
	{
		GDefaultImGuiRenderer_SetWindowSize(viewport, size);
	}
}

static void(*GDefaultImGuiRenderer_SwapBuffers)(ImGuiViewport* viewport, void* render_arg) = nullptr;
static void ImGui_Renderer_SwapBuffers_Hook(ImGuiViewport* viewport, void* render_arg)
{
	if (RenderThread::IsRunning())
	{
		RenderThread::AddCommand("ImGui::Renderer_SwapBuffers", [viewport, render_arg]()
		{
			if (GDefaultImGuiRenderer_SwapBuffers)
				GDefaultImGuiRenderer_SwapBuffers(viewport, render_arg);
		});
	}
	else if (GDefaultImGuiRenderer_SwapBuffers)
	{
		GDefaultImGuiRenderer_SwapBuffers(viewport, render_arg);
	}
}

void SetupImGuiRendererHook()
{
	ImGuiPlatformIO& platform_io = ImGui::GetPlatformIO();
	if (platform_io.Renderer_RenderWindow != ImGui_Renderer_RenderWindow_Hook)
	{
		GDefaultImGuiRenderer_RenderWindow = platform_io.Renderer_RenderWindow;
		platform_io.Renderer_RenderWindow = ImGui_Renderer_RenderWindow_Hook;
	}
	if (platform_io.Renderer_CreateWindow != ImGui_Renderer_CreateWindow_Hook)
	{
		GDefaultImGuiRenderer_CreateWindow = platform_io.Renderer_CreateWindow;
		platform_io.Renderer_CreateWindow = ImGui_Renderer_CreateWindow_Hook;
	}
	if (platform_io.Renderer_DestroyWindow != ImGui_Renderer_DestroyWindow_Hook)
	{
		GDefaultImGuiRenderer_DestroyWindow = platform_io.Renderer_DestroyWindow;
		platform_io.Renderer_DestroyWindow = ImGui_Renderer_DestroyWindow_Hook;
	}
	if (platform_io.Renderer_SetWindowSize != ImGui_Renderer_SetWindowSize_Hook)
	{
		GDefaultImGuiRenderer_SetWindowSize = platform_io.Renderer_SetWindowSize;
		platform_io.Renderer_SetWindowSize = ImGui_Renderer_SetWindowSize_Hook;
	}
	if (platform_io.Renderer_SwapBuffers != ImGui_Renderer_SwapBuffers_Hook)
	{
		GDefaultImGuiRenderer_SwapBuffers = platform_io.Renderer_SwapBuffers;
		platform_io.Renderer_SwapBuffers = ImGui_Renderer_SwapBuffers_Hook;
	}
}

class EditorRendererBase : public IEditorRenderer
{
public:
	virtual bool initialize(EditorWindow& mainWindow)
	{
		if ( !doInitialize(mainWindow) )
			return false;

		unsigned char* pixels;
		int width, height;
		ImGui::GetIO().Fonts->GetTexDataAsRGBA32(&pixels, &width, &height);

		SetupImGuiRendererHook();
		return true;
	}

	virtual bool doInitialize(EditorWindow& mainWindow) = 0;
};


class EditorRendererD3D11 : public EditorRendererBase
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

	virtual bool doInitialize(EditorWindow& mainWindow)
	{
		mDevice = static_cast<D3D11System*>(GRHISystem)->mDevice;
		mDeviceContext = static_cast<D3D11System*>(GRHISystem)->mDeviceContext;

		VERIFY_RETURN_FALSE(ImGui_ImplWin32_Init(mainWindow.getHWnd()));
		VERIFY_RETURN_FALSE(ImGui_ImplDX11_Init(mDevice, mDeviceContext));

		if (RenderThread::IsRunning())
		{
			RenderThread::AddCommand("Editor::initializeWindowRenderData", [this, &mainWindow]()
			{
				initializeWindowRenderData(mainWindow);
			});
		}
		else
		{
			VERIFY_RETURN_FALSE(initializeWindowRenderData(mainWindow));
		}
		return true;
	}

	virtual void shutdown()
	{
		ImGui_ImplDX11_Shutdown();
		ImGui_ImplWin32_Shutdown();
	}

	void beginFrame()
	{
		ImGui_ImplWin32_NewFrame();
	}

	void beginRender(EditorWindow& window) override
	{
		ImGui_ImplDX11_NewFrame();
	}
	TComPtr< ID3D11Device > mDevice;
	TComPtr< ID3D11DeviceContext > mDeviceContext;

	bool initializeWindowRenderData(EditorWindow& window) override
	{
		auto renderData = std::make_unique<WindowRenderData>();
		VERIFY_RETURN_FALSE(renderData->initRenderResource(window, mDevice));
		window.mRenderData = std::move(renderData);
		return true;
	}

	void renderWindow(EditorWindow& window, ImDrawData* drawData) override
	{
		WindowRenderData* renderData = window.getRenderData<WindowRenderData>();

		const float clear_color_with_alpha[4] = { 0,0,0,1 };
		mDeviceContext->OMSetRenderTargets(1, &renderData->mRenderTargetView, NULL);
		mDeviceContext->ClearRenderTargetView(renderData->mRenderTargetView, clear_color_with_alpha);
		ImGui_ImplDX11_RenderDrawData(drawData);
		renderData->mSwapChain->Present(0, 0);
	}

	virtual ImTextureID getTextureID(Render::RHITexture2D& texture) override
	{
		return (ImTextureID)static_cast<D3D11Texture2D&>(texture).mSRV.getResource();
	}

	void notifyWindowResize(EditorWindow& window, int width, int height) override
	{
		if (RenderThread::IsRunning())
		{
			RenderThread::AddCommand("Editor::notifyWindowResize", [this, &window, width, height]()
			{
				notifyWindowResizeInternal(window, width, height);
			});
		}
		else
		{
			notifyWindowResizeInternal(window, width, height);
		}
	}

	void notifyWindowResizeInternal(EditorWindow& window, int width, int height)
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

class EditorRendererD3D12 : public EditorRendererBase
{
public:
	class WindowRenderData : public IEditorWindowRenderData
	{
	public:
		bool initRenderResource(EditorWindow& window, D3D12System* system)
		{
			auto device = system->mDevice;

			HRESULT hr;
			TComPtr<IDXGIFactory4> factory;
			hr = CreateDXGIFactory1(IID_PPV_ARGS(&factory));

			DXGI_SWAP_CHAIN_DESC1 swapChainDesc; 
			ZeroMemory(&swapChainDesc, sizeof(swapChainDesc));
			swapChainDesc.Width = window.getWidth();
			swapChainDesc.Height = window.getHeight();
			swapChainDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
			swapChainDesc.SampleDesc.Count = 1;
			swapChainDesc.BufferUsage = DXGI_USAGE_BACK_BUFFER | DXGI_USAGE_RENDER_TARGET_OUTPUT;
			swapChainDesc.BufferCount = NUM_BACK_BUFFERS;
			swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;

			TComPtr<IDXGISwapChain1> swapChain1;
			VERIFY_D3D_RESULT_RETURN_FALSE(factory->CreateSwapChainForHwnd(
				system->mRenderContext.mCommandQueue,
				window.getHWnd(),
				&swapChainDesc,
				nullptr,
				nullptr,
				&swapChain1));

			swapChain1.castTo(mSwapChain);

			createRenderTarget(device);

			for (int i = 0; i < NUM_BACK_BUFFERS; i++)
			{
				device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&mFrameContext[i].CommandAllocator));
			}
			device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, mFrameContext[0].CommandAllocator, nullptr, IID_PPV_ARGS(&mCommandList));
			mCommandList->Close();

			device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&mFence));
			mFenceEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);

			return true;
		}

		bool createRenderTarget(ID3D12DeviceRHI* device)
		{
			D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc = {};
			rtvHeapDesc.NumDescriptors = NUM_BACK_BUFFERS;
			rtvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
			rtvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
			device->CreateDescriptorHeap(&rtvHeapDesc, IID_PPV_ARGS(&mRtvDescHeap));

			SIZE_T rtvDescriptorSize = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
			D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = mRtvDescHeap->GetCPUDescriptorHandleForHeapStart();

			for (UINT i = 0; i < NUM_BACK_BUFFERS; i++)
			{
				mSwapChain->GetBuffer(i, IID_PPV_ARGS(&mRenderTargets[i]));
				device->CreateRenderTargetView(mRenderTargets[i], nullptr, rtvHandle);
				mRtvDescHandles[i] = rtvHandle;
				rtvHandle.ptr += rtvDescriptorSize;
			}
			return true;
		}

		void cleanupRenderTarget()
		{
			for (UINT i = 0; i < NUM_BACK_BUFFERS; i++) mRenderTargets[i].reset();
			mRtvDescHeap.reset();
		}

		struct FrameContext
		{
			TComPtr<ID3D12CommandAllocator> CommandAllocator;
			UINT64 FenceValue = 0;
		};

		static int const NUM_BACK_BUFFERS = 3;
		FrameContext mFrameContext[NUM_BACK_BUFFERS];
		TComPtr<IDXGISwapChain3> mSwapChain;
		TComPtr<ID3D12DescriptorHeap> mRtvDescHeap;
		TComPtr<ID3D12Resource> mRenderTargets[NUM_BACK_BUFFERS];
		D3D12_CPU_DESCRIPTOR_HANDLE mRtvDescHandles[NUM_BACK_BUFFERS];
		TComPtr<ID3D12GraphicsCommandList> mCommandList;

		TComPtr<ID3D12Fence> mFence;
		HANDLE mFenceEvent = nullptr;
		UINT64 mFenceValue = 0;

		~WindowRenderData()
		{
			if (mFenceEvent)
			{
				CloseHandle(mFenceEvent);
				mFenceEvent = nullptr;
			}
		}
	};

	D3D12System* mSystem = nullptr;
	TComPtr<ID3D12DeviceRHI> mDevice;
	TComPtr<ID3D12DescriptorHeap> mSrvDescHeap;
	Render::D3D12RenderTargetsState mEditorRenderTargetsState;
	
	virtual bool doInitialize(EditorWindow& mainWindow) override
	{
		mSystem = static_cast<D3D12System*>(GRHISystem);
		mDevice = mSystem->mDevice;

		mEditorRenderTargetsState.colorBuffers[0].format = DXGI_FORMAT_R8G8B8A8_UNORM;
		mEditorRenderTargetsState.numColorBuffers = 1;
		mEditorRenderTargetsState.updateFormatGUID();

		VERIFY_RETURN_FALSE(ImGui_ImplWin32_Init(mainWindow.getHWnd()));

		D3D12_DESCRIPTOR_HEAP_DESC desc = {};
		desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
		desc.NumDescriptors = 1000;
		desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
		mDevice->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&mSrvDescHeap));

		ImGui_ImplDX12_Init(mDevice.get(), WindowRenderData::NUM_BACK_BUFFERS,
			DXGI_FORMAT_R8G8B8A8_UNORM, mSrvDescHeap.get(),
			mSrvDescHeap->GetCPUDescriptorHandleForHeapStart(),
			mSrvDescHeap->GetGPUDescriptorHandleForHeapStart());

		if (RenderThread::IsRunning())
		{
			RenderThread::AddCommand("Editor::initializeWindowRenderData", [this, &mainWindow]()
			{
				initializeWindowRenderData(mainWindow);
			});
		}
		else
		{
			VERIFY_RETURN_FALSE(initializeWindowRenderData(mainWindow));
		}

		return true;
	}

	virtual void shutdown() override
	{
		ImGui_ImplDX12_Shutdown();
		ImGui_ImplWin32_Shutdown();
		mSrvDescHeap.reset();
	}

	void beginFrame() override
	{
		ImGui_ImplWin32_NewFrame();
	}

	void beginRender(EditorWindow& window) override
	{
		ImGui_ImplDX12_NewFrame();
	}

	bool initializeWindowRenderData(EditorWindow& window) override
	{
		auto renderData = std::make_unique<WindowRenderData>();
		VERIFY_RETURN_FALSE(renderData->initRenderResource(window, mSystem));
		window.mRenderData = std::move(renderData);
		return true;
	}

	void renderWindow(EditorWindow& window, ImDrawData* drawData) override
	{
		WindowRenderData* renderData = window.getRenderData<WindowRenderData>();

		UINT backBufferIdx = renderData->mSwapChain->GetCurrentBackBufferIndex();
		auto cmdAlloc = renderData->mFrameContext[backBufferIdx].CommandAllocator;
		auto cmdList = renderData->mCommandList;

		if (renderData->mFence->GetCompletedValue() < renderData->mFrameContext[backBufferIdx].FenceValue)
		{
			renderData->mFence->SetEventOnCompletion(renderData->mFrameContext[backBufferIdx].FenceValue, renderData->mFenceEvent);
			WaitForSingleObject(renderData->mFenceEvent, INFINITE);
		}

		cmdAlloc->Reset();
		cmdList->Reset(cmdAlloc, nullptr);

		D3D12_RESOURCE_BARRIER barrier = FD3D12Init::TransitionBarrier(renderData->mRenderTargets[backBufferIdx], D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET);
		cmdList->ResourceBarrier(1, &barrier);

		const float clear_color_with_alpha[4] = { 0, 0, 0, 1 };
		cmdList->ClearRenderTargetView(renderData->mRtvDescHandles[backBufferIdx], clear_color_with_alpha, 0, nullptr);
		cmdList->OMSetRenderTargets(1, &renderData->mRtvDescHandles[backBufferIdx], FALSE, nullptr);

#if 1
		for (auto const& pair : mTextureIDMap)
		{
			Render::D3D12Texture2D& textureD3D = static_cast<Render::D3D12Texture2D&>(*pair.first);
			if (textureD3D.mCurrentStates != D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE)
			{
				D3D12_RESOURCE_BARRIER barrier = FD3D12Init::TransitionBarrier(textureD3D.getResource(), textureD3D.mCurrentStates, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
				cmdList->ResourceBarrier(1, &barrier);
				textureD3D.mCurrentStates = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
			}
		}
#endif

		ID3D12DescriptorHeap* descriptorHeaps[] = { mSrvDescHeap.get() };
		cmdList->SetDescriptorHeaps(1, descriptorHeaps);
		mCmdListUsing = cmdList.get();

		auto prevCmdList = mSystem->mRenderContext.mGraphicsCmdList;
		bool bPrevRecording = mSystem->mRenderContext.mbIsRecording;
		auto prevRenderTargetsState = mSystem->mRenderContext.mRenderTargetsState;

		mSystem->mRenderContext.mRenderTargetsState = nullptr;
		mSystem->mRenderContext.setGraphicsCommandList((Render::ID3D12GraphicsCommandListRHI*)cmdList.get(), true);
		mSystem->mRenderContext.mRenderTargetsState = &mEditorRenderTargetsState;

		ImGui_ImplDX12_RenderDrawData(drawData, cmdList.get());

		mSystem->mRenderContext.setGraphicsCommandList(prevCmdList, bPrevRecording);
		mSystem->mRenderContext.mRenderTargetsState = prevRenderTargetsState;

		barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
		barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_PRESENT;
		cmdList->ResourceBarrier(1, &barrier);
		cmdList->Close();

		ID3D12CommandList* ppCommandLists[] = { cmdList.get() };
		mSystem->mRenderContext.mCommandQueue->ExecuteCommandLists(1, ppCommandLists);

		renderData->mSwapChain->Present(1, 0);

		renderData->mFenceValue++;
		renderData->mFrameContext[backBufferIdx].FenceValue = renderData->mFenceValue;
		mSystem->mRenderContext.mCommandQueue->Signal(renderData->mFence.get(), renderData->mFenceValue);
	}

	ID3D12GraphicsCommandList* mCmdListUsing;

	void restoreRenderState() 
	{
		ID3D12DescriptorHeap* descriptorHeaps[] = { mSrvDescHeap.get() };
		mCmdListUsing->SetDescriptorHeaps(1, descriptorHeaps);
	}

	void notifyWindowResize(EditorWindow& window, int width, int height) override
	{
		WindowRenderData* renderData = window.getRenderData<WindowRenderData>();
		if (renderData && renderData->mSwapChain.isValid())
		{
			// Need to flush queue before resizing
			//mSystem->mRenderContext.waitForGpu();
			renderData->cleanupRenderTarget();
			renderData->mSwapChain->ResizeBuffers(0, width, height, DXGI_FORMAT_UNKNOWN, 0);
			renderData->createRenderTarget(mDevice);
		}
	}

	int mSrvAllocCount = 1; // 0 is used for font
	std::unordered_map<Render::RHITexture2D*, ImTextureID> mTextureIDMap;

	virtual ImTextureID getTextureID(Render::RHITexture2D& texture) override
	{
		auto iter = mTextureIDMap.find(&texture);
		if (iter != mTextureIDMap.end())
			return iter->second;

		Render::D3D12Texture2D& textureD3D = static_cast<Render::D3D12Texture2D&>(texture);
		
		int slot = mSrvAllocCount++;
		
		D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle = mSrvDescHeap->GetCPUDescriptorHandleForHeapStart();
		cpuHandle.ptr += slot * mDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

		D3D12_GPU_DESCRIPTOR_HANDLE gpuHandle = mSrvDescHeap->GetGPUDescriptorHandleForHeapStart();
		gpuHandle.ptr += slot * mDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

		mDevice->CopyDescriptorsSimple(1, cpuHandle, textureD3D.mSRV.getCPUHandleCopy(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

		mTextureIDMap[&texture] = (ImTextureID)gpuHandle.ptr;

		return (ImTextureID)gpuHandle.ptr;

	}
};

#if USE_VULKAN

struct FVulkan
{
	static VkSampleCountFlagBits ToSampleCount(int numSamples)
	{
		if (numSamples <= 1) return VK_SAMPLE_COUNT_1_BIT;
		if (numSamples <= 2) return VK_SAMPLE_COUNT_2_BIT;
		if (numSamples <= 4) return VK_SAMPLE_COUNT_4_BIT;
		if (numSamples <= 8) return VK_SAMPLE_COUNT_8_BIT;
		if (numSamples <= 16) return VK_SAMPLE_COUNT_16_BIT;
		if (numSamples <= 32) return VK_SAMPLE_COUNT_32_BIT;
		return VK_SAMPLE_COUNT_64_BIT;
	}

	static uint32_t GetMemoryTypeIndex(VulkanDevice& device, uint32_t typeBits, VkMemoryPropertyFlags properties)
	{
		for (uint32_t i = 0; i < device.memoryProperties.memoryTypeCount; ++i)
		{
			if ((typeBits & (1 << i)) && (device.memoryProperties.memoryTypes[i].propertyFlags & properties) == properties)
				return i;
		}
		return 0xFFFFFFFF;
	}

	static VkSurfaceKHR CreateWindowSurface(VulkanSystem* system, HWND hWnd)
	{
		VkWin32SurfaceCreateInfoKHR createInfo = {};
		createInfo.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
		createInfo.hwnd = hWnd;
		createInfo.hinstance = GetModuleHandle(nullptr);

		VkSurfaceKHR surface;
		if (vkCreateWin32SurfaceKHR(system->mInstance, &createInfo, nullptr, &surface) != VK_SUCCESS)
		{
			return VK_NULL_HANDLE;
		}
		return surface;
	}

	static VkCommandBuffer CreateCommandBuffer(VulkanDevice* device, VkCommandPool pool, VkCommandBufferLevel level, bool begin = false)
	{
		VkCommandBufferAllocateInfo allocInfo = {};
		allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		allocInfo.commandPool = pool;
		allocInfo.level = level;
		allocInfo.commandBufferCount = 1;

		VkCommandBuffer cmdBuffer;
		if (vkAllocateCommandBuffers(device->logicalDevice, &allocInfo, &cmdBuffer) != VK_SUCCESS)
			return VK_NULL_HANDLE;

		if (begin)
		{
			VkCommandBufferBeginInfo beginInfo = {};
			beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
			vkBeginCommandBuffer(cmdBuffer, &beginInfo);
		}
		return cmdBuffer;
	}

	static void FlushCommandBuffer(VulkanDevice* device, VkCommandBuffer commandBuffer, VkQueue queue, VkCommandPool pool, bool free = true, VkSemaphore waitSemaphore = VK_NULL_HANDLE)
	{
		if (commandBuffer == VK_NULL_HANDLE)
			return;

		vkEndCommandBuffer(commandBuffer);

		VkSubmitInfo submitInfo = {};
		submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		submitInfo.commandBufferCount = 1;
		submitInfo.pCommandBuffers = &commandBuffer;

		VkPipelineStageFlags waitStage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		if (waitSemaphore != VK_NULL_HANDLE)
		{
			submitInfo.waitSemaphoreCount = 1;
			submitInfo.pWaitSemaphores = &waitSemaphore;
			submitInfo.pWaitDstStageMask = &waitStage;
		}

		VkFenceCreateInfo fenceInfo = {};
		fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
		VkFence fence;
		vkCreateFence(device->logicalDevice, &fenceInfo, nullptr, &fence);

		vkQueueSubmit(queue, 1, &submitInfo, fence);
		vkWaitForFences(device->logicalDevice, 1, &fence, VK_TRUE, UINT64_MAX);
		vkDestroyFence(device->logicalDevice, fence, nullptr);

		if (free)
		{
			vkFreeCommandBuffers(device->logicalDevice, pool, 1, &commandBuffer);
		}
	}

	static bool InitializeSwapChain(VulkanSwapChainData* swapChain, VulkanDevice& device, VkSurfaceKHR surface, uint32 const usageQueueFamilyIndices[], VkFormat depthFormat, int numSamples, bool bVSync)
	{
		swapChain->mDevice = &device;
		swapChain->mNumSamples = numSamples;

		VkSurfaceCapabilitiesKHR capabilities;
		vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device.physicalDevice, surface, &capabilities);

		uint32 formatCount;
		vkGetPhysicalDeviceSurfaceFormatsKHR(device.physicalDevice, surface, &formatCount, nullptr);
		TArray<VkSurfaceFormatKHR> formats; formats.resize(formatCount);
		vkGetPhysicalDeviceSurfaceFormatsKHR(device.physicalDevice, surface, &formatCount, formats.data());

		uint32 presentModeCount;
		vkGetPhysicalDeviceSurfacePresentModesKHR(device.physicalDevice, surface, &presentModeCount, nullptr);
		TArray<VkPresentModeKHR> presentModes; presentModes.resize(presentModeCount);
		vkGetPhysicalDeviceSurfacePresentModesKHR(device.physicalDevice, surface, &presentModeCount, presentModes.data());

		VkSurfaceFormatKHR usageFormat = formats[0];
		for (const auto& availableFormat : formats)
		{
			if (availableFormat.format == VK_FORMAT_B8G8R8A8_UNORM &&
				availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
			{
				usageFormat = availableFormat;
				break;
			}
		}

		VkPresentModeKHR presentMode = VK_PRESENT_MODE_FIFO_KHR;
		if (!bVSync)
		{
			for (const auto& availablePresentMode : presentModes)
			{
				if (availablePresentMode == VK_PRESENT_MODE_MAILBOX_KHR)
				{
					presentMode = availablePresentMode;
					break;
				}
				else if (availablePresentMode == VK_PRESENT_MODE_IMMEDIATE_KHR)
				{
					presentMode = availablePresentMode;
				}
			}
		}

		swapChain->mImageFormat = usageFormat.format;
		swapChain->mImageSize = capabilities.currentExtent;

		uint32 imageCount = capabilities.minImageCount + 1;
		if (capabilities.maxImageCount > 0 && imageCount > capabilities.maxImageCount)
		{
			imageCount = capabilities.maxImageCount;
		}

		VkSwapchainCreateInfoKHR swapChainInfo = {};
		swapChainInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
		swapChainInfo.surface = surface;
		swapChainInfo.minImageCount = imageCount;
		swapChainInfo.imageFormat = usageFormat.format;
		swapChainInfo.imageColorSpace = usageFormat.colorSpace;
		swapChainInfo.imageExtent = swapChain->mImageSize;
		swapChainInfo.imageArrayLayers = 1;
		swapChainInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
		swapChainInfo.preTransform = capabilities.currentTransform;
		swapChainInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
		swapChainInfo.presentMode = presentMode;
		swapChainInfo.clipped = VK_TRUE;

		if (usageQueueFamilyIndices[EQueueFamily::Graphics] != usageQueueFamilyIndices[EQueueFamily::Present])
		{
			swapChainInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
			swapChainInfo.pQueueFamilyIndices = usageQueueFamilyIndices;
			swapChainInfo.queueFamilyIndexCount = 2;
		}
		else
		{
			swapChainInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
		}

		if (vkCreateSwapchainKHR(device.logicalDevice, &swapChainInfo, nullptr, &swapChain->mSwapChain) != VK_SUCCESS)
			return false;

		vkGetSwapchainImagesKHR(device.logicalDevice, swapChain->mSwapChain, &imageCount, nullptr);
		swapChain->mImages.resize(imageCount);
		vkGetSwapchainImagesKHR(device.logicalDevice, swapChain->mSwapChain, &imageCount, swapChain->mImages.data());

		for (auto image : swapChain->mImages)
		{
			VkImageViewCreateInfo imageViewInfo = {};
			imageViewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
			imageViewInfo.image = image;
			imageViewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
			imageViewInfo.format = swapChain->mImageFormat;
			imageViewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			imageViewInfo.subresourceRange.levelCount = 1;
			imageViewInfo.subresourceRange.layerCount = 1;

			VkImageView imageView;
			if (vkCreateImageView(device.logicalDevice, &imageViewInfo, nullptr, &imageView) != VK_SUCCESS)
				return false;
			swapChain->mImageViews.push_back(imageView);
		}

		swapChain->mDepthFormat = depthFormat;
		if (swapChain->mDepthFormat != VK_FORMAT_UNDEFINED)
		{
			VkImageCreateInfo imageInfo = {};
			imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
			imageInfo.imageType = VK_IMAGE_TYPE_2D;
			imageInfo.extent.width = swapChain->mImageSize.width;
			imageInfo.extent.height = swapChain->mImageSize.height;
			imageInfo.extent.depth = 1;
			imageInfo.mipLevels = 1;
			imageInfo.arrayLayers = 1;
			imageInfo.format = swapChain->mDepthFormat;
			imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
			imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
			imageInfo.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
			imageInfo.samples = FVulkan::ToSampleCount(swapChain->mNumSamples);
			imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

			vkCreateImage(device.logicalDevice, &imageInfo, nullptr, &swapChain->mDepthImage);

			VkMemoryRequirements memReqs;
			vkGetImageMemoryRequirements(device.logicalDevice, swapChain->mDepthImage, &memReqs);

			VkMemoryAllocateInfo memAlloc = {};
			memAlloc.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
			memAlloc.allocationSize = memReqs.size;
			memAlloc.memoryTypeIndex = FVulkan::GetMemoryTypeIndex(device, memReqs.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
			vkAllocateMemory(device.logicalDevice, &memAlloc, nullptr, &swapChain->mDepthMemory);
			vkBindImageMemory(device.logicalDevice, swapChain->mDepthImage, swapChain->mDepthMemory, 0);

			VkImageViewCreateInfo viewInfo = {};
			viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
			viewInfo.image = swapChain->mDepthImage;
			viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
			viewInfo.format = swapChain->mDepthFormat;
			viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
			if (swapChain->mDepthFormat == VK_FORMAT_D32_SFLOAT_S8_UINT || swapChain->mDepthFormat == VK_FORMAT_D24_UNORM_S8_UINT)
				viewInfo.subresourceRange.aspectMask |= VK_IMAGE_ASPECT_STENCIL_BIT;
			viewInfo.subresourceRange.baseMipLevel = 0;
			viewInfo.subresourceRange.levelCount = 1;
			viewInfo.subresourceRange.baseArrayLayer = 0;
			viewInfo.subresourceRange.layerCount = 1;

			vkCreateImageView(device.logicalDevice, &viewInfo, nullptr, &swapChain->mDepthView);
		}

		if (swapChain->mNumSamples > 1)
		{
			VkImageCreateInfo imageInfo = {};
			imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
			imageInfo.imageType = VK_IMAGE_TYPE_2D;
			imageInfo.extent.width = swapChain->mImageSize.width;
			imageInfo.extent.height = swapChain->mImageSize.height;
			imageInfo.extent.depth = 1;
			imageInfo.mipLevels = 1;
			imageInfo.arrayLayers = 1;
			imageInfo.format = swapChain->mImageFormat;
			imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
			imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
			imageInfo.usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT;
			imageInfo.samples = FVulkan::ToSampleCount(swapChain->mNumSamples);
			imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

			vkCreateImage(device.logicalDevice, &imageInfo, nullptr, &swapChain->mMSAAColorImage);

			VkMemoryRequirements memReqs;
			vkGetImageMemoryRequirements(device.logicalDevice, swapChain->mMSAAColorImage, &memReqs);

			VkMemoryAllocateInfo memAlloc = {};
			memAlloc.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
			memAlloc.allocationSize = memReqs.size;
			memAlloc.memoryTypeIndex = FVulkan::GetMemoryTypeIndex(device, memReqs.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
			vkAllocateMemory(device.logicalDevice, &memAlloc, nullptr, &swapChain->mMSAAColorMemory);
			vkBindImageMemory(device.logicalDevice, swapChain->mMSAAColorImage, swapChain->mMSAAColorMemory, 0);

			VkImageViewCreateInfo viewInfo = {};
			viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
			viewInfo.image = swapChain->mMSAAColorImage;
			viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
			viewInfo.format = swapChain->mImageFormat;
			viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			viewInfo.subresourceRange.baseMipLevel = 0;
			viewInfo.subresourceRange.levelCount = 1;
			viewInfo.subresourceRange.baseArrayLayer = 0;
			viewInfo.subresourceRange.layerCount = 1;

			vkCreateImageView(device.logicalDevice, &viewInfo, nullptr, &swapChain->mMSAAColorView);
		}

		return true;
	}

	static void CleanupSwapChain(VulkanSwapChainData* swapChain)
	{
		if (swapChain->mDevice == nullptr)
			return;

		VkDevice device = swapChain->mDevice->logicalDevice;
		for (auto view : swapChain->mImageViews)
		{
			vkDestroyImageView(device, view, nullptr);
		}
		swapChain->mImageViews.clear();
		swapChain->mImages.clear();

		if (swapChain->mDepthView) vkDestroyImageView(device, swapChain->mDepthView, nullptr);
		if (swapChain->mDepthImage) vkDestroyImage(device, swapChain->mDepthImage, nullptr);
		if (swapChain->mDepthMemory) vkFreeMemory(device, swapChain->mDepthMemory, nullptr);
		swapChain->mDepthView = VK_NULL_HANDLE;
		swapChain->mDepthImage = VK_NULL_HANDLE;
		swapChain->mDepthMemory = VK_NULL_HANDLE;

		if (swapChain->mMSAAColorView) vkDestroyImageView(device, swapChain->mMSAAColorView, nullptr);
		if (swapChain->mMSAAColorImage) vkDestroyImage(device, swapChain->mMSAAColorImage, nullptr);
		if (swapChain->mMSAAColorMemory) vkFreeMemory(device, swapChain->mMSAAColorMemory, nullptr);
		swapChain->mMSAAColorView = VK_NULL_HANDLE;
		swapChain->mMSAAColorImage = VK_NULL_HANDLE;
		swapChain->mMSAAColorMemory = VK_NULL_HANDLE;

		if (swapChain->mSwapChain != VK_NULL_HANDLE)
		{
			vkDestroySwapchainKHR(device, swapChain->mSwapChain, nullptr);
			swapChain->mSwapChain = VK_NULL_HANDLE;
		}
	}

	static VkImageView CreateTextureView(VulkanTexture& texture, int level, int indexLayer = 0)
	{
		VkImageViewCreateInfo creatInfo = FVulkanInit::imageViewCreateInfo();
		creatInfo.viewType = texture.mViewType;
		creatInfo.format = texture.mFormatVK;
		if (texture.mFormatVK == VK_FORMAT_R8_UNORM || texture.mFormatVK == VK_FORMAT_R16_UNORM)
		{
			creatInfo.components = { VK_COMPONENT_SWIZZLE_R, VK_COMPONENT_SWIZZLE_R, VK_COMPONENT_SWIZZLE_R, VK_COMPONENT_SWIZZLE_R };
		}
		else
		{
			creatInfo.components = { VK_COMPONENT_SWIZZLE_R, VK_COMPONENT_SWIZZLE_G, VK_COMPONENT_SWIZZLE_B, VK_COMPONENT_SWIZZLE_A };
		}
		creatInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		creatInfo.subresourceRange.baseMipLevel = level;
		creatInfo.subresourceRange.levelCount = 1;
		creatInfo.subresourceRange.baseArrayLayer = indexLayer;
		creatInfo.subresourceRange.layerCount = (texture.mViewType == VK_IMAGE_VIEW_TYPE_CUBE) ? 6 : 1;
		creatInfo.image = texture.image;

		VkImageView result = VK_NULL_HANDLE;
		vkCreateImageView(texture.mDevice, &creatInfo, nullptr, &result);
		return result;

	}

	static int ImGuiCreateVkSurface(ImGuiViewport* viewport, ImU64 instance, const void* allocator, ImU64* out_surface)
	{
		VkWin32SurfaceCreateInfoKHR createInfo = {};
		createInfo.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
		createInfo.hwnd = (HWND)viewport->PlatformHandle;
		createInfo.hinstance = GetModuleHandle(nullptr);

		VkSurfaceKHR surface = VK_NULL_HANDLE;
		VkResult err = vkCreateWin32SurfaceKHR((VkInstance)instance, &createInfo, (const VkAllocationCallbacks*)allocator, &surface);
		if (err == VK_SUCCESS)
		{
			*out_surface = (ImU64)surface;
		}
		return (int)err;
	}
};

class EditorRendererVulkan : public EditorRendererBase
{
public:

	class WindowRenderData : public IEditorWindowRenderData
	{
	public:
		bool initRenderResource(EditorWindow& window, VulkanSystem* system)
		{
			mSurface = FVulkan::CreateWindowSurface(system, window.getHWnd());
			if (mSurface == VK_NULL_HANDLE)
				return false;

			mSwapChain = std::make_unique<VulkanSwapChainData>();
			if (!FVulkan::InitializeSwapChain(mSwapChain.get(), *system->mDevice, mSurface, system->mUsageQueueFamilyIndices, 
				system->mSwapChain->mDepthFormat, system->mNumSamples, false))
				return false;

			VkSemaphoreCreateInfo semaphoreInfo = {};
			semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
			vkCreateSemaphore(system->mDevice->logicalDevice, &semaphoreInfo, nullptr, &mImageAvailableSemaphore);

			// Use the system's swap chain render pass for compatibility
			mRenderPass = system->mSwapChainRenderPass;
			createFramebuffers(system->mDevice->logicalDevice);

			return true;
		}


		void createFramebuffers(VkDevice device)
		{
			mFramebuffers.resize(mSwapChain->mImageViews.size());
			for (size_t i = 0; i < mSwapChain->mImageViews.size(); i++)
			{
				VkImageView attachments[3];
				uint32 attachmentCount = 0;
				if (mSwapChain->mNumSamples > 1)
				{
					attachments[attachmentCount++] = mSwapChain->mMSAAColorView;
					attachments[attachmentCount++] = mSwapChain->mImageViews[i];
				}
				else
				{
					attachments[attachmentCount++] = mSwapChain->mImageViews[i];
				}

				if (mSwapChain->mDepthView != VK_NULL_HANDLE)
				{
					attachments[attachmentCount++] = mSwapChain->mDepthView;
				}

				VkFramebufferCreateInfo framebufferInfo = {};
				framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
				framebufferInfo.renderPass = mRenderPass;
				framebufferInfo.attachmentCount = attachmentCount;
				framebufferInfo.pAttachments = attachments;
				framebufferInfo.width = mSwapChain->mImageSize.width;
				framebufferInfo.height = mSwapChain->mImageSize.height;
				framebufferInfo.layers = 1;
				vkCreateFramebuffer(device, &framebufferInfo, nullptr, &mFramebuffers[i]);
			}
		}

		void cleanup(VkDevice device, VkInstance instance)
		{
			for (auto fb : mFramebuffers) vkDestroyFramebuffer(device, fb, nullptr);
			mFramebuffers.clear();
			// Note: mRenderPass is shared from system, don't destroy it here
			if (mImageAvailableSemaphore) vkDestroySemaphore(device, mImageAvailableSemaphore, nullptr);
			if (mSwapChain) FVulkan::CleanupSwapChain(mSwapChain.get());
			if (mSurface) vkDestroySurfaceKHR(instance, mSurface, nullptr);
		}

		VkSurfaceKHR mSurface = VK_NULL_HANDLE;
		std::unique_ptr<VulkanSwapChainData> mSwapChain;
		VkRenderPass mRenderPass = VK_NULL_HANDLE;
		VkSemaphore mImageAvailableSemaphore = VK_NULL_HANDLE;
		TArray<VkFramebuffer> mFramebuffers;
	};

	virtual bool doInitialize(EditorWindow& mainWindow) override
	{
		mSystem = static_cast<VulkanSystem*>(GRHISystem);
		mDevice = mSystem->mDevice;

		VkDescriptorPoolSize pool_sizes[] =
		{
			{ VK_DESCRIPTOR_TYPE_SAMPLER, 1000 },
			{ VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1000 },
			{ VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 1000 },
			{ VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1000 },
			{ VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, 1000 },
			{ VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, 1000 },
			{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1000 },
			{ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1000 },
			{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 1000 },
			{ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, 1000 },
			{ VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 1000 }
		};
		VkDescriptorPoolCreateInfo pool_info = {};
		pool_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
		pool_info.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
		pool_info.maxSets = 1000 * ARRAY_SIZE(pool_sizes);
		pool_info.poolSizeCount = (uint32_t)ARRAY_SIZE(pool_sizes);
		pool_info.pPoolSizes = pool_sizes;
		vkCreateDescriptorPool(mDevice->logicalDevice, &pool_info, nullptr, &mDescriptorPool);

		ImGui_ImplWin32_Init(mainWindow.getHWnd());

		if (ImGui::GetIO().ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
		{
			ImGuiPlatformIO& platform_io = ImGui::GetPlatformIO();
			platform_io.Platform_CreateVkSurface = FVulkan::ImGuiCreateVkSurface;
		}

		ImGui_ImplVulkan_InitInfo init_info = {};
		init_info.Instance = mSystem->mInstance;
		init_info.PhysicalDevice = mDevice->physicalDevice;
		init_info.Device = mDevice->logicalDevice;
		init_info.QueueFamily = mSystem->mUsageQueueFamilyIndices[EQueueFamily::Graphics];
		init_info.Queue = mSystem->mGraphicsQueue;
		init_info.DescriptorPool = mDescriptorPool;
		init_info.Subpass = 0;
		init_info.MinImageCount = mSystem->mSwapChain->getImageCount();
		init_info.ImageCount = mSystem->mSwapChain->getImageCount();
		init_info.MSAASamples = FVulkan::ToSampleCount(mSystem->mSwapChain->mNumSamples);
		init_info.Allocator = nullptr;

		ImGui_ImplVulkan_Init(&init_info, mSystem->mSwapChainRenderPass);

		{
			VkCommandBuffer command_buffer = FVulkan::CreateCommandBuffer(mDevice, mSystem->mGraphicsCommandPool, VK_COMMAND_BUFFER_LEVEL_PRIMARY, true);
			ImGui_ImplVulkan_CreateFontsTexture(command_buffer);
			FVulkan::FlushCommandBuffer(mDevice, command_buffer, mSystem->mGraphicsQueue, mSystem->mGraphicsCommandPool);
			ImGui_ImplVulkan_DestroyFontUploadObjects();
		}

		if (RenderThread::IsRunning())
		{
			RenderThread::AddCommand("Editor::initializeWindowRenderData", [this, &mainWindow]()
			{
				initializeWindowRenderData(mainWindow);
			});
		}
		else
		{
			initializeWindowRenderData(mainWindow);
		}

		return true;
	}

	virtual void shutdown() override
	{
		vkDeviceWaitIdle(mDevice->logicalDevice);
		ImGui_ImplVulkan_Shutdown();
		ImGui_ImplWin32_Shutdown();
		vkDestroyDescriptorPool(mDevice->logicalDevice, mDescriptorPool, nullptr);
	}

	virtual void beginFrame() override
	{
		ImGui_ImplWin32_NewFrame();
	}

	virtual void beginRender(EditorWindow& window) override
	{
		ImGui_ImplVulkan_NewFrame();
	}

	virtual bool initializeWindowRenderData(EditorWindow& window) override
	{
		auto renderData = std::make_unique<WindowRenderData>();
		if (!renderData->initRenderResource(window, mSystem))
			return false;
		window.mRenderData = std::move(renderData);
		return true;
	}

	virtual void renderWindow(EditorWindow& window, ImDrawData* drawData) override
	{
		WindowRenderData* renderData = window.getRenderData<WindowRenderData>();
		if (!renderData) return;

		uint32_t imageIndex;
		VkResult result = vkAcquireNextImageKHR(mDevice->logicalDevice, renderData->mSwapChain->mSwapChain, UINT64_MAX, renderData->mImageAvailableSemaphore, VK_NULL_HANDLE, &imageIndex);
		if (result == VK_ERROR_OUT_OF_DATE_KHR) return;

		VkCommandBuffer cmdBuffer = FVulkan::CreateCommandBuffer(mDevice, mSystem->mGraphicsCommandPool, VK_COMMAND_BUFFER_LEVEL_PRIMARY, true);

		VkRenderPassBeginInfo renderPassInfo = {};
		renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
		renderPassInfo.renderPass = renderData->mRenderPass;
		renderPassInfo.framebuffer = renderData->mFramebuffers[imageIndex];
		renderPassInfo.renderArea.offset = { 0, 0 };
		renderPassInfo.renderArea.extent = renderData->mSwapChain->mImageSize;

		TArray<VkClearValue> clearValues;
		VkClearValue clearColor = { { { 0.0f, 0.0f, 0.0f, 1.0f } } };
		clearValues.push_back(clearColor);

		if (mSystem->mNumSamples > 1)
		{
			VkClearValue padding = {};
			clearValues.push_back(padding);
		}

		if (renderData->mSwapChain->mDepthView != VK_NULL_HANDLE)
		{
			VkClearValue clearDepth = {};
			clearDepth.depthStencil = { 1.0f, 0 };
			clearValues.push_back(clearDepth);
		}

		renderPassInfo.clearValueCount = clearValues.size();
		renderPassInfo.pClearValues = clearValues.data();

		mSystem->mDrawContext.mActiveCmdBuffer = cmdBuffer;
		mSystem->mProfileCore->mCmdList = cmdBuffer;
		mSystem->mDrawContext.mInsideRenderPass = true;

		vkCmdBeginRenderPass(cmdBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
		ImGui_ImplVulkan_RenderDrawData(drawData, cmdBuffer);
		
		if (mSystem->mDrawContext.mInsideRenderPass)
		{
			vkCmdEndRenderPass(cmdBuffer);
			mSystem->mDrawContext.mInsideRenderPass = false;
		}

		mSystem->mDrawContext.mActiveCmdBuffer = VK_NULL_HANDLE;
		mSystem->mProfileCore->mCmdList = VK_NULL_HANDLE;

		FVulkan::FlushCommandBuffer(mDevice, cmdBuffer, mSystem->mGraphicsQueue, mSystem->mGraphicsCommandPool, true, renderData->mImageAvailableSemaphore);

		VkPresentInfoKHR presentInfo = {};
		presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
		presentInfo.swapchainCount = 1;
		presentInfo.pSwapchains = &renderData->mSwapChain->mSwapChain;
		presentInfo.pImageIndices = &imageIndex;
		vkQueuePresentKHR(mSystem->mPresentQueue, &presentInfo);
	}

	virtual void notifyWindowResize(EditorWindow& window, int width, int height) override
	{
		if (RenderThread::IsRunning())
		{
			RenderThread::AddCommand("Editor::notifyWindowResize", [this, &window, width, height]()
			{
				notifyWindowResizeInternal(window, width, height);
			});
		}
		else
		{
			notifyWindowResizeInternal(window, width, height);
		}
	}

	void notifyWindowResizeInternal(EditorWindow& window, int width, int height)
	{
		WindowRenderData* renderData = window.getRenderData<WindowRenderData>();
		if (renderData && renderData->mSwapChain)
		{
			renderData->cleanup(mDevice->logicalDevice, mSystem->mInstance);
			renderData->initRenderResource(window, mSystem);
		}
	}

	VulkanSystem* mSystem = nullptr;
	VulkanDevice* mDevice = nullptr;
	VkDescriptorPool mDescriptorPool = VK_NULL_HANDLE;

	std::unordered_map<Render::RHITexture2D*, ImTextureID> mTextureIDMap;
	virtual ImTextureID getTextureID(Render::RHITexture2D& texture) override
	{
		auto iter = mTextureIDMap.find(&texture);
		if (iter != mTextureIDMap.end())
			return iter->second;

		VulkanTexture2D& textureVK = static_cast<VulkanTexture2D&>(texture);
		VkDescriptorSet descriptorSet = ImGui_ImplVulkan_AddTexture(mDevice->mDefaultSampler, FVulkan::CreateTextureView(textureVK, 0), VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
		mTextureIDMap[&texture] = (ImTextureID)descriptorSet;
		return (ImTextureID)descriptorSet;
	}
};
#endif


class EditorRendererOpenGL : public EditorRendererBase
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

	virtual bool doInitialize(EditorWindow& mainWindow)
	{
		VERIFY_RETURN_FALSE(ImGui_ImplWin32_Init(mainWindow.getHWnd()));
		VERIFY_RETURN_FALSE(ImGui_ImplOpenGL3_Init());
		if (RenderThread::IsRunning())
		{
			RenderThread::AddCommand("Editor::initializeWindowRenderData", [this, &mainWindow]()
			{
				initializeWindowRenderData(mainWindow);
			});
			//RenderThread::FlushCommands();
		}
		else
		{
			VERIFY_RETURN_FALSE(initializeWindowRenderData(mainWindow));
		}

		return true;
	}
	virtual void shutdown()
	{
		ImGui_ImplOpenGL3_Shutdown();
		ImGui_ImplWin32_Shutdown();
	}

	void beginFrame()
	{
		ImGui_ImplWin32_NewFrame();
	}

	void beginRender(EditorWindow& window) override
	{
		ImGui_ImplOpenGL3_NewFrame();
	}

	virtual ImTextureID getTextureID(Render::RHITexture2D& texture) override
	{
		return (ImTextureID)static_cast<OpenGLTexture2D&>(texture).getHandle();
	}

	bool initializeWindowRenderData(EditorWindow& window) override
	{
		auto renderData = std::make_unique<WindowRenderData>();
		VERIFY_RETURN_FALSE(renderData->initRenderResource(window));
		window.mRenderData = std::move(renderData);
		return true;
	}

	void renderWindow(EditorWindow& window, ImDrawData* drawData) override
	{
		WindowRenderData* renderData = window.getRenderData<WindowRenderData>();

		renderData->mContext.makeCurrent();
		glBindFramebuffer(GL_FRAMEBUFFER, 0);
		glClearColor(0,0,0,1);
		glClear(GL_COLOR_BUFFER_BIT);
		ImGui_ImplOpenGL3_RenderDrawData(drawData);
		renderData->mContext.swapBuffer();
	}

	void notifyWindowResize(EditorWindow& window, int width, int height) override
	{
		if (RenderThread::IsRunning())
		{
			RenderThread::AddCommand("Editor::notifyWindowResize", [this, &window, width, height]()
			{
				notifyWindowResizeInternal(window, width, height);
			});
			//RenderThread::FlushCommands();
		}
		else
		{
			notifyWindowResizeInternal(window, width, height);
		}
	}

	void notifyWindowResizeInternal(EditorWindow& window, int width, int height)
	{
		WindowRenderData* renderData = window.getRenderData<WindowRenderData>();
	}
};

IEditorRenderer* IEditorRenderer::Create()
{
	IEditorRenderer* result = nullptr;
	switch (GRHISystem->getName())
	{
	case RHISystemName::D3D11:
		result = new EditorRendererD3D11;
		break;
	case RHISystemName::D3D12:
		result = new EditorRendererD3D12;
		break;
	case RHISystemName::OpenGL:
		result = new EditorRendererOpenGL;
		break;
#if USE_VULKAN
	case RHISystemName::Vulkan:
		result = new EditorRendererVulkan;
		break;
#endif
	default:
		LogWarning(0, "Editor can't supported %s", Literal::ToString(GRHISystem->getName()));
		break;
	}

	GEditorRenderer = result;
	return result;
}
