#ifndef RichPlayerTurn_h__
#define RichPlayerTurn_h__

#include "RichBase.h"

#include "RichWorld.h"
#include "RichTool.h"

namespace Rich
{

	enum RequestID
	{
		REQ_NONE ,
		REQ_ROMATE_DICE_VALUE ,
		REQ_BUY_LAND ,
		REQ_UPGRADE_LAND ,
		REQ_MOVE_DIR ,
	};

	union ReqData
	{
		int numDice;

		struct 
		{
			LandCell* land;
			int   money;
		};
		struct
		{
			int      numDir;
			DirType* dir;
		};
	};

	class IController
	{
	public:
		virtual void queryAction( RequestID id , PlayerTurn& turn , ReqData const& data ) = 0;
	};




	union ReplyData
	{
		int intVal;
	};

	typedef stdex::function< void ( ReplyData const& ) > ReqCallback;





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
			STEP_PROCESS_CELL ,
			STEP_MOVE_END ,
			STEP_END ,
		};

		enum TurnState
		{
			TURN_END  ,
			TURN_KEEP ,
			TURN_WAIT ,
		};

		Player*  getPlayer(){ return mPlayer; }

		void     beginTurn( Player& player );
		bool     update();
		void     endTurn();

		void     goMoveByPower( int movePower );
		void     goMoveByStep( int numStep );

		

		void replyBuyLand( ReplyData const& answer , LandCell* land , int piece );
		void replyUpgradeLevel( ReplyData const& answer , LandCell* land , int cost );
		void replyRomateDiceValue( ReplyData const& answer );

		void calcRomateDiceValue( int totalStep , int numDice , int value[] );
		void replyAction( ReplyData const& answer );

		bool waitQuery()
		{
			return mReqInfo.id != REQ_NONE;
		}

		struct RequestInfo
		{
			RequestID   id;
			ReqCallback callback;
		};

		bool     sendTurnMsg( WorldMsg const& event );
		void     queryAction( RequestID id, ReqData const& data );
		bool     checkKeepRun();
		void     updateTurnStep();

		void     useTool( ToolType type )
		{
			Tool* tool = Tool::FromType( type );

		}

		TurnStep     mStep;
		TurnState    mTurnState;
		
		bool         mUseRomateDice;
		RequestInfo  mReqInfo;
		RequestID    mLastAnswerReq;

		ITurnControl* mControl;

		DirType      mMoveDir;
		Tile*        mTile;
		int          mNumDice;

		MapCoord     mPosPrevMove;
		int          mTotalMoveStep;
		int          mCurMoveStep;

		Player*      mPlayer;
	};
}



#endif // RichPlayerTurn_h__
