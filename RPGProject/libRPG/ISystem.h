#ifndef ISystem_h__
#define ISystem_h__

#include "WindowsHeader.h"

class ISystem;
class IGameMod;
class GameFramework;
class RenderSystem;
class ScriptSystem;
class EntitySystem;
class IOSPlatform;

struct SGlobalEnv
{
	ISystem*          system;
	IGameMod*         gameMod;
	GameFramework*    framework;
	EntitySystem*     entitySystem;
	RenderSystem*     renderSystem;
	ScriptSystem*     scriptSystem;
	IOSPlatform*      OSPlatform;

	SGlobalEnv()
	{
		system        = NULL;
		gameMod       = NULL;
		framework     = NULL;
		entitySystem  = NULL;
		renderSystem  = NULL;
		scriptSystem  = NULL;
		OSPlatform    = NULL;
	}
};

extern SGlobalEnv* gEnv;

enum PlatformID
{
	PSI_WIN_XP ,
	PSI_WIN_7  ,
	PSI_WIN_VISTA ,
	PSI_WIN_OTHER ,
	PSI_LINUX ,
	PSI_OSX   ,

	PSI_XOBX  ,
	PSI_PS3   ,
};

class IOSPlatform
{
public:
	virtual PlatformID getPlatformID() = 0;
};

struct SSystemInitParams
{
	HINSTANCE    hInstance;
	HWND         hWnd;
	int          bufferWidth;
	int          bufferHeight;
	IOSPlatform* platform;

	bool         isDesigner;


	SSystemInitParams()
	{
		isDesigner = false;
		bufferWidth  = -1;
		bufferHeight = -1;
	}
};


class ISystem
{
public:
	ISystem(){ gEnv = &mEvt; }

	virtual bool  init( SSystemInitParams& params ) = 0;
	virtual void  release() = 0;

	EntitySystem*  getEntitySystem(){ return mEvt.entitySystem;  }
	ScriptSystem*  getScriptSystem(){ return mEvt.scriptSystem;  }
protected:
	SGlobalEnv mEvt;
};




#endif // ISystem_h__
