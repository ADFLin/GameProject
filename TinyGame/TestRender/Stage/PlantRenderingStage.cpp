#include "PlantRenderingStage.h"
#include "Renderer/MeshUtility.h"

namespace Render
{

	REGISTER_STAGE_ENTRY("Plant Rendering", PlantRenderingStage, EExecGroup::FeatureDev, "Render");

	IMPLEMENT_SHADER_PROGRAM(GenerateHeightProgram);
	IMPLEMENT_SHADER_PROGRAM(RenderProgram);

	float RandomFloat()
	{
		return float(Global::Random()) / (float)RAND_MAX;
	}

	float RandomFloat(float s, float e)
	{
		return s + (e - s) * float(Global::Random()) / (float)RAND_MAX;
	}

	float BiasFunction(float x, float bias)
	{
		float k = Math::Pow(1 - bias, 3);
		return (x * k) / (x * k - x + 1);
	}

	bool PlantRenderingStage::buildPlantMesh()
	{
		VERIFY_RETURN_FALSE(createMeshBaseData());
		generatePlantHeight();
		VERIFY_RETURN_FALSE(applyHeightToMesh());
		return true;
	}

	bool PlantRenderingStage::createMeshBaseData()
	{
		VERIFY_RETURN_FALSE(FMeshBuild::OctSphere(mMeshData, mMesh.mInputLayoutDesc, 1.0, mSettings.meshLevel));
		VERIFY_RETURN_FALSE(mMeshData.initializeRHI(mMesh));
		VERIFY_RETURN_FALSE(mMeshOffsetData.initializeResource(mMesh.mVertexBuffer->getNumElements(), EStructuredBufferType::RWBuffer, true));
		return true;
	}

	bool PlantRenderingStage::applyHeightToMesh()
	{
		int numVertices = mMesh.getVertexCount();
		float* pHeight = mMeshOffsetData.lock();
		TArray< uint8 > vertexData = mMeshData.vertexData;
		auto posWriter = mMesh.makePositionWriter(vertexData.data());
		for (int i = 0; i < numVertices; ++i)
		{
			Vector3& pos = posWriter[i];
			pos *= pHeight[i];
		}
		mMeshOffsetData.unlock();
		MeshUtility::FillTriangleListNormal(mMesh.mInputLayoutDesc, vertexData.data(), numVertices, mMeshData.indexData.data(), mMeshData.indexData.size(), EVertex::ATTRIBUTE_NORMAL, true);
		VERIFY_RETURN_FALSE(mMesh.createRHIResource(vertexData.data(), numVertices, mMeshData.indexData.data(), mMeshData.indexData.size()));


		return true;
	}

	bool PlantRenderingStage::generateCraterData()
	{
		VERIFY_RETURN_FALSE( mCraterData.initializeResource(mSettings.numCraters, EStructuredBufferType::Buffer) );
		auto pData = mCraterData.lock();
		if (pData)
		{
			for (int i = 0; i < (int)mCraterData.getElementNum(); ++i)
			{
				auto& info = pData[i];

				float theta = Math::PI * RandomFloat();
				float phi = 2 * Math::PI * RandomFloat();
				float ct, st;
				Math::SinCos(theta,st , ct);
				float cp, sp;
				Math::SinCos(phi, sp , cp);
				info.pos = Vector3(st * cp, st * sp, ct);
				info.radius = mSettings.radiusMinMax.x + (mSettings.radiusMinMax.y - mSettings.radiusMinMax.x) * BiasFunction(RandomFloat(), mSettings.radiusDistribution);
			}
			mCraterData.unlock();
		}

		return true;
	}

	void PlantRenderingStage::generatePlantHeight()
	{
		RHICommandList& commandList = RHICommandList::GetImmediateList();

		int32 numVertices = mMesh.mVertexBuffer->getNumElements();
		int32 stride = mMesh.mVertexBuffer->getElementSize() / sizeof(float);
		RHISetShaderProgram(commandList, mProgGenerateHeight->getRHI());

		mProgGenerateHeight->setParam(commandList, SHADER_PARAM(NumVertices), numVertices);
		mProgGenerateHeight->setParam(commandList, SHADER_PARAM(VertexStride), stride);
		mProgGenerateHeight->setStorageBuffer(commandList, SHADER_PARAM(VertexBlock), *mMesh.mVertexBuffer, EAccessOp::ReadOnly);
		mProgGenerateHeight->setStorageBuffer(commandList, SHADER_PARAM(HeightBlock), *mMeshOffsetData.getRHI(), EAccessOp::WriteOnly);

		mProgGenerateHeight->setParam(commandList, SHADER_PARAM(RimWidth), mSettings.rimWidth);
		mProgGenerateHeight->setParam(commandList, SHADER_PARAM(RimSteepness), mSettings.rimSteepness);
		mProgGenerateHeight->setParam(commandList, SHADER_PARAM(FloorHeight), mSettings.floorHeight);
		mProgGenerateHeight->setParam(commandList, SHADER_PARAM(Smoothness), mSettings.smoothness);
		mProgGenerateHeight->setStorageBuffer(commandList, SHADER_PARAM(CraterBlock), *mCraterData.getRHI(), EAccessOp::ReadOnly);
		mProgGenerateHeight->setParam(commandList, SHADER_PARAM(NumCraters), (int)mCraterData.getElementNum());

		RHIDispatchCompute(commandList, (numVertices + 127) / 128, 1, 1);
		RHIFlushCommand(commandList);

	}

	bool PlantRenderingStage::setupRenderResource(ERenderSystem systemName)
	{
		VERIFY_RETURN_FALSE(mProgGenerateHeight = ShaderManager::Get().getGlobalShaderT<GenerateHeightProgram>());
		VERIFY_RETURN_FALSE(mProgRender = ShaderManager::Get().getGlobalShaderT<RenderProgram>());

		generateCraterData();
		VERIFY_RETURN_FALSE(buildPlantMesh());
		return true;
	}

	bool PlantRenderingStage::onInit()
	{
		if (!BaseClass::onInit())
			return false;
		::Global::GUI().cleanupWidget();

		DevFrame* frame = WidgetUtility::CreateDevFrame();

		FWidgetProperty::Bind(frame->addCheckBox(UI_ANY, "Wireframe"), bWireframe);

		FWidgetProperty::Bind(frame->addSlider("Mesh Level"), mSettings.meshLevel, 0, 250,
			[this](int level)
			{
				mPendingBuildMask = EBuildMask::All;
			}
		);
		FWidgetProperty::Bind(frame->addSlider("Craters Num"), mSettings.numCraters, 0, 500,
			[this](int level)
			{
				mPendingBuildMask = EBuildMask::Crater | EBuildMask::PlantHeight;
			}
		);
		FWidgetProperty::Bind(frame->addSlider("Radius Min"), mSettings.radiusMinMax.x, 0.0f, 1.0f,
			[this](float value)
			{
				mPendingBuildMask = EBuildMask::Crater | EBuildMask::PlantHeight;
			}
		);
		FWidgetProperty::Bind(frame->addSlider("Radius Max"), mSettings.radiusMinMax.y, 0.0f, 1.0f,
			[this](float value)
			{
				mPendingBuildMask = EBuildMask::Crater | EBuildMask::PlantHeight;
			}
		);
		FWidgetProperty::Bind(frame->addSlider("Radius Distribution"), mSettings.radiusDistribution, -2.0f, 2.0f,
			[this](float value)
			{
				mPendingBuildMask = EBuildMask::PlantHeight;
			}
		);
		FWidgetProperty::Bind(frame->addSlider("Floor Height"), mSettings.floorHeight, -1.0f, 2.0f,
			[this](float value)
			{
				mPendingBuildMask = EBuildMask::PlantHeight;
			}
		);
		FWidgetProperty::Bind(frame->addSlider("Rim Steepness"), mSettings.rimSteepness, -2.0f, 2.0f,
			[this](float value)
			{
				mPendingBuildMask = EBuildMask::PlantHeight;
			}
		);
		FWidgetProperty::Bind(frame->addSlider("Rim Width"), mSettings.rimWidth, 0.0f, 10.0f,
			[this](float value)
			{
				mPendingBuildMask = EBuildMask::PlantHeight;
			}
		);
		FWidgetProperty::Bind(frame->addSlider("Smoothness"), mSettings.smoothness, 0.0f, 10.0f,
			[this](float value)
			{
				mPendingBuildMask = EBuildMask::PlantHeight;
			}
		);
		restart();
		return true;
	}

	void PlantRenderingStage::onUpdate(long time)
	{
		BaseClass::onUpdate(time);

		if (mPendingBuildMask)
		{
			if (mPendingBuildMask & EBuildMask::MeshBase)
			{
				createMeshBaseData();
			}
			if (mPendingBuildMask & EBuildMask::Crater)
			{
				generateCraterData();
			}
			if (mPendingBuildMask & EBuildMask::PlantHeight)
			{
				generatePlantHeight();
				applyHeightToMesh();
			}

			mPendingBuildMask = 0;
		}

		int frame = time / gDefaultTickTime;
		for (int i = 0; i < frame; ++i)
			tick();

		updateFrame(frame);
	}

	void PlantRenderingStage::onRender(float dFrame)
	{
		initializeRenderState();


		RHICommandList& commandList = RHICommandList::GetImmediateList();
		Vec2i screenSize = ::Global::GetScreenSize();

		mView.frameCount += 1;
		mView.rectOffset = IntVector2(0, 0);
		mView.rectSize = screenSize;

		RHISetFrameBuffer(commandList, nullptr);
		RHIClearRenderTargets(commandList, EClearBits::All, &LinearColor(0.2, 0.2, 0.2, 1), 1);

		RHISetViewport(commandList, mView.rectOffset.x, mView.rectOffset.y, mView.rectSize.x, mView.rectSize.y);
		RHISetDepthStencilState(commandList, TStaticDepthStencilState<>::GetRHI());
		RHISetBlendState(commandList, TStaticBlendState<>::GetRHI());
		RHISetRasterizerState(commandList, TStaticRasterizerState<>::GetRHI());

		RHISetFixedShaderPipelineState(commandList, mView.worldToClip);
		DrawUtility::AixsLine(commandList);

		auto& rasterizerState = (bWireframe) ? TStaticRasterizerState<ECullMode::Back, EFillMode::Wireframe>::GetRHI() : TStaticRasterizerState<>::GetRHI();
		RHISetRasterizerState(commandList, rasterizerState);

		RHISetShaderProgram(commandList, mProgRender->getRHI());

		mProgRender->setParam(commandList, SHADER_PARAM(XForm), mView.worldToClip);
		mProgRender->setStorageBuffer(commandList, SHADER_PARAM(HeightBlock), *mMeshOffsetData.getRHI(), EAccessOp::ReadOnly);
		mView.setupShader(commandList, *mProgRender);

		mMesh.draw(commandList);

		Graphics2D& g = Global::GetGraphics2D();
	}

}//namespace Render