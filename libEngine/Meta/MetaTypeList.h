#ifndef MetaTypeList_h__
#define MetaTypeList_h__

#include "MetaBase.h"
#include "Select.h"

namespace Meta
{
	struct NullType 
	{
		typedef NullType TypeValue;
		typedef NullType Next;
	};

	template < class T , class Q >
	struct TypeListNode
	{
		typedef T TypeValue;
		typedef Q Next;
	};


	template < class T >
	struct TypeListNode< T , NullType >
	{
		typedef T TypeValue;
		typedef NullType Next;
	};

	//template< class T1 , class T2 >
	//struct TypeList : TypeListNode< T1 , T2 >{};

	//template< class T1 >
	//struct TypeList< T1 , NullType > : TypeListNode< T1 , NullType >{};


	//template< class T , class T1 , class T2 >
	//struct TypeList< T , TypeList< T1 , T2 > > 
	//	:TypeListNode< T , TypeList< T1 , T2 > >{};

	//template< class T , class T1 , class T2 , class T3 >
	//struct TypeList< T , T1 , T2 , T3 > 
	//	:TypeListNode< T , TypeList< T1 , T2 , T3 > >{};



	template < int N >
	struct Count
	{
		enum { Result = N };
	};

	namespace Detail
	{
		template< class TList , int N >
		struct ListLenghImpl
		{
			typedef typename TSelect< 
				IsSameType< typename TList::TypeValue  , NullType >::Result ,
				Count< N > , ListLenghImpl< typename TList::Next, N + 1 >
			>::ResultType ResultType;

			enum { Result = ResultType::Result };
		};

		template< class TList , class T , int N >
		struct ListFindImpl
		{
			typedef typename TSelect< 
				IsSameType< typename TList::TypeValue  , T >::Result ,
				TureType , ListFindImpl< typename TList::Next, T , N + 1 >
			>::ResultType ResultType;

			enum { Result = ResultType::Result };
			enum 
			{ 
				Index  = TSelectValue< 
				IsSameType< typename TList::TypeValue  , T >::Result  ,
				N ,
				ListFindImpl< typename TList::Next, T , N + 1 >::Index
				>::Result 
			};
		};

		template< class T , int N >
		struct ListFindImpl< NullType , T , N >
		{
			enum { Result = 0 };
			enum { Index  = -1 };
		};
	}

	template< class TList >
	struct ListLengh : Detail::ListLenghImpl< TList , 0 >{};

	template< class TList , class T >
	struct ListFind : Detail::ListFindImpl< TList , T , 0 >{};



}
#endif // MetaTypeList_h__