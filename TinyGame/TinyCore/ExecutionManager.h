#ifndef ExecutionManager_h__
#define ExecutionManager_h__

#include "TinyCore/StageRegister.h"
#include "PlatformThread.h"
#include "DataStructure/Array.h"
#include <string>
#include <functional>
#include <functional>
#include <memory>

class ExecutionPanel;

class ExecutionManager : public IExecutionServices
{
public:
	ExecutionManager();
	~ExecutionManager();

	// IExecutionServices interface
	void pauseExecution(uint32 threadId) override;
	ExecutionRenderScope registerRender(uint32 threadId, ExecutionRenderFunc const& func, TVector2<int> const& size, bool bTheadSafe) override;
	EKeyCode::Type waitInputKey(uint32 threadId) override;
	std::string    waitInputText(uint32 threadId, char const* defaultText) override;

	// Management methods
	void terminateThread(Thread* thread);
	void pauseThread(Thread* thread);
	void resumeThread(Thread* thread);
	void resumeAllThreads();

	bool isThreadExecuting(Thread* thread);

	void onPanelTextInput(ExecutionPanel* panel, char const* text);
	MsgReply onPanelKeyMsg(ExecutionPanel* panel, class KeyMsg const& msg);

	struct ExecutionData
	{
		Thread* thread;
		ExecutionPanel* panel = nullptr;
		std::string name;

		enum
		{
			eNone,
			eWaitKey,
			eWaitText,
		};
		int waitType = eNone;
		bool bPaused = false;
		EKeyCode::Type key;
		std::string text;
	};

	ExecutionData& registerThread(Thread* thread, char const* name);
	void unregisterThread(Thread* thread);

	void setCommandQueue(std::function<void(std::function<void()>)> queueFunc)
	{
		mQueueCommandFunc = queueFunc;
	}

private:
	ExecutionData* findExecutionAssumeLocked(uint32 threadId);
	void pauseExecution(ExecutionData& exec, Mutex::Locker& locker);
	void resumeExecution(ExecutionData& exec);
	ExecutionPanel* createExecutionPanel(ExecutionData& exec, TVector2<int> const& size, bool bTheadSafe);

	Mutex mThreadDataMutex;
	ConditionVariable mCV;
	TArray< ExecutionData > mRunningExecutions;
	std::function<void(std::function<void()>)> mQueueCommandFunc;
};

#endif // ExecutionManager_h__
