#pragma once
#ifndef RenderThread_H_4D1A07E0_D65E_40C7_B784_3776C5BC6147
#define RenderThread_H_4D1A07E0_D65E_40C7_B784_3776C5BC6147

#include "PlatformThread.h"
#include "DataStructure/CycleQueue.h"
#include "DataStructure/LinearAllocator.h"
#include "DataStructure/Array.h"
#include "CoreShare.h"

// Forward declaration
class RenderCommandList;

class RenderExecuteContext
{

};

class RenderCommand
{
public:
	virtual ~RenderCommand() = default;
	virtual void execute(RenderExecuteContext& context) {}
	
	char const* debugName = nullptr;
	RenderCommand* next = nullptr;
};


class RenderCommand_FlushCommands : public RenderCommand
{
public:
	RenderCommand_FlushCommands()
	{
		debugName = "FlushCommands";
	}
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

	virtual void execute(RenderExecuteContext& context) override
	{
		mFunc();
	}

	TFunc mFunc;
};

class RenderCommandList
{
public:
	RenderCommandList()
		: mAllocator(1024 * 64) // 64KB Page Size
	{

	}

	RenderCommandList(RenderCommandList&& other) noexcept
		: mAllocator(std::move(other.mAllocator))
		, mHead(other.mHead)
		, mTail(other.mTail)
	{
		other.mHead = nullptr;
		other.mTail = nullptr;
	}

	RenderCommandList& operator=(RenderCommandList&& other) noexcept
	{
		if (this != &other)
		{
			mAllocator = std::move(other.mAllocator);
			mHead = other.mHead;
			mTail = other.mTail;

			other.mHead = nullptr;
			other.mTail = nullptr;
		}
		return *this;
	}

	// Deleted copy to encourage move semantics
	RenderCommandList(const RenderCommandList&) = delete;
	RenderCommandList& operator=(const RenderCommandList&) = delete;

	template< typename TCommand, typename ...TArgs >
	TCommand* allocCommand(TArgs&& ...args)
	{
		void* memory = mAllocator.alloc(sizeof(TCommand));
		TCommand* command = new(memory) TCommand(std::forward<TArgs>(args)...); // Placement new
		
		if (mTail)
		{
			mTail->next = command;
		}
		else
		{
			mHead = command;
		}
		mTail = command;
		
		return command;
	}

	template < typename TFunc >
	void addCommand(char const* name, TFunc&& func)
	{
		auto* command = allocCommand<TFuncRenderCommand<TFunc>>(std::forward<TFunc>(func));
		command->debugName = name;
	}

	bool isEmpty() const { return mHead == nullptr; }

	// Allow iteration
	RenderCommand* getHead() const { return mHead; }
	RenderCommand* getTail() const { return mTail; }

	void clear()
	{
		// Destructors? 
		// RenderCommands are usually POD-like or have specific destructors.
		// Since we use LinearAllocator which cleans up pages, we might need to manually call destructors if they have side effects beyond memory.
		// For now, assuming standard command pattern where resources are managed via ref counting or specialized cleanup not reliant on command destructor for critical unique resources.
		// However, TFuncRenderCommand might capture non-trivial objects.
		
		RenderCommand* cmd = mHead;
		while (cmd)
		{
			RenderCommand* next = cmd->next;
			cmd->~RenderCommand();
			cmd = next;
		}

		mAllocator.clear();
		mHead = nullptr;
		mTail = nullptr;
	}

private:
	LinearAllocator mAllocator;
	RenderCommand*  mHead = nullptr;
	RenderCommand*  mTail = nullptr;

	friend class RenderThread;
};


class RenderCommand_ExecuteList : public RenderCommand
{
public:
	RenderCommand_ExecuteList(RenderCommandList&& list)
		: mList(std::move(list))
	{
		debugName = "ExecuteCommandList";
	}

	~RenderCommand_ExecuteList();
	void execute(RenderExecuteContext& context) override;

	RenderCommandList mList;
};

class CORE_API RenderThread : public RunnableThreadT<RenderThread>
{
public:
	Mutex mCommandMutex;
	TCycleQueue< RenderCommand* > mCommandList;
	// New Command List handling
	TArray<RenderCommandList> mFreeCommandLists;

	ConditionVariable mWaitCV;
	bool bHasNewWork = false;
	void notifyNewWork()
	{
		Mutex::Locker lock(mCommandMutex);
		bHasNewWork = true;
		mWaitCV.notifyOne();
	}

	void add(RenderCommand* commnad);

	static RenderThread* StaticInstance;

	bool bRunning = true;
	unsigned run();

	static bool IsRunning(); 

	static void Initialize();
	static void Finalize();

	static void SetThreadCommandList(RenderCommandList* list);
	static RenderCommandList* GetThreadCommandList();

	template< typename TCommand, typename ...TArgs >
	static TCommand* AllocCommand(TArgs&& ...args)
	{
		RenderCommandList* list = GetThreadCommandList();
		if (list)
		{
			return list->allocCommand<TCommand>(std::forward<TArgs>(args)...);
		}

		TCommand* command = new TCommand(std::forward<TArgs>(args)...);
		if (IsRunning())
		{
			StaticInstance->add(command);
		}
		else
		{
			RenderExecuteContext context;
			command->execute(context);
			delete command;
			return nullptr;
		}
		return command;
	}

	template < typename TFunc >
	static void AddCommand(char const* name, TFunc&& func)
	{
		if (IsRunning())
		{
			auto* command = AllocCommand<TFuncRenderCommand<TFunc>>(std::forward<TFunc>(func));
			if (command) command->debugName = name;
		}
		else
		{
			func();
		}
	}

	RenderCommandList allocCommandList()
	{
		Mutex::Locker lock(mCommandMutex);
		if (mFreeCommandLists.empty())
		{
			return RenderCommandList();
		}

		RenderCommandList list = std::move(mFreeCommandLists.back());
		mFreeCommandLists.pop_back();
		return list;
	}

	void commitCommandList(RenderCommandList& list)
	{
		AllocCommand<RenderCommand_ExecuteList>(std::move(list));
	}

	void recycleCommandList(RenderCommandList&& list)
	{
		Mutex::Locker lock(mCommandMutex);
		mFreeCommandLists.push_back(std::move(list));
	}

	static RenderThread& Get()
	{
		return *StaticInstance;
	}

	static void FlushCommands();
};

CORE_API bool IsInRenderThread();

#endif // RenderThread_H_4D1A07E0_D65E_40C7_B784_3776C5BC6147
