#ifndef GreedySnakeAction_h__
#define GreedySnakeAction_h__

#include "GameAction.h"
#include "GreedySnakeScene.h"

namespace GreedySnake
{
	class CFrameAcionTemplate : public KeyFrameActionTemplate
	{
	public:
		CFrameAcionTemplate( Scene* scene )
			:KeyFrameActionTemplate( gMaxPlayerNum )
			,mScene( scene )
		{}

		void firePortAction(ActionPort port, ActionTrigger& trigger )
		{
			mScene->fireSnakeAction( port, trigger );
		}
		Scene* mScene;
	};

	typedef ServerKeyFrameCollector ServerFrameCollector;
	typedef ClientKeyFrameCollector ClientFrameCollector;

}//namespace GreedySnake

#endif // GreedySnakeAction_h__
