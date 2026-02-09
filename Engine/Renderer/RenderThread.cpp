#include "RenderThread.h"

#include "ProfileSystem.h"
#include "SystemPlatform.h"
#include "CoreShare.h"

#if CORE_SHARE

RenderThread* RenderThread::StaticInstance = nullptr;
uint32 GRenderThreadId = 0;


void RenderThread::add(RenderCommand* commnad)
{
	Mutex::Locker lock(mCommandMutex);
	mCommandList.push_back(commnad);
}

unsigned RenderThread::run()
{
	GRenderThreadId = PlatformThread::GetCurrentThreadId();
	PlatformThread::SetThreadName(GRenderThreadId, "RenderThread");
	ProfileSystem::Get().setThreadName(GRenderThreadId, "RenderThread");

	RenderExecuteContext context;
	while (bRunning)
	{
		RenderCommand* command = nullptr;
		{
			Mutex::Locker lock(mCommandMutex);

			if (!mCommandList.empty())
			{
				command = mCommandList.front();
				mCommandList.pop_front();
			}
		}
		if (command)
		{
			command->execute(context);
			delete command;
		}
		else
		{
			SystemPlatform::Sleep(0);
		}
	}

	return 0;
}

bool RenderThread::IsRunning() { return GRenderThreadId != 0; }

void RenderThread::Initialize()
{
	StaticInstance = new RenderThread;
	StaticInstance->start();
}

void RenderThread::Finalize()
{
	if (StaticInstance == nullptr)
		return;

	FlushCommands();
	StaticInstance->bRunning = false;
	GRenderThreadId = 0;
	StaticInstance->join();

	delete StaticInstance;
	StaticInstance = nullptr;
}

void RenderThread::FlushCommands()
{
	auto command = AllocCommand<RenderCommand_FlushCommands>();
	command->wait();
}

bool IsInRenderThread()
{
	return GRenderThreadId == PlatformThread::GetCurrentThreadId();
}

#endif
