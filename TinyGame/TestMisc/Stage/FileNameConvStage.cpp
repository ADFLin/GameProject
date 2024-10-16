#include "Stage/TestStageHeader.h"
#include "FileSystem.h"

#include "DrawEngine.h"

class FileNameConvStage : public StageBase
{
	using BaseClass = StageBase;
public:
	FileNameConvStage() {}

	bool onInit() override
	{
		if (!BaseClass::onInit())
			return false;
		::Global::GUI().cleanupWidget();

		auto frame = WidgetUtility::CreateDevFrame();
		frame->addButton("Conv Path", [this](int event, GWidget*)
		{
			char const* dirPath = "D:\\DownloadsNew";
			char path[1024];
			if (SystemPlatform::OpenDirectoryName(path, ARRAYSIZE(path), dirPath , "Select Convert Directory" , ::Global::GetDrawEngine().getWindowHandle()))
			{
				convFileName(path);
			}

			return true;
		});

		restart();
		return true;
	}

	void onEnd() override
	{
		BaseClass::onEnd();
	}

	void restart() {}

	void convFileName(char const* dirPath)
	{
		FileIterator fileIiter;
		if (FFileSystem::FindFiles(dirPath, nullptr, fileIiter))
		{
			for (; fileIiter.haveMore(); fileIiter.goNext())
			{
				if (fileIiter.isDirectory() )
					continue;

				LogMsg(fileIiter.getFileName());
			}
		}
	}

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


REGISTER_STAGE_ENTRY("File Name Conv", FileNameConvStage, EExecGroup::Test);