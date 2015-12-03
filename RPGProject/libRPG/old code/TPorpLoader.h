#ifndef TPorpLoader_h__
#define TPorpLoader_h__

#include <typeinfo>
#include "FunCallback.h"
#include <boost/type_traits/is_same.hpp>
#include <boost/type_traits/is_enum.hpp>


namespace PropSpace{

	typedef ::boost::true_type  true_type;
	typedef ::boost::false_type false_type;

	template < class T , class P , class Q >
	struct Selector
	{
		typedef P result_type;
	};
	template < class P , class Q >
	struct Selector< false_type , P , Q >
	{
		typedef Q result_type;
	};

	template < class P , class T1 , class T2 ,class T3 >
	struct Selector< false_type , P , Selector< T1 , T2 , T3 > >
	{
		typedef typename Selector<T1,T2,T3>::result_type result_type;
	};

template < class VType >
class TPropBase
{
public:
	virtual ~TPropBase(){}
	virtual void  setVal( VType& val ) = 0;
	virtual void  getVal( VType& val ) = 0;
};

template < class VType , class Obj , class TypeTrait >
class TPropMemVar : public TPropBase< VType >
{
public:
	typedef typename TypeTrait::Type     Type;
	typedef typename TypeTrait::CastType CastType;
	typedef Type (Obj::*MemPtr);
	TPropMemVar( Obj* o  , MemPtr ptr )
		:memptr(ptr) , obj(o){}

	virtual void  setVal(VType& val)
	{
		doSetVal( val , boost::is_same< Type , CastType >::type() );
	}

	void doSetVal( VType& val , true_type )
	{
		val.getVal( obj->*memptr );
	}

	void doSetVal( VType& val , false_type )
	{
		CastType castVal;
		val.getVal( castVal );
		obj->*memptr = ( Type ) castVal;
	}

	virtual void  getVal(VType& val)
	{
		doGetVal( val , boost::is_same< Type , CastType>::type() );
	}

	void doGetVal( VType& val , true_type )
	{
		val.setVal( (obj->*memptr) );
	}

	void doGetVal( VType& val , false_type )
	{
		CastType castVal = (CastType)( obj->*memptr );
		val.setVal( castVal );
	}
	MemPtr  memptr;
	Obj*    obj;
};

template < class VType , class Obj , class TypeTrait , class SetFun , class GetFun >
class TPropMemFun : public TPropBase< VType >
{
public:
	typedef typename TypeTrait::Type     Type;
	typedef typename TypeTrait::CastType CastType;
	TPropMemFun(  Obj* o, SetFun sf , GetFun gf )
		:sfun( sf ) , gfun( gf ), obj(o){}

	virtual void  setVal(VType& val)
	{
		val.getVal( m_val );
		(obj->*sfun)( (Type) m_val  );
	}

	virtual void  getVal(VType& val)
	{
		m_val = (CastType) (obj->*gfun)();
		val.setVal( m_val );
	}

	CastType   m_val;
	SetFun     sfun;
	GetFun     gfun;
	Obj*       obj;
};


//template < class VType >
//class TPropVar : public TPropBase< VType >
//{
//public:
//	virtual void  setVal(VType& val);
//	virtual void  getVal(VType& val);
//	T* var;
//};

//class TPropFun : public TPropBase
//{
//
//};


//class TPropIO
//{
//public:
//	class PropIDType;
//	class VType;
//
//	void getVal( PropIDType& id , TPropBase<VType>* prop );
//	void setVal( PropIDType& id , TPropBase<VType>* prop );
//  void clearProp();
//
//	template < class T >
//	PropIDType  append( wxString const& name , T* );
//
//	void addClass( char const* name );
//};


template < class T >
struct NormalTypeTrait
{
	typedef T  Type;
	typedef T  CastType;
};

template < class T >
struct EnumTypeTrait
{
	typedef T   Type;
	typedef int CastType;
};

template < class TPropIO >
class TPropLoader : public TPropIO
{
public:
	typedef typename TPropIO::PropIDType PropIDType;
	typedef typename TPropIO::VType      VType;

	struct PropData
	{
		PropIDType id;
		TPropBase<VType>*  prop;
	};

	void changePropID( char const* name , PropIDType id )
	{
		PropMap::iterator iter = m_propMap.find( name );
		if ( iter != m_propMap.end() )
			iter->second.id = id;
	}

	template < class Obj , class Type >
	void addMemVar( char const* name , Obj* o  , Type (Obj::*MemPtr) )
	{
		typedef typename 
		Selector< boost::is_enum< Type >::type , EnumTypeTrait< Type > ,
			NormalTypeTrait< Type >  >::result_type TypeTrait;
		PropData data;
		data.prop = new TPropMemVar< VType , Obj , TypeTrait >( o , MemPtr );
		data.id   = append(  name , TypeTrait() );
		m_propMap.insert( std::make_pair( name , data ) );
	}

	template < class Obj , class SetFun , class GetFun , class Helper >
	void addMemFun( char const* name  , Obj* o, SetFun sf , GetFun gf , Helper )
	{

		typedef typename Helper::BaseType Type;

		typedef typename 
			Selector< boost::is_enum< Type >::type , EnumTypeTrait< Type > ,
			NormalTypeTrait< Type >  >::result_type TypeTrait;

		PropData data;
		data.prop = new TPropMemFun< VType , Obj , TypeTrait , SetFun , GetFun >( o , sf , gf );
		data.id   = append( name , TypeTrait() );
		
		m_propMap.insert( std::make_pair( name , data ) );
	}


	template < class Obj , class SetFun , class GetFun , class Helper >
	void addEnumFlagMemFun( char const* name  , Obj* o, SetFun sf , GetFun gf , Helper )
	{
		typedef typename Helper::BaseType Type;

		typedef typename Selector< boost::is_enum< Type >::type , EnumTypeTrait< Type > ,
			     NormalTypeTrait< Type >  >::result_type TypeTrait;
		PropData data;
		data.prop = new TPropMemFun< VType , Obj , TypeTrait , SetFun , GetFun >( o , sf , gf );
		data.id   = appendEnumFlag( name , (Type*)0);

		m_propMap.insert( std::make_pair( name , data ) );
	}

	template <class T>
	void load( T* obj )
	{
		TPropCollector<T>::collect( *this , obj );
	}
	void input( char const* name )
	{
		PropData* data = getPropData( name );
		if ( data )
			getVal( data->id , data->prop );
	}

	void removeAll()
	{
		TPropIO::clearProp();

		for ( PropMap::iterator iter = m_propMap.begin();
			iter != m_propMap.end() ; ++iter )
		{
			PropData& data = iter->second;
			delete data.prop;
		}

		m_propMap.clear();

	}

	void output( char const* name )
	{
		PropData* data = getPropData( name );
		if ( data )
			setVal( data->id , data->prop );
	}

	void outputAll()
	{
		for ( PropMap::iterator iter = m_propMap.begin();
			  iter != m_propMap.end() ; ++iter )
		{
			PropData& data = iter->second;
			setVal( data.id , data.prop );
		}
	}

	void inputAll()
	{
		for ( PropMap::iterator iter = m_propMap.begin();
			iter != m_propMap.end() ; ++iter )
		{
			PropData& data = iter->second;
			getVal( data.id , data.prop );
		}
	}
	
	PropData* getPropData( char const* name )
	{
		PropMap::iterator iter = m_propMap.find( name );

		if ( iter != m_propMap.end() )
			return &iter->second;
		return NULL;
	}

	typedef std::map< TString , PropData  > PropMap;

	PropMap m_propMap;
};

}//namespace PropSpace

template< class T >
class TPropCollector {};

#define ACCESS_PROP_COLLECT()\
	template<class T>\
	friend class TPropCollector;

#define DEFINE_PROP_COLLECTOR( Class )\
	template<>\
	class TPropCollector< Class >\
	{\
	public:\
		template < class Loader >\
		static void collect( Loader& loader , Class* obj ){

#define BEGIN_PROP_CLASS_NOBASE( Class )\
	DEFINE_PROP_COLLECTOR( Class )\
		loader.addClass( #Class );

#define END_PROP_CLASS() } };

#define BEGIN_PROP_CLASS( Class , BaseClass ) \
	DEFINE_PROP_COLLECTOR( Class )\
		TPropCollector< BaseClass >::collect( loader , obj );\
		loader.addClass( #Class );
		
#define PROP_MEMBER( Name , Var )\
	loader.addMemVar( Name  , obj , &Var );

#define PROP_MEMFUN( Name , Type  , SetFun , GetFun )\
	loader.addMemFun( Name  , obj , &SetFun  , &GetFun , detail::TypeTraits<Type>() );

#define PROP_ENUM_FLAG_MEMFUN( Name  , EnumClass , SetFun , GetFun )\
	loader.addEnumFlagMemFun(  Name  , obj , &SetFun  , &GetFun  , detail::TypeTraits<EnumClass>() );

#define PROP_VAR( Type , Name , Var )
#define DEFINE_PROP_FUN( Type , Name , Fun )


#define PROP_SUB_CLASS_PTR( SubClass , var )\
	TPropCollector< SubClass >::collect( loader , obj->var );

#define PROP_SUB_CLASS( SubClass , var )\
	TPropCollector< SubClass >::collect( loader , &obj->var );

struct EnumTypeData
{
	int  val;
	char const* str;
};

#define BEGIN_ENUM_DATA( EnumType )\
	template<>\
	class TPropCollector< EnumType  >\
	{\
	public:\
		template < class Loader >\
		static void collect( Loader& loader , EnumType* obj )\
		{\
			loader.addMemVar( #EnumType  , obj , (EnumType*)0 );\
		}\
		static EnumTypeData* getEnumData( size_t& size )\
		{\
			static EnumTypeData enumData[] = {
	
#define END_ENUM_DATA() };\
			size = ARRAY_SIZE( enumData );\
			return enumData;\
		}\
	};

#define ENUM_DATA( val ) { val , #val } ,
#define GET_PROP_ENUM_DATA( EnumType , size )\
	TPropCollector< EnumType >::getEnumData( size )


//BEGIN_PROP_CLASS_NOBASE( FyVec3D )
//	PROP_MEMBER(  "x" , FyVec3D::x )
//	PROP_MEMBER(  "y" , FyVec3D::y )
//	PROP_MEMBER(  "z" , FyVec3D::z )
//END_PROP_CLASS()
//
//BEGIN_PROP_CLASS_NOBASE( TVec2D )
//	PROP_MEMBER( "px" , TVec2D::x )
//	PROP_MEMBER( "py" , TVec2D::y )
//END_PROP_CLASS()
//



#endif // TPorpLoader_h__