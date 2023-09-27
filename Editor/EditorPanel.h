#pragma once

#ifndef EditorPanel_H_7C414633_0488_402F_BBBB_9EAC3396913E
#define EditorPanel_H_7C414633_0488_402F_BBBB_9EAC3396913E

#include "EditorConfig.h"

#define IMGUI_DEFINE_MATH_OPERATORS
#include "ImGui/imgui.h"

#include "Meta/IsBaseOf.h"
#include "Meta/EnableIf.h"
#include "DataStructure/Array.h"

#include "CString.h"
#include "RHI/RHIGraphics2D.h"
#include "Math/Vector2.h"

using ::Math::Vector2;

template< typename TFunc >
class TImGuiCustomRenderer
{
public:
	TImGuiCustomRenderer(TFunc&& func)
		:mFunc(std::forward<TFunc>(func))
	{

	}

	void render(const ImDrawList* parentlist, const ImDrawCmd* cmd)
	{
		mFunc(parentlist, cmd);
	}

	static void Entry(const ImDrawList* parentlist, const ImDrawCmd* cmd)
	{
		TImGuiCustomRenderer* renderer = (TImGuiCustomRenderer*)cmd->UserCallbackData;
		renderer->render(parentlist, cmd);
		renderer->~TImGuiCustomRenderer();
	}
	TFunc mFunc;
};

class EditorRenderGloabal
{
public:
	RHIGraphics2D& getGraphics();
	void   saveRenderTarget();
	void   resetRenderTarget();

	template< typename TFunc >
	void addCustomFunc(TFunc&& func)
	{
		void* ptr = mAllocator.alloc(sizeof(TImGuiCustomRenderer<TFunc>));
		new (ptr) TImGuiCustomRenderer<TFunc>(std::forward<TFunc>(func));

		ImGui::GetWindowDrawList()->AddCallback(TImGuiCustomRenderer<TFunc>::Entry, ptr);
		ImGui::GetWindowDrawList()->AddCallback(ImDrawCallback_ResetRenderState, nullptr);
	}

	void beginFrame()
	{
		mAllocator.clearFrame();
	}

	void endFrame()
	{


	}

	static EditorRenderGloabal& Get();

private:
	FrameAllocator mAllocator;

	EditorRenderGloabal()
		:mAllocator(2048)
	{
	}
};

struct WindowRenderParams 
{
	ImGuiWindowFlags flags = ImGuiWindowFlags_None;
	ImVec2 InitSize = ImVec2(200,200);
};

class IEditorPanel
{
public:
	virtual ~IEditorPanel() = default;
	virtual void onOpen(){}
	virtual void onClose(){}
	virtual void onShow(){}
	virtual void onHide(){}

	virtual bool preRender() { return true; }
	virtual void render(){}
	virtual void postRender(){}

	virtual void getRenderParams(WindowRenderParams& params) const {}
};


class IEditorPanelFactory
{
public:
	virtual ~IEditorPanelFactory(){}
	virtual IEditorPanel* create() = 0;
};


template< typename T >
class TEditorPanelFactory : public IEditorPanelFactory
{
public:
	virtual IEditorPanel* create() { return new T; }
};


struct EditorPanelDesc
{
	EditorPanelDesc(
		char const* name,
		bool bSingleton,
		bool bAlwaysCreation)
		: name(name)
		, bSingleton(bSingleton)
		, bAlwaysCreation(bAlwaysCreation)
	{

	}

	char const* name;
	bool bSingleton;
	bool bAlwaysCreation;
};

struct EditorPanelInfo
{
	EditorPanelInfo(EditorPanelDesc const& desc, IEditorPanelFactory* factory)
		:desc(desc)
		,factory(factory)
	{

	}

	template< typename T >
	static IEditorPanelFactory* MakeFactory()
	{
		static TEditorPanelFactory< T > StaticFactory;
		return &StaticFactory;
	}

	EditorPanelDesc desc;
	IEditorPanelFactory* factory;

	template< typename T >
	static void Register(EditorPanelDesc const& desc)
	{
		GetList().push_back(EditorPanelInfo(desc, MakeFactory<T>()));
	}

	static  EditorPanelInfo const* Find(char const* className)
	{
		for(auto const& info : GetList())
		{
			if ( info.desc.name == className ||
			     FCString::CompareIgnoreCase(info.desc.name, className) == 0)
				return &info;
		}
		return nullptr;
	}

	static EDITOR_API TArray< EditorPanelInfo >& GetList();
};

struct EditorPanelRegister
{
	template< typename T >
	EditorPanelRegister(T*, EditorPanelDesc const& desc)
	{
		EditorPanelInfo::Register<T>(desc);
	}
};

#define REGISTER_EDITOR_PANEL(CLASS , NAME, ...)\
	static EditorPanelRegister ANONYMOUS_VARIABLE(GEditorPanelRegister)((CLASS*)0, EditorPanelDesc(NAME, ##__VA_ARGS__) );


#endif // EditorPanel_H_7C414633_0488_402F_BBBB_9EAC3396913E
