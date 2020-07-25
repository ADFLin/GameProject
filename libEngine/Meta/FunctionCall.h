#pragma once
#ifndef FunctionCall_H_58BFEA1C_6688_4D54_8C39_EA3DAEFF4C96
#define FunctionCall_H_58BFEA1C_6688_4D54_8C39_EA3DAEFF4C96

#include "MetaBase.h"

namespace Private
{
	template< class T, class ...Args >
	struct THaveFunctionCallOperatorImpl
	{
		struct Yes { char dummy[2]; };

		template< class U, decltype(std::declval<U>()(std::declval<Args>()...))(U::*)(Args...) >
		struct SFINAE {};
		template< class U >
		static Yes Tester(SFINAE<U, &U::operator() >*);
		template< class U  >
		static char Tester(...);
		static bool constexpr Value = sizeof(Tester<T>(0)) == sizeof(Yes);
	};
}

template< class T, class ...Args >
struct THaveFuncionCallOperator : Meta::HaveResult< Private::THaveFunctionCallOperatorImpl< T, Args... >::Value >
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
			typedef RT (*Func)(P1 PARAM1_MODIFFER , P2 PARAM2_MODIFFER );\
            typedef void (*NoSupportFun) (P1&, OP_NAME##Detail);\
		};\
		template< class P1 , class P2 , typename EvalType<P1,P2,decltype( OP ( std::declval<P1>(), std::declval<P2>() ) )>::Func func >\
		static auto Tester(P1*) -> decltype( OP ( std::declval<P1>(), std::declval<P2>() ) );\
		template< class P1 , class P2 , typename EvalType<P1,P2,void>::NoSupportFun >\
		static Meta::FalseType  Tester(...);\
		template< class P1 , class P2 >\
		struct Impl\
		{\
			enum { Value = Meta::IsSameType< Meta::FalseType, decltype(Tester< P1,P2, &(OP) >(0)) >::Value == 0 };\
		};\
	};\
	template< class P1 , class P2>\
	struct OP_NAME : Meta::HaveResult< OP_NAME##Detail::Impl< P1 , P2 >::Value > {};

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
