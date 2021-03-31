#include "Stage/TestStageHeader.h"

#include "Math/Vector2.h"
#include "Math/Math2D.h"
namespace Laser2D
{
	
	class Trace
	{




	};

	class Device
	{
	public:
		Math::Rotation2D mRotation;
		Math::Vector2    mPos;

		virtual void getCollectionInfo();
		virtual void update();
		virtual void effect();
	};

	class Device_Source : public Device
	{
	public:


	};

	class World
	{



	};

	class TestStage : public StageBase
	{
		using BaseClass = StageBase;
	public:
		TestStage() {}

		bool onInit() override
		{
			if (!BaseClass::onInit())
				return false;
			::Global::GUI().cleanupWidget();
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
			if (!msg.isDown())
				return false;

			switch (msg.getCode())
			{
			case EKeyCode::R: restart(); break;
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


}