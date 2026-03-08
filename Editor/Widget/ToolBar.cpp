#include "ToolBar.h"
#include "ImGui/imgui.h"
#include "ImGui/imgui_internal.h"

REGISTER_EDITOR_PANEL(ToolBar, ToolBar::ClassName, false, false);

void ToolBar::render()
{
	if (ImGui::IsWindowDocked())
	{
		ImGuiDockNode* node = ImGui::GetWindowDockNode();
		if (node)
		{
			node->LocalFlags |= ImGuiDockNodeFlags_NoResizeY | ImGuiDockNodeFlags_NoResizeX;
		}
	}

	for (int i = 0; i < mTools.size(); ++i)
	{
		auto const& tool = mTools[i];
		switch (tool.type)
		{
		case ToolInfo::Button:
			if (ImGui::Button(tool.label.c_str()))
			{
				if (tool.onClick)
					tool.onClick();
			}
			break;
		case ToolInfo::Separator:
			ImGui::SeparatorEx(ImGuiSeparatorFlags_Vertical);
			break;
		}

		if (i < mTools.size() - 1)
			ImGui::SameLine();
	}
}

void ToolBar::postRender()
{
	if (ImGui::IsWindowDocked())
	{
		ImGuiDockNode* node = ImGui::GetWindowDockNode();
		if (node)
		{
			// 鎖定寬度與高度調整
			node->LocalFlags |= ImGuiDockNodeFlags_NoResizeY | ImGuiDockNodeFlags_NoResizeX;

			// 計算內容高度
			float contentHeight = ImGui::GetCursorPosY() + ImGui::GetStyle().WindowPadding.y;
			// 計算內容寬度 (最後一個 Item 的右邊界 - 視窗左邊界 + 邊距)
			float contentWidth = ImGui::GetItemRectMax().x - ImGui::GetWindowPos().x + ImGui::GetStyle().WindowPadding.x;

			// 計算裝飾高度 (TabBar 等)
			float decorationHeight = node->Size.y - ImGui::GetWindowHeight();

			node->Size.y = node->SizeRef.y = contentHeight + decorationHeight;
			node->Size.x = node->SizeRef.x = contentWidth;
		}
	}
	ImGui::PopStyleVar(2); // Pop WindowPadding and ItemSpacing
}
