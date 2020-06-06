#pragma once
#ifndef BRDFTestStage_H_7A995FB3_7E3D_4113_8393_AD185D2EDB89
#define BRDFTestStage_H_7A995FB3_7E3D_4113_8393_AD185D2EDB89

#include "TestRenderStageBase.h"
#include "RHI/SceneRenderer.h"

namespace Render
{

	struct ImageBaseLightingData;

	struct IBLBuildSetting
	{
		int envSize;

		int irradianceSize;
		int irradianceSampleCount[2];
		int perfilteredSize;
		int prefilterSampleCount;
		int BRDFSampleCount;

		IBLBuildSetting()
		{
			envSize = 1024;
			irradianceSize = 256;
			perfilteredSize = 256;
			irradianceSampleCount[0] = 128;
			irradianceSampleCount[1] = 64;
			prefilterSampleCount = 2048;
			BRDFSampleCount = 2048;
		}
	};

	struct IBLResource
	{
		static int const NumPerFilteredLevel = 5;
		RHITextureCubeRef texture;
		RHITextureCubeRef irradianceTexture;
		RHITextureCubeRef perfilteredTexture;
		static TGlobalRHIResource< RHITexture2D > SharedBRDFTexture;

		static bool InitializeBRDFTexture(void* data)
		{
			SharedBRDFTexture.initialize(RHICreateTexture2D(Texture::eFloatRGBA, 512, 512, 1, 0 , TCF_DefalutValue , data ) );
			return SharedBRDFTexture.isValid();
		}
		static void FillSharedBRDFData(std::vector< uint8 >& outData)
		{
			return ReadTextureData(*IBLResource::SharedBRDFTexture , Texture::eFloatRGBA, 0, outData);

		}
		bool initializeRHI(ImageBaseLightingData& IBLData);
		bool initializeRHI(IBLBuildSetting const& setting);
		void fillData(ImageBaseLightingData& outData);

		static void GetCubeMapData(std::vector< uint8 >& data , Texture::Format format, int size , int level, void* outData[])
		{
			int formatSize = Texture::GetFormatSize(format);
			int textureSize = Math::Max(size >> level, 1);
			int faceDataSize = textureSize * textureSize * formatSize;

			for( int i = 0; i < Texture::FaceCount; ++i )
				outData[i] = &data[i * faceDataSize];
		}

		static void ReadTextureData(RHITextureCube& texture, Texture::Format format, int level, std::vector< uint8 >& outData)
		{
			int formatSize = Texture::GetFormatSize(format);
			int textureSize = Math::Max(texture.getSize() >> level, 1);
			int faceDataSize = textureSize * textureSize * formatSize;
			outData.resize(Texture::FaceCount * faceDataSize);
			OpenGLCast::To(&texture)->bind();
			for( int i = 0; i < Texture::FaceCount; ++i )
			{
				glGetTexImage(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, level, OpenGLTranslate::BaseFormat(format), OpenGLTranslate::TextureComponentType(format), &outData[faceDataSize*i]);
			}
			OpenGLCast::To(&texture)->unbind();
		}

		static void ReadTextureData(RHITexture2D& texture, Texture::Format format, int level, std::vector< uint8 >& outData)
		{
			int formatSize = Texture::GetFormatSize(format);
			int dataSize = Math::Max(texture.getSizeX() >> level, 1) * Math::Max(texture.getSizeY() >> level, 1) * formatSize;
			outData.resize(dataSize);
			OpenGLCast::To(&texture)->bind();
			glGetTexImage(GL_TEXTURE_2D, level, OpenGLTranslate::BaseFormat(format), OpenGLTranslate::TextureComponentType(format), &outData[0]);
			OpenGLCast::To(&texture)->unbind();
		}
	};

	struct ImageBaseLightingData
	{
		int envMapSize;
		std::vector< uint8 > envMap;
		int irradianceSize;
		std::vector< uint8 > irradiance;
		int perFilteredSize;
		std::vector< uint8 > perFiltered[IBLResource::NumPerFilteredLevel];

		template< class OP >
		void serialize(OP op)
		{
			op & envMapSize & irradianceSize & perFilteredSize;
			op & envMap;
			op & irradiance;
			op & perFiltered;
		}
	};

	TYPE_SUPPORT_SERIALIZE_FUNC(ImageBaseLightingData);

	class IBLShaderParameters
	{
	public:
		void bindParameters(ShaderParameterMap const& parameterMap)
		{
			mParamIrradianceTexture.bind(parameterMap, SHADER_PARAM(IrradianceTexture));
			mParamPrefilteredTexture.bind(parameterMap, SHADER_PARAM(PrefilteredTexture));
			mParamPreIntegratedBRDFTexture.bind(parameterMap, SHADER_PARAM(PreIntegratedBRDFTexture));
		}
		void setParameters(RHICommandList& commandList, ShaderProgram& shader , IBLResource& resource)
		{
			if( mParamIrradianceTexture.isBound() )
			{
				shader.setTexture(commandList, mParamIrradianceTexture, resource.irradianceTexture);
			}

			shader.setTexture(commandList, mParamPrefilteredTexture , resource.perfilteredTexture, mParamPrefilteredTextureSampler,
							  TStaticSamplerState< Sampler::eTrilinear, Sampler::eClamp, Sampler::eClamp, Sampler::eClamp > ::GetRHI());

			if ( resource.SharedBRDFTexture.isValid() )
			{
				shader.setTexture(commandList, mParamPreIntegratedBRDFTexture, *resource.SharedBRDFTexture , mParamPreIntegratedBRDFTextureSampler,
								  TStaticSamplerState< Sampler::eBilinear, Sampler::eClamp, Sampler::eClamp > ::GetRHI());
			}
		}

		ShaderParameter mParamIrradianceTexture;
		ShaderParameter mParamPrefilteredTexture;
		ShaderParameter mParamPreIntegratedBRDFTexture;
		ShaderParameter mParamPrefilteredTextureSampler;
		ShaderParameter mParamPreIntegratedBRDFTextureSampler;
	};

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
				{ Shader::eVertex , SHADER_ENTRY(MainVS) },
				{ Shader::ePixel  , SHADER_ENTRY(MainPS) },
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
				{ Shader::eVertex , SHADER_ENTRY(ScreenVS) },
				{ Shader::ePixel  , SHADER_ENTRY(DownsamplePS) },
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
				{ Shader::eVertex , SHADER_ENTRY(ScreenVS) },
				{ Shader::ePixel  , SHADER_ENTRY(MainPS) },
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
				{ Shader::eVertex , SHADER_ENTRY(MainVS) },
				{ Shader::ePixel  , SHADER_ENTRY(MainPS) },
			};
			return entries;
		}
	};



	class IBLResourceBuilder
	{
	public:
		bool loadOrBuildResource(DataCacheInterface& dataCache, char const* path, RHITexture2D& HDRImage, IBLResource& resource, IBLBuildSetting const& setting = IBLBuildSetting() );
		bool buildIBLResource(RHITexture2D& envTexture, IBLResource& resource , IBLBuildSetting const& setting );
		bool initializeShaderProgram();

		class EquirectangularToCubeProgram* mProgEquirectangularToCube = nullptr;
		class IrradianceGenProgram* mProgIrradianceGen = nullptr;
		class PrefilteredGenProgram* mProgPrefilterdGen = nullptr;
		class PreIntegrateBRDFGenProgram* mProgPreIntegrateBRDFGen = nullptr;
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

		SceneRenderTargets mSceneRenderTargets;

		RHITexture2DRef mNormalTexture;
		RHITexture2DRef mHDRImage;
		RHITexture2DRef mRockTexture;

		Mesh mSkyBox;





		IBLResource mIBLResource;

		bool onInit() override;

		void onEnd() override
		{
			ShaderManager::Get().unregisterShaderAssets(::Global::GetAssetManager());
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
