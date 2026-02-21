#include "GameViewportPanel.h"

#include "ImGui/imgui_internal.h"

#include "Editor.h"

#include "RHI/D3D11Command.h"
#include "RHI/RHICommand.h"

#include "EditorUtils.h"
#include "ProfileSystem.h"
#include "BitmapDC.h"

using namespace Render;

//REGISTER_EDITOR_PANEL(GameViewportPanel, GameViewportPanel::ClassName, false, false);

class EditorRenderContext final : public IEditorRenderContext
{
public:
	void copyToRenderTarget(BitmapDC& bufferDC) override
	{
#if 0
		if (GRHISystem->getName() == RHISystemName::D3D11)
		{
			static_cast<D3D11Texture2D*>(texture)->bitbltFromDevice(bufferDC);
		}
		else if (GRHISystem->getName() == RHISystemName::OpenGL)
#endif
		{
			TArray<uint8> pixels;
			pixels.resize(4 * texture->getSizeX() * texture->getSizeY());
			bufferDC.readPixels(pixels.data());
			RHIUpdateTexture(*texture, 0, 0, texture->getSizeX(), texture->getSizeY(), pixels.data());
		}
	}
	void setRenderTexture(Render::RHITexture2D& inTexture) override
	{
		texture = &inTexture;
	}
	RHITexture2D* texture;
	GameViewportPanel* mPanel;
};

void GameViewportPanel::onOpen()
{
	ImGuiWindow* window = ImGui::GetCurrentWindowRead();

	TVector2<int> size = mViewport->getInitialSize();
	mTexture = RHICreateTexture2D(TextureDesc::Type2D(ETexture::BGRA8, size.x, size.y).AddFlags(TCF_PlatformGraphicsCompatible));
}

void GameViewportPanel::onClose()
{

}

void GameViewportPanel::render()
{
	PROFILE_ENTRY("GameViewportPanel");

	EditorRenderContext context;
	context.texture = mTexture;
	context.mPanel = this;
	mViewport->renderViewport(context);

	FImGui::DisableBlend();
	ImGui::InvisibleButton("Viewport", ImVec2(context.texture->getSizeX(), context.texture->getSizeY()));
	ImGui::SetItemAllowOverlap();
	ImGui::SetCursorPos(ImGui::GetItemRectMin() - ImGui::GetWindowPos() - ImVec2(0, ImGui::GetScrollY())); 
	ImGui::Image(FImGui::GetTextureID(*context.texture), ImVec2(context.texture->getSizeX(), context.texture->getSizeY()));
	//ImGui::ImageButton(FImGui::GetTextureID(*context.texture), ImVec2(context.texture->getSizeX(), context.texture->getSizeY()));
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

	if (ImGui::IsWindowFocused())
	{
		for (int i = 0; i < 512; i++)
		{
			if (ImGui::IsKeyPressed((ImGuiKey)i, false))
			{
				mViewport->onViewportKeyEvent(i, true);
			}
			else if (ImGui::IsKeyReleased((ImGuiKey)i))
			{
				mViewport->onViewportKeyEvent(i, false);
			}
		}

		ImGuiIO& io = ImGui::GetIO();
		for (int i = 0; i < io.InputQueueCharacters.Size; i++)
		{
			mViewport->onViewportCharEvent(io.InputQueueCharacters[i]);
		}
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

