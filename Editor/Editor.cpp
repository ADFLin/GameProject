#include "Editor.h"


#include "WindowsMessageHandler.h"
#include "WindowsPlatform.h"
#include "Platform/Windows/ComUtility.h"
#include "LogSystem.h"

#include "ImGui/imgui.h"

#include <D3D11.h>
#include "ImGui/backends/imgui_impl_win32.h"
#include "ImGui/backends/imgui_impl_dx11.h"
#include "EditorPanel.h"

#include "RHI/RHICommand.h"
#include "RHI/D3D11Command.h"
#include "RHI/TextureAtlas.h"
#include "EditorUtils.h"
#include "PropertySet.h"

using namespace Render;

#pragma comment(lib , "D3D11.lib")
#pragma comment(lib , "DXGI.lib")
#pragma comment(lib , "dxguid.lib")

#define EDITOR_INI "Editor.ini"
class EditorWindow : public WinFrameT< EditorWindow >
{
public:
	LPTSTR getWinClassName() { return TEXT("EditorWindow"); }
	DWORD  getWinStyle() { return WS_OVERLAPPEDWINDOW | WS_VISIBLE; }
	DWORD  getWinExtStyle() { return WS_EX_OVERLAPPEDWINDOW; }

};

#define ERROR_MSG_GENERATE( HR , CODE , FILE , LINE )\
	LogWarning(1, "ErrorCode = 0x%x File = %s Line = %s %s ", HR , FILE, #LINE, #CODE)

#define VERIFY_D3D_RESULT_INNER( FILE , LINE , CODE ,ERRORCODE )\
	{ HRESULT hResult = CODE; if( FAILED(hResult) ){ ERROR_MSG_GENERATE( hResult , CODE, FILE, LINE ); ERRORCODE } }

#define VERIFY_D3D_RESULT( CODE , ERRORCODE ) VERIFY_D3D_RESULT_INNER( __FILE__ , __LINE__ , CODE , ERRORCODE )
#define VERIFY_D3D_RESULT_RETURN_FALSE( CODE ) VERIFY_D3D_RESULT_INNER( __FILE__ , __LINE__ , CODE , return false; )

IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
static class Editor* GEditor = nullptr;

class Editor : public IEditor
	         , public WindowsMessageHandlerT< Editor >
{
public:
	
	using WindowsMessageHandler = WindowsMessageHandlerT< Editor >;

	Editor()
	{
		GEditor = this;
	}

	~Editor()
	{
		ImGui_ImplDX11_Shutdown();
		ImGui_ImplWin32_Shutdown();
		ImGui::DestroyContext();
	}
	bool initialize()
	{
		mSetttings.loadFile(EDITOR_INI);

		for (auto& info : EditorPanelInfo::List)
		{
			if (info.desc.bAlwaysCreation)
			{
				ActivePanel& panel = findOrAddPanel(info);
				panel.bOpened = true;
			}
		}
		return true;
	}

	bool initializeRender()
	{
		if (!mWindow.create("Editor", 1600, 900, &WndProc) )
			return false;

#if 0
		uint32 deviceFlags = D3D11_CREATE_DEVICE_BGRA_SUPPORT;
		deviceFlags |= D3D11_CREATE_DEVICE_DEBUG;

		VERIFY_D3D_RESULT_RETURN_FALSE(D3D11CreateDevice(NULL, D3D_DRIVER_TYPE_HARDWARE, NULL, deviceFlags, NULL, 0, D3D11_SDK_VERSION, &mDevice, NULL, &mDeviceContext));
#else

		RHISystemInitParams initParam;
		initParam.numSamples = 1;
		initParam.bVSyncEnable = true;
		initParam.bDebugMode = true;
		initParam.hWnd = NULL;
		initParam.hDC = NULL;
		if (!RHISystemInitialize(RHISystemName::D3D11, initParam))
			return false;

		mDevice = static_cast<D3D11System*>(GRHISystem)->mDevice;
		mDeviceContext = static_cast<D3D11System*>(GRHISystem)->mDeviceContext;
#endif

		HRESULT hr;
		TComPtr<IDXGIFactory> factory;
		hr = CreateDXGIFactory(IID_PPV_ARGS(&factory));

		UINT quality = 0;
		DXGI_SWAP_CHAIN_DESC swapChainDesc; ZeroMemory(&swapChainDesc, sizeof(swapChainDesc));
		swapChainDesc.OutputWindow = mWindow.getHWnd();
		swapChainDesc.Windowed = true;
		swapChainDesc.SampleDesc.Count = 1;
		swapChainDesc.SampleDesc.Quality = quality;
		swapChainDesc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
		swapChainDesc.BufferDesc.Width = mWindow.getWidth();
		swapChainDesc.BufferDesc.Height = mWindow.getHeight();
		swapChainDesc.BufferCount = 2;
		swapChainDesc.BufferUsage = DXGI_USAGE_BACK_BUFFER | DXGI_USAGE_RENDER_TARGET_OUTPUT;
		swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;
		swapChainDesc.Flags = 0;

		VERIFY_D3D_RESULT_RETURN_FALSE(factory->CreateSwapChain(mDevice, &swapChainDesc, &mSwapChain));

		CreateRenderTarget();

		ImGui::CreateContext();
		importStyle();

		ImGuiIO& io = ImGui::GetIO();
		io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
		io.ConfigFlags |= ImGuiConfigFlags_NavEnableSetMousePos; // Enable some options
		io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;

		VERIFY_RETURN_FALSE(ImGui_ImplWin32_Init(mWindow.getHWnd()));
		VERIFY_RETURN_FALSE(ImGui_ImplDX11_Init(mDevice, mDeviceContext));

		FImGui::InitializeRHI();

		for (auto& panel : mPanels)
		{
			panel.widget->onOpen();
		}
		return true;
	}


	void release() override
	{
		FImGui::ReleaseRHI();

		if (GRHISystem)
		{
			RHISystemShutdown();
		}
		delete this;
	}


	bool handleWindowResize(int sizeX, int sizeY) 
	{
		if( mSwapChain )
		{
			CleanupRenderTarget();
			mSwapChain->ResizeBuffers(0, sizeX, sizeY, DXGI_FORMAT_UNKNOWN, 0);
			CreateRenderTarget();
		}
		return true; 
	}

	void CreateRenderTarget()
	{
		ID3D11Texture2D* pBackBuffer;
		mSwapChain->GetBuffer(0, IID_PPV_ARGS(&pBackBuffer));
		mDevice->CreateRenderTargetView(pBackBuffer, NULL, &mRenderTargetView);
		pBackBuffer->Release();

	}
	void CleanupRenderTarget()
	{
		mRenderTargetView.reset();
	}
	EditorWindow mWindow;

	TComPtr< ID3D11Device > mDevice;
	TComPtr< ID3D11DeviceContext > mDeviceContext;
	TComPtr<IDXGISwapChain> mSwapChain;
	TComPtr< ID3D11RenderTargetView > mRenderTargetView;
	void update() override
	{

	}

	void render() override
	{
		ImGui_ImplDX11_NewFrame();
		ImGui_ImplWin32_NewFrame();
		ImGui::NewFrame();

		ImGui::DockSpaceOverViewport(ImGui::GetMainViewport());

		renderMenu();

		for (auto& panel : mPanels)
		{
			if (!panel.bOpened)
				continue;
				
			panel.widget->render(panel.info->desc.name, &panel.bOpened);
		}

		// End of frame: render Dear ImGui
		ImGui::Render();
		const float clear_color_with_alpha[4] = { 0,0,0,1 };
		mDeviceContext->OMSetRenderTargets(1, &mRenderTargetView, NULL);
		mDeviceContext->ClearRenderTargetView(mRenderTargetView, clear_color_with_alpha);
		ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
		mSwapChain->Present(1, 0);
	}

	void renderMenu()
	{
		static bool bShowIconTexture = true;
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

			if (ImGui::BeginMenu("Window"))
			{
				for (auto& info : EditorPanelInfo::List)
				{
					if (ImGui::MenuItem(info.desc.name))
					{
						ActivePanel& panel = findOrAddPanel(info);
						if (!panel.bOpened)
						{
							panel.bOpened = true;
							panel.widget->onOpen();
						}
						else
						{

						}
					}
				}
				ImGui::EndMenu();
			}

			if (ImGui::BeginMenu("Debug"))
			{
				if (ImGui::MenuItem("Show Icon Texture", NULL , &bShowIconTexture))
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

		if (bShowIconTexture)
		{
			int width = FImGui::mIconAtlas.getTexture().getSizeX();
			int height = FImGui::mIconAtlas.getTexture().getSizeY();
			ImGui::SetNextWindowSize(ImVec2(width, height), ImGuiCond_FirstUseEver);
			if (ImGui::Begin("Icon Texture", &bShowIconTexture))
			{
				ImGui::Image(FImGui::IconTextureID(), ImVec2(width, height));
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
		EditorPanelInfo const* info;
		IEditorPanel* widget = nullptr;
		bool bOpened = true;
	};
	std::vector< ActivePanel > mPanels;
	ActivePanel& findOrAddPanel(EditorPanelInfo const& info)
	{
		for (auto& panel : mPanels)
		{
			if (panel.info == &info)
				return panel;
		}

		ActivePanel newPanel;
		newPanel.info = &info;
		newPanel.widget = info.factory->create();
		mPanels.push_back(newPanel);
		return mPanels.back();
	}


	bool bSizeMove = false;
	LRESULT processWindowMessage(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
	{
		if (ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam))
			return true;

		switch (msg)
		{
		case WM_SIZE:
			if (mSwapChain.isValid() && wParam != SIZE_MINIMIZED)
			{
				CleanupRenderTarget();
				mSwapChain->ResizeBuffers(0, (UINT)LOWORD(lParam), (UINT)HIWORD(lParam), DXGI_FORMAT_UNKNOWN, 0);
				CreateRenderTarget();
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

	PropertySet mSetttings;
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
		settings->setKeyValue(name, section, InlineString<>::Make("(%g, %g)", value.x, value.y));
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
		settings->setKeyValue(name, section, InlineString<>::Make("(%g, %g, %g, %g)", value.x, value.y, value.z, value.w));
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
	op(LayoutAlign)\
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