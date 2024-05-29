#include "TestRenderStageBase.h"
#include "RHI/SceneRenderer.h"

namespace Render
{

	class TemplateRenderingTestStage : public TestRenderStageBase
	{
		using BaseClass = TestRenderStageBase;
	public:
		TemplateRenderingTestStage() {}

		bool onInit() override
		{
			if (!BaseClass::onInit())
				return false;

			IntVector2 screenSize = ::Global::GetScreenSize();
			::Global::GUI().cleanupWidget();

			return true;
		}


		virtual ERenderSystem getDefaultRenderSystem() override
		{
			return ERenderSystem::D3D11;
		}

		virtual void configRenderSystem(ERenderSystem systenName, RenderSystemConfigs& systemConfigs) override
		{

		}


		virtual bool setupRenderResource(ERenderSystem systemName) override
		{
			VERIFY_RETURN_FALSE(BaseClass::setupRenderResource(systemName));

			return true;
		}


		virtual void preShutdownRenderSystem(bool bReInit = false) override
		{
			BaseClass::preShutdownRenderSystem(bReInit);
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
#if 0
	REGISTER_STAGE_ENTRY("Template Rendering", TemplateRenderTestStage, EExecGroup::FeatureDev);
#endif
}