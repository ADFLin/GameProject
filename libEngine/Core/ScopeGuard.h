#pragma once
#ifndef ScopeGuard_H_FDB42512_AFB1_41DB_B6F9_31A0B7245168
#define ScopeGuard_H_FDB42512_AFB1_41DB_B6F9_31A0B7245168

#include <exception>

namespace ScopeGuardSupport
{
	struct TriggerPolicy
	{
		bool checkFire();
	};

	template< class TFunc , class TTriggerPolicy >
	struct TScopeImpl : private TTriggerPolicy
	{
	public:
		FORCEINLINE TScopeImpl(TFunc&& func)
			:mFunc(std::forward<TFunc>(func))
		{
		}

		FORCEINLINE TScopeImpl(TScopeImpl&& rhs) = default;

		~TScopeImpl()
		{
			if (TTriggerPolicy::checkFire())
			{
				mFunc();
			}
		}

		TFunc mFunc;
	};


	struct ScopeSuccessPolicy
	{
	public:
		bool checkFire()
		{
			return mUncaughtExceptionCount == std::uncaught_exceptions();
		}
		int mUncaughtExceptionCount = std::uncaught_exceptions();
	};

	template< class TFunc >
	using TScopeSuccess = TScopeImpl< TFunc, ScopeSuccessPolicy >;

	struct ScopeFailPolicy
	{
	public:
		bool checkFire()
		{
			return mUncaughtExceptionCount != std::uncaught_exceptions();
		}
		int mUncaughtExceptionCount = std::uncaught_exceptions();
	};

	template< class TFunc >
	using TScopeFail = TScopeImpl< TFunc, ScopeFailPolicy >;


	struct ScopeExitPolicy
	{
	public:
		bool checkFire()
		{
			return true;
		}
	};

	template< class TFunc >
	using TScopeExit = TScopeImpl< TFunc, ScopeExitPolicy >;

	struct ExitSyntax
	{
		template <typename TFunc>
		FORCEINLINE TScopeExit<TFunc> operator+(TFunc&& func) const
		{
			return TScopeExit<TFunc>(std::forward<TFunc>(func));
		}
	};

	struct SuccessSyntax
	{
		template <typename TFunc>
		FORCEINLINE TScopeSuccess<TFunc> operator+(TFunc&& func) const
		{
			return TScopeSuccess<TFunc>(std::forward<TFunc>(func));
		}
	};

	struct FailSyntax
	{
		template <typename TFunc>
		FORCEINLINE TScopeFail<TFunc> operator+(TFunc&& func) const
		{
			return TScopeFail<TFunc>(std::forward<TFunc>(func));
		}
	};
}

#define ON_SCOPE_EXIT const auto ANONYMOUS_VARIABLE(ScopeExit_) = ::ScopeGuardSupport::ExitSyntax() + [&]()
#define ON_SCOPE_SUCCESS const auto ANONYMOUS_VARIABLE(ScopeSuccess_) = ::ScopeGuardSupport::SuccessSyntax() + [&]()
#define ON_SCOPE_FAIL const auto ANONYMOUS_VARIABLE(ScopeFail_) = ::ScopeGuardSupport::FailSyntax() + [&]()

#endif // ScopeGuard_H_FDB42512_AFB1_41DB_B6F9_31A0B7245168
