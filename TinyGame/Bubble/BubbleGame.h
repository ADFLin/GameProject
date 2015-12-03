#ifndef BubbleGame_h__
#define BubbleGame_h__

#include "GamePackage.h"
#include "GameReplay.h"

#include "BubbleAction.h"

#define BUBBLE_NAME "Bubble"

class StageManager;

namespace Bubble
{
	class Controller : public SimpleController
	{
	public:
		virtual bool checkAction( ActionParam& param );
		int     oldX;
	};

	class CGamePackage : public IGamePackage
	{
	public:
		char const*     getName(){ return BUBBLE_NAME;  }
		GameController& getController(){  return mController;  }
		ReplayTemplate* createReplayTemplate( unsigned version );
		GameSubStage*   createSubStage( unsigned id );
		bool            getAttribValue( AttribValue& value );
		SettingHepler*  createSettingHelper( SettingHelperType type );
		void            enter( StageManager& manger );
		virtual void    deleteThis(){ delete this; }
	private:
		Controller mController;
	};

	struct BubbleGameInfo
	{
		unsigned padding;
	};

	class BubbleReplayTemplate : public ReplayTemplate
	{
	public:
		static uint32 const Version = VERSION(0,0,1);

		bool   getReplayInfo( ReplayInfo& info );

		void   startFrame();
		size_t inputNodeData( unsigned id , char* data );
		bool   checkAction( ActionParam& param );
		void   listenAction( ActionParam& param );

		void   registerNode( OldVersion::Replay& replay );
		void   recordFrame( OldVersion::Replay& replay , unsigned frame );

	private:
		struct ActionBitNode
		{
			unsigned pID : 4;
			unsigned actionBit : 28;
		};

		struct MouseActionBitNode
		{
			unsigned pID : 4;
			unsigned actionBit : 28;
			int      offset;
		};

		BubbleGameInfo mGameInfo;
		unsigned mActionBit[ gBubbleMaxPlayerNum ];
		int      mMoveOffset[ gBubbleMaxPlayerNum ];
	};


}// namespace Bubble


#endif // BubbleGame_h__
