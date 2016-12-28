#pragma once
#ifndef MetaTemplate_h__
#define MetaTemplate_h__

#include "CppVersion.h"

#if CPP_TYPE_TRAITS_SUPPORT
#include <type_traits>
#else
#include <boost/type_traits.hpp>
#endif

namespace Meta
{
	template < class T , T value >
	struct ConstIntegral
	{
		static T const Result = value;
	};
	struct EmptyType {};

	typedef ConstIntegral< bool, true > TrueType;
	typedef ConstIntegral< bool, false > FalseType;

	template < bool value > 
	struct HaveResult : ConstIntegral< bool , value >
	{
		typedef ConstIntegral< bool, value > ResultType;
	};



	template < bool , class T1 , class T2 >
	struct Select{};

	template < class T1 , class T2 >
	struct Select< true , T1 , T2 >
	{
		typedef T1 ResultType;
	};

	template < class T1 , class T2 >
	struct Select< false , T1 , T2 >
	{
		typedef T2 ResultType;
	};

	template < bool , int N1 , int N2 >
	struct SelectValue{};

	template < int N1 , int N2 >
	struct SelectValue< true , N1 , N2 >{ enum  { Result = N1 }; };

	template < int N1 , int N2 >
	struct SelectValue< false , N1 , N2  >{ enum  { Result = N2 }; };


	template < class T , class Q >
	struct IsSameType : HaveResult< false >{};

	template < class T >
	struct IsSameType< T , T > : HaveResult< true >{};


	template < class T >
	struct IsPointer : HaveResult< false >{};
	template < class T >
	struct IsPointer< T* > : HaveResult< true >{};
	template < class T >
	struct IsPointer< T const* > : HaveResult< true >{};

	template < class T >
	struct IsPrimary : HaveResult< IsPointer<T>::Result >
	{};

#define DEINE_PRIMARY_TYPE( type )\
	template <> struct IsPrimary< type > : HaveResult< true >{};

	DEINE_PRIMARY_TYPE( bool )

	DEINE_PRIMARY_TYPE( char )
	DEINE_PRIMARY_TYPE( short )
	DEINE_PRIMARY_TYPE( int )
	DEINE_PRIMARY_TYPE( long )
	DEINE_PRIMARY_TYPE( long long )

	DEINE_PRIMARY_TYPE( unsigned char )
	DEINE_PRIMARY_TYPE( unsigned short )
	DEINE_PRIMARY_TYPE( unsigned int )
	DEINE_PRIMARY_TYPE( unsigned long )
	DEINE_PRIMARY_TYPE( unsigned long long )

	DEINE_PRIMARY_TYPE( double )
	DEINE_PRIMARY_TYPE( float )

#undef DEINE_PRIMARY_TYPE


	template < class T > 
	struct IsPod
		: HaveResult< 
#if CPP_TYPE_TRAITS_SUPPORT
			std::is_pod<T>::value
#else
			boost::is_pod<T>::value
#endif
		>
	{};


	template< bool value , class R = void >
	struct EnableIf {};

	template< class R >
	struct EnableIf< true , R >
	{ 
		typedef R ResultType;
	};

}//namespace Meta

template< class T, class ...Args >
struct HaveFunCallOperatorImpl
{
	template< class U, decltype(std::declval<U>()(std::declval<Args>()...))(U::*)(Args...) >
	struct SFINAE {};
	template< class U >
	static Meta::TrueType Test(SFINAE<U, &U::operator() >*);
	template< class U  >
	static Meta::FalseType Test(...);
	static bool const Result = Meta::IsSameType< Meta::TrueType, decltype(Test<T>(0)) >::Result;
};

template< class T, class ...Args >
struct HaveFunCallOperator : Meta::HaveResult< HaveFunCallOperatorImpl< T, Args... >::Result >
{

};

#define SUPPORT_BINARY_OPERATOR( OP_NAME, OP, PARAM1_MODIFFER, PARAM2_MODIFFER)\
	struct OP_NAME##Detial\
	{\
		template < class P1 >\
		friend void OP (P1&, OP_NAME##Detial);\
		template < class P1 , class P2 , class RT >\
		struct EvalType\
		{\
			typedef RT (*Fun)(P1 PARAM1_MODIFFER , P2 PARAM2_MODIFFER );\
            typedef void (*NoSupportFun) (P1&, OP_NAME##Detial);\
		};\
		template< class P1 , class P2 , typename EvalType<P1,P2,decltype( OP ( std::declval<P1>(), std::declval<P2>() ) )>::Fun fun >\
		static auto Test(P1*) -> decltype( OP ( std::declval<P1>(), std::declval<P2>() ) );\
		template< class P1 , class P2 , typename EvalType<P1,P2,void>::NoSupportFun >\
		static Meta::FalseType  Test(...);\
		template< class P1 , class P2 >\
		struct Impl\
		{\
			enum { Result = Meta::IsSameType< Meta::FalseType, decltype(Test< P1,P2, &(OP) >(0)) >::Result == 0 };\
		};\
	};\
	template< class P1 , class P2>\
	struct OP_NAME : Meta::HaveResult< OP_NAME##Detial::Impl< P1 , P2 >::Result > {};


#endif // MetaTemplate_h__