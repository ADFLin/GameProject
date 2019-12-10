#include "Stage/TestStageHeader.h"
#include "FileSystem.h"


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
			if (SystemPlatform::OpenDirectoryName(path, ARRAYSIZE(path), dirPath , "Select Convert Directory"))
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
	void tick() {}
	void updateFrame(int frame) {}

	void convFileName(char const* dirPath)
	{
		FileIterator fileIiter;
		if (FileSystem::FindFiles(dirPath, nullptr, fileIiter))
		{
			for (; fileIiter.haveMore(); fileIiter.goNext())
			{
				if (fileIiter.isDirectory() )
					continue;

				LogMsg(fileIiter.getFileName());
			}
		}
	}

	void onUpdate(long time) override
	{
		BaseClass::onUpdate(time);

		int frame = time / gDefaultTickTime;
		for (int i = 0; i < frame; ++i)
			tick();

		updateFrame(frame);
	}

	void onRender(float dFrame) override
	{
		Graphics2D& g = Global::GetGraphics2D();
	}

	bool onMouse(MouseMsg const& msg) override
	{
		if (!BaseClass::onMouse(msg))
			return false;
		return true;
	}

	bool onKey(KeyMsg const& msg) override
	{
		if (!msg.isDown() )
			return false;
		switch (msg.getCode())
		{
		case EKeyCode::R: restart(); break;
		}
		return false;
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


REGISTER_STAGE("File Name Conv", FileNameConvStage, EStageGroup::Test);