#pragma once
#ifndef ToolBar_H_
#define ToolBar_H_

#include "EditorPanel.h"
#include <vector>
#include <string>
#include <functional>

class ToolBar : public IEditorPanel
{
public:
	static constexpr char const* ClassName = "ToolBar";

	void render() override;
	void getRenderParams(WindowRenderParams& params) const override
	{
		params.flags |= ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoTitleBar;
	}

	bool preRender() override
	{
		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(4, 4));
		ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(8, 2));
		return true;
	}

	void postRender() override;

	struct ToolInfo
	{
		enum Type
		{
			Button,
			Separator,
		};
		Type type;
		std::string label;
		std::function<void()> onClick;
	};

	std::vector<ToolInfo> mTools;

	void addButton(char const* label, std::function<void()> const& onClick)
	{
		ToolInfo info;
		info.type = ToolInfo::Button;
		info.label = label;
		info.onClick = onClick;
		mTools.push_back(std::move(info));
	}

	void addSeparator()
	{
		ToolInfo info;
		info.type = ToolInfo::Separator;
		mTools.push_back(std::move(info));
	}

	void clear()
	{
		mTools.clear();
	}
};

#endif
