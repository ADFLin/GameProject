#include "TinyGamePCH.h"
#include "GamePackage.h"

#include "GameControl.h"
#include "StageBase.h"

class EmptyController : public GameController
{
public:
	virtual bool  scanInput( bool beUpdateFrame ){ return false;  }
	virtual bool  checkAction( ActionParam& param  ){ return false;  }

	virtual void  blockKeyEvent( bool beB ){}
	virtual bool  haveLockMouse(){ return false;  }

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
GameController& IGamePackage::getController()
{
	return gEmptyController;
}

void IGamePackage::beginPlay( StageModeType type, StageManager& manger )
{
	switch( type )
	{
	case SMT_SINGLE_GAME: manger.changeStage( STAGE_SINGLE_GAME ); break;
	case SMT_REPLAY: manger.changeStage( STAGE_REPLAY_GAME ); break;
	case SMT_NET_GAME: /*manger.changeStage( STAGE_NET_GAME );*/ break;
	}
}
