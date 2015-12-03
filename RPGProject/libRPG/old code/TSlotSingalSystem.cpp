#include "TSlotSingalSystem.h"

#include "EventType.h"
#include "TLogicObj.h"

void TSignal<void>::send( TRefObj* sender )
{
	SignalManager::instance().addSignal(
		EvalEvtType<void>::result_value	, id , sender , NULL  );
}


TSignal<void>::TSignal(  char const* name )
{
	id = SignalManager::instance().resigter(  name , EvalEvtType<void>::result_value );
}


int SignalManager::connect( TRefObj* sender , char const* signalName , TRefObj* holder , char const* slotName )
{
	SignalInfo* info  = getSignalInfo( signalName );

	if ( !info  )
		return CONNECT_FAIL_NO_SIGNAL_RESIGTE;

	SlotFunBase*  slotFun = holder->getSlotFun( slotName );

	if ( !slotFun )
		return CONNECT_FAIL_NO_SOLT_RESIGTE;

	if ( info->type != slotFun->type && slotFun->type != SLOT_VOID )
		return CONNECT_FAIL_SLOT_TYPE_ERROR;

	int result = TEventSystem::instance().connectSignal( holder  , info->type , sender , info->id , slotFun );
	return result;
}

void SignalManager::disconnect( TRefObj* sender , char const* signalName , TRefObj* holder , char const* slotName )
{
	SignalInfo* info  = getSignalInfo(  signalName );

	assert( info );

	SlotFunBase*  slotFun = holder->getSlotFun( slotName );

	assert( slotFun );

	TEventSystem::instance().disconnectSignal( holder  , info->type , sender , info->id , slotFun );
}

int SignalManager::resigter( char const* name , int soltType )
{
	int id = resigterID;
	++resigterID;

	SignalInfo info;
	info.id   = id;
	info.type = (EventType)( soltType );

	m_signalMap.insert( std::make_pair( name , info ) );
	return id;
}

SignalInfo* SignalManager::getSignalInfo(char const* name )
{

	SignalMap::iterator nameIter = m_signalMap.find( name );

	if ( nameIter == m_signalMap.end() )
		return NULL;

	return &nameIter->second;
}

namespace SignalSpace{

int getSlotList( SlotMap& map , char const** name , int maxNum )
{
	int result = 0;
	for ( SlotMap::iterator iter = map.begin() ; iter != map.end(); ++iter )
	{
		if ( result == maxNum )
			break;

		name[ result ] = iter->first;
		++result;
	}
	return result;
}

SlotFunBase* getSlotFun( SlotMap& map , char const* name )
{
	SlotMap::iterator iter = map.find( name );
	if ( iter != map.end() )
		return iter->second;
	return NULL;
}

int  getSignalList( char const** signalStr , int num , char const** name , int maxNum )
{
	int result = 0;
	for(  ; result < num ; ++result )
	{
		if ( result >= maxNum )
			break;

		name[ result ] = signalStr[ result ];
	}
	return result;
}

} //namespace SignalSpace
