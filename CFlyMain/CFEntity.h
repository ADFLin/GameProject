#ifndef CFEntity_h__
#define CFEntity_h__

#include "CFBase.h"

#include <vector>

#define CF_ERROR_ID ((unsigned)-1)

//  31              15         3    0
//  |----------------|---------|----|
//      serial         index    type

#define CF_ENTITY_TYPE_BIT_NUM   4
#define CF_ENTITY_TYPE_MAX       ( 1 << CF_ENTITY_TYPE_BIT_NUM )
#define CF_ENTITY_TYPE_MASK      ( CF_ENTITY_TYPE_MAX - 1 )

#define CF_ENTITY_INDEX_BIT_NUM  16
#define CF_ENTITY_INDEX_MAX      ( 1 << CF_ENTITY_INDEX_BIT_NUM )

#define CF_ENTITY_SERIAL_BIT_NUM  ( 32 - CF_ENTITY_INDEX_BIT_NUM - CF_ENTITY_TYPE_BIT_NUM )
#define CF_ENTITY_SERIAL_MAX      ( 1 << CF_ENTITY_SERIAL_BIT_NUM )

namespace CFly
{

	enum EntityType
	{
		ET_WORLD  ,
		ET_SCENE  ,
		ET_VIEWPORT ,
		ET_OBJECT ,
		ET_SPRITE ,
		ET_ACTOR ,
		ET_CAMERA ,
		ET_TEXTURE ,
		ET_MATERIAL,
		ET_LIGHT ,
		ET_SHADER ,
	};

	class Entity
	{
	public:
		Entity():m_id( CF_ERROR_ID ){}
		~Entity();
		unsigned getEntityID() const { return m_id; }
	private:
		void     setEntityID( unsigned id ){ m_id = id; }
		friend class EntityManger;
		unsigned m_id;

	};

	template< class T >
	struct EntityTypeMap {};





#define DEFINE_ENTITY_TYPE( type , typeID )\
	template<>\
	struct EntityTypeMap< type >{ enum { value = typeID }; };


	class EntityManger : public Singleton< EntityManger >
	{
	public:

		struct ID
		{
			ID(){}
			ID( unsigned id ):value( id ){}
			union
			{
				unsigned value;
				struct  
				{
					unsigned type   : CF_ENTITY_TYPE_BIT_NUM;
					unsigned index  : CF_ENTITY_INDEX_BIT_NUM;
					unsigned serial : CF_ENTITY_SERIAL_BIT_NUM;	
				};
			};	
		};

		EntityManger();

		static unsigned getIndex ( unsigned id ){ ID temp(id); return temp.index;  }
		static unsigned getSerial( unsigned id ){ ID temp(id); return temp.serial;  }
		static unsigned getType( unsigned id )  { ID temp(id); return temp.type;  }
		static unsigned calcEntityID( unsigned type , unsigned idx , unsigned serial );

		template< class T >
		unsigned registerEntity( T* entity )
		{
			return markEntity( entity , EntityTypeMap< T >::value );
		}
		bool    removeEntity( Entity* entity );
		Entity* extractEntity( unsigned id , unsigned type );
		Entity* extractEntityBits( unsigned id , unsigned typeBits );
		bool    checkType( Entity* entity , unsigned type );
	protected:

		unsigned markEntity( Entity* entity , unsigned type );

		unsigned m_curSerialNum;
		struct Slot_t
		{
			Entity*   entity;
			unsigned   serialNum;
		};
		std::vector< Slot_t >    m_SoltVec;
		std::vector< unsigned >  mUnusedSlotIDVec;

		template< class T >
		friend T*  entity_cast( Entity* entity );

	};

	template <class T >
	struct UnpackPtr
	{
		typedef T result_type;
	};

	template < class T >
	struct UnpackPtr< T* >
	{
		typedef T result_type;
	};

	template< class T >
	T* entity_cast( Entity* entity )
	{
		if ( !entity )
			return nullptr;
	
		if ( !EntityManger::getInstance().checkType( 
				entity ,
				EntityTypeMap< T >::value ) )
			return nullptr;

		return static_cast< T* >( entity );
	}


}//namespace CFly

#endif // CFEntity_h__