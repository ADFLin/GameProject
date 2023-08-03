#ifndef RichPlayerTurn_h__
#define RichPlayerTurn_h__

#include "RichBase.h"

#include "RichWorld.h"
#include "RichTool.h"

namespace Rich
{

	class ActionEvent
	{
	public:

		virtual void doAction(Player* player, PlayerTurn& turn) = 0;
	};

	enum ActionRequestID
	{
		REQ_NONE ,
		REQ_ROMATE_DICE_VALUE ,
		REQ_BUY_LAND ,
		REQ_BUY_STATION ,
		REQ_UPGRADE_LAND ,
		REQ_MOVE_DIR ,
		REQ_DISPOSE_ASSET,
		REQ_MOVE_DIRECTLY ,
	};

	struct ActionReqestData
	{
		union
		{
			struct
			{
				int numDice;
			};
			struct //Buy
			{
				union
				{
					class OwnableArea* ownable;
					class LandArea*    land;
					class StationArea* station;
				};
				int   money;
			};
			struct
			{
				int         numLink;
				LinkHandle* links;
			};
			struct 
			{
				int debt;
				Player* creditor;
			};
		};
	};

	class IPlayerController
	{
	public:
		virtual void startTurn(PlayerTurn& turn){}
		virtual void endTurn(PlayerTurn& turn){}
		virtual void queryAction( ActionRequestID id , PlayerTurn& turn , ActionReqestData const& data ) = 0;
	};

	struct ActionReplyData : ParamCollection<2>
	{

	};

	typedef std::function< void ( ActionReplyData const& ) > ReqCallback;

	class PlayerTurn
	{

	public:

		Player*  getPlayer(){ return mPlayer; }
		World&   getWorld();

		void     startTurn( Player& player );

		void     endTurn();

		void     runTurnAsync();

		void     tick()
		{

		}

		void     goMoveByPower( int movePower );
		void     goMoveByStep( int numStep );

		void     moveDirectly(Player& player);

		void     moveTo(Player& player, Area& area, bool bTriggerArea = true);


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


		void     useTool( ToolType type )
		{
			Tool* tool = Tool::FromType( type );
			tool->use(*this);
		}

		void doBankruptcy(Player& player);

		bool processDebt(Player& player, int debt, std::function<void(int)> callback );
		
		bool         mUseRomateDice;
		RequestInfo  mReqInfo;
		void queryResuest(Player& player, ActionReqestData const& data);
		ActionRequestID  mLastAnswerReq;

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
