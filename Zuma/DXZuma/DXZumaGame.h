#include "ZBase.h"

#include "GameLoop.h"
#include "D3DWinPlatform.h"
#include "ZumaGame.h"
#include "SysMsgHandler.h"

#include "D3D9RenderSystem.h"

using namespace Zuma;

class MyFrame : public D3D9Frame< MyFrame >
{
public:

};

class DXZumaGame : public ZumaGame
{
public:

	virtual IRenderSystem* createRenderSystem();
	virtual bool setupWindow( char const* title , int w , int h  );

	bool onEvent( int evtID , ZEventData const& data , ZEventHandler* sender);

	MyFrame mFrame;

};