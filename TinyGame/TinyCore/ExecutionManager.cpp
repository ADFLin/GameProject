#include "TinyGamePCH.h"
#include "ExecutionManager.h"
#include "ExecutionPanel.h"
#include "RenderUtility.h"

#include "GameWidgetID.h"
#include "GameGUISystem.h"

ExecutionManager::ExecutionManager()
{
}

ExecutionManager::~ExecutionManager()
{
}

void ExecutionManager::pauseExecution(uint32 threadId)
{
	Mutex::Locker locker(mThreadDataMutex);
	ExecutionData* exec = findExecutionAssumeLocked(threadId);
	if (exec)
	{
		exec->bPaused = true;
		pauseExecution(*exec, locker);
	}
}

ExecutionRenderScope ExecutionManager::registerRender(uint32 threadId, ExecutionRenderFunc const& func, TVector2<int> const& size, bool bTheadSafe)
{
	Mutex::Locker locker(mThreadDataMutex);
	for (auto& exec : mRunningExecutions)
	{
		if (exec.thread->getID() == threadId)
		{
			auto panel = createExecutionPanel(exec, Vec2i(600, 600), bTheadSafe);
			panel->renderFunc = func;
			return ExecutionRenderScope(&panel->renderFlag);
		}
	}
	return ExecutionRenderScope();
}

EKeyCode::Type ExecutionManager::waitInputKey(uint32 threadId)
{
	Mutex::Locker locker(mThreadDataMutex);
	ExecutionData* exec = findExecutionAssumeLocked(threadId);
	if (exec)
	{
		exec->waitType = ExecutionData::eWaitKey;
		exec->key = EKeyCode::None;
		pauseExecution(*exec, locker);
		return exec->key;
	}
	return EKeyCode::None;
}

std::string ExecutionManager::waitInputText(uint32 threadId, char const* defaultText)
{
	Mutex::Locker locker(mThreadDataMutex);
	ExecutionData* exec = findExecutionAssumeLocked(threadId);
	if (exec)
	{
		exec->waitType = ExecutionData::eWaitText;
		exec->text = "";

		if (exec->panel == nullptr)
		{
			createExecutionPanel(*exec, Vec2i(400, 35), false);
		}

		if (mQueueCommandFunc)
		{
			mQueueCommandFunc([panel = exec->panel, text = std::string(defaultText)]()
			{
				panel->addTextInputUI(text.c_str());
			});
		}
		
		pauseExecution(*exec, locker);
		return exec->text;
	}
	return "";
}

void ExecutionManager::terminateThread(Thread* thread)
{
	if (thread)
	{
		unregisterThread(thread);
		thread->kill();
	}
}

void ExecutionManager::pauseThread(Thread* thread)
{
	Mutex::Locker locker(mThreadDataMutex);
	for (auto& exec : mRunningExecutions)
	{
		if (exec.thread == thread)
		{
			exec.bPaused = true;
			break;
		}
	}
}

void ExecutionManager::resumeThread(Thread* thread)
{
	Mutex::Locker locker(mThreadDataMutex);
	for (auto& exec : mRunningExecutions)
	{
		if (exec.thread == thread)
		{
			if (exec.bPaused || exec.waitType != ExecutionData::eNone)
			{
				resumeExecution(exec);
			}
			break;
		}
	}
}

void ExecutionManager::resumeAllThreads()
{
	Mutex::Locker locker(mThreadDataMutex);
	for (auto& exec : mRunningExecutions)
	{
		if (exec.bPaused || exec.waitType != ExecutionData::eNone)
		{
			resumeExecution(exec);
		}
	}
}

bool ExecutionManager::isThreadExecuting(Thread* thread)
{
	Mutex::Locker locker(mThreadDataMutex);
	for (auto& exec : mRunningExecutions)
	{
		if (exec.thread == thread)
		{
			return !exec.bPaused && exec.waitType == ExecutionData::eNone;
		}
	}
	return false;
}

ExecutionManager::ExecutionData& ExecutionManager::registerThread(Thread* thread, char const* name)
{
	Mutex::Locker locker(mThreadDataMutex);
	ExecutionData data;
	data.thread = thread;
	data.name = name;
	mRunningExecutions.push_back(data);
	return mRunningExecutions.back();
}

void ExecutionManager::onPanelTextInput(ExecutionPanel* panel, char const* text)
{
	Mutex::Locker locker(mThreadDataMutex);
	for (auto& exec : mRunningExecutions)
	{
		if (exec.panel == panel)
		{
			if (exec.waitType == ExecutionData::eWaitText)
			{
				exec.text = text;
				resumeExecution(exec);
			}
			break;
		}
	}
}

MsgReply ExecutionManager::onPanelKeyMsg(ExecutionPanel* panel, KeyMsg const& msg)
{
	if (msg.isDown())
	{
		Mutex::Locker locker(mThreadDataMutex);
		for (auto& exec : mRunningExecutions)
		{
			if (exec.panel != panel)
				continue;

			if (exec.waitType == ExecutionData::eWaitKey)
			{
				exec.key = msg.getCode();
				resumeExecution(exec);
				return MsgReply::Handled();
			}
		}
	}
	return MsgReply::Unhandled();
}

void ExecutionManager::unregisterThread(Thread* thread)
{
	Mutex::Locker locker(mThreadDataMutex);
	mRunningExecutions.removePred([this, thread](ExecutionData const& data)
	{
		if (data.thread == thread)
		{
			if (data.panel)
			{
				if (mQueueCommandFunc)
				{
					mQueueCommandFunc([panel = data.panel]()
					{
						panel->destroy();
					});
				}
			}
			return true;
		}
		return false;
	});
}

ExecutionManager::ExecutionData* ExecutionManager::findExecutionAssumeLocked(uint32 threadId)
{
	for (auto& exec : mRunningExecutions)
	{
		if (exec.thread->getID() == threadId)
			return &exec;
	}
	return nullptr;
}

void ExecutionManager::pauseExecution(ExecutionData& exec, Mutex::Locker& locker)
{
	if (exec.panel)
	{
		if (exec.panel->mMutex)
		{
			exec.panel->mMutex->unlock();
		}
	}

	// Use Platform Condition Variable
	mCV.wait(locker, [&]() 
	{ 
		return !exec.bPaused && exec.waitType == ExecutionData::eNone; 
	});

	if (exec.panel)
	{
		if (exec.panel->mMutex)
		{
			exec.panel->mMutex->lock();
		}
	}
}

void ExecutionManager::resumeExecution(ExecutionData& exec)
{
	exec.bPaused = false;
	exec.waitType = ExecutionData::eNone;
	mCV.notifyAll();
}

ExecutionPanel* ExecutionManager::createExecutionPanel(ExecutionData& exec, TVector2<int> const& size, bool bTheadSafe)
{
	if (exec.panel)
		return exec.panel;

	auto panel = new ExecutionPanel(UI_EXECUTE_PANEL, Vec2i(0, 0), size, nullptr);
	panel->thread = exec.thread;
	panel->mName = exec.name.c_str();
	panel->manager = this;
	panel->setRenderSize(size);
	panel->renderFlag = 0;
	panel->bThreadSafe = bTheadSafe;
	exec.panel = panel;

	if (bTheadSafe == false)
	{
		panel->mMutex = std::make_unique<Mutex>();
		// Since we are creating this panel on the running tread, lock it immediately
		panel->mMutex->lock();
	}

	if (mQueueCommandFunc)
	{
		mQueueCommandFunc([panel]()
		{
			::Global::GUI().addWidget(panel);
		});
	}

	return panel;
}
