#ifndef ExecutionPanel_h__
#define ExecutionPanel_h__

#include "TinyCore/GameWidget.h"
#include "TinyCore/StageRegister.h"
#include "PlatformThread.h"
#include <memory>
#include <string>

class ExecutionManager;
class Thread;
class GTextCtrl;

class ExecutionPanel : public GFrame
{
public:
	using BaseClass = GFrame;

	ExecutionPanel(int id, Vec2i const& pos, Vec2i const size, GWidget* parent);

	enum
	{
		UI_TEXT_INPUT = GFrame::NEXT_UI_ID,
		NEXT_UI_ID,
	};

	void addTextInputUI(char const* defaultText);

	virtual bool onChildEvent(int event, int id, GWidget* ui) override;
	void setRenderSize(Vec2i const& size);

	void onRender() override;
	MsgReply onKeyMsg(KeyMsg const& msg) override;

	ExecutionManager* manager = nullptr;
	std::string mName;
	Thread* thread = nullptr;
	ExecutionRenderFunc renderFunc;
	bool bThreadSafe = true;
	volatile int32 renderFlag = 0;
	std::unique_ptr<Mutex> mMutex;

	bool mIsMinimized = false;
	Vec2i mFullSize;
	GTextCtrl* mTextCtrl = nullptr;
};

#endif // ExecutionPanel_h__
