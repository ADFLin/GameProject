#include "RayTracingStage.h"
#include "RHI/RHIGraphics2D.h"
#include "RHI/ShaderManager.h"
#include "InputManager.h"
#include "GameGUISystem.h"
#include "Widget/WidgetUtility.h"
#include "Json.h"
#include "FileSystem.h"


using namespace Render;

#define CHAISCRIPT_NO_THREADS
#include "chaiscript/chaiscript.hpp"
namespace Chai = chaiscript;

#include "Editor.h"

REGISTER_STAGE_ENTRY("Raytracing Test", RayTracingTestStage, EExecGroup::FeatureDev, "Render");

IMPLEMENT_SHADER(RayTracingPS, EShader::Pixel, SHADER_ENTRY(MainPS));
IMPLEMENT_SHADER(AccumulatePS, EShader::Pixel, SHADER_ENTRY(AccumulatePS));
IMPLEMENT_SHADER(DenoisePS, EShader::Pixel, SHADER_ENTRY(DenoisePS));
IMPLEMENT_SHADER(EditorPreviewVS, EShader::Vertex, SHADER_ENTRY(MainVS));
IMPLEMENT_SHADER(EditorPreviewPS, EShader::Pixel, SHADER_ENTRY(MainPS));
IMPLEMENT_SHADER(ScreenOutlinePS, EShader::Pixel, SHADER_ENTRY(MainPS));

bool RayTracingTestStage::onInit()
{
	if (!BaseClass::onInit())
		return false;


	mView.gameTime = 0;
	mView.realTime = 0;
	mView.frameCount = 0;

	mbGamePased = false;
	mCamera.lookAt(Vector3(20, 20, 20), Vector3(0, 0, 0), Vector3(0, 0, 1));

	loadCameaTransform();


	::Global::GUI().cleanupWidget();


	auto frame = WidgetUtility::CreateDevFrame();
	frame->addCheckBox("Draw Debug", mbDrawDebug);
	frame->addCheckBox("Use MIS", bUseMIS);
	frame->addCheckBox("Use BVH4", bUseBVH4);
	frame->addCheckBox("Use Denoise", bUseDenoise);

	auto choice = frame->addChoice("DebugDisplay Mode", UI_StatsMode);

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
	choice->setSelection((int)mDebugDisplayMode);

	GSlider* slider;
	slider = frame->addSlider("BoundBoxWarningCount", UI_ANY);
	slider->onGetShowValue = [this]() -> std::string { return FStringConv::From(mBoundBoxWarningCount); };
	slider->showValue();
	FWidgetProperty::Bind(slider, mBoundBoxWarningCount, 0, 1000);
	slider = frame->addSlider("TriangleWarningCount", UI_ANY);
	slider->onGetShowValue = [this]() -> std::string { return FStringConv::From(mTriangleWarningCount); };
	slider->showValue();
	FWidgetProperty::Bind(slider, mTriangleWarningCount, 0, 500);
	slider = frame->addSlider("DebugShowDepth", UI_ANY);
	slider->onGetShowValue = [this]() -> std::string { return FStringConv::From(mDebugShowDepth); };
	slider->showValue();
	FWidgetProperty::Bind(slider, mDebugShowDepth, 0, 32, [this](int)
	{
		showNodeBound(mDebugShowDepth);
	});
	Vector2 lookPos = Vector2(0, 0);
	mWorldToScreen = RenderTransform2D::LookAt(::Global::GetScreenSize(), lookPos, Vector2(0, 1), ::Global::GetScreenSize().x / 800.0f);
	mScreenToWorld = mWorldToScreen.inverse();
	//generatePath();

	restart();

#if TINY_WITH_EDITOR
	if (::Global::Editor())
	{
		::Global::Editor()->addGameViewport(this);

		DetailViewConfig config;
		config.name = "Ray Tracing Settings";
		mDetailView = ::Global::Editor()->createDetailView(config);


		// --- Rendering category ---
		mDetailView->setCategory("Rendering");
		//mDetailView->addValue(bUseMIS, "Use MIS");
		mDetailView->addValue(bUseBVH4, "Use BVH4");
		mDetailView->addValue(bUseDenoise, "Use Denoise");
		mDetailView->addValue(bSplitAccumulate, "Split Accumulate");
		mDetailView->addValue((int&)mDebugDisplayMode, "Debug Mode");
		mDetailView->addValue(bUseMIS, "Use MIS");

		mDetailView->clearCategory();

		config.name = "Objects";
		mObjectsDetailView = ::Global::Editor()->createDetailView(config);

		config.name = "Materials";
		mMaterialsDetailView = ::Global::Editor()->createDetailView(config);

		mDetailView->addCategoryCallback("Rendering", [frame](char const*)
		{
			frame->refresh();
		});

		ToolBarConfig toolBarConfig;
		toolBarConfig.name = "Ray Tracing Tools";
		mToolBar = ::Global::Editor()->createToolBar(toolBarConfig);
		mToolBar->addButton("Restart", [this]() { restart(); });
		mToolBar->addSeparator();
		mToolBar->addButton("Save Camera", [this]() { saveCameraTransform(); });
		mToolBar->addButton("Load Camera", [this]() { loadCameaTransform(); });
		mToolBar->addButton("Save Scene", [this]()
		{
			char filePath[512] = "RayTracingScene.json";
			OpenFileFilterInfo filters[] = { {"Json File", "*.json"} };
			if (SystemPlatform::SaveFileName(filePath, ARRAY_SIZE(filePath), filters, nullptr, "Save Scene"))
			{
				saveScene(filePath);
			}
		});
		mToolBar->addButton("Load Scene", [this]()
		{
			char filePath[512] = "";
			OpenFileFilterInfo filters[] = { {"Json File", "*.json"} };
			if (SystemPlatform::OpenFileName(filePath, ARRAY_SIZE(filePath), filters, "RayTracingScene.json"))
			{
				loadScene(filePath);
			}
		});

		toolBarConfig.name = "Ray Tracing Tools2";
		mToolBar = ::Global::Editor()->createToolBar(toolBarConfig);
		mToolBar->addButton("Translate", [this]() { mGizmoType = EGizmoType::Translate; });
		mToolBar->addButton("Rotate", [this]() { mGizmoType = EGizmoType::Rotate; });
		mToolBar->addButton("Scale", [this]() { mGizmoType = EGizmoType::Scale; });
		mToolBar->addSeparator();
		mToolBar->addButton("Local/World", [this]() 
		{ 
			mGizmoMode = (mGizmoMode == EGizmoMode::Local) ? EGizmoMode::World : EGizmoMode::Local; 
		});
		mToolBar->addSeparator();
		mToolBar->addButton("AA", [this]() { restart(); });
		mToolBar->addSeparator();
		mToolBar->addButton("BB", [this]() { saveCameraTransform(); });
		mToolBar->addButton("CC", [this]() { loadCameaTransform(); });
	}
#endif

	return true;
}

void RayTracingTestStage::onUpdate(GameTimeSpan deltaTime)
{
	BaseClass::onUpdate(deltaTime);

	if (!mbGamePased)
	{
		mView.gameTime += deltaTime;
	}

	mCamera.updatePosition(deltaTime);

#if TINY_WITH_EDITOR
	mEditorCamera.updatePosition(deltaTime);
	if (bDataChanged)
	{
		rebuildSceneBVH();
		bDataChanged = false;
	}
#endif
}

RayTracingTestStage::RayTracingTestStage()
{
	mbGamePased = false;
}

void RayTracingTestStage::loadScene(char const* path)
{
	JsonFile* file = JsonFile::Load(path);
	if (file)
	{
		JsonSerializer serializer(file->getObject(), true);
		serializer.serialize("Materials", materials);
		serializer.serialize("Objects", objects);
		file->release();
		bDataChanged = true;
		mView.frameCount = 0;
	}
}

void RayTracingTestStage::saveScene(char const* path)
{
	JsonFile* file = JsonFile::Create();
	if (file)
	{
		JsonSerializer serializer(file->getObject(), false);
		serializer.serialize("Materials", materials);
		serializer.serialize("Objects", objects);
		std::string jsonStr = file->toString();
		FFileUtility::SaveFromBuffer(path, (uint8 const*)jsonStr.c_str(), (uint32)jsonStr.length());
		file->release();
	}
}

void RayTracingTestStage::onRender(float dFrame)
{
	RHICommandList& commandList = RHICommandList::GetImmediateList();
	RHISetFrameBuffer(commandList, nullptr);
	RHIClearRenderTargets(commandList, EClearBits::Color, &LinearColor(0, 0, 0, 1), 1, FRHIZBuffer::FarPlane);

	bool bResetFrameCount = mCamera.getPos() != mLastPos || mCamera.getRotation().getEulerZYX() != mLastRoation;
	bResetFrameCount |= (mDebugDisplayMode == EDebugDsiplayMode::None) && (mLastDebugDisplayModeRender != mDebugDisplayMode);
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

	mLastDebugDisplayModeRender = mDebugDisplayMode;

	Vec2i screenSize = ::Global::GetScreenSize();
	mSceneRenderTargets.prepare(screenSize);

	mViewFrustum.mNear = 0.01;
	mViewFrustum.mFar = 800.0;
	mViewFrustum.mAspect = float(screenSize.x) / screenSize.y;
	mViewFrustum.mYFov = Math::DegToRad(90 / mViewFrustum.mAspect);

	RHIResourceTransition(commandList, { (RHIResource*)&mSceneRenderTargets.getFrameTexture(), 
		                                 (RHIResource*)&mSceneRenderTargets.getGBuffer().getRenderTexture(EGBuffer::A) }, EResourceTransition::RenderTarget);
	RHIResourceTransition(commandList, { (RHIResource*)&mSceneRenderTargets.getPrevFrameTexture() }, EResourceTransition::SRV);
	RHISetViewport(commandList, 0, 0, screenSize.x, screenSize.y);
	mSceneRenderTargets.getFrameBuffer()->setTexture(1, mSceneRenderTargets.getGBuffer().getRenderTexture(EGBuffer::A));
	mSceneRenderTargets.getFrameBuffer()->removeDepth();
	RHISetFrameBuffer(commandList, mSceneRenderTargets.getFrameBuffer());

	mView.rectOffset = IntVector2(0, 0);
	mView.rectSize = screenSize;


	mView.setupTransform(mCamera.getPos(), mCamera.getRotation(), mViewFrustum.getPerspectiveMatrix());

	RHISetBlendState(commandList, TStaticBlendState<>::GetRHI());
	RHISetDepthStencilState(commandList, StaticDepthDisableState::GetRHI());
	RHISetRasterizerState(commandList, TStaticRasterizerState<ECullMode::None>::GetRHI());

	{
		RayTracingPS::PermutationDomain permutationVector;


		if (mDebugDisplayMode != EDebugDsiplayMode::None)
		{
			permutationVector.set<RayTracingPS::UseDebugDisplay>(true);
			permutationVector.set<RayTracingPS::UseSplitAccumulate>(false);
			permutationVector.set<RayTracingPS::UseMIS>(false);
		}
		else
		{
			permutationVector.set<RayTracingPS::UseDebugDisplay>(false);
			permutationVector.set<RayTracingPS::UseSplitAccumulate>(bSplitAccumulate);
			permutationVector.set<RayTracingPS::UseMIS>(bUseMIS);
		}

		permutationVector.set<RayTracingPS::UseBVH4>(bUseBVH4);
		RayTracingPS* rayTracingPS = ::ShaderManager::Get().getGlobalShaderT<RayTracingPS>(permutationVector);

		GPU_PROFILE("RayTracing");
		GraphicsShaderStateDesc state;
		state.vertex = mScreenVS->getRHI();
		state.pixel = rayTracingPS->getRHI();
		RHISetGraphicsShaderBoundState(commandList, state);

		if (mDebugDisplayMode != EDebugDsiplayMode::None)
		{
			rayTracingPS->setParam(commandList, SHADER_PARAM(DisplayMode), int(mDebugDisplayMode));
			rayTracingPS->setParam(commandList, SHADER_PARAM(BoundBoxWarningCount), int(mBoundBoxWarningCount));
			rayTracingPS->setParam(commandList, SHADER_PARAM(TriangleWarningCount), int(mTriangleWarningCount));
		}

		mView.setupShader(commandList, *rayTracingPS);
		SetStructuredStorageBuffer(commandList, *rayTracingPS, mMaterialBuffer);
		SetStructuredStorageBuffer(commandList, *rayTracingPS, mObjectBuffer);
		SetStructuredStorageBuffer(commandList, *rayTracingPS, mVertexBuffer);
		SetStructuredStorageBuffer(commandList, *rayTracingPS, mMeshBuffer);
		if (bUseBVH4)
		{
			SetStructuredStorageBuffer(commandList, *rayTracingPS, mBVH4NodeBuffer);
		}
		else
		{
			SetStructuredStorageBuffer(commandList, *rayTracingPS, mBVHNodeBuffer);
		}

		SetStructuredStorageBuffer< SceneBVHNodeData >(commandList, *rayTracingPS, mSceneBVHNodeBuffer);
		if (bUseBVH4 && 0)
		{
			SetStructuredStorageBuffer< SceneBVH4NodeData > (commandList, *rayTracingPS, mSceneBVH4NodeBuffer);
			SetStructuredStorageBuffer< ObjectIdData >(commandList, *rayTracingPS, mObjectIdBufferV4);
		}
		else
		{
			SetStructuredStorageBuffer< ObjectIdData >(commandList, *rayTracingPS, mObjectIdBuffer);
		}

		if (mEmittingObjectIdBuffer.isValid())
		{
			SetStructuredStorageBuffer< EmittingObjectIdData >(commandList, *rayTracingPS, mEmittingObjectIdBuffer);
		}

		rayTracingPS->setParam(commandList, SHADER_PARAM(NumObjects), mNumObjects);
		rayTracingPS->setParam(commandList, SHADER_PARAM(NumEmittingObjects), mNumEmittingObjects);

		float blendFactor = 1.0f / float(Math::Min(mView.frameCount, 4096) + 1);
		rayTracingPS->setParam(commandList, SHADER_PARAM(BlendFactor), blendFactor);
		SET_SHADER_TEXTURE_AND_SAMPLER(commandList, *rayTracingPS, HistoryTexture, mSceneRenderTargets.getPrevFrameTexture(), TStaticSamplerState<>::GetRHI());
		if (mIBLResource.texture.isValid())
		{
			SET_SHADER_TEXTURE_AND_SAMPLER(commandList, *rayTracingPS, SkyTexture, *mIBLResource.texture, TStaticSamplerState<>::GetRHI());
			rayTracingPS->mIBLParams.setParameters(commandList, *rayTracingPS, mIBLResource);
		}
		DrawUtility::ScreenRect(commandList);
	}

	RHITexture2D* pOutputTexture = &mSceneRenderTargets.getFrameTexture();
	if (mDebugDisplayMode == EDebugDsiplayMode::None && bSplitAccumulate)
	{
		GPU_PROFILE("Accumulate");

		GraphicsShaderStateDesc state;
		state.vertex = mScreenVS->getRHI();
		state.pixel = mAccumulatePS->getRHI();
		RHISetGraphicsShaderBoundState(commandList, state);

		SET_SHADER_TEXTURE_AND_SAMPLER(commandList, *mAccumulatePS, HistoryTexture, mSceneRenderTargets.getPrevFrameTexture(), TStaticSamplerState<>::GetRHI());
		SET_SHADER_TEXTURE_AND_SAMPLER(commandList, *mAccumulatePS, FrameTexture, mSceneRenderTargets.getFrameTexture(), TStaticSamplerState<>::GetRHI());
		DrawUtility::ScreenRect(commandList);

	}

	if (bUseDenoise && mDebugDisplayMode == EDebugDsiplayMode::None)
	{
		GPU_PROFILE("Denoise");
		RHIResourceTransition(commandList, { (RHIResource*)&mSceneRenderTargets.getFrameTexture() , 
			                                 (RHIResource*)&mSceneRenderTargets.getGBuffer().getRenderTexture(EGBuffer::A) }, EResourceTransition::SRV);
		RHIResourceTransition(commandList, { (RHIResource*)mDenoiseTexture.get() }, EResourceTransition::RenderTarget);
		RHISetFrameBuffer(commandList, mDenoiseFrameBuffer);

		GraphicsShaderStateDesc state;
		state.vertex = mScreenVS->getRHI();
		state.pixel = mDenoisePS->getRHI();
		RHISetGraphicsShaderBoundState(commandList, state);

		SET_SHADER_TEXTURE_AND_SAMPLER(commandList, *mDenoisePS, FrameTexture, mSceneRenderTargets.getFrameTexture(), TStaticSamplerState<>::GetRHI());
		SET_SHADER_TEXTURE_AND_SAMPLER(commandList, *mDenoisePS, GBufferTextureA, mSceneRenderTargets.getGBuffer().getRenderTexture(EGBuffer::A), TStaticSamplerState<>::GetRHI());
		mDenoisePS->setParam(commandList, SHADER_PARAM(ScreenSizeInv), Vector2(1.0f / screenSize.x, 1.0f / screenSize.y));
		mView.setupShader(commandList, *mDenoisePS);

		DrawUtility::ScreenRect(commandList);
		pOutputTexture = mDenoiseTexture;
	}

	RHIResourceTransition(commandList, { (RHIResource*)pOutputTexture }, EResourceTransition::SRV);

	{
		GPU_PROFILE("CopyToBackBuffer");
		RHISetFrameBuffer(commandList, nullptr);
		RHISetViewport(commandList, 0, 0, screenSize.x, screenSize.y);
		ShaderHelper::Get().copyTextureToBuffer(commandList, *pOutputTexture);

		mSceneRenderTargets.swapFrameTexture();
	}

	RHIFlushCommand(commandList);
	GTextureShowManager.registerRenderTarget(GRenderTargetPool);
	
	if ( mbDrawDebug )
	{
		GPU_PROFILE("DrawDebug");
		mDebugPrimitives.drawDynamic(commandList, mView);

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

MsgReply RayTracingTestStage::onKey(KeyMsg const& msg)
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
			loadSceneRHIResource();
			break;
		}
	}
	return BaseClass::onKey(msg);
}

namespace RT
{

	class SceneBuilderImpl : public ISceneBuilder
	{
	public:

		SceneBuilderImpl(SceneData& scene)
			:mScene(scene)
		{
			meshImporter = MeshImporterRegistry::Get().getMeshImproter("FBX");


		}

		SceneData& mScene;

		IMeshImporterPtr meshImporter;
		TArray< MeshVertexData > meshVertices;
		BVHTree meshBVH;
		TArray< BVHNodeData > meshBVHNodes;
		TArray< BVH4NodeData > meshBVH4Nodes;

		static int GenerateTriangleVertices(BVHTree& meshBVH, MeshImportData& meshData, MeshVertexData* pOutData)
		{
			auto posReader = meshData.makeAttributeReader(EVertex::ATTRIBUTE_POSITION);
			auto normalReader = meshData.makeAttributeReader(EVertex::ATTRIBUTE_NORMAL);

			MeshVertexData* pData = pOutData;
			for(auto const& leaf : meshBVH.leaves)
			{
				for (int id : leaf.ids)
				{
					for (int n = 0; n < 3; ++n)
					{
						MeshVertexData& vertex = *(pData++);
						int index = id + n;
						vertex.pos = posReader[index];
						vertex.normal = normalReader[index];
					}
				}
			}

			return pData - pOutData;
		}

		struct BuildMeshResult
		{
			TArrayView< MeshVertexData const > vertices;
			TArrayView< BVHNodeData const >  nodes;
			TArrayView< BVH4NodeData const > nodesV4;
			int nodeIndex;
			int nodeIndexV4;

			int triangleIndex;
			int numTriangles;
		};

		void buildMeshData(MeshImportData& meshData, BuildMeshResult& buildResult)
		{
			TIME_SCOPE("BuildMeshData");

			auto posReader = meshData.makeAttributeReader(EVertex::ATTRIBUTE_POSITION);
			auto normalReader = meshData.makeAttributeReader(EVertex::ATTRIBUTE_NORMAL);

			int numTriangles = meshData.indices.size() / 3;

			TArray< BVHTree::Primitive > primitives;
			primitives.resize(numTriangles);
			for (int indexTriangle = 0; indexTriangle < numTriangles; ++indexTriangle)
			{
				auto& primitive = primitives[indexTriangle];
				primitive.bound.invalidate();

				int index = 3 * indexTriangle;
				primitive.center = Vector3::Zero();
				for (int n = 0; n < 3; ++n)
				{
					Vector3 const& pos = posReader[index + n];
					primitive.bound.addPoint(pos);
					primitive.center += pos;
				}			
				primitive.id = index;
				primitive.center /= 3.0f;
			}

			meshBVH.clear();
			int indexLeafStart = meshBVH.leaves.size();
			TIME_SCOPE("BuildBVH");
			BVHTree::Builder builder(meshBVH);
			int indexRoot = builder.build(MakeConstView(primitives));

			auto stats = meshBVH.calcStats();
			LogMsg("Node Count : %u ,Leaf Count : %u", meshBVH.nodes.size(), meshBVH.leaves.size());
			LogMsg("Leaf Depth : %d - %d (%.2f)", stats.minDepth, stats.maxDepth, stats.meanDepth);
			LogMsg("Leaf Tris : %d - %d (%.2f)", stats.minCount, stats.maxCount, stats.meanCount);

			if (mScene.meshes.size() == 0)
			{
				mScene.mDebugBVH = meshBVH;
			}

			int triangleIndex = meshVertices.size();
			meshVertices.resize(triangleIndex + 3 * numTriangles);

			int numV = GenerateTriangleVertices(meshBVH, meshData, meshVertices.data() + triangleIndex);
			CHECK(numV == numTriangles * 3);
			int nodeIndex = meshBVHNodes.size();
			BVHNodeData::Generate(meshBVH, meshBVHNodes, triangleIndex);
			int nodeIndexV4 = meshBVH4Nodes.size();
			int numNodeV4 = BVH4NodeData::Generate(meshBVH, meshBVH4Nodes, triangleIndex);

			buildResult.vertices = TArrayView<MeshVertexData const>(meshVertices.data() + triangleIndex , numV);
			buildResult.nodes    = TArrayView<BVHNodeData const>(meshBVHNodes.data() + nodeIndex, meshBVH.nodes.size());
			buildResult.nodeIndex = nodeIndex;
			buildResult.nodesV4 = TArrayView<BVH4NodeData const>(meshBVH4Nodes.data() + nodeIndexV4, numNodeV4);
			buildResult.nodeIndexV4 = nodeIndexV4;
			buildResult.numTriangles = numTriangles;
			buildResult.triangleIndex = triangleIndex;
		}


		static void FixOffset(TArray< BVHNodeData >& nodes, int nodeOffset, int triangleOffset)
		{
			for(BVHNodeData& node : nodes)
			{
				if (node.isLeaf())
				{
					node.left += triangleOffset;
				}
				else
				{
					node.left += nodeOffset;
					node.right += nodeOffset;
				}
			}
		}

		static void FixOffset(TArray< BVH4NodeData >& nodes, int nodeOffset, int triangleOffset)
		{
			for (BVH4NodeData& node : nodes)
			{
				for (int i = 0; i < 4; ++i)
				{
					if (node.children[i] == -1)
						continue;

					if (node.primitiveCounts[i] > 0)
					{
						node.children[i] += triangleOffset;
					}
					else
					{
						node.children[i] += nodeOffset;
					}
				}
			}
		}

		int loadMesh(char const* path)
		{
			TIME_SCOPE("LoadMesh");
			MeshData mesh;

			auto& dataCache = ::Global::DataCache();
			DataCacheKey cacheKey;
			cacheKey.typeName = "MESH_BVH";
			cacheKey.version = "1e987635-09dc-521a-af73-116bc953dfe6";
			cacheKey.keySuffix.add(path);

			TArray< MeshVertexData > vertices;
			TArray< BVHNodeData > nodes;
			TArray< BVH4NodeData > nodesV4;
			int nodeIndex;
			int nodeIndexV4;
			int triangleIndex;
			auto LoadCache = [&](IStreamSerializer& serializer)-> bool
			{
				serializer >> vertices;
				serializer >> nodes;
				serializer >> nodeIndex;
				serializer >> triangleIndex;
				serializer >> nodesV4;
				serializer >> nodeIndexV4;
				return true;
			};
			if (dataCache.loadDelegate(cacheKey, LoadCache))
			{
				FixOffset(nodes, (int)meshBVHNodes.size() - nodeIndex, (int)meshVertices.size() - triangleIndex);
				FixOffset(nodesV4, (int)meshBVH4Nodes.size() - nodeIndexV4, (int)meshVertices.size() - triangleIndex);

				nodeIndex = meshBVHNodes.size();
				nodeIndexV4 = meshBVH4Nodes.size();
				triangleIndex = meshVertices.size();
				meshBVHNodes.append(nodes);
				meshVertices.append(vertices);
				meshBVH4Nodes.append(nodesV4);

				mesh.startIndex = triangleIndex;
				mesh.numTriangles = vertices.size() / 3;
				mesh.nodeIndex = nodeIndex;
				mesh.nodeIndexV4 = nodeIndexV4;

			}
			else
			{
				MeshImportData meshData;
				IMeshImporterPtr useImporter = meshImporter;
				if ( FCString::Compare(FFileUtility::GetExtension(path), "glb") == 0 )
				{
					useImporter = MeshImporterRegistry::Get().getMeshImproter("GLB");
				}

				if (!useImporter || !useImporter->importFromFile(path, meshData))
				{
					return INDEX_NONE;
				}
				BuildMeshResult buildResult;
				buildMeshData(meshData, buildResult);
				dataCache.saveDelegate(cacheKey, [&buildResult](IStreamSerializer& serializer)-> bool
				{
					serializer << buildResult.vertices;
					serializer << buildResult.nodes;
					serializer << buildResult.nodeIndex;
					serializer << buildResult.triangleIndex;
					serializer << buildResult.nodesV4;
					serializer << buildResult.nodeIndexV4;
					return true;
				});

				mesh.startIndex = buildResult.triangleIndex;
				mesh.numTriangles = buildResult.numTriangles;
				mesh.nodeIndex = buildResult.nodeIndex;
				mesh.nodeIndexV4 = buildResult.nodeIndexV4;
			}

			mScene.meshes.push_back(mesh);
			std::shared_ptr<Render::Mesh> renderMesh = std::make_shared<Render::Mesh>();
			IMeshImporterPtr useImporter = meshImporter;
			if (FCString::Compare(FFileUtility::GetExtension(path), "glb") == 0)
			{
				useImporter = MeshImporterRegistry::Get().getMeshImproter("GLB");
			}
			if (useImporter && useImporter->importFromFile(path, *renderMesh, nullptr))
			{
				mScene.mSceneMeshes.push_back(renderMesh);
			}
			else
			{
				mScene.mSceneMeshes.push_back(nullptr);
			}
			return mScene.meshes.size() - 1;
		}

		bool addDefaultObjects()
		{
			{
				//VERIFY_RETURN_FALSE(loadMesh("Mesh/dragonMid.fbx") != INDEX_NONE);
				VERIFY_RETURN_FALSE(loadMesh("Mesh/robot.glb") != INDEX_NONE);
#if 0
				VERIFY_RETURN_FALSE(loadMesh("Mesh/dragon.fbx") != INDEX_NONE);


				VERIFY_RETURN_FALSE(loadMesh("Mesh/Knight.fbx") != INDEX_NONE);
				VERIFY_RETURN_FALSE(loadMesh("Mesh/King.fbx") != INDEX_NONE);


				VERIFY_RETURN_FALSE(loadMesh("Mesh/Suzanne.fbx") != INDEX_NONE);
#endif
			}

			{
				float L = 55;
				float planeRoughness = 0.5f;
				const MaterialData mats[] =
				{
					{ Color3f(0,0,0), 0.0, 1.0,L * Color3f(1,1,1) ,0 },
					{ Color3f(1,1,1), planeRoughness, 1.0, Color3f(0,0,0) ,0},
					{ Color3f(0,1,0), planeRoughness, 1.0, Color3f(0,0,0) ,0},
					{ Color3f(1,0,0), planeRoughness, 1.0, Color3f(0,0,0) ,0},
					{ Color3f(0,0,1), planeRoughness, 1.0, Color3f(0,0,0) ,0},
					{ 0.5 * Color3f(1,1,1), 0.5, 0.2, Color3f(0,0,0) ,0},
					{ Color3f(1,1,1), 1.0, 1.0, Color3f(0,0,0) ,1.51, 0.8},
				};

				mScene.materials.append(mats, mats + ARRAY_SIZE(mats));

			}

			{
				float lightLength = 1.5;
				float length = 5;
				float hl = 0.5 * length;
				float thickness = 0.1;
				ObjectData objs[] =
				{
#if 1
					FObject::Box(Vector3(lightLength,lightLength,thickness) ,0 ,Vector3(0, 0, 0.5 * (length - 2 * thickness) + hl)),

					FObject::Box(Vector3(length,length,thickness), 1, Vector3(0, 0, 0.5 * (length - thickness) + hl)),
					FObject::Box(Vector3(length,length,thickness), 2, Vector3(0, 0, -0.5 * (length - thickness) + hl)),
					FObject::Box(Vector3(length,thickness, length), 3, Vector3(0, 0.5 * (length - thickness) ,hl)),
					FObject::Box(Vector3(length,thickness, length), 4, Vector3(0, -0.5 * (length - thickness) ,hl)),
					FObject::Box(Vector3(thickness,length, length), 5, Vector3(-0.5 * (length - thickness), 0 ,hl)),

					//FObject::Mesh(0, 1.2, 1, Vector3(0,0,-0.5 *length + 2), Quaternion::Rotate(Vector3(0,0,1) , Math::DegToRad(70))),
					FObject::Mesh(0, 0.04, 5, Vector3(0,0,1), Quaternion::Rotate(Vector3(0,0,1) , Math::DegToRad(60))),
					//FObject::Mesh(2, 1.5, 1, Vector3(0,0,-0.5 *length + 2) , Quaternion::Rotate(Vector3(0,0,1) , Math::DegToRad(70))),
					//FObject::Sphere(0.4, 6, Vector3(0, 1.5, 4.0)),
					//FObject::Sphere(0.4, 6, Vector3(8, 0, 2.5)),
#else
					FObject::Mesh(0, 1, 1, Vector3(0,0,0)),
#endif
				};
				mScene.objects.append(objs, objs + ARRAY_SIZE(objs));

			}
			return true;
		}

		bool buildRHIResource()
		{
			TIME_SCOPE("BuildRHIResource");

			if (mScene.meshes.size() > 0)
			{
				VERIFY_RETURN_FALSE(mScene.mVertexBuffer.initializeResource(MakeConstView(meshVertices), EStructuredBufferType::Buffer));
				VERIFY_RETURN_FALSE(mScene.mMeshBuffer.initializeResource(MakeConstView(mScene.meshes), EStructuredBufferType::Buffer));
				VERIFY_RETURN_FALSE(mScene.mBVHNodeBuffer.initializeResource(MakeConstView(meshBVHNodes), EStructuredBufferType::Buffer));
				VERIFY_RETURN_FALSE(mScene.mBVH4NodeBuffer.initializeResource(MakeConstView(meshBVH4Nodes), EStructuredBufferType::Buffer));
			}

			mScene.mMeshVertices = meshVertices;
			mScene.mMeshBVHNodes = meshBVHNodes;

			mScene.rebuildSceneBVH();
			return true;
		}

	};

#define SCRIPT_NAME( NAME ) #NAME

	class FScriptCommon
	{
	public:
		template< typename TModuleType >
		static void RegisterMath(TModuleType& module)
		{
			//Vector3
			module.add(Chai::constructor< Vector3(float) >(), SCRIPT_NAME(Vector3));
			module.add(Chai::constructor< Vector3(float, float, float) >(), SCRIPT_NAME(Vector3));
			module.add(Chai::fun(static_cast<Vector3& (Vector3::*)(Vector3 const&)>(&Vector3::operator=)), SCRIPT_NAME(= ));
			module.add(Chai::fun(static_cast<Vector3(*)(Vector3 const&, Vector3 const&)>(&Math::operator+)), SCRIPT_NAME(+));
			module.add(Chai::fun(static_cast<Vector3(*)(Vector3 const&, Vector3 const&)>(&Math::operator-)), SCRIPT_NAME(-));
			module.add(Chai::fun(static_cast<Vector3(*)(float, Vector3 const&)>(&Math::operator*)), SCRIPT_NAME(*));
			//Color3f
			module.add(Chai::constructor< Color3f(float, float, float) >(), SCRIPT_NAME(Color3f));
			module.add(Chai::fun(static_cast<Color3f& (Color3f::*)(Color3f const&)>(&Color3f::operator=)), SCRIPT_NAME(= ));
		}
	};

	float Deg2RadFactor = Math::PI / 180.0;

	class SceneScriptImpl : public ISceneScript
	{
	public:

		SceneBuilderImpl* mBuilder;
		Chai::ChaiScript script;
		Chai::ModulePtr CommonModule;
		SceneScriptImpl()
		{
			TIME_SCOPE("CommonModule Init");
			CommonModule.reset(new Chai::Module);
			FScriptCommon::RegisterMath(*CommonModule);
			registerAssetId(*CommonModule);
			registerSceneObject(*CommonModule);
			script.add(CommonModule);
			registerSceneFunc(script);
		}

		virtual bool setup(ISceneBuilder& builder, char const* fileName) override
		{
			mBuilder = (SceneBuilderImpl*)&builder;
#if 1
			try
			{
				std::string path = GetFilePath(fileName);
				TIME_SCOPE("Script Eval");
				script.eval_file(path.c_str());
			}
			catch (const std::exception& e)
			{
				LogMsg("Load Scene Fail!! : %s", e.what());
			}
#else
			mBuilder->addDefaultObjects();
#endif
			return true;
		}


		virtual void release() override
		{
			delete this;
		}

		static void registerSceneObject(Chai::Module& module)
		{

		}

		static void registerAssetId(Chai::Module& module)
		{

		}
		void registerSceneFunc(Chai::ChaiScript& script)
		{
			script.add(Chai::fun([this](int matId, Vector3 const& pos, float radius) { Sphere(matId, pos, radius); }), SCRIPT_NAME(Sphere));
			script.add(Chai::fun([this](int meshId, int matId, Vector3 const& pos, float scale) { Mesh(meshId, matId, pos, scale); }), SCRIPT_NAME(Mesh));
			script.add(Chai::fun([this](int matId, Vector3 pos, Vector3 size) { Box(matId, pos, size); }), SCRIPT_NAME(Box));
			script.add(Chai::fun([this](Color3f baseColor, float roughness, float specular, Color3f emissiveColor, float refractiveIndex, float refractive)
				{ return Material(baseColor, roughness, specular, emissiveColor, refractiveIndex, refractive); }), SCRIPT_NAME(Material));
			script.add(Chai::fun([this](std::string const& path) { return LoadMesh(path); }), SCRIPT_NAME(LoadMesh));
			script.add(Chai::fun([this]() { AddDefaultObjects(); }), SCRIPT_NAME(AddDefaultObjects));
		}

		void Sphere(int matId, Vector3 const& pos, float radius)
		{
			mBuilder->mScene.objects.push_back(FObject::Sphere(radius, matId, pos));
		}
		void Mesh(int meshId, int matId, Vector3 const& pos, float scale)
		{
			mBuilder->mScene.objects.push_back(FObject::Mesh(meshId, scale, matId, pos));
		}

		void Box(int matId, Vector3 pos, Vector3 size)
		{
			mBuilder->mScene.objects.push_back(FObject::Box(size, matId, pos));
		}

		int LoadMesh(std::string const& path)
		{
			return mBuilder->loadMesh(path.c_str());
		}

		int Material(
			Color3f baseColor,
			float   roughness,
			float   specular,
			Color3f emissiveColor,
			float   refractiveIndex,
			float   refractive)
		{
			int result = mBuilder->mScene.materials.size();
			mBuilder->mScene.materials.push_back(MaterialData(baseColor, roughness, specular, emissiveColor, refractive, refractiveIndex));
			return result;
		}

		bool bDefaultObjectAdded = false;
		void AddDefaultObjects()
		{
			if (bDefaultObjectAdded)
				return;

			bDefaultObjectAdded = true;
			int meshId = mBuilder->mScene.meshes.size();
			int matId = mBuilder->mScene.materials.size();
			mBuilder->addDefaultObjects();

			std::map<std::string, Chai::Boxed_Value> locals;
			for (int id = meshId; id < mBuilder->mScene.meshes.size(); ++id)
			{
				locals.emplace(InlineString<>::Make("DefMeshId_%d", id - meshId), id);
			}
			for (int id = matId; id < mBuilder->mScene.materials.size(); ++id)
			{
				locals.emplace(InlineString<>::Make("DefMatId_%d", id - matId), id);
			}
			script.set_locals(locals);
		}
	};

	std::string ISceneScript::GetFilePath(char const* fileName)
	{
		InlineString< 512 > path;
		path.format("Script/RTScene/%s.chai", fileName);
		return path;
	}

	ISceneScript* ISceneScript::Create()
	{
		return new SceneScriptImpl;
	}

	void SceneData::rebuildSceneBVH()
	{
		TIME_SCOPE("RebuildSceneBVH");

		mDebugPrimitives.clear();

		BVHTree objectBVH;
		TArray< BVHTree::Primitive > primitives;
		primitives.resize(objects.size());
		for (int i = 0; i < objects.size(); ++i)
		{
			auto const& object = objects[i];
			BVHTree::Primitive& primitive = primitives[i];
			primitive.id = i;

			primitive.bound.invalidate();
			switch (object.type)
			{
			case OBJ_SPHERE:
				{
					float r = object.meta.x;
					primitive.center = object.pos;
					primitive.bound.min = -Vector3(r, r, r);
					primitive.bound.max = Vector3(r, r, r);
					primitive.bound.translate(primitive.center);
				}
				break;
			case OBJ_CUBE:
				{
					Vector3 halfSize = object.meta;
					primitive.center = object.pos;
					primitive.bound = GetAABB(object.rotation, halfSize);
					primitive.bound.translate(primitive.center);
				}
				break;
			case OBJ_TRIANGLE_MESH:
				if (meshes.isValidIndex(AsValue<int32>(object.meta.x)))
				{
					MeshData& mesh = meshes[AsValue<int32>(object.meta.x)];
					float scale = object.meta.y;

					Vector3 boundMin = mMeshBVHNodes[mesh.nodeIndex].boundMin;
					Vector3 boundMax = mMeshBVHNodes[mesh.nodeIndex].boundMax;

					Vector3 offset = (scale * 0.5f) * (boundMax + boundMin);
					Vector3 halfSize = (scale * 0.5f) * (boundMax - boundMin);

					primitive.center = object.pos + object.rotation.rotate(offset);
					primitive.bound = GetAABB2(offset, object.rotation, halfSize);
					primitive.bound.translate(object.pos);
				}
				break;
			case OBJ_QUAD:
				{
					Vector3 halfSize = object.meta;
					primitive.center = object.pos;
					primitive.bound = GetAABB(object.rotation, halfSize);
					primitive.bound.translate(primitive.center);
				}
				break;
			}

			addDebugAABB(primitive.bound);
		}

		BVHTree::Builder builder(objectBVH);
		builder.minSplitPrimitiveCount = 2;
		builder.build(MakeConstView(primitives));

		TArray< BVHNodeData > nodes;
		TArray< int > objectIds;
		BVHNodeData::Generate(objectBVH, nodes, objectIds);
		mSceneBVHNodeBuffer.initializeResource(nodes.size(), EStructuredBufferType::Buffer);
		mSceneBVHNodeBuffer.updateBuffer(nodes);

		mObjectIdBuffer.initializeResource(objectIds.size(), EStructuredBufferType::Buffer);
		mObjectIdBuffer.updateBuffer(objectIds);

		TArray< BVH4NodeData > nodesV4;
		TArray< int > objectIdsV4;
		BVH4NodeData::Generate(objectBVH, nodesV4, objectIdsV4);
		mSceneBVH4NodeBuffer.initializeResource(nodesV4.size(), EStructuredBufferType::Buffer);
		mSceneBVH4NodeBuffer.updateBuffer(nodesV4);

		mObjectIdBufferV4.initializeResource(objectIdsV4.size(), EStructuredBufferType::Buffer);
		mObjectIdBufferV4.updateBuffer(objectIdsV4);

		mNumObjects = objects.size();
		mObjectBuffer.initializeResource(objects.size(), EStructuredBufferType::Buffer);
		mObjectBuffer.updateBuffer(objects);

		mMaterialBuffer.initializeResource(materials.size(), EStructuredBufferType::Buffer);
		mMaterialBuffer.updateBuffer(materials);

		TArray<int32> emittingObjectIds;
		for (int i = 0; i < objects.size(); ++i)
		{
			int matId = objects[i].materialId;
			if (matId >= 0 && matId < materials.size())
			{
				auto const& emissive = materials[matId].emissiveColor;
				if (emissive.r > 0 || emissive.g > 0 || emissive.b > 0)
				{
					emittingObjectIds.push_back(i);
				}
			}
		}

		mNumEmittingObjects = emittingObjectIds.size();
		if (emittingObjectIds.empty()) 
		{
			emittingObjectIds.push_back(0); // Dummy fallback
		}
		mEmittingObjectIdBuffer.initializeResource(emittingObjectIds.size(), EStructuredBufferType::Buffer);
		mEmittingObjectIdBuffer.updateBuffer(emittingObjectIds);

		mSceneBVHNodes = nodes;
		mSceneObjectIds = objectIds;
	}

}

bool RayTracingTestStage::loadSceneRHIResource()
{
	TIME_SCOPE("LoadScene");
	RT::SceneBuilderImpl builder(*this);
	if (mScript == nullptr)
	{
		mScript = RT::ISceneScript::Create();
	}

	VERIFY_RETURN_FALSE(mScript->setup(builder, "TestA"));
	VERIFY_RETURN_FALSE(builder.buildRHIResource());
	
	return true;
}

void RayTracingTestStage::configRenderSystem(ERenderSystem systenName, RenderSystemConfigs& systemConfigs)
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

bool RayTracingTestStage::setupRenderResource(ERenderSystem systemName)
{
	VERIFY_RETURN_FALSE(ShaderHelper::Get().init());

	if (!loadCommonShader())
		return false;
	if (!createSimpleMesh())
		return false;

	{
		ScreenVS::PermutationDomain permutationVector;
		permutationVector.set<ScreenVS::UseTexCoord>(true);
		VERIFY_RETURN_FALSE(mScreenVS = ::ShaderManager::Get().getGlobalShaderT<ScreenVS>(permutationVector));
	}

	VERIFY_RETURN_FALSE(mAccumulatePS = ::ShaderManager::Get().getGlobalShaderT<AccumulatePS>());
	VERIFY_RETURN_FALSE(mDenoisePS = ::ShaderManager::Get().getGlobalShaderT<DenoisePS>());

	VERIFY_RETURN_FALSE(mPreviewVS = ::ShaderManager::Get().getGlobalShaderT<EditorPreviewVS>());
	VERIFY_RETURN_FALSE(mPreviewPS = ::ShaderManager::Get().getGlobalShaderT<EditorPreviewPS>());

	VERIFY_RETURN_FALSE(mScreenOutlinePS = ::ShaderManager::Get().getGlobalShaderT<ScreenOutlinePS>());

	mSceneRenderTargets.mFrameBufferFormat = ETexture::RGBA32F;
	mSceneRenderTargets.mGBuffer.mTargetFomats[EGBuffer::A] = ETexture::RGBA32F;
	VERIFY_RETURN_FALSE(mSceneRenderTargets.initializeRHI());

	{
		Vec2i screenSize = ::Global::GetScreenSize();
		mDenoiseTexture = RHICreateTexture2D(mSceneRenderTargets.mFrameBufferFormat, screenSize.x, screenSize.y, 1, 1, TCF_RenderTarget | TCF_CreateSRV);
		mDenoiseFrameBuffer = RHICreateFrameBuffer();
		mDenoiseFrameBuffer->setTexture(0, *mDenoiseTexture);
	}
	mDebugPrimitives.initializeRHI();

	{
		char const* HDRImagePath = "Texture/HDR/A.hdr";
		mHDRImage = RHIUtility::LoadTexture2DFromFile(HDRImagePath, TextureLoadOption().HDR());
		if (mHDRImage)
		{
			mIBLResourceBuilder.initializeShader();
			VERIFY_RETURN_FALSE(mIBLResourceBuilder.loadOrBuildResource(::Global::DataCache(), HDRImagePath, *mHDRImage, mIBLResource));
		}
	}

	VERIFY_RETURN_FALSE(loadSceneRHIResource());

#if TINY_WITH_EDITOR
	if (mObjectsDetailView)
	{
		auto OnPropertyChange = [this](char const*)
		{
			bDataChanged = true;
			mView.frameCount = 0;
		};

		PropertyViewHandle handle = mObjectsDetailView->addValue(objects, "Objects");
		mObjectsDetailView->addCallback(handle, OnPropertyChange);
	}
	if (mMaterialsDetailView)
	{
		auto OnPropertyChange = [this](char const*)
		{
			bDataChanged = true;
			mView.frameCount = 0;
		};

		PropertyViewHandle handle = mMaterialsDetailView->addValue(materials, "Materials");
		mMaterialsDetailView->addCallback(handle, OnPropertyChange);
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

void RayTracingTestStage::preShutdownRenderSystem(bool bReInit /*= false*/)
{
	mScreenVS = nullptr;
	mView.releaseRHIResource();
	mMaterialBuffer.releaseResource();
	mObjectBuffer.releaseResource();
	mVertexBuffer.releaseResource();
	mMeshBuffer.releaseResource();
	mBVHNodeBuffer.releaseResource();
	mBVH4NodeBuffer.releaseResource();
	mSceneBVHNodeBuffer.releaseResource();
	mSceneBVH4NodeBuffer.releaseResource();

	mObjectBuffer.releaseResource();

	mObjectIdBuffer.releaseResource();
	mObjectIdBufferV4.releaseResource();

	mObjectIdBuffer.releaseResource();
	mObjectIdBufferV4.releaseResource();

	mDebugPrimitives.releaseRHI();
	mSceneRenderTargets.releaseRHI();
	mFrameBuffer.release();
	mIBLResource.releaseRHI();
	mIBLResourceBuilder.releaseRHI();
	mHDRImage.release();
}

#if TINY_WITH_EDITOR
TVector2<int> RayTracingTestStage::getInitialSize()
{
	return TVector2<int>(1280, 720);
}

void RayTracingTestStage::resizeViewport(int w, int h)
{
	mEditorViewportSize = Vec2i(w, h);
	mSceneRenderTargets.prepare(mEditorViewportSize);
}

void RayTracingTestStage::renderViewport(IEditorViewportRenderContext& context)
{
	PROFILE_ENTRY("RenderViewport");

	GRenderTargetPool.freeAllUsedElements();

	if (bEditorCameraValid == false)
	{
		mEditorCamera.lookAt(mCamera.getPos(), mCamera.getPos() + mCamera.getViewDir(), mCamera.getUpDir());

		bEditorCameraValid = true;
	}

	RHICommandList& commandList = RHICommandList::GetImmediateList();

	using namespace Render;
	RenderTargetDesc desc;
	desc.size = mEditorViewportSize;
	desc.format = ETexture::RGBA32F;
	desc.debugName = "RayTracingStageColor";
	PooledRenderTargetRef colorRT = GRenderTargetPool.fetchElement(desc);
	desc.format = ETexture::D24S8;
	desc.debugName = "RayTracingStageDepth";
	PooledRenderTargetRef depthRT = GRenderTargetPool.fetchElement(desc);

	if (mFrameBuffer == nullptr)
	{
		mFrameBuffer = RHICreateFrameBuffer();
	}
	mFrameBuffer->setTexture(0, static_cast<RHITexture2D&>(*colorRT->texture));
	mFrameBuffer->setDepth(static_cast<RHITexture2D&>(*depthRT->texture));

	RHISetFrameBuffer(commandList, mFrameBuffer);
	
	if (mEditorViewportSize.x <= 0 || mEditorViewportSize.y <= 0)
		return;

	RHISetViewport(commandList, 0.0f, 0.0f, (float)mEditorViewportSize.x, (float)mEditorViewportSize.y);
	RHISetScissorRect(commandList, 0, 0, mEditorViewportSize.x, mEditorViewportSize.y);

	RHIClearRenderTargets(commandList, EClearBits::All, &LinearColor(0.1f, 0.2f, 0.3f, 1.0f), 1, FRHIZBuffer::FarPlane);

	RHISetBlendState(commandList, TStaticBlendState<>::GetRHI());
	RHISetDepthStencilState(commandList, TStaticDepthStencilState<true>::GetRHI());
	RHISetRasterizerState(commandList, TStaticRasterizerState<>::GetRHI());

	float aspect = (float)mEditorViewportSize.x / (float)Math::Max(1, mEditorViewportSize.y);
	Matrix4 proj = PerspectiveMatrix(Math::DegToRad(60.0f), aspect, 0.01f, 1000.0f);

	ViewInfo view;
	view.setupTransform(mEditorCamera.getPos(), mEditorCamera.getRotation(), proj);
	view.rectOffset = IntVector2(0, 0);
	view.rectSize = mEditorViewportSize;

	GraphicsShaderStateDesc state;
	state.vertex = mPreviewVS->getRHI();
	state.pixel = mPreviewPS->getRHI();
	RHISetGraphicsShaderBoundState(commandList, state);

	auto DrawObject = [&](ObjectData const& obj, MaterialData const& mat, bool bSelected)
	{
		Matrix4 modelScale = Matrix4::Identity();
		Mesh* mesh = nullptr;
		switch (obj.type)
		{
		case OBJ_CUBE: modelScale = Matrix4::Scale(obj.meta); mesh = &getMesh(SimpleMeshId::Box); break;
		case OBJ_SPHERE: modelScale = Matrix4::Scale(obj.meta.x / 2.5f); mesh = &getMesh(SimpleMeshId::Sphere); break;
		case OBJ_TRIANGLE_MESH:
			modelScale = Matrix4::Scale(obj.meta.y);
			if (mSceneMeshes.isValidIndex(AsValue<int32>(obj.meta.x))) { mesh = mSceneMeshes[AsValue<int32>(obj.meta.x)].get(); }
			break;
		case OBJ_QUAD: modelScale = Matrix4::Scale(obj.meta); mesh = &getMesh(SimpleMeshId::Plane); break;
		}

		if (mesh)
		{
			Matrix4 world = modelScale * Matrix4::Rotate(obj.rotation) * Matrix4::Translate(obj.pos);
			if (bSelected)
			{
				RHISetDepthStencilState(commandList, TStaticDepthStencilState<true, ECompareFunc::LessEqual, true, ECompareFunc::Always, EStencil::Replace, EStencil::Replace, EStencil::Replace, 0xff, 0xff>::GetRHI(), 1);
			}
			else
			{
				RHISetDepthStencilState(commandList, TStaticDepthStencilState<true>::GetRHI());
			}

			view.setupShader(commandList, *mPreviewVS);
			SET_SHADER_PARAM(commandList, *mPreviewVS, World, world);
			SET_SHADER_PARAM(commandList, *mPreviewPS, BaseColor, LinearColor(mat.baseColor, 1.0f - mat.refractive));
			SET_SHADER_PARAM(commandList, *mPreviewPS, EmissiveColor, LinearColor(mat.emissiveColor, 1.0f));
			SET_SHADER_PARAM(commandList, *mPreviewPS, Roughness, mat.roughness);
			SET_SHADER_PARAM(commandList, *mPreviewPS, Specular, mat.specular);
			SET_SHADER_PARAM(commandList, *mPreviewPS, SelectionAlpha, 0.0f);

			mesh->draw(commandList);
		}
	};

	RHISetBlendState(commandList, TStaticBlendState<>::GetRHI());
	for (int i = 0; i < objects.size(); ++i)
	{
		auto const& obj = objects[i];
		auto const& mat = materials[obj.materialId];

		if (mat.refractive == 0.0f)
		{
			DrawObject(obj, mat, i == mSelectedObjectId);
		}
	}

	RHISetBlendState(commandList, StaticTranslucentBlendState::GetRHI());
	for (int i = 0; i < objects.size(); ++i)
	{
		auto const& obj = objects[i];
		auto const& mat = materials[obj.materialId];

		if (mat.refractive > 0.0f)
		{
			DrawObject(obj, mat, i == mSelectedObjectId);
		}
	}

	RHISetFrameBuffer(commandList, context.frameBuffer);
	RHISetViewport(commandList, 0.0f, 0.0f, (float)mEditorViewportSize.x, (float)mEditorViewportSize.y);
	RHISetScissorRect(commandList, 0, 0, mEditorViewportSize.x, mEditorViewportSize.y);

	{
		GraphicsShaderStateDesc state;
		state.vertex = mScreenVS->getRHI();
		state.pixel = mScreenOutlinePS->getRHI();
		RHISetGraphicsShaderBoundState(commandList, state);

		RHIShaderResourceViewRef stencilSRV = RHICreateSRV(static_cast<RHITexture2D&>(*depthRT->texture), ETexture::StencilView);
		SET_SHADER_TEXTURE_AND_SAMPLER(commandList, *mScreenOutlinePS, SceneDepthTexture, *stencilSRV, TStaticSamplerState<ESampler::Point>::GetRHI());
		SET_SHADER_TEXTURE_AND_SAMPLER(commandList, *mScreenOutlinePS, SceneColorTexture, *colorRT->texture, TStaticSamplerState<>::GetRHI());
		SET_SHADER_PARAM(commandList, *mScreenOutlinePS, OutlineColor, (mSelectedObjectId != INDEX_NONE) ? LinearColor(1, 0.6, 0, 1) : LinearColor(0,0,0,0));
		SET_SHADER_PARAM(commandList, *mScreenOutlinePS, ScreenSize, Vector2(mEditorViewportSize));

		RHISetBlendState(commandList, TStaticBlendState<>::GetRHI());
		RHISetDepthStencilState(commandList, StaticDepthDisableState::GetRHI());
		DrawUtility::ScreenRect(commandList);
	}

	RHIClearRenderTargets(commandList, EClearBits::Depth, nullptr, 0, FRHIZBuffer::FarPlane);

	RHISetBlendState(commandList, TStaticBlendState<>::GetRHI());
	RHISetDepthStencilState(commandList, TStaticDepthStencilState<>::GetRHI());
	RHISetRasterizerState(commandList, TStaticRasterizerState<>::GetRHI());

	drawGizmo(commandList, view);

	RHISetFixedShaderPipelineState(commandList, view.worldToClip, LinearColor(1, 1, 1, 1));
	DrawUtility::AixsLine(commandList, 2.0f);
}

#if TINY_WITH_EDITOR
void RayTracingTestStage::refreshDetailView()
{
	if (mDetailView)
	{
		mDetailView->clearCategory();

		if (mSelectedObjectId != INDEX_NONE)
		{
			auto OnPropertyChange = [this](char const*)
			{
				bDataChanged = true;
				mView.frameCount = 0;
			};

			mDetailView->clearCategoryViews("Object");
			mDetailView->setCategory("Object");
			PropertyViewHandle hObj = mDetailView->addStruct(objects[mSelectedObjectId]);
			mDetailView->addCallback(hObj, OnPropertyChange);

			mDetailView->clearCategoryViews("Material");
			mDetailView->setCategory("Material");
			PropertyViewHandle hMat = mDetailView->addStruct(materials[objects[mSelectedObjectId].materialId]);
			mDetailView->addCallback(hMat, OnPropertyChange);

			mDetailView->clearCategory();
		}
	}
}
#endif

void RayTracingTestStage::onViewportMouseEvent(MouseMsg const& msg)
{
	static Vec2i oldPos = msg.getPos();
	if (msg.isRightDown())
	{
		float rotateSpeed = 0.01f;
		Vector2 off = rotateSpeed * Vector2(msg.getPos() - oldPos);
		mEditorCamera.rotateByMouse(off.x, off.y);
	}

	float fovY = Math::DegToRad(60.0f);
	float aspect = (float)mEditorViewportSize.x / mEditorViewportSize.y;
	float tanHalfFov = Math::Tan(fovY * 0.5f);
	float clipX = (2.0f * (msg.getPos().x / (float)mEditorViewportSize.x) - 1.0f) * aspect * tanHalfFov;
	float clipY = (1.0f - 2.0f * (msg.getPos().y / (float)mEditorViewportSize.y)) * tanHalfFov;
	Vector3 rayPos = mEditorCamera.getPos();
	Vector3 rayDir = mEditorCamera.getRotation().rotate(Vector3(clipX, clipY, 1.0f)).getNormal();

	auto UpdateGizmoAxis = [&]()
	{
		mGizmoAxis = -1;
		if (mSelectedObjectId == INDEX_NONE || mGizmoType == EGizmoType::None)
			return;

		auto const& obj = objects[mSelectedObjectId];
		float distToCam = (obj.pos - rayPos).length();
		float scaleFactor = distToCam * 0.15f;
		float length = 1.0f * scaleFactor;
		float thickness = 0.15f * scaleFactor;
		float minDistance = 1e10;

		auto GetAxisDir = [&](ObjectData const& obj, int idx)
		{
			Vector3 axis(0, 0, 0);
			axis[idx] = 1.0f;
			if (mGizmoMode == EGizmoMode::Local || mGizmoType == EGizmoType::Scale)
			{
				return obj.rotation.rotate(axis);
			}
			return axis;
		};

		for (int i = 0; i < 3; ++i)
		{
			Vector3 axisDir = GetAxisDir(obj, i);

			Vector3 scale;
			Vector3 localOffset(0, 0, 0);
			if (i == 0) { scale = Vector3(length, thickness, thickness); localOffset.x = 0.5f; }
			else if (i == 1) { scale = Vector3(thickness, length, thickness); localOffset.y = 0.5f; }
			else { scale = Vector3(thickness, thickness, length); localOffset.z = 0.5f; }

			Matrix4 worldRotate = Matrix4::Identity();
			if (mGizmoMode == EGizmoMode::Local || mGizmoType == EGizmoType::Scale)
			{
				worldRotate = Matrix4::Rotate(obj.rotation);
			}

			Matrix4 localToWorld = Matrix4::Scale(scale) * worldRotate * Matrix4::Translate(obj.pos + axisDir * (0.5f * length));

			Matrix4 worldToLocal;
			float det;
			if (localToWorld.inverseAffine(worldToLocal, det))
			{
				Vector3 localRayPos = TransformPosition(rayPos, worldToLocal);
				Vector3 localRayDir = TransformVector(rayDir, worldToLocal).getNormal();
				float t[2];
				if (Math::LineAABBTest(localRayPos, localRayDir, Vector3(-0.5f, -0.5f, -0.5f), Vector3(0.5f, 0.5f, 0.5f), t))
				{
					if (t[0] > 0 && t[0] < minDistance)
					{
						minDistance = t[0];
						mGizmoAxis = i;
					}
				}
			}
		}
	};

	if (msg.onMoving() && !mIsGizmoDragging)
	{
		UpdateGizmoAxis();
	}
	else if (msg.onLeftDown())
	{
		UpdateGizmoAxis();
		if (mGizmoAxis != -1)
		{
			mIsGizmoDragging = true;
			mGizmoLastRayPos = rayPos;
			mGizmoLastRayDir = rayDir;
		}
		else
		{
			mIsGizmoDragging = false;
			float minDistance = 1e10;
			int hitId = INDEX_NONE;

			for (int i = 0; i < objects.size(); ++i)
			{
				auto const& obj = objects[i];
				float distance = 1e10;
				bool bHit = false;

				Matrix4 localToWorld;
				float modelScaleValue = 1.0f;
				switch (obj.type)
				{
				case OBJ_SPHERE:
					bHit = Math::RaySphereTest(rayPos, rayDir, obj.pos, obj.meta.x, distance);
					break;
				case OBJ_TRIANGLE_MESH:
					{
						int32 meshId = AsValue<int32>(obj.meta.x);
						if (meshes.isValidIndex(meshId))
						{
							auto const& mesh = meshes[meshId];
							float scale = obj.meta.y;
							Vector3 boundMin = mMeshBVHNodes[mesh.nodeIndex].boundMin;
							Vector3 boundMax = mMeshBVHNodes[mesh.nodeIndex].boundMax;
							Vector3 offset = (scale * 0.5f) * (boundMax + boundMin);
							Vector3 halfSize = (scale * 0.5f) * (boundMax - boundMin);

							localToWorld = Matrix4::Translate(offset) * Matrix4::Rotate(obj.rotation) * Matrix4::Translate(obj.pos);
							Matrix4 worldToLocal;
							float det;
							if (localToWorld.inverseAffine(worldToLocal, det))
							{
								Vector3 localRayPos = TransformPosition(rayPos, worldToLocal);
								Vector3 localRayDir = TransformVector(rayDir, worldToLocal).getNormal();
								float t[2];
								if (Math::LineAABBTest(localRayPos, localRayDir, -halfSize, halfSize, t))
								{
									if (t[0] > 0)
									{
										bHit = true;
										distance = t[0];
									}
								}
							}
						}
					}
					break;
				case OBJ_CUBE:
				case OBJ_QUAD:
					{
						localToWorld = Matrix4::Rotate(obj.rotation) * Matrix4::Translate(obj.pos);
						Matrix4 worldToLocal;
						float det;
						if (localToWorld.inverseAffine(worldToLocal, det))
						{
							Vector3 localRayPos = TransformPosition(rayPos, worldToLocal);
							Vector3 localRayDir = TransformVector(rayDir, worldToLocal).getNormal();
							float t[2];
							if (Math::LineAABBTest(localRayPos, localRayDir, -obj.meta, obj.meta, t))
							{
								if (t[0] > 0)
								{
									bHit = true;
									distance = t[0];
								}
							}
						}
					}
					break;
				}

				if (bHit && distance < minDistance)
				{
					minDistance = distance;
					hitId = i;
				}
			}

			if (hitId != mSelectedObjectId)
			{
				mSelectedObjectId = hitId;
				refreshDetailView();
			}
		}
	}
	else if (msg.onLeftUp())
	{
		mIsGizmoDragging = false;
	}
	else if (mIsGizmoDragging && msg.isLeftDown())
	{
		auto& obj = objects[mSelectedObjectId];
		
		auto GetAxisDir = [&](ObjectData const& obj, int idx)
		{
			Vector3 axis(0, 0, 0);
				axis[idx] = 1.0f;
			if (mGizmoMode == EGizmoMode::Local || mGizmoType == EGizmoType::Scale)
			{
				return obj.rotation.rotate(axis);
			}
			return axis;
		};

		Vector3 axisDir = GetAxisDir(obj, mGizmoAxis);

		auto GetClosestPointOnAxis = [](Vector3 const& rayPos, Vector3 const& rayDir, Vector3 const& axisPos, Vector3 const& axisDir)
		{
			Vector3 w = rayPos - axisPos;
			float k = axisDir.dot(rayDir);
			float det = 1.0f - k * k;
			if (Math::Abs(det) < 1e-6f)
				return axisPos; 
			float s = (w.dot(axisDir) - w.dot(rayDir) * k) / det;
			return axisPos + s * axisDir;
		};

		if (mGizmoType == EGizmoType::Translate)
		{
			Vector3 p2 = GetClosestPointOnAxis(mGizmoLastRayPos, mGizmoLastRayDir, obj.pos, axisDir);
			Vector3 curP2 = GetClosestPointOnAxis(rayPos, rayDir, obj.pos, axisDir);
			obj.pos += (curP2 - p2);
		}
		else if (mGizmoType == EGizmoType::Scale)
		{
			Vector3 p2 = GetClosestPointOnAxis(mGizmoLastRayPos, mGizmoLastRayDir, obj.pos, axisDir);
			Vector3 curP2 = GetClosestPointOnAxis(rayPos, rayDir, obj.pos, axisDir);
			float dist = (p2 - obj.pos).length();
			float curDist = (curP2 - obj.pos).length();
			if (dist > 0.001f)
			{
				float s = curDist / dist;
				obj.meta[mGizmoAxis] *= s;
			}
		}
		else if (mGizmoType == EGizmoType::Rotate)
		{
			Vector3 N = axisDir;
			float dotDir = N.dot(rayDir);
			float dotLastDir = N.dot(mGizmoLastRayDir);

			if (Math::Abs(dotDir) > 1e-6f && Math::Abs(dotLastDir) > 1e-6f)
			{
				float d = -N.dot(obj.pos);
				float t = -(N.dot(rayPos) + d) / dotDir;
				Vector3 curHit = rayPos + t * rayDir;

				float tLast = -(N.dot(mGizmoLastRayPos) + d) / dotLastDir;
				Vector3 lastHit = mGizmoLastRayPos + tLast * mGizmoLastRayDir;

				Vector3 v1 = lastHit - obj.pos;
				Vector3 v2 = curHit - obj.pos;
				if (v1.length() > 0.001f && v2.length() > 0.001f)
				{
					float angle = Math::ACos(Math::Clamp(v1.getNormal().dot(v2.getNormal()), -1.0f, 1.0f));
					Vector3 cross = v1.cross(v2);
					if (cross.dot(N) < 0) angle = -angle;
					obj.rotation = Quaternion::Rotate(N, angle) * obj.rotation;
				}
			}
		}
		
		mGizmoLastRayPos = rayPos;
		mGizmoLastRayDir = rayDir;
		bDataChanged = true;
		mView.frameCount = 0;
	}

	oldPos = msg.getPos();
}

void RayTracingTestStage::onViewportKeyEvent(unsigned key, bool isDown)
{
	float baseImpulse = 500;
	switch (key)
	{
	case EKeyCode::W: mEditorCamera.moveForwardImpulse = isDown ? baseImpulse : 0; break;
	case EKeyCode::S: mEditorCamera.moveForwardImpulse = isDown ? -baseImpulse : 0; break;
	case EKeyCode::D: mEditorCamera.moveRightImpulse = isDown ? baseImpulse : 0; break;
	case EKeyCode::A: mEditorCamera.moveRightImpulse = isDown ? -baseImpulse : 0; break;
	case EKeyCode::RControl:
	case EKeyCode::LControl: mbControlDown = isDown; break;
	}

	if (isDown)
	{
		if (key == EKeyCode::Delete)
		{
			if (mSelectedObjectId != INDEX_NONE)
			{
				objects.erase(objects.begin() + mSelectedObjectId);
				mSelectedObjectId = INDEX_NONE;
				bDataChanged = true;
				mView.frameCount = 0;
				refreshDetailView();
			}
		}
		else if (key == EKeyCode::V && mbControlDown)
		{
			if (mSelectedObjectId != INDEX_NONE)
			{
				ObjectData copy = objects[mSelectedObjectId];
				objects.push_back(copy);
				mSelectedObjectId = objects.size() - 1;
				bDataChanged = true;
				mView.frameCount = 0;
				refreshDetailView();
			}
		}
		else if (key == EKeyCode::Q)
		{
			mGizmoType = EGizmoType::Translate;
		}
		else if (key == EKeyCode::E)
		{
			mGizmoType = EGizmoType::Scale;
		}
		else if (key == EKeyCode::R)
		{
			mGizmoType = EGizmoType::Rotate;
		}
	}
}

void RayTracingTestStage::onViewportCharEvent(unsigned code)
{

}

void RayTracingTestStage::drawGizmo(RHICommandList& commandList, ViewInfo& view)
{
	if (mSelectedObjectId == INDEX_NONE || mGizmoType == EGizmoType::None)
		return;

	auto const& obj = objects[mSelectedObjectId];
	Vector3 pos = obj.pos;

	GraphicsShaderStateDesc state;
	state.vertex = mPreviewVS->getRHI();
	state.pixel = mPreviewPS->getRHI();
	RHISetGraphicsShaderBoundState(commandList, state);
	float distToCam = (pos - view.worldPos).length();
	float scaleFactor = distToCam * 0.15f;
	float length = 1.0f * scaleFactor;
	float thickness = 0.03f * scaleFactor;

	if (mIsGizmoDragging)
	{
		thickness *= 1.5f;
	}

	auto GetAxisDir = [&](ObjectData const& obj, int idx)
	{
		Vector3 axis(0, 0, 0);
		axis[idx] = 1.0f;
		if (mGizmoMode == EGizmoMode::Local || mGizmoType == EGizmoType::Scale)
		{
			return obj.rotation.rotate(axis);
		}
		return axis;
	};

	auto drawAxis = [&](Vector3 axisDir, LinearColor color, int axisIdx)
	{
		// --- Shaft (thin box) ---
		Matrix4 world;
		Vector3 scale;

		if (axisIdx == 0) { scale = Vector3(length, thickness, thickness); }
		else if (axisIdx == 1) { scale = Vector3(thickness, length, thickness); }
		else { scale = Vector3(thickness, thickness, length); }

		scale *= 0.5f;

		Matrix4 worldRotate = Matrix4::Identity();
		if (mGizmoMode == EGizmoMode::Local || mGizmoType == EGizmoType::Scale)
		{
			worldRotate = Matrix4::Rotate(obj.rotation);
		}

		world = Matrix4::Scale(scale) * worldRotate * Matrix4::Translate(pos + axisDir * 0.5 * length);

		LinearColor drawColor = (mGizmoAxis == axisIdx) ? LinearColor::White() : color;

		auto SetShaderParams = [&](Matrix4 const& w)
		{
			view.setupShader(commandList, *mPreviewVS);
			SET_SHADER_PARAM(commandList, *mPreviewVS, World, w);
			SET_SHADER_PARAM(commandList, *mPreviewPS, BaseColor, drawColor);
			SET_SHADER_PARAM(commandList, *mPreviewPS, EmissiveColor, drawColor * 0.5f);
			SET_SHADER_PARAM(commandList, *mPreviewPS, Roughness, 1.0f);
			SET_SHADER_PARAM(commandList, *mPreviewPS, Specular, 0.0f);
			SET_SHADER_PARAM(commandList, *mPreviewPS, SelectionAlpha, 0.0f);
		};

		SetShaderParams(world);
		getMesh(SimpleMeshId::Box).draw(commandList);

		// --- Tip shape at the end of the axis ---
		Vector3 tipPos = pos + axisDir * length;
		float tipSize = scaleFactor * 0.18f;   // uniform tip scale matching shaft

		// Rotate 'from' direction to axisDir
		auto MakeAlignRotation = [](Vector3 const& from, Vector3 const& to) -> Quaternion
		{
			float d = from.dot(to);
			if (d > 0.9999f)
				return Quaternion::Identity();
			if (d < -0.9999f)
			{
				// pick any perpendicular axis
				Vector3 perp = (Math::Abs(from.x) < 0.9f) ? Vector3(1, 0, 0) : Vector3(0, 1, 0);
				return Quaternion::Rotate(from.cross(perp).getNormal(), Math::PI);
			}
			Vector3 cross = from.cross(to);
			float angle = Math::ACos(Math::Clamp(d, -1.0f, 1.0f));
			return Quaternion::Rotate(cross.getNormal(), angle);
		};

		if (mGizmoType == EGizmoType::Translate)
		{
			Quaternion rot = MakeAlignRotation(Vector3(0, 0, 1), axisDir);
			Matrix4 tipWorld = Matrix4::Scale(tipSize) * Matrix4::Rotate(rot) * Matrix4::Translate(tipPos);
			SetShaderParams(tipWorld);
			getMesh(SimpleMeshId::Cone).draw(commandList);
		}
		else if (mGizmoType == EGizmoType::Rotate)
		{
			Quaternion rot = MakeAlignRotation(Vector3(0, 0, 1), axisDir);
			Matrix4 tipWorld = Matrix4::Scale(tipSize) * Matrix4::Rotate(rot) * Matrix4::Translate(tipPos);
			SetShaderParams(tipWorld);
			getMesh(SimpleMeshId::Doughnut2).draw(commandList);
		}

		else if (mGizmoType == EGizmoType::Scale)
		{
			Matrix4 tipWorld = Matrix4::Scale(tipSize * 0.5) * worldRotate * Matrix4::Translate(tipPos);
			SetShaderParams(tipWorld);
			getMesh(SimpleMeshId::Box).draw(commandList);
		}
	};
	
	drawAxis(GetAxisDir(obj, 0), LinearColor(1, 0, 0), 0);
	drawAxis(GetAxisDir(obj, 1), LinearColor(0, 1, 0), 1);
	drawAxis(GetAxisDir(obj, 2), LinearColor(0, 0, 1), 2);
}
#endif

