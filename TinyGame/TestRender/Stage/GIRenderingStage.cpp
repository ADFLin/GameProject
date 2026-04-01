#include "Stage/TestRenderStageBase.h"
#include "Renderer/MeshImportor.h"
#include "Renderer/BasePassRendering.h"
#include "Renderer/RenderTargetPool.h"
#include "RHI/Material.h"
#include "RHI/SceneRenderer.h"
#include "RHI/ShaderManager.h"
#include "ProfileSystem.h"

namespace Render
{
	struct SurfelData
	{
		Vector4 positionAndRadius; // xyz: position, w: radius
		Vector4 normalAndDummy;    // xyz: normal, w: dummy
	};

	class SurfelAllocationProgram : public GlobalShaderProgram
	{
	public:
		DECLARE_SHADER_PROGRAM(SurfelAllocationProgram, Global);

		static char const* GetShaderFileName() { return "Shader/SurfelAllocation"; }
		static TArrayView< ShaderEntryInfo const > GetShaderEntries()
		{
			static ShaderEntryInfo const entries[] = 
			{ 
				{ EShader::Compute , SHADER_ENTRY(SurfelAllocationCS) } 
			};
			return entries;
		}

		void bindParameters(ShaderParameterMap const& parameterMap)
		{
			mGBufferParams.bindParameters(parameterMap);

			BIND_SHADER_PARAM(parameterMap, SurfelBuffer);
			BIND_SHADER_PARAM(parameterMap, SurfelCounter);
			BIND_SHADER_PARAM(parameterMap, SceneDepth);
		}

		GBufferShaderParameters mGBufferParams;
		DEFINE_SHADER_PARAM(SurfelBuffer);
		DEFINE_SHADER_PARAM(SurfelCounter);
		DEFINE_SHADER_PARAM(SceneDepth);
	};

	IMPLEMENT_SHADER_PROGRAM(SurfelAllocationProgram);

	class SurfelDisplayProgram : public GlobalShaderProgram
	{
	public:
		DECLARE_SHADER_PROGRAM(SurfelDisplayProgram, Global);

		static char const* GetShaderFileName() { return "Shader/SurfelAllocation"; }
		static TArrayView< ShaderEntryInfo const > GetShaderEntries()
		{
			static ShaderEntryInfo const entries[] = {
				{ EShader::Vertex , SHADER_ENTRY(SurfelDisplayVS) },
				{ EShader::Pixel , SHADER_ENTRY(SurfelDisplayPS) },
			};
			return entries;
		}

		void bindParameters(ShaderParameterMap const& parameterMap)
		{
			BIND_SHADER_PARAM(parameterMap, SurfelBuffer);
			BIND_SHADER_PARAM(parameterMap, SurfelCounter);
		}

		DEFINE_SHADER_PARAM(SurfelBuffer);
		DEFINE_SHADER_PARAM(SurfelCounter);
	};

	IMPLEMENT_SHADER_PROGRAM(SurfelDisplayProgram);

	class DebugNormalProgram : public GlobalShaderProgram
	{
	public:
		DECLARE_SHADER_PROGRAM(DebugNormalProgram, Global);

		static char const* GetShaderFileName() { return "Shader/DebugNormal"; }
		static TArrayView< ShaderEntryInfo const > GetShaderEntries()
		{
			static ShaderEntryInfo const entries[] = {
				{ EShader::Vertex , SHADER_ENTRY(DebugNormalVS) },
				{ EShader::Pixel , SHADER_ENTRY(DebugNormalPS) },
			};
			return entries;
		}

		void bindParameters(ShaderParameterMap const& parameterMap)
		{
		}
	};

	IMPLEMENT_SHADER_PROGRAM(DebugNormalProgram);

	class GIRenderingStage : public TestRenderStageBase
	{
		using BaseClass = TestRenderStageBase;
	public:
		GIRenderingStage() {}

		Mesh mModelMesh;
		Matrix4 mModelXForm;

		FrameRenderTargets mSceneRenderTargets;
		RHIFrameBufferRef  mGBufferFrameBuffer;
		MaterialMaster     mMaterial;

		TStructuredBuffer<SurfelData> mSurfelBuffer;
		RHIBufferRef mSurfelCounter;

		bool bShowGBuffer = true;
		bool bDebugVertexNormal = false;

		bool onInit() override
		{
			if (!BaseClass::onInit())
				return false;

			::Global::GUI().cleanupWidget();
			auto frame = WidgetUtility::CreateDevFrame();
			frame->addCheckBox("Show GBuffer", bShowGBuffer);
			frame->addCheckBox("Debug Vertex Normal", bDebugVertexNormal);

			mCamera.lookAt(Vector3(10, 10, 10), Vector3(0, 0, 0), Vector3(0, 0, 1));
			return true;
		}

		ERenderSystem getDefaultRenderSystem() override 
		{ 
			return ERenderSystem::None; 
		}

		void configRenderSystem(ERenderSystem systenName, RenderSystemConfigs& systemConfigs) override
		{
			systemConfigs.screenWidth = 1024;
			systemConfigs.screenHeight = 768;
		}

		bool setupRenderResource(ERenderSystem systemName) override
		{
			VERIFY_RETURN_FALSE(BaseClass::setupRenderResource(systemName));
			VERIFY_RETURN_FALSE(mSceneRenderTargets.initializeRHI());
			VERIFY_RETURN_FALSE(mGBufferFrameBuffer = RHICreateFrameBuffer());
			VERIFY_RETURN_FALSE(mMaterial.loadFile("SimpleTest2"));

			mSurfelBuffer.initializeResource(100000, EStructuredBufferType::RWBuffer);

			uint32 initValue = 0;
			mSurfelCounter =  RHICreateBuffer(sizeof(uint32), 1, BCF_CreateUAV | BCF_CreateSRV, &initValue);

			MeshImportData importData;
			mModelXForm = Matrix4::Scale(0.01);
			if (LoadMeshDataFromFile(importData, "Model/BistroExterior.fbx"))
			{
				mModelMesh.mInputLayoutDesc = importData.desc;
				mModelMesh.mType = EPrimitive::TriangleList;
				mModelMesh.createRHIResource(importData);
			}

			return true;
		}

		void preShutdownRenderSystem(bool bReInit = false) override
		{
			mModelMesh.releaseRHIResource();
			mSceneRenderTargets.releaseRHI();
			mGBufferFrameBuffer.release();
			mMaterial.releaseRHI();
			mSurfelBuffer.releaseResource();
			mSurfelCounter.release();
			BaseClass::preShutdownRenderSystem(bReInit);
		}

		struct MeshProcesserBasePass
		{
			MeshProcesserBasePass(RHICommandList& commandList, ViewInfo& view, MaterialMaster& material)
				:commandList(commandList), view(view)
			{
				shaderProgram = material.getShaderT< DeferredBasePassProgram >(nullptr);
				if (shaderProgram)
				{
					RHISetShaderProgram(commandList, shaderProgram->getRHI());
					view.setupShader(commandList, *shaderProgram);
				}
			}

			void process(Mesh& mesh, Matrix4 const& localToWorld)
			{
				if (!shaderProgram) return;
				Matrix4 worldToLocal;
				float det;
				localToWorld.inverseAffine(worldToLocal, det);
				shaderProgram->setParam(commandList, SHADER_PARAM(Primitive.localToWorld), localToWorld);
				shaderProgram->setParam(commandList, SHADER_PARAM(Primitive.worldToLocal), worldToLocal);
				mesh.draw(commandList, LinearColor(1, 1, 1, 1));
			}

			DeferredBasePassProgram* shaderProgram;
			ViewInfo& view;
			RHICommandList& commandList;
		};

		void onRender(float time) override
		{
			RHICommandList& commandList = RHICommandList::GetImmediateList();
			initializeRenderState();

			IntVector2 screenSize = ::Global::GetScreenSize();
			mSceneRenderTargets.prepare(screenSize, 1);

			{
				GPU_PROFILE("Base Pass");
				RHIResource* transitionResources[EGBuffer::Count + 2];
				transitionResources[0] = &mSceneRenderTargets.getFrameTexture();
				transitionResources[1] = &mSceneRenderTargets.getDepthTexture();
				for (int i = 0; i < EGBuffer::Count; ++i)
					transitionResources[i + 2] = &mSceneRenderTargets.getGBuffer().getRenderTexture((EGBuffer::Type)i);
				
				RHIResourceTransition(commandList, transitionResources, EResourceTransition::RenderTarget);

				mGBufferFrameBuffer->setTexture(0, mSceneRenderTargets.getFrameTexture());
				mSceneRenderTargets.getGBuffer().attachToBuffer(*mGBufferFrameBuffer);
				mGBufferFrameBuffer->setDepth(mSceneRenderTargets.getDepthTexture());

				RHISetFrameBuffer(commandList, mGBufferFrameBuffer);
				LinearColor clearColors[EGBuffer::Count + 1] = { LinearColor(0, 0, 0, 1),LinearColor(0, 0, 0, 0),LinearColor(0, 0, 0, 0),LinearColor(0, 0, 0, 0),LinearColor(0, 0, 0, 0) };
				RHIClearRenderTargets(commandList, EClearBits::All, clearColors, EGBuffer::Count + 1);
				RHISetViewport(commandList, 0, 0, screenSize.x, screenSize.y);

				RHISetBlendState(commandList, TStaticBlendState<>::GetRHI());
				RHISetRasterizerState(commandList, TStaticRasterizerState<ECullMode::Back>::GetRHI());
				RHISetDepthStencilState(commandList, TStaticDepthStencilState<>::GetRHI());

				MeshProcesserBasePass processer(commandList, mView, mMaterial);
				processer.process(mModelMesh, mModelXForm);
				RHISetFrameBuffer(commandList, nullptr);
			}

			{
				GPU_PROFILE("Surfel Allocation");
				uint32 zeroValue = 0;
				RHIUpdateBuffer(commandList, *mSurfelCounter, 0, 1, &zeroValue);

				// Transition G-Buffer and Depth for SRV reading
				RHIResource* gbufferResources[EGBuffer::Count + 1];
				gbufferResources[0] = &mSceneRenderTargets.getDepthTexture();
				for (int i = 0; i < EGBuffer::Count; ++i)
					gbufferResources[i + 1] = &mSceneRenderTargets.getGBuffer().getRenderTexture((EGBuffer::Type)i);
				
				RHIResourceTransition(commandList, gbufferResources, EResourceTransition::SRV);

				RHIResource* uavs[] = { mSurfelBuffer.getRHI(), mSurfelCounter };
				RHIResourceTransition(commandList, uavs, EResourceTransition::UAV);

				SurfelAllocationProgram* shaderProgram = ShaderManager::Get().getGlobalShaderT<SurfelAllocationProgram>();
				RHISetShaderProgram(commandList, shaderProgram->getRHI());
				mView.setupShader(commandList, *shaderProgram); 
				shaderProgram->mGBufferParams.setParameters(commandList, *shaderProgram, mSceneRenderTargets);
				shaderProgram->setTexture(commandList, shaderProgram->SHADER_MEMBER_PARAM(SceneDepth), mSceneRenderTargets.getDepthTexture());

				shaderProgram->setStorageBuffer(commandList, shaderProgram->SHADER_MEMBER_PARAM(SurfelBuffer), *mSurfelBuffer.getRHI(), EAccessOp::ReadAndWrite);
				shaderProgram->setStorageBuffer(commandList, shaderProgram->SHADER_MEMBER_PARAM(SurfelCounter), *mSurfelCounter, EAccessOp::ReadAndWrite);

				uint32 groupX = (mView.rectSize.x + 7) / 8;
				uint32 groupY = (mView.rectSize.y + 7) / 8;
				RHIDispatchCompute(commandList, groupX, groupY, 1);
				
				RHIResourceTransition(commandList, uavs, EResourceTransition::SRV);
			}

			if (bDebugVertexNormal)
			{
				DebugNormalProgram* shaderProgram = ShaderManager::Get().getGlobalShaderT<DebugNormalProgram>();
				RHISetShaderProgram(commandList, shaderProgram->getRHI());
				mView.setupShader(commandList, *shaderProgram);
				Matrix4 worldToLocal;
				float det;
				mModelXForm.inverseAffine(worldToLocal, det);
				shaderProgram->setParam(commandList, SHADER_PARAM(Primitive.localToWorld), mModelXForm);
				shaderProgram->setParam(commandList, SHADER_PARAM(Primitive.worldToLocal), worldToLocal);
				mModelMesh.draw(commandList, LinearColor(1, 1, 1, 1));

			}
			else
			{
				bitBltToBackBuffer(commandList, mSceneRenderTargets.getGBuffer().getResolvedTexture(EGBuffer::B));
			}

			{
				GPU_PROFILE("Surfel Display");
				SurfelDisplayProgram* shaderProgram = ShaderManager::Get().getGlobalShaderT<SurfelDisplayProgram>();
				
				RHISetFrameBuffer(commandList, nullptr);

				RHISetShaderProgram(commandList, shaderProgram->getRHI());
				mView.setupShader(commandList, *shaderProgram);
				shaderProgram->setStorageBuffer(commandList, shaderProgram->SHADER_MEMBER_PARAM(SurfelBuffer), *mSurfelBuffer.getRHI(), EAccessOp::ReadOnly);
				shaderProgram->setStorageBuffer(commandList, shaderProgram->SHADER_MEMBER_PARAM(SurfelCounter), *mSurfelCounter, EAccessOp::ReadOnly);

				RHISetDepthStencilState(commandList, StaticDepthDisableState::GetRHI());
				RHISetRasterizerState(commandList, TStaticRasterizerState<ECullMode::None>::GetRHI());
				RHISetInputStream(commandList, nullptr, nullptr , 0);
				RHIDrawPrimitiveInstanced(commandList, EPrimitive::TriangleList, 0, 6, 81920);
				
				RHISetFrameBuffer(commandList, nullptr);
			}

			if (bShowGBuffer)
			{
				GPU_PROFILE("Draw GBuffer");
				Matrix4 projectMatrix = AdjustProjectionMatrixForRHI(OrthoMatrix(0, screenSize.x, screenSize.y, 0, -1, 1));
				mSceneRenderTargets.getGBuffer().drawTextures(commandList, projectMatrix, screenSize / 4, IntVector2(2, 2));
			}


			GTextureShowManager.registerRenderTarget(GRenderTargetPool);
		}
	};

	REGISTER_STAGE_ENTRY("GI Rendering", GIRenderingStage, EExecGroup::FeatureDev);
}