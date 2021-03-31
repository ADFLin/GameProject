#ifndef GameInterface_h__
#define GameInterface_h__

#include "Base.h"

class QWidget;
class GameStage;
class RenderEngine;
class SoundManager;
class TextureManager;
class IFont;

class RHIGraphics2D;

class IGame
{
public:
	virtual ~IGame();

	static IGame& Get();

	RenderEngine*   getRenderEenine(){ return mRenderEngine; }
	SoundManager*   getSoundMgr(){ return mSoundMgr; }

	Vec2i const&  getScreenSize(){ return mScreenSize; }
	Vec2i const&  getMousePos(){ return mMousePos; }


	RHIGraphics2D&  getGraphics2D() { return *mGraphics2D; }

	
	virtual void  addStage( GameStage* stage, bool removePrev ) = 0;
	virtual void  stopPlay() = 0;


	virtual void  procWidgetEvent( int event , int id , QWidget* sender ) = 0;
	virtual void  procSystemEvent() = 0;

	virtual IFont* getFont( int idx ) = 0;

	RHIGraphics2D* mGraphics2D;
protected:
	Vec2i           mScreenSize;
	Vec2i           mMousePos;

	FPtr< RenderEngine >  mRenderEngine;
	FPtr< SoundManager >  mSoundMgr;
	
};

IGame* getGame();

#endif // GameInterface_h__
