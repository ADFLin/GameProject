#ifndef Coroutines_h__236D6542_36DD_4B46_8899_164DE1E3E983
#define Coroutines_h__236D6542_36DD_4B46_8899_164DE1E3E983

#include "Cpp11StdConfig.h"
#include "DataStructure/Array.h"
#include "HashString.h"

#include "Template/ArrayView.h"
#include "Core/IntegerType.h"
#include "CoreShare.h"

#include "boost/coroutine2/all.hpp"
#include "boost/ref.hpp"
#include "boost/bind.hpp"
#include "boost/function.hpp"
#include <typeindex>


using YieldContextHandle = void*;
class IYieldInstruction
{
public:
	virtual ~IYieldInstruction() = default;
	virtual void startYield(YieldContextHandle handle) {}
	virtual bool isKeepWaiting() const
	{
		return true;
	}
};


namespace Coroutines
{
	class ExecutionContext;
	using ExecutionType = boost::coroutines2::asymmetric_coroutine< ExecutionContext& >::push_type;
	using YeildType = boost::coroutines2::asymmetric_coroutine< ExecutionContext&  >::pull_type;
	struct ExecutionContext;

	struct ExecutionHandle 
	{
		static ExecutionHandle Completed(void* ptr)
		{
			ExecutionHandle result(ptr);
			result.mPtr = -result.mPtr;
			return result;
		}

		explicit ExecutionHandle(void* ptr = nullptr)
		{
			CHECK(intptr_t(ptr) >= 0);
			mPtr = intptr_t(ptr);
		}

		bool isValid() const { return mPtr > 0; }
		bool isCompleted() const { return mPtr < 0; }
		bool isNone() const { return mPtr == 0; }

		ExecutionContext* getPointer() const { CHECK(!isCompleted()); return (ExecutionContext*)(intptr_t)(mPtr); }


		int64 mPtr;
	};

	struct YieldInstructionProxy : public IYieldInstruction
	{
	public:
		YieldInstructionProxy(IYieldInstruction* ptr) :mPtr(ptr) { CHECK(mPtr); }

		virtual void startYield(YieldContextHandle handle) { mPtr->startYield(handle); }
		virtual bool isKeepWaiting() const { return mPtr->isKeepWaiting(); }
		IYieldInstruction* mPtr;
	};

	template< class T , size_t Size >
	struct TInlineDynamicObject
	{
		~TInlineDynamicObject()
		{
			release();
		}

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

		template< typename Q , TEnableIf_Type< std::is_base_of_v< IYieldInstruction, std::remove_reference_t<Q> >, bool > = true >
		void construct(Q&& value)
		{
			using Type = std::remove_reference_t<Q>;
			CHECK(mPtr == nullptr);
			if (sizeof(Type) > sizeof(mStorage))
			{
				mPtr = new Type(std::forward<Q>(value));
			}
			else
			{
				FTypeMemoryOp::Construct((Type*)mStorage, std::forward<Q>(value));
				mPtr = (T*)mStorage;
			}
		}

		void release()
		{
			if ( mPtr )
			{
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
		}
		operator T*() { return mPtr; }
		T* operator->() { return mPtr; }


		T*   mPtr = nullptr;
		char mStorage[Size];
	};

#define SIMPLE_SYNC_INDEX     MaxInt32
#define SLEEPING_SYNC_INDEX  (MaxInt32 - 1)

	struct ExecutionContext
	{
		ExecutionContext(std::function< void() >&& entryFunc);
		~ExecutionContext();


		FORCEINLINE void doYeild()
		{
			(*mYeild)();
		}

		void const* yeildReturn()
		{
			CHECK(mInstruction == nullptr);
			mDependenceIndex = SLEEPING_SYNC_INDEX;
			doYeild();
			return mYeildReturnData;
		}

		void const* yeildReturn(ExecutionHandle childContext)
		{
			CHECK(mInstruction == nullptr);
			if ( childContext.isValid() )
			{
				mDependenceIndex = SIMPLE_SYNC_INDEX;
				doYeild();
				return mYeildReturnData;
			}
			return nullptr;
		}

		template< typename T, TEnableIf_Type< std::is_base_of_v< IYieldInstruction, std::remove_reference_t<T> >, bool > = true >
		void const* yeildReturn(T&& value)
		{
			using Type = std::remove_reference_t<T>;
			CHECK(mInstruction == nullptr);
			mInstruction.construct<Type>(value);
			mInstruction->startYield(this);
			doYeild();
			return mYeildReturnData;
		}

		void const* yeildReturn(IYieldInstruction* instruction)
		{
			CHECK(mInstruction == nullptr);
			mInstruction.construct(YieldInstructionProxy(instruction));
			mInstruction->startYield(this);
			doYeild();
			return mYeildReturnData;
		}

		void breakReturn()
		{
			YeildType* yeild = mYeild;
			mYeild = nullptr;
			doYeild();
		}
		template< typename T >
		void setYeildReturn(T const& value)
		{
			mYeildReturnType = typeid(T);
			mYeildReturnData = &value;
		}

		void setYeildReturn()
		{
			mYeildReturnType = typeid(void);
			mYeildReturnData = nullptr;
		}

		void execute();

		HashString tag;

		std::type_index    mYeildReturnType;
		void const*        mYeildReturnData;
		ExecutionContext*  mParent = nullptr;
		YeildType*         mYeild = nullptr;
		std::function< void() > mEntryFunc;
		ExecutionType      mExecution;
		int mDependenceIndex = INDEX_NONE;

		TInlineDynamicObject<IYieldInstruction, 32> mInstruction;
	};

	struct ThreadContext
	{
		CORE_API static ThreadContext& Get();

		~ThreadContext();

		ExecutionContext& getExecutingContext() { CHECK(mExecutingContext); return *mExecutingContext; }


		void cleanup();
		void checkAllExecutions();

		bool bUseExecutions = false;

		void stopExecution(ExecutionHandle& handle);

		void stopAllExecutions(HashString tag);

		void stopFirstExecution(HashString tag);

		bool resumeExecution(YieldContextHandle handle);
		bool resumeExecution(ExecutionHandle& handle);


		template< typename T >
		bool resumeExecution(ExecutionHandle& handle, T const& value)
		{
			if (!handle.isValid())
				return false;

			auto context = handle.getPointer();
			if (!canResumeExecutionInternal(context))
				return false;

			context->setYeildReturn(value);
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

			TArray<ExecutionContext*> executions;

			void append(std::initializer_list<ExecutionHandle> handles)
			{
				for (auto handle : handles)
				{
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
		void raceExecutions(std::initializer_list<ExecutionHandle> executions);
		void rushExecutions(std::initializer_list<ExecutionHandle> executions);

		ExecutionContext* newExecution(std::function< void() >&& entryFunc);

		void checkExecution(ExecutionContext& context);

		template< typename TFunc >
		ExecutionHandle start(TFunc&& func)
		{
			return startInternal(std::forward<TFunc>(func));
		}

		ExecutionHandle startInternal(std::function< void() > entryFunc);
		bool executeContextInternal(ExecutionContext& context);
		bool postExecuteContext(ExecutionContext& context);
		void cleanupCompletedExecutions();


		ExecutionContext* mExecutingContext = nullptr;
		TArray<ExecutionContext*> mExecutions;
		TArray<int> mIndexCompleted;


		TArray<DependencyFlow> mDependencyFlows;
		int mIndexFreeFlow = INDEX_NONE;

		DependencyFlow* fetchDependencyFlow();
		void removeDependencyFlow(int index);

	};


	template<typename TFunc>
	FORCEINLINE ExecutionHandle Start(TFunc&& func)
	{
		ThreadContext& theadContext = ThreadContext::Get();
		return theadContext.start(std::forward<TFunc>(func));
	}


	template< typename T >
	FORCEINLINE void YeildReturn(T&& value)
	{
		ExecutionContext& context = ThreadContext::Get().getExecutingContext();
		context.yeildReturn(value);
	}

	template< typename R, typename T >
	FORCEINLINE R YeildReturn(T&& value)
	{
		ExecutionContext& context = ThreadContext::Get().getExecutingContext();
		R* returnPtr = (R*)context.yeildReturn(value);
		if (context.mYeildReturnType == typeid(R))
		{
			return *returnPtr;
		}
		return R();
	}

	FORCEINLINE void YeildReturn(IYieldInstruction* instruction)
	{
		ExecutionContext& context = ThreadContext::Get().getExecutingContext();
		context.yeildReturn(instruction);
	}

	template< typename R >
	FORCEINLINE R YeildReturn(IYieldInstruction* instruction)
	{
		ExecutionContext& context = ThreadContext::Get().getExecutingContext();
		R* returnPtr = (R*)context.yeildReturn(instruction);
		if (context.mYeildReturnType == typeid(R))
		{
			return *returnPtr;
		}
		return R();
	}

	FORCEINLINE void YeildReturn(std::nullptr_t)
	{
		ExecutionContext& context = ThreadContext::Get().getExecutingContext();
		context.yeildReturn();
	}

	template< typename R >
	FORCEINLINE R YeildReturn(std::nullptr_t)
	{
		ExecutionContext& context = ThreadContext::Get().getExecutingContext();
		R* returnPtr = (R*)context.yeildReturn();
		if (context.mYeildReturnType == typeid(R))
		{
			return *returnPtr;
		}

		return R();
	}

	FORCEINLINE void YeildReturn(ExecutionHandle childContext)
	{
		ExecutionContext& context = ThreadContext::Get().getExecutingContext();
		context.yeildReturn(childContext);
	}

	FORCEINLINE void BreakReturn()
	{
		ExecutionContext& context = ThreadContext::Get().getExecutingContext();
		context.breakReturn();
	}

	FORCEINLINE void SyncReturn(std::initializer_list<ExecutionHandle> contexts)
	{
		ThreadContext::Get().syncExecutions(contexts);
	}

	FORCEINLINE void RaceReturn(std::initializer_list<ExecutionHandle> contexts )
	{
		ThreadContext::Get().raceExecutions(contexts);
	}

	FORCEINLINE void RushReturn(std::initializer_list<ExecutionHandle> contexts)
	{
		ThreadContext::Get().rushExecutions(contexts);
	}

	FORCEINLINE bool Resume(YieldContextHandle handle)
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
}

#define CO_YEILD ::Coroutines::YeildReturn
#define CO_BREAK() ::Coroutines::BreakReturn();

#define CO_SYNC( ... ) ::Coroutines::SyncReturn({ ##__VA_ARGS__ } )
#define CO_RACE( ... ) ::Coroutines::RaceReturn({ ##__VA_ARGS__ } )
#define CO_RUSH( ... ) ::Coroutines::RushReturn({ ##__VA_ARGS__ } )


#endif // Coroutines_h__236D6542_36DD_4B46_8899_164DE1E3E983
