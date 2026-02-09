#include "D3D12RayTracingTestStage.h"

#include "RHI/RHIRayTracingTypes.h"
#include "Math/GeometryPrimitive.h"
#include "RHI/RHIGraphics2D.h"
#include "RHI/RHICommand.h"
#include "RHI/RHIUtility.h"
#include "RHI/ShaderManager.h"


#include "RHI/D3D12Common.h"
#include "RHI/D3D12RayTracing.h"
#include "Widget/WidgetUtility.h"

using namespace Render;

struct RTVertex
{
	Vector3 pos;
	Vector2 uv;
};


class RayTracingTestShader : public GlobalShader
{
	DECLARE_SHADER(RayTracingTestShader, Global);
public:
	static char const* GetShaderFileName() { return "Shader/Test/RayTracingTest"; }
	void bindParameters(ShaderParameterMap const& parameterMap)
	{
		BIND_SHADER_PARAM(parameterMap, gOutput);
		BIND_SHADER_PARAM(parameterMap, gDepthOutput);
		BIND_SHADER_PARAM(parameterMap, gScene);
		BIND_TEXTURE_PARAM(parameterMap, gTexture);
		BIND_SHADER_PARAM(parameterMap, gClipToWorld);
		BIND_SHADER_PARAM(parameterMap, gCameraPos);
		BIND_SHADER_PARAM(parameterMap, gFrameIndex);
		BIND_SHADER_PARAM(parameterMap, gLightPos);
		BIND_SHADER_PARAM(parameterMap, gLightRadius);
		BIND_SHADER_PARAM(parameterMap, gNumSamples);
	}
	void setParameters(RHICommandList& commandList, RHITexture2D& outputTexture, RHITexture2D& depthOutput, RHITopLevelAccelerationStructure& tlas, RHITexture2D& testTexture, Render::ViewInfo const& view, uint32 frameIndex, Math::Vector3 const& lightPos, float lightRadius, uint32 numSamples)
	{
		setRWTexture(commandList, mParamgOutput, outputTexture, 0, EAccessOp::WriteOnly);
		setRWTexture(commandList, mParamgDepthOutput, depthOutput, 0, EAccessOp::WriteOnly);
		RHISetShaderAccelerationStructure(commandList, getRHI(), "gScene", &tlas);
		setTexture(commandList, mParamgTexture, testTexture, mParamgTextureSampler, TStaticSamplerState<ESampler::Trilinear>::GetRHI());
		
		SET_SHADER_PARAM(commandList, *this, gClipToWorld, view.clipToWorld);
		SET_SHADER_PARAM(commandList, *this, gCameraPos, view.worldPos);
		SET_SHADER_PARAM(commandList, *this, gFrameIndex, (int32)frameIndex);
		SET_SHADER_PARAM(commandList, *this, gLightPos, lightPos);
		SET_SHADER_PARAM(commandList, *this, gLightRadius, lightRadius);
		SET_SHADER_PARAM(commandList, *this, gNumSamples, (int32)numSamples);
	}
	DEFINE_SHADER_PARAM(gOutput);
	DEFINE_SHADER_PARAM(gDepthOutput);
	DEFINE_SHADER_PARAM(gScene); 
	DEFINE_TEXTURE_PARAM(gTexture);
	DEFINE_SHADER_PARAM(gClipToWorld);
	DEFINE_SHADER_PARAM(gCameraPos);
	DEFINE_SHADER_PARAM(gFrameIndex);
	DEFINE_SHADER_PARAM(gLightPos);
	DEFINE_SHADER_PARAM(gLightRadius);
	DEFINE_SHADER_PARAM(gNumSamples);
};

class RayTracingHitShader : public RayTracingTestShader
{
	DECLARE_SHADER(RayTracingHitShader, Global);
public:
	void bindParameters(ShaderParameterMap const& parameterMap)
	{
		RayTracingTestShader::bindParameters(parameterMap);
		BIND_SHADER_PARAM(parameterMap, gInstanceColor);
		BIND_SHADER_PARAM(parameterMap, gVertices);
	}

	DEFINE_SHADER_PARAM(gInstanceColor);
	DEFINE_SHADER_PARAM(gVertices);
};

class TriangleHitShader : public RayTracingHitShader
{
	DECLARE_SHADER(TriangleHitShader, Global);
};

class FloorHitShader : public RayTracingHitShader
{
	DECLARE_SHADER(FloorHitShader, Global);
};

class RayMissShader : public RayTracingTestShader
{
	DECLARE_SHADER(RayMissShader, Global);
};

IMPLEMENT_SHADER(RayTracingTestShader, EShader::RayGen, SHADER_ENTRY(RayGen));
IMPLEMENT_SHADER(TriangleHitShader, EShader::RayClosestHit, SHADER_ENTRY(TriangleHit));
IMPLEMENT_SHADER(FloorHitShader, EShader::RayClosestHit, SHADER_ENTRY(FloorHit));
IMPLEMENT_SHADER(RayMissShader, EShader::RayMiss, SHADER_ENTRY(RayMiss_Test));

class ShadowDenoiserShader : public GlobalShader
{
	DECLARE_SHADER(ShadowDenoiserShader, Global);
public:
	static char const* GetShaderFileName() { return "Shader/Test/ShadowDenoiser"; }
	void bindParameters(ShaderParameterMap const& parameterMap)
	{
		BIND_SHADER_PARAM(parameterMap, gInput);
		BIND_SHADER_PARAM(parameterMap, gOutput);
		BIND_SHADER_PARAM(parameterMap, gHistory);
		BIND_SHADER_PARAM(parameterMap, gDepth);
		BIND_SHADER_PARAM(parameterMap, gClipToWorld);
		BIND_SHADER_PARAM(parameterMap, gWorldToPrevClip);
		BIND_SHADER_PARAM(parameterMap, gCameraPos);
		BIND_SHADER_PARAM(parameterMap, gDenoiserAlpha);
		BIND_SHADER_PARAM(parameterMap, gSpatialRadius);
		BIND_SHADER_PARAM(parameterMap, gSigmaScale);
		BIND_SHADER_PARAM(parameterMap, gConfidenceScale);
		BIND_SHADER_PARAM(parameterMap, gMaxSpatialRadius);
		BIND_SHADER_PARAM(parameterMap, gHardnessScale);
	}
	void setParameters(RHICommandList& commandList, RHITexture2D& input, RHITexture2D& output, RHITexture2D& history, RHITexture2D& depth, Render::ViewInfo const& view, Render::ViewInfo const& prevView, float alpha, float spatialRadius, float sigmaScale, float confidenceScale, float maxSpatialRadius, float hardnessScale)
	{
		setTexture(commandList, mParamgInput, input);
		setRWTexture(commandList, mParamgOutput, output, 0, EAccessOp::WriteOnly);
		setTexture(commandList, mParamgHistory, history);
		setTexture(commandList, mParamgDepth, depth);
		SET_SHADER_PARAM(commandList, *this, gClipToWorld, view.clipToWorld);
		SET_SHADER_PARAM(commandList, *this, gWorldToPrevClip, prevView.worldToClip);
		SET_SHADER_PARAM(commandList, *this, gCameraPos, view.worldPos);
		SET_SHADER_PARAM(commandList, *this, gDenoiserAlpha, alpha);
		SET_SHADER_PARAM(commandList, *this, gSpatialRadius, (int32)spatialRadius);
		SET_SHADER_PARAM(commandList, *this, gSigmaScale, sigmaScale);
		SET_SHADER_PARAM(commandList, *this, gConfidenceScale, confidenceScale);
		SET_SHADER_PARAM(commandList, *this, gMaxSpatialRadius, maxSpatialRadius);
		SET_SHADER_PARAM(commandList, *this, gHardnessScale, hardnessScale);
	}
	DEFINE_SHADER_PARAM(gInput);
	DEFINE_SHADER_PARAM(gOutput);
	DEFINE_SHADER_PARAM(gHistory);
	DEFINE_SHADER_PARAM(gDepth);
	DEFINE_SHADER_PARAM(gClipToWorld);
	DEFINE_SHADER_PARAM(gWorldToPrevClip);
	DEFINE_SHADER_PARAM(gCameraPos);
	DEFINE_SHADER_PARAM(gDenoiserAlpha);
	DEFINE_SHADER_PARAM(gSpatialRadius);
	DEFINE_SHADER_PARAM(gSigmaScale);
	DEFINE_SHADER_PARAM(gConfidenceScale);
	DEFINE_SHADER_PARAM(gMaxSpatialRadius);
	DEFINE_SHADER_PARAM(gHardnessScale);
};
IMPLEMENT_SHADER(ShadowDenoiserShader, EShader::Compute, SHADER_ENTRY(main));

D3D12RayTracingTestStage::D3D12RayTracingTestStage()
{

}

bool D3D12RayTracingTestStage::onInit()
{
	if (!BaseClass::onInit())
		return false;

	::Global::GUI().cleanupWidget();

	mCamera.lookAt(Vector3(-50, 0, 20), Vector3(0, 0, 0), Vector3(0, 0, 1));
	mCamera.moveSpeedScale = 0.5f;

	auto devFrame = WidgetUtility::CreateDevFrame();
	devFrame->addText("Light Position");
	FWidgetProperty::Bind(devFrame->addSlider(UI_ANY), mLightParams.pos.x, -100.0f, 100.0f);
	FWidgetProperty::Bind(devFrame->addSlider(UI_ANY), mLightParams.pos.y, -100.0f, 100.0f);
	FWidgetProperty::Bind(devFrame->addSlider(UI_ANY), mLightParams.pos.z, 0.0f, 200.0f);
	devFrame->addText("Light Radius");
	FWidgetProperty::Bind(devFrame->addSlider(UI_ANY), mLightParams.radius, 0.0f, 10.0f);
	devFrame->addText("Denoiser Controls");
	FWidgetProperty::Bind(devFrame->addSlider(UI_ANY), mLightParams.denoiserAlpha, 0.001f, 0.2f);
	FWidgetProperty::Bind(devFrame->addSlider(UI_ANY), mLightParams.spatialRadius, 1.0f, 12.0f);
	FWidgetProperty::Bind(devFrame->addSlider(UI_ANY), mLightParams.maxSpatialRadius, 1.0f, 32.0f);
	FWidgetProperty::Bind(devFrame->addSlider(UI_ANY), mLightParams.hardnessScale, 1.0f, 50.0f);
	FWidgetProperty::Bind(devFrame->addSlider(UI_ANY), mLightParams.sigmaScale, 0.5f, 10.0f);
	FWidgetProperty::Bind(devFrame->addSlider(UI_ANY), mLightParams.confidenceScale, 1.0f, 50.0f);
	devFrame->addText("RayTracing");
	FWidgetProperty::Bind(devFrame->addSlider(UI_ANY), mLightParams.numSamples, 1, 64);

	return true;
}

void D3D12RayTracingTestStage::onEnd()
{
	BaseClass::onEnd();
}

void D3D12RayTracingTestStage::onUpdate(GameTimeSpan deltaTime)
{
	BaseClass::onUpdate(deltaTime);
	mCamera.updatePosition(deltaTime);
}

MsgReply D3D12RayTracingTestStage::onKey(KeyMsg const& msg)
{
	if (msg.isDown())
	{
		switch (msg.getCode())
		{
		case EKeyCode::W: mCamera.moveForwardImpulse = 100.0f; break;
		case EKeyCode::S: mCamera.moveForwardImpulse = -100.0f; break;
		case EKeyCode::D: mCamera.moveRightImpulse = 100.0f; break;
		case EKeyCode::A: mCamera.moveRightImpulse = -100.0f; break;
		case EKeyCode::Q: mCamera.moveUpImpulse = 100.0f; break;
		case EKeyCode::E: mCamera.moveUpImpulse = -100.0f; break;
		}
	}
	else
	{
		switch (msg.getCode())
		{
		case EKeyCode::W: if (mCamera.moveForwardImpulse > 0) mCamera.moveForwardImpulse = 0; break;
		case EKeyCode::S: if (mCamera.moveForwardImpulse < 0) mCamera.moveForwardImpulse = 0; break;
		case EKeyCode::D: if (mCamera.moveRightImpulse > 0) mCamera.moveRightImpulse = 0; break;
		case EKeyCode::A: if (mCamera.moveRightImpulse < 0) mCamera.moveRightImpulse = 0; break;
		case EKeyCode::Q: if (mCamera.moveUpImpulse > 0) mCamera.moveUpImpulse = 0; break;
		case EKeyCode::E: if (mCamera.moveUpImpulse < 0) mCamera.moveUpImpulse = 0; break;
		}
	}
	return BaseClass::onKey(msg);
}

MsgReply D3D12RayTracingTestStage::onMouse(MouseMsg const& msg)
{
	if (msg.onLeftDown())
	{
		mLastMousePos = msg.getPos();
	}
	if (msg.isLeftDown())
	{
		if (msg.onMoving())
		{
			float rotateSpeed = 0.01;
			Vector2 diff = rotateSpeed * Vector2(msg.getPos() - mLastMousePos);
			mCamera.rotateByMouse(diff.x, diff.y);
			mLastMousePos = msg.getPos();
		}
	}
	return BaseClass::onMouse(msg);
}

void D3D12RayTracingTestStage::restart()
{

}

bool D3D12RayTracingTestStage::setupRenderResource(ERenderSystem systemName)
{
	if (!GRHISupportRayTracing)
	{
		LogMsg("Ray Tracing is not supported by the current RHI.");
		return true;
	}

	setupRayTracing();
	return true;
}

void D3D12RayTracingTestStage::setupRayTracing()
{
	RHICommandList& commandList = RHICommandList::GetImmediateList();

	// 1. Create Triangle Geometry
	{
		TArray<RTVertex> vertices = 
		{
			{ Vector3(0, 0, 5),   Vector2(0.5f, 0.0f) },
			{ Vector3(5, 0, -5),  Vector2(1.0f, 1.0f) },
			{ Vector3(-5, 0,-5), Vector2(0.0f, 1.0f) }
		};

		BufferDesc bufferDesc;
		bufferDesc.elementSize = sizeof(RTVertex);
		bufferDesc.numElements = (uint32)vertices.size();
		bufferDesc.creationFlags = BCF_UsageVertex | BCF_CreateUAV | BCF_CreateSRV | BCF_Structured; 

		mVertexBuffer = RHICreateBuffer(bufferDesc, vertices.data());
		
		RayTracingGeometryDesc geomDesc;
		geomDesc.type = ERayTracingGeometryType::Triangles;
		geomDesc.vertexBuffer = mVertexBuffer;
		geomDesc.vertexCount = (uint32)vertices.size();
		geomDesc.vertexStride = sizeof(RTVertex);
		geomDesc.vertexFormat = EVertex::Float3;
		geomDesc.bOpaque = true;

		mBLAS = GRHISystem->RHICreateBottomLevelAccelerationStructure(&geomDesc, 1, EAccelerationStructureBuildFlags::PreferFastTrace);
		RHIBuildAccelerationStructure(commandList, mBLAS, nullptr, nullptr);
	}

	// 2. Create Floor Geometry
	{
		TArray<RTVertex> vertices = 
		{
			{ Vector3(-50, -50, -10), Vector2(0,0) },
			{ Vector3( 50, -50, -10), Vector2(1,0) },
			{ Vector3( 50,  50, -10), Vector2(1,1) },
			{ Vector3(-50, -50, -10), Vector2(0,0) },
			{ Vector3( 50,  50, -10), Vector2(1,1) },
			{ Vector3(-50,  50, -10), Vector2(0,1) }
		};

		BufferDesc bufferDesc;
		bufferDesc.elementSize = sizeof(RTVertex);
		bufferDesc.numElements = (uint32)vertices.size();
		bufferDesc.creationFlags = BCF_UsageVertex | BCF_CreateUAV | BCF_CreateSRV | BCF_Structured; 

		mPlaneVertexBuffer = RHICreateBuffer(bufferDesc, vertices.data());
		
		RayTracingGeometryDesc geomDesc;
		geomDesc.type = ERayTracingGeometryType::Triangles;
		geomDesc.vertexBuffer = mPlaneVertexBuffer;
		geomDesc.vertexCount = (uint32)vertices.size();
		geomDesc.vertexStride = sizeof(RTVertex);
		geomDesc.vertexFormat = EVertex::Float3;
		geomDesc.bOpaque = true;

		mPlaneBLAS = GRHISystem->RHICreateBottomLevelAccelerationStructure(&geomDesc, 1, EAccelerationStructureBuildFlags::PreferFastTrace);
		RHIBuildAccelerationStructure(commandList, mPlaneBLAS, nullptr, nullptr);
	}

	// 4. Create Box BLAS
	{
		float s = 0.5f;
		RTVertex vertices[] = {
			// Front
			{ {-s, -s,  s}, {0, 1} }, { { s, -s,  s}, {1, 1} }, { { s,  s,  s}, {1, 0} },
			{ {-s, -s,  s}, {0, 1} }, { { s,  s,  s}, {1, 0} }, { {-s,  s,  s}, {0, 0} },
			// Back
			{ {-s, -s, -s}, {0, 1} }, { {-s,  s, -s}, {0, 0} }, { { s,  s, -s}, {1, 0} },
			{ {-s, -s, -s}, {0, 1} }, { { s,  s, -s}, {1, 0} }, { { s, -s, -s}, {1, 1} },
			// Top
			{ {-s,  s, -s}, {0, 1} }, { {-s,  s,  s}, {0, 0} }, { { s,  s,  s}, {1, 0} },
			{ {-s,  s, -s}, {0, 1} }, { { s,  s,  s}, {1, 0} }, { { s,  s, -s}, {1, 1} },
			// Bottom
			{ {-s, -s, -s}, {0, 1} }, { { s, -s, -s}, {1, 1} }, { { s, -s,  s}, {1, 0} },
			{ {-s, -s, -s}, {0, 1} }, { { s, -s,  s}, {1, 0} }, { {-s, -s,  s}, {0, 0} },
			// Left
			{ {-s, -s, -s}, {0, 1} }, { {-s, -s,  s}, {1, 1} }, { {-s,  s,  s}, {1, 0} },
			{ {-s, -s, -s}, {0, 1} }, { {-s,  s,  s}, {1, 0} }, { {-s,  s, -s}, {0, 0} },
			// Right
			{ { s, -s, -s}, {0, 1} }, { { s,  s, -s}, {0, 0} }, { { s,  s,  s}, {1, 0} },
			{ { s, -s, -s}, {0, 1} }, { { s,  s,  s}, {1, 0} }, { { s, -s,  s}, {1, 1} },
		};

		BufferDesc bufferDesc;
		bufferDesc.elementSize = sizeof(RTVertex);
		bufferDesc.numElements = ARRAY_SIZE(vertices);
		bufferDesc.creationFlags = BCF_UsageVertex | BCF_CreateUAV | BCF_CreateSRV | BCF_Structured;
		mBoxVertexBuffer = RHICreateBuffer(bufferDesc, vertices);

		RayTracingGeometryDesc geomDesc;
		geomDesc.type = ERayTracingGeometryType::Triangles;
		geomDesc.vertexBuffer = mBoxVertexBuffer;
		geomDesc.vertexCount = ARRAY_SIZE(vertices);
		geomDesc.vertexStride = sizeof(RTVertex);
		geomDesc.vertexFormat = EVertex::Float3;
		geomDesc.bOpaque = true;

		mBoxBLAS = GRHISystem->RHICreateBottomLevelAccelerationStructure(&geomDesc, 1, EAccelerationStructureBuildFlags::PreferFastTrace);
		RHIBuildAccelerationStructure(commandList, mBoxBLAS, nullptr, nullptr);
	}

	// 5. Create TLAS with many instances
	const int numBoxes = 1000;
	mTLAS = GRHISystem->RHICreateTopLevelAccelerationStructure(2 + numBoxes, EAccelerationStructureBuildFlags::PreferFastTrace);

	TArray<RayTracingInstanceDesc> instances;
	// Triangle Instance -> Hit Group 0
	{
		RayTracingInstanceDesc desc;
		desc.instanceID = 0;
		desc.recordIndex = 0;
		desc.flags = 0;
		desc.transform = Matrix4::Translate(0, 0, 5);
		desc.blas = mBLAS;
		instances.push_back(desc);
	}

	// Floor Instance -> Hit Group 1
	{
		RayTracingInstanceDesc desc;
		desc.instanceID = 1;
		desc.recordIndex = 1;
		desc.flags = 0;
		desc.transform = Matrix4::Identity();
		desc.blas = mPlaneBLAS;
		instances.push_back(desc);
	}

	// 1000 Boxes -> Hit Group 2...1001
	for (int i = 0; i < numBoxes; ++i)
	{
		RayTracingInstanceDesc desc;
		desc.instanceID = 2 + i;
		desc.recordIndex = 2 + i;
		desc.flags = 0;
		
		int gridX = i % 10;
		int gridY = (i / 10) % 10;
		int gridZ = i / 100;
		desc.transform = Matrix4::Translate((gridX - 5) * 5.0f, (gridZ + 1) * 10.0f, (gridY - 5) * 5.0f);
		desc.blas = mBoxBLAS;
		instances.push_back(desc);
	}
	
	RHIUpdateTopLevelAccelerationStructureInstances(commandList, mTLAS, instances.data(), (uint32)instances.size());
	RHIBuildAccelerationStructure(commandList, mTLAS, nullptr, nullptr);

	RHIFlushCommand(commandList);
	mInstanceLocalData.resize(instances.size());

	// 6. Create PSO
	RayTracingPipelineStateInitializer psoInit;
	
	RayTracingTestShader* rayGenShader = ShaderManager::Get().getGlobalShaderT<RayTracingTestShader>();
	TriangleHitShader* triangleShader = ShaderManager::Get().getGlobalShaderT<TriangleHitShader>();
	FloorHitShader* floorShader = ShaderManager::Get().getGlobalShaderT<FloorHitShader>();
	RayMissShader* missShader = ShaderManager::Get().getGlobalShaderT<RayMissShader>();

	psoInit.rayGenShader = rayGenShader->getRHI();
	psoInit.missShader = missShader->getRHI(); 
	
	// Register Triangle Hit Group at Index 0
	{
		RayTracingShaderGroup group;
		group.closestHitShader = triangleShader->getRHI();
		group.localDataSize = sizeof(HitData);
		psoInit.hitGroups.push_back(group);
	}
	// Register Floor Hit Group at Index 1
	{
		RayTracingShaderGroup group;
		group.closestHitShader = floorShader->getRHI();
		psoInit.hitGroups.push_back(group);
	}

	// Register Box Hit Group template at Index 2
	{
		RayTracingShaderGroup group;
		group.closestHitShader = triangleShader->getRHI();
		group.localDataSize = sizeof(HitData);
		psoInit.hitGroups.push_back(group);
	}
	
	psoInit.maxPayloadSize = sizeof(float) * 6; // float4 color + float depth + uint isShadow
	psoInit.maxRecursionDepth = 2;

	mRTPSO = GRHISystem->RHICreateRayTracingPipelineState(psoInit);
	if (!mRTPSO.isValid()) LogMsg("Failed to create RTPSO");
	else
	{
		mShaderTable = GRHISystem->RHICreateRayTracingShaderTable(mRTPSO);
	}
	
	// Create Output Texture
	IntVector2 screenSize = ::Global::GetScreenSize();
	TextureDesc texDesc = TextureDesc::Type2D(ETexture::RGBA32F, screenSize.x, screenSize.y);
	texDesc.creationFlags = TCF_CreateUAV | TCF_CreateSRV;
	mOutputTexture = RHICreateTexture2D(texDesc);
	mOutputTexture2 = RHICreateTexture2D(texDesc);
	mHistoryTexture = RHICreateTexture2D(texDesc);
	
	TextureDesc depthDesc = TextureDesc::Type2D(ETexture::RG32F, screenSize.x, screenSize.y);
	depthDesc.creationFlags = TCF_CreateUAV | TCF_CreateSRV;
	mDepthTexture = RHICreateTexture2D(depthDesc);

	if (!mOutputTexture.isValid() || !mOutputTexture2.isValid() || !mHistoryTexture.isValid()) LogMsg("Failed to create Output Texture");
	else
	{
		// Load Texture
		mTestTexture = RHIUtility::LoadTexture2DFromFile("Texture/rocks.png");
		if (!mTestTexture.isValid()) LogMsg("Failed to load test texture!");
	}

	if (mShaderTable.isValid())
	{
		// Setup Local Data for each instance once
		uint64 triangleVBAddr = D3D12Cast::GetResource(mVertexBuffer)->GetGPUVirtualAddress();
		uint64 boxVBAddr = D3D12Cast::GetResource(mBoxVertexBuffer)->GetGPUVirtualAddress();

		for (int i = 0; i < (int)mInstanceLocalData.size(); ++i)
		{
			HitData data;
			data.uvScale = Math::Vector3(1, 1, 0);
			if (i >= 2 && (i % 2 != 0)) data.uvScale = Math::Vector3(0.2f, 0.2f, 0);

			if (i == 0) // Triangle
			{
				data.color = Color4f(1, 1, 1, 1);
				data.vertexBufferAddr = triangleVBAddr;
			}
			else if (i == 1) // Floor
			{
				data.color = Color4f(1, 1, 1, 1);
				data.vertexBufferAddr = 0; // Not used
			}
			else
			{
				// Procedural color for boxes
				float r = (i % 7) / 7.0f;
				float g = ((i / 7) % 7) / 7.0f;
				float b = ((i / 49) % 7) / 7.0f;
				data.color = Color4f(r, g, b, 1.0f);
				data.vertexBufferAddr = boxVBAddr;
			}
			
			RayTracingTestShader* triangleShader = ShaderManager::Get().getGlobalShaderT<RayTracingTestShader>();
			RayTracingTestShader* floorShader = triangleShader; // They share the same shader file in this test

			// 1. Set Shader (Identifier)
			GlobalShader* targetShader = (i == 1) ? floorShader : triangleShader;
			auto d3dTable = static_cast<D3D12RayTracingShaderTable*>(mShaderTable.get());
			
			int groupIndex = (i == 1) ? 1 : 0; 
			d3dTable->setShader(i, targetShader->getRHI(), groupIndex);

			// 2. Set Data (Inline Local Data)
			ShaderParameter paramColor;
			if (targetShader->getParameter("gInstanceColor", paramColor))
			{
				d3dTable->setValue(i, EShader::RayHit, paramColor, &data.color);
			}

			ShaderParameter paramUVScale;
			if (targetShader->getParameter("gUVScale", paramUVScale))
			{
				d3dTable->setValue(i, EShader::RayHit, paramUVScale, &data.uvScale);
			}

			// 3. Set Storage Buffer
			if (i == 0 || i >= 2) 
			{
				ShaderParameter paramVB;
				if (targetShader->getParameter("gVertices", paramVB))
				{
					d3dTable->setStorageBuffer(i, EShader::RayHit, paramVB, (i == 0) ? mVertexBuffer : mBoxVertexBuffer, EAccessOp::ReadOnly);
				}
			}
		}
	}
}

void D3D12RayTracingTestStage::onRender(float dFrame)
{
	Vec2i screenSize = ::Global::GetScreenSize();
	RHICommandList& commandList = RHICommandList::GetImmediateList();

	// Update ViewInfo
	mView.setupTransform(mCamera.getViewMatrix(), AdjustProjectionMatrixForRHI(PerspectiveMatrix(Math::DegToRad(60), (float)screenSize.x / (screenSize.y * 0.9f), 1.0f, 1000.0f)));
	mView.rectOffset = IntVector2(0, 0);
	mView.rectSize = screenSize;
	mView.worldPos = mCamera.getPos();

	RHISetViewport(commandList, 0, 0, screenSize.x, screenSize.y);
	RHISetScissorRect(commandList, 0, 0, screenSize.x, screenSize.y);
	RHISetFrameBuffer(commandList, nullptr);
	
	// Clear the whole screen to RED first
	RHIClearRenderTargets(commandList, EClearBits::All, &LinearColor(1, 0, 0, 1), 1);

	if (mRTPSO.isValid() && mTLAS.isValid() && mOutputTexture.isValid())
	{
		{
			RHIResource* transitionResources[] = { mOutputTexture, mDepthTexture };
			RHIResourceTransition(commandList, transitionResources, EResourceTransition::UAV);
		}

		// Set parameters
		RHISetRayTracingPipelineState(commandList, mRTPSO, mShaderTable);

		RayTracingTestShader* shader = ShaderManager::Get().getGlobalShaderT<RayTracingTestShader>();
		if (shader)
		{
			shader->setParameters(commandList, *mOutputTexture, *mDepthTexture, *mTLAS, *mTestTexture, mView, mFrameCount, mLightParams.pos, mLightParams.radius, (uint32)mLightParams.numSamples);
			++mFrameCount;
		}

		RHIDispatchRays(commandList, mOutputTexture->getSizeX(), mOutputTexture->getSizeY() * 0.9f, 1);
		
		{
			RHIResource* transitionResources[] = { mOutputTexture, mHistoryTexture, mDepthTexture };
			RHIResourceTransition(commandList, transitionResources, EResourceTransition::SRV);
		}
		{
			RHIResource* transitionResources[] = { mOutputTexture2 };
			RHIResourceTransition(commandList, transitionResources, EResourceTransition::UAV);
		}

		// Post-Process: Denoiser (Ping-Pong + Temporal)
		ShadowDenoiserShader* denoiser = ShaderManager::Get().getGlobalShaderT<ShadowDenoiserShader>();
		if (denoiser)
		{
			RHISetComputeShader(commandList, denoiser->getRHI());
			denoiser->setParameters(commandList, *mOutputTexture, *mOutputTexture2, *mHistoryTexture, *mDepthTexture, mView, mPrevView, mLightParams.denoiserAlpha, mLightParams.spatialRadius, mLightParams.sigmaScale, mLightParams.confidenceScale, mLightParams.maxSpatialRadius, mLightParams.hardnessScale);
			RHIDispatchCompute(commandList, (mOutputTexture->getSizeX() + 15) / 16, (mOutputTexture->getSizeY() + 15) / 16, 1);
			
		}

		{
			RHIResource* transitionResources[] = { mOutputTexture2 };
			RHIResourceTransition(commandList, transitionResources, EResourceTransition::SRV);
		}

		// Save current view for next frame
		mPrevView = mView;

		RHIGraphics2D& g = ::Global::GetRHIGraphics2D();
		g.beginRender();

		g.setBrush(Color3f(1, 1, 1));
		g.drawTexture(*mOutputTexture2, Vector2(0,0), Vector2(screenSize.x, screenSize.y));
		g.endRender();

		// Ping-Pong: The denoised result (Output2) becomes the next frame's history
		std::swap(mOutputTexture2, mHistoryTexture);
	}
}

REGISTER_STAGE_ENTRY("D3D12 Ray Tracing", D3D12RayTracingTestStage, EExecGroup::FeatureDev, "Render|RHI");
