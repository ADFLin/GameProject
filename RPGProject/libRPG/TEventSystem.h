#ifndef TEventSystem_h__
#define TEventSystem_h__

#include "common.h"

#include "Singleton.h"
#include "CHandle.h"
#include "EventType.h"

#include <map>
#include <list>
#include <vector>

class TEntityBase;
class SlotFunBase;
enum  EventType;

#define CONNECT_SUCESS_UPDATE_ID    2
#define CONNECT_SUCESS              1
#define CONNECT_FAIL_NO_EVENT_TYPE -1
#define CONNECT_FAIL_READY_HOLD    -2

#define CONNECT_FAIL_SLOT_TYPE_ERROR   -3
#define CONNECT_FAIL_NO_SIGNAL_RESIGTE -4
#define CONNECT_FAIL_NO_SOLT_RESIGTE   -5


class EvtCallBack;

class TEventSystem : public SingletonT< TEventSystem >
{
public:
	enum FnType
	{
		FN_DEUFLT ,
		FN_SLOT   ,
	};

	enum
	{
		EVRF_TYPE_MASK = 0x00000f ,
		EVRF_USAGE_REF = 0x000010 ,
	};

	TEventSystem();
 
	void  updateFrame();
	void  addEvent( TEvent& event );

	int   connectEvent(  EventType type ,int id , EvtCallBack const& callback , TRefObj* holder );


	//int   connectSignal( TRefObj* holder , int type , TRefObj* sender , int signalID , SlotFunBase* slot );
	//void  disconnectSignal( TRefObj* holder , int type , TRefObj* sender , int signalID , SlotFunBase* slot );

	void  disconnectEvent( EventType type , EvtCallBack const& callback , TRefObj* holder  = nullptr );
	
	//void addSignal( int slotType , int id ,TRefObj* obj , void* data );

	void  process( TEvent& event );

	//bool   isProcessInFun( TRefObj* entity , FnMemEvent fun );
protected:
	void processInternal( TEvent& event );

	void processDisconnect();
	
	struct TEventReg
	{
		unsigned      flag;
		EventType     type;
		int           id;
		RefHandle     holder;
		EvtCallBack   callBack;
		//SlotFunBase* slotFun;
		//for signal;
		//RefHandle     sender;
	};

	struct EvtRemove
	{
		EventType     type;
		TRefObj*      holder;
		EvtCallBack   callBack;
	};
	typedef std::list< EvtRemove > EvtRemoveList;
	EvtRemoveList mDisConEvtList;

	typedef std::list< TEventReg > EvtRegList;
	int  checkEvent( EvtCallBack const& callback , TRefObj* holder , EventType type ,int id , EvtRegList** regList 
		            /*, TRefObj* sender , SlotFunBase* slot*/ );
	void removeEvent( EvtRemove const& er );

	bool        mProcessEvent;

	struct EvtRegData
	{
		EvtRegList evtRegList;
	};

	typedef std::vector<TEvent> EventVector;
	EventVector mFrameEventVec;

	typedef std::map< EventType , EvtRegData > EvtTypeDataMap;
	EvtTypeDataMap m_evtTypeMap;
};





#endif // TEventSystem_h__