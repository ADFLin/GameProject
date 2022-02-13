#pragma once
#ifndef EnableIf_H_E86DEFC1_2207_479C_B4F6_A96E1C6AEBA2
#define EnableIf_H_E86DEFC1_2207_479C_B4F6_A96E1C6AEBA2

template< bool value, class R = void >
struct TEnableIf {};

template< class R >
struct TEnableIf< true, R >
{
	typedef R Type;
};

template< bool value, class R = void >
using TEnableIf_Type = typename TEnableIf< value, R >::Type;

#endif // EnableIf_H_E86DEFC1_2207_479C_B4F6_A96E1C6AEBA2
