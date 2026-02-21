#include "RenderThread.h"

#include "ProfileSystem.h"
#include "SystemPlatform.h"
#include "TemplateMisc.h"
#include "CoreShare.h"

#if CORE_SHARE

RenderThread* RenderThread::StaticInstance = nullptr;
uint32 GRenderThreadId = 0;
thread_local RenderCommandList* GRenderThreadCommandList = nullptr;

void RenderThread::SetThreadCommandList(RenderCommandList* list)
{
	GRenderThreadCommandList = list;
}

RenderCommandList* RenderThread::GetThreadCommandList()
{
	return GRenderThreadCommandList;
}


void RenderThread::add(RenderCommand* commnad)
{
	Mutex::Locker lock(mCommandMutex);
	mCommandList.push_back(commnad);
	bHasNewWork = true;
	mWaitCV.notifyOne();
}


RenderCommand_ExecuteList::~RenderCommand_ExecuteList()
{
	mList.clear();
}

void RenderCommand_ExecuteList::execute(RenderExecuteContext& context)
{
	RenderCommand* cmd = mList.getHead();
	while (cmd)
	{
		if (cmd->debugName)
		{
			//LogMsg("  SubCommand : %s", cmd->debugName);
		}
		cmd->execute(context);
		
		RenderCommand* next = cmd->next;
		cmd = next;
	}

	mList.clear();
	RenderThread::Get().recycleCommandList(std::move(mList));
}

#include "DeferredReleaseQueue.h"

unsigned RenderThread::run()
{
	GRenderThreadId = PlatformThread::GetCurrentThreadId();
	PlatformThread::SetThreadName(GRenderThreadId, "RenderThread");
	ProfileSystem::Get().setThreadName(GRenderThreadId, "RenderThread");

	RenderExecuteContext context;
	while (bRunning)
	{
		DeferredReleaseQueue::Get().process();


		RenderCommand* command = nullptr;

		{
			Mutex::Locker lock(mCommandMutex);

			while (mCommandList.empty() && !bHasNewWork && bRunning)
			{
				PROFILE_ENTRY("RenderThread.Wait");
				mWaitCV.wait(lock);
			}
			bHasNewWork = false;

			if (!mCommandList.empty())
			{
				command = mCommandList.front();
				mCommandList.pop_front();
			}
		}

		if (command)
		{
			PROFILE_ENTRY("RenderThread.ExecuteCommand");
			if (command->debugName)
			{
				//LogMsg("Execute RenderCommand : %s", command->debugName);
			}
			command->execute(context);
			delete command;
		}
	}

	return 0;
}

bool RenderThread::IsRunning() { return GRenderThreadId != 0; }

void RenderThread::Initialize()
{
	StaticInstance = new RenderThread;

	Render::RHIResource::DeferDeleteDelegate = [](Render::RHIResource* resource) -> bool
	{
		if (DeferredReleaseQueue::Get().isActive())
		{
			DeferredReleaseQueue::Get().enqueue(resource);
			RenderThread::Get().notifyNewWork();
			return true;
		}

		return false;
	};
	DeferredReleaseQueue::Get().setActive(true);

	StaticInstance->start();
}

void RenderThread::Finalize()
{
	if (StaticInstance == nullptr)
		return;

	FlushCommands();
	StaticInstance->bRunning = false;
	StaticInstance->notifyNewWork();
	GRenderThreadId = 0;
	StaticInstance->join();

	DeferredReleaseQueue::Get().setActive(false);
	DeferredReleaseQueue::Get().process(); // Final cleanup

	delete StaticInstance;
	StaticInstance = nullptr;
}

void RenderThread::FlushCommands()
{
	if (!IsRunning())
		return;

	RenderCommand_FlushCommands* flushCommand = nullptr;
	if (GRenderThreadCommandList)
	{
		// Allocate flush command into the thread-local list
		flushCommand = GRenderThreadCommandList->allocCommand<RenderCommand_FlushCommands>();
		
		RenderCommandList* localList = GRenderThreadCommandList;
		// Temporary disable the thread-local list to force the commit command into the global queue
		GRenderThreadCommandList = nullptr;
		StaticInstance->commitCommandList(*localList);
		GRenderThreadCommandList = localList;
	}
	else
	{
		// No thread-local list, allocate into global queue directly
		flushCommand = AllocCommand<RenderCommand_FlushCommands>();
	}

	if (flushCommand)
	{
		flushCommand->wait();
	}
}

bool IsInRenderThread()
{
	return GRenderThreadId == PlatformThread::GetCurrentThreadId();
}

#endif
