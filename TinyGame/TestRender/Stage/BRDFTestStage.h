#pragma once
#ifndef BRDFTestStage_H_7A995FB3_7E3D_4113_8393_AD185D2EDB89
#define BRDFTestStage_H_7A995FB3_7E3D_4113_8393_AD185D2EDB89

#include "TestRenderStageBase.h"
#include "RHI/SceneRenderer.h"

namespace Render
{

	struct ImageBaseLightingData;

	struct IBLResource
	{
		static int const NumPerFilteredLevel = 5;
		RHITextureCubeRef texture;
		RHITextureCubeRef irradianceTexture;
		RHITextureCubeRef perfilteredTexture;
		static TGlobalRHIResource< RHITexture2D > SharedBRDFTexture;

		static bool InitializeBRDFTexture(void* data)
		{
			SharedBRDFTexture.Initialize(RHICreateTexture2D(Texture::eFloatRGBA, 512, 512, 1, 0 , TCF_DefalutValue , data ) );
			return SharedBRDFTexture.isValid();
		}
		static void FillSharedBRDFData(std::vector< uint8 >& outData)
		{
			return GetTextureData(*IBLResource::SharedBRDFTexture.getRHI(), Texture::eFloatRGBA, 0, outData);

		}
		bool initializeRHI(ImageBaseLightingData* IBLData);

		void fillData(ImageBaseLightingData& outData);
		static void GetCubeMapData(std::vector< uint8 >& data , Texture::Format format, int size , int level, void* outData[])
		{
			int formatSize = Texture::GetFormatSize(format);
			int textureSize = Math::Max(size >> level, 1);
			int faceDataSize = textureSize * textureSize * formatSize;

			for( int i = 0; i < Texture::FaceCount; ++i )
				outData[i] = &data[i * faceDataSize];
		}



		static void GetTextureData(RHITextureCube& texture, Texture::Format format, int level, std::vector< uint8 >& outData)
		{
			int formatSize = Texture::GetFormatSize(format);
			int textureSize = Math::Max(texture.getSize() >> level, 1);
			int faceDataSize = textureSize * textureSize * formatSize;
			outData.resize(Texture::FaceCount * faceDataSize);
			OpenGLCast::To(&texture)->bind();
			for( int i = 0; i < Texture::FaceCount; ++i )
			{
				glGetTexImage(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, level, GLConvert::BaseFormat(format), GLConvert::TextureComponentType(format), &outData[faceDataSize*i]);
			}
			OpenGLCast::To(&texture)->unbind();
		}

		static void GetTextureData(RHITexture2D& texture, Texture::Format format, int level, std::vector< uint8 >& outData)
		{
			int formatSize = Texture::GetFormatSize(format);
			int dataSize = Math::Max(texture.getSizeX() >> level, 1) * Math::Max(texture.getSizeY() >> level, 1) * formatSize;
			outData.resize(dataSize);
			OpenGLCast::To(&texture)->bind();
			glGetTexImage(GL_TEXTURE_2D, level, GLConvert::BaseFormat(format), GLConvert::TextureComponentType(format), &outData[0]);
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
			parameterMap.bind(mParamIrradianceTexture, SHADER_PARAM(IrradianceTexture));
			parameterMap.bind(mParamPrefilteredTexture, SHADER_PARAM(PrefilteredTexture));
			parameterMap.bind(mParamPreIntegratedBRDFTexture, SHADER_PARAM(PreIntegratedBRDFTexture));
		}
		void setParameters( ShaderProgram& shader , IBLResource const& resource)
		{
			shader.setTexture(mParamIrradianceTexture, resource.irradianceTexture);
			shader.setTexture(mParamPrefilteredTexture , resource.perfilteredTexture,
							  TStaticSamplerState< Sampler::eTrilinear, Sampler::eClamp, Sampler::eClamp, Sampler::eClamp > ::GetRHI());

			if ( resource.SharedBRDFTexture.getRHI() )
			{
				shader.setTexture(mParamPreIntegratedBRDFTexture, *resource.SharedBRDFTexture.getRHI(),
								  TStaticSamplerState< Sampler::eBilinear, Sampler::eClamp, Sampler::eClamp > ::GetRHI());
			}
		}

		ShaderParameter mParamIrradianceTexture;
		ShaderParameter mParamPrefilteredTexture;
		ShaderParameter mParamPreIntegratedBRDFTexture;
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
		typedef GlobalShaderProgram BaseClass;
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

		void bindParameters(ShaderParameterMap& parameterMap)
		{
			mParamIBL.bindParameters(parameterMap);
		}

		void setParameters(IBLResource const& IBL)
		{
			mParamIBL.setParameters(*this, IBL);
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
		void bindParameters(ShaderParameterMap& parameterMap)
		{
			for( int i = 0; i < MaxInputNum; ++i )
			{
				FixString<128> name;
				parameterMap.bind(mParamTextureInput[i], name.format("TextureInput%d", i));
			}
		}

		void setParameters(ShaderProgram& shader, PostProcessContext const& context)
		{
			for( int i = 0; i < MaxInputNum; ++i )
			{
				if( !mParamTextureInput[i].isBound() )
					break;
				if( context.getTexture(i) )
					shader.setTexture(mParamTextureInput[i], *context.getTexture(i));
			}
		}

		static int const MaxInputNum = 4;

		ShaderParameter mParamTextureInput[MaxInputNum];


	};

	class BloomDownsample : public GlobalShaderProgram
	{
	public:

		typedef GlobalShaderProgram BaseClass;
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

		void bindParameters(ShaderParameterMap& parameterMap)
		{
			parameterMap.bind(mParamTargetTexture, SHADER_PARAM(TargetTexture));
		}
		void setParameters(PostProcessContext const& context, RHITexture2D& targetTexture)
		{
			setTexture(mParamTargetTexture, targetTexture, TStaticSamplerState< Sampler::ePoint, Sampler::eClamp, Sampler::eClamp >::GetRHI());
		}

		ShaderParameter mParamTargetTexture;
	};

	class TonemapProgram : public GlobalShaderProgram
	{
	public:
		typedef GlobalShaderProgram BaseClass;
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

		void bindParameters(ShaderParameterMap& parameterMap)
		{
			mParamPostProcess.bindParameters(parameterMap);
		}
		void setParameters(PostProcessContext const& context)
		{
			mParamPostProcess.setParameters(*this, context);
		}
		PostProcessParameters mParamPostProcess;

	};


	class SkyBoxProgram : public GlobalShaderProgram
	{
	public:
		typedef GlobalShaderProgram BaseClass;
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
		bool loadOrBuildResource( DataCacheInterface& dataCache , char const* path, RHITexture2D& HDRImage , IBLResource& resource);
		bool buildIBLResource(RHITexture2D& envTexture, IBLResource& resource);
		bool initializeShaderProgram();

		class EquirectangularToCubeProgram* mProgEquirectangularToCube = nullptr;
		class IrradianceGenProgram* mProgIrradianceGen = nullptr;
		class PrefilteredGenProgram* mProgPrefilterdGen = nullptr;
		class PreIntegrateBRDFGenProgram* mProgPreIntegrateBRDFGen = nullptr;
	};


	class BRDFTestStage : public TestRenderStageBase
	{
		typedef TestRenderStageBase BaseClass;
	public:
		BRDFTestStage() {}

		LightProbeVisualizeParams mParams;

		IBLResourceBuilder mBuilder;

		class LightProbeVisualizeProgram* mProgVisualize;
		class TonemapProgram* mProgTonemap;
		class SkyBoxProgram* mProgSkyBox;

		TStructuredUniformBuffer< LightProbeVisualizeParams > mParamBuffer;

		SceneRenderTargets mSceneRenderTargets;

		RHITexture2DRef mNormalTexture;
		RHITexture2DRef mHDRImage;
		RHITexture2DRef mRockTexture;

		Mesh mSkyBox;





		IBLResource mIBLResource;

		virtual bool onInit();

		virtual void onEnd()
		{
			ShaderManager::Get().unregisterShaderAssets(::Global::GetAssetManager());
			BaseClass::onEnd();
		}

		void restart()
		{

		}
		void tick()
		{

		}
		void updateFrame(int frame)
		{
		}

		virtual void onUpdate(long time)
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

		void onRender(float dFrame);

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
			case Keyboard::eX:


				
				break;
			default:
				break;
			}
			return BaseClass::onKey(key, isDown);
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

}

#endif // BRDFTestStage_H_7A995FB3_7E3D_4113_8393_AD185D2EDB89
