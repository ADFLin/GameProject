#pragma once
#ifndef IsBaseOf_H_90713FBB_7886_4358_881A_FD56E99B92CF
#define IsBaseOf_H_90713FBB_7886_4358_881A_FD56E99B92CF
#include "MetaBase.h"

namespace Private
{
	template< class T, class TBase >
	struct TIsBaseOfImpl
	{
		struct Yes { char dummy[2]; };

		static char Tester(...);
		static Yes  Tester(TBase*);
		static bool constexpr Value = sizeof(Tester((T*)(nullptr))) == sizeof(Yes);
	};
}

template< class T, class TBase >
struct TIsBaseOf : Meta::HaveResult< Private::TIsBaseOfImpl<T, TBase >::Value >
{

};


#endif // IsBaseOf_H_90713FBB_7886_4358_881A_FD56E99B92CF
