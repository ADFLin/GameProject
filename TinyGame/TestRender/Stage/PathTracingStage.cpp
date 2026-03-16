#include "PathTracingStage.h"

#include "PathTracing/PathTracingBVH.h"

#include "RHI/RHIGraphics2D.h"
#include "InputManager.h"
#include "GameGUISystem.h"
#include "Widget/WidgetUtility.h"
#include "Json.h"
#include "FileSystem.h"
#include "DataCacheInterface.h"

#include "ProfileSystem.h"

using namespace Render;


#if TINY_WITH_EDITOR
#include "Editor.h"
#endif


REGISTER_STAGE_ENTRY("Path Tracing", PathTracingStage, EExecGroup::FeatureDev, "Render");


PathTracingStage::PathTracingStage()
{

}

bool PathTracingStage::onInit()
{
	if (!BaseClass::onInit())
		return false;

	mCamera.lookAt(Vector3(20, 20, 20), Vector3(0, 0, 0), Vector3(0, 0, 1));

	loadCameaTransform();

	::Global::GUI().cleanupWidget();


	auto frame = WidgetUtility::CreateDevFrame();
	frame->addCheckBox("Draw Debug", mbDrawDebug);
	frame->addCheckBox("Use MIS", mRenderConfig.bUseMIS);
	frame->addCheckBox("Use BVH4", mRenderConfig.bUseBVH4);
	frame->addCheckBox("Use Denoise", mRenderConfig.bUseDenoise);
	frame->addCheckBox("Use Hardware RT", mRenderConfig.bUseHardwareRayTracing);
	auto choice = frame->addChoice("DebugDisplay Mode", UI_StatsMode);

	mFrame = frame;

	char const* ModeTextList[] = 
	{
		"None",
		"BoundingBox",
		"Triangle",
		"Mix",
		"HitNoraml",
		"HitPos",
		"BaseColor",
		"EmissiveColor",
		"Roughness",
		"Specular",
	};
	static_assert(ARRAY_SIZE(ModeTextList) == (int)EDebugDsiplayMode::COUNT);
	for (int i = 0; i < (int)EDebugDsiplayMode::COUNT; ++i)
	{
		choice->addItem(ModeTextList[i]);
	}
	choice->setSelection((int)mRenderConfig.debugDisplayMode);

	GSlider* slider;
	slider = frame->addSlider("BoundBoxWarningCount", UI_ANY);
	slider->onGetShowValue = [this]() -> std::string { return FStringConv::From(mRenderConfig.boundBoxWarningCount); };
	slider->showValue();
	FWidgetProperty::Bind(slider, mRenderConfig.boundBoxWarningCount, 0, 1000);
	slider = frame->addSlider("TriangleWarningCount", UI_ANY);
	slider->onGetShowValue = [this]() -> std::string { return FStringConv::From(mRenderConfig.triangleWarningCount); };
	slider->showValue();
	FWidgetProperty::Bind(slider, mRenderConfig.triangleWarningCount, 0, 500);
	slider = frame->addSlider("DebugShowDepth", UI_ANY);
	slider->onGetShowValue = [this]() -> std::string { return FStringConv::From(mDebugShowDepth); };
	slider->showValue();
	FWidgetProperty::Bind(slider, mDebugShowDepth, 0, 32, [this](int)
	{
		mRenderer.showNodeBound(mDebugShowDepth);
	});


	Vector2 lookPos = Vector2(0, 0);
	mWorldToScreen = RenderTransform2D::LookAt(::Global::GetScreenSize(), lookPos, Vector2(0, 1), ::Global::GetScreenSize().x / 800.0f);
	mScreenToWorld = mWorldToScreen.inverse();
	//generatePath();

	restart();

#if TINY_WITH_EDITOR
	if (::Global::Editor())
	{
		PathTracingEditor::setContext(this);
		PathTracingEditor::initialize();
	}
#endif

	return true;
}

void PathTracingStage::onEnd()
{
	if (mScript)
	{
		mScript->release();
	}
#if TINY_WITH_EDITOR
	PathTracingEditor::finalize();
#endif
	BaseClass::onEnd();
}

void PathTracingStage::onUpdate(GameTimeSpan deltaTime)
{
	BaseClass::onUpdate(deltaTime);
	mCamera.updatePosition(deltaTime);

#if TINY_WITH_EDITOR

	PathTracingEditor::update(deltaTime);

	if (bNeedReload)
	{
		bNeedReload = false;
		bMeshChanged = false;
		loadSceneData();
	}

	if (bDataChanged)
	{
		bDataChanged = false;
		RHIFlushCommand(RHICommandList::GetImmediateList());

		if (bMeshChanged)
		{
			bMeshChanged = false;
			mRenderer.clearScene();

			mSceneData.meshes.clear();
			auto meshInfos = std::move(mSceneData.meshInfos);
			SceneBuildContext context(mSceneData, mRenderer);
			for(auto const& meshInfo : meshInfos)
			{
				FSceneBuilder::LoadMesh(context, meshInfo.path.c_str(), meshInfo.transform);
			}
			mRenderer.buildMeshResource(mSceneData.meshes);

		}

		mRenderer.buildSceneResource(RHICommandList::GetImmediateList(), mSceneData);
	}
#endif
}

void PathTracingStage::loadScene(char const* path)
{
	JsonFile* file = JsonFile::Load(path);
	if (file)
	{
		JsonSerializer serializer(file->getObject(), true);
		serializer.serialize("Materials", mSceneData.materials);
		serializer.serialize("Objects", mSceneData.objects);
		serializer.serialize("MeshInfos", mSceneData.meshInfos);
		file->release();

		bMeshChanged = true;
		bDataChanged = true;
		mView.frameCount = 0;
	}
}

void PathTracingStage::saveScene(char const* path)
{
	JsonFile* file = JsonFile::Create();
	if (file)
	{
		JsonSerializer serializer(file->getObject(), false);
		serializer.serialize("Materials", mSceneData.materials);
		serializer.serialize("Objects", mSceneData.objects);
		serializer.serialize("MeshInfos", mSceneData.meshInfos);
		std::string jsonStr = file->toString();
		FFileUtility::SaveFromBuffer(path, (uint8 const*)jsonStr.c_str(), (uint32)jsonStr.length());
		file->release();
	}
}

void PathTracingStage::onRender(float dFrame)
{
	RHICommandList& commandList = RHICommandList::GetImmediateList();
	RHISetFrameBuffer(commandList, nullptr);
	RHIClearRenderTargets(commandList, EClearBits::Color, &LinearColor(0, 0, 0, 1), 1, FRHIZBuffer::FarPlane);

	bool bResetFrameCount = mCamera.getPos() != mLastPos || mCamera.getRotation().getEulerZYX() != mLastRoation;
	bResetFrameCount |= (mRenderConfig.debugDisplayMode == EDebugDsiplayMode::None) && (mLastDebugDisplayModeRender != mRenderConfig.debugDisplayMode);
#if TINY_WITH_EDITOR
	if (bDataChanged)
	{
		bResetFrameCount = true;
	}
#endif

	if (bResetFrameCount)
	{
		mLastPos = mCamera.getPos();
		mLastRoation = mCamera.getRotation().getEulerZYX();
		mView.frameCount = 0;
	}
	else
	{
		mView.frameCount += 1;
	}

	mLastDebugDisplayModeRender = mRenderConfig.debugDisplayMode;

	Vec2i screenSize = ::Global::GetScreenSize();
	mSceneRenderTargets.prepare(screenSize);

	mViewFrustum.mNear = 0.01;
	mViewFrustum.mFar = 800.0;
	mViewFrustum.mAspect = float(screenSize.x) / screenSize.y;
	mViewFrustum.mYFov = Math::DegToRad(90 / mViewFrustum.mAspect);

	mView.rectOffset = IntVector2(0, 0);
	mView.rectSize = screenSize;
	mView.setupTransform(mCamera.getPos(), mCamera.getRotation(), mViewFrustum.getPerspectiveMatrix());

	RHISetViewport(commandList, 0, 0, screenSize.x, screenSize.y);

	RHITexture2D* outputTexture = mRenderer.render(commandList, mRenderConfig, mView, mSceneRenderTargets);

	{
		GPU_PROFILE("CopyToBackBuffer");
		RHISetFrameBuffer(commandList, nullptr);
		RHISetViewport(commandList, 0, 0, screenSize.x, screenSize.y);
		ShaderHelper::Get().copyTextureToBuffer(commandList, *outputTexture);

		mSceneRenderTargets.swapFrameTexture();
	}

#if 0
	GTextureShowManager.registerRenderTarget(GRenderTargetPool);
	GTextureShowManager.registerTexture("FrameA", &mSceneRenderTargets.getFrameTexture());
	GTextureShowManager.registerTexture("FrameB", &mSceneRenderTargets.getPrevFrameTexture());
#endif
	if ( mbDrawDebug)
	{
		GPU_PROFILE("DrawDebug");
		mRenderer.mDebugPrimitives.drawDynamic(commandList, mView);

		RHIGraphics2D& g = Global::GetRHIGraphics2D();
		g.beginRender();

		if(0)
		{
			TRANSFORM_PUSH_SCOPE(g.getTransformStack(), mWorldToScreen, false);
			RenderUtility::SetPen(g, EColor::White);
			g.drawCircle(Vector2::Zero(), 100);
			
			for (auto const& line : mPaths)
			{
				g.setPen(line.color);
				g.drawLine(line.start, line.end);

			}
		}

		g.endRender();
	}
}

MsgReply PathTracingStage::onKey(KeyMsg const& msg)
{
	float baseImpulse = 500;
	if (InputManager::Get().isKeyDown(EKeyCode::LShift))
		baseImpulse = 5;
	switch (msg.getCode())
	{
	case EKeyCode::W: mCamera.moveForwardImpulse = msg.isDown() ? baseImpulse : 0; break;
	case EKeyCode::S: mCamera.moveForwardImpulse = msg.isDown() ? -baseImpulse : 0; break;
	case EKeyCode::D: mCamera.moveRightImpulse = msg.isDown() ? baseImpulse : 0; break;
	case EKeyCode::A: mCamera.moveRightImpulse = msg.isDown() ? -baseImpulse : 0; break;
	case EKeyCode::Z: mCamera.moveUp(0.5); break;
	case EKeyCode::X: mCamera.moveUp(-0.5); break;
	}

	if (msg.isDown())
	{
		switch (msg.getCode())
		{
		case EKeyCode::R: restart(); break;
		case EKeyCode::C:
			saveCameraTransform();
			break;
		case EKeyCode::M:
			loadCameaTransform();
			break;
		case EKeyCode::V:
			bNeedReload = true;
			break;
		}
	}
	return BaseClass::onKey(msg);
}

bool PathTracingStage::loadSceneData()
{
	TIME_SCOPE("LoadSceneData");
	
	if (mScript == nullptr)
	{
		mScript = PathTracing::ISceneScript::Create();
	}

	mRenderer.clearScene();
	mSceneData.clearScene();
	SceneBuildContext context(mSceneData, mRenderer);
	VERIFY_RETURN_FALSE(FSceneBuilder::RunScript(context, *mScript, "DefaultScene"));

	bDataChanged = true;
	return true;
}

void PathTracingStage::addMeshFromFile(char const* filePath)
{
	SceneBuildContext context(mSceneData, mRenderer);

	int meshId = FSceneBuilder::LoadMesh(context, filePath, Transform::Identity());
	if (meshId != INDEX_NONE)
	{
		mSceneData.objects.push_back(FObject::Mesh(meshId, 1.0f, 0, Vector3::Zero()));
		bDataChanged = true;
		mView.frameCount = 0;
		refreshDetailView();
	}
}

void PathTracingStage::configRenderSystem(ERenderSystem systenName, RenderSystemConfigs& systemConfigs)
{
#if 1
	systemConfigs.screenWidth = 1024;
	systemConfigs.screenHeight = 768;
#else
	systemConfigs.screenWidth = 1920;
	systemConfigs.screenHeight = 1080;
#endif
	systemConfigs.bVSyncEnable = false;
}

bool PathTracingStage::setupRenderResource(ERenderSystem systemName)
{
	VERIFY_RETURN_FALSE(ShaderHelper::Get().init());

	if (!loadCommonShader())
		return false;
	if (!createSimpleMesh())
		return false;

	if (!mRenderer.initializeRHI(ETexture::RGBA32F))
		return false;



	{
		InputLayoutDesc desc;
		desc.addElement(0, EVertex::ATTRIBUTE_POSITION, EVertex::Float3);
		desc.addElement(0, EVertex::ATTRIBUTE_NORMAL, EVertex::Float3);
		mMeshInputLayout = RHICreateInputLayout(desc);
	}

	mSceneRenderTargets.mFrameBufferFormat = ETexture::RGBA32F;
	mSceneRenderTargets.mGBuffer.mTargetFomats[EGBuffer::A] = ETexture::RGBA32F;
	VERIFY_RETURN_FALSE(mSceneRenderTargets.initializeRHI());


	VERIFY_RETURN_FALSE(loadSceneData());

#if TINY_WITH_EDITOR
	PathTracingEditor::initializeRHI();

	if (mObjectsDetailView)
	{
		auto OnPropertyChange = [this](char const*)
		{
			bDataChanged = true;
			mView.frameCount = 0;
		};

		PropertyViewHandle handle = mObjectsDetailView->addValue(mSceneData.objects, "Objects");
		mObjectsDetailView->addCallback(handle, OnPropertyChange);
	}
	if (mMaterialsDetailView)
	{
		auto OnPropertyChange = [this](char const*)
		{
			bDataChanged = true;
			mView.frameCount = 0;
		};

		PropertyViewHandle handle = mMaterialsDetailView->addValue(mSceneData.materials, "Materials");
		mMaterialsDetailView->addCallback(handle, OnPropertyChange);
	}

	if (mMeshInfosDetailView)
	{
		auto OnPropertyChange = [this](char const* propertyName)
		{
			int index = INDEX_NONE;
			if (propertyName && sscanf(propertyName, "MeshInfos[%d]", &index) == 1)
			{
				updateMeshImportTransform(index);
				mView.frameCount = 0;
			}
			else
			{
				bNeedReload = true;
				mView.frameCount = 0;
			}
		};
		PropertyViewHandle handle = mMeshInfosDetailView->addValue(mSceneData.meshInfos, "MeshInfos");
		mMeshInfosDetailView->addCallback(handle, OnPropertyChange);
	}
#endif

	if (0)
	{
		Vec2i screenSize = ::Global::GetScreenSize();
		mSceneRenderTargets.prepare(screenSize);

		RHICommandList& commandList = RHICommandList::GetImmediateList();
		RHISetFrameBuffer(commandList, mSceneRenderTargets.getFrameBuffer());
		LinearColor clearColors[2] = { LinearColor(0, 0, 0, 0) , LinearColor(0, 0, 0, 0) };
		RHIClearRenderTargets(commandList, EClearBits::Color, clearColors, 1);
		RHISetFrameBuffer(commandList, nullptr);
		mSceneRenderTargets.swapFrameTexture();
		RHISetFrameBuffer(commandList, mSceneRenderTargets.getFrameBuffer());
		//LinearColor clearColors[2] = { LinearColor(0, 0, 0, 0) , LinearColor(0, 0, 0, 0) };
		RHIClearRenderTargets(commandList, EClearBits::Color, clearColors, 1);
		RHISetFrameBuffer(commandList, nullptr);
	}

	return true;
}

void PathTracingStage::preShutdownRenderSystem(bool bReInit /*= false*/)
{
	mRenderer.releaseRHI();

	mView.releaseRHIResource();
	mSceneRenderTargets.releaseRHI();
	mFrameBuffer.release();
}

#if TINY_WITH_EDITOR
void PathTracingStage::updateMeshImportTransform(int index)
{
	SceneBuildContext context(mSceneData, mRenderer);
	if (!FSceneBuilder::UpdateMeshImportTransform(context, index, mSceneData.meshInfos[index].transform))
	{
		bNeedReload = true;
	}
	mView.frameCount = 0;
}
#endif

