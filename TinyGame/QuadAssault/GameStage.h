#ifndef GameStage_h__
#define GameStage_h__

#include "Base.h"
#include "SystemMessage.h"

#include <functional>
class Game;
class QWidget;
class MouseMsg;
class KeyMsg;

class RHIGraphics2D;

enum StateTransition
{
	ST_NONE    ,
	ST_FADEIN  , 
	ST_FADEOUT ,
};

class SrceenFade
{
public:
	typedef std::function< void () > Callback;

	SrceenFade();

	void fadeIn( Callback const& cb = NULL ) {  mFunFisish = cb ; mState = eIN;  }
	void fadeOut( Callback const& cb = NULL ){  mFunFisish = cb ; mState = eOUT; }
	void setColor( float c ){ mColor = c; }
	void render(RHIGraphics2D& g);
	void update( float dt );

private:
	enum State
	{
		eIN ,
		eOUT ,
		eNONE ,
	};
	State    mState;
	float    mColor;
	float    mFadeSpeed;
	Callback mFunFisish;
};

class GameStage
{
public:		
	GameStage();
	virtual bool onInit() = 0;
	virtual void onExit() = 0;	
	virtual void onUpdate( float deltaT ) = 0;	
	virtual void onRender() = 0;

	virtual MsgReply onMouse( MouseMsg const& msg ){ return MsgReply::Unhandled(); }
	virtual MsgReply onKey(KeyMsg const& msg) { return MsgReply::Unhandled(); }
	virtual void onWidgetEvent( int event , int id , QWidget* sender ){}

	void  stop(){ mNeedStop = true; }
	bool  needStop(){ return mNeedStop; }

protected:
	bool  mNeedStop;	
};

#endif // GameStage_h__