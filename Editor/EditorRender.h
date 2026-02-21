#pragma once
#ifndef EditorRender_H_B319F6D8_D7E1_42A2_9589_68CE133B5A0C
#define EditorRender_H_B319F6D8_D7E1_42A2_9589_68CE133B5A0C

#include "WindowsMessageHandler.h"
#include "WindowsPlatform.h"

#include "ImGui/backends/imgui_impl_win32.h"
#include "ImGui/imgui.h"

#include <memory>


struct ImDrawDataSnapshot
{
	ImDrawData data;
	ImVector<ImDrawList*> cmdLists;

	ImDrawDataSnapshot(ImDrawData* src)
	{
		data = *src;
		cmdLists.resize(src->CmdListsCount);
		for (int i = 0; i < src->CmdListsCount; ++i)
		{
			cmdLists[i] = src->CmdLists[i]->CloneOutput();
		}
		data.CmdLists = cmdLists.Data;
	}

	~ImDrawDataSnapshot()
	{
		for (int i = 0; i < cmdLists.Size; i++)
		{
			IM_DELETE(cmdLists[i]);
		}
	}

	static ImDrawDataSnapshot* Copy(ImDrawData* src)
	{
		if (src == nullptr || !src->Valid)
			return nullptr;
		return new ImDrawDataSnapshot(src);
	}
};


IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
static class Editor* GEditor = nullptr;


class EditorWindow;
class IEditorRenderer
{
public:
	static IEditorRenderer* Create();
	~IEditorRenderer() = default;
	virtual bool initialize(EditorWindow& mainWindow) { return true; }
	virtual void shutdown() {}
	virtual void beginFrame() {}
	virtual void beginRender(EditorWindow& window) {}
	virtual void endFrame() {}

	virtual bool initializeWindowRenderData(EditorWindow& window) { return true; }
	virtual void renderWindow(EditorWindow& window, ImDrawData* drawData) {}
	virtual void notifyWindowResize(EditorWindow& window, int width, int height) {}

};

class IEditorWindowRenderData
{
public:
	virtual ~IEditorWindowRenderData() = default;
};

class EditorWindow : public WinFrameT< EditorWindow >
{
public:

	EditorWindow()
	{
		mGuiContext = ImGui::CreateContext();
	}

	~EditorWindow()
	{
		ImGui::DestroyContext(mGuiContext);
	}

	LPTSTR getWinClassName() { return TEXT("EditorWindow"); }
	DWORD  getWinStyle() { return WS_OVERLAPPEDWINDOW | WS_VISIBLE | WS_BORDER; }
	DWORD  getWinExtStyle() { return WS_EX_OVERLAPPEDWINDOW; }


	void beginRender()
	{
		ImGui::SetCurrentContext(mGuiContext);
	}
	void endRender()
	{

	}

	LRESULT processMessage(IEditorRenderer& renderer, HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
	{

		ImGui::SetCurrentContext(mGuiContext);

		if (ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam))
			return true;

		switch (msg)
		{
		case WM_SIZE:
			if (wParam != SIZE_MINIMIZED)
			{
				renderer.notifyWindowResize(*this, (UINT)LOWORD(lParam), (UINT)HIWORD(lParam));
			}
			break;
		case WM_SYSCOMMAND:
			if ((wParam & 0xfff0) == SC_KEYMENU)
				return 0;
			break;
		case WM_ENTERSIZEMOVE:
			bSizeMoved = true;
			break;

		case WM_EXITSIZEMOVE:
			bSizeMoved = false;
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

	template< typename RenderData >
	RenderData* getRenderData() { return static_cast<RenderData*>(mRenderData.get()); }

	std::unique_ptr< IEditorWindowRenderData > mRenderData;

	ImGuiContext* mGuiContext;

	bool bSizeMoved = false;

};


#endif // EditorRender_H_B319F6D8_D7E1_42A2_9589_68CE133B5A0C
