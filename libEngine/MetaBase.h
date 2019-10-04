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
	template < class T , T V >
	struct ConstIntegral
	{
		static T constexpr Value = V;
	};
	struct EmptyType {};

	typedef ConstIntegral< bool, true > TrueType;
	typedef ConstIntegral< bool, false > FalseType;

	template < bool V > 
	struct HaveResult : ConstIntegral< bool , V >
	{
		typedef ConstIntegral< bool, V > Type;
	};

	template < bool , class T1 , class T2 >
	struct Select{ typedef T1 Type; };

	template < class T1 , class T2 >
	struct Select< false , T1 , T2 >{  typedef T2 Type; };

	template < bool , int N1 , int N2 >
	struct SelectValue{ enum { Value = N1 }; };
	template < int N1 , int N2 >
	struct SelectValue< false , N1 , N2  >{ enum  { Value = N2 }; };

	template < class T , class Q >
	struct IsSameType : HaveResult< false >{};
	template < class T >
	struct IsSameType< T , T > : HaveResult< true >{};


	template < class T >
	struct RemoveCV { typedef T Type; };
	template < class T >
	struct RemoveCV< T const > { typedef T Type; };
	template < class T >
	struct RemoveCV< volatile T const > { typedef T Type; };


	template < class T >
	struct IsPointerInternal : HaveResult< false > {};
	template < class T >
	struct IsPointerInternal< T* > : HaveResult< true >{};
	template < class T >
	struct IsPointerInternal< T const* > : HaveResult< true >{};

	template < class T >
	struct IsPointer : IsPointerInternal< typename RemoveCV<T>::Type > {};

	template < class T >
	struct IsPrimary : HaveResult< IsPointer<T>::Value >
	{};

#define DEINE_PRIMARY_TYPE( type )\
	template <> struct IsPrimary< type > : HaveResult< true >{};

#define DEINE_PRIMARY_TYPE_WITH_UNSIGNED( type )\
	DEINE_PRIMARY_TYPE( type )\
	DEINE_PRIMARY_TYPE( unsigned type)

	DEINE_PRIMARY_TYPE( bool )

	DEINE_PRIMARY_TYPE_WITH_UNSIGNED( char )
	DEINE_PRIMARY_TYPE_WITH_UNSIGNED( short )
	DEINE_PRIMARY_TYPE_WITH_UNSIGNED( int )
	DEINE_PRIMARY_TYPE_WITH_UNSIGNED( long )
	DEINE_PRIMARY_TYPE_WITH_UNSIGNED( long long )

	DEINE_PRIMARY_TYPE( double )
	DEINE_PRIMARY_TYPE( float )

#undef DEINE_PRIMARY_TYPE
#undef DEINE_PRIMARY_TYPE_WITH_UNSIGNED


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
		typedef R Type;
	};

	template< class ...Args >
	struct And {};
	template< class T , class ...Args >
	struct And< T , Args...> : HaveResult< T::Value && And< Args... >::Value > {};
	template<>
	struct And<> : HaveResult< true > {};

	template< class T, class ...Args >
	struct Or : HaveResult< T::Value || Or< Args... >::Value > {};
}//namespace Meta

template< class T, class ...Args >
struct HaveFunctionCallOperatorImpl
{
	template< class U, decltype(std::declval<U>()(std::declval<Args>()...))(U::*)(Args...) >
	struct SFINAE {};
	template< class U >
	static Meta::TrueType Test(SFINAE<U, &U::operator() >*);
	template< class U  >
	static Meta::FalseType Test(...);
	static bool const Value = Meta::IsSameType< Meta::TrueType, decltype(Test<T>(0)) >::Value;
};

template< class T, class ...Args >
struct HaveFuncionCallOperator : Meta::HaveResult< HaveFunctionCallOperatorImpl< T, Args... >::Value >
{

};

#define DEFINE_SUPPORT_BINARY_OPERATOR_TYPE( OP_NAME, OP, PARAM1_MODIFFER, PARAM2_MODIFFER)\
	struct OP_NAME##Detail\
	{\
		template < class P1 >\
		friend void OP (P1&, OP_NAME##Detail);\
		template < class P1 , class P2 , class RT >\
		struct EvalType\
		{\
			typedef RT (*Fun)(P1 PARAM1_MODIFFER , P2 PARAM2_MODIFFER );\
            typedef void (*NoSupportFun) (P1&, OP_NAME##Detail);\
		};\
		template< class P1 , class P2 , typename EvalType<P1,P2,decltype( OP ( std::declval<P1>(), std::declval<P2>() ) )>::Fun fun >\
		static auto Test(P1*) -> decltype( OP ( std::declval<P1>(), std::declval<P2>() ) );\
		template< class P1 , class P2 , typename EvalType<P1,P2,void>::NoSupportFun >\
		static Meta::FalseType  Test(...);\
		template< class P1 , class P2 >\
		struct Impl\
		{\
			enum { Value = Meta::IsSameType< Meta::FalseType, decltype(Test< P1,P2, &(OP) >(0)) >::Value == 0 };\
		};\
	};\
	template< class P1 , class P2>\
	struct OP_NAME : Meta::HaveResult< OP_NAME##Detail::Impl< P1 , P2 >::Value > {};


#endif // MetaTemplate_h__