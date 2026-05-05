#pragma once
#ifndef RenderThread_H_4D1A07E0_D65E_40C7_B784_3776C5BC6147
#define RenderThread_H_4D1A07E0_D65E_40C7_B784_3776C5BC6147

#include "PlatformThread.h"
#include "DataStructure/CycleQueue.h"
#include "DataStructure/LinearAllocator.h"
#include "DataStructure/Array.h"
#include "CoreShare.h"
#include <type_traits>

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
	virtual void destroy() { this->~RenderCommand(); }
	
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
		return mEvent.wait(time);
	}
	void wait()
	{
		mEvent.wait();
	}

	virtual void execute(RenderExecuteContext& context)
	{
		mEvent.fire();
	}
	virtual void destroy() override { this->~RenderCommand_FlushCommands(); }

	ThreadEvent mEvent;
};

template < typename TFunc >
class TFuncRenderCommand : public RenderCommand
{
public:
	template <typename UFunc>
	TFuncRenderCommand(UFunc&& func)
		:mFunc(std::forward<UFunc>(func))
	{
	}

	virtual void execute(RenderExecuteContext& context) override
	{
		if constexpr (TCheckConcept < CFunctionCallable, TFunc, RenderExecuteContext&>::Value)
		{
			mFunc(context);
		}
		else
		{
			mFunc();
		}
	}
	virtual void destroy() override { this->~TFuncRenderCommand(); }

	TFunc mFunc;
};

class RenderCommandList
{
public:
	RenderCommandList()
		: mAllocator(1024 * 64) // 64KB Page Size
	{

	}

	~RenderCommandList()
	{
		clear();
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
			clear();
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
		using StoredFunc = std::decay_t<TFunc>;
		auto* command = allocCommand<TFuncRenderCommand<StoredFunc>>(std::forward<TFunc>(func));
		command->debugName = name;
	}

	bool isEmpty() const { return mHead == nullptr; }

	// Allow iteration
	RenderCommand* getHead() const { return mHead; }
	RenderCommand* getTail() const { return mTail; }

	void clear()
	{
		RenderCommand* cmd = mHead;
		while (cmd)
		{
			RenderCommand* next = cmd->next;
			cmd->destroy();
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
	void destroy() override { this->~RenderCommand_ExecuteList(); }

	RenderCommandList mList;
};

class RenderThread : public RunnableThreadT<RenderThread>
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

	CORE_API void add(RenderCommand* commnad);

	CORE_API static RenderThread* StaticInstance;

	bool bRunning = true;
	CORE_API unsigned run();

	CORE_API static bool IsRunning();

	CORE_API static void Initialize();
	CORE_API static void Finalize();

	CORE_API static void SetThreadCommandList(RenderCommandList* list);
	CORE_API static RenderCommandList* GetThreadCommandList();

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
			using StoredFunc = std::decay_t<TFunc>;
			auto* command = AllocCommand<TFuncRenderCommand<StoredFunc>>(std::forward<TFunc>(func));
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

	CORE_API static void FlushCommands();
};

CORE_API bool IsInRenderThread();

#endif // RenderThread_H_4D1A07E0_D65E_40C7_B784_3776C5BC6147
