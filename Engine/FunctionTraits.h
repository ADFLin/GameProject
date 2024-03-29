#ifndef FunCallback_h__
#define FunCallback_h__

#include "Meta/MetaTypeList.h"
#include "Meta/IndexList.h"

namespace Meta
{
	template< int Index, class ...Args >
	struct GetArgsType {};

	template< int Index, class T, class ...Args >
	struct GetArgsType< Index, T, Args... >
	{
		typedef typename GetArgsType< Index - 1, Args... >::Type Type;
	};

	template< class T, class ...Args >
	struct GetArgsType< 0, T, Args... >
	{
		typedef T Type;
	};

	template< class ...Args >
	struct TypeList
	{
		enum { Length = sizeof...(Args), };
	};

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

	class MemFuncType {};
	class BaseFuncType {};

	template< class FuncSig >
	struct FuncTraits;

	template< class RT, class ...Args >
	struct FuncTraits< RT(*)(Args...) >
	{
		typedef BaseFuncType FuncType;
		typedef TypeList< Args... > ArgList;
		typedef RT ResultType;

		enum { NumArgs = ArgList::Length };
	};

	template< class T , class RT ,class ...Args >
	struct FuncTraits< RT (T::*)(Args...) >
	{
		typedef MemFuncType FuncType;
		typedef TypeList< Args... > ArgList;
		typedef RT ResultType;

		enum { NumArgs = ArgList::Length };
	};

	template< class RT, class T >
	FORCEINLINE RT Invoke(RT(T::*func)(), T* pObject, void* argsData[])
	{
		return (pObject->*func)();
	}

	template< class RT, class T, class TObj, class ...Args , size_t ...Is>
	FORCEINLINE RT InvokeInternal(RT(T::*func)(Args...), TObj* pObject, void* argsData[] , TIndexList<Is...>)
	{
		return (pObject->*func)(*TypeTraits<Args>::PtrType(argsData[Is])...);
	}

	template< class RT, class T , class TObj , class ...Args >
	FORCEINLINE RT Invoke(RT(T::*func)(Args...), TObj* pObject, void* argsData[])
	{
		return InvokeInternal(func, pObject, argsData, TIndexRange<0, sizeof...(Args)>());
	}

	template< class RT>
	FORCEINLINE RT Invoke(RT(*func)(), void* argsData[])
	{
		return (*func)();
	}

	template< class RT, class ...Args, size_t ...Is>
	FORCEINLINE RT InvokeInternal(RT(*func)(Args...), void* argsData[], TIndexList<Is...>)
	{
		return (*func)(*TypeTraits<Args>::PtrType(argsData[Is])...);
	}

	template< class RT, class ...Args >
	FORCEINLINE RT Invoke(RT(*func)(Args...), void* argsData[])
	{
		return InvokeInternal(func, argsData, TIndexRange<0, sizeof...(Args)>());
	}

}//namespace Meta







#endif // FunCallback_h__
