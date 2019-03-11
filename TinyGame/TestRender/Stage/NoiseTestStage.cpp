#include "Stage/TestStageHeader.h"

#include "RHI/RHICommon.h"
#include "RHI/ShaderCompiler.h"
#include "RHI/RenderContext.h"
#include "RHI/DrawUtility.h"
#include "RHI/MeshUtility.h"
#include "RHI/GpuProfiler.h"

#include "Random.h"


#if 0

class ModifiedPerlinNoise
{





	float getValue(float pos);
	float getValue(Vector2 const& pos);
	float getValue(Vector3 const& pos);
	float getValue(Vector4 const& pos);

	uint32 mValue[512];

};
#endif

namespace Render
{

	class NoiseShaderProgramBase : public GlobalShaderProgram
	{
		DECLARE_GLOBAL_SHADER(NoiseShaderProgramBase);
	public:
		static void SetupShaderCompileOption(ShaderCompileOption& option) 
		{

		}
		static char const* GetShaderFileName()
		{
			return "Shader/Game/NoiseTest";
		}

		static TArrayView< ShaderEntryInfo const > GetShaderEntries()
		{
			static ShaderEntryInfo const entries[] =
			{
				{ Shader::eVertex , SHADER_ENTRY(ScreenVS) },
				{ Shader::ePixel  , SHADER_ENTRY(MainPS) },
			};
			return entries;
		}


		virtual void bindParameters(ShaderParameterMap& parameterMap) override
		{
			parameterMap.bind(mParamTime, SHADER_PARAM(Time));
			parameterMap.bind(mParamRandTexture, SHADER_PARAM(RandTexture));
			parameterMap.bind(mParamFBMFactor, SHADER_PARAM(FBMFactor));
		}

		void setParameters(float time , RHITexture2DRef& randTexture , Vector4 const& FBMFactor)
		{
			if( mParamTime.isBound() )
			{
				setParam(mParamTime, time);
			}
			if( mParamRandTexture.isBound() )
			{
				setTexture(mParamRandTexture, *randTexture, TStaticSamplerState<Sampler::ePoint>::GetRHI());
			}
			if( mParamFBMFactor.isBound() )
			{
				setParam(mParamFBMFactor, FBMFactor);
			}
		}


		ShaderParameter mParamTime;
		ShaderParameter mParamRandTexture;
		ShaderParameter mParamFBMFactor;
	};

	IMPLEMENT_GLOBAL_SHADER(NoiseShaderProgramBase);

	template< bool bUseRandTexture , bool bUseFBMFactor = true >
	class TNoiseShaderProgram : public NoiseShaderProgramBase
	{
		typedef NoiseShaderProgramBase BaseClass;
		DECLARE_GLOBAL_SHADER(TNoiseShaderProgram);
	public:
		static void SetupShaderCompileOption(ShaderCompileOption& option)
		{
			BaseClass::SetupShaderCompileOption(option);
			option.addDefine(SHADER_PARAM(USE_RAND_TEXTURE), bUseRandTexture);
			option.addDefine(SHADER_PARAM(USE_FBM_FACTOR), bUseFBMFactor);
		}
	};

#define IMPLEMENT_NOISE_SHADER( T1 , T2 )\
	typedef TNoiseShaderProgram< T1 , T2 > NoiseShaderProgram##T1##T2;\
	IMPLEMENT_GLOBAL_SHADER( NoiseShaderProgram##T1##T2 );

	IMPLEMENT_NOISE_SHADER(false, false);
	IMPLEMENT_NOISE_SHADER(false, true);
	IMPLEMENT_NOISE_SHADER(true, false);
	IMPLEMENT_NOISE_SHADER(true, true);

	class NoiseTestStage : public StageBase
	{
		typedef StageBase BaseClass;
	public:
		NoiseTestStage() {}


		RHITexture2DRef mRandomTexture;

		virtual bool onInit()
		{
			if( !BaseClass::onInit() )
				return false;

			if( !Global::GetDrawEngine()->initializeRHI(RHITargetName::OpenGL, 1) )
				return false;

			mProgNoise = ShaderManager::Get().getGlobalShaderT< TNoiseShaderProgram<true,false> >(true);
			mProgNoiseUseTexture = ShaderManager::Get().getGlobalShaderT< TNoiseShaderProgram<true,true> >(true);
			Random::Well512 rand;
			int randSize = 512;
			std::vector< float > randomData(randSize * randSize);
			for( float& v : randomData )
			{
				v = double(rand.rand()) / double(std::numeric_limits<uint32>::max());
			}
			mRandomTexture = RHICreateTexture2D(Texture::eR32F, randSize, randSize, 1, TCF_DefalutValue , randomData.data() , 1);

			ShaderManager::Get().registerShaderAssets(::Global::GetAssetManager());

			::Global::GUI().cleanupWidget();

			auto devFrame = WidgetUtility::CreateDevFrame();
			WidgetPropery::Bind(devFrame->addSlider(UI_ANY), mFBMFactor.x, 0, 20);
			WidgetPropery::Bind(devFrame->addSlider(UI_ANY), mFBMFactor.y, 0, 3);
			WidgetPropery::Bind(devFrame->addSlider(UI_ANY), mFBMFactor.z, 0, 2);
			WidgetPropery::Bind(devFrame->addSlider(UI_ANY), mFBMFactor.w, 0, 20);
			restart();
			return true;
		}

		virtual void onEnd()
		{
			ShaderManager::Get().unregisterShaderAssets(::Global::GetAssetManager());
			::Global::GetDrawEngine()->shutdownRHI(true);
			BaseClass::onEnd();
		}


		float mTime = 0;
		Vector4 mFBMFactor = Vector4(100, 1.0 , 0.5 , 5);

		NoiseShaderProgramBase* mProgNoise;
		NoiseShaderProgramBase* mProgNoiseUseTexture;
		void drawNoiseImage(IntVector2 const& pos, IntVector2 const& size, NoiseShaderProgramBase& shader)
		{
			GPU_PROFILE("drawNoiseImage");
			RHISetViewport(pos.x, pos.y, size.x, size.y);
			GL_BIND_LOCK_OBJECT(shader);
			shader.setParameters(mTime , mRandomTexture,mFBMFactor);
			DrawUtility::ScreenRectShader();
		}

		void restart() {}
		void tick() {}
		void updateFrame(int frame) {}

		virtual void onUpdate(long time)
		{

			mTime += float(time) / 1000.0f;

			BaseClass::onUpdate(time);

			int frame = time / gDefaultTickTime;
			for( int i = 0; i < frame; ++i )
				tick();

			updateFrame(frame);
		}

		void onRender(float dFrame)
		{
			glClearColor(0.2, 0.2, 0.2, 1);
			glClearDepth(1.0);
			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

			drawNoiseImage(IntVector2(25, 100), IntVector2(350, 350), *mProgNoise);
			drawNoiseImage(IntVector2(400, 100), IntVector2(350, 350), *mProgNoiseUseTexture);
		}

		bool onMouse(MouseMsg const& msg)
		{
			if( !BaseClass::onMouse(msg) )
				return false;
			return true;
		}

		bool onKey(unsigned key, bool isDown)
		{
			if( !isDown )
				return false;
			switch( key )
			{
			case Keyboard::eR: restart(); break;
			}
			return false;
		}

		virtual bool onWidgetEvent(int event, int id, GWidget* ui) override
		{
			switch( id )
			{
			default:
				break;
			}

			return BaseClass::onWidgetEvent(event, id, ui);
		}
	protected:
	};



	REGISTER_STAGE2("Noise Test", NoiseTestStage, EStageGroup::FeatureDev, 1);
}