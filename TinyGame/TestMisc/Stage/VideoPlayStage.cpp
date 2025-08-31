#include "Stage/TestStageHeader.h"

#include "Platform/Windows/MediaFoundationHeader.h"



class VideoPlayStage : public StageBase
{
	using BaseClass = StageBase;
public:
	VideoPlayStage() {}

	bool onInit() override
	{
		if (!BaseClass::onInit())
			return false;

		FMediaFoundation::Initialize();

		::Global::GUI().cleanupWidget();
		restart();
		return true;
	}

	void onEnd() override
	{
		BaseClass::onEnd();
	}

	void restart() {}

	void onUpdate(GameTimeSpan deltaTime) override
	{
		BaseClass::onUpdate(deltaTime);
	}

	void onRender(float dFrame) override
	{
		Graphics2D& g = Global::GetGraphics2D();
	}

	MsgReply onMouse(MouseMsg const& msg) override
	{
		return BaseClass::onMouse(msg);
	}

	MsgReply onKey(KeyMsg const& msg) override
	{
		if (msg.isDown())
		{
			switch (msg.getCode())
			{
			case EKeyCode::R: restart(); break;
			}
		}
		return BaseClass::onKey(msg);
	}

	bool onWidgetEvent(int event, int id, GWidget* ui) override
	{
		switch (id)
		{
		default:
			break;
		}

		return BaseClass::onWidgetEvent(event, id, ui);
	}
protected:
};


REGISTER_STAGE_ENTRY("Video Play", VideoPlayStage, EExecGroup::Test, "Render");