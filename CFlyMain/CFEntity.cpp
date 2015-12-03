#include "CFlyPCH.h"
#include "CFEntity.h"

namespace CFly
{
	EntityManger::EntityManger()
	{
		m_curSerialNum = 0;
		m_SoltVec.reserve( 64 );
	}

	bool EntityManger::checkType( Entity* entity , unsigned type )
	{
		unsigned id = entity->getEntityID();
		return type == getType( id );
	}

	Entity* EntityManger::extractEntity( unsigned id , unsigned type )
	{
		ID temp( id );
		if ( type != getType( id ) )
			return NULL;

		unsigned idx = getIndex( id );

		if ( idx >= m_SoltVec.size() )
			return NULL;

		Slot_t& slot = m_SoltVec[ idx ];

		if ( slot.serialNum != getSerial( id ) )
			return NULL;

		return slot.entity;
	}


	Entity* EntityManger::extractEntityBits( unsigned id , unsigned typeBits )
	{
		if ( typeBits &= (  1 << getType( id ) ) )
			return NULL;

		unsigned idx = getIndex( id );

		if ( idx >= m_SoltVec.size() )
			return NULL;

		Slot_t& slot = m_SoltVec[ idx ];

		if ( slot.serialNum != getSerial( id ) )
			return NULL;

		return slot.entity;
	}


	bool EntityManger::removeEntity( Entity* entity )
	{
		unsigned id = entity->getEntityID();
		if ( id == CF_ERROR_ID )
			return false;

		unsigned index = getIndex( id );
		Slot_t& slot = m_SoltVec[ index ];

		if ( entity != slot.entity )
			return false;

		slot.entity = NULL;
		//entity->setRefHandle( RefHandle() );
		mUnusedSlotIDVec.push_back( index );

		entity->setEntityID( CF_ERROR_ID );
		return true;
	}

	unsigned EntityManger::markEntity( Entity* entity , unsigned type )
	{
		unsigned slotIndex;
		Slot_t* pSlot;
		if ( !mUnusedSlotIDVec.empty() )
		{
			slotIndex = mUnusedSlotIDVec.back();
			mUnusedSlotIDVec.pop_back();

			pSlot = &m_SoltVec[ slotIndex ];

			if ( pSlot->serialNum == m_curSerialNum )
			{
				m_curSerialNum = ( m_curSerialNum + 1 ) % CF_ENTITY_SERIAL_MAX; 
			}

			pSlot->entity = entity;
			pSlot->serialNum = m_curSerialNum;

			m_curSerialNum = ( m_curSerialNum + 1 ) % CF_ENTITY_SERIAL_MAX;
		}
		else
		{
			slotIndex = m_SoltVec.size();
			assert(  slotIndex < CF_ENTITY_INDEX_MAX  );
			Slot_t slot;
			slot.entity = entity;
			slot.serialNum = m_curSerialNum;
			m_curSerialNum = ( m_curSerialNum + 1 ) % CF_ENTITY_SERIAL_MAX;

			m_SoltVec.push_back( slot );
			pSlot = &m_SoltVec.back();
		}


		unsigned id = calcEntityID( type , slotIndex , pSlot->serialNum );
		entity->setEntityID( id );

		return id;
	}

	unsigned EntityManger::calcEntityID( unsigned type , unsigned idx , unsigned serial )
	{
		ID temp;
		temp.type   = type;
		temp.index  = idx;
		temp.serial = serial;
		return temp.value;
	}

	Entity::~Entity()
	{
		EntityManger::getInstance().removeEntity( this );
	}

}//namespace CFly
