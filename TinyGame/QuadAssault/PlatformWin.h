#ifndef PlatformWin_h__
#define PlatformWin_h__

#include "PlatformBase.h"

#include "WindowsHeader.h"
#undef CreateWindow

class GameWindowWin;
class GLContextWindows;

#define USE_SFML_WINDOW  0

#if USE_SFML_WINDOW
#include "PlatformSFML.h"
#endif


class GameWindowWin
{
public:
	~GameWindowWin();
	void  release(){ delete this; }
	Vec2i getSize(){ return mSize; }
	
	void  showCursor( bool bShow );
	void  close();
	void  setSystemListener( ISystemListener& listener ){ mListener = &listener; }
	void  procSystemMessage();
	HWND  getSystemHandle(){ return mhWnd; }

private:

	friend class PlatformWin;
	GameWindowWin();
	
	bool create( char const* title , Vec2i const& size , int colorBit , bool bFullScreen );

	ISystemListener* mListener;

	unsigned mMouseState;

	static LRESULT CALLBACK DefaultProc( HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam );

	bool procMsg( UINT message, WPARAM wParam, LPARAM lParam );
	bool precMouseMsg( UINT message, WPARAM wParam, LPARAM lParam );

	
	HWND     mhWnd;
	Vec2i    mSize;
};

class GLContextWindows
{
public:
	GLContextWindows();
	~GLContextWindows();
	bool init( GameWindowWin& window , GLConfig& config );
	void release();
	bool setCurrent();
	void swapBuffers();

private:
	HDC   mhDC;
	HGLRC mhRC;
};

#if USE_SFML_WINDOW
typedef GameWindowSFML PlatformWindow;
typedef GLContextSFML  PlatformGLContext;
#else
typedef GameWindowWin     PlatformWindow;
typedef GLContextWindows  PlatformGLContext;
#endif

class PlatformWin
{
public:
	static int64           GetTickCount();
	static bool            IsKeyPressed( unsigned key );
	static bool            IsButtonPressed( unsigned button );

	static PlatformWindow*     CreateWindow( char const* title , Vec2i const& size , int colorBit , bool bFullScreen );
	static PlatformGLContext*  CreateGLContext( PlatformWindow& window , GLConfig& config );
};


#endif // PlatformWin_h__
