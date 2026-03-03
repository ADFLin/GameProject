#include "Coroutines.h"

#include "LogSystem.h"
#include "TemplateMisc.h"

namespace Coroutines
{

#if CORE_SHARE_CODE
	ThreadContext& ThreadContext::Get()
	{
		thread_local static ThreadContext StaticContext;
		return StaticContext;
	}
#endif

	static uint32 GCoroutineVersionCounter = 0;

	bool ExecutionHandle::isValid() const
	{
		if (mPtr <= 0) return false;
		ExecutionContext* ctx = (ExecutionContext*)(intptr_t)mPtr;
		// Step 1: Check if the pointer is still registered in the current thread's context
		if (ThreadContext::Get().getExecutionOrderIndex(ctx) == INDEX_NONE)
			return false;
		// Step 2: Check if the version matches (avoids ABA problem)
		return ctx->mVersion == mVersion;
	}

	static void ExecuteEntry(YieldType& yield)
	{
		ExecutionContext& context = yield.get();

		context.mYield = &yield;
		try
		{
			context.mEntryFunc();
		}
		catch (boost::context::detail::forced_unwind const&)
		{
			throw;
		}
		catch (...)
		{
			LogWarning(0, "Coroutine caught an unhandled exception.");
		}
		context.mYield = nullptr;
	}

	ExecutionContext::ExecutionContext(std::function< void() >&& entryFunc, size_t stackSize, uint32 version)
		:mEntryFunc(std::move(entryFunc))
		,mVersion(version)
	{
		boost::coroutines2::fixedsize_stack custom_stack(stackSize);
		mExecution.construct(custom_stack, ExecuteEntry);
	}

	ExecutionContext::~ExecutionContext()
	{
	}

	void ExecutionContext::execute()
	{
		ThreadContext::Get().removeFromActive(*this);
		CHECK(mYield);
		if (mInstruction.isValid())
		{
			mInstruction.release();
		}
		(*mExecution)(*this);
	}

	void ExecutionContext::addPendingActive()
	{
		if (mActiveIndex == INDEX_NONE)
		{
			mActiveIndex = ThreadContext::Get().mActiveExecutions.size();
			ThreadContext::Get().mActiveExecutions.push_back(this);
		}
	}


	ThreadContext::~ThreadContext()
	{
		cleanup();
	}

	int ThreadContext::getExecutionOrderIndex(ExecutionContext* context)
	{
		int index = mExecutions.findIndex(context);
		if (index == INDEX_NONE)
			return INDEX_NONE;

		if (mIndexCompleted.findIndex(index) != INDEX_NONE)
			return INDEX_NONE;

		return index;
	}

	int ThreadContext::getExecutionOrderIndex(ExecutionHandle handle)
	{
		if (!handle.isValid())
			return INDEX_NONE;
		return getExecutionOrderIndex(handle.getPointer());
	}


	struct DependencyScope
	{
		DependencyScope(ThreadContext& context, ThreadContext::DependencyFlow::Mode inMode)
			:context(context)
		{
			CHECK(context.mExecutingContext->mDependenceIndex == INDEX_NONE);
			flow = context.fetchDependencyFlow();
			CHECK(flow->executions.empty());
			flow->mode = inMode;
			context.mExecutingContext->mDependenceIndex = flow - context.mDependencyFlows.data();
		}


		~DependencyScope()
		{
			context.removeDependencyFlow(context.mExecutingContext->mDependenceIndex);
			context.mExecutingContext->mDependenceIndex = INDEX_NONE;
		}
		ThreadContext& context;
		ThreadContext::DependencyFlow* flow;

	};

	void ThreadContext::syncExecutions(std::initializer_list<ExecutionHandle> executions)
	{
		TArray<ExecutionContext*> activeExecutions;
		for (int index = 0; index < executions.size(); ++index)
		{
			auto handle = executions.begin()[index];
			if (isCompleted(handle) == false)
			{
				activeExecutions.push_back(handle.getPointer());
			}
		}

		if (activeExecutions.empty())
		{
			return;
		}

#if CO_USE_DEPENDENT_REFCOUNT
		for (auto exec : activeExecutions)
		{
			CHECK(!isCompleted(exec));
			exec->mDependentRefCount += 1;
		}
#endif

		DependencyScope scopedDependency{ *this, DependencyFlow::eSync };
		scopedDependency.flow->executions = std::move(activeExecutions);
		mExecutingContext->doYield();
		return;
	}

	int FindIndex(std::initializer_list<ExecutionHandle> executions, ExecutionContext* context)
	{
		int index = 0;
		for (int index = 0; index < executions.size(); ++index)
		{
			auto handle = executions.begin()[index];
			if (handle.mPtr == (int64)context)
				return index;
		}

		NEVER_REACH("FindIndex");
		return INDEX_NONE;
	}

	int ThreadContext::raceExecutions(std::initializer_list<ExecutionHandle> executions)
	{
		int index = 0;
		for (int index = 0; index < executions.size(); ++index)
		{
			auto handle = executions.begin()[index];
			if (isCompleted(handle))
			{
				for (auto otherHandle : executions)
				{
					int index = getExecutionOrderIndex(otherHandle);
					if (index != INDEX_NONE)
					{
						auto exec = otherHandle.getPointer();
						if (exec->mAbandonFunc)
						{
							exec->mAbandonFunc();
						}
						destroyExecution(index);
					}
				}
				return index;
			}
		}

		DependencyScope scopedDependency{ *this, DependencyFlow::eRace };
#if CO_USE_DEPENDENT_REFCOUNT
		for (int index = 0; index < executions.size(); ++index)
		{
			auto handle = executions.begin()[index];
			if (!isCompleted(handle))
			{
				handle.getPointer()->mDependentRefCount += 1;
			}
		}
#endif
		scopedDependency.flow->append(executions);
		mExecutingContext->doYield();

		int result = FindIndex(executions, scopedDependency.flow->executionLastCompleted);
		return result;
	}

	int ThreadContext::rushExecutions(std::initializer_list<ExecutionHandle> executions)
	{
		int index = 0;
		for (int index = 0; index < executions.size(); ++index)
		{
			auto handle = executions.begin()[index];
			if (isCompleted(handle))
				return index;
		}


		DependencyScope scopedDependency{ *this, DependencyFlow::eRush };

#if CO_USE_DEPENDENT_REFCOUNT
		for (int index = 0; index < executions.size(); ++index)
		{
			auto handle = executions.begin()[index];
			if (!isCompleted(handle))
			{
				handle.getPointer()->mDependentRefCount += 1;
			}
		}
#endif
		scopedDependency.flow->append(executions);
		mExecutingContext->doYield();
		int result = FindIndex(executions, scopedDependency.flow->executionLastCompleted);
		return result;
	}

	ExecutionContext* ThreadContext::newExecution(std::function< void() >&& entryFunc, size_t stackSize)
	{
		ExecutionContext* context = new ExecutionContext(std::move(entryFunc), stackSize, ++GCoroutineVersionCounter);
		mExecutions.push_back(context);
		return context;
	}

	bool ThreadContext::checkExecution(ExecutionContext& context)
	{
		if (context.mDependenceIndex != INDEX_NONE )
			return false;

		if (context.mInstruction.isValid() && context.mInstruction->isKeepWaiting())
			return false;

		context.setYieldReturn();
		executeContextInternal(context);
		return true;
	}

	ExecutionHandle ThreadContext::startInternal(std::function< void() > entryFunc, size_t stackSize)
	{
		ExecutionContext* context = newExecution(std::move(entryFunc), stackSize);
		ExecutionContext* parentContext = mExecutingContext;
		mExecutingContext = context;
		(*context->mExecution)(*context);
		mExecutingContext = parentContext;
		bool bKeep = postExecuteContext(*context);
		cleanupCompletedExecutions();
		if (bKeep)
		{
			context->mParent = parentContext;
			return ExecutionHandle(context, context->mVersion);
		}
		return ExecutionHandle::Completed(context);
	}

	bool ThreadContext::executeContextInternal(ExecutionContext& context)
	{
		ExecutionContext* parentContext = mExecutingContext;
		mExecutingContext = &context;
		context.execute();
		mExecutingContext = parentContext;
		return postExecuteContext(context);
	}

	bool ThreadContext::postExecuteContext(ExecutionContext& context)
	{
		bool bKeep = bool(*context.mExecution) && context.mYield;
		if (bKeep)
		{
			return true;
		}

		auto parent = context.mParent;
		destroyExecution(mExecutions.findIndex(&context));

		if (parent == nullptr)
			return false;

		//ensure parent is exist
		int indexParent = getExecutionOrderIndex(parent);
		if (indexParent == INDEX_NONE)
		{
			return false;
		}

		if (parent->mDependenceIndex != INDEX_NONE)
		{			
			if (parent->mDependenceIndex != SIMPLE_SYNC_INDEX)
			{
				auto& flow = mDependencyFlows[parent->mDependenceIndex];
				flow.executionLastCompleted = &context;
				flow.executions.removeSwap(&context);

				if (flow.mode == DependencyFlow::eSync)
				{
					if (!flow.executions.empty())
					{
						for (auto exec : flow.executions)
						{
							int index = getExecutionOrderIndex(exec);
							if (index != INDEX_NONE)
							{
								return false;
							}
						}
					}
				}
				else if (flow.mode == DependencyFlow::eRace)
				{
					for (auto exec : flow.executions)
					{
						int index = getExecutionOrderIndex(exec);
						if (index != INDEX_NONE)
						{
#if CO_USE_DEPENDENT_REFCOUNT
							exec->mDependentRefCount -= 1;
							if (exec->mDependentRefCount <= 0)
#endif
							{

								if (exec->mAbandonFunc)
								{
									exec->mAbandonFunc();
								}
								destroyExecution(index);
							}
						}
					}
				}
			}

			CHECK(parent->mInstruction == nullptr);
			parent->setYieldReturn();
			executeContextInternal(*parent);

		}
		

		return false;
	}

	void ThreadContext::cleanup()
	{
		CHECK(mExecutingContext == nullptr);
		for (auto execution : mExecutions)
		{
			execution->destroy();
			delete execution;
		}
		mExecutions.clear();
		mActiveExecutions.clear();
		mIndexCompleted.clear();
	}

	void ThreadContext::checkAllExecutions()
	{
		if (mActiveExecutions.empty())
			return;

		{
			TGuardValue<bool> scopedValue(bUseExecutions, true);
			for (int i = 0; i < mActiveExecutions.size(); )
			{
				ExecutionContext& context = *mActiveExecutions[i];

				if (checkExecution(context))
					continue;

				++i;
			}
		}
		cleanupCompletedExecutions();
	}

	void ThreadContext::stopExecution(ExecutionHandle& handle)
	{
		int index = getExecutionOrderIndex(handle);
		if (index != INDEX_NONE)
		{
			destroyExecution(index);

			if (handle.getPointer() == mExecutingContext)
			{
				mExecutingContext->doYield();
			}
			else if (mExecutingContext == nullptr)
			{
				cleanupCompletedExecutions();
			}

			handle.mPtr = 0;
		}
	}

	void ThreadContext::stopAllExecutions(HashString tag)
	{
		int indexStop = INDEX_NONE;
		for (int index = 0; index < mExecutions.size(); ++index)
		{
			auto exec = mExecutions[index];
			if (exec->tag == tag)
			{
				if (exec == mExecutingContext)
					indexStop = index;
				else
				{
					destroyExecution(index);
				}
			}
		}

		if (indexStop != INDEX_NONE)
		{
			mExecutingContext->doYield();
			destroyExecution(indexStop);
		}
		else if (mExecutingContext == nullptr)
		{
			cleanupCompletedExecutions();
		}
	}

	void ThreadContext::stopFirstExecution(HashString tag)
	{
		for (int index = 0; index < mExecutions.size(); ++index)
		{
			auto exec = mExecutions[index];
			if (exec->tag == tag)
			{
				if (exec == mExecutingContext)
				{
					mExecutingContext->doYield();
					destroyExecution(index);
				}
				else if (mExecutingContext == nullptr)
				{
					destroyExecution(index);
					cleanupCompletedExecutions();
				}
				return;
			}
		}
	}

	bool ThreadContext::resumeExecution(CoroutineContextHandle handle)
	{
		ExecutionContext* context = (ExecutionContext*)handle;
		if (!canResumeExecutionInternal(context))
			return false;

		return resumeExecutionInternal(context);
	}

	bool ThreadContext::resumeExecution(ExecutionHandle& handle)
	{
		if (!handle.isValid())
			return false;

		auto context = handle.getPointer();
		if (!canResumeExecutionInternal(context))
			return false;

		context->setYieldReturn();
		bool bKeep = resumeExecutionInternal(context);
		if (!bKeep)
		{
			handle = ExecutionHandle::Completed(context);
		}
		return bKeep;
	}

	bool ThreadContext::canResumeExecutionInternal(ExecutionContext* context)
	{
		if (mExecutingContext)
		{
			LogWarning(0, "Can't Resume Execution during other executing");
			return false;
		}
		int index = getExecutionOrderIndex(context);
		if (index == INDEX_NONE)
		{
			return false;
		}
		return true;
	}

	bool ThreadContext::resumeExecutionInternal(ExecutionContext* context)
	{
		CHECK(context->mDependenceIndex == INDEX_NONE);
		removeFromActive(*context);
		bool bKeep = executeContextInternal(*context);
		cleanupCompletedExecutions();
		return bKeep;
	}

	void ThreadContext::removeFromActive(ExecutionContext& context)
	{
		if (context.mActiveIndex != INDEX_NONE)
		{
			int index = context.mActiveIndex;
			mActiveExecutions.removeIndexSwap(index);
			if (index < mActiveExecutions.size())
			{
				mActiveExecutions[index]->mActiveIndex = index;
			}
			context.mActiveIndex = INDEX_NONE;
		}
	}

	void ThreadContext::destoryCompletedExecutions()
	{
		if (mIndexCompleted.empty())
			return;

		for (int i = mIndexCompleted.size() - 1; i >= 0; --i)
		{
			int index = mIndexCompleted[i];
			delete mExecutions[index];
			mExecutions[index] = nullptr;
		}
	}

	void ThreadContext::cleanupCompletedExecutions()
	{
		CHECK(bUseExecutions == false);
		if (mIndexCompleted.empty())
			return;

#if 0
		if (mIndexCompleted.size() > 1)
		{
			std::sort(mIndexCompleted.begin(), mIndexCompleted.end(), std::less());
		}

		int indexCheck = 0;
		int indexMove = 0;
		int index = 0;

		for (; index < mExecutions.size(); ++index)
		{
			if (index == mIndexCompleted[indexCheck])
			{
				delete mExecutions[index];
				++indexCheck;
				if (indexCheck == mIndexCompleted.size())
				{
					++index;
					break;
				}
			}
			else if (indexMove != index)
			{
				mExecutions[indexMove] = mExecutions[index];
				++indexMove;
			}
			else
			{
				++indexMove;
			}
		}

		CHECK(indexCheck == mIndexCompleted.size());
		if (index != mExecutions.size() && indexMove != index)
		{
#if 1
			int num = mExecutions.size() - index;
			FTypeMemoryOp::MoveSequence(&mExecutions[indexMove], num, &mExecutions[index]);
			indexMove += num;
#else
			while (index < mExecutions.size())
			{
				mExecutions[indexMove] = mExecutions[index];
				++indexMove;
				++index;
			}
#endif
		}
		mExecutions.resize(indexMove);

#else
		if (mIndexCompleted.size() > 1)
		{
			std::sort(mIndexCompleted.begin(), mIndexCompleted.end(), std::greater());
		}

		for (auto index : mIndexCompleted)
		{
			removeFromActive(*mExecutions[index]);
			delete mExecutions[index];
			mExecutions.removeIndexSwap(index);
		}
#endif
		mIndexCompleted.clear();
	}

	ThreadContext::DependencyFlow* ThreadContext::fetchDependencyFlow()
	{
		DependencyFlow* result;
		if (mIndexFreeFlow != INDEX_NONE)
		{
			result = &mDependencyFlows[mIndexFreeFlow];
			mIndexFreeFlow = result->linkIndex;
		}
		else
		{
			result = mDependencyFlows.addDefault();
		}
		return result;
	}

	void ThreadContext::removeDependencyFlow(int index)
	{
		DependencyFlow& flow = mDependencyFlows[index];
		flow.executions.clear();
		flow.linkIndex = mIndexFreeFlow;
		flow.executionLastCompleted = nullptr;
		mIndexFreeFlow = index;
	}

}