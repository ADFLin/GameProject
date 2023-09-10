#include "TestRHIStage.h"

namespace Render
{
	REGISTER_STAGE_ENTRY("Test RHI", TestRHIStage, EExecGroup::FeatureDev, 10, "RHI");

	class CubeTextureTest : public TestExample
	{
	public:
		bool enter(TestInitContext& context) override
		{
			mMesh = &context.getAssetData().getMesh(SimpleMeshId::SkyBox);

#define RGBA(r,g,b,a) FColor::ToRGBA(r,g,b,a)
			uint32 FaceColorMap[] = { 
				RGBA(1.0f,0.0f,0.0f,1.0f) , 
				RGBA(0.0f,1.0f,1.0f,1.0f) ,
				RGBA(0.0f,1.0f,0.0f,1.0f) ,
				RGBA(1.0f,0.0f,1.0f,1.0f) ,
				RGBA(0.0f,0.0f,1.0f,1.0f) ,
				RGBA(1.0f,1.0f,0.0f,1.0f) ,
			};

			int texSize = 8;
			int faceOffset = texSize * texSize;
			TArray<uint32> data;
			data.resize(faceOffset*ETexture::FaceCount);
			uint32* pDataList[ETexture::FaceCount];
			for (int face = 0; face < ETexture::FaceCount; ++face)
			{
				pDataList[face] = data.data() + face * faceOffset;
				uint32* pData = pDataList[face];
				for (int index = 0; index < faceOffset; ++index)
				{
					int i = index % texSize;
					int j = index / texSize;
					if (i == j)
					{
						float s = float(i) / texSize;
						pData[index] = RGBA(s, s, s, 1);
					}
					else
					{
						pData[index] = FaceColorMap[face];
					}
				}
			}
#undef RGBA

			mTexCube = RHICreateTextureCube(ETexture::RGBA8, texSize, 0, TCF_DefalutValue, (void**)pDataList);

			TArray<uint8> readData;
			RHIReadTexture(*mTexCube, ETexture::RGBA8, 0, readData);
			mProgSkyBox = context.getAssetData().mProgSkyBox;

			return true;
		}
		void exit() override
		{
			mTexCube.release();
		}
		void render(TestRenderContext& context) override
		{
			ViewInfo& view = context.getView();
			RHICommandList& commandList = RHICommandList::GetImmediateList();
			
			//RHISetFixedShaderPipelineState(commandList, view.worldToClip, LinearColor(1, 0, 0, 1));

			RHISetDepthStencilState(commandList, StaticDepthDisableState::GetRHI());
			RHISetRasterizerState(commandList, TStaticRasterizerState<ECullMode::None>::GetRHI());

			RHISetShaderProgram(commandList, mProgSkyBox->getRHI());
			mProgSkyBox->setTexture(commandList,
				SHADER_PARAM(CubeTexture), mTexCube,
				SHADER_SAMPLER(CubeTexture), TStaticSamplerState< ESampler::Point, ESampler::Clamp, ESampler::Clamp, ESampler::Clamp > ::GetRHI());
			mProgSkyBox->setParam(commandList, SHADER_PARAM(CubeLevel), float(0));

			context.getView().setupShader(commandList, *mProgSkyBox);
			mMesh->draw(commandList);


			RHISetDepthStencilState(commandList, TStaticDepthStencilState<>::GetRHI());
			RHISetFixedShaderPipelineState(commandList, view.worldToClip);
			DrawUtility::AixsLine(commandList);

			Matrix4 porjectMatrix = AdjProjectionMatrixForRHI(OrthoMatrix(0, view.rectSize.x, 0, view.rectSize.y, -1, 1));
			//DrawUtility::DrawCubeTexture(commandList, porjectMatrix,  *mTexCube, Vec2i(0, 0), 100);

			DrawUtility::DrawCubeTexture(commandList, porjectMatrix, *mTexCube, Vec2i(0, 0), Vec2i(400,200));
		}


		RHITextureCubeRef mTexCube;
		SkyBoxProgram* mProgSkyBox = nullptr;
		Mesh* mMesh;

	};
	bool TestRHIStage::onInit()
	{
		if (!BaseClass::onInit())
			return false;

		::Global::GUI().cleanupWidget();

		registerTest<CubeTextureTest>("CubeTest");
		return true;
	}

	void TestRHIStage::onEnd()
	{
		BaseClass::onEnd();
	}

	void TestRHIStage::onRender(float dFrame)
	{
		Vec2i screenSize = ::Global::GetScreenSize();

		RHICommandList& commandList = RHICommandList::GetImmediateList();

		initializeRenderState();

		if (mCurExample)
		{
			TestRenderContext context(mView);
			mCurExample->render(context);
		}
	}

	void TestRHIStage::changeExample(HashString name)
	{
		auto iter = mExampleMap.find(name);
		if (iter == mExampleMap.end())
			return;

		TestExample* newExample = iter->second();
		if (newExample == nullptr)
			return;

		if (mCurExample)
		{
			mCurExample->exit();
			delete mCurExample;
			mCurExample = nullptr;
		}

		TestInitContext context(*this);
		if (!newExample->enter(context))
		{
			delete newExample;
		}
		mCurExample = newExample;
	}

	bool TestRHIStage::setupRenderResource(ERenderSystem systemName)
	{
		BaseClass::setupRenderResource(systemName);

		VERIFY_RETURN_FALSE(SharedAssetData::createSimpleMesh());
		VERIFY_RETURN_FALSE(SharedAssetData::loadCommonShader());

		changeExample("CubeTest");
		if (mCurExample)
		{
			TestInitContext context(*this);
			if (!mCurExample->enter(context))
			{
				return false;
			}
		}

		return true;
	}

	ERenderSystem TestRHIStage::getDefaultRenderSystem()
	{
		return ERenderSystem::OpenGL;
	}

	bool TestRHIStage::isRenderSystemSupported(ERenderSystem systemName)
	{
		if (systemName == ERenderSystem::D3D12 || systemName == ERenderSystem::Vulkan)
			return false;
		return true;
	}

	void TestRHIStage::preShutdownRenderSystem(bool bReInit)
	{
		if (mCurExample)
		{
			mCurExample->exit();
		}
		BaseClass::preShutdownRenderSystem(bReInit);
	}

}

