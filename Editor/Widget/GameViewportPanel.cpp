#include "GameViewportPanel.h"

#include "ImGui/imgui_internal.h"

#include "Editor.h"

#include "RHI/D3D11Command.h"
#include "RHI/RHICommand.h"

#include "EditorUtils.h"
#include "ProfileSystem.h"

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
	PROFILE_ENTRY("GameViewportPanel");

	EditorRenderContext context;
	context.texture = mTexture;
	mViewport->renderViewport(context);

	FImGui::DisableBlend();
	ImGui::ImageButton(FImGui::GetTextureID(*context.texture), ImVec2(context.texture->getSizeX(), context.texture->getSizeY()));
	FImGui::RestoreBlend();

	if (ImGui::IsItemHovered())
	{
		Vector2 mousePositionAbsolute = FImGuiConv::To(ImGui::GetMousePos());
		Vector2 screenPositionAbsolute = FImGuiConv::To(ImGui::GetItemRectMin());
		Vector2 mousePositionRelative = mousePositionAbsolute - screenPositionAbsolute;

		uint32 buttonEvent = MBS_MOVING;

		auto ProcButtonMsg = [&]( uint16 button , ImGuiMouseButton imbtn) -> bool
		{
			bool bWasButtonDown = mMouseState & button;
			if (ImGui::IsMouseDown(imbtn))
			{
				mMouseState |= button;
				if (!bWasButtonDown)
				{
					buttonEvent = button | MBS_DOWN;
					MouseMsg msg(mousePositionRelative, buttonEvent, mMouseState);
					mViewport->onViewportMouseEvent(msg);
					return true;
				}

			}
			else if (ImGui::IsMouseReleased(imbtn))
			{
				mMouseState &= ~button;
				if (bWasButtonDown)
				{
					buttonEvent = button;
					MouseMsg msg(mousePositionRelative, buttonEvent, mMouseState);
					mViewport->onViewportMouseEvent(msg);
					return true;
				}
			}
			else if (ImGui::IsMouseDoubleClicked(imbtn))
			{
				buttonEvent = button | MBS_DOUBLE_CLICK;
				MouseMsg msg(mousePositionRelative, buttonEvent, mMouseState);
				mViewport->onViewportMouseEvent(msg);
				return true;
			}

			return false;
		};


		bool haveEvent = ProcButtonMsg(MBS_LEFT, ImGuiMouseButton_Left);
		haveEvent |= ProcButtonMsg(MBS_RIGHT, ImGuiMouseButton_Right);
		haveEvent |= ProcButtonMsg(MBS_MIDDLE, ImGuiMouseButton_Middle);

		if ( !haveEvent && mLastMousePos != mousePositionRelative )
		{
			MouseMsg msg(mousePositionRelative, MBS_MOVING, mMouseState);
			mViewport->onViewportMouseEvent(msg);
		}

		mLastMousePos = mousePositionRelative;
	}
}

bool GameViewportPanel::preRender()
{
	ImVec2 view = ImGui::GetContentRegionAvail();

	if (view.x != mSize.x || view.y != mSize.y)
	{
		if (view.x == 0 || view.y == 0)
		{
			// The window is too small or collapsed.
			return false;
		}
		mSize = view;
		mViewport->resizeViewport(mSize.x, mSize.y);
		mTexture = RHICreateTexture2D(ETexture::BGRA8, mSize.x, mSize.y, 0, 1, TCF_DefalutValue | TCF_PlatformGraphicsCompatible);
		// The window state has been successfully changed.
		return true;
	}
	// The window state has not changed.
	return true;
}

