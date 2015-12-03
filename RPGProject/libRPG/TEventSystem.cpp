#include "ProjectPCH.h"
#include "TEventSystem.h"

#include "CHandle.h"
#include "EventType.h"

#include "ProfileSystem.h"
//#include "TSlotSingalSystem.h"


TEventSystem::TEventSystem()
{
	mProcessEvent = false;
}


void TEventSystem::updateFrame()
{
	//DevMsg( 1 , "NumEvnt %d in this frame" , m_frameEventVec.size() );

	PROFILE_ENTRY("Event Process");

	mProcessEvent = true;
	for( int i = 0; i < mFrameEventVec.size() ; ++ i )
	{
		TEvent& event = mFrameEventVec[i];
		processInternal( event );
	}
	mProcessEvent = false;
	mFrameEventVec.clear();

	processDisconnect();

	return;

}

void TEventSystem::addEvent( TEvent& event )
{
	mFrameEventVec.push_back( event );
	//process( event );
}

int TEventSystem::checkEvent( EvtCallBack const& callback , TRefObj* holder , EventType type ,int id , EvtRegList** regList 
							/*, TRefObj* sender , SlotFunBase* slot*/ )
{
	EvtTypeDataMap::iterator evtIter = m_evtTypeMap.find( type );
	if ( evtIter == m_evtTypeMap.end() )
	{
		m_evtTypeMap.insert( std::make_pair( type , EvtRegData() ) );
		evtIter = m_evtTypeMap.find( type );
	}

	EvtRegData& data = evtIter->second;

	EvtRegList& evtRegList = data.evtRegList;

	*regList = &evtRegList;

	for( EvtRegList::iterator iter = evtRegList.begin(); 
		iter != evtRegList.end() ; ++ iter )
	{
		if ( iter->callBack != callback )
			continue;

		if ( holder )
		{
			if ( holder == iter->holder )
				return CONNECT_FAIL_READY_HOLD;
		}

		if ( iter->id == EVENT_ANY_ID )
			return CONNECT_FAIL_READY_HOLD;

		if ( iter->id == id )
		{
			unsigned fnType = iter->flag & EVRF_TYPE_MASK;

			if ( fnType != FN_SLOT )
				return CONNECT_FAIL_READY_HOLD;
			//check Signal-Slot
			//if ( iter->sender == sender  && iter->slotFun == slot )
			//	return CONNECT_FAIL_READY_HOLD;
		}
	}

	return CONNECT_SUCESS;
}


void TEventSystem::processInternal( TEvent& event )
{

	EvtTypeDataMap::iterator evtIter = m_evtTypeMap.find( event.type );
	if ( evtIter == m_evtTypeMap.end() )
		return;

	EvtRegData& data = evtIter->second;

	EvtRegList& evtRegList = data.evtRegList;

	for( EvtRegList::iterator iter = evtRegList.begin(); 
		iter != evtRegList.end() ; )
	{
		if ( iter->id != EVENT_ANY_ID && event.id != EVENT_ANY_ID )
		{
			if ( iter->id != event.id )
			{
				++iter;
				continue;
			}
		}

		unsigned fnType = iter->flag &  EVRF_TYPE_MASK;

		switch( fnType )
		{
		case FN_DEUFLT:
			{
				if ( iter->flag & EVRF_USAGE_REF )
				{
					TRefObj* holder = iter->holder;
					if ( !holder )
					{
						iter = evtRegList.erase( iter );
						continue;
					}
				}

				iter->callBack.exec( event );
			}
			break;
		case FN_SLOT:
			break;
		}
		
		++iter;
	}
}

//bool TEventSystem::isProcessInFun( TRefObj* entity , FnMemEvent fun )
//{
//	if ( !m_curEventReg ) return false;
//	return ( m_curEventReg->holder == entity &&
//		     m_curEventReg->memfun == fun );
//}

void TEventSystem::disconnectEvent( EventType type , EvtCallBack const& callback , TRefObj* holder  )
{
	TEventReg  evtReg;
	evtReg.type   = type;
	evtReg.id     = EVENT_ANY_ID;
	evtReg.holder = holder ;
	evtReg.flag   = FN_DEUFLT;

	EvtRemove er;
	er.type = type;
	er.callBack = callback;
	er.holder = holder;

	mDisConEvtList.push_back( er );

}



//void TEventSystem::addSignal( int slotType , int id ,TRefObj* obj , void* data )
//{
//	TEvent event( (EventType)(EVT_SIGNAL + slotType), id , obj , &data );
//	addEvent( event );
//}
//
//int TEventSystem::connectSignal( TRefObj* holder , int type  , TRefObj* sender, int signalID , SlotFunBase* slot )
//{
//	TEventReg* evtReg;
//
//	EventType evtType = (EventType)(EVT_SIGNAL + type);
//	EvtRegList* regList = NULL;
//	int result = checkEvent( holder , evtType , signalID , &regList , sender , slot );
//
//	if ( result == CONNECT_SUCESS )
//	{
//		TEventReg evtReg;
//
//		evtReg.type     = evtType;
//		evtReg.holder   = holder;
//		evtReg.id       = signalID;
//		evtReg.fnType   = FN_SLOT;
//		evtReg.slotFun  = slot;
//		evtReg.sender   = sender;
//		regList->push_back( evtReg );
//	}
//
//	return result;
//}

//void TEventSystem::disconnectSignal( TRefObj* holder , int type , TRefObj* sender , int signalID , SlotFunBase* slot )
//{
//	TEventReg evtReg;
//	evtReg.type    = (EventType)(EVT_SIGNAL + type);
//	evtReg.id      = signalID;
//	evtReg.fnType  = FN_SLOT;
//	evtReg.holder  = holder;
//	evtReg.sender  = sender;
//	evtReg.slotFun = slot;
//
//	m_disEvtList.push_back( evtReg );
//}

void TEventSystem::removeEvent( EvtRemove const& er )
{
	EvtTypeDataMap::iterator evtIter = m_evtTypeMap.find( er.type );
	if ( evtIter == m_evtTypeMap.end() )
		return;

	EvtRegData& data = evtIter->second;
	EvtRegList& evtRegList = data.evtRegList;

	for( EvtRegList::iterator iter2 = evtRegList.begin(); 
		iter2 != evtRegList.end() ; )
	{
		if ( iter2->callBack != er.callBack )
		{
			++iter2;
			continue;
		}
		if ( er.holder )
		{
			if ( iter2->flag & EVRF_USAGE_REF && er.holder == iter2->holder )
			{
				++iter2;
				continue;
			}
		}
		iter2 = evtRegList.erase( iter2 );
	}

}
void TEventSystem::processDisconnect()
{
	for ( EvtRemoveList::iterator iter = mDisConEvtList.begin(); 
		 iter != mDisConEvtList.end() ; ++iter )
	{
		removeEvent( *iter );
	}
	mDisConEvtList.clear();
}

int TEventSystem::connectEvent( EventType type ,int id , EvtCallBack const& callback , TRefObj* holder )
{
	EvtRegList* regList = NULL;
	int result = checkEvent( callback , holder , type , id , &regList );

	if ( result == CONNECT_SUCESS )
	{
		TEventReg evtReg;

		evtReg.type     = type;
		evtReg.holder   = holder;
		evtReg.id       = id;
		
		evtReg.callBack = callback;
		evtReg.flag     = FN_DEUFLT;

		if ( holder )
			evtReg.flag |= EVRF_USAGE_REF;

		regList->push_back( evtReg );
	}
	return result;
}

void TEventSystem::process( TEvent& event )
{
	mProcessEvent = true;
	processInternal( event );
	mProcessEvent = false;
}

void UG_SendEvent( TEvent& event )
{
	TEventSystem::getInstance().addEvent( event );
}

void UG_ProcessEvent( TEvent& event )
{
	TEventSystem::getInstance().process( event );
}

void  UG_ConnectEvent( EventType type , int id , EvtCallBack const& callback , TRefObj* holder )
{
	TEventSystem::getInstance().connectEvent( type , id , callback , holder );
}
void  UG_DisconnectEvent( EventType type , EvtCallBack const& callback , TRefObj* holder )
{
	TEventSystem::getInstance().disconnectEvent( type , callback , holder );
}

