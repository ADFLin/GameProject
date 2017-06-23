#ifndef GLZumaGame_h__
#define GLZumaGame_h__

#include "ZBase.h"
#include "WinGLPlatform.h"

#include "ZumaGame.h"
#include "GLRenderSystem.h"
#include "ZParticle.h"
#include "WindowsMessageHander.h"

namespace Zuma
{
	class ZFont;

}//namespace Zuma

using namespace Zuma;

class GLZumaGame : public ZumaGame 
	             , public WinGLFrameT< GLZumaGame >
{
public:

	GLZumaGame();

	bool onEvent( int evtID , ZEventData const& data , ZEventHandler* child );
	bool onUIEvent( int evtID , int uiID , ZWidget* ui );

	virtual IRenderSystem* createRenderSystem();
	using   WinGLFrame::setupWindow;
	virtual bool setupWindow( char const* title , int w , int h );
};


#endif // GLZumaGame_h__
