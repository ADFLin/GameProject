#pragma once
#ifndef ScopeExit_H_28A04138_B52A_4910_AC60_C9E65BCDE165
#define ScopeExit_H_28A04138_B52A_4910_AC60_C9E65BCDE165

namespace ScopeExitSupport
{
	template< class TFunc >
	struct TScopeGuard
	{
	public:
		FORCEINLINE TScopeGuard(TFunc&& fun)
			:mFun(std::forward<TFunc>(fun))
		{
		}

		FORCEINLINE TScopeGuard(TScopeGuard&& rhs) = default;

		~TScopeGuard()
		{
			mFun();
		}

		TFunc mFun;
	};

	struct Syntax
	{
		template <typename TFunc>
		FORCEINLINE TScopeGuard<TFunc> operator+(TFunc&& fun) const
		{
			return TScopeGuard<TFunc>(std::forward<TFunc>(fun));
		}
	};

}

#define ON_SCOPE_EXIT const auto ANONYMOUS_VARIABLE(ScopeGuard_) = ::ScopeExitSupport::Syntax() + [&]()


#endif // ScopeExit_H_28A04138_B52A_4910_AC60_C9E65BCDE165
