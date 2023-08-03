#ifndef Coroutines_h__236D6542_36DD_4B46_8899_164DE1E3E983
#define Coroutines_h__236D6542_36DD_4B46_8899_164DE1E3E983

#include "Cpp11StdConfig.h"
#include "DataStructure/Array.h"
#include "HashString.h"

#include "Template/ArrayView.h"
#include "Core/IntegerType.h"

#include "boost/coroutine2/all.hpp"
#include "boost/ref.hpp"
#include "boost/bind.hpp"
#include "boost/function.hpp"

//Useful for visualize debug data
class FunctionJumper
{

	typedef boost::coroutines2::asymmetric_coroutine< void >::pull_type ImplType;
	typedef boost::coroutines2::asymmetric_coroutine< void >::push_type YeildType;

public:

	~FunctionJumper()
	{
		delete mImpl;
	}

	template< class TFunc >
	void start( TFunc func )
	{
		mbYeild = false;
		auto entryFun = std::bind(&FunctionJumper::execEntry< TFunc >, this, std::placeholders::_1, func );
		//entryFun(*(YeildType*)0);
		mImpl = new ImplType( entryFun );
	}

	void jump()
	{
		if ( !isRunning() )
			return;
		mbYeild = !mbYeild;
		if ( mbYeild )
			(*mYeild)();
		else
			(*mImpl)();
	}

	bool isRunning() const
	{
		return mYeild != NULL;
	}

private:

	template< class TFunc >
	void execEntry( YeildType& type , TFunc func )
	{
		mYeild = &type;
		func();
		mYeild = NULL;
	}

	bool       mbYeild;
	YeildType* mYeild = nullptr;
	ImplType*  mImpl = nullptr;

};


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
	using ExecutionType = boost::coroutines2::asymmetric_coroutine< void >::pull_type;
	using YeildType = boost::coroutines2::asymmetric_coroutine< void >::push_type;
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

#define SIMPLE_SYNC_INDEX MaxInt32
	struct ExecutionContext
	{
		~ExecutionContext();


		FORCEINLINE void doYeild()
		{
			(*mYeild)();
		}

		void yeildReturn()
		{
			CHECK(mInstruction == nullptr);
			doYeild();
		}

		void yeildReturn(ExecutionHandle childContext)
		{
			CHECK(mInstruction == nullptr);
			if ( childContext.isValid() )
			{
				mDependenceIndex = SIMPLE_SYNC_INDEX;
				doYeild();
			}
		}

		template< typename T, TEnableIf_Type< std::is_base_of_v< IYieldInstruction, std::remove_reference_t<T> >, bool > = true >
		void yeildReturn(T&& value)
		{
			using Type = std::remove_reference_t<T>;
			CHECK(mInstruction == nullptr);
			mInstruction.construct<T>(value);
			mInstruction->startYield(this);
			doYeild();
		}

		void yeildReturn(IYieldInstruction* instruction)
		{
			CHECK(mInstruction == nullptr);
			mInstruction.construct(YieldInstructionProxy(instruction));
			mInstruction->startYield(this);
			doYeild();
		}

		void breakReturn()
		{
			YeildType* yeild = mYeild;
			mYeild = nullptr;
			doYeild();
		}

		void execute();

		HashString tag;

		int mDependenceIndex = INDEX_NONE;
		ExecutionContext*  mParent = nullptr;
		ExecutionType*     mExecution = nullptr;
		YeildType*         mYeild = nullptr;
		TInlineDynamicObject<IYieldInstruction, 32> mInstruction;
	};

	struct ThreadContext
	{
		static ThreadContext& Get()
		{
			thread_local static ThreadContext StaticContext;
			return StaticContext;
		}

		~ThreadContext();

		ExecutionContext& getExecutingContext() { CHECK(mExecutingContext); return *mExecutingContext; }


		void cleanup();
		void checkAllExecutions();

		bool bUseExecutions = false;

		void stopExecution(ExecutionHandle handle);

		void stopAllExecutions(HashString tag);

		void stopFirstExecution(HashString tag);

		bool resumeExecution(YieldContextHandle handle);
		bool resumeExecution(ExecutionHandle handle);

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
		int getExecutionOrderIndex(ExecutionHandle handle)
		{
			if (!handle.isValid())
				return INDEX_NONE;
			return getExecutionOrderIndex(handle.getPointer());
		}

		int getExecutionOrderIndex(ExecutionContext* context)
		{
			int index = mExecutions.findIndex(context);

			if (index == INDEX_NONE)
				return INDEX_NONE;

			if (mIndexCompleted.findIndex(index) != INDEX_NONE)
				return INDEX_NONE;

			return index;
		}

		void syncExecutions(std::initializer_list<ExecutionHandle> executions);
		void raceExecutions(std::initializer_list<ExecutionHandle> executions);
		void rushExecutions(std::initializer_list<ExecutionHandle> executions);

		ExecutionContext* newExecution();

		void checkExecution(ExecutionContext& context);

		template< typename TFunc >
		ExecutionHandle start(TFunc&& func)
		{
			return startInternal([this, &func](ExecutionContext& context)
			{
				auto EntryFunc = [this, &context, func](YeildType& yeild)
				{
					context.mYeild = &yeild;
					func();
					context.mYeild = nullptr;
				};
				context.mExecution = new ExecutionType(EntryFunc);
			});
		}

		ExecutionHandle startInternal(std::function< void(ExecutionContext&) > entryFunc);
		void executeContextInternal(ExecutionContext& context);
		void postExecuteContext(ExecutionContext& context);
		void cleaupCompletedExecutions();


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

	FORCEINLINE void YeildReturn(IYieldInstruction* instruction)
	{
		ExecutionContext& context = ThreadContext::Get().getExecutingContext();
		context.yeildReturn(instruction);
	}

	FORCEINLINE void YeildReturn(std::nullptr_t)
	{
		ExecutionContext& context = ThreadContext::Get().getExecutingContext();
		context.yeildReturn();
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

	FORCEINLINE bool Resume(ExecutionHandle handle)
	{
		return ThreadContext::Get().resumeExecution(handle);
	}
}

#define CO_YEILD( EXPR ) ::Coroutines::YeildReturn(EXPR);
#define CO_BREAK() ::Coroutines::BreakReturn();

#define CO_SYNC( ... ) ::Coroutines::SyncReturn({ ##__VA_ARGS__ } );
#define CO_RACE( ... ) ::Coroutines::RaceReturn({ ##__VA_ARGS__ } );
#define CO_RUSH( ... ) ::Coroutines::RushReturn({ ##__VA_ARGS__ } );


#endif // Coroutines_h__236D6542_36DD_4B46_8899_164DE1E3E983
