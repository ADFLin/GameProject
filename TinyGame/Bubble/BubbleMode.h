#ifndef BubbleMode_h__
#define BubbleMode_h__

#include "BubbleLevel.h"
#include <vector>

class ActionTrigger;
struct ActionPort;
class Graphics2D;

namespace Bubble
{
	enum EventID
	{
		BU_EVT_REMOVE_SHOOT_BUBBLE ,
		BU_EVT_UPDATE_SHOOT_BUBBLE ,
		BU_EVT_ADD_FALL_BUBBLE ,
		BU_EVT_POP_BUBBLE ,
		BU_EVT_TICK ,
	};

	struct Event
	{
		int id;
		union
		{
			struct
			{
				Level::Bubble* bubble;
				unsigned       result;
			};
			struct
			{
				BubbleCell* cell;
				int index;
			};
		};
	};

	class Mode;

	class PlayerData : public LevelListener
	{
	public:
		PlayerData( Mode* mode , unsigned id );
		Scene&   getScene(){ return *mScene;  }
		unsigned getId(){ return mId; }
	private:
		virtual void onRemoveShootBubble( Bubble* bubble );
		virtual void onUpdateShootBubble( Bubble* bubble , unsigned result );
		virtual void onPopCell( BubbleCell& cell , int index );
		virtual void onAddFallCell( BubbleCell& cell , int index );

		unsigned mId;
	protected:
		Scene* mScene;
		Mode*  mMode;
	};

	class Mode
	{
	public:
		virtual void tick(){}
		virtual PlayerData* createPlayerData( unsigned id ) = 0;
		virtual void onLevelEvent( PlayerData& data , Event& event ) = 0;
	};

	class PlayerDataManager
	{
	public:
		void        setMode( Mode* mode ){  mMode = mode;  }
		void        tick();
		void        updateFrame( int frame );
		void        fireAction( ActionTrigger& trigger );
		void        firePlayerAction(ActionPort port, ActionTrigger& trigger );
		void        render( Graphics2D& g );
		PlayerData* createData();
		PlayerData* getPlayerData( unsigned id )
		{
			return mPlayerDataVec[id];
		}
		typedef std::vector< PlayerData* > PlayerDataVec;
		PlayerDataVec mPlayerDataVec;
		Mode*         mMode;
	};


	class TestMode : public Mode
	{
	public:
		virtual PlayerData* createPlayerData( unsigned id )
		{
			return new PlayerData( this , id );
		}
		virtual void onLevelEvent( PlayerData& data , Event& event )
		{

		}
	};




}//namespace Bubble


#endif // BubbleMode_h__
