#include "ProjectPCH.h"
#include "CHandle.h"

#include "TEventSystem.h"

#include <cassert>

unsigned HandleManager::s_CurSerialNum = 0;

DEFINE_HANDLE( HandledObject )

HandledObject::~HandledObject()
{
	HandleManager::Get().unMarkRefHandle( this );
}

unsigned int HandledObject::getRefID()
{
	if ( m_refHandle.getID() == ERROR_ID )
		return HandleManager::Get().markRefHandle( this );
	return m_refHandle.getID();
}

//int TRefObj::listenEvent( EventType type , int id )
//{
//	return TEventSystem::instance().connectEvent( this , type , id );
//}
//
//int TRefObj::listenEvent( EventType type , int id , FnEvent callback )
//{
//	return TEventSystem::instance().connectEvent( this , type , id , callback );
//}
//
//int TRefObj::listenEvent( EventType type , int id , FnMemEvent callback )
//{
//	return TEventSystem::instance().connectEvent( this , type , id , callback );
//}
//
//void TRefObj::sendEvent( EventType type , void* data, bool destoryData )
//{
//	TEvent event( type , m_refHandle.getID() , this , data , destoryData );
//	TEventSystem::instance().addEvent( event );
//}


unsigned HandleManager::markRefHandle( HandledObject* entity )
{
	unsigned slotIndex;
	Slot_t* pSlot;
	if ( m_UnUsedSlotIDVec.size() != 0 )
	{
		slotIndex = m_UnUsedSlotIDVec.back();
		m_UnUsedSlotIDVec.pop_back();

		pSlot = &m_SoltVec[ slotIndex ];
		pSlot->entity = entity;
		if ( pSlot->serialNum == s_CurSerialNum )
		{
			s_CurSerialNum = ( s_CurSerialNum + 1 ) % MAX_SERIAL_NUM; 
		}
		pSlot->serialNum = s_CurSerialNum;
		s_CurSerialNum = ( s_CurSerialNum + 1 ) % MAX_SERIAL_NUM;
	}
	else
	{
		slotIndex = m_SoltVec.size();
		assert(  slotIndex < MAX_ENTITY_NUM  );
		Slot_t slot;
		slot.entity = entity;
		slot.serialNum = s_CurSerialNum;
		s_CurSerialNum = ( s_CurSerialNum + 1 ) % MAX_SERIAL_NUM;
		m_SoltVec.push_back( slot );
		pSlot = &m_SoltVec.back();
	}

	RefHandle handle( slotIndex , pSlot->serialNum );
	entity->setRefHandle( handle );

	return handle.getID();
}

HandledObject* HandleManager::extractHandle( RefHandle handle )
{
	if ( ERROR_ID == handle.getID() )
		return NULL;

	unsigned index = handle.getEntityIndex();

	Slot_t& slot = m_SoltVec[ index ];

	if ( slot.serialNum == handle.getSerialNum() )
	{
		return slot.entity;
	}
	else
	{
		return NULL;
	}
}

void HandleManager::unMarkRefHandle( HandledObject* entity )
{
	RefHandle handle = entity->getRefHandle();

	if ( handle.getID() == ERROR_ID )
		return;

	unsigned index = handle.getEntityIndex();

	Slot_t& slot = m_SoltVec[ index ];

	assert( entity == slot.entity );

	slot.entity = NULL;
	entity->setRefHandle( RefHandle() );

	m_UnUsedSlotIDVec.push_back( index );
}

