#include "Editor.h"

#define IMGUI_DEFINE_MATH_OPERATORS
#include "ImGui/imgui.h"
#include "ImGui/imgui_internal.h"

#include "EditorPanel.h"
#include "EditorRender.h"
#include "EditorUtils.h"

#include "Widget/GameViewportPanel.h"
#include "Widget/DetailViewPanel.h"
#include "Widget/TextureViewerPanel.h"

#include "LogSystem.h"
#include "InlineString.h"
#include "PropertySet.h"
#include "Renderer/SceneDebug.h"

#define EDITOR_INI "Editor.ini"


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

		mRenderer.reset( IEditorRenderer::Create() );
		if (mRenderer == nullptr)
		{
			return false;
		}
		
		VERIFY_RETURN_FALSE(mRenderer->initialize(mMainWindow));

		mGraphics = new RHIGraphics2D;
		mGraphics->initializeRHI();
		FImGui::InitializeRHI();

		importStyle();

		for (auto& info : EditorPanelInfo::GetList())
		{
			if (info.desc.bAlwaysCreation)
			{
				ActivePanel& panel = findOrAddPanel(info);
				panel.bOpenRequest = true;
			}
		}

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

	bool bAdvanceFrame = false;

	void update() override
	{
		mRenderer->beginFrame();
		ImGui::NewFrame();
		EditorRenderGloabal::Get().beginFrame();
		bAdvanceFrame = true;

		if (bPanelInitialized == false)
		{
			bPanelInitialized = true;

			for (auto& info : EditorPanelInfo::GetList())
			{
				if (info.desc.bAlwaysCreation)
				{
					//ActivePanel& panel = findOrAddPanel(info);
					//panel.bOpenRequest = true;
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
			if (panel.bOpened == false && panel.bOpenRequest == false)
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
	}

	bool bPanelInitialized = false;


	void render() override
	{
		if (!bAdvanceFrame)
			return;

		// End of frame: render Dear ImGui
		ImGui::Render();
		mRenderer->renderWindow(mMainWindow);

		ImGui::UpdatePlatformWindows();
		ImGui::RenderPlatformWindowsDefault();
	}

	void addGameViewport(IEditorGameViewport* viewport) override
	{
		ActivePanel* newPanel = createNamedPanel(GameViewportPanel::ClassName);
		if (newPanel)
		{
			static_cast<GameViewportPanel*>(newPanel->widget)->mViewport = viewport;
			mViewportPanels.push_back(newPanel->widget);
		}
	}


	TArray< IEditorPanel* > mViewportPanels;

	Render::ITextureShowManager* mTextureShowManager = nullptr;

	void setTextureShowManager(Render::ITextureShowManager* manager) override
	{
		ActivePanel* panel = findNamedPanel(TextureViewerPanel::ClassName);
		if (panel)
		{
			static_cast<TextureViewerPanel*>(panel->widget)->mTextureShowManager = manager;
		}
	}

	bool bShowEditorPreferences = true;
	bool bShowDemoWindow = true;

	void renderMainMenu()
	{

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
				for (auto& info : EditorPanelInfo::GetList())
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
				if (ImGui::MenuItem("Show ImGui Demo", NULL, &bShowDemoWindow))
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
					exportStyle();
					saveSettings();
				}
			}
			ImGui::End();
		}

		if (bShowDemoWindow)
		{
			ImGui::ShowDemoWindow(&bShowDemoWindow);
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
			return panel.widget == widget;
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
	void exportStyle();

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
			bool bDestroyPanel = false;
			void release() override
			{
				if (bDestroyPanel)
				{
					GEditor->destroyPanel(panel);
				}
				else
				{
					panel->clearAllViews();
				}
				delete this;
			}

			PropertyViewHandle addView(Reflection::EPropertyType type, void* ptr, char const* name)
			{
				return panel->addView(type, ptr, name);
			}
			PropertyViewHandle addView(Reflection::StructType* structData, void* ptr, char const* name)
			{
				return panel->addView(structData, ptr, name);
			}
			PropertyViewHandle addView(Reflection::EnumType* enumData, void* ptr, char const* name)
			{
				return panel->addView(enumData, ptr, name);
			}
			PropertyViewHandle addView(Reflection::PropertyBase* property, void* ptr, char const* name)
			{
				return panel->addView(property, ptr, name);
			}
			void addCallback(PropertyViewHandle handle, std::function<void(char const*)> const& callback)
			{
				panel->addCallback(handle, callback);
			}
			void removeView(PropertyViewHandle handle)
			{
				panel->removeView(handle);
			}
			void clearAllViews()
			{
				panel->clearAllViews();
			}
		};

		ActivePanel* panel = findOrCreateNamedPanel(DetailViewPanel::ClassName, config.name);
		if (panel)
		{
			EditorDetailView* detailView = new EditorDetailView;
			detailView->panel = (DetailViewPanel*)panel->widget;
			detailView->panel->clearAllViews();
			detailView->bDestroyPanel = panel->name != DetailViewPanel::ClassName;

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



void Editor::exportStyle()
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

EditorRenderGloabal& EditorRenderGloabal::Get()
{
	static EditorRenderGloabal StaticInstance;
	return StaticInstance;
}