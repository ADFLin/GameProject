#include "TestRenderStageBase.h"

#include "RHI/SceneRenderer.h"

namespace Render
{
	struct IBLResource
	{
		static int const NumPerFilteredLevel = 5;
		RHITextureCubeRef texture;
		RHITextureCubeRef irradianceTexture;
		RHITextureCubeRef perfilteredTexture;
		static TGlobalRHIResource<  RHITexture2D > SharedBRDFTexture;

		bool initializeRHI()
		{
			VERIFY_RETURN_FALSE(texture = RHICreateTextureCube(Texture::eFloatRGBA, 1024));
			VERIFY_RETURN_FALSE(irradianceTexture = RHICreateTextureCube(Texture::eFloatRGBA, 256));
			VERIFY_RETURN_FALSE(perfilteredTexture = RHICreateTextureCube(Texture::eFloatRGBA, 256, NumPerFilteredLevel));

			return true;
		}
	};

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

	class BRDFTestStage : public TestRenderStageBase
	{
		typedef TestRenderStageBase BaseClass;
	public:
		BRDFTestStage() {}

		LightProbeVisualizeParams mParams;

		class LightProbeVisualizeProgram* mProgVisualize;
		class TonemapProgram* mProgTonemap;
		class SkyBoxProgram* mProgSkyBox;
		class EquirectangularToCubeProgram* mProgEquirectangularToCube;
		class IrradianceGenProgram* mProgIrradianceGen;
		class PrefilteredGenProgram* mProgPrefilterdGen;
		class PreIntegrateBRDFGenProgram* mProgPreIntegrateBRDFGen;

		TStructuredUniformBuffer< LightProbeVisualizeParams > mParamBuffer;

		SceneRenderTargets mSceneRenderTargets;

		RHITexture2DRef mNormalTexture;
		RHITexture2DRef mHDRImage;
		RHITexture2DRef mRockTexture;

		Mesh mSkyBox;

		template< class Func >
		void UpdateCubeTexture(OpenGLFrameBuffer& frameBuffer , RHITextureCube& cubeTexture, ShaderProgram& updateShader, int level , Func shaderSetup)
		{
			
			int size = cubeTexture.getSize() >> level;

			RHISetViewport(0, 0, size, size);
			RHISetRasterizerState(TStaticRasterizerState< ECullMode::None >::GetRHI());
			RHISetDepthStencilState(StaticDepthDisableState::GetRHI());
			RHISetBlendState(TStaticBlendState< CWM_RGBA >::GetRHI());
			for( int i = 0; i < Texture::FaceCount; ++i )
			{
				frameBuffer.setTexture(0, cubeTexture, Texture::Face(i) , level );
				GL_BIND_LOCK_OBJECT(frameBuffer);
				GL_BIND_LOCK_OBJECT(updateShader);
				updateShader.setParam(SHADER_PARAM(FaceDir), Texture::GetFaceDir(Texture::Face(i)));
				updateShader.setParam(SHADER_PARAM(FaceUpDir), Texture::GetFaceUpDir(Texture::Face(i)));
				shaderSetup();
				DrawUtility::ScreenRectShader();
			}
		}

		static void GetTextureData(RHITextureCube& texture, Texture::Format format, int level , std::vector< uint8 >& outData)
		{
			int formatSize = Texture::GetFormatSize(format);
			int textureSize = texture.getSize() >> level;
			int faceDataSize = textureSize * textureSize * formatSize;
			outData.resize(6 * faceDataSize);
			OpenGLCast::To(&texture)->bind();
			for( int i = 0; i < Texture::FaceCount; ++i )
			{
				glGetTexImage(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GLConvert::BaseFormat(format), GLConvert::TextureComponentType(format), &outData[faceDataSize*i]);
			}
			OpenGLCast::To(&texture)->unbind();
		}

		static void GetTextureData(RHITexture2D& texture, Texture::Format format, int level , std::vector< uint8 >& outData  )
		{
			int formatSize = Texture::GetFormatSize(format);
			int dataSize = ( texture.getSizeX() >> level ) * ( texture.getSizeY() >> level ) * formatSize;
			outData.resize(dataSize);
			OpenGLCast::To(&texture)->bind();
			glGetTexImage(GL_TEXTURE_2D , level , GLConvert::BaseFormat(format), GLConvert::TextureComponentType(format), &outData[0]);
			OpenGLCast::To(&texture)->unbind();
		}

		struct ImageBaseLightingData
		{
			std::string fileName;
			std::vector< uint8 > EnvMap;
			std::vector< uint8 > diffuseEnvMap;
			std::vector< uint8 > perFilteredEnvMap;
			std::vector< uint8 > envBRDF;
		};



		IBLResource mIBLResource;

		void buildIBLResource(RHITexture2D& EnvTexture, IBLResource& resource );
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
				buildIBLResource( *mHDRImage , mIBLResource );
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
