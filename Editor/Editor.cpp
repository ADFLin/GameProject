#include "Editor.h"


#include "WindowsMessageHandler.h"
#include "WindowsPlatform.h"
#include "Platform/Windows/ComUtility.h"
#include "LogSystem.h"

#define IMGUI_DEFINE_MATH_OPERATORS
#include "ImGui/imgui.h"
#include "ImGui/imgui_internal.h"

#include <D3D11.h>
#include "ImGui/backends/imgui_impl_win32.h"
#include "ImGui/backends/imgui_impl_dx11.h"
#include "EditorPanel.h"

#include "RHI/RHICommand.h"
#include "RHI/D3D11Command.h"
#include "RHI/TextureAtlas.h"
#include "EditorUtils.h"
#include "PropertySet.h"
#include "Renderer/SceneDebug.h"

#include "Widget/GameViewportPanel.h"
#include "RHI/Font.h"
#include "../TinyGame/TinyCore/RenderDebug.h"
#include "Widget/DetailViewPanel.h"




using namespace Render;

#pragma comment(lib , "D3D11.lib")
#pragma comment(lib , "DXGI.lib")
#pragma comment(lib , "dxguid.lib")


#define ERROR_MSG_GENERATE( HR , CODE , FILE , LINE )\
	LogWarning(1, "ErrorCode = 0x%x File = %s Line = %s %s ", HR , FILE, #LINE, #CODE)

#define VERIFY_D3D_RESULT_INNER( FILE , LINE , CODE ,ERRORCODE )\
	{ HRESULT hResult = CODE; if( FAILED(hResult) ){ ERROR_MSG_GENERATE( hResult , CODE, FILE, LINE ); ERRORCODE } }

#define VERIFY_D3D_RESULT( CODE , ERRORCODE ) VERIFY_D3D_RESULT_INNER( __FILE__ , __LINE__ , CODE , ERRORCODE )
#define VERIFY_D3D_RESULT_RETURN_FALSE( CODE ) VERIFY_D3D_RESULT_INNER( __FILE__ , __LINE__ , CODE , return false; )


#define EDITOR_INI "Editor.ini"

IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
static class Editor* GEditor = nullptr;


class EditorWindow;
class IEditorRenderer
{
public:
	~IEditorRenderer() = default;
	virtual bool initialize(EditorWindow& mainWindow){ return true; }
	virtual void shutdown() {}
	virtual void beginFrame() {}
	virtual void endFrame() {}

	virtual bool initializeWindowRenderData(EditorWindow& window) { return true; }
	virtual void renderWindow(EditorWindow& window) {}

	virtual void saveRenderTarget() = 0;
	virtual void resetRenderTarget() = 0;

	virtual void notifyWindowResize(EditorWindow& window, int width, int height) {}
};

class IEditorWindowRenderData
{
public:
	virtual ~IEditorWindowRenderData() = default;
};

class EditorWindow : public WinFrameT< EditorWindow >
{
public:

	EditorWindow()
	{
		mGuiContext = ImGui::CreateContext();
	}

	~EditorWindow()
	{
		ImGui::DestroyContext(mGuiContext);
	}

	LPTSTR getWinClassName() { return TEXT("EditorWindow"); }
	DWORD  getWinStyle() { return WS_OVERLAPPEDWINDOW | WS_VISIBLE | WS_BORDER; }
	DWORD  getWinExtStyle() { return WS_EX_OVERLAPPEDWINDOW; }


	void beginRender()
	{
		ImGui::SetCurrentContext(mGuiContext);
	}
	void endRender()
	{

	}

	LRESULT processMessage(IEditorRenderer& renderer , HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
	{

		ImGui::SetCurrentContext(mGuiContext);

		if (ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam))
			return true;

		switch (msg)
		{
		case WM_SIZE:
			if (wParam != SIZE_MINIMIZED)
			{
				renderer.notifyWindowResize(*this, (UINT)LOWORD(lParam), (UINT)HIWORD(lParam));
			}
			break;
		case WM_SYSCOMMAND:
			if ((wParam & 0xfff0) == SC_KEYMENU)
				return 0;
			break;
		case WM_ENTERSIZEMOVE:
			bSizeMove = true;
			break;

		case WM_EXITSIZEMOVE:
			bSizeMove = false;
			break;
		case WM_DESTROY:
			::PostQuitMessage(0);
			return 0;

#if 0
		case WM_PAINT:
			if (bSizeMove)
			{
				render();
			}
			else
			{
				PAINTSTRUCT ps;
				BeginPaint(hWnd, &ps);
				EndPaint(hWnd, &ps);
			}
			break;
#endif
		}
		return ::DefWindowProc(hWnd, msg, wParam, lParam);
	}

	template< typename RenderData >
	RenderData* getRenderData() { return static_cast<RenderData*>(mRenderData.get()); }

	std::unique_ptr< IEditorWindowRenderData > mRenderData;

	ImGuiContext* mGuiContext;

	bool bSizeMove = false;

};



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

	void saveRenderTarget() override
	{
		mDeviceContext->OMGetRenderTargets(ARRAY_SIZE(mSavedRenderTarget),
			mSavedRenderTarget, &mSavedDepth);
	}
	void resetRenderTarget() override
	{
		mDeviceContext->OMSetRenderTargets(1, mSavedRenderTarget, mSavedDepth);
	}


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


class Editor : public IEditor
{
public:

	std::unique_ptr<IEditorRenderer> mRenderer;
	RHIGraphics2D* mGraphics = nullptr;


	Editor()
	{
		GEditor = this;
	}

	~Editor()
	{
		mRenderer->shutdown();
	}

	bool initialize()
	{
		mSetttings.loadFile(EDITOR_INI);
		return true;
	}

	bool initializeRender()
	{
		if (!mMainWindow.create("Editor", 1600, 900, &WndProc) )
			return false;

		addWindow(mMainWindow);

		ImGuiIO& io = ImGui::GetIO();
		io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
		io.ConfigFlags |= ImGuiConfigFlags_NavEnableSetMousePos; // Enable some options
		io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
		io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;

		switch (GRHISystem->getName())
		{
		case RHISystemName::D3D11:
			mRenderer = std::make_unique<EditorRendererD3D11>();
			break;
		default:
			LogWarning(0, "Editor can't supported %s", Literal::ToString(GRHISystem->getName()));
			return false;
		} 
		
		VERIFY_RETURN_FALSE(mRenderer->initialize(mMainWindow));

		mGraphics = new RHIGraphics2D;
		mGraphics->initializeRHI();
		FImGui::InitializeRHI();

		importStyle();

		return true;
	}

	void createChildWindow()
	{
		EditorWindow* newWindow = new EditorWindow;

	}

	void addWindow(EditorWindow& window)
	{
	
		mWindowMap.emplace(window.getHWnd(), &window);
	}

	void release() override
	{
		cleanupPanels();
		FImGui::ReleaseRHI();
		delete this;
	}

	void update() override
	{

	}

	bool bPanelInitialized = false;


	void render() override
	{

		mRenderer->beginFrame();
		ImGui::NewFrame();

		if (bPanelInitialized == false)
		{
			bPanelInitialized = true;

			for (auto& info : EditorPanelInfo::List)
			{
				if (info.desc.bAlwaysCreation)
				{
					ActivePanel& panel = findOrAddPanel(info);
					panel.bOpenRequest = true;
				}
				else
				{
#if 0
					ImGuiID id = ImHashStr(info.desc.name);
					ImGuiWindowSettings* settings = ImGui::FindWindowSettings(id);
					if (settings)
					{
						ActivePanel& panel = findOrAddPanel(info);
						//panel.bOpenRequest = !settings->Closed;
					}
#endif
				}
			}
		}

		mMainWindow.beginRender();

		ImGui::DockSpaceOverViewport(ImGui::GetMainViewport());
		renderMainMenu();

		for (auto& panel : mPanels)
		{
			if (panel.bOpened == false && panel.bOpenRequest == false )
				continue;


			WindowRenderParams params;
			panel.widget->getRenderParams(params);
			ImGui::SetNextWindowSize(params.InitSize, ImGuiCond_FirstUseEver);

			bool bStillOpened = true;
			if (ImGui::Begin(panel.name.c_str(), &bStillOpened, params.flags))
			{
				panel.bOpenRequest = false;
				if (panel.bOpened == false)
				{
					panel.widget->onOpen();
					panel.bOpened = true;
				}

				if (panel.bShown == false)
				{
					panel.widget->onShow();
					panel.bShown = true;
				}

				if (panel.widget->preRender())
				{
					panel.widget->render();
					panel.widget->postRender();
				}

				if (bStillOpened == false)
				{
					panel.widget->onClose();
					panel.bOpened = false;
				}
			}
			else
			{
				if (panel.bOpened)
				{
					panel.widget->onHide();
					panel.bShown = false;
				}
			}
			ImGui::End();
		}

		// End of frame: render Dear ImGui
		ImGui::Render();
		mRenderer->renderWindow(mMainWindow);

		ImGui::UpdatePlatformWindows();
		ImGui::RenderPlatformWindowsDefault();
	}

	void addGameViewport(IEditorGameViewport* viewport) override
	{
		EditorPanelInfo const* info = EditorPanelInfo::Find("GameViewport");
		if (info == nullptr)
		{
			return;
		}

		ActivePanel* newPanel = createNamedPanel("GameViewport");
		if (newPanel)
		{
			static_cast<GameViewportPanel*>(newPanel->widget)->mViewport = viewport;
			mViewportPanels.push_back(newPanel->widget);
		}
	}


	TArray< IEditorPanel* > mViewportPanels;

	Render::ITextureShowManager* mTextureShowManager = nullptr;

	void setTextureShowManager(ITextureShowManager* manager) override
	{
		mTextureShowManager = manager;
	}
	void renderMainMenu()
	{
		static bool bShowTextureViewer = true;
		static bool bShowEditorPreferences = true;
		static bool showDemoWindow = true;

		if (ImGui::BeginMainMenuBar())
		{
			if (ImGui::BeginMenu("Editor"))
			{
				if (ImGui::MenuItem("Editor Preferences.."))
				{
					bShowEditorPreferences = true;
				}
				ImGui::EndMenu();
			}

			auto ActMenuPanel = [](ActivePanel& panel)
			{						
				if (!panel.bOpened)
				{
					panel.bOpenRequest = true;
				}
				else
				{
					ImGui::SetWindowFocus(panel.name.c_str());
				}
			};
			if (ImGui::BeginMenu("Window"))
			{
				for (auto& info : EditorPanelInfo::List)
				{
					if ( !info.desc.bSingleton )
						continue;

					ActivePanel* panel = findPanel(info);
					bool bSelected = panel && panel->bOpened;
					if (ImGui::MenuItem(info.desc.name, NULL , bSelected))
					{
						if (panel == nullptr)
						{
							panel = &addPanel(info);
						}
						ActMenuPanel(*panel);
					}
				}
				if (ImGui::BeginMenu("Viewport"))
				{
					for (auto widget : mViewportPanels)
					{
						int index = mPanels.findIndexPred([widget](ActivePanel const& panel)
						{
							return panel.widget == widget;
						});
						if (index != INDEX_NONE)
						{
							auto& panel = mPanels[index];
							if (ImGui::MenuItem(panel.name.c_str(), NULL, panel.bOpened))
							{
								ActMenuPanel(panel);
							}
						}
					}
					ImGui::EndMenu();
				}
				ImGui::EndMenu();
			}

			if (ImGui::BeginMenu("Debug"))
			{
				if (ImGui::MenuItem("Show Atlas Texture", NULL , &bShowTextureViewer))
				{

				}

				if (ImGui::MenuItem("Show ImGui Demo", NULL, &showDemoWindow))
				{

				}
				ImGui::EndMenu();
			}
			ImGui::EndMainMenuBar();
		}

		if ( bShowEditorPreferences )
		{
			ImGui::SetNextWindowSize(ImVec2(500, 500), ImGuiCond_FirstUseEver);
			if (ImGui::Begin("Editor Preferences", &bShowEditorPreferences))
			{
				if (ImGui::Button("Save Style"))
				{
					exprotStyle();
					saveSettings();
				}
			}
			ImGui::End();
		}

		if (bShowTextureViewer)
		{
			int width = FImGui::mIconAtlas.getTexture().getSizeX();
			int height = FImGui::mIconAtlas.getTexture().getSizeY();
			ImGui::SetNextWindowSize(ImVec2(width, height), ImGuiCond_FirstUseEver);
			if (ImGui::Begin("Texture Viewer", &bShowTextureViewer, ImGuiWindowFlags_HorizontalScrollbar))
			{
				RHITexture2D* texture = nullptr;
				static int selection = 0;
				ImGui::AlignTextToFramePadding();
				ImGui::Text("Type");
				ImGui::SameLine();
				ImGui::SetNextItemWidth(200);


				char const* DefaultTextureNames[] =
				{
					"Icon", "Font",
				};
				char const* viewTextureName = "";
				if (selection < ARRAY_SIZE(DefaultTextureNames))
				{
					viewTextureName = DefaultTextureNames[selection];
				}
				else if ( mTextureShowManager )
				{
					int index = ARRAY_SIZE(DefaultTextureNames);
					for (auto const& pair : mTextureShowManager->getTextureMap())
					{
						if (index == selection)
						{
							viewTextureName = pair.first.c_str();
							break;
						}
						++index;
					}
				}

				if (ImGui::BeginCombo("##TextureList", viewTextureName))
				{
					int index = 0;
					for (auto name : DefaultTextureNames)
					{
						const bool is_selected = (selection == index);
						if (ImGui::Selectable(name, is_selected))
							selection = index;

						if (is_selected)
							ImGui::SetItemDefaultFocus();
						++index;

					}
					if (mTextureShowManager)
					{
						for (auto const& pair : mTextureShowManager->getTextureMap())
						{
							const bool is_selected = (selection == index);
							if (ImGui::Selectable(pair.first.c_str(), is_selected))
								selection = index;

							if (is_selected)
								ImGui::SetItemDefaultFocus();
							++index;
						}
					}
					ImGui::EndCombo();
				}

				static float scale = 1.0;
				static bool  bUseAlpha = false;
				ImGui::SameLine();
				ImGui::SetNextItemWidth(200);
				ImGui::Text("Scale");
				ImGui::SameLine();
				ImGui::Checkbox("UseAlpha", &bUseAlpha);
				ImGui::SameLine();
				ImGui::DragFloat("##Scale", &scale, 0.1 , 0.0f , 10.0f);
				switch (selection)
				{
				case 0: texture = &FImGui::mIconAtlas.getTexture(); break;
				case 1: texture = &Render::FontCharCache::Get().getTexture(); break;
				default:
					if (mTextureShowManager)
					{
						int index = ARRAY_SIZE(DefaultTextureNames);
						for (auto& pair : mTextureShowManager->getTextureMap())
						{
							if (index == selection)
							{
								if (pair.second->texture.get())
								{
									texture = const_cast< Render::RHITextureBase*>( pair.second->texture.get() )->getTexture2D();
								}
								break;
							}
							++index;
						}
					}
					break;
				}

				if (texture)
				{
					width = texture->getSizeX();
					height = texture->getSizeY();
					if (!bUseAlpha)
					{
						FImGui::DisableBlend();
					}
					ImGui::Image(FImGui::GetTextureID(*texture), ImVec2(scale * width, scale * height));
					if (!bUseAlpha)
					{
						FImGui::RestoreBlend();
					}
				}
			}
			ImGui::End();
		}

		if (showDemoWindow)
		{
			ImGui::ShowDemoWindow(&showDemoWindow);
		}
	}

	struct ActivePanel
	{
		std::string name;
		EditorPanelInfo const* info;
		IEditorPanel* widget = nullptr;
		bool bOpenRequest = false;
		bool bOpened = false;
		bool bShown = false;
	};

	TArray< ActivePanel > mPanels;
	ActivePanel& findOrAddPanel(EditorPanelInfo const& info)
	{
		ActivePanel* panel = findPanel(info);
		if (panel)
		{
			return *panel;
		}
		return addPanel(info);
	}

	ActivePanel& addPanel(EditorPanelInfo const& info)
	{
		ActivePanel newPanel;
		newPanel.name = info.desc.name;
		newPanel.info = &info;
		newPanel.widget = info.factory->create();
		mPanels.push_back(newPanel);
		return mPanels.back();
	}

	ActivePanel* findPanel(EditorPanelInfo const& info)
	{
		for (auto& panel : mPanels)
		{
			if (panel.info == &info)
				return &panel;
		}
		return nullptr;
	}

	void destroyPanel(IEditorPanel* widget)
	{
		int index = mPanels.findIndexPred([widget](ActivePanel const& panel)
		{
			return panel.widget;
		});
		if (index != INDEX_NONE)
		{
			mPanels.removeIndex(index);
			delete widget;
		}
	}

	void cleanupPanels()
	{
		for (auto& panel : mPanels)
		{
			delete panel.widget;
		}
		mPanels.clear();
	}
	ActivePanel* findOrCreateNamedPanel(char const* className, char const* name = nullptr)
	{
		EditorPanelInfo const* info = EditorPanelInfo::Find(className);
		if (info == nullptr)
		{
			return nullptr;
		}
		ActivePanel* panel = findNamedPanelInternal(*info, className, name);
		if (panel == nullptr)
		{
			panel = createNamedPanelInternal(*info, className, name);
		}
		return panel;
	}

	ActivePanel* findNamedPanel(char const* className, char const* name = nullptr)
	{
		EditorPanelInfo const* info = EditorPanelInfo::Find(className);
		if (info == nullptr)
		{
			return nullptr;
		}
		return findNamedPanelInternal(*info, className, name);
	}

	ActivePanel* createNamedPanel(char const* className, char const* name = nullptr)
	{
		EditorPanelInfo const* info = EditorPanelInfo::Find(className);
		if (info == nullptr)
		{
			return nullptr;
		}
		return createNamedPanelInternal(*info, className, name);
	}

	ActivePanel* findNamedPanelInternal(EditorPanelInfo const& info, char const* className, char const* name)
	{
		if (name)
		{
			for (auto& panel : mPanels)
			{
				if (panel.info == &info && panel.name == name)
					return &panel;
			}
		}
		else
		{
			for (auto& panel : mPanels)
			{
				if (panel.info == &info && panel.name == info.desc.name)
					return &panel;
			}
		}
		return nullptr;
	}

	ActivePanel* createNamedPanelInternal(EditorPanelInfo const& info, char const* className, char const* name = nullptr)
	{
		IEditorPanel* widget = info.factory->create();

		ActivePanel newPanel;
		newPanel.name = (name) ? name : info.desc.name;
		newPanel.info = &info;
		newPanel.widget = widget;
		newPanel.bOpenRequest = true;
		mPanels.push_back(std::move(newPanel));
		return &mPanels.back();
	}

	LRESULT processWindowMessage(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
	{
		auto iter = mWindowMap.find(hWnd);
		if (iter == mWindowMap.end())
		{
			return ::DefWindowProc(hWnd, msg, wParam, lParam);
		}

		EditorWindow* window = iter->second;
		return window->processMessage(*mRenderer, hWnd, msg, wParam, lParam);
	}

	static LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
	{
		return GEditor->processWindowMessage(hWnd, msg, wParam, lParam);
	}

	void importStyle();
	void exprotStyle();

	void saveSettings()
	{
		if (!mSetttings.saveFile(EDITOR_INI))
		{

		}
	}

	EditorWindow mMainWindow;
	TArray< std::unique_ptr< EditorWindow > > mChildWindows;
	std::unordered_map< HWND, EditorWindow* > mWindowMap;
	PropertySet mSetttings;

	IEditorDetailView* createDetailView(DetailViewConfig const& config) override
	{
		class EditorDetailView : public IEditorDetailView
		{
		public:
			DetailViewPanel* panel;
			void release() override
			{
				GEditor->destroyPanel(panel);
				delete this;
			}
			void addPrimitive(Reflection::EPropertyType type, void* ptr)
			{
				panel->addPrimitive(type, ptr);
			}
			void addStruct(Reflection::StructType* structData, void* ptr)
			{
				panel->addStruct(structData, ptr);
			}
			void addEnum(Reflection::EnumType* enumData, void* ptr) override
			{
				panel->addEnum(enumData, ptr);
			}
			void clearProperties() override
			{
				panel->clearProperties();
			}
		};

		ActivePanel* panel = findOrCreateNamedPanel(DetailViewPanel::ClassName, config.name);
		if (panel)
		{
			if (config.name)
			{
				panel->name = config.name;
			}
			EditorDetailView* detailView = new EditorDetailView;
			detailView->panel = (DetailViewPanel*)panel->widget;
			detailView->panel->clearProperties();
			return detailView;
		}

		return nullptr;
	}

};


struct FImGuiSettingHelper
{
public:
	void write(char const* name, bool value)
	{
		settings->setKeyValue(name, section, value);
	}
	void read(char const* name, bool& value)
	{
		settings->tryGetBoolValue(name, section, value);
	}
	void write(char const* name, int value)
	{
		settings->setKeyValue(name, section, value);
	}
	void read(char const* name, int& value)
	{
		settings->tryGetIntValue(name, section, value);
	}
	void write(char const* name, float value)
	{
		settings->setKeyValue(name, section, value);
	}
	void read(char const* name, float& value)
	{
		settings->tryGetFloatValue(name, section, value);
	}
	void write(char const* name, ImVec2 const& value)
	{
		settings->setKeyValue(name, section, InlineString<>::Make("(%g, %g)", value.x, value.y).c_str());
	}
	void read(char const* name, ImVec2& value)
	{
		char const* text;
		if (settings->tryGetStringValue(name, section, text))
		{
			sscanf(text, "(%f, %f)", &value.x, &value.y);
		}
	}
	void write(char const* name, ImVec4 const& value)
	{
		settings->setKeyValue(name, section, InlineString<>::Make("(%g, %g, %g, %g)", value.x, value.y, value.z, value.w).c_str());
	}
	void read(char const* name, ImVec4& value)
	{
		char const* text;
		if (settings->tryGetStringValue(name, section, text))
		{
			sscanf(text, "(%f, %f, %f, %f)", &value.x, &value.y, &value.z, &value.w);
		}
	}
	char const* section;
	PropertySet* settings;
};

#define IMGUI_STYLE_LIST(op)\
	op(Alpha)\
	op(DisabledAlpha)\
	op(Alpha)\
	op(DisabledAlpha)\
	op(WindowPadding)\
	op(WindowRounding)\
	op(WindowBorderSize)\
	op(WindowMinSize)\
	op(WindowTitleAlign)\
	op(WindowMenuButtonPosition)\
	op(ChildRounding)\
	op(ChildBorderSize)\
	op(PopupRounding)\
	op(PopupBorderSize)\
	op(FramePadding)\
	op(FrameRounding)\
	op(FrameBorderSize)\
	op(ItemSpacing)\
	op(ItemInnerSpacing)\
	op(CellPadding)\
	op(TouchExtraPadding)\
	op(IndentSpacing)\
	op(ColumnsMinSpacing)\
	op(ScrollbarSize)\
	op(ScrollbarRounding)\
	op(GrabMinSize)\
	op(GrabRounding)\
	/*op(LayoutAlign)*/\
	op(LogSliderDeadzone)\
	op(TabRounding)\
	op(TabBorderSize)\
	op(TabMinWidthForCloseButton)\
	op(ColorButtonPosition)\
	op(ButtonTextAlign)\
	op(SelectableTextAlign)\
	op(DisplayWindowPadding)\
	op(DisplaySafeAreaPadding)\
	op(MouseCursorScale)\
	op(AntiAliasedLines)\
	op(AntiAliasedLinesUseTex)\
	op(AntiAliasedFill)\
	op(CurveTessellationTol)\
	op(CircleTessellationMaxError)\

#define IMGUI_STYLE_COLOR_LIST(op)\
	op(ImGuiCol_Text)\
	op(ImGuiCol_TextDisabled)\
	op(ImGuiCol_WindowBg)\
	op(ImGuiCol_ChildBg)\
	op(ImGuiCol_PopupBg)\
	op(ImGuiCol_Border)\
	op(ImGuiCol_BorderShadow)\
	op(ImGuiCol_FrameBg)\
	op(ImGuiCol_FrameBgHovered)\
	op(ImGuiCol_FrameBgActive)\
	op(ImGuiCol_TitleBg)\
	op(ImGuiCol_TitleBgActive)\
	op(ImGuiCol_TitleBgCollapsed)\
	op(ImGuiCol_MenuBarBg)\
	op(ImGuiCol_ScrollbarBg)\
	op(ImGuiCol_ScrollbarGrab)\
	op(ImGuiCol_ScrollbarGrabHovered)\
	op(ImGuiCol_ScrollbarGrabActive)\
	op(ImGuiCol_CheckMark)\
	op(ImGuiCol_SliderGrab)\
	op(ImGuiCol_SliderGrabActive)\
	op(ImGuiCol_Button)\
	op(ImGuiCol_ButtonHovered)\
	op(ImGuiCol_ButtonActive)\
	op(ImGuiCol_Header)\
	op(ImGuiCol_HeaderHovered)\
	op(ImGuiCol_HeaderActive)\
	op(ImGuiCol_Separator)\
	op(ImGuiCol_SeparatorHovered)\
	op(ImGuiCol_SeparatorActive)\
	op(ImGuiCol_ResizeGrip)\
	op(ImGuiCol_ResizeGripHovered)\
	op(ImGuiCol_ResizeGripActive)\
	op(ImGuiCol_Tab)\
	op(ImGuiCol_TabHovered)\
	op(ImGuiCol_TabActive)\
	op(ImGuiCol_TabUnfocused)\
	op(ImGuiCol_TabUnfocusedActive)\
	op(ImGuiCol_DockingPreview)\
	op(ImGuiCol_DockingEmptyBg)\
	op(ImGuiCol_PlotLines)\
	op(ImGuiCol_PlotLinesHovered)\
	op(ImGuiCol_PlotHistogram)\
	op(ImGuiCol_PlotHistogramHovered)\
	op(ImGuiCol_TableHeaderBg)\
	op(ImGuiCol_TableBorderStrong)\
	op(ImGuiCol_TableBorderLight)\
	op(ImGuiCol_TableRowBg)\
	op(ImGuiCol_TableRowBgAlt)\
	op(ImGuiCol_TextSelectedBg)\
	op(ImGuiCol_DragDropTarget)\
	op(ImGuiCol_NavHighlight)\
	op(ImGuiCol_NavWindowingHighlight)\
	op(ImGuiCol_NavWindowingDimBg)\
	op(ImGuiCol_ModalWindowDimBg)\



void Editor::exprotStyle()
{
	ImGuiStyle& style = ImGui::GetStyle();
	FImGuiSettingHelper helper;
	helper.settings = &mSetttings;

	helper.section = "Style";
#define WRITE_OP(NAME)\
	helper.write(#NAME, style.NAME);
	IMGUI_STYLE_LIST(WRITE_OP);
#undef WRITE_OP

	helper.section = "StyleColor";
#define WRITE_OP(NAME)\
	helper.write(#NAME + 9, style.Colors[NAME]);
	IMGUI_STYLE_COLOR_LIST(WRITE_OP);
#undef WRITE_OP
}

void Editor::importStyle()
{
	ImGuiStyle& style = ImGui::GetStyle();
	FImGuiSettingHelper helper;
	helper.settings = &mSetttings;

	helper.section = "Style";
#define READ_OP(NAME)\
	helper.read(#NAME, style.NAME);
	IMGUI_STYLE_LIST(READ_OP);
#undef READ_OP

	helper.section = "StyleColor";
#define READ_OP(NAME)\
	helper.read(#NAME + 9, style.Colors[NAME]);
	IMGUI_STYLE_COLOR_LIST(READ_OP);
#undef READ_OP
}

IEditor* IEditor::Create()
{
	Editor* editor = new Editor;
	if (!editor->initialize())
	{
		delete editor;
		return nullptr;
	}

	return editor;
}



RHIGraphics2D& EditorRenderGloabal::getGraphics()
{
	return *GEditor->mGraphics;
}

void EditorRenderGloabal::saveRenderTarget()
{
	GEditor->mRenderer->saveRenderTarget();
}

void EditorRenderGloabal::resetRenderTarget()
{
	GEditor->mRenderer->resetRenderTarget();
}

EditorRenderGloabal& EditorRenderGloabal::Get()
{
	static EditorRenderGloabal StaticInstance;
	return StaticInstance;
}