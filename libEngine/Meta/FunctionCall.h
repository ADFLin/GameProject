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

#endif // FunctionCall_H_58BFEA1C_6688_4D54_8C39_EA3DAEFF4C96
