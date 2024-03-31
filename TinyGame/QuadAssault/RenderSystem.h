#ifndef RenderSystem_h__
#define RenderSystem_h__

#include "Base.h"
#include "Platform.h"

#include "Math/TVector2.h"

class TextureManager;
class RHIGraphics2D;

class IFont
{
public:
	virtual void release() = 0;
public:
	static IFont* LoadFile( char const* path );
};


class IText
{
public:
	virtual Vec2f getBoundSize() const = 0;
	virtual void  setString( char const* str ) = 0;
	virtual void  setColor( Color4ub const& color ) = 0;
	virtual void  setFont( IFont* font ) = 0;
	virtual void  setCharSize( int size ) = 0;
	virtual void  release() = 0;
public:
	static IText* Create( IFont* font , int size , Color4ub const& color );
	static IText* Create();
};

enum
{
	TEXT_SIDE_LEFT   = BIT(0),
	TEXT_SIDE_RIGHT  = BIT(1),
	TEXT_SIDE_TOP    = BIT(2),
	TEXT_SIDE_BOTTOM = BIT(3),
};

class ITextRenderer
{
public:
	virtual ~ITextRenderer(){}
	virtual void draw(IText* text, Vec2f const& pos, unsigned sideFlag ) = 0;
	static ITextRenderer& Get();
};


class RenderSystem
{
public:
	RenderSystem();
	~RenderSystem();

	bool init();
	void cleanup();
	bool prevRender();
	void postRender();

	TextureManager* getTextureMgr(){ return mTextureMgr; }
	void drawText(IText* text, Vec2f const& pos, unsigned sideFlag = 0 );
	void drawText(IFont* font, int fontSize, char const* str, Vec2f const& pos, Color4ub const& color, unsigned sideFlag = 0);

	RHIGraphics2D*  mGraphics2D = nullptr;
private:

	TextureManager* mTextureMgr;
	PlatformWindow*   mRenderWindow;
	PlatformGLContext*  mContext;
};

RenderSystem* getRenderSystem();

#endif // RenderSystem_h__
