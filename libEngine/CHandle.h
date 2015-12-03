#ifndef CHandle_h__
#define CHandle_h__

#define MAX_ENTITY_NUM_BIT 10
#define MAX_ENTITY_NUM     ( 1 << MAX_ENTITY_NUM_BIT )
#define ENTITY_INDEX_MASK  (MAX_ENTITY_NUM - 1)
#define MAX_SERIAL_NUM     ( 0xffffffff >> MAX_ENTITY_NUM_BIT )
#define ERROR_ID           0xffffffff


class TRefObj;

template< class T >
class CHandle
{
public:
	typedef CHandle< TRefObj >    RefHandle;

	CHandle(){  m_ID = ERROR_ID; }
	CHandle( unsigned id ){	 m_ID = id;  }
	CHandle( unsigned index , unsigned serial )
	{
		m_ID = ( serial << MAX_ENTITY_NUM_BIT ) + index;  
	}

	unsigned getEntityIndex(){ return m_ID & ENTITY_INDEX_MASK ; }
	unsigned getSerialNum(){ return m_ID >> MAX_ENTITY_NUM_BIT; }
	unsigned getID(){ return m_ID; }

	CHandle& operator = ( T* entity ){  return set( entity);	 }
	CHandle& operator = ( CHandle& handle ){	m_ID = handle.m_ID;  return *this;	}
	bool     operator == ( T*  entity ){	return  get() == entity;  }
	bool     operator != ( T*  entity ){	return  get() != entity;  }

	operator RefHandle(){ return RefHandle( m_ID ); }
	operator T*(){ return get(); }
	T* operator->(){ return get(); }
	
	T*       get();
	CHandle& set( T* entity );

	void setID( unsigned id ){ m_ID = id; }


	template< class T2 >
	bool operator ==( CHandle<T2>& handle ){  return  m_ID == handle.m_ID;  }
	template< class T2 >
	bool operator !=( CHandle<T2>& handle ){  return  m_ID != handle.m_ID;  }
	template< class T2 >
	bool operator ==( T2*  entity ){	return  get() == entity;  }
	template< class T2 >
	bool operator !=( T2*  entity ){	return  get() != entity;  }

	//template< class T2>
	//CHandle operator=( CHandle< T2 >& handle );


protected:
	unsigned m_ID;
};

typedef CHandle< TRefObj >    RefHandle;

struct TEvent;
enum   EventType;
typedef void (TRefObj::*FnMemEvent)();
typedef void (*FnEvent)();

class SlotFunBase;

class TRefObj
{
public:
	TRefObj(){}
	//cast type need
	virtual ~TRefObj();
	RefHandle    getRefHandle(){ return m_refHandle; }
	void         setRefHandle(RefHandle val) { m_refHandle = val; }
	unsigned int getRefID();

protected:
	RefHandle   m_refHandle;

public:
	//Event System
	virtual void onEvent( TEvent& event ){}
	int          listenEvent( EventType type , int id );
	int          listenEvent( EventType type , int id , FnEvent callback );
	int          listenEvent( EventType type , int id , FnMemEvent callback );
	//data don't use local var
	void         sendEvent( EventType type , void* data, bool destoryData );
	virtual int  getSignalList( char const** name , int maxNum ){ return 0; }
	virtual int  getSlotList( char const** name , int maxNum ){ return 0; }
	virtual SlotFunBase* getSlotFun( char const*name ){ return 0; }
};


#include "Singleton.h"
#include <vector>

class THandleManager : public SingletonT< THandleManager >
{
public:
	TRefObj*     extractHandle( RefHandle handle );
	unsigned     markRefHandle( TRefObj* entity );
	void         unMarkRefHandle( TRefObj* entity );

protected:

	static unsigned s_CurSerialNum;
	struct Slot_t
	{
		TRefObj*   entity;
		unsigned   serialNum;
	};
	std::vector< Slot_t >    m_SoltVec;
	std::vector< unsigned >  m_UnUsedSlotIDVec;

};

#define DEFINE_HANDLE( type )\
template<>\
type* CHandle< type >::get()\
{\
	return (type*) THandleManager::instance().extractHandle( *this );\
}\
template<>\
CHandle<type>& CHandle<type>::set( type* entity )\
{\
	if ( entity )\
	{\
		unsigned ID = entity->getRefHandle().getID();\
		if ( ID == ERROR_ID )\
		{\
			ID = THandleManager::instance().markRefHandle( entity );\
		}\
		m_ID = ID;\
	}\
	else { m_ID = ERROR_ID; }\
	return *this;\
}

#endif // CHandle_h__