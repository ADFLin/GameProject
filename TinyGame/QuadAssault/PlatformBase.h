#ifndef PlatformBase_h__
#define PlatformBase_h__

#include "Core/IntegerType.h"
#include "SystemMessage.h"

#include "Math/TVector2.h"
typedef TVector2< int > Vec2i;

struct  GLConfig
{
	int colorBits;
};

enum SystemEvent
{
	SYS_WINDOW_CLOSE ,
	SYS_QUIT ,
};

class ISystemListener
{
public:
	virtual bool onSystemEvent( SystemEvent event ){ return true; }
	virtual bool onMouse( MouseMsg const& msg ){ return true; }
	virtual bool onKey( KeyMsg const& msg ){ return true; }
	virtual bool onChar( unsigned code ){ return true; }
};





#endif // PlatformBase_h__
