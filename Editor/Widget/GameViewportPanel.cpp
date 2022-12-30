#include "GameViewportPanel.h"

#include "ImGui/imgui_internal.h"

#include "Editor.h"

#include "RHI/D3D11Command.h"
#include "RHI/RHICommand.h"

#include "EditorUtils.h"

using namespace Render;

REGISTER_EDITOR_PANEL(GameViewportPanel, "GameViewport", false, false);

class EditorRenderContext final : public IEditorRenderContext
{
public:
	void copyToRenderTarget(void* SrcHandle) override
	{
		static_cast<D3D11Texture2D*>(texture)->bitbltFromDevice((HDC)SrcHandle);
	}
	void setRenderTexture(Render::RHITexture2D& inTexture) override
	{
		texture = &inTexture;
	}
	RHITexture2D* texture;
};

void GameViewportPanel::onOpen()
{
	ImGuiWindow* window = ImGui::GetCurrentWindowRead();
	window->ContentSize;
	mTexture = RHICreateTexture2D(ETexture::BGRA8, 800, 600, 0, 1, TCF_DefalutValue | TCF_PlatformGraphicsCompatible);
}

void GameViewportPanel::onClose()
{

}

void GameViewportPanel::render()
{
	EditorRenderContext context;
	context.texture = mTexture;
	mViewport->renderViewport(context);

	FImGui::DisableBlend();
	ImGui::ImageButton(FImGui::GetTextureID(*context.texture), ImVec2(context.texture->getSizeX(), context.texture->getSizeY()));
	FImGui::RestoreBlend();

	if (ImGui::IsItemHovered())
	{
		ImVec2 mousePositionAbsolute = ImGui::GetMousePos();
		ImVec2 screenPositionAbsolute = ImGui::GetItemRectMin();
		ImVec2 mousePositionRelative = ImVec2(mousePositionAbsolute.x - screenPositionAbsolute.x, mousePositionAbsolute.y - screenPositionAbsolute.y);

		uint32 buttonEvent = MBS_MOVING;

		auto ProcButtonMsg = [&]( uint16 button , ImGuiMouseButton imbtn) -> bool
		{
			if (ImGui::IsMouseDown(imbtn))
			{
				mMouseState |= button;
				buttonEvent = button | MBS_DOWN;
			}
			else if (ImGui::IsMouseReleased(imbtn))
			{
				mMouseState &= ~button;
				buttonEvent = button;
			}
			else if (ImGui::IsMouseDoubleClicked(imbtn))
			{
				buttonEvent = button | MBS_DOUBLE_CLICK;
			}
			else
			{
				return false;
			}
			return true;
		};


		if (ProcButtonMsg(MBS_LEFT, ImGuiMouseButton_Left) ||
			ProcButtonMsg(MBS_RIGHT, ImGuiMouseButton_Right) ||
			ProcButtonMsg(MBS_MIDDLE, ImGuiMouseButton_Middle))
		{


		}

		MouseMsg msg(Vec2i(mousePositionRelative.x, mousePositionRelative.y), buttonEvent, mMouseState);
		mViewport->onViewportMouseEvent(msg);
	}
}

