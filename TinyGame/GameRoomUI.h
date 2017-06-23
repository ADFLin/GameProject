#ifndef GameRoomUI_h__
#define GameRoomUI_h__

#include "GameWidget.h"
#include "GameGlobal.h"
#include "GamePlayer.h"

class ComWorker;
class ServerWorker;
class SPPlayerStatus;
class PlayerListPanel;

enum SlotState
{
	SLOT_OPEN       = -1,
	SLOT_CLOSE      = -2,
	SLOT_PLAYER     = -3,
	SLOT_DISABLE    = -4,
};

class  SlotFrame : public GFrame
{
	typedef GFrame BaseClass;
public:
	struct Slot 
	{
		SlotId      id;
		SlotState   state;
		PlayerInfo const* info;
		GChoice*    choice;
		SlotFrame*  frame;
		int         idxData;
	};


	SlotFrame( Slot& slot , Vec2i const& pos , Vec2i const& size , PlayerListPanel* parent );

	Slot&     getSlot(){ return mSlot;  }
	GChoice*  getStateChioce(){ return mStateChoice; }

	void onRender();
	bool onMouseMsg( MouseMsg const& msg );
	void enableDrag( bool beCan ){ mCanDrag = beCan; }

	Slot&     mSlot;
	GChoice*  mStateChoice;
	bool      mCanDrag;
};

enum
{
	EVT_SLOT_CHANGE = EVT_EXTERN_EVENT_ID + 10 ,
	EVT_PLAYER_TYPE_CHANGE ,
};


class  PlayerListPanel : public GPanel
{
	typedef GPanel BaseClass;
public:
	static GAME_API Vec2i const WidgetSize;
	static int const NoGroup = 0;

	typedef SlotFrame::Slot Slot;

	GAME_API PlayerListPanel( IPlayerManager* manager , int _id , Vec2i const& pos , GWidget* parent );
	GAME_API void init( bool haveServer );
	

	GAME_API void    setupPlayerList( SPPlayerStatus& status );
	GAME_API void    refreshPlayerList( int* idx , int* state );

	GAME_API SlotId  addPlayer( PlayerInfo const& info );

	Slot&    getSlot( SlotId id ){ return *mPlayerSlots[ id ];  }
	void     swapSlot( SlotId s1 , SlotId s2 );
	PlayerId emptySlot( SlotId id , SlotState state );
	SlotId   findFreeSlot();
	SlotId   findPlayer( PlayerId id );
	void     onUpdateUI();
	void     onRender();

	
	virtual bool onChildEvent( int event , int id , GWidget* ui );

	int     getSlotState( int* idx , int* state );
	
	int     getSlotNum(){ return mSlotNum; }
	void    setupSlotNum( int num );

	static int const OpenSelection = 0;
	static int const CloseSelection = 1;

	void   procSlotFrameMouseMsg( SlotFrame* frame , MouseMsg const& msg );
	SlotId calcSoltId( int y )
	{
		int idx = ( y - 10 + 35 / 2 ) / 35;
		if ( idx < 0 || idx >= getSlotNum() )
			return ERROR_SLOT_ID;
		return SlotId( idx );
	}
	Vec2i calcSlotFramePos( SlotId id )
	{
		return Vec2i( 20 , 10 + id * 35 );
	}
	SlotFrame* mDragFrame;
	SlotFrame* mSwapFrame;
	void  getSwapSlot( SlotId swapId[2] );

	IPlayerManager* getPlayerManager(){ return mManager; }
protected:
	void   setSlot( int index , char const* name );
	void   addSlotFrameAnim( SlotFrame* frame , SlotId destId );

	int             mSlotNum;
	Slot*           mPlayerSlots[ MAX_PLAYER_NUM ];
	Slot            mSlotStorage[ MAX_PLAYER_NUM ];
	int             mSlotGroup[ MAX_PLAYER_NUM ];
	IPlayerManager* mManager;
};


class  ComMsgPanel : public GPanel
{
	typedef GPanel BaseClass;
public:
	enum
	{
		UI_MSG_CTRL   = BaseClass::NEXT_UI_ID ,
		UI_MSG_SLIDER ,
		NEXT_UI_ID ,
	};

	GAME_API ComMsgPanel( int _id , Vec2i const& pos , Vec2i const& size , GWidget* parent );

	void setWorker( ComWorker* worker ){ mWorker = worker; }
	GAME_API void addMessage( char const* str , unsigned long color );

	void renderText( GWidget* ui );
	bool onChildEvent( int event , int id , GWidget* ui );

	void onFocus( bool beF )
	{
		if ( beF )
			mMsgTextCtrl->makeFocus();
	}
	void clearInputString()
	{
		mMsgTextCtrl->clearValue();
	}
	void clearMessage()
	{
		mMsgList.clear();
	}

	struct  MsgInfo
	{
		MsgInfo( char const* s , unsigned long c ):msg( s ) ,color( c ){}
		unsigned long  color;
		std::string    msg;
	};
	typedef  std::list< MsgInfo > MsgList;

	ComWorker* mWorker;
	MsgList    mMsgList;
	GTextCtrl* mMsgTextCtrl;
};
#endif // GameRoomUI_h__