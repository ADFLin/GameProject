#pragma once

#ifndef EditorPanel_H_7C414633_0488_402F_BBBB_9EAC3396913E
#define EditorPanel_H_7C414633_0488_402F_BBBB_9EAC3396913E

#include "EditorConfig.h"

#include "Meta/IsBaseOf.h"
#include "Meta/EnableIf.h"

#include "ImGui/imgui.h"
#include <vector>

#include "CString.h"


struct WindowRenderParams 
{
	ImGuiWindowFlags flags = ImGuiWindowFlags_None;
	ImVec2 InitSize = ImVec2(200,200);
};

class IEditorPanel
{
public:
	virtual ~IEditorPanel(){}
	virtual void onOpen(){}
	virtual void onClose(){}
	virtual void render(){}

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
		:name(name)
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
		List.push_back(EditorPanelInfo(desc, MakeFactory<T>()));
	}

	static  EditorPanelInfo const* Find(char const* name)
	{
		for(auto const& info : List)
		{
			if (FCString::CompareIgnoreCase(info.desc.name, name) == 0)
				return &info;
		}
		return nullptr;
	}

	EDITOR_API static std::vector< EditorPanelInfo > List;

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
