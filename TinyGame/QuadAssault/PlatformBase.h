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
	virtual MsgReply onMouse( MouseMsg const& msg ){ return MsgReply::Unhandled(); }
	virtual MsgReply onKey( KeyMsg const& msg ){ return MsgReply::Unhandled(); }
	virtual MsgReply onChar( unsigned code ) { return MsgReply::Unhandled(); }
};





#endif // PlatformBase_h__
