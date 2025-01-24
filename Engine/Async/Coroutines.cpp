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

	static void ExecuteEntry(YeildType& yeild)
	{
		ExecutionContext& context = yeild.get();
		context.mYeild = &yeild;
		context.mEntryFunc();
		context.mYeild = nullptr;
	}

	ExecutionContext::ExecutionContext(std::function< void() >&& entryFunc)
		:mEntryFunc(std::move(entryFunc))
	{
		mExecution.construct(ExecuteEntry);
	}

	ExecutionContext::~ExecutionContext()
	{
		mInstruction.release();
	}

	void ExecutionContext::execute()
	{
		CHECK(mYeild);
		mInstruction.release();
		(*mExecution)(*this);
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

		CHECK(mExecutingContext->mDependenceIndex == INDEX_NONE);
		DependencyFlow* flow = fetchDependencyFlow();
		CHECK(flow->executions.empty());
		flow->mode = DependencyFlow::eSync;
		flow->executions = std::move(activeExecutions);

		mExecutingContext->mDependenceIndex = flow - mDependencyFlows.data();
		CHECK(mExecutingContext->mInstruction == nullptr);
		mExecutingContext->doYeild();
		removeDependencyFlow(mExecutingContext->mDependenceIndex);
		mExecutingContext->mDependenceIndex = INDEX_NONE;
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
						destroyExecution(index);
					}
				}
				return index;
			}
		}
		DependencyFlow* flow = fetchDependencyFlow();
		CHECK(flow->executions.empty());
		flow->mode = DependencyFlow::eRace;
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
		flow->append(executions);

		CHECK(mExecutingContext->mDependenceIndex == INDEX_NONE);
		mExecutingContext->mDependenceIndex = flow - mDependencyFlows.data();
		CHECK(mExecutingContext->mInstruction == nullptr);
		mExecutingContext->doYeild();

		int result = FindIndex(executions, flow->executionLastCompleted);
		removeDependencyFlow(mExecutingContext->mDependenceIndex);
		mExecutingContext->mDependenceIndex = INDEX_NONE;
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

		DependencyFlow* flow = fetchDependencyFlow();
		CHECK(flow->executions.empty());
		flow->mode = DependencyFlow::eRush;
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
		flow->append(executions);

		CHECK(mExecutingContext->mDependenceIndex == INDEX_NONE);
		mExecutingContext->mDependenceIndex = flow - mDependencyFlows.data();
		CHECK(mExecutingContext->mInstruction == nullptr);
		mExecutingContext->doYeild();

		int result = FindIndex(executions, flow->executionLastCompleted);
		removeDependencyFlow(mExecutingContext->mDependenceIndex);
		mExecutingContext->mDependenceIndex = INDEX_NONE;
		return result;
	}

	ExecutionContext* ThreadContext::newExecution(std::function< void() >&& entryFunc)
	{
		ExecutionContext* context = new ExecutionContext(std::move(entryFunc));
		mExecutions.push_back(context);
		return context;
	}

	void ThreadContext::checkExecution(ExecutionContext& context)
	{
		if (context.mDependenceIndex != INDEX_NONE )
			return;

		if (context.mInstruction == nullptr || 
			context.mInstruction->isKeepWaiting() == false)
		{
			context.setYeildReturn();
			executeContextInternal(context);
		}
	}

	ExecutionHandle ThreadContext::startInternal(std::function< void() > entryFunc)
	{
		ExecutionContext* context = newExecution(std::move(entryFunc));
		ExecutionContext* parentContext = mExecutingContext;
		mExecutingContext = context;
		(*context->mExecution)(*context);
		mExecutingContext = parentContext;
		bool bKeep = postExecuteContext(*context);
		cleanupCompletedExecutions();
		if (bKeep)
		{
			context->mParent = parentContext;
			return ExecutionHandle(context);
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
		bool bKeep = bool(*context.mExecution) && context.mYeild;
		if (bKeep == false)
		{
			auto parent = context.mParent;
			destroyExecution(mExecutions.findIndex(&context));

			if (parent && parent->mDependenceIndex != INDEX_NONE)
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
								{
#endif
									if (exec->mAbandonFunc)
									{
										exec->mAbandonFunc();
									}
									destroyExecution(index);
#if CO_USE_DEPENDENT_REFCOUNT
								}
#endif
							}
						}
					}
				}

				CHECK(parent->mInstruction == nullptr);
				parent->setYeildReturn();
				executeContextInternal(*parent);
			}
		}

		return bKeep;
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
	}

	void ThreadContext::checkAllExecutions()
	{
		{
			TGuardValue<bool> scopedValue(bUseExecutions, true);
			for (auto execution : mExecutions)
			{
				checkExecution(*execution);
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
				mExecutingContext->doYeild();
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
			mExecutingContext->doYeild();
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
					mExecutingContext->doYeild();
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

	bool ThreadContext::resumeExecution(YieldContextHandle handle)
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

		context->setYeildReturn();
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
		bool bKeep = executeContextInternal(*context);
		cleanupCompletedExecutions();
		return bKeep;
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