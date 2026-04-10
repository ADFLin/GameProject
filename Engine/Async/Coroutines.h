#ifndef Coroutines_h__236D6542_36DD_4B46_8899_164DE1E3E983
#define Coroutines_h__236D6542_36DD_4B46_8899_164DE1E3E983

#include "Cpp11StdConfig.h"
#include "DataStructure/Array.h"
#include "HashString.h"

#include "Template/ArrayView.h"
#include "Template/Optional.h"
#include "Core/IntegerType.h"
#include "CoreShare.h"

#include "boost/coroutine2/all.hpp"
#include "boost/ref.hpp"
#include "boost/bind.hpp"
#include "boost/function.hpp"
#include <typeindex>
#include <initializer_list>
#include <type_traits>



using CoroutineContextHandle = void*;
class IAwaitInstruction
{
public:
	virtual ~IAwaitInstruction() = default;
	virtual void onSuspend(CoroutineContextHandle handle) {}
	virtual bool isKeepWaiting() const
	{
		return true;
	}
};

namespace Coroutines
{
	template< typename T >
	struct THasIsKeepWaitingOverride
	{
		static constexpr bool value = !std::is_same_v<
			decltype(&T::isKeepWaiting),
			bool (IAwaitInstruction::*)() const
		>;
	};
	class ExecutionContext;
	struct ExecutionHandle;

	using ExecutionType = boost::coroutines2::asymmetric_coroutine< ExecutionContext& >::push_type;
	using YieldType = boost::coroutines2::asymmetric_coroutine< ExecutionContext&  >::pull_type;

	struct ExecutionHandle
	{
		static ExecutionHandle Completed(ExecutionContext* ptr)
		{
			ExecutionHandle result(ptr, 0);
			result.mPtr = -result.mPtr;
			return result;
		}

		typedef ExecutionContext* ContextPtr;
		ExecutionHandle() { mPtr = 0; mVersion = 0; }

		explicit ExecutionHandle(ExecutionContext* ptr, uint32 version)
		{
			mPtr = intptr_t(ptr);
			mVersion = version;
		}

		bool isValid() const;
		bool isCompleted() const { return mPtr < 0; }
		bool isNone() const { return mPtr == 0; }

		ExecutionContext* getPointer() const { CHECK(mPtr > 0); return (ExecutionContext*)(intptr_t)(mPtr); }

		int64  mPtr;
		uint32 mVersion;
	};

	struct AwaitInstructionProxy : public IAwaitInstruction
	{
	public:
		AwaitInstructionProxy(IAwaitInstruction* ptr) :mPtr(ptr) { CHECK(mPtr); }

		virtual void onSuspend(CoroutineContextHandle handle) { mPtr->onSuspend(handle); }
		virtual bool isKeepWaiting() const { return mPtr->isKeepWaiting(); }
		IAwaitInstruction* mPtr;
	};


	template< class T , size_t Size >
	struct TInlineDynamicObject
	{
		~TInlineDynamicObject()
		{
			if (isValid())
			{
				release();
			}
		}

		bool isValid() const { return !!mPtr; }
		template< typename Q, typename ...TArgs >
		void construct(TArgs&& ...args)
		{
			CHECK(mPtr == nullptr);
			if (sizeof(Q) > sizeof(mStorage))
			{
				mPtr = new Q(std::forward<TArgs>(args)...);
			}
			else
			{
				FTypeMemoryOp::Construct((Q*)mStorage, std::forward<TArgs>(args)...);
				mPtr = (T*)mStorage;
			}
		}

		void release()
		{
			CHECK(isValid());
			if (mPtr != (void*)mStorage)
			{
				delete mPtr;
			}
			else
			{
				mPtr->~T();
			}
			mPtr = nullptr;
		}
		operator T*() { return mPtr; }
		T* operator->() { return mPtr; }


		T*   mPtr = nullptr;
		char mStorage[Size];
	};

#define SIMPLE_SYNC_INDEX     MaxInt32
#define CO_USE_DEPENDENT_REFCOUNT 0

#define CO_DEFAULT_STACK_SIZE (128 * 1024)

	class ExecutionContext
	{
	public:
		ExecutionContext(std::function< void() >&& entryFunc, size_t stackSize, uint32 version);
		~ExecutionContext();


		FORCEINLINE void doYield()
		{
			CHECK(mYield);
			(*mYield)();
		}

		void yieldReturn()
		{
			CHECK(mInstruction == nullptr);
			CHECK(mDependenceIndex == INDEX_NONE);
			doYield();
		}

		void yieldReturn(ExecutionHandle childContext)
		{
			CHECK(mInstruction == nullptr);
			if ( childContext.isValid() )
			{
				mDependenceIndex = SIMPLE_SYNC_INDEX;
				doYield();
				mDependenceIndex = INDEX_NONE;
			}
		}

		void destroy()
		{
			mExecution.release();
			mYield = nullptr;
		}

		template< typename T, TEnableIf_Type< std::is_base_of_v< IAwaitInstruction, std::remove_reference_t<T> >, bool > = true >
		void yieldReturn(T&& value)
		{
			if (!value.isKeepWaiting())
				return;
			
			using Type = std::remove_reference_t<T>;
			CHECK(mInstruction == nullptr);
			mInstruction.construct<Type>(value);
			if constexpr (THasIsKeepWaitingOverride<Type>::value)
			{
				addPendingActive();
			}
			mInstruction->onSuspend(this);
			doYield();
		}

		void yieldReturn(IAwaitInstruction* instruction)
		{
			if (!instruction->isKeepWaiting())
				return;

			CHECK(mInstruction == nullptr);
			mInstruction.construct<AwaitInstructionProxy>(instruction);
			addPendingActive();
			mInstruction->onSuspend(this);
			doYield();
		}

		void breakReturn()
		{
			YieldType* yield = mYield;
			mYield = nullptr;
			doYield();
		}

		struct YieldReturnData
		{
			YieldReturnData()
				:type(typeid(void))
				,ptr(nullptr)
			{}

			std::type_index type;
			void const* ptr;

			template< typename T >
			void setValue(T const& value)
			{
				type = typeid(T);
				ptr = &value;
			}

			void setVoid()
			{
				type = typeid(void);
				ptr = nullptr;
			}

			template< typename R >
			R getReturn()
			{
				if (type == typeid(R))
				{
					return *(R*)ptr;
				}
				return R();
			}
		};
		template< typename T >
		void setYieldReturn(T const& value)
		{
			mReturnData.setValue(value);
		}

		void setYieldReturn()
		{
			mReturnData.setVoid();
		}

		void execute();
		void addPendingActive();

		HashString tag;
		YieldReturnData    mReturnData;
		ExecutionContext*  mParent = nullptr;
		YieldType*         mYield = nullptr;
		std::function< void() > mEntryFunc;
		std::function< void() > mAbandonFunc;
		TOptional<ExecutionType> mExecution;
		uint32 mVersion = 0;
		int mDependenceIndex = INDEX_NONE;
		int mActiveIndex = INDEX_NONE;
#if CO_USE_DEPENDENT_REFCOUNT
		int mDependentRefCount = 0;
#endif
		TInlineDynamicObject<IAwaitInstruction, 64> mInstruction;
	};

	struct ThreadContext
	{
		CORE_API static ThreadContext& Get();

		~ThreadContext();

		ExecutionContext& getExecutingContext() { CHECK(mExecutingContext); return *mExecutingContext; }

		bool isRunning() { return !!mExecutingContext; }
		void cleanup();
		void checkAllExecutions();

		bool bUseExecutions = false;

		void stopExecution(ExecutionHandle& handle);

		void stopAllExecutions(HashString tag);

		void stopFirstExecution(HashString tag);

		bool resumeExecution(CoroutineContextHandle handle);
		bool resumeExecution(ExecutionHandle& handle);

		void destroyExecution(int index)
		{
			CHECK(mIndexCompleted.findIndex(index) == INDEX_NONE);
			mExecutions[index]->destroy();
			mIndexCompleted.push_back(index);
		}

		template< typename T >
		bool resumeExecution(ExecutionHandle& handle, T const& value)
		{
			if (!handle.isValid())
				return false;

			auto context = handle.getPointer();
			if (!canResumeExecutionInternal(context))
				return false;

			context->setYieldReturn(value);
			bool bKeep = resumeExecutionInternal(context);
			if (!bKeep)
			{
				handle = ExecutionHandle::Completed(context);
			}
			return bKeep;
		}

		bool canResumeExecutionInternal(ExecutionContext* context);
		bool resumeExecutionInternal(ExecutionContext* context);


		struct DependencyFlow
		{
			enum Mode : int16
			{
				eSync,
				eRace,
				eRush,
			};
			union
			{
				Mode  mode;
				int16 linkIndex;
			};

			ExecutionContext*     executionLastCompleted = nullptr;
			TArray<ExecutionContext*> executions;

			void append(std::initializer_list<ExecutionHandle> handles)
			{
				for (int index = 0; index < handles.size(); ++index)
				{
					auto handle = handles.begin()[index];
					if(!handle.isValid())
						continue;

					executions.push_back(handle.getPointer());
				}
			}
		};

		bool isCompleted(ExecutionHandle handle)
		{
			return getExecutionOrderIndex(handle) == INDEX_NONE;
		}
		bool isCompleted(ExecutionContext* context)
		{
			return getExecutionOrderIndex(context) == INDEX_NONE;
		}
		int getExecutionOrderIndex(ExecutionHandle handle);

		int getExecutionOrderIndex(ExecutionContext* context);

		void syncExecutions(std::initializer_list<ExecutionHandle> executions);
		int raceExecutions(std::initializer_list<ExecutionHandle> executions);
		int rushExecutions(std::initializer_list<ExecutionHandle> executions);

		ExecutionContext* newExecution(std::function< void() >&& entryFunc, size_t stackSize);

		bool checkExecution(ExecutionContext& context);

		template< typename TFunc >
		ExecutionHandle start(TFunc&& func, size_t stackSize = CO_DEFAULT_STACK_SIZE)
		{
			return startInternal(std::forward<TFunc>(func), stackSize);
		}

		ExecutionHandle startInternal(std::function< void() > entryFunc, size_t stackSize);
		bool executeContextInternal(ExecutionContext& context);
		bool postExecuteContext(ExecutionContext& context);
		void cleanupCompletedExecutions();
		void destoryCompletedExecutions();

		ExecutionContext* mExecutingContext = nullptr;
		TArray<ExecutionContext*> mExecutions;
		TArray<int> mIndexCompleted;

		TArray<DependencyFlow> mDependencyFlows;
		int mIndexFreeFlow = INDEX_NONE;

		DependencyFlow* fetchDependencyFlow();
		void removeDependencyFlow(int index);

		void removeFromActive(ExecutionContext& context);
		TArray<ExecutionContext*> mActiveExecutions;
	};

	FORCEINLINE bool IsRunning()
	{
		return ThreadContext::Get().isRunning();
	}

	template<typename TFunc>
	FORCEINLINE ExecutionHandle Start(TFunc&& func, size_t stackSize = CO_DEFAULT_STACK_SIZE)
	{
		return ThreadContext::Get().start(std::forward<TFunc>(func), stackSize);
	}

	template< typename T >
	FORCEINLINE void YieldReturn(T&& value)
	{
		ExecutionContext& context = ThreadContext::Get().getExecutingContext();
		context.yieldReturn(value);
	}

	template< typename R, typename T >
	FORCEINLINE R YieldReturn(T&& value)
	{
		ExecutionContext& context = ThreadContext::Get().getExecutingContext();
		context.yieldReturn(value);
		return context.mReturnData.getReturn<R>();
	}

	FORCEINLINE void YieldReturn(IAwaitInstruction* instruction)
	{
		ExecutionContext& context = ThreadContext::Get().getExecutingContext();
		context.yieldReturn(instruction);
	}

	template< typename R >
	FORCEINLINE R YieldReturn(IAwaitInstruction* instruction)
	{
		ExecutionContext& context = ThreadContext::Get().getExecutingContext();
		context.yieldReturn(instruction);
		return context.mReturnData.getReturn<R>();
	}

	FORCEINLINE void YieldReturn(std::nullptr_t)
	{
		ExecutionContext& context = ThreadContext::Get().getExecutingContext();
		context.yieldReturn();
	}

	template< typename R >
	FORCEINLINE R YieldReturn(std::nullptr_t)
	{
		ExecutionContext& context = ThreadContext::Get().getExecutingContext();
		context.yieldReturn();
		return context.mReturnData.getReturn<R>();
	}

	FORCEINLINE void YieldReturn(ExecutionHandle childContext)
	{
		ExecutionContext& context = ThreadContext::Get().getExecutingContext();
		context.yieldReturn(childContext);
	}

	FORCEINLINE void BreakReturn()
	{
		ExecutionContext& context = ThreadContext::Get().getExecutingContext();
		context.breakReturn();
	}

	FORCEINLINE void SyncReturn(std::initializer_list<ExecutionHandle> contexts)
	{
		return ThreadContext::Get().syncExecutions(contexts);
	}

	FORCEINLINE int RaceReturn(std::initializer_list<ExecutionHandle> contexts )
	{
		return ThreadContext::Get().raceExecutions(contexts);
	}

	FORCEINLINE int RushReturn(std::initializer_list<ExecutionHandle> contexts)
	{
		return ThreadContext::Get().rushExecutions(contexts);
	}

	FORCEINLINE bool Resume(CoroutineContextHandle handle)
	{
		return ThreadContext::Get().resumeExecution(handle);
	}

	FORCEINLINE bool Resume(ExecutionHandle& handle)
	{
		return ThreadContext::Get().resumeExecution(handle);
	}

	template< typename T >
	FORCEINLINE bool Resume(ExecutionHandle& handle, T const& value)
	{
		return ThreadContext::Get().resumeExecution(handle, value);
	}

	FORCEINLINE void Stop(ExecutionHandle& handle)
	{
		ThreadContext::Get().stopExecution(handle);
	}

	// --- Feature 1: Conditional Waiting ---
	class WaitUntil : public IAwaitInstruction
	{
	public:
		WaitUntil(std::function<bool()> cond) : mCond(std::move(cond)) {}
		virtual bool isKeepWaiting() const override { return !mCond(); }
	private:
		std::function<bool()> mCond;
	};

	class WaitWhile : public IAwaitInstruction
	{
	public:
		WaitWhile(std::function<bool()> cond) : mCond(std::move(cond)) {}
		virtual bool isKeepWaiting() const override { return mCond(); }
	private:
		std::function<bool()> mCond;
	};

	// --- Feature 2: Event System (Reactive) ---
	class Event
	{
	public:
		struct WaitInstruction : public IAwaitInstruction
		{
			WaitInstruction(Event& e) : mOwner(e) {}
			~WaitInstruction() { if (mHandle.isValid()) mOwner.removeWaiter(mHandle); }
			virtual void onSuspend(CoroutineContextHandle handle) override 
			{ 
				ExecutionContext* ctx = (ExecutionContext*)handle;
				mHandle = ExecutionHandle(ctx, ctx->mVersion);
				mOwner.addWaiter(mHandle); 
			}

			Event&            mOwner;
			ExecutionHandle   mHandle;
		};

		WaitInstruction wait() { return WaitInstruction(*this); }

		void trigger()
		{
			TArray<ExecutionHandle> waiters = std::move(mWaiters);
			for (auto& handle : waiters)
			{
				ThreadContext::Get().resumeExecution(handle);
			}
		}

		template< typename T >
		void trigger(T const& value)
		{
			TArray<ExecutionHandle> waiters = std::move(mWaiters);
			for (auto& handle : waiters)
			{
				ThreadContext::Get().resumeExecution(handle, value);
			}
		}

	private:
		void addWaiter(ExecutionHandle handle) { mWaiters.push_back(handle); }
		void removeWaiter(ExecutionHandle handle) 
		{ 
			int index = mWaiters.findIndexPred([&handle](auto const& h) { return h.mPtr == handle.mPtr; });
			if (index != INDEX_NONE) mWaiters.removeIndexSwap(index);
		}
		TArray<ExecutionHandle> mWaiters;
	};
}

#define CO_YIELD ::Coroutines::YieldReturn
#define CO_AWAIT ::Coroutines::YieldReturn
#define CO_BREAK() ::Coroutines::BreakReturn();

#define CO_SYNC( ... ) ::Coroutines::SyncReturn({ ##__VA_ARGS__ } )
#define CO_RACE( ... ) ::Coroutines::RaceReturn({ ##__VA_ARGS__ } )
#define CO_RUSH( ... ) ::Coroutines::RushReturn({ ##__VA_ARGS__ } )


#endif // Coroutines_h__236D6542_36DD_4B46_8899_164DE1E3E983
