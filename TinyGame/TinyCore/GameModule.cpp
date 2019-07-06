#include "TinyGamePCH.h"
#include "GameModule.h"

#include "GameControl.h"
#include "StageBase.h"

class EmptyController : public GameController
{
public:
	virtual void  setupInput(ActionProcessor& proccessor) {}

	virtual void  blockAllAction( bool beB ){}
	virtual bool  shouldLockMouse(){ return false;  }

	virtual void  setPortControl( unsigned port , unsigned cID ){}
	virtual void  setKey( unsigned cID , ControlAction action , unsigned key ){}
	virtual char  getKey( unsigned cID , ControlAction action ) { return 0;  }
	virtual bool  checkKey( unsigned cID , ControlAction action ) { return false; }

	virtual void  recvMouseMsg( MouseMsg const& msg ){}
	virtual void  clearFrameInput(){}
};

namespace
{
	EmptyController gEmptyController;
}
GameController& IGameModule::getController()
{
	return gEmptyController;
}

void IGameModule::beginPlay( StageModeType type, StageManager& manger )
{
	switch( type )
	{
	case SMT_SINGLE_GAME:
		{
			AttribValue value(ATTR_SINGLE_SUPPORT);
			if( getAttribValue(value) && value.iVal )
			{
				manger.changeStage(STAGE_SINGLE_GAME);
			}
			else
			{


			}
		}
		break;
	case SMT_REPLAY: 
		{
			AttribValue value(ATTR_REPLAY_SUPPORT);
			if( getAttribValue(value) && value.iVal )
			{
				manger.changeStage(STAGE_REPLAY_GAME);
			}
			else
			{


			}
		}
		break;
	case SMT_NET_GAME: 
		{
			AttribValue value(ATTR_NET_SUPPORT);
			if( getAttribValue(value) && value.iVal )
			{
				manger.changeStage( STAGE_NET_GAME );
			}
			else
			{


			}
		}
		break;
	}
}
