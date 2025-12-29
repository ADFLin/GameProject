#include "TinyGamePCH.h"
#include "GameModule.h"

#include "GameControl.h"
#include "StageBase.h"

class EmptyInputControl : public InputControl
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
	EmptyInputControl GEmptyInputControl;
}

HashString IGameModule::FeatureName = "GameModule";

void IGameModule::startupModule()
{
	IModularFeatures::Get().registerFeature(FeatureName, this);
}

void IGameModule::shutdownModule()
{
	IModularFeatures::Get().unregisterFeature(FeatureName, this);
}

InputControl& IGameModule::getInputControl()
{
	return GEmptyInputControl;
}

bool IGameModule::changeDefaultStage(StageManager& stageManager, EGameMode modeType)
{
	switch( modeType )
	{
	case EGameMode::Single:
		{
			GameAttribute value(ATTR_SINGLE_SUPPORT);
			if( queryAttribute(value) && value.iVal )
			{
				stageManager.changeStage(STAGE_SINGLE_GAME);
				return true;
			}
			else
			{


			}
		}
		break;
	case EGameMode::Replay:
		{
			GameAttribute value(ATTR_REPLAY_SUPPORT);
			if( queryAttribute(value) && value.iVal )
			{
				stageManager.changeStage(STAGE_REPLAY_GAME);
				return true;
			}
			else
			{


			}
		}
		break;
	case EGameMode::Net:
		{
			GameAttribute value(ATTR_NET_SUPPORT);
			if( queryAttribute(value) && value.iVal )
			{
				stageManager.changeStage(STAGE_NET_GAME);
				return true;
			}
			else
			{


			}
		}
		break;
	}

	return false;
}
