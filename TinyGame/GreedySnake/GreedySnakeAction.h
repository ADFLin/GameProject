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

		void firePortAction( ActionTrigger& trigger )
		{
			mScene->fireSnakeAction( trigger );
		}
		Scene* mScene;
	};

	typedef SVKeyFrameGenerator ServerFrameGenerator;
	typedef CLKeyFrameGenerator ClientFrameGenerator;

}//namespace GreedySnake

#endif // GreedySnakeAction_h__
