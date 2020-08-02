#pragma once
#ifndef BRDFTestStage_H_7A995FB3_7E3D_4113_8393_AD185D2EDB89
#define BRDFTestStage_H_7A995FB3_7E3D_4113_8393_AD185D2EDB89

#include "TestRenderStageBase.h"
#include "RHI/SceneRenderer.h"

#include "Renderer/IBLResource.h"

namespace Render
{
	struct GPU_BUFFER_ALIGN LightProbeVisualizeParams
	{
		DECLARE_BUFFER_STRUCT(LightProbeVisualizeParamsBlock);
		Vector3 xAxis;
		float gridSize;
		Vector3 yAxis;
		float probeRadius;
		Vector3 startPos;
		float dummy2;

		IntVector2 gridNum;
		int dummy[2];

		LightProbeVisualizeParams()
		{
			gridNum = IntVector2(10, 10);
			xAxis = Vector3(1, 0, 0);
			yAxis = Vector3(0, 0, 1);

			gridSize = 1;
			probeRadius = 0.45;
			startPos = -0.5 * Vector3(gridNum.x - 1, 0, gridNum.y - 1) * gridSize;

		}
	};


	class LightProbeVisualizeProgram : public GlobalShaderProgram
	{
	public:
		using BaseClass = GlobalShaderProgram;
		DECLARE_SHADER_PROGRAM(LightProbeVisualizeProgram, Global);

		static char const* GetShaderFileName()
		{
			return "Shader/LightProbeVisualize";
		}
		static TArrayView< ShaderEntryInfo const > GetShaderEntries()
		{
			static ShaderEntryInfo const entries[] =
			{
				{ EShader::Vertex , SHADER_ENTRY(MainVS) },
				{ EShader::Pixel  , SHADER_ENTRY(MainPS) },
			};
			return entries;
		}

		void bindParameters(ShaderParameterMap const& parameterMap) override
		{
			mParamIBL.bindParameters(parameterMap);
		}

		void setParameters(RHICommandList& commandList, IBLResource& IBL)
		{
			mParamIBL.setParameters(commandList, *this, IBL);
		}

		IBLShaderParameters mParamIBL;
	};

	struct PostProcessContext
	{
	public:
		RHITexture2D* getTexture(int slot = 0) const { return mInputTexture[slot]; }
		RHITexture2DRef mInputTexture[4];
	};


	struct PostProcessParameters
	{
		void bindParameters(ShaderParameterMap const& parameterMap)
		{
			for( int i = 0; i < MaxInputNum; ++i )
			{
				FixString<128> name;
				name.format("TextureInput%d", i);
				mParamTextureInput[i].bind(parameterMap, name);
			}
		}

		void setParameters(RHICommandList& commandList, ShaderProgram& shader, PostProcessContext const& context)
		{
			for( int i = 0; i < MaxInputNum; ++i )
			{
				if( !mParamTextureInput[i].isBound() )
					break;
				if( context.getTexture(i) )
					shader.setTexture(commandList, mParamTextureInput[i], *context.getTexture(i));
			}
		}

		static int const MaxInputNum = 4;

		ShaderParameter mParamTextureInput[MaxInputNum];


	};

	class BloomDownsample : public GlobalShaderProgram
	{
	public:

		using BaseClass = GlobalShaderProgram;
		DECLARE_SHADER_PROGRAM(BloomDownsample, Global);

		static char const* GetShaderFileName()
		{
			return "Shader/Bloom";
		}
		static TArrayView< ShaderEntryInfo const > GetShaderEntries()
		{
			static ShaderEntryInfo const entries[] =
			{
				{ EShader::Vertex , SHADER_ENTRY(ScreenVS) },
				{ EShader::Pixel  , SHADER_ENTRY(DownsamplePS) },
			};
			return entries;
		}

		void bindParameters(ShaderParameterMap const& parameterMap) override
		{
			mParamTargetTexture.bind(parameterMap, SHADER_PARAM(TargetTexture));
		}
		void setParameters(RHICommandList& commandList, PostProcessContext const& context, RHITexture2D& targetTexture)
		{
			setTexture(commandList, mParamTargetTexture, targetTexture, 
					   mParamTargetTextureSampler, TStaticSamplerState< Sampler::ePoint, Sampler::eClamp, Sampler::eClamp >::GetRHI());
		}
		ShaderParameter mParamTargetTextureSampler;
		ShaderParameter mParamTargetTexture;
	};

	class TonemapProgram : public GlobalShaderProgram
	{
	public:
		using BaseClass = GlobalShaderProgram;
		DECLARE_SHADER_PROGRAM(TonemapProgram, Global);


		static char const* GetShaderFileName()
		{
			return "Shader/Tonemap";
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
			mParamPostProcess.bindParameters(parameterMap);
		}
		void setParameters(RHICommandList& commandList, PostProcessContext const& context)
		{
			mParamPostProcess.setParameters(commandList, *this, context);
		}
		PostProcessParameters mParamPostProcess;

	};


	class SkyBoxProgram : public GlobalShaderProgram
	{
	public:
		using BaseClass = GlobalShaderProgram;
		DECLARE_SHADER_PROGRAM(SkyBoxProgram, Global);

		static char const* GetShaderFileName()
		{
			return "Shader/SkyBox";
		}
		static TArrayView< ShaderEntryInfo const > GetShaderEntries()
		{
			static ShaderEntryInfo const entries[] =
			{
				{ EShader::Vertex , SHADER_ENTRY(MainVS) },
				{ EShader::Pixel  , SHADER_ENTRY(MainPS) },
			};
			return entries;
		}

		void bindParameters(ShaderParameterMap const& parameterMap)
		{
			BIND_SHADER_PARAM(parameterMap, Texture);
			BIND_SHADER_PARAM(parameterMap, CubeTexture);
			BIND_SHADER_PARAM(parameterMap, CubeLevel);
		}

		DEFINE_SHADER_PARAM(Texture);
		DEFINE_SHADER_PARAM(CubeTexture);
		DEFINE_SHADER_PARAM(CubeLevel);
	};

	class BRDFTestStage : public TestRenderStageBase
	{
		using BaseClass = TestRenderStageBase;
	public:
		BRDFTestStage() {}

		LightProbeVisualizeParams mParams;

		IBLResourceBuilder mBuilder;

		class LightProbeVisualizeProgram* mProgVisualize;
		class TonemapProgram* mProgTonemap;
		class SkyBoxProgram* mProgSkyBox;

		TStructuredBuffer< LightProbeVisualizeParams > mParamBuffer;

		FrameRenderTargets mSceneRenderTargets;

		RHITexture2DRef mNormalTexture;
		RHITexture2DRef mHDRImage;
		RHITexture2DRef mRockTexture;

		Mesh mSkyBox;

		bool mbUseShaderBlit = false;


		IBLResource mIBLResource;

		ERenderSystem getDefaultRenderSystem() override
		{
			return ERenderSystem::D3D11;
		}
		void configRenderSystem(ERenderSystem systemName, RenderSystemConfigs& systemConfigs)
		{
			BaseClass::configRenderSystem(systemName, systemConfigs);
			systemConfigs.screenWidth = 1024;
			systemConfigs.screenHeight = 768;
		}
		virtual bool initializeRHIResource() override;
		virtual void releaseRHIResource(bool bReInit = false) override;

		bool onInit() override;



		void onEnd() override
		{
			BaseClass::onEnd();
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

		}

		bool bEnableTonemap = true;
		bool bShowIrradiance = false;

		struct ESkyboxShow
		{
			enum 
			{
				Normal ,
				Irradiance ,
				Prefiltered_0,
				Prefiltered_1,
				Prefiltered_2,
				Prefiltered_3,
				Prefiltered_4,

				Count ,
			};
		};
		int SkyboxShowIndex = ESkyboxShow::Normal;

		void onRender(float dFrame) override;

		bool onMouse(MouseMsg const& msg) override
		{
			if( !BaseClass::onMouse(msg) )
				return false;
			return true;
		}

		bool onKey(KeyMsg const& msg) override
		{
			if( !msg.isDown())
			{
				switch (msg.getCode())
				{
				case EKeyCode::X:
					break;
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

}

#endif // BRDFTestStage_H_7A995FB3_7E3D_4113_8393_AD185D2EDB89
