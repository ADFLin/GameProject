#include "TestRenderStageBase.h"
#include "RHI/SceneRenderer.h"

namespace Render
{


	class TemplateRenderTestStage : public TestRenderStageBase
	{
		using BaseClass = TestRenderStageBase;
	public:
		TemplateRenderTestStage() {}

		bool onInit() override
		{
			if (!BaseClass::onInit())
				return false;

			if (!ShaderHelper::Get().init())
				return false;

			IntVector2 screenSize = ::Global::GetDrawEngine().getScreenSize();

			::Global::GUI().cleanupWidget();
			return true;
		}

		void onEnd() override
		{


		}

		void restart() override
		{

		}

		void tick() override 
		{

		}

		void updateFrame(int frame) override 
		{

		}

		void onUpdate(long time) override
		{
			BaseClass::onUpdate(time);
		}

		void onRender(float dFrame) override
		{

		}

		bool onMouse(MouseMsg const& msg) override
		{
			if (!BaseClass::onMouse(msg))
				return false;
			return true;
		}

		bool onKey(KeyMsg const& msg) override
		{
			if (msg.isDown())
			{
				switch (msg.getCode())
				{
				default:
					break;
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

	//REGISTER_STAGE("Template Render", TemplateRenderTestStage, EStageGroup::FeatureDev);

}