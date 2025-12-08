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

		ExecutionHandle() { mPtr == 0; }

		explicit ExecutionHandle(void* ptr)
		{
			CHECK(intptr_t(ptr) >= 0);
			mPtr = intptr_t(ptr);
		}

		bool isValid() const { return mPtr > 0; }
		bool isCompleted() const { return mPtr < 0; }
		bool isNone() const { return mPtr == 0; }

		ExecutionContext* getPointer() const { CHECK(isValid()); return (ExecutionContext*)(intptr_t)(mPtr); }


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
			if (isVaild())
			{
				release();
			}
		}

		bool isVaild() const { return !!mPtr; }
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
			CHECK(isVaild());
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

	struct ExecutionContext
	{
		ExecutionContext(std::function< void() >&& entryFunc);
		~ExecutionContext();


		FORCEINLINE void doYeild()
		{
			CHECK(mYeild);
			(*mYeild)();
		}

		void yeildReturn()
		{
			CHECK(mInstruction == nullptr);
			CHECK(mDependenceIndex == INDEX_NONE);
			doYeild();
		}

		void yeildReturn(ExecutionHandle childContext)
		{
			CHECK(mInstruction == nullptr);
			if ( childContext.isValid() )
			{
				mDependenceIndex = SIMPLE_SYNC_INDEX;
				doYeild();
				mDependenceIndex = INDEX_NONE;
			}
		}

		void destroy()
		{
			mExecution.release();
			mYeild = nullptr;
		}

		template< typename T, TEnableIf_Type< std::is_base_of_v< IYieldInstruction, std::remove_reference_t<T> >, bool > = true >
		void yeildReturn(T&& value)
		{
			using Type = std::remove_reference_t<T>;
			CHECK(mInstruction == nullptr);
			mInstruction.construct<Type>(value);
			mInstruction->startYield(this);
			doYeild();
		}

		void yeildReturn(IYieldInstruction* instruction)
		{
			CHECK(mInstruction == nullptr);
			mInstruction.construct< YieldInstructionProxy>(instruction);
			mInstruction->startYield(this);
			doYeild();
		}

		void breakReturn()
		{
			YeildType* yeild = mYeild;
			mYeild = nullptr;
			doYeild();
		}

		struct YeildReturnData
		{
			YeildReturnData()
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
		void setYeildReturn(T const& value)
		{
			mReturnData.setValue(value);
		}

		void setYeildReturn()
		{
			mReturnData.setVoid();
		}

		void execute();

		HashString tag;
		YeildReturnData    mReturnData;
		ExecutionContext*  mParent = nullptr;
		YeildType*         mYeild = nullptr;
		std::function< void() > mEntryFunc;
		std::function< void() > mAbandonFunc;
		TOptional<ExecutionType> mExecution;
		int mDependenceIndex = INDEX_NONE;
#if CO_USE_DEPENDENT_REFCOUNT
		int mDependentRefCount = 0;
#endif
		TInlineDynamicObject<IYieldInstruction, 32> mInstruction;
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

		bool resumeExecution(YieldContextHandle handle);
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
		void destoryCompletedExecutions();

		ExecutionContext* mExecutingContext = nullptr;
		TArray<ExecutionContext*> mExecutions;
		TArray<int> mIndexCompleted;

		TArray<DependencyFlow> mDependencyFlows;
		int mIndexFreeFlow = INDEX_NONE;

		DependencyFlow* fetchDependencyFlow();
		void removeDependencyFlow(int index);

	};

	FORCEINLINE bool IsRunning()
	{
		return ThreadContext::Get().isRunning();
	}

	template<typename TFunc>
	FORCEINLINE ExecutionHandle Start(TFunc&& func)
	{
		return ThreadContext::Get().start(std::forward<TFunc>(func));
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
		context.yeildReturn(value);
		return context.mReturnData.getReturn<R>();
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
		context.yeildReturn(instruction);
		return context.mReturnData.getReturn<R>();
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
		context.yeildReturn();
		return context.mReturnData.getReturn<R>();
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
