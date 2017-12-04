#ifndef PlatformSF_h__
#define PlatformSF_h__

#include "PlatformBase.h"

#if USE_SFML

#include <SFML/Window.hpp>

class GameWindowSFML
{
public:
	
	~GameWindowSFML();

	void  release(){ delete this; }
	Vec2i getSize();
	void  showCursor( bool bShow );
	void  close();
	void  setSystemListener( ISystemListener& listener ){ mListener = &listener; }
	void  procSystemMessage();

private:
	friend class PlatformSFML;
	friend class GLContextSFML;
	friend class PlatformWin;
	friend class RenderSystem;

	GameWindowSFML();
	bool create( char const* title , Vec2i const& size , int colorBit , bool bFullScreen );

	ISystemListener* mListener;
	unsigned mMouseState;
	sf::RenderWindow mImpl;
};


class GLContextSFML
{
public:
	bool init( GameWindowSFML& window , GLConfig& config );
	void release();
	bool setCurrent();
	void swapBuffers();

private:

public:
	sf::RenderWindow* mWindow;

};

typedef GameWindowSFML PlatformWindow;
typedef GLContextSFML  PlatformGLContext;

class PlatformSFML
{
public:
	static int64           GetTickCount();
	static bool            IsKeyPressed( unsigned key );
	static bool            IsButtonPressed( unsigned button );

	static PlatformWindow*      CreateWindow( char const* title , Vec2i const& size , int colorBit , bool bFullScreen );
	static PlatformGLContext*   CreateGLContext( PlatformWindow& window , GLConfig& config );
};

#endif //USE_SFML


#endif // PlatformSF_h__
