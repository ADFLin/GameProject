#include "WindowsWindowBase.h"

LRESULT CALLBACK WindowsWindowBase::DefaultProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)                  /* handle the messages */
	{
	case WM_DESTROY:
		::PostQuitMessage(0);       /* send a WM_QUIT to the message queue */
		break;
	}
	return ::DefWindowProc(hWnd, message, wParam, lParam);
}

bool WindowsWindowBase::setFullScreen(unsigned bits)
{
	DEVMODE dmScreenSettings;

	memset(&dmScreenSettings, 0, sizeof(dmScreenSettings));

	dmScreenSettings.dmSize = sizeof(dmScreenSettings);
	dmScreenSettings.dmPelsWidth = mWidth;
	dmScreenSettings.dmPelsHeight = mHeight;
	dmScreenSettings.dmBitsPerPel = bits;
	dmScreenSettings.dmFields = DM_BITSPERPEL | DM_PELSWIDTH | DM_PELSHEIGHT;

	if (ChangeDisplaySettings(&dmScreenSettings, CDS_FULLSCREEN) != DISP_CHANGE_SUCCESSFUL)
	{
		return false;
	}

	return true;
}

void WindowsWindowBase::destroyInternal()
{
	if (mbFullscreen)
	{
		::ChangeDisplaySettings(NULL, 0);
		mbFullscreen = false;
	}

	if (mhDC)
	{
		::ReleaseDC(mhWnd, mhDC);
		mhDC = NULL;
	}

	if (mhWnd)
	{
		HWND hWndToDestroy = mhWnd;
		mhWnd = NULL;
		::DestroyWindow(hWndToDestroy);
	}
}

TVector2<int> WindowsWindowBase::getPosition()
{
	RECT rect;
	GetWindowRect(mhWnd, &rect);
	return TVector2<int>(rect.left, rect.top);
}

void WindowsWindowBase::setPosition(TVector2<int> const& InPos)
{
	BOOL result = SetWindowPos(mhWnd, 0, InPos.x, InPos.y, 0, 0, SWP_NOSIZE | SWP_NOZORDER);
}

