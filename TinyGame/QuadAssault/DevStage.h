#ifndef DevStage_h__
#define DevStage_h__

#include "GameStage.h"
#include "RenderSystem.h"


class TestBase
{
public:
	virtual ~TestBase(){}
	virtual bool onInit() = 0;
	virtual void setDevMsg( FString& str ){}
	virtual void onUpdate( float dt ){}
	virtual void onRender() = 0;
	virtual MsgReply onMouse( MouseMsg const& msg ){ return MsgReply::Unhandled(); }
};

class DevStage : public GameStage
{
	using BaseClass = GameStage;
public:
	DevStage();
	~DevStage();

	virtual bool onInit();
	virtual void onExit();
	virtual void onUpdate( float deltaT );
	virtual void onRender();
	virtual MsgReply onKey(KeyMsg const& msg);
	virtual MsgReply onMouse(MouseMsg const& msg);

	FPtr< TestBase > mTest;
	Texture*  mTexCursor;
	FObjectPtr< IText > mDevMsg;
};

#endif // DevStage_h__
