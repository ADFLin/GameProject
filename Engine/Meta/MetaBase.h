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

	typedef ConstIntegral< bool, true > TureType;
	typedef ConstIntegral< bool, false > FalseType;

	template < bool V > 
	struct HaveResult : ConstIntegral< bool , V >
	{
		typedef ConstIntegral< bool, V > Type;
	};

	template < class T , class Q >
	struct IsSameType : HaveResult< false >{};
	template < class T >
	struct IsSameType< T , T > : HaveResult< true >{};


	template < class T >
	struct RemoveConst { using Type = T; };
	template < class T >
	struct RemoveConst< T const > { using Type = T; };

	template < class T >
	struct RemoveVolatile { using Type = T; };
	template < class T >
	struct RemoveVolatile< volatile T > { using Type = T; };

	template < class T >
	struct RemoveRef { using Type = T; };
	template < class T >
	struct RemoveRef< T& > { using Type = T; };

	template < class T >
	struct RemoveCV{ using Type = typename RemoveVolatile< typename RemoveConst<T>::Type >::Type; };
	template < class T >
	struct RemoveCVRef { using Type = typename RemoveRef< typename RemoveCV<T>::Type >::Type; };

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


	template< class ...Args >
	struct And {};
	template< class T , class ...Args >
	struct And< T , Args...> : HaveResult< T::Value && And< Args... >::Value > {};
	template<>
	struct And<> : HaveResult< true > {};

	template< class T, class ...Args >
	struct Or : HaveResult< T::Value || Or< Args... >::Value > {};

}//namespace Meta

#endif // MetaTemplate_h__