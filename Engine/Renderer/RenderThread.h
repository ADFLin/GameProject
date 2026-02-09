#pragma once
#ifndef RenderThread_H_4D1A07E0_D65E_40C7_B784_3776C5BC6147
#define RenderThread_H_4D1A07E0_D65E_40C7_B784_3776C5BC6147

#include "PlatformThread.h"
#include "DataStructure/CycleQueue.h"
#include "CoreShare.h"

class RenderExecuteContext
{

};

class RenderCommand
{
public:
	virtual ~RenderCommand() = default;
	virtual void execute(RenderExecuteContext& context) {}
};


class RenderCommand_FlushCommands : public RenderCommand
{
public:
	bool wait(double time)
	{
		mEvent.wait(time);
		return true;
	}
	void wait()
	{
		mEvent.wait();
	}

	virtual void execute(RenderExecuteContext& context)
	{
		mEvent.fire();
	}

	ThreadEvent mEvent;
};

template < typename TFunc >
class TFuncRenderCommand : public RenderCommand
{
public:
	TFuncRenderCommand(TFunc&& func)
		:mFunc(std::forward<TFunc>(func))
	{

	}

	virtual void execute(RenderExecuteContext& context)
	{
		mFunc();
	}

	TFunc mFunc;
};


class CORE_API RenderThread : public RunnableThreadT<RenderThread>
{
public:
	Mutex mCommandMutex;
	TCycleQueue< RenderCommand* > mCommandList;

	void add(RenderCommand* commnad);

	static RenderThread* StaticInstance;

	bool bRunning = true;
	unsigned run();

	static bool IsRunning(); 

	static void Initialize();

	static void Finalize();

	template< typename TCommand, typename ...TArgs >
	static TCommand* AllocCommand(TArgs&& ...args)
	{
		TCommand* command = new TCommand(std::forward<TArgs>(args)...);
		StaticInstance->add(command);
		return command;
	}

	template < typename TFunc >
	static void AddCommand(char const* name, TFunc&& func)
	{
		AllocCommand<TFuncRenderCommand<TFunc>>(std::forward<TFunc>(func));
	}

	static void FlushCommands();
};

CORE_API bool IsInRenderThread();

#endif // RenderThread_H_4D1A07E0_D65E_40C7_B784_3776C5BC6147
