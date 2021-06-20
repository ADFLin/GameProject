#pragma once
#ifndef FunctionCall_H_58BFEA1C_6688_4D54_8C39_EA3DAEFF4C96
#define FunctionCall_H_58BFEA1C_6688_4D54_8C39_EA3DAEFF4C96

#include "MetaBase.h"

struct FunctionCallable
{
	template< typename T, typename... Args >
	static auto requires(T& t , Args... args) -> decltype 
	(	
		t(args...)
	);
};

#define DEFINE_HAVE_MEMBER_FUNC_IMPL( CLASS_NAME , FUNC_NAME )\
template< class T >\
struct CLASS_NAME\
{\
	template< class U , int (U::*)() const> struct SFINAE {};\
	template< class U >\
	static Meta::TureType  Tester( SFINAE< U , &U::FUNC_NAME >* );\
	template< class U >\
	static Meta::FalseType Tester( ...);\
	enum { Value = Meta::IsSameType< Meta::TureType, decltype( Tester<T>(0) ) >::Value };\
};

#endif // FunctionCall_H_58BFEA1C_6688_4D54_8C39_EA3DAEFF4C96
