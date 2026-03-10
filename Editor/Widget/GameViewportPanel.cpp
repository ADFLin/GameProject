#include "GameViewportPanel.h"

#include "ImGui/imgui_internal.h"

#include "Editor.h"

#include "RHI/D3D11Command.h"
#include "RHI/RHICommand.h"

#include "EditorUtils.h"
#include "ProfileSystem.h"
#include "BitmapDC.h"
#include "Renderer/RenderThread.h"

using namespace Render;

//REGISTER_EDITOR_PANEL(GameViewportPanel, GameViewportPanel::ClassName, false, false);



class EditorViewportUpdateContext final : public IEditorViewportUpdateContext
{
public:
	void setRenderTexture(Render::RHITexture2D& inTexture) override
	{
		texture = &inTexture;
	}

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
	RHITexture2D* texture;
	GameViewportPanel* mPanel;
};

class EditorViewportRenderContext final : public IEditorViewportRenderContext
{
public:
	RHITexture2D* texture;
	GameViewportPanel* mPanel;
};

void GameViewportPanel::onOpen()
{
	ImGuiWindow* window = ImGui::GetCurrentWindowRead();

	TVector2<int> size = mViewport->getInitialSize();
	mTexture = RHICreateTexture2D(TextureDesc::Type2D(ETexture::BGRA8, size.x, size.y).AddFlags(TCF_PlatformGraphicsCompatible));
	mTexture = RHICreateTexture2D(ETexture::BGRA8, size.x, size.y, 0, 1, TCF_DefalutValue | TCF_RenderTarget | TCF_PlatformGraphicsCompatible);
	mDepthTexture = RHICreateTexture2D(ETexture::D32FS8, size.x, size.y, 0, 1, TCF_DefalutValue | TCF_RenderTarget);
	mFrameBuffer = RHICreateFrameBuffer();
	mFrameBuffer->setTexture(0, *mTexture);
	mFrameBuffer->setDepth(*mDepthTexture);
}

void GameViewportPanel::onClose()
{

}

static unsigned ImGuiKeyToVK(ImGuiKey key)
{
	switch (key)
	{
	case ImGuiKey_Tab:           return VK_TAB;
	case ImGuiKey_LeftArrow:     return VK_LEFT;
	case ImGuiKey_RightArrow:    return VK_RIGHT;
	case ImGuiKey_UpArrow:       return VK_UP;
	case ImGuiKey_DownArrow:     return VK_DOWN;
	case ImGuiKey_PageUp:        return VK_PRIOR;
	case ImGuiKey_PageDown:      return VK_NEXT;
	case ImGuiKey_Home:          return VK_HOME;
	case ImGuiKey_End:           return VK_END;
	case ImGuiKey_Insert:        return VK_INSERT;
	case ImGuiKey_Delete:        return VK_DELETE;
	case ImGuiKey_Backspace:     return VK_BACK;
	case ImGuiKey_Space:         return VK_SPACE;
	case ImGuiKey_Enter:         return VK_RETURN;
	case ImGuiKey_Escape:        return VK_ESCAPE;
	case ImGuiKey_Apostrophe:    return VK_OEM_7;
	case ImGuiKey_Comma:         return VK_OEM_COMMA;
	case ImGuiKey_Minus:         return VK_OEM_MINUS;
	case ImGuiKey_Period:        return VK_OEM_PERIOD;
	case ImGuiKey_Slash:         return VK_OEM_2;
	case ImGuiKey_Semicolon:     return VK_OEM_1;
	case ImGuiKey_Equal:         return VK_OEM_PLUS;
	case ImGuiKey_LeftBracket:   return VK_OEM_4;
	case ImGuiKey_Backslash:     return VK_OEM_5;
	case ImGuiKey_RightBracket:  return VK_OEM_6;
	case ImGuiKey_GraveAccent:   return VK_OEM_3;
	case ImGuiKey_CapsLock:      return VK_CAPITAL;
	case ImGuiKey_ScrollLock:    return VK_SCROLL;
	case ImGuiKey_NumLock:       return VK_NUMLOCK;
	case ImGuiKey_PrintScreen:   return VK_SNAPSHOT;
	case ImGuiKey_Pause:         return VK_PAUSE;
	case ImGuiKey_Keypad0:       return VK_NUMPAD0;
	case ImGuiKey_Keypad1:       return VK_NUMPAD1;
	case ImGuiKey_Keypad2:       return VK_NUMPAD2;
	case ImGuiKey_Keypad3:       return VK_NUMPAD3;
	case ImGuiKey_Keypad4:       return VK_NUMPAD4;
	case ImGuiKey_Keypad5:       return VK_NUMPAD5;
	case ImGuiKey_Keypad6:       return VK_NUMPAD6;
	case ImGuiKey_Keypad7:       return VK_NUMPAD7;
	case ImGuiKey_Keypad8:       return VK_NUMPAD8;
	case ImGuiKey_Keypad9:       return VK_NUMPAD9;
	case ImGuiKey_KeypadDecimal: return VK_DECIMAL;
	case ImGuiKey_KeypadDivide:  return VK_DIVIDE;
	case ImGuiKey_KeypadMultiply:return VK_MULTIPLY;
	case ImGuiKey_KeypadSubtract:return VK_SUBTRACT;
	case ImGuiKey_KeypadAdd:     return VK_ADD;
	case ImGuiKey_LeftShift:     return VK_LSHIFT;
	case ImGuiKey_LeftCtrl:      return VK_LCONTROL;
	case ImGuiKey_LeftAlt:       return VK_LMENU;
	case ImGuiKey_LeftSuper:     return VK_LWIN;
	case ImGuiKey_RightShift:    return VK_RSHIFT;
	case ImGuiKey_RightCtrl:     return VK_RCONTROL;
	case ImGuiKey_RightAlt:      return VK_RMENU;
	case ImGuiKey_RightSuper:    return VK_RWIN;
	case ImGuiKey_Menu:          return VK_APPS;
	case ImGuiKey_0: return '0'; case ImGuiKey_1: return '1'; case ImGuiKey_2: return '2';
	case ImGuiKey_3: return '3'; case ImGuiKey_4: return '4'; case ImGuiKey_5: return '5';
	case ImGuiKey_6: return '6'; case ImGuiKey_7: return '7'; case ImGuiKey_8: return '8';
	case ImGuiKey_9: return '9';
	case ImGuiKey_A: return 'A'; case ImGuiKey_B: return 'B'; case ImGuiKey_C: return 'C';
	case ImGuiKey_D: return 'D'; case ImGuiKey_E: return 'E'; case ImGuiKey_F: return 'F';
	case ImGuiKey_G: return 'G'; case ImGuiKey_H: return 'H'; case ImGuiKey_I: return 'I';
	case ImGuiKey_J: return 'J'; case ImGuiKey_K: return 'K'; case ImGuiKey_L: return 'L';
	case ImGuiKey_M: return 'M'; case ImGuiKey_N: return 'N'; case ImGuiKey_O: return 'O';
	case ImGuiKey_P: return 'P'; case ImGuiKey_Q: return 'Q'; case ImGuiKey_R: return 'R';
	case ImGuiKey_S: return 'S'; case ImGuiKey_T: return 'T'; case ImGuiKey_U: return 'U';
	case ImGuiKey_V: return 'V'; case ImGuiKey_W: return 'W'; case ImGuiKey_X: return 'X';
	case ImGuiKey_Y: return 'Y'; case ImGuiKey_Z: return 'Z';
	case ImGuiKey_F1:  return VK_F1;  case ImGuiKey_F2:  return VK_F2;
	case ImGuiKey_F3:  return VK_F3;  case ImGuiKey_F4:  return VK_F4;
	case ImGuiKey_F5:  return VK_F5;  case ImGuiKey_F6:  return VK_F6;
	case ImGuiKey_F7:  return VK_F7;  case ImGuiKey_F8:  return VK_F8;
	case ImGuiKey_F9:  return VK_F9;  case ImGuiKey_F10: return VK_F10;
	case ImGuiKey_F11: return VK_F11; case ImGuiKey_F12: return VK_F12;
	default: return 0;
	}
}

void GameViewportPanel::update()
{
	PROFILE_ENTRY("GameViewportPanel");

	EditorViewportUpdateContext context;
	context.texture = mTexture;
	context.mPanel = this;
	mViewport->updateViewport(context);

	FImGui::DisableBlend();
	ImGui::InvisibleButton("Viewport", ImVec2(context.texture->getSizeX(), context.texture->getSizeY()));
	ImGui::SetItemAllowOverlap();
	ImGui::SetCursorPos(ImGui::GetItemRectMin() - ImGui::GetWindowPos() - ImVec2(0, ImGui::GetScrollY()));
	ImGui::Image(FImGui::GetTextureID(*context.texture), ImVec2(context.texture->getSizeX(), context.texture->getSizeY()));
	
	if (context.texture == mTexture)
	{
		bRenderRequested = true;
	}
	else
	{
		bRenderRequested = false;
	}

	FImGui::RestoreBlend();

	ImRect viewportRect(ImGui::GetItemRectMin(), ImGui::GetItemRectMax());
	bool bIsHovered = ImGui::IsItemHovered();
	bool bIsActive = ImGui::IsItemActive();



	ImGuiContext& g = *GImGui;
	for (int i = 0; i < g.InputEventsTrail.Size; ++i)
	{
		ImGuiInputEvent& e = g.InputEventsTrail[i];
		switch (e.Type)
		{
		case ImGuiInputEventType_MousePos:
		{
			Vector2 mousePos(e.MousePos.PosX, e.MousePos.PosY);
			Vector2 relPos = mousePos - FImGuiConv::To(viewportRect.Min);
			if (viewportRect.Contains(FImGuiConv::To(mousePos)) || bIsActive)
			{
				mLastMousePos = relPos;
				MouseMsg msg(relPos, MBS_MOVING, mMouseState);
				mViewport->onViewportMouseEvent(msg);
			}
		}
		break;
		case ImGuiInputEventType_MouseButton:
		{
			uint16 buttonBit = 0;
			if (e.MouseButton.Button == ImGuiMouseButton_Left) buttonBit = MBS_LEFT;
			else if (e.MouseButton.Button == ImGuiMouseButton_Right) buttonBit = MBS_RIGHT;
			else if (e.MouseButton.Button == ImGuiMouseButton_Middle) buttonBit = MBS_MIDDLE;

			if (buttonBit != 0)
			{
				if (e.MouseButton.Down) mMouseState |= buttonBit;
				else mMouseState &= ~buttonBit;

				if (bIsHovered || bIsActive)
				{
					uint16 eventFlag = buttonBit;
					if (e.MouseButton.Down)
					{
						eventFlag |= MBS_DOWN;
						if (ImGui::IsMouseDoubleClicked(e.MouseButton.Button))
							eventFlag |= MBS_DOUBLE_CLICK;
					}
					MouseMsg msg(mLastMousePos, eventFlag, mMouseState);
					mViewport->onViewportMouseEvent(msg);
				}
			}
		}
		break;
		case ImGuiInputEventType_MouseWheel:
		{
			if (bIsHovered)
			{
				uint16 eventFlag = MBS_WHEEL;
				if (e.MouseWheel.WheelY < 0) eventFlag |= MBS_DOWN;
				MouseMsg msg(mLastMousePos, eventFlag, mMouseState);
				mViewport->onViewportMouseEvent(msg);
			}
		}
		break;
		case ImGuiInputEventType_Key:
		{
			if (ImGui::IsWindowFocused())
			{
				unsigned vk = ImGuiKeyToVK(e.Key.Key);
				if (vk != 0)
				{
					mViewport->onViewportKeyEvent(vk, e.Key.Down);
				}
			}
		}
		break;
		case ImGuiInputEventType_Text:
		{
			if (ImGui::IsWindowFocused())
			{
				mViewport->onViewportCharEvent(e.Text.Char);
			}
		}
		break;
		}
	}
}

void GameViewportPanel::render()
{
	if (!bRenderRequested)
		return;

	RenderThread::AddCommand("ViewportRender", [this]()
	{
		EditorViewportRenderContext context;
		context.texture = mTexture;
		context.frameBuffer = mFrameBuffer;
		context.graphics = &EditorRenderGloabal::Get().getGraphics();
		mViewport->renderViewport(context);
	});
}

bool GameViewportPanel::preUpdate()
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
		mTexture = RHICreateTexture2D(ETexture::BGRA8, mSize.x, mSize.y, 0, 1, TCF_DefalutValue | TCF_RenderTarget | TCF_PlatformGraphicsCompatible);
		mDepthTexture = RHICreateTexture2D(ETexture::D32FS8, mSize.x, mSize.y, 0, 1, TCF_DefalutValue | TCF_RenderTarget);
		mFrameBuffer->setTexture(0, *mTexture);
		mFrameBuffer->setDepth(*mDepthTexture);
		// The window state has been successfully changed.
		return true;
	}
	// The window state has not changed.
	return true;
}
