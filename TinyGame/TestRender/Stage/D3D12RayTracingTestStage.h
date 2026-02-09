#pragma once
#include "Stage/TestStageHeader.h"
#include "RHI/RHIRayTracingTypes.h"
#include "GameRenderSetup.h"
#include "Math/Vector4.h"
#include "Renderer/SimpleCamera.h"
#include "Renderer/SceneView.h"

class D3D12RayTracingTestStage : public StageBase, public IGameRenderSetup
{
	using BaseClass = StageBase;
public:
	D3D12RayTracingTestStage();

	bool onInit() override;
	void onEnd() override;
	void onUpdate(GameTimeSpan deltaTime) override;
	void onRender(float dFrame) override;
	virtual MsgReply onKey(KeyMsg const& msg) override;
	virtual MsgReply onMouse(MouseMsg const& msg) override;

	void restart();



	ERenderSystem getDefaultRenderSystem() override
	{
		return ERenderSystem::D3D12;
	}


	bool setupRenderResource(ERenderSystem systemName) override;

protected:
	void setupRayTracing();

	Render::RHIBottomLevelAccelerationStructureRef mBLAS;
	Render::RHIBottomLevelAccelerationStructureRef mPlaneBLAS;
	Render::RHIBottomLevelAccelerationStructureRef mBoxBLAS;
	Render::RHITopLevelAccelerationStructureRef mTLAS;
	Render::RHIRayTracingPipelineStateRef mRTPSO;
	Render::RHIRayTracingShaderTableRef mShaderTable;
	Render::RHIBufferRef mVertexBuffer;
	Render::RHIBufferRef mPlaneVertexBuffer;
	Render::RHIBufferRef mBoxVertexBuffer;
	Render::RHITexture2DRef mOutputTexture;
	Render::RHITexture2DRef mOutputTexture2;
	Render::RHITexture2DRef mHistoryTexture;
	Render::RHITexture2DRef mDepthTexture;
	Render::RHITexture2DRef mTestTexture;

	struct alignas(16) HitData
	{
		Color4f       color;
		Math::Vector3 uvScale;
		uint32        padding;
		uint64        vertexBufferAddr;
	};
	TArray<HitData> mInstanceLocalData;

	struct LightParams
	{
		Math::Vector3 pos = Math::Vector3(20, -10, 50);
		float   radius = 2.5f;
		float   denoiserAlpha = 0.01f;
		int     numSamples = 8;
		float   spatialRadius = 4.0f;
		float   sigmaScale = 2.0f;
		float   confidenceScale = 8.0f;
		float   maxSpatialRadius = 12.0f;
		float   hardnessScale = 10.0f;
	} mLightParams;

	Render::SimpleCamera mCamera;
	Render::ViewInfo mView;
	Render::ViewInfo mPrevView;
	Vec2i mLastMousePos;
	uint32 mFrameCount = 0;
};

