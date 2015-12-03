#ifndef EventType_h__
#define EventType_h__

enum EventType
{
	EVT_PlAYER_DEAD ,
	EVT_COMBAT_DAMAGE ,

	EVT_FX_DELETE    ,
	EVT_FX_PLAY_NEXT ,

	EVT_SKILL_BUFF_START ,
	EVT_SKILL_BUFF_END   ,
	EVT_SKILL_CANCEL     ,
	EVT_SKILL_END        ,

	EVT_TALK_START ,
	EVT_TALK_CONCENT ,
	EVT_TALK_SECTION_END ,
	EVT_TALK_END ,

	EVT_ENTITY_DESTORY   ,

	EVT_SKILL_LEARN ,


	EVT_PLAY_CD_START ,
	EVT_SKILL_CD_FINISH ,
	EVT_ITEM_CD_FINISH ,

	EVT_UI_BUTTON_CLICK ,


	EVT_SIGNAL ,
	EVT_SIGNAL_VOID ,
	EVT_SIGNAL_BOOL ,
	EVT_SIGNAL_INT ,
	EVT_SIGNAL_UNKNOW,

	EVT_HOT_KEY ,
	EVT_LEVEL_UP ,

	EVT_UNDEFINE ,
};

#define EVENT_ANY_ID -1

struct TEvent
{
	TEvent( EventType type , int id = EVENT_ANY_ID , 
		    void* sender = nullptr , void* data = nullptr )
		:type( type )
		,id(id)
		,sender(sender)
		,data(data)
	{

	}
	TEvent()
	{
		type   = EVT_UNDEFINE;
		sender = nullptr;
		data   = nullptr;
	}
	EventType    type;
	int          id;
	void*        sender;
	void*        data;
};

class EvtCallBack
{
public:
	template< class T , class Fun >
	EvtCallBack( T* ptr , Fun fun )
	{
		mCallback.bind( ptr , fun );
	}
	template< class Fun >
	EvtCallBack( Fun fun )
	{
		mCallback.bind( fun );
	}
	EvtCallBack(){}

	void exec( TEvent const& event ){ mCallback( event ); }

	EvtCallBack& operator = ( EvtCallBack const& rh ){ mCallback = rh.mCallback ; return *this; }
	bool operator == ( EvtCallBack const& rh ) const { return mCallback == rh.mCallback ; }
	bool operator != ( EvtCallBack const& rh ) const { return mCallback != rh.mCallback ; }
private:
	fastdelegate::FastDelegate< void ( TEvent const& ) > mCallback;
};


#endif // EventType_h__