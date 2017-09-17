#ifndef RichPlayerTurn_h__
#define RichPlayerTurn_h__

#include "RichBase.h"

#include "RichWorld.h"
#include "RichTool.h"

namespace Rich
{

	enum ActionRequestID
	{
		REQ_NONE ,
		REQ_ROMATE_DICE_VALUE ,
		REQ_BUY_LAND ,
		REQ_UPGRADE_LAND ,
		REQ_MOVE_DIR ,
	};

	struct ActionReqestData
	{
		union
		{
			int numDice;

			struct
			{
				LandArea* land;
				int   money;
			};
			struct
			{
				int         numLink;
				LinkHandle* links;
			};
		};
	};

	class IController
	{
	public:
		virtual void queryAction( ActionRequestID id , PlayerTurn& turn , ActionReqestData const& data ) = 0;
	};

	struct ActionReplyData : ParamCollection<2>
	{

	};

	typedef stdex::function< void ( ActionReplyData const& ) > ReqCallback;

	class ITurnControl
	{
	public:
		virtual bool needRunTurn() = 0;
	};

	class PlayerTurn
	{

	public:

		enum TurnStep
		{
			STEP_WAIT  ,
			STEP_START ,
			STEP_THROW_DICE ,
			STEP_MOVE_START ,
			STEP_MOVE_DIR ,
			STEP_MOVE_EVENT ,
			STEP_PROCESS_AREA ,
			STEP_MOVE_END ,
			STEP_END ,
		};

		enum TurnState
		{
			TURN_KEEP,
			TURN_WAIT,
			TURN_END  ,
		};

		Player*  getPlayer(){ return mPlayer; }
		World&   getWorld();

		void     beginTurn( Player& player );
		bool     update();
		void     endTurn();

		void     goMoveByPower( int movePower );
		void     goMoveByStep( int numStep );
		void     obtainMoney(int money);

		void     loseMoney(int money);

		void     calcRomateDiceValue( int totalStep , int numDice , int value[] );
		void     replyAction( ActionReplyData const& answer );

		bool     isWaitingQuery()
		{
			return mReqInfo.id != REQ_NONE;
		}

		struct RequestInfo
		{
			ActionRequestID   id;
			ReqCallback callback;
		};

		bool     sendTurnMsg( WorldMsg const& event );
		void     queryAction( ActionRequestID id, ActionReqestData const& data );
		bool     checkKeepRun();
		void     updateTurnStep();

		void     useTool( ToolType type )
		{
			Tool* tool = Tool::FromType( type );
			tool->use(*this);
		}

		TurnStep     mStep;
		TurnState    mTurnState;
		
		bool         mUseRomateDice;
		RequestInfo  mReqInfo;
		ActionRequestID    mLastAnswerReq;

		ITurnControl* mControl;

		LinkHandle   mMoveLinkHandle;
		Tile*        mTile;
		int          mNumDice;

		MapCoord     mPosPrevMove;
		int          mTotalMoveStep;
		int          mCurMoveStep;

		Player*      mPlayer;
	};
}



#endif // RichPlayerTurn_h__
