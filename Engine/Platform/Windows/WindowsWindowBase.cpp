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

bool WindowsWindowBase::RegisterWindowClass(LPTSTR className, WNDPROC wndProc, DWORD wIcon, WORD wSIcon)
{
	WNDCLASSEX  wc;

	HINSTANCE hInstance = ::GetModuleHandle(NULL);

	// Create the window class for the main window
	wc.cbSize = sizeof(WNDCLASSEX);
	wc.style = CS_HREDRAW | CS_VREDRAW | CS_DBLCLKS | CS_OWNDC /*|CS_CLASSDC*/;
	wc.lpfnWndProc = wndProc;
	wc.cbClsExtra = 0;
	wc.cbWndExtra = 0;
	wc.hInstance = hInstance;
	wc.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(wIcon));
	wc.hIconSm = LoadIcon(hInstance, MAKEINTRESOURCE(wSIcon));
	wc.hCursor = LoadCursor(NULL, IDC_ARROW);
	wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
	wc.lpszMenuName = NULL;
	wc.lpszClassName = className;

	// Register the window class
	if (!RegisterClassEx(&wc))
		return false;
	return true;
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

bool WindowsWindowBase::isShow() const
{
	return ::IsWindowVisible(mhWnd);
}

void WindowsWindowBase::show(bool bShow)
{
	::ShowWindow(mhWnd, bShow ? SW_SHOW : SW_HIDE);
}

bool WindowsWindowBase::createWindow(TCHAR const* szTitle, LPTSTR className, DWORD style, DWORD exStyle)
{
	RECT rect;
	SetRect(&rect, 0, 0, mWidth, mHeight);
	AdjustWindowRectEx(&rect, style, FALSE, exStyle);

	mhWnd = CreateWindowEx(
		exStyle,
		className,
		szTitle,
		style,
		CW_USEDEFAULT, CW_USEDEFAULT,
		rect.right - rect.left,
		rect.bottom - rect.top,
		NULL,
		NULL,
		::GetModuleHandle(NULL),
		this
	);

	if (mhWnd == NULL)
		return false;

	mhDC = GetDC(mhWnd);
	return true;
}

