#include "Coroutines.h"

#include "LogSystem.h"
#include "TemplateMisc.h"

namespace Coroutines
{

	ExecutionContext::ExecutionContext()
		:mYeildReturnType(typeid(void))
	{

	}

	ExecutionContext::~ExecutionContext()
	{
		delete mExecution;
		mInstruction.release();
	}

	void ExecutionContext::execute()
	{
		mYeildReturnType = typeid(void);

		CHECK(mYeild);
		mInstruction.release();
		(*mExecution)(nullptr);
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

	ExecutionContext* ThreadContext::newExecution()
	{
		ExecutionContext* context = new ExecutionContext;
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
			executeContextInternal(context);
		}
	}

	ExecutionHandle ThreadContext::startInternal(std::function< void(ExecutionContext&) > entryFunc)
	{
		ExecutionContext* context = newExecution();
		ExecutionContext* parentContext = mExecutingContext;
		mExecutingContext = context;
		entryFunc(*context);
		mExecutingContext = parentContext;
		postExecuteContext(*context);

		bool bEnd = context->mYeild == nullptr;
		cleanupCompletedExecutions();
		if (bEnd)
		{
			return ExecutionHandle::Completed(context);
		}

		context->mParent = mExecutingContext;
		return ExecutionHandle(context);
	}

	void ThreadContext::executeContextInternal(ExecutionContext& context)
	{
		ExecutionContext* parentContext = mExecutingContext;
		mExecutingContext = &context;
		context.execute();
		mExecutingContext = parentContext;
		postExecuteContext(context);
	}

	void ThreadContext::postExecuteContext(ExecutionContext& context)
	{
		if (context.mYeild == nullptr)
		{
			if (context.mParent && context.mParent->mDependenceIndex != INDEX_NONE)
			{			
				if (context.mParent->mDependenceIndex == SIMPLE_SYNC_INDEX)
				{
					CHECK(context.mParent->mInstruction == nullptr);
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
							return;
						}
					}

					CHECK(context.mParent->mInstruction == nullptr);
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
						std::sort(mIndexCompleted.begin(), mIndexCompleted.end(), std::less<int>());
					}
					removeDependencyFlow(context.mParent->mDependenceIndex);
				}			
			}
			else
			{
				mIndexCompleted.push_back(mExecutions.findIndex(&context));
			}
		}
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

	void ThreadContext::stopExecution(ExecutionHandle handle)
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
		return resumeExecutionInternal(context);
	}

	bool ThreadContext::resumeExecution(ExecutionHandle& handle)
	{
		if (!handle.isValid())
			return false;

		if (!canResumeExecutionInternal(handle.getPointer()))
			return false;

		executeContextInternal(*handle.getPointer());
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

	bool ThreadContext::resumeExecutionInternal(ExecutionContext* context)
	{
		if (!canResumeExecutionInternal(context))
			return false;
		
		executeContextInternal(*context);
		return true;
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