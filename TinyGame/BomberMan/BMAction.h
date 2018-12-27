#ifndef BMAction_h__
#define BMAction_h__

#include "GameAction.h"

namespace BomberMan
{
	class Player;
	class World;

	enum Action
	{
		ACT_BM_MOVE_LEFT ,
		ACT_BM_MOVE_RIGHT ,
		ACT_BM_MOVE_TOP ,
		ACT_BM_MOVE_DOWN ,
		ACT_BM_BOMB ,
		ACT_BM_FUNCTION ,
	};

	void EvalPlayerAction( Player& player , ActionTrigger& trigger );

	class CFrameActionTemplate : public TKeyFrameActionTemplate< KeyFrameData >
	{
	public:
		static unsigned const LastVersion = MAKE_VERSION(0,0,1);

		CFrameActionTemplate( World& level );
		virtual void firePortAction( ActionTrigger& trigger );
		World& mLevel;
	};

	typedef SVKeyFrameGenerator CServerFrameGenerator;
	typedef CLKeyFrameGenerator CClientFrameGenerator;

}//namespace BomberMan


#endif // BMAction_h__
