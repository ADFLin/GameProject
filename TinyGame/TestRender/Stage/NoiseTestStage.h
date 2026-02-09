#pragma once
#ifndef NoiseTestStage_H_30F74EE6_4F5A_4D25_AABE_DEE26988138F
#define NoiseTestStage_H_30F74EE6_4F5A_4D25_AABE_DEE26988138F

#include "Stage/TestRenderStageBase.h"

#include "RHI/Scene.h"
#include "RHI/SceneRenderer.h"
#include "Renderer/VolumetricLighting.h"

#include "DataCacheInterface.h"
#include "DataStreamBuffer.h"
#include "Random.h"


namespace Render
{
	struct NoiseShaderParamsData
	{
		float time;
		Vector4 FBMFactor;
		RHITexture2DRef randTexture;
		RHITexture2DRef noiseTexture;
		RHITexture3DRef volumeTexture;
		RHITexture3DRef noiseVolumeTexture;

		NoiseShaderParamsData()
		{
			FBMFactor = Vector4(100, 1.0, 0.5, 5);
		}

		void releaseRHI()
		{
			randTexture.release();
			noiseTexture.release();
			volumeTexture.release();
			noiseVolumeTexture.release();
		}
	};

	struct V3
	{
		V3() = default;
		float x, y, z;
	};

	struct Foo
	{
		int a;
		int c;
		Vector3 v;
	};
	static_assert(std::is_standard_layout<Foo>::value, "aa");
	static_assert(std::is_trivially_constructible<Foo>::value, "cc");
	static_assert(std::is_trivial<Foo>::value, "bb");

	class NoiseShaderProgramBase : public GlobalShaderProgram
	{
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
				{ EShader::Vertex , SHADER_ENTRY(ScreenVS) },
				{ EShader::Pixel  , SHADER_ENTRY(MainPS) },
			};
			return entries;
		}


		void bindParameters(ShaderParameterMap const& parameterMap) override
		{
			BIND_SHADER_PARAM(parameterMap, Time);
			BIND_SHADER_PARAM(parameterMap, FBMFactor);
			BIND_TEXTURE_PARAM(parameterMap, RandTexture);
			BIND_TEXTURE_PARAM(parameterMap, NoiseTexture);
			BIND_TEXTURE_PARAM(parameterMap, VolumeTexture);
			BIND_TEXTURE_PARAM(parameterMap, NoiseVolumeTexture);
		}

		void setParameters(RHICommandList& commandList, NoiseShaderParamsData& data)
		{
			if( mParamTime.isBound() )
			{
				setParam(commandList, mParamTime, data.time);
			}
			if( mParamFBMFactor.isBound() )
			{
				setParam(commandList, mParamFBMFactor, data.FBMFactor);
			}
			if( mParamRandTexture.isBound() )
			{
				SET_SHADER_TEXTURE_AND_SAMPLER(commandList, *this , RandTexture, *data.randTexture, TStaticSamplerState<ESampler::Point>::GetRHI());
			}
			if( mParamNoiseTexture.isBound() )
			{
				SET_SHADER_TEXTURE_AND_SAMPLER(commandList, *this, NoiseTexture, *data.noiseTexture, TStaticSamplerState<ESampler::Bilinear>::GetRHI());
			}
			if( mParamVolumeTexture.isBound() )
			{
				SET_SHADER_TEXTURE_AND_SAMPLER(commandList, *this, VolumeTexture, *data.volumeTexture, TStaticSamplerState<ESampler::Bilinear>::GetRHI());
			}
			if( mParamNoiseVolumeTexture.isBound() )
			{
				SET_SHADER_TEXTURE_AND_SAMPLER(commandList, *this, NoiseVolumeTexture, *data.noiseVolumeTexture, TStaticSamplerState<ESampler::Bilinear>::GetRHI());
			}
		}

		DEFINE_SHADER_PARAM(Time);
		DEFINE_SHADER_PARAM(FBMFactor);

		DEFINE_TEXTURE_PARAM(RandTexture);
		DEFINE_TEXTURE_PARAM(NoiseTexture);
		DEFINE_TEXTURE_PARAM(VolumeTexture);
		DEFINE_TEXTURE_PARAM(NoiseVolumeTexture);

	};

	class NoiseShaderTestProgram : public NoiseShaderProgramBase
	{
		using BaseClass = NoiseShaderProgramBase;
		DECLARE_SHADER_PROGRAM(NoiseShaderTestProgram, Global);
	public:
		static TArrayView< ShaderEntryInfo const > GetShaderEntries()
		{
			static ShaderEntryInfo const entries[] =
			{
				{ EShader::Vertex , SHADER_ENTRY(ScreenVS) },
				{ EShader::Pixel  , SHADER_ENTRY(TestPS) },
			};
			return entries;
		}
	};

	IMPLEMENT_SHADER_PROGRAM(NoiseShaderTestProgram);

	template< bool bUseRandTexture, bool bUseFBMFactor = true >
	class TNoiseShaderProgram : public NoiseShaderProgramBase
	{
		using BaseClass = NoiseShaderProgramBase;
		DECLARE_SHADER_PROGRAM(TNoiseShaderProgram, Global);
	public:
		static void SetupShaderCompileOption(ShaderCompileOption& option)
		{
			BaseClass::SetupShaderCompileOption(option);
			option.addDefine(SHADER_PARAM(USE_RAND_TEXTURE), bUseRandTexture);
			option.addDefine(SHADER_PARAM(USE_FBM_FACTOR), bUseFBMFactor);
		}
	};


	struct SmokeParams
	{

		float densityFactor = 1.0;
		float phaseG = 0.3;
		float scatterCoefficient = 0.98;
		float albedo = 1;

		float stepSize = 0.6;

	};

	class SmokeRenderProgram : public NoiseShaderProgramBase
	{
		using BaseClass = NoiseShaderProgramBase;
		DECLARE_SHADER_PROGRAM(SmokeRenderProgram, Global);
	public:
		static char const* GetShaderFileName()
		{
			return "Shader/Game/NoiseTest";
		}

		static TArrayView< ShaderEntryInfo const > GetShaderEntries()
		{
			static ShaderEntryInfo const entries[] =
			{
				{ EShader::Vertex , SHADER_ENTRY(ScreenVS) },
				{ EShader::Pixel  , SHADER_ENTRY(SmokeRenderPS) },
			};
			return entries;
		}

		void bindParameters(ShaderParameterMap const& parameterMap) override
		{
			BaseClass::bindParameters(parameterMap);
			BIND_SHADER_PARAM(parameterMap, VolumeMin);
			BIND_SHADER_PARAM(parameterMap, VolumeSize);
			BIND_SHADER_PARAM(parameterMap, ScatterFactor);
			BIND_SHADER_PARAM(parameterMap, StepSize);
			BIND_TEXTURE_PARAM(parameterMap, SceneDepthTexture);
		}

		void setParameters(RHICommandList& commandList, ViewInfo& view, NoiseShaderParamsData& data, Vector3 volumeCenter, Vector3 volumeSize, SmokeParams const& smokeParams)
		{
			BaseClass::setParameters(commandList, data);
			view.setupShader(commandList, *this);
			SET_SHADER_PARAM(commandList, *this , VolumeMin, volumeCenter - 0.5 * volumeSize);
			SET_SHADER_PARAM(commandList, *this, VolumeSize, volumeSize);

			Vector4 scatterFactor;
			scatterFactor.x = smokeParams.densityFactor;
			scatterFactor.y = smokeParams.phaseG;
			scatterFactor.z = smokeParams.scatterCoefficient;
			scatterFactor.w = smokeParams.scatterCoefficient / smokeParams.albedo;
			SET_SHADER_PARAM(commandList, *this, ScatterFactor, scatterFactor);
			SET_SHADER_PARAM(commandList, *this, StepSize, smokeParams.stepSize);
		}

		DEFINE_SHADER_PARAM( VolumeMin );
		DEFINE_SHADER_PARAM( VolumeSize );
		DEFINE_SHADER_PARAM( ScatterFactor );
		DEFINE_SHADER_PARAM( StepSize );
		DEFINE_TEXTURE_PARAM(SceneDepthTexture);
	};


	class SmokeBlendProgram : public GlobalShaderProgram
	{
		using BaseClass = GlobalShaderProgram;
		DECLARE_SHADER_PROGRAM(SmokeBlendProgram, Global);
	public:
		static char const* GetShaderFileName()
		{
			return "Shader/Game/NoiseTest";
		}

		static TArrayView< ShaderEntryInfo const > GetShaderEntries()
		{
			static ShaderEntryInfo const entries[] =
			{
				{ EShader::Vertex , SHADER_ENTRY(ScreenVS) },
				{ EShader::Pixel  , SHADER_ENTRY(SmokeBlendPS) },
			};
			return entries;
		}

		void bindParameters(ShaderParameterMap const& parameterMap) override
		{
			BaseClass::bindParameters(parameterMap);
			BIND_TEXTURE_PARAM(parameterMap, FrameTexture);
			BIND_TEXTURE_PARAM(parameterMap, HistroyTexture);
			BIND_TEXTURE_PARAM(parameterMap, SceneDepthTexture);
		}

		void setParameters(RHICommandList& commandList, ViewInfo& view, RHITexture2D& frameTexture, RHITexture2D& historyTexture)
		{
			view.setupShader(commandList, *this);
			SET_SHADER_TEXTURE_AND_SAMPLER(commandList, *this, FrameTexture, frameTexture, TStaticSamplerState<ESampler::Bilinear>::GetRHI());
			if( mParamHistroyTexture.isBound() )
			{
				SET_SHADER_TEXTURE_AND_SAMPLER(commandList, *this, HistroyTexture, historyTexture, TStaticSamplerState<ESampler::Bilinear>::GetRHI());
			}
		}

		DEFINE_TEXTURE_PARAM(FrameTexture);
		DEFINE_TEXTURE_PARAM(HistroyTexture);
		DEFINE_TEXTURE_PARAM(SceneDepthTexture);
	};


	class ResovleDepthProgram : public GlobalShaderProgram
	{
		using BaseClass = GlobalShaderProgram;
		DECLARE_SHADER_PROGRAM(ResovleDepthProgram, Global);
	public:
		static char const* GetShaderFileName()
		{
			return "Shader/ResolveRenderTarget";
		}

		static TArrayView< ShaderEntryInfo const > GetShaderEntries()
		{
			static ShaderEntryInfo const entries[] =
			{
				{ EShader::Vertex , SHADER_ENTRY(ScreenVS) },
				{ EShader::Pixel  , SHADER_ENTRY(ResolveDepthPS) },
			};
			return entries;
		}
	};



	IMPLEMENT_SHADER_PROGRAM(ResovleDepthProgram);


	class NoiseTestStage : public TestRenderStageBase

	{
		using BaseClass = TestRenderStageBase;
	public:
		NoiseTestStage() {}

		NoiseShaderParamsData mData;

		NoiseShaderProgramBase* mProgNoise;
		NoiseShaderProgramBase* mProgNoiseUseTexture;
		NoiseShaderProgramBase* mProgNoiseTest;
		SmokeRenderProgram*     mProgSmokeRender;
		SmokeBlendProgram*      mProgSmokeBlend;

		class PointToRectOutlineProgram* mProgPointToRectOutline;

		ResovleDepthProgram*    mProgResolveDepth;

		TStructuredBuffer< TiledLightInfo > mLightsBuffer;

		int indexFrameTexture = 1;
		RHITexture2DRef    mSmokeFrameTextures[2];
		RHITexture2DRef    mSmokeDepthTexture;
		RHIFrameBufferRef  mSmokeFrameBuffer;
		RHIFrameBufferRef  mResolveFrameBuffer;

		RHIFrameBufferRef  mFrameBuffer;
		RHITexture2DRef    mDepthBuffer;
		RHITexture2DRef    mResolvedDepthBuffer;
		RHITexture2DRef    mScreenBuffer;

		RHITexture2DRef    mGrassTexture;
		Mesh  mGrassMesh;
		Mesh  mGrassMeshOpt;
		bool  mbUseOptMesh = false;

		ShaderProgram    mProgGrass;
		ShaderProgram    mProgGrassInstanced;
		InstancedMesh    mInstancedMesh;
		bool             mbDrawInstaced = true;

		TArray< Vector2 > mImagePixels;

		SmokeParams mSmokeParams;
		float mDensityFactor = 1.0;
		float mPhaseG = 0.3;
		float mScatterCoefficient = 1;
		float mAlbedo = 1;
		TArray< LightInfo > mLights;

		ERenderSystem getDefaultRenderSystem() override
		{
			return ERenderSystem::D3D11;
		}
		void configRenderSystem(ERenderSystem systenName, RenderSystemConfigs& systemConfigs)
		{
			systemConfigs.screenWidth = 1024;
			systemConfigs.screenHeight = 768;
		}

		virtual bool setupRenderResource(ERenderSystem systemName) override;
		virtual void preShutdownRenderSystem(bool bReInit = false) override;

		bool onInit() override;
		void onEnd() override;


		void updateLightToBuffer()
		{
			TiledLightInfo* pLightInfo = mLightsBuffer.lock();
			for( int i = 0; i < mLights.size(); ++i )
			{
				pLightInfo[i].setValue(mLights[i]);
			}
			mLightsBuffer.unlock();
		}

		void drawNoiseImage(RHICommandList& commandList, IntVector2 const& pos, IntVector2 const& size, NoiseShaderProgramBase& shader)
		{
			Vec2i screenSize = ::Global::GetScreenSize();
			GPU_PROFILE("drawNoiseImage");
			if (GRHIViewportOrgToNDCPosY > 0)
			{	
				RHISetViewport(commandList, pos.x, screenSize.y - pos.y - size.y , size.x, size.y);
			}
			else
			{
				RHISetViewport(commandList, pos.x, pos.y, size.x, size.y);	
			}		
			RHISetShaderProgram(commandList, shader.getRHI());
			shader.setParameters(commandList, mData);
			DrawUtility::ScreenRect(commandList);
		}


		void restart() override
		{
			mData.time = 0;
		}

		void onUpdate(GameTimeSpan deltaTime) override
		{
			BaseClass::onUpdate(deltaTime);
			mData.time += deltaTime;
		}

		void onRender(float dFrame) override;

		MsgReply onMouse(MouseMsg const& msg) override
		{
			return BaseClass::onMouse(msg);
		}

		MsgReply onKey(KeyMsg const& msg) override
		{
			if( msg.isDown() )
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
			switch( id )
			{
			default:
				break;
			}

			return BaseClass::onWidgetEvent(event, id, ui);
		}


	protected:
	};



}//namespace Render

#endif // NoiseTestStage_H_30F74EE6_4F5A_4D25_AABE_DEE26988138F