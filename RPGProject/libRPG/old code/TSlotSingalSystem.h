#ifndef TSlotSingalSystem_h__
#define TSlotSingalSystem_h__

#include "TEventSystem.h"
#include "EventType.h"

enum 
{
	SLOT_VOID = 1,
	SLOT_BOOL,
	SLOT_INT,
	SLOT_UNKNOW,
};

class TLogicObj;

template < class T >
struct EvalEvtType
{	enum { result_value = SLOT_UNKNOW }; };

template <>
struct EvalEvtType< int >
{	enum { result_value = SLOT_INT }; };

template <>
struct EvalEvtType< void >
{	enum { result_value = SLOT_VOID }; };

template <>
struct EvalEvtType< bool >
{	enum { result_value = SLOT_BOOL }; };


class SlotFunBase
{
public:
	virtual void exec( TRefObj* obj , void* data ) = 0;
	int type;


	template< class C , class T >
	static SlotFunBase* makeSlot( void (C::*fnSlot)(T) )
	{
		typedef void (TRefObj::*FunType)( T );
		return new SlotFun< T >( ( FunType) fnSlot );
	}
	template< class C >
	static SlotFunBase* makeSlot( void (C::*fnSlot)() )
	{
		typedef void (TRefObj::*FunType)();
		return new SlotFun< void >( (FunType) fnSlot );
	}
};

template < class T >
class SlotFun : public SlotFunBase
{
public:
	typedef void (TRefObj::*FnSlot)(T);
	SlotFun( FnSlot fun )
		:fun(fun){ type = EvalEvtType<T>::result_value; }

	FnSlot fun;

	void exec( TRefObj* obj , void* data )
	{
		(obj->*fun)( *((T*)data) );
	}	
};

template<>
class SlotFun< void > : public SlotFunBase
{
public:
	typedef void (TRefObj::*FnSlot)();
	SlotFun( FnSlot fun )
		:fun(fun){ type = EvalEvtType<void>::result_value; }
	void exec( TRefObj* obj , void* data )
	{
		(obj->*fun)();
	}
	FnSlot fun;
};



template < class T >
struct SignalNameSpace;

template < class T >
struct SlotNameSpace;

struct StrCmp
{		
	bool operator()( char const* s1 , char const* s2 ) const
	{
		return strcmp( s1 , s2) < 0;
	}
};

typedef std::map< char const* , SlotFunBase* , StrCmp > SlotMap;

namespace SignalSpace{

int getSlotList( SlotMap& map , char const** name , int maxNum );
SlotFunBase* getSlotFun( SlotMap& map , char const* name );
int getSignalList( char const** signalStr , int num , char const** name , int maxNum );

}//namespace SlotSpace


#define SLOT_CLASS( Class )\
	public:\
		SlotFunBase* getSlotFun( char const* name );\
		int          getSlotList( char const** name , int maxNum );

#define BEGIN_SLOT_MAP_NOBASE( Class )\
	template<>\
	struct SlotNameSpace< Class >\
	{\
		typedef Class SlotClass;\
		static SlotMap&  getSlotMap(){  static SlotMap slotMap; return slotMap; }\
		static SlotFunBase* getSlotFun( char const* name )\
		{  return SignalSpace::getSlotFun( getSlotMap() , name );  }\
		static int getSlotList( char const** name , int maxNum )\
		{  return SignalSpace::getSlotList( getSlotMap() , name , maxNum );  }\
		static struct SlotMapIntiHelper\
		{\
			SlotMapIntiHelper()\
			{\

#define BEGIN_SLOT_MAP( Class , BaseClass )\
	template<>\
	struct SlotNameSpace< Class >\
	{\
		typedef Class SlotClass;\
		typedef std::map< char const* , SlotFunBase* , StrCmp > SlotMap;\
		static SlotMap&  getSlotMap(){  static SlotMap slotMap; return slotMap; }\
		static SlotFunBase* getSlotFun( char const* name )\
		{\
			SlotFunBase* slot = SignalSpace::getSlotList( getSlotMap() , name , maxNum );\
			if ( slot )  return slot;\
			return SlotNameSpace< BaseClass >::getSlotFun( name );\
		}\
		static int getSlotList( char const** name , int maxNum )\
		{\
			int num = SlotNameSpace< BaseClass >::getSlotList( name , maxNum );\
			return SignalSpace::getSlotList( getSlotMap() , name + num , maxNum - num );\
		}\
		static struct SlotMapIntiHelper\
		{\
			SlotMapIntiHelper()\
			{\

#define DEF_SLOT( Fun )\
				getSlotMap().insert( std::make_pair( #Fun , SlotFunBase::makeSlot( &SlotClass::Fun ) ) );

#define END_SLOT_MAP( Class )\
			}\
		}helper;\
	};\
	SlotNameSpace< Class  >::SlotMapIntiHelper SlotNameSpace< TLogicObj >::helper;\
	SlotFunBase* Class::getSlotFun( char const* name )\
	{  return SlotNameSpace< Class >::getSlotFun( name );  }\
	int  Class::getSlotList( char const** name , int maxNum )\
	{  return SlotNameSpace< Class >::getSlotList( name , maxNum );  }

#define SINGNAL_CLASS( Class )\
	private:\
		typedef Class SignalClass;\
	public:\
		int     getSignalList( char const** name , int maxNum );
	

#define BEGIN_SIGNAL_MAP_NOBASE( Class )\
	template<>\
	class SignalNameSpace< Class >\
	{\
	public:\
		static char const* signalStr[];\
		static int  const  signalNum;\
		static int  getSignalList( char const** name , int maxNum )\
		{  return SignalSpace::getSignalList( signalStr , signalNum , name , maxNum );  }\
	};\
	char const* SignalNameSpace< Class >::signalStr[] = \
	{\

#define DEF_SIGNAL( name )  #name ,

#define END_SIGNAL_MAP( Class )\
	};\
	int const SignalNameSpace< Class >::signalNum = ARRAY_SIZE( SignalNameSpace< Class >::signalStr );\
	int    Class::getSignalList( char const** name , int maxNum )\
	{  return SignalNameSpace< Class >::getSignalList( name  , maxNum );  }


#define SIGNAL( type , name )\
	static TSignal< type >&  getSignal_##name()\
	{ static TSignal< type > sig(#name); return sig; }

#define INIT_SIGNAL( name )\
	getSignal_##name();

#define SEND_SIGNAL( name , data )\
	getSignal_##name().send( this , data );
#define SEND_SIGNAL_VOID( name )\
	getSignal_##name().send( this );


#define SIGNAL_ERROR_ID (~(unsigned)0 )
template < class T >
class TSignal
{
public:
	TSignal( char const* name )
	{
		id = SignalManager::instance().resigter( 
			name , EvalEvtType< T>::result_value );
	}

	~TSignal(){}
	void send( TRefObj* sender , T val )
	{
		data = val;
		SignalManager::instance().addSignal(
			EvalEvtType< T>::result_value	, id , sender , &data  );
	}

	T    data;
	int  id;
};

template <>
class TSignal<void>
{
public:
	TSignal( char const* name );
	void send( TRefObj* sender );
	int  id;
};

struct SignalInfo
{
	int         id;
	EventType   type;
};


typedef std::map< char const* , SignalInfo , StrCmp > SignalMap;

class SignalManager : public Singleton<SignalManager>
{
public:
	SignalManager()
	{
		resigterID = 0;
	}

	int  connect( TRefObj* sender , char const* signalName , TRefObj* holder , char const* slotName );
	void disconnect( TRefObj* sender , char const* signalName , TRefObj* holder , char const* slotName );
	void addSignal( int slotType , int id ,TRefObj* obj , void* data )
	{
		TEvent event( (EventType)(EVT_SIGNAL + slotType), id , obj , &data );
		TEventSystem::instance().addEvent( event );
	}

	SignalInfo* getSignalInfo(char const* name );
	int         resigter( char const* name , int soltType );

protected:

	int  resigterID;
	SignalMap    m_signalMap;
};

#endif // TSlotSingalSystem_h__