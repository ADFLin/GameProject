#include "RayTracingStage.h"
#include "RHI/RHIGraphics2D.h"
#include "InputManager.h"

#define CHAISCRIPT_NO_THREADS
#include "chaiscript/chaiscript.hpp"
namespace Chai = chaiscript;

REGISTER_STAGE_ENTRY("Raytracing Test", RayTracingTestStage, EExecGroup::FeatureDev, "Render");

IMPLEMENT_SHADER(RayTracingPS, EShader::Pixel, SHADER_ENTRY(MainPS));
IMPLEMENT_SHADER(AccumulatePS, EShader::Pixel, SHADER_ENTRY(AccumulatePS));

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
	auto choice = frame->addChoice("DebugDisplay Mode", UI_StatsMode);

	char const* ModeTextList[] = 
	{
		"None",
		"BoundingBox",
		"Triangle",
		"Mix",
		"HitNoraml",
		"HitPos",
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
	return true;
}

void RayTracingTestStage::onRender(float dFrame)
{
	GRenderTargetPool.freeAllUsedElements();

	RHICommandList& commandList = RHICommandList::GetImmediateList();
	RHISetFrameBuffer(commandList, nullptr);
	RHIClearRenderTargets(commandList, EClearBits::Color, &LinearColor(0, 0, 0, 1), 1, FRHIZBuffer::FarPlane);

	bool bResetFrameCount = mCamera.getPos() != mLastPos || mCamera.getRotation().getEulerZYX() != mLastRoation;
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

	Vec2i screenSize = ::Global::GetScreenSize();
	mSceneRenderTargets.prepare(screenSize);

	mViewFrustum.mNear = 0.01;
	mViewFrustum.mFar = 800.0;
	mViewFrustum.mAspect = float(screenSize.x) / screenSize.y;
	mViewFrustum.mYFov = Math::DegToRad(90 / mViewFrustum.mAspect);

	RHIResourceTransition(commandList, { &mSceneRenderTargets.getFrameTexture() }, EResourceTransition::RenderTarget);
	RHIResourceTransition(commandList, { &mSceneRenderTargets.getPrevFrameTexture() }, EResourceTransition::SRV);
	RHISetViewport(commandList, 0, 0, screenSize.x, screenSize.y);
	RHISetFrameBuffer(commandList, mSceneRenderTargets.getFrameBuffer());

	mView.rectOffset = IntVector2(0, 0);
	mView.rectSize = screenSize;


	mView.setupTransform(mCamera.getPos(), mCamera.getRotation(), mViewFrustum.getPerspectiveMatrix());

	RHISetBlendState(commandList, TStaticBlendState<>::GetRHI());
	RHISetDepthStencilState(commandList, StaticDepthDisableState::GetRHI());
	RHISetRasterizerState(commandList, TStaticRasterizerState<ECullMode::None>::GetRHI());

	{
		RayTracingPS* rayTracingPS = mRayTracingPSMap[mDebugDisplayMode == EDebugDsiplayMode::None ? (bSplitAccumulate ? 2 : 0 ) : 1];

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
		SetStructuredStorageBuffer(commandList, *rayTracingPS, mBVHNodeBuffer);
		SetStructuredStorageBuffer< SceneBVHNodeData >(commandList, *rayTracingPS, mSceneBVHNodeBuffer);
		SetStructuredStorageBuffer< ObjectIdData >(commandList, *rayTracingPS, mObjectIdBuffer);

		rayTracingPS->setParam(commandList, SHADER_PARAM(NumObjects), mNumObjects);
		SET_SHADER_TEXTURE_AND_SAMPLER(commandList, *rayTracingPS, HistoryTexture, mSceneRenderTargets.getPrevFrameTexture(), TStaticSamplerState<>::GetRHI());

		DrawUtility::ScreenRect(commandList);


	}

	if (mDebugDisplayMode == EDebugDsiplayMode::None && bSplitAccumulate)
	{
		GPU_PROFILE("Accumulate");

		GraphicsShaderStateDesc state;
		state.vertex = mScreenVS->getRHI();
		state.pixel = mAccumulatePS->getRHI();
		RHISetGraphicsShaderBoundState(commandList, state);

		SET_SHADER_TEXTURE_AND_SAMPLER(commandList, *mAccumulatePS, HistoryTexture, mSceneRenderTargets.getPrevFrameTexture(), TStaticSamplerState<>::GetRHI());
		DrawUtility::ScreenRect(commandList);

	}

	RHIResourceTransition(commandList, { &mSceneRenderTargets.getFrameTexture() }, EResourceTransition::SRV);

	{
		GPU_PROFILE("CopyToBackBuffer");
		RHISetFrameBuffer(commandList, nullptr);
		RHISetViewport(commandList, 0, 0, screenSize.x, screenSize.y);
		ShaderHelper::Get().copyTextureToBuffer(commandList, mSceneRenderTargets.getFrameTexture());

		mSceneRenderTargets.swapFrameTexture();
	}

	if ( mbDrawDebug )
	{
		GPU_PROFILE("DrawDebug");
		mDebugPrimitives.drawDynamic(commandList, mView);

		RHIGraphics2D& g = Global::GetRHIGraphics2D();
		g.beginRender();

		if(0)
		{
			TRANSFORM_PUSH_SCOPE(g.getTransformStack(), mWorldToScreen, true);
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
		TArray< MeshData > meshes;
		TArray< ObjectData> objects;
		TArray< MaterialData > materials;
		BVHTree meshBVH;
		TArray< BVHNodeData > meshBVHNodes;

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
			TArrayView< BVHNodeData const > nodes;
			int nodeIndex;
			int triangleIndex;
			int numTriangles;
		};

		void buildMeshData(MeshImportData& meshData, BuildMeshResult& buildResult)
		{
			TIME_SCOPE("BuildMeshData");

			auto posReader = meshData.makeAttributeReader(EVertex::ATTRIBUTE_POSITION);
			auto normalReader = meshData.makeAttributeReader(EVertex::ATTRIBUTE_NORMAL);

			int numTriangles = meshData.indices.size() / 3;

			int triangleIndex = meshVertices.size();

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

			if (meshes.size() == 0)
			{
				mScene.mDebugBVH = meshBVH;
			}

			meshVertices.resize(triangleIndex + 3 * numTriangles);

			int numV = GenerateTriangleVertices(meshBVH, meshData, meshVertices.data() + triangleIndex);
			CHECK(numV == numTriangles * 3);
			int nodeIndex = meshBVHNodes.size();
			BVHNodeData::Generate(meshBVH, meshBVHNodes, triangleIndex);
			buildResult.vertices = TArrayView<MeshVertexData const>(meshVertices.data() + triangleIndex , numV);
			buildResult.nodes    = TArrayView<BVHNodeData const>(meshBVHNodes.data() + nodeIndex, meshBVH.nodes.size());
			buildResult.nodeIndex = nodeIndex;
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

		int loadMesh(char const* path)
		{
			TIME_SCOPE("LoadMesh");
			MeshData mesh;

			auto& dataCache = ::Global::DataCache();
			DataCacheKey cacheKey;
			cacheKey.typeName = "MESH_BVH";
			cacheKey.version = "1e987655-09dc-431a-bf73-116bc953dfe6";
			cacheKey.keySuffix.add(path);

			TArray< MeshVertexData > vertices;
			TArray< BVHNodeData > nodes;
			int nodeIndex;
			int triangleIndex;
			auto LoadCache = [&](IStreamSerializer& serializer)-> bool
			{
				serializer >> vertices;
				serializer >> nodes;
				serializer >> nodeIndex;
				serializer >> triangleIndex;
				return true;
			};
			if (dataCache.loadDelegate(cacheKey, LoadCache))
			{
				FixOffset(nodes, (int)meshBVHNodes.size() - nodeIndex, (int)meshVertices.size() - triangleIndex);

				nodeIndex = meshBVHNodes.size();
				triangleIndex = meshVertices.size();
				meshBVHNodes.append(nodes);
				meshVertices.append(vertices);

				mesh.startIndex = triangleIndex;
				mesh.numTriangles = vertices.size() / 3;
				mesh.nodeIndex = nodeIndex;
			}
			else
			{
				MeshImportData meshData;
				if ( !meshImporter->importFromFile(path, meshData) )
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
					return true;
				});

				mesh.startIndex = buildResult.triangleIndex;
				mesh.numTriangles = buildResult.numTriangles;
				mesh.nodeIndex = buildResult.nodeIndex;
			}

			meshes.push_back(mesh);
			return meshes.size() - 1;
		}

		bool addDefaultObjects()
		{
			{
				VERIFY_RETURN_FALSE(loadMesh("Mesh/dragonMid.fbx") != INDEX_NONE);
				//VERIFY_RETURN_FALSE(loadMesh("Mesh/dragon.fbx") != INDEX_NONE);
#if 1
				VERIFY_RETURN_FALSE(loadMesh("Mesh/Knight.fbx") != INDEX_NONE);
				VERIFY_RETURN_FALSE(loadMesh("Mesh/King.fbx") != INDEX_NONE);


				VERIFY_RETURN_FALSE(loadMesh("Mesh/Suzanne.fbx") != INDEX_NONE);
#endif
			}

			{
				float L = 55;
				const MaterialData mats[] =
				{
					{ Color3f(0,0,0), 1.0, L * Color3f(1,1,1) ,0 },
					{ Color3f(1,1,1), 1.0, Color3f(0,0,0) ,0},
					{ Color3f(0,1,0), 1.0, Color3f(0,0,0) ,0},
					{ Color3f(1,0,0), 1.0, Color3f(0,0,0) ,0},
					{ Color3f(0,0,1), 1.0, Color3f(0,0,0) ,0},
					{ 0.5 * Color3f(1,1,1), 1.0, Color3f(0,0,0) ,0},
					{ Color3f(1,1,1), 1.0, Color3f(0,0,0) ,1.51, 0.8},
				};

				materials.append(mats, mats + ARRAY_SIZE(mats));

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
					FObject::Mesh(0, 0.04, 1, Vector3(0,0,1), Quaternion::Rotate(Vector3(0,0,1) , Math::DegToRad(60))),
					//FObject::Mesh(2, 1.5, 1, Vector3(0,0,-0.5 *length + 2) , Quaternion::Rotate(Vector3(0,0,1) , Math::DegToRad(70))),
					//FObject::Sphere(0.4, 6, Vector3(0, 1.5, 4.0)),
					//FObject::Sphere(0.4, 6, Vector3(8, 0, 2.5)),
#else
					FObject::Mesh(0, 1, 1, Vector3(0,0,0)),
#endif
				};
				objects.append(objs, objs + ARRAY_SIZE(objs));

			}
			return true;
		}

		bool buildRHIResource()
		{
			TIME_SCOPE("BuildRHIResource");

			if (meshes.size() > 0)
			{
				VERIFY_RETURN_FALSE(mScene.mVertexBuffer.initializeResource(MakeConstView(meshVertices), EStructuredBufferType::Buffer));
				VERIFY_RETURN_FALSE(mScene.mMeshBuffer.initializeResource(MakeConstView(meshes), EStructuredBufferType::Buffer));
				VERIFY_RETURN_FALSE(mScene.mBVHNodeBuffer.initializeResource(MakeConstView(meshBVHNodes), EStructuredBufferType::Buffer));
			}

			{

				auto GetAABB = [](Quaternion const& rotation, Vector3 const halfSize) -> Math::TAABBox<Vector3>
				{
					Math::TAABBox<Vector3> result;

					result.invalidate();
					for (uint32 i = 0; i < 8; ++i)
					{
						Vector3 offset;
						offset.x = (i & 0x1) ? halfSize.x : -halfSize.x;
						offset.y = (i & 0x2) ? halfSize.y : -halfSize.y;
						offset.z = (i & 0x4) ? halfSize.z : -halfSize.z;
						result.addPoint(rotation.rotate(offset));
					}
					return result;
				};

				auto GetAABB2 = [](Vector3 const& center, Quaternion const& rotation, Vector3 const halfSize) -> Math::TAABBox<Vector3>
				{
					Math::TAABBox<Vector3> result;

					result.invalidate();
					for (uint32 i = 0; i < 8; ++i)
					{
						Vector3 offset;
						offset.x = (i & 0x1) ? halfSize.x : -halfSize.x;
						offset.y = (i & 0x2) ? halfSize.y : -halfSize.y;
						offset.z = (i & 0x4) ? halfSize.z : -halfSize.z;
						result.addPoint(rotation.rotate(offset + center));
					}
					return result;
				};

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

							Vector3 boundMin = meshBVHNodes[mesh.nodeIndex].boundMin;
							Vector3 boundMax = meshBVHNodes[mesh.nodeIndex].boundMax;


							Vector3 offset = (scale * 0.5) * (boundMax + boundMin);
							Vector3 halfSize = (scale * 0.5) * (boundMax - boundMin);

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

				}

				BVHTree::Builder builder(objectBVH);
				builder.maxLeafPrimitiveCount = 2;

				int indexRoot = builder.build(MakeConstView(primitives));


				TArray< BVHNodeData > nodes;
				TArray< int > objectIds;
				BVHNodeData::Generate(objectBVH, nodes, objectIds);

				VERIFY_RETURN_FALSE(mScene.mSceneBVHNodeBuffer.initializeResource(nodes.size(), EStructuredBufferType::Buffer));
				mScene.mSceneBVHNodeBuffer.updateBuffer(nodes);

				VERIFY_RETURN_FALSE(mScene.mObjectIdBuffer.initializeResource(objectIds.size(), EStructuredBufferType::Buffer));
				mScene.mObjectIdBuffer.updateBuffer(objectIds);

				mScene.mNumObjects = objects.size();
				VERIFY_RETURN_FALSE(mScene.mObjectBuffer.initializeResource(objects.size(), EStructuredBufferType::Buffer));
				mScene.mObjectBuffer.updateBuffer(objects);

				VERIFY_RETURN_FALSE(mScene.mMaterialBuffer.initializeResource(materials.size(), EStructuredBufferType::Buffer));
				mScene.mMaterialBuffer.updateBuffer(materials);
			}

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
#if 0
			try
			{
				std::string path = GetFilePath(fileName);
				TIME_SCOPE("Script Eval");
				script.eval_file(path.c_str());
			}
			catch (const std::exception& e)
			{
				LogMsg("Load Scene Fail!! : %s", e.what());
				return false;
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
			script.add(Chai::fun(&SceneScriptImpl::Sphere, this), SCRIPT_NAME(Sphere));
			script.add(Chai::fun(&SceneScriptImpl::Mesh, this), SCRIPT_NAME(Mesh));
			script.add(Chai::fun(&SceneScriptImpl::Material, this), SCRIPT_NAME(Material));
			script.add(Chai::fun(&SceneScriptImpl::LoadMesh, this), SCRIPT_NAME(LoadMesh));
			script.add(Chai::fun(&SceneScriptImpl::AddDefaultObjects, this), SCRIPT_NAME(AddDefaultObjects));
		}

		void Sphere(int matId, Vector3 const& pos , float radius)
		{
			mBuilder->objects.push_back(FObject::Sphere(radius, matId, pos));
		}
		void Mesh(int meshId, int matId, Vector3 const& pos, float scale)
		{
			mBuilder->objects.push_back(FObject::Mesh(meshId, scale, matId, pos));
		}

		void Box(int matId, Vector3 pos, Vector3 size)
		{
			mBuilder->objects.push_back(FObject::Box(size, matId, pos));
		}

		int LoadMesh(std::string const& path)
		{
			return mBuilder->loadMesh(path.c_str());
		}

		int Material(
			Color3f baseColor,
			float   roughness,
			Color3f emissiveColor,
			float   refractiveIndex,
			float   refractive)
		{
			int result = mBuilder->materials.size();
			mBuilder->materials.push_back({ baseColor, roughness, emissiveColor, refractiveIndex, refractive } );
			return result;
		}

		void AddDefaultObjects()
		{
			mBuilder->addDefaultObjects();

			int id = mBuilder->meshes.size();
			std::map<std::string, Chai::Boxed_Value> locals;
			locals.emplace("DefMeshId_0", id - 3);
			locals.emplace("DefMeshId_1", id - 2);
			locals.emplace("DefMeshId_2", id - 1);	
#if 0
			id = mBuilder->materials.size();
			locals.emplace("DefMatId_0", id - 3);
			locals.emplace("DefMatId_1", id - 2);
			locals.emplace("DefMatId_2", id - 1);
#endif
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

bool RayTracingTestStage::setupRenderResource(ERenderSystem systemName)
{
	VERIFY_RETURN_FALSE(ShaderHelper::Get().init());

	{
		ScreenVS::PermutationDomain permutationVector;
		permutationVector.set<ScreenVS::UseTexCoord>(true);
		VERIFY_RETURN_FALSE(mScreenVS = ::ShaderManager::Get().getGlobalShaderT<ScreenVS>(permutationVector));
	}

	for (int i = 0; i < 2; ++i)
	{
		RayTracingPS::PermutationDomain permutationVector;
		permutationVector.set<RayTracingPS::UseDebugDisplay>(i);
		VERIFY_RETURN_FALSE(mRayTracingPSMap[i] = ::ShaderManager::Get().getGlobalShaderT<RayTracingPS>(permutationVector));
	}

	{
		RayTracingPS::PermutationDomain permutationVector;
		permutationVector.set<RayTracingPS::UseDebugDisplay>(false);
		permutationVector.set<RayTracingPS::UseSplitAccumulate>(true);
		VERIFY_RETURN_FALSE(mRayTracingPSMap[2] = ::ShaderManager::Get().getGlobalShaderT<RayTracingPS>(permutationVector));
	}
	VERIFY_RETURN_FALSE(mAccumulatePS = ::ShaderManager::Get().getGlobalShaderT<AccumulatePS>());

	VERIFY_RETURN_FALSE(mSceneRenderTargets.initializeRHI());
	mDebugPrimitives.initializeRHI();

	VERIFY_RETURN_FALSE(loadSceneRHIResource());

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
	std::fill_n( mRayTracingPSMap, ARRAY_SIZE(mRayTracingPSMap), nullptr);
	mView.releaseRHIResource();
	mMaterialBuffer.releaseResource();
	mObjectBuffer.releaseResource();
	mVertexBuffer.releaseResource();
	mMeshBuffer.releaseResource();
	mBVHNodeBuffer.releaseResource();
	mSceneBVHNodeBuffer.releaseResource();
	mObjectBuffer.releaseResource();

	mSceneRenderTargets.releaseRHI();
	mDebugPrimitives.releaseRHI();
}
