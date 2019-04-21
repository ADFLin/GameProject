#include "TetrisPCH.h"

#include "TetrisAction.h"
#include "TetrisLevelManager.h"

#include "GameServer.h"
#include "GameClient.h"

namespace Tetris
{

	CFrameActionTemplate::CFrameActionTemplate( GameWorld& world ) 
		:KeyFrameActionTemplate( gTetrisMaxPlayerNum )
		,mWorld( world )
	{

	}

	void CFrameActionTemplate::firePortAction(ActionPort port, ActionTrigger& trigger )
	{
		mWorld.fireLevelAction( port , trigger );
	}

}//namespace Tetris
