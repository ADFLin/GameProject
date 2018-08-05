#ifndef FunCallback_h__
#define FunCallback_h__

#include "MetaTypeList.h"

namespace detail
{


template< class T >
struct TypeTraits
{
	typedef T  BaseType;
	typedef T& RefType;
	typedef T* PtrType;
};

template < class T >
struct TypeTraits< T& >
{
	typedef T  BaseType;
	typedef T& RefType;
	typedef T* PtrType;
};

template < class T >
struct TypeTraits< T const& >
{
	typedef T  BaseType;
	typedef T& RefType;
	typedef T* PtrType;
};

template< class T>
struct TypeToParam;


class MemFunType{};
class BaseFunType{};

template< class FunSig >
struct FunTraits;

template < class RT >
struct FunTraits< RT (*)() >
{
	typedef BaseFunType FunType;
	enum { NumParam = 0 };
	static char const** getParam()
	{
		return NULL;
	}

};

template < class RT , class P1 >
struct FunTraits< RT (*)(P1) >
{
	typedef BaseFunType FunType;
	enum { NumParam = 1 };
	typedef typename TypeTraits<P1>::BaseType Param1;

	static char const** getParam()
	{
		static char const* s_param[] =
		{
			TypeToParam<Param1>::getTypeParam() ,
		};
		return &s_param[0];
	}
};

template < class RT , class P1 , class P2 >
struct FunTraits< RT (*)(P1 , P2) >
{
	typedef BaseFunType FunType;
	enum { NumParam = 2 };
	typedef typename TypeTraits<P1>::BaseType Param1;
	typedef typename TypeTraits<P2>::BaseType Param2;

	static char const** getParam()
	{
		static char const* s_param[] =
		{
			TypeToParam<Param1>::getTypeParam() ,
			TypeToParam<Param2>::getTypeParam()
		};
		return &s_param[0];
	}
};



template < class RT , class P1 , class P2 , class P3 >
struct FunTraits< RT (*)(P1 , P2 , P3) >
{
	typedef BaseFunType FunType;
	enum { NumParam = 3 };
	typedef typename TypeTraits<P1>::BaseType Param1;
	typedef typename TypeTraits<P2>::BaseType Param2;
	typedef typename TypeTraits<P3>::BaseType Param3;

	static char const** getParam()
	{
		static char const* s_param[] =
		{
			TypeToParam<Param1>::getTypeParam() ,
			TypeToParam<Param2>::getTypeParam() ,
			TypeToParam<Param3>::getTypeParam()
		};
		return &s_param[0];
	}
};


template < class RT , class OBJ >
struct FunTraits< RT (OBJ::*)() >
{
	typedef MemFunType FunType;
	enum { NumParam = 0 };
	typedef OBJ ObjType;
	static char const** getParam()
	{
		return NULL;
	}
};
template < class RT , class OBJ , class P1 >
struct FunTraits< RT (OBJ::*)(P1) >
{
	typedef MemFunType FunType;

	enum { NumParam = 1 };

	typedef OBJ ObjType;
	typedef typename TypeTraits<P1>::BaseType Param1;

	static char const** getParam()
	{
		static char const* s_param[] =
		{
			TypeToParam<Param1>::getTypeParam()
		};
		return &s_param[0];
	}
};

template < class RT , class OBJ , class P1 , class P2 >
struct FunTraits< RT (OBJ::*)(P1 , P2) >
{
	typedef MemFunType FunType;

	enum { NumParam = 2 };
	typedef OBJ ObjType;
	typedef typename TypeTraits<P1>::BaseType Param1;
	typedef typename TypeTraits<P2>::BaseType Param2;

	static char const** getParam()
	{
		static char const* s_param[] =
		{
			TypeToParam<Param1>::getTypeParam() ,
			TypeToParam<Param2>::getTypeParam()
		};
		return &s_param[0];
	}
};


template < class RT , class OBJ , class P1 , class P2 , class P3 >
struct FunTraits< RT (OBJ::*)(P1 , P2 , P3) >
{
	typedef MemFunType FunType;

	enum { NumParam = 3 };
	typedef OBJ ObjType;
	typedef typename TypeTraits<P1>::BaseType Param1;
	typedef typename TypeTraits<P2>::BaseType Param2;
	typedef typename TypeTraits<P3>::BaseType Param3;

	static char const** getParam()
	{
		static char const* s_param[] =
		{
			TypeToParam<Param1>::getTypeParam() ,
			TypeToParam<Param2>::getTypeParam() ,
			TypeToParam<Param3>::getTypeParam()
		};
		return &s_param[0];
	}
};

template < class RT , class OBJ , class P1 , class P2 , class P3 ,class P4 >
struct FunTraits< RT (OBJ::*)(P1 , P2 , P3 , P4) >
{
	typedef MemFunType FunType;

	enum { NumParam = 4 };
	typedef OBJ ObjType;
	typedef typename TypeTraits<P1>::BaseType Param1;
	typedef typename TypeTraits<P2>::BaseType Param2;
	typedef typename TypeTraits<P3>::BaseType Param3;
	typedef typename TypeTraits<P4>::BaseType Param4;

	static char const** getParam()
	{
		static char const* s_param[] =
		{
			TypeToParam<Param1>::getTypeParam() ,
			TypeToParam<Param2>::getTypeParam() ,
			TypeToParam<Param3>::getTypeParam() ,
			TypeToParam<Param4>::getTypeParam()
		};
		return &s_param[0];
	}
};

template < class FunSig , int PN >
struct FunParamHelper
{
};


#define DEF_FUN_PARAM_HELPER( PN )  \
	template <class FunSig> \
	struct FunParamHelper< FunSig , PN > \
	{ typedef typename FunTraits< FunSig >::Param##PN type; };

DEF_FUN_PARAM_HELPER( 1 )
DEF_FUN_PARAM_HELPER( 2 )
DEF_FUN_PARAM_HELPER( 3 )
DEF_FUN_PARAM_HELPER( 4 )

template< class FunSig , int N >
struct FunCallback{};

template< class FunSig >
struct FunCallback< FunSig , 0 >
{
	typedef FunTraits< FunSig > FT;

	FunCallback(){}
	FunCallback( void** data ){}

	void execute( FunSig fun ){ fun(); }
	template < class T >
	void execute( FunSig fun , T* obj ){ (obj->*fun)(); }
};

template< class FunSig >
struct FunCallback< FunSig , 1 >
{
	typedef FunTraits< FunSig > FT;
	typedef typename FT::Param1 P1;
	FunCallback( P1& _p1)
		:p1(_p1){}
	FunCallback(void* argsData[])
		:p1( *((P1*)argsData[0]) ){}

	void execute( FunSig fun ){ fun(p1); }
	template < class T >
	void execute( FunSig fun , T* obj ){ (obj->*fun)(p1); }

	P1& p1;
};


template< class FunSig >
struct FunCallback< FunSig , 2 >
{
	typedef FunTraits< FunSig > FT;
	typedef typename FT::Param1 P1;
	typedef typename FT::Param2 P2;
	FunCallback( P1& _p1 , P2& _p2 )
		:p1(_p1),p2(_p2){}

	FunCallback(void* argsData[])
		:p1( *((P1*)argsData[0]) )
		,p2( *((P2*)argsData[1]) ){}

	void execute( FunSig fun ){ fun(p1,p2); }
	template < class T >
	void execute( FunSig fun , T* obj ){ (obj->*fun)(p1,p2); }

	P1& p1; P2& p2;
};


template< class FunSig >
struct FunCallback< FunSig , 3 >
{
	typedef FunTraits< FunSig > FT;
	typedef typename FT::Param1 P1;
	typedef typename FT::Param2 P2;
	typedef typename FT::Param3 P3;
	FunCallback( P1& _p1 , P2& _p2 , P3& _p3)
		:p1(_p1),p2(_p2),p3(_p3){}

	FunCallback(void* argsData[])
		:p1( *((P1*)argsData[0]) )
		,p2( *((P2*)argsData[1]) )
		,p3( *((P3*)argsData[2]) ){}

	void execute( FunSig fun ){ fun(p1,p2,p3); }
	template < class T >
	void execute( FunSig fun , T* obj ){ (obj->*fun)(p1,p2,p3); }

	P1& p1; P2& p2; P3& p3;
};

template< class FunSig >
struct FunCallback< FunSig , 4 >
{
	typedef FunTraits< FunSig > FT;
	typedef typename FT::Param1 P1;
	typedef typename FT::Param2 P2;
	typedef typename FT::Param3 P3;
	typedef typename FT::Param4 P4;
	FunCallback(P1& _p1 , P2& _p2 , P3& _p3 , P4& _p4 )
		:p1(_p1),p2(_p2),p3(_p3),p4(_p4){}

	FunCallback(void* argsData[])
		:p1( *((P1*)argsData[0]) )
		,p2( *((P2*)argsData[1]) )
		,p3( *((P3*)argsData[2]) )
		,p4( *((P4*)argsData[3]) ) {}

	void execute( FunSig fun ){ fun(p1,p2,p3,p4); }
	template < class T >
	void execute( FunSig fun , T* obj ){ (obj->*fun)(p1,p2,p3,p4); }

	P1& p1; P2& p2; P3& p3; P4& p4;
};

template < bool chioce , class T1 , class T2>
struct SelectType{};

template < class T1 , class T2>
struct SelectType< true , T1 , T2 >
{
	typedef T1 ResultType;
};

template < class T1 , class T2>
struct SelectType< false , T1 , T2 >
{
	typedef T2 ResultType;
};

template < class FunSig >
struct FunCallbackHelper
{
	typedef FunCallback< FunSig , FunTraits< FunSig >::NumParam > Type;
};


}//namespace detail



template< class FunSig , class T >
inline void ExecuteCallbackFun( FunSig fun , T* obj , void* argsData[])
{
	typename detail::FunCallbackHelper< FunSig >::Type callback(argsData);
	callback.execute( fun , obj );
}


template< class FunSig >
inline void ExecuteCallbackFun( FunSig fun , void* argsData[])
{
	typename detail::FunCallbackHelper< FunSig >::Type callback(argsData);
	callback.execute( fun );
}


#define DEF_TYPE_PARAM( type , mapName ) \
namespace detail {\
	template<>\
	struct TypeToParam< type >\
	{ \
		enum { size = sizeof( type ) };\
		static char const*  getTypeParam(){ return mapName; }\
		static char const** getParam()\
		{\
			static char const* s_param[] = { getTypeParam() };\
			return &s_param[0]; \
		}\
	};\
}


DEF_TYPE_PARAM( bool          , "%d" )
DEF_TYPE_PARAM( float         , "%f" )
DEF_TYPE_PARAM( double        , "%lf")
DEF_TYPE_PARAM( unsigned      , "%u" )
DEF_TYPE_PARAM( int           , "%d" )
DEF_TYPE_PARAM( unsigned char , "%u" )
DEF_TYPE_PARAM( char          , "%d" )
DEF_TYPE_PARAM( char*         , "%s" )
DEF_TYPE_PARAM( char const*   , "%s" )



#endif // FunCallback_h__
