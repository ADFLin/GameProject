#include "TestRenderStageBase.h"

#include "RHI/DrawUtility.h"
#include "RHI/RHIGraphics2D.h"

namespace Render
{

	class GravityFieldProgram : public GlobalShaderProgram
	{
		DECLARE_SHADER_PROGRAM(GravityFieldProgram, Global)
		static void SetupShaderCompileOption(ShaderCompileOption& option)
		{
		}
		static char const* GetShaderFileName()
		{
			return "Shader/Game/GravityField";
		}
		static TArrayView< ShaderEntryInfo const > GetShaderEntries()
		{
			static ShaderEntryInfo const entries[] =
			{
				{ EShader::Vertex , SHADER_ENTRY(ScreenVS) },
				{ EShader::Pixel  , SHADER_ENTRY(FieldRenderPS) },
			};
			return entries;
		}

		void bindParameters(ShaderParameterMap const& parameterMap)
		{
			BIND_SHADER_PARAM(parameterMap, UvToWorldPos);
			BIND_SHADER_PARAM(parameterMap, NumPlants);
			BIND_SHADER_PARAM(parameterMap, GravityScale);
			BIND_SHADER_PARAM(parameterMap, PlantDataTexture);
		}

		DEFINE_SHADER_PARAM(UvToWorldPos);
		DEFINE_SHADER_PARAM(NumPlants);
		DEFINE_SHADER_PARAM(GravityScale);
		DEFINE_SHADER_PARAM(PlantDataTexture);
	};
	IMPLEMENT_SHADER_PROGRAM(GravityFieldProgram);


	class GravityFieldTestStage : public TestRenderStageBase
	{
		using BaseClass = TestRenderStageBase;
	public:
		GravityFieldTestStage() {}

		float mIsolevel = 0.5;

		// pos = (x,y) , mass = z  
		std::vector< Vector3 > mPlantList;
		GravityFieldProgram* mProgFieldRender;
		RHITexture1DRef mPlantDataTexture;
		float mGravityScale = 1.0f;

		bool onInit() override
		{
			if (!BaseClass::onInit())
				return false;


			mPlantList.push_back(Vector3(20, 10, 100));
			mPlantList.push_back(Vector3(300, 430, 400));
			mPlantList.push_back(Vector3(600, 100, 700));
			mPlantList.push_back(Vector3(100, 100, 500));

			::Global::GUI().cleanupWidget();
			auto devFrame = WidgetUtility::CreateDevFrame();
			FWidgetProperty::Bind(devFrame->addSlider(UI_ANY), mGravityScale, 10, 1000, 2);
			return true;
		}

		virtual bool setupRenderResource(ERenderSystem systemName) override
		{
			BaseClass::setupRenderResource(systemName);

			VERIFY_RETURN_FALSE(SharedAssetData::createSimpleMesh());
			//VERIFY_RETURN_FALSE(SharedAssetData::loadCommonShader());
			VERIFY_RETURN_FALSE(mProgFieldRender = ShaderManager::Get().getGlobalShaderT< GravityFieldProgram >());
			VERIFY_RETURN_FALSE(mPlantDataTexture = RHICreateTexture1D(ETexture::RGB32F, mPlantList.size(), 0, 1, TCF_DefalutValue, mPlantList.data()));

			return true;

		}


		virtual void preShutdownRenderSystem(bool bReInit = false) override
		{
			SharedAssetData::releaseRHIResource(bReInit);

			mProgFieldRender = nullptr;
			mPlantDataTexture.release();

			BaseClass::preShutdownRenderSystem(bReInit);
		}

		void onEnd() override
		{

			BaseClass::onEnd();
		}

		void restart() override
		{

		}

		void onUpdate(GameTimeSpan deltaTime) override
		{
			BaseClass::onUpdate(deltaTime);
		}

		void onRender(float dFrame) override
		{
			initializeRenderState();

			RHICommandList& commandList = RHICommandList::GetImmediateList();


			Vec2i screenSize = ::Global::GetScreenSize();
			Matrix4 uvToWorldPos = Matrix4::Translate(Vector3(0, -1, 0)) * Matrix4::Scale(Vector3(screenSize.x ,-screenSize.y , 1));

			RHISetShaderProgram(commandList, mProgFieldRender->getRHI());
			SET_SHADER_PARAM(commandList, *mProgFieldRender, UvToWorldPos, AdjProjectionMatrixForRHI( uvToWorldPos ));
			SET_SHADER_PARAM(commandList, *mProgFieldRender, NumPlants, (int)mPlantList.size());
			SET_SHADER_PARAM(commandList, *mProgFieldRender, GravityScale, mGravityScale);
			SET_SHADER_TEXTURE(commandList, *mProgFieldRender, PlantDataTexture, *mPlantDataTexture);
			DrawUtility::ScreenRect(commandList);

			RHIGraphics2D& g = ::Global::GetRHIGraphics2D();
			g.beginRender();
			g.setTextColor(Color3f(1, 1, 0));
			SimpleTextLayout textLayout;
			textLayout.posX = 10;
			textLayout.posY = 10;
			textLayout.offset = 15;
			g.endRender();
		}

		MsgReply onMouse(MouseMsg const& msg) override
		{
			if (msg.onLeftDown())
			{
				Vector3 data;
				data.x = msg.x();
				data.y = msg.y();
				data.z = 500;
				mPlantList.push_back(data);
				mPlantDataTexture = RHICreateTexture1D(ETexture::RGB32F, mPlantList.size(), 0, 1, TCF_DefalutValue, mPlantList.data());
			}
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

	REGISTER_STAGE_ENTRY("Gravity Field", GravityFieldTestStage, EExecGroup::FeatureDev, 1);
}