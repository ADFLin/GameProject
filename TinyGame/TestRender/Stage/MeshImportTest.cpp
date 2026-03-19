#include "BRDFTestStage.h"

#include "Stage/TestRenderStageBase.h"
#include "RHI/RHIUtility.h"
#include "ProfileSystem.h"
#include "Renderer/MeshImportor.h"
#include "MeshImporterGLB.h"
#include "RHI/RHIUtility.h"
#include "Image/ImageData.h"
#include "CString.h"
#include <unordered_map>

namespace Render
{
	class EnvTestProgram : public ShaderProgram
	{
		using BaseClass = ShaderProgram;
		void bindParameters(ShaderParameterMap const& parameterMap) override
		{
			BaseClass::bindParameters(parameterMap);
			mParamIBL.bindParameters(parameterMap);
		}
	public:
		IBLShaderParameters mParamIBL;
	};

	class PongProgram : public ShaderProgram
	{
	public:


	};

	struct SectionMaterial
	{
		RHITexture2DRef baseColorTexture;
		RHITexture2DRef normalTexture;
		RHITexture2DRef metallicRoughnessTexture;
		LinearColor baseColorFactor;
	};

	class GLBMaterialBuildSettings : public MeshImportSettings
	{
	public:
		GLBMaterialBuildSettings(TArray<SectionMaterial>& outMaterials)
			: mMaterials(outMaterials)
		{
		}

		void setupMaterial(int indexMesh, int indexSection, void* pMaterialInfo) override
		{
			GLBMaterialInfo* info = (GLBMaterialInfo*)pMaterialInfo;
			if (mMaterials.size() <= indexSection)
				mMaterials.resize(indexSection + 1);

			auto& sectionMat = mMaterials[indexSection];
			sectionMat.baseColorFactor = info->baseColorFactor;
			sectionMat.baseColorTexture = getTexture(info->images, info->baseColorTexture.imageIndex, true);
			sectionMat.normalTexture = getTexture(info->images, info->normalTexture.imageIndex, false);
			sectionMat.metallicRoughnessTexture = getTexture(info->images, info->metallicRoughnessTexture.imageIndex, false);
		}

		RHITexture2D* getTexture(TArrayView<GLBImageInfo const> const& images, int imageIndex, bool bSRGB)
		{
			if (imageIndex < 0 || imageIndex >= images.size())
				return nullptr;

			auto it = mImageCache.find(imageIndex);
			if (it != mImageCache.end())
				return it->second;

			auto const& imgInfo = images[imageIndex];
			ImageData imageData;
			imageData.width = imgInfo.width;
			imageData.height = imgInfo.height;
			imageData.numComponent = imgInfo.component;
			imageData.dataSize = imgInfo.dataSize;
			imageData.data = (void*)imgInfo.data;

			RHITexture2DRef tex = RHIUtility::CreateTexture2D(imageData, TextureLoadOption().SRGB(bSRGB).AutoMipMap());
			
			InlineString<64> name;
			name.format("GLB_Img_%d%s", imageIndex, bSRGB ? "_s" : "");
			GTextureShowManager.registerTexture(name.c_str(), tex);

			mImageCache[imageIndex] = tex;
			imageData.data = nullptr;
			return tex;
		}

		TArray<SectionMaterial>& mMaterials;
		std::unordered_map<int, RHITexture2DRef> mImageCache;
	};

	class MeshImportTestStage : public BRDFTestStage
	{
		using BaseClass = BRDFTestStage;
	public:
		MeshImportTestStage()
		{


		}

		EnvTestProgram mTestShader;
		ShaderProgram  mPongShader;
		ShaderProgram  mClipCoordShader;
		Mesh mMesh;
		TArray<Mesh> mCharMeshes;
		Mesh mRobotMesh;
		TArray<SectionMaterial> mRobotMaterials;

		RHITexture2DRef mDiffuseTexture;
		RHITexture2DRef mNormalTexture;
		RHITexture2DRef mMetalTexture;
		RHITexture2DRef mRoughnessTexture;
		RHITexture2DRef mDefaultNormalTexture;
		RHITexture2DRef mDefaultMetalTexture;
		RHITexture2DRef mDefaultRoughnessTexture;

		bool mbUseMipMap = true;

		float mSkyLightInstensity = 1.0;


		bool onInit() override
		{
			if( !BaseClass::onInit() )
				return false;

			auto frame = ::Global::GUI().findTopWidget< DevFrame >();
			FWidgetProperty::Bind(frame->addCheckBox(UI_ANY, "Use MinpMap"), mbUseMipMap);
			FWidgetProperty::Bind(frame->addSlider(UI_ANY), mSkyLightInstensity , 0 , 10 );
 			return true;
		}

		ERenderSystem getDefaultRenderSystem() override
		{
			return ERenderSystem::None;
		}
		virtual bool setupRenderResource(ERenderSystem systemName) override
		{
			BaseClass::setupRenderResource(systemName);

			{
				TIME_SCOPE("EnvLightingTest Shader");
				VERIFY_RETURN_FALSE(ShaderManager::Get().loadFile(mTestShader, "Shader/Game/EnvLightingTest", SHADER_ENTRY(MainVS), SHADER_ENTRY(MainPS), nullptr));
			}
			{
				TIME_SCOPE("Pong Shader");
				VERIFY_RETURN_FALSE(ShaderManager::Get().loadFile(mPongShader, "Shader/Game/PongShader", SHADER_ENTRY(MainVS), SHADER_ENTRY(MainPS), nullptr));
			}

			{
				TIME_SCOPE("ClipCoord Shader");
				VERIFY_RETURN_FALSE(ShaderManager::Get().loadFile(mClipCoordShader, "Shader/Game/ClipCoord", SHADER_ENTRY(ScreenVS), SHADER_ENTRY(MainPS), nullptr));
			}
			{
				uint32 colorN = 0xffff8080;
				mDefaultNormalTexture = RHICreateTexture2D(ETexture::RGBA8, 1, 1, 1, 1, TCF_DefalutValue, &colorN);
				uint32 colorM = 0xff101010;
				mDefaultMetalTexture = RHICreateTexture2D(ETexture::RGBA8, 1, 1, 1, 1, TCF_DefalutValue, &colorM);
				uint32 colorR = 0xffffffff;
				mDefaultRoughnessTexture = RHICreateTexture2D(ETexture::RGBA8, 1, 1, 1, 1, TCF_DefalutValue, &colorR);
			}
			{
				TIME_SCOPE("Mesh Texture");
				VERIFY_RETURN_FALSE(mDiffuseTexture = RHIUtility::LoadTexture2DFromFile(::Global::DataCache(), "Mesh/Cerberus/Cerberus_A.tga", TextureLoadOption().SRGB().AutoMipMap().FlipV()));
				VERIFY_RETURN_FALSE(mNormalTexture = RHIUtility::LoadTexture2DFromFile(::Global::DataCache(), "Mesh/Cerberus/Cerberus_N.tga", TextureLoadOption().AutoMipMap().FlipV()));
				VERIFY_RETURN_FALSE(mMetalTexture = RHIUtility::LoadTexture2DFromFile(::Global::DataCache(), "Mesh/Cerberus/Cerberus_M.tga", TextureLoadOption().AutoMipMap().FlipV()));
				VERIFY_RETURN_FALSE(mRoughnessTexture = RHIUtility::LoadTexture2DFromFile(::Global::DataCache(), "Mesh/Cerberus/Cerberus_R.tga", TextureLoadOption().AutoMipMap().FlipV()));

				GTextureShowManager.registerTexture("CD", mDiffuseTexture);
				GTextureShowManager.registerTexture("CN", mNormalTexture);
				GTextureShowManager.registerTexture("CM", mMetalTexture);
				GTextureShowManager.registerTexture("CR", mRoughnessTexture);
			}
			{
				TIME_SCOPE("FBX Mesh");
				auto LoadFBXMesh = [this](Mesh& mesh, char const* filePath)
				{
					return BuildMeshFromFile(mesh, filePath, [](Mesh& mesh, char const* filePath) -> bool
					{
						IMeshImporterPtr importer = MeshImporterRegistry::Get().getMeshImproter("FBX");
						VERIFY_RETURN_FALSE(importer->importFromFile(filePath, mesh, nullptr));
						return true;
					});
				};

				auto LoadFBXMeshes = [this](TArray<Mesh>& meshes, char const* filePath) -> bool
				{
					return BuildMultiMeshFromFile(meshes, filePath, [](TArray<Mesh>& meshes, char const* filePath) -> bool
					{
						IMeshImporterPtr importer = MeshImporterRegistry::Get().getMeshImproter("FBX");
						VERIFY_RETURN_FALSE(importer->importMultiFromFile(filePath, meshes, nullptr));
						return true;
					});
				};

				LoadFBXMeshes(mCharMeshes, "Mesh/Tracer/tracer.FBX");
				LoadFBXMesh(mMesh, "Mesh/Cerberus/Cerberus_LP.FBX");

				{
					TIME_SCOPE("GLB Mesh");
					BuildMeshFromFile(mRobotMesh, "Mesh/robot.glb", [this](Mesh& mesh, char const* filePath) -> bool
					{
						IMeshImporterPtr importer = MeshImporterRegistry::Get().getMeshImproter("GLB");
						GLBMaterialBuildSettings settings(mRobotMaterials);
						VERIFY_RETURN_FALSE(importer->importFromFile(filePath, mesh, &settings));
						return true;
					});
				}
			}

			return true;
		}


		virtual void preShutdownRenderSystem(bool bReInit = false) override
		{
			mTestShader.releaseRHI();

			mDiffuseTexture.release();
			mNormalTexture.release();
			mMetalTexture.release();
			mRoughnessTexture.release();
			mDefaultNormalTexture = nullptr;
			mDefaultMetalTexture = nullptr;
			mDefaultRoughnessTexture = nullptr;
			mMesh.releaseRHIResource();
			mRobotMesh.releaseRHIResource();
			mRobotMaterials.clear();
			for (auto& mesh : mCharMeshes)
			{
				mesh.releaseRHIResource();
			}

			BaseClass::preShutdownRenderSystem();
		}

		void onEnd() override
		{
			mTestShader.releaseRHI();
			BaseClass::onEnd();
		}

		void onRender(float dFrame) override
		{

			Vec2i screenSize = ::Global::GetScreenSize();
			RHICommandList& commandList = RHICommandList::GetImmediateList();
			initializeRenderState();

			mSceneRenderTargets.prepare(screenSize);
#if 0
			//RHISetFrameBuffer(commandList, nullptr);
			RHISetFrameBuffer(commandList, mSceneRenderTargets.getFrameBuffer());
			RHISetShaderProgram(commandList, mClipCoordShader.getRHI());
			DrawUtility::ScreenRect(commandList);
			bitBltToBackBuffer(commandList, mSceneRenderTargets.getFrameTexture());
			return;
#endif
			{
				GPU_PROFILE("Scene");
				mSceneRenderTargets.attachDepthTexture();
				RHISetFrameBuffer(commandList, mSceneRenderTargets.getFrameBuffer());	
				RHIResourceTransition(commandList, { &mSceneRenderTargets.getFrameTexture() }, EResourceTransition::RenderTarget);
				RHIClearRenderTargets(commandList, EClearBits::Color | EClearBits::Depth | EClearBits::Stencil,
					&LinearColor(0.2, 0.2, 0.2, 1), 1);

				drawSkyBox(commandList, mView, *mHDRImage, mIBLResource, SkyboxShowIndex);

				RHISetDepthStencilState(commandList, TStaticDepthStencilState<>::GetRHI());
				RHISetRasterizerState(commandList, TStaticRasterizerState<>::GetRHI());
				{
					RHISetFixedShaderPipelineState(commandList, mView.worldToClip);
					DrawUtility::AixsLine(commandList, 100);
				}
				RHISetShaderProgram(commandList, mTestShader.getRHI());
				mView.setupShader(commandList, mTestShader);
				mTestShader.setParam(commandList, SHADER_PARAM(SkyLightInstensity), mSkyLightInstensity);
				mTestShader.mParamIBL.setParameters(commandList, mTestShader, mIBLResource);

				if(1)
				{
					auto& samplerState = (mbUseMipMap) ? TStaticSamplerState<ESampler::Trilinear>::GetRHI() : TStaticSamplerState<ESampler::Bilinear>::GetRHI();
					mTestShader.setTexture(commandList, SHADER_PARAM(DiffuseTexture), mDiffuseTexture , SHADER_SAMPLER(DiffuseTexture), samplerState);
					mTestShader.setTexture(commandList, SHADER_PARAM(NormalTexture), mNormalTexture, SHADER_SAMPLER(NormalTexture), samplerState);
					mTestShader.setTexture(commandList, SHADER_PARAM(MetalTexture), mMetalTexture, SHADER_SAMPLER(MetalTexture), samplerState);
					mTestShader.setTexture(commandList, SHADER_PARAM(RoughnessTexture), mRoughnessTexture, SHADER_SAMPLER(RoughnessTexture), samplerState);

					SET_SHADER_PARAM(commandList, mTestShader, LocalToWorld, Matrix4::Identity());
					mTestShader.setParam(commandList, SHADER_PARAM(SkyLightInstensity), mSkyLightInstensity);
					mTestShader.mParamIBL.setParameters(commandList, mTestShader, mIBLResource);
					mMesh.draw(commandList);
				}

				if (!mRobotMaterials.empty())
				{
					SET_SHADER_PARAM(commandList, mTestShader, LocalToWorld, Matrix4::Scale(10) * Matrix4::Translate(50,0,0));
					for (int i = 0; i < mRobotMesh.mSections.size(); ++i)
					{
						if (i < mRobotMaterials.size())
						{
							auto& mat = mRobotMaterials[i];
							auto& samplerState = (mbUseMipMap) ? TStaticSamplerState<ESampler::Trilinear>::GetRHI() : TStaticSamplerState<ESampler::Bilinear>::GetRHI();
							mTestShader.setTexture(commandList, SHADER_PARAM(DiffuseTexture), mat.baseColorTexture ? *mat.baseColorTexture : *GDefaultMaterialTexture2D, SHADER_SAMPLER(DiffuseTexture), samplerState);
							mTestShader.setTexture(commandList, SHADER_PARAM(NormalTexture), mat.normalTexture ? *mat.normalTexture : *mDefaultNormalTexture, SHADER_SAMPLER(NormalTexture), samplerState);
							mTestShader.setTexture(commandList, SHADER_PARAM(MetalTexture), mat.metallicRoughnessTexture ? *mat.metallicRoughnessTexture : *mDefaultMetalTexture, SHADER_SAMPLER(MetalTexture), samplerState);
							mTestShader.setTexture(commandList, SHADER_PARAM(RoughnessTexture), mat.metallicRoughnessTexture ? *mat.metallicRoughnessTexture : *mDefaultRoughnessTexture, SHADER_SAMPLER(RoughnessTexture), samplerState);
							// In GLB, Metal and Roughness are in the same texture (B and G channels typically)
							// Our test shader might expect them separately, so we just bind them both to the same texture for now.
						}
						mRobotMesh.drawSection(commandList, i);
					}
				}

				if(0)
				{
					RHISetRasterizerState(commandList, TStaticRasterizerState<ECullMode::None>::GetRHI());
#if 0
					RHISetFixedShaderPipelineState(commandList, mView.worldToClip);
#else
					RHISetShaderProgram(commandList, mPongShader.getRHI());
					mView.setupShader(commandList, mPongShader);
#endif
					for (auto& mesh : mCharMeshes)
					{
						mesh.draw(commandList);
					}
				}
				if ( 0 )
				{
					GPU_PROFILE("LightProbe Visualize");

					RHISetShaderProgram(commandList, mProgVisualize->getRHI());
					mProgVisualize->setStructuredUniformBufferT< LightProbeVisualizeParams >(commandList, *mParamBuffer.getRHI());
					mProgVisualize->setTexture(commandList, SHADER_PARAM(NormalTexture), mNormalTexture);
					mView.setupShader(commandList, *mProgVisualize);
					mProgVisualize->setParameters(commandList, mIBLResource);
					RHIDrawPrimitiveInstanced(commandList, EPrimitive::Quad, 0, 4, mParams.gridNum.x * mParams.gridNum.y);
				}

				RHISetFrameBuffer(commandList, nullptr);
			}

			RHISetDepthStencilState(commandList, StaticDepthDisableState::GetRHI());


			if( bEnableTonemap )
			{
				GPU_PROFILE("Tonemap");
				RHIResourceTransition(commandList, { &mSceneRenderTargets.getFrameTexture() }, EResourceTransition::SRV);
				FTonemap::Render(commandList, mSceneRenderTargets);
			}

			RHIResourceTransition(commandList, { &mSceneRenderTargets.getFrameTexture() }, EResourceTransition::SRV);
			bitBltToBackBuffer(commandList, mSceneRenderTargets.getFrameTexture());
		}
	};

	REGISTER_STAGE_ENTRY("Mesh Import Test", MeshImportTestStage, EExecGroup::FeatureDev, 1, "Render|Test");

}