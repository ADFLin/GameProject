#pragma once
#ifndef BRDFTestStage_H_7A995FB3_7E3D_4113_8393_AD185D2EDB89
#define BRDFTestStage_H_7A995FB3_7E3D_4113_8393_AD185D2EDB89

#include "Stage/TestRenderStageBase.h"
#include "RHI/SceneRenderer.h"

#include "Renderer/IBLResource.h"
#include "Renderer/Tonemap.h"

namespace Render
{
	struct GPU_ALIGN LightProbeVisualizeParams
	{
		DECLARE_UNIFORM_BUFFER_STRUCT(LightProbeVisualizeParamsBlock);
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
			gridNum = IntVector2(20, 20);
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
					   mParamTargetTextureSampler, TStaticSamplerState< ESampler::Point, ESampler::Clamp, ESampler::Clamp >::GetRHI());
		}
		ShaderParameter mParamTargetTextureSampler;
		ShaderParameter mParamTargetTexture;
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


		TStructuredBuffer< LightProbeVisualizeParams > mParamBuffer;

		FrameRenderTargets mSceneRenderTargets;

		RHITexture2DRef mNormalTexture;
		RHITexture2DRef mHDRImage;
		RHITexture2DRef mRockTexture;
		bool mbUseShaderBlit = false;


		IBLResource mIBLResource;

		ERenderSystem getDefaultRenderSystem() override
		{
			return ERenderSystem::D3D11;
		}
		void configRenderSystem(ERenderSystem systenName, RenderSystemConfigs& systemConfigs)
		{
			BaseClass::configRenderSystem(systenName, systemConfigs);
			systemConfigs.screenWidth = 1024;
			systemConfigs.screenHeight = 768;
		}
		bool setupRenderResource(ERenderSystem systemName) override;
		void preShutdownRenderSystem(bool bReInit = false) override;
		bool isRenderSystemSupported(ERenderSystem systemName) override;

		bool onInit() override;



		void onEnd() override
		{
			BaseClass::onEnd();
		}

		void restart() override
		{

		}

		bool bEnableTonemap = true;
		bool bShowIrradiance = false;
		int SkyboxShowIndex = ESkyboxShow::Normal;

		void onRender(float dFrame) override;

		MsgReply onMouse(MouseMsg const& msg) override
		{
			return BaseClass::onMouse(msg);
		}

		MsgReply onKey(KeyMsg const& msg) override
		{
			if( msg.isDown())
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
