#pragma once
#ifndef ScopeExit_H_28A04138_B52A_4910_AC60_C9E65BCDE165
#define ScopeExit_H_28A04138_B52A_4910_AC60_C9E65BCDE165

namespace ScopeExitSupport
{
	template< class Fun >
	struct TScopeGuard
	{
	public:
		FORCEINLINE TScopeGuard(Fun&& fun)
			:mFun(std::forward<Fun>(fun))
		{
		}

		FORCEINLINE TScopeGuard(TScopeGuard&& rhs) = default;

		~TScopeGuard()
		{
			mFun();
		}

		Fun mFun;
	};

	struct Syntax
	{
		template <typename Fun>
		FORCEINLINE TScopeGuard<Fun> operator+(Fun&& fun) const
		{
			return TScopeGuard<Fun>(std::forward<Fun>(fun));
		}
	};

}

#define ON_SCOPE_EXIT const auto ANONYMOUS_VARIABLE(ScopeGuard_) = ::ScopeExitSupport::Syntax() + [&]()


#endif // ScopeExit_H_28A04138_B52A_4910_AC60_C9E65BCDE165
