#ifndef TetrisAction_h__
#define TetrisAction_h__

#include "GameAction.h"

namespace Tetris
{
	class GameWorld;

	class CFrameActionTemplate : public KeyFrameActionTemplate
	{
	public:
		static unsigned const LastVersion = MAKE_VERSION(0,0,1);

		CFrameActionTemplate( GameWorld& world );
		void  firePortAction( ActionTrigger& trigger );
		GameWorld& mWorld;
	};

	typedef ServerKeyFrameCollector CServerFrameGenerator;
	typedef ClientKeyFrameCollector CClientFrameGenerator;

} //namespace Tetris

#endif // TetrisAction_h__
