#ifndef BoolBranch_h__
#define BoolBranch_h__

template< bool... Is>
struct TBoolList {};

template< typename TFunc, bool ...Is, typename ...Args >
FORCEINLINE auto BoolBranch(TBoolList<Is...>, bool p0, Args... args)
{
	if (p0)
	{
		return BoolBranch<TFunc>(TBoolList<Is..., true>(), args...);
	}
	else
	{
		return BoolBranch<TFunc>(TBoolList<Is..., false>(), args...);
	}
}

template< typename TFunc, bool ...Is >
FORCEINLINE auto BoolBranch(TBoolList<Is...>)
{
	return TFunc::template Exec<Is...>();
}

template< typename TFunc, typename ...Args >
FORCEINLINE auto BoolBranch(Args... args)
{
	return BoolBranch<TFunc>(TBoolList<>(), args...);
}

#endif // BoolBranch_h__
