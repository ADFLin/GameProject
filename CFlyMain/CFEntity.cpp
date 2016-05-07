#include "CFlyPCH.h"
#include "CFEntity.h"

namespace CFly
{
	EntityManager::EntityManager()
	{
		mCurSerialNum = 0;
		m_SoltVec.reserve( 256 );
	}

	bool EntityManager::checkType( Entity* entity , unsigned type )
	{
		uint32 id = entity->getEntityID();
		return type == getType( id );
	}

	Entity* EntityManager::extractEntity( uint32 id , unsigned type )
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


	Entity* EntityManager::extractEntityBits( uint32 id , unsigned typeBits )
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


	bool EntityManager::removeEntity( Entity* entity )
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
		mUnusedSlotIndices.push_back( index );

		entity->setEntityID( CF_ERROR_ID );
		return true;
	}

	unsigned EntityManager::markEntity( Entity* entity , unsigned type )
	{
		unsigned slotIndex;
		Slot_t* pSlot;
		if ( !mUnusedSlotIndices.empty() )
		{
			slotIndex = mUnusedSlotIndices.back();
			mUnusedSlotIndices.pop_back();

			pSlot = &m_SoltVec[ slotIndex ];

			if ( pSlot->serialNum == mCurSerialNum )
			{
				mCurSerialNum = ( mCurSerialNum + 1 ) % CF_ENTITY_SERIAL_MAX; 
			}

			pSlot->entity = entity;
			pSlot->serialNum = mCurSerialNum;

			mCurSerialNum = ( mCurSerialNum + 1 ) % CF_ENTITY_SERIAL_MAX;
		}
		else
		{
			slotIndex = m_SoltVec.size();
			assert(  slotIndex < CF_ENTITY_INDEX_MAX  );
			Slot_t slot;
			slot.entity = entity;
			slot.serialNum = mCurSerialNum;
			mCurSerialNum = ( mCurSerialNum + 1 ) % CF_ENTITY_SERIAL_MAX;

			m_SoltVec.push_back( slot );
			pSlot = &m_SoltVec.back();
		}


		unsigned id = calcEntityID( type , slotIndex , pSlot->serialNum );
		entity->setEntityID( id );

		return id;
	}

	uint32 EntityManager::calcEntityID( unsigned type , unsigned idx , unsigned serial )
	{
		ID temp;
		temp.type   = type;
		temp.index  = idx;
		temp.serial = serial;
		return temp.value;
	}

	Entity::~Entity()
	{
		EntityManager::getInstance().removeEntity( this );
	}

}//namespace CFly
