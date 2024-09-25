#include "Coroutines.h"

#include "LogSystem.h"
#include "TemplateMisc.h"

namespace Coroutines
{

	ExecutionContext::ExecutionContext(std::function< void() >&& entryFunc)
		:mYeildReturnType(typeid(void))
		,mEntryFunc(std::move(entryFunc))
	{

	}

	ExecutionContext::~ExecutionContext()
	{
		delete mExecution;
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
		for (auto handle : executions)
		{
			if (isCompleted(handle))
			{
				activeExecutions.push_back(handle.getPointer());
			}
		}

		if (activeExecutions.empty())
		{
			return;
		}

		DependencyFlow* flow = fetchDependencyFlow();
		CHECK(flow->executions.empty());
		flow->mode = DependencyFlow::eSync;
		flow->executions = std::move(activeExecutions);

		mExecutingContext->mDependenceIndex = flow - mDependencyFlows.data();
		CHECK(mExecutingContext->mInstruction == nullptr);
		mExecutingContext->doYeild();
	}

	void ThreadContext::raceExecutions(std::initializer_list<ExecutionHandle> executions)
	{
		for (auto handle : executions)
		{
			if (isCompleted(handle))
			{
				for (auto otherHandle : executions)
				{
					int index = getExecutionOrderIndex(otherHandle);
					if ( index != INDEX_NONE)
					{
						mIndexCompleted.push_back(index);
					}
				}
				return;
			}
		}

		DependencyFlow* flow = fetchDependencyFlow();
		CHECK(flow->executions.empty());
		flow->mode = DependencyFlow::eRace;
		flow->append(executions);

		mExecutingContext->mDependenceIndex = flow - mDependencyFlows.data();
		CHECK(mExecutingContext->mInstruction == nullptr);
		mExecutingContext->doYeild();
	}

	void ThreadContext::rushExecutions(std::initializer_list<ExecutionHandle> executions)
	{
		for (auto handle : executions)
		{
			if (isCompleted(handle))
				return;
		}

		DependencyFlow* flow = fetchDependencyFlow();
		CHECK(flow->executions.empty());
		flow->mode = DependencyFlow::eRush;
		flow->append(executions);

		mExecutingContext->mDependenceIndex = flow - mDependencyFlows.data();
		CHECK(mExecutingContext->mInstruction == nullptr);
		mExecutingContext->doYeild();
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

	static void ExecuteEntry(YeildType& yeild)
	{
		ExecutionContext& context = yeild.get();
		context.mYeild = &yeild;
		context.mEntryFunc();
		context.mYeild = nullptr;
	}

	ExecutionHandle ThreadContext::startInternal(std::function< void() > entryFunc)
	{
		ExecutionContext* context = newExecution(std::move(entryFunc));
		ExecutionContext* parentContext = mExecutingContext;
		mExecutingContext = context;
		context->mExecution = new ExecutionType(ExecuteEntry);
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

	void ThreadContext::executeContextInternal(ExecutionContext& context)
	{
		ExecutionContext* parentContext = mExecutingContext;
		mExecutingContext = &context;
		context.execute();
		mExecutingContext = parentContext;
		postExecuteContext(context);
	}

	bool ThreadContext::postExecuteContext(ExecutionContext& context)
	{
		bool bKeep = context.mExecution && bool(*context.mExecution) && context.mYeild;
		if (bKeep == false)
		{
			if (context.mParent && context.mParent->mDependenceIndex != INDEX_NONE)
			{			
				if (context.mParent->mDependenceIndex == SIMPLE_SYNC_INDEX)
				{
					CHECK(context.mParent->mInstruction == nullptr);
					context.setYeildReturn();
					executeContextInternal(*context.mParent);
					mIndexCompleted.push_back(mExecutions.findIndex(&context));
				}
				else
				{
					auto& flow = mDependencyFlows[context.mParent->mDependenceIndex];
					if (flow.mode == DependencyFlow::eSync)
					{
						flow.executions.remove(&context);
						if (!flow.executions.empty())
						{
							mIndexCompleted.push_back(mExecutions.findIndex(&context));
							return bKeep;
						}
					}

					CHECK(context.mParent->mInstruction == nullptr);
					context.mParent->setYeildReturn();
					executeContextInternal(*context.mParent);

					if (flow.mode == DependencyFlow::eRace)
					{
						for (auto exec : flow.executions)
						{
							int index = getExecutionOrderIndex(exec);
							if (index != INDEX_NONE)
							{
								mIndexCompleted.push_back(index);
							}
						}
						std::sort(mIndexCompleted.begin(), mIndexCompleted.end(), std::less());
					}
					removeDependencyFlow(context.mParent->mDependenceIndex);
				}			
			}
			else
			{
				mIndexCompleted.push_back(mExecutions.findIndex(&context));
			}
		}

		return bKeep;
	}

	void ThreadContext::cleanup()
	{
		CHECK(mExecutingContext == nullptr);
		for (auto execution : mExecutions)
		{
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
			mIndexCompleted.push_back(index);
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
		bool bStop = false;
		for (int index = 0; index < mExecutions.size(); ++index)
		{
			auto exec = mExecutions[index];
			if (exec->tag == tag)
			{
				mIndexCompleted.push_back(index);
			}
			if (exec == mExecutingContext)
				bStop = true;
		}
		if (bStop)
		{
			mExecutingContext->doYeild();
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
				mIndexCompleted.push_back(index);
				if (exec == mExecutingContext)
				{
					mExecutingContext->doYeild();
				}
				else if (mExecutingContext == nullptr)
				{
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

		resumeExecutionInternal(context);
		return true;
	}

	bool ThreadContext::resumeExecution(ExecutionHandle& handle)
	{
		if (!handle.isValid())
			return false;

		auto context = handle.getPointer();
		if (!canResumeExecutionInternal(context))
			return false;

		context->setYeildReturn();
		resumeExecutionInternal(context);
		if (isCompleted(handle.getPointer()))
		{
			handle = ExecutionHandle::Completed(handle.getPointer());
		}	
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

	void ThreadContext::resumeExecutionInternal(ExecutionContext* context)
	{
		CHECK(context->mDependenceIndex == SLEEP_SYNC_INDEX);
		context->mDependenceIndex = INDEX_NONE;
		executeContextInternal(*context);
	}

	void ThreadContext::cleanupCompletedExecutions()
	{
		CHECK(bUseExecutions == false);
		if (mIndexCompleted.empty())
			return;

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
		mIndexFreeFlow = index;
	}

}