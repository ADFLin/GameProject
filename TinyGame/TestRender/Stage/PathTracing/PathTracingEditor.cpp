#include "GameConfig.h"

#if TINY_WITH_EDITOR

#include "PathTracingEditor.h"
#include "PathTracingCommon.h"
#include "PathTracingScene.h"
#include "PathTracingRenderer.h"
#include "DetailCustomization.h"
#include "Image/ImageData.h"
#include "FileSystem.h"
#include "Math/Base.h"

#include "RHI/DrawUtility.h"
#include "Renderer/RenderTargetPool.h"

#include "Stage/TestRenderStageBase.h"

#include "ProfileSystem.h"
#include "Math/PrimitiveTest.h"


namespace PathTracing
{

	class EditorPreviewVS : public GlobalShader
	{
		DECLARE_SHADER(EditorPreviewVS, Global);
	public:
		static char const* GetShaderFileName() { return "Shader/Game/EditorPreview"; }
		void bindParameters(ShaderParameterMap const& parameterMap)
		{
			BIND_SHADER_PARAM(parameterMap, World);
		}
		DEFINE_SHADER_PARAM(World);
	};

	class EditorPreviewPS : public GlobalShader
	{
		DECLARE_SHADER(EditorPreviewPS, Global);
	public:
		static char const* GetShaderFileName() { return "Shader/Game/EditorPreview"; }
		void bindParameters(ShaderParameterMap const& parameterMap)
		{
			BIND_SHADER_PARAM(parameterMap, BaseColor);
			BIND_SHADER_PARAM(parameterMap, EmissiveColor);
			BIND_SHADER_PARAM(parameterMap, Roughness);
			BIND_SHADER_PARAM(parameterMap, Specular);
			BIND_SHADER_PARAM(parameterMap, SelectionAlpha);
			BIND_SHADER_PARAM(parameterMap, ObjectIndex);
		}

		DEFINE_SHADER_PARAM(BaseColor);
		DEFINE_SHADER_PARAM(EmissiveColor);
		DEFINE_SHADER_PARAM(Roughness);
		DEFINE_SHADER_PARAM(Specular);
		DEFINE_SHADER_PARAM(SelectionAlpha);
		DEFINE_SHADER_PARAM(ObjectIndex);
	};

	class EditorPickingPS : public GlobalShader
	{
		DECLARE_SHADER(EditorPickingPS, Global);
	public:
		static char const* GetShaderFileName() { return "Shader/Game/EditorPreview"; }
		void bindParameters(ShaderParameterMap const& parameterMap)
		{
			BIND_SHADER_PARAM(parameterMap, ObjectIndex);
		}
		DEFINE_SHADER_PARAM(ObjectIndex);
	};

	class ScreenOutlinePS : public GlobalShader
	{
		DECLARE_SHADER(ScreenOutlinePS, Global);
	public:
		static char const* GetShaderFileName() { return "Shader/Game/ScreenOutline"; }
		void bindParameters(ShaderParameterMap const& parameterMap)
		{
			BIND_TEXTURE_PARAM(parameterMap, SceneDepthTexture);
			BIND_TEXTURE_PARAM(parameterMap, SceneColorTexture);
			BIND_SHADER_PARAM(parameterMap, OutlineColor);
			BIND_SHADER_PARAM(parameterMap, ScreenSize);
		}
		DEFINE_TEXTURE_PARAM(SceneDepthTexture);
		DEFINE_TEXTURE_PARAM(SceneColorTexture);
		DEFINE_SHADER_PARAM(OutlineColor);
		DEFINE_SHADER_PARAM(ScreenSize);
	};

	IMPLEMENT_SHADER(EditorPreviewVS, EShader::Vertex, SHADER_ENTRY(MainVS));
	IMPLEMENT_SHADER(EditorPreviewPS, EShader::Pixel, SHADER_ENTRY(MainPS));
	IMPLEMENT_SHADER(EditorPickingPS, EShader::Pixel, SHADER_ENTRY(PickingPS));
	IMPLEMENT_SHADER(ScreenOutlinePS, EShader::Pixel, SHADER_ENTRY(MainPS));


	class ObjectDataCustomization : public IDetailCustomization
	{
	public:
		ObjectDataCustomization()
		{
		}

		void customizeLayout(IDetailLayoutBuilder& builder) override
		{
			ObjectData* data = (ObjectData*)builder.getStructPtr();

			builder.addProperty("type");
			builder.addProperty("pos");
			builder.addProperty("rotation");

			switch (data->type)
			{
			case EObjectType::Sphere:
				builder.addProperty(Reflection::PropertyCollector::GetProperty<float>(), &data->meta.x, "Radius");
				builder.hideProperty("meta");
				break;
			case EObjectType::Cube:
				builder.addProperty("meta", "Half Size");
				break;
			case EObjectType::Quad:
				builder.addProperty(Reflection::PropertyCollector::GetProperty<float>(), &data->meta.x, "Width");
				builder.addProperty(Reflection::PropertyCollector::GetProperty<float>(), &data->meta.y, "Height");
				builder.hideProperty("meta");
				break;
			case EObjectType::Disc:
				builder.addProperty(Reflection::PropertyCollector::GetProperty<float>(), &data->meta.x, "Width");
				builder.addProperty(Reflection::PropertyCollector::GetProperty<float>(), &data->meta.y, "Height");
				builder.hideProperty("meta");
				break;
			case EObjectType::Mesh:
				builder.addProperty(Reflection::PropertyCollector::GetProperty<int>(), &data->meta.x, "Mesh ID");
				builder.addProperty("meta.y", "Scale");
				builder.hideProperty("meta");
				break;
			}

			builder.addProperty("materialId");
		}
	};

	void PathTracingEditor::initialize()
	{
		auto editor = ::Global::Editor();
		editor->addGameViewport(this);

		editor->registerCustomization(Reflection::GetStructType<ObjectData>(), std::make_shared<ObjectDataCustomization>());

		auto& renderConfig = mContext->getRenderConfig();

		DetailViewConfig config;
		config.name = "Ray Tracing Settings";
		mDetailView = editor->createDetailView(config);

		// --- Rendering category ---
		mDetailView->setCategory("Rendering");
		mDetailView->addValue(renderConfig.bUseBVH4, "Use BVH4");
		mDetailView->addValue(renderConfig.bUseDenoise, "Use Denoise");
		mDetailView->addValue(renderConfig.bSplitAccumulate, "Split Accumulate");
		mDetailView->addValue((int&)renderConfig.debugDisplayMode, "Debug Mode");
		mDetailView->addValue(renderConfig.bUseMIS, "Use MIS");
		mDetailView->addValue(renderConfig.bUseHardwareRayTracing, "Use Hardware RT");

		mDetailView->clearCategory();

		config.name = "Objects";
		mObjectsDetailView = editor->createDetailView(config);

		config.name = "Materials";
		mMaterialsDetailView = editor->createDetailView(config);

		config.name = "MeshInfos";
		mMeshInfosDetailView = editor->createDetailView(config);

		mDetailView->addCategoryCallback("Rendering", [this](char const*)
		{
			mContext->notifyDataChanged(EDataActionType::ModifyRenderConfig);
		});

		ToolBarConfig toolBarConfig;
		toolBarConfig.name = "Ray Tracing Tools";
		mToolBar = editor->createToolBar(toolBarConfig);
		mToolBar->addButton("Save Camera", [this]() { mContext->executeCommand(EEditorCommand::SaveCamera); });
		mToolBar->addButton("Load Camera", [this]() { mContext->executeCommand(EEditorCommand::LoadCamera); });
		mToolBar->addButton("Reload Scene", [this]() { mContext->executeCommand(EEditorCommand::ReloadScene); });
		mToolBar->addButton("Add Mesh", [this]()
		{
			char filePath[512] = "";
			OpenFileFilterInfo filters[] = { {"Mesh File", "*.fbx;*.glb;*.obj"} };
			if (SystemPlatform::OpenFileName(filePath, ARRAY_SIZE(filePath), filters, ""))
			{
				mContext->addMeshFromFile(filePath);
			}
		});
		mToolBar->addButton("Save Scene", [this]()
		{
			char filePath[512] = "RayTracingScene.json";
			OpenFileFilterInfo filters[] = { {"Json File", "*.json"} };
			if (SystemPlatform::SaveFileName(filePath, ARRAY_SIZE(filePath), filters, nullptr, "Save Scene"))
			{
				mContext->saveScene(filePath);
			}
		});
		mToolBar->addButton("Load Scene", [this]()
		{
			char filePath[512] = "";
			OpenFileFilterInfo filters[] = { {"Json File", "*.json"} };
			if (SystemPlatform::OpenFileName(filePath, ARRAY_SIZE(filePath), filters, "RayTracingScene.json"))
			{
				mContext->loadScene(filePath);
			}
		});
		mToolBar->addSeparator();
		mToolBar->addButton("Translate", [this]() { mGizmoType = EGizmoType::Translate; });
		mToolBar->addButton("Rotate", [this]() { mGizmoType = EGizmoType::Rotate; });
		mToolBar->addButton("Scale", [this]() { mGizmoType = EGizmoType::Scale; });
		mToolBar->addSeparator();
		mToolBar->addButton("Local/World", [this]()
		{
			mGizmoMode = (mGizmoMode == EGizmoMode::Local) ? EGizmoMode::World : EGizmoMode::Local;
		});
		mToolBar->addSeparator();
		mToolBar->addButton("Save Image", [this]()
		{
			mContext->saveImage();
		});
	}

	void PathTracingEditor::finalize()
	{
		if (mDetailView) mDetailView->release();
		if (mObjectsDetailView) mObjectsDetailView->release();
		if (mMaterialsDetailView) mMaterialsDetailView->release();
		if (mToolBar) mToolBar->release();

		if (::Global::Editor())
		{
			::Global::Editor()->removeGameViewport(this);
		}
	}

	bool PathTracingEditor::initializeRHI()
	{
		{
			ScreenVS::PermutationDomain permutationVector;
			permutationVector.set<ScreenVS::UseTexCoord>(true);
			VERIFY_RETURN_FALSE(mScreenVS = ShaderManager::Get().getGlobalShaderT<ScreenVS>(permutationVector));
		}

		VERIFY_RETURN_FALSE(mPreviewVS = ShaderManager::Get().getGlobalShaderT<EditorPreviewVS>());
		VERIFY_RETURN_FALSE(mPreviewPS = ShaderManager::Get().getGlobalShaderT<EditorPreviewPS>());
		VERIFY_RETURN_FALSE(mPickingPS = ShaderManager::Get().getGlobalShaderT<EditorPickingPS>());
		VERIFY_RETURN_FALSE(mScreenOutlinePS = ShaderManager::Get().getGlobalShaderT<ScreenOutlinePS>());

		{
			InputLayoutDesc desc;
			desc.addElement(0, EVertex::ATTRIBUTE_POSITION, EVertex::Float3);
			desc.addElement(0, EVertex::ATTRIBUTE_NORMAL, EVertex::Float3);
			mMeshInputLayout = RHICreateInputLayout(desc);
		}

		return true;
	}

	void PathTracingEditor::releaseRHI()
	{

	}

	void PathTracingEditor::renderViewport(IEditorViewportRenderContext& context)
	{
		PROFILE_ENTRY("RenderViewport");

		if (bEditorCameraValid == false)
		{
			auto& camera = mContext->getMainCamera();
			mEditorCamera.lookAt(camera.getPos(), camera.getPos() + camera.getViewDir(), camera.getUpDir());

			bEditorCameraValid = true;
		}

		RHICommandList& commandList = RHICommandList::GetImmediateList();


		auto& sceneData = mContext->getSceneData();
		auto& renderer = mContext->getRenderer();


		RHIBeginRender(false);

		using namespace Render;
		RenderTargetDesc desc;
		desc.size = mEditorViewportSize;
		desc.format = ETexture::RGBA32F;
		desc.debugName = "RayTracingStageColor";
		desc.clearColor = LinearColor(0.1f, 0.2f, 0.3f, 1.0f);
		PooledRenderTargetRef colorRT = GRenderTargetPool.fetchElement(desc);
		desc.format = ETexture::R32U;
		desc.debugName = "RayTracingStagePicking";
		desc.clearColor = LinearColor(float(0xffffffff), 0, 0, 0);
		mPickingRT = GRenderTargetPool.fetchElement(desc);

		desc.format = ETexture::D24S8;
		desc.debugName = "RayTracingStageDepth";
		desc.clearColor = LinearColor(FRHIZBuffer::FarPlane, 0, 0, 0);
		PooledRenderTargetRef depthRT = GRenderTargetPool.fetchElement(desc);

		RHIResourceTransition(commandList, { colorRT->texture, mPickingRT->texture, depthRT->texture }, EResourceTransition::RenderTarget);

		if (mFrameBuffer == nullptr)
		{
			mFrameBuffer = RHICreateFrameBuffer();
		}
		mFrameBuffer->setTexture(0, static_cast<RHITexture2D&>(*colorRT->texture));
		mFrameBuffer->setTexture(1, static_cast<RHITexture2D&>(*mPickingRT->texture));
		mFrameBuffer->setDepth(static_cast<RHITexture2D&>(*depthRT->texture));

		RHISetFrameBuffer(commandList, mFrameBuffer);

		if (mEditorViewportSize.x <= 0 || mEditorViewportSize.y <= 0)
		{
			RHIEndRender(false);
			GRenderTargetPool.freeAllUsedElements();
			return;
		}

		RHISetViewport(commandList, 0.0f, 0.0f, (float)mEditorViewportSize.x, (float)mEditorViewportSize.y);
		RHISetScissorRect(commandList, 0, 0, mEditorViewportSize.x, mEditorViewportSize.y);

		LinearColor clearColors[] = { LinearColor(0.1f, 0.2f, 0.3f, 1.0f), LinearColor(float(0xffffffff), 0, 0, 0) };
		RHIClearRenderTargets(commandList, EClearBits::All, clearColors, 2, FRHIZBuffer::FarPlane, 0);

		RHISetBlendState(commandList, TStaticBlendState<>::GetRHI());
		RHISetDepthStencilState(commandList, TStaticDepthStencilState<true>::GetRHI());
		RHISetRasterizerState(commandList, TStaticRasterizerState<>::GetRHI());

		float aspect = (float)mEditorViewportSize.x / (float)Math::Max(1, mEditorViewportSize.y);
		Matrix4 proj = PerspectiveMatrix(Math::DegToRad(60.0f), aspect, 0.01f, 1000.0f);

		static int sEditorFrameCount = 0;
		static Vector3 sLastEditorCameraPos;
		static Quaternion sLastEditorCameraRot;
		if (sLastEditorCameraPos != mEditorCamera.getPos() || sLastEditorCameraRot != mEditorCamera.getRotation())
		{
			sEditorFrameCount = 0;
			sLastEditorCameraPos = mEditorCamera.getPos();
			sLastEditorCameraRot = mEditorCamera.getRotation();
		}

		ViewInfo& view = mEditorView;
		view.setupTransform(mEditorCamera.getPos(), mEditorCamera.getRotation(), proj);
		view.rectOffset = IntVector2(0, 0);
		view.rectSize = mEditorViewportSize;
		view.frameCount = sEditorFrameCount++;

		GraphicsShaderStateDesc state;
		state.vertex = mPreviewVS->getRHI();
		state.pixel = mPreviewPS->getRHI();
		RHISetGraphicsShaderBoundState(commandList, state);
		view.setupShader(commandList, *mPreviewVS);

		auto SetupSahderParams = [&](int index, MaterialData const& mat, Matrix4 const& xForm)
		{
			SET_SHADER_PARAM(commandList, *mPreviewVS, World, xForm);
			SET_SHADER_PARAM(commandList, *mPreviewPS, BaseColor, LinearColor(mat.baseColor, Math::Max(0.1f, 1.0f - mat.refractive)));
			SET_SHADER_PARAM(commandList, *mPreviewPS, EmissiveColor, LinearColor(mat.emissiveColor, 1.0f));
			SET_SHADER_PARAM(commandList, *mPreviewPS, Roughness, mat.roughness);
			SET_SHADER_PARAM(commandList, *mPreviewPS, Specular, mat.specular);
			SET_SHADER_PARAM(commandList, *mPreviewPS, SelectionAlpha, 0.0f);
			SET_SHADER_PARAM(commandList, *mPreviewPS, ObjectIndex, index);
		};

		auto DrawObject = [&](int index, ObjectData const& obj, MaterialData const& mat, bool bSelected)
		{
			Matrix4 modelScale = Matrix4::Identity();
			Mesh* mesh = nullptr;
			switch (obj.type)
			{
			case EObjectType::Cube:
				modelScale = Matrix4::Scale(obj.meta); 
				mesh = &mContext->getMesh(SimpleMeshId::Box); 
				break;
			case EObjectType::Sphere: 
				modelScale = Matrix4::Scale(obj.meta.x); 
				mesh = &mContext->getMesh(SimpleMeshId::Sphere); 
				break;
			case EObjectType::Quad:
				modelScale = Matrix4::Scale(obj.meta.x, obj.meta.y, 1.0f);
				mesh = &mContext->getMesh(SimpleMeshId::Plane);
				break;
			case EObjectType::Disc:
				modelScale = Matrix4::Scale(obj.meta.x, obj.meta.y, 1.0f);
				mesh = &mContext->getMesh(SimpleMeshId::Disc);
				break;
			case EObjectType::Mesh:
				{
					modelScale = Matrix4::Scale(obj.meta.y);
					int meshId = AsValue<int32>(obj.meta.x);
					if (sceneData.meshes.isValidIndex(meshId))
					{
						auto const& meshData = sceneData.meshes[meshId];
						Matrix4 world = modelScale * Matrix4::Rotate(obj.rotation) * Matrix4::Translate(obj.pos);
						if (bSelected)
						{
							RHISetDepthStencilState(commandList, TStaticDepthStencilState<true, ECompareFunc::LessEqual, true, ECompareFunc::Always, EStencil::Replace, EStencil::Replace, EStencil::Replace, 0xff, 0xff>::GetRHI(), 1);
						}
						else
						{
							RHISetDepthStencilState(commandList, TStaticDepthStencilState<true>::GetRHI());
						}

						SetupSahderParams(index, mat, world);

						InputStreamInfo inputStream;
						inputStream.buffer = mContext->getRenderer().mVertexBuffer.getRHI();
						RHISetInputStream(commandList, mMeshInputLayout, &inputStream, 1);
						RHIDrawPrimitive(commandList, EPrimitive::TriangleList, meshData.startIndex, meshData.numTriangles * 3);
					}
				}
				return;

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

				SetupSahderParams(index, mat, world);
				mesh->draw(commandList);
			}
		};

		RHISetBlendState(commandList, TStaticBlendState<>::GetRHI());
		for (int i = 0; i < sceneData.objects.size(); ++i)
		{
			auto const& obj = sceneData.objects[i];
			auto const& mat = sceneData.materials[obj.materialId];

			if (mat.refractive == 0.0f)
			{
				DrawObject(i, obj, mat, i == mSelectedObjectId);
			}
		}

		RHISetBlendState(commandList, TStaticBlendState<
			CWM_RGBA, EBlend::SrcAlpha, EBlend::OneMinusSrcAlpha, EBlend::Add, EBlend::One, EBlend::Zero, EBlend::Add, false, true,
			CWM_RGBA>::GetRHI());
		for (int i = 0; i < sceneData.objects.size(); ++i)
		{
			auto const& obj = sceneData.objects[i];
			auto const& mat = sceneData.materials[obj.materialId];

			if (mat.refractive > 0.0f)
			{
				DrawObject(i, obj, mat, i == mSelectedObjectId);
			}
		}


		RHIResourceTransition(commandList, { colorRT->texture, depthRT->texture }, EResourceTransition::SRV);
		RHIResourceTransition(commandList, { mPickingRT->texture }, EResourceTransition::Present);

		RHISetFrameBuffer(commandList, context.frameBuffer);
		RHISetViewport(commandList, 0.0f, 0.0f, (float)mEditorViewportSize.x, (float)mEditorViewportSize.y);
		RHISetScissorRect(commandList, 0, 0, mEditorViewportSize.x, mEditorViewportSize.y);

		{
			GraphicsShaderStateDesc state;
			state.vertex = mScreenVS->getRHI();
			state.pixel = mScreenOutlinePS->getRHI();
			RHISetGraphicsShaderBoundState(commandList, state);

			if (mStencilSRV == nullptr || mStencilSRVTexture != depthRT->texture.get())
			{
				mStencilSRV = RHICreateSRV(static_cast<RHITexture2D&>(*depthRT->texture), ETexture::StencilView);
				mStencilSRVTexture = static_cast<RHITexture2D*>(depthRT->texture.get());
			}
			RHIShaderResourceView& stencilSRV = *mStencilSRV;
			SET_SHADER_TEXTURE_AND_SAMPLER(commandList, *mScreenOutlinePS, SceneDepthTexture, stencilSRV, TStaticSamplerState<ESampler::Point>::GetRHI());
			SET_SHADER_TEXTURE_AND_SAMPLER(commandList, *mScreenOutlinePS, SceneColorTexture, *colorRT->texture, TStaticSamplerState<>::GetRHI());
			SET_SHADER_PARAM(commandList, *mScreenOutlinePS, OutlineColor, (mSelectedObjectId != INDEX_NONE) ? LinearColor(1, 0.6, 0, 1) : LinearColor(0, 0, 0, 0));
			SET_SHADER_PARAM(commandList, *mScreenOutlinePS, ScreenSize, Vector2(mEditorViewportSize));

			RHISetBlendState(commandList, TStaticBlendState<>::GetRHI());
			RHISetDepthStencilState(commandList, StaticDepthDisableState::GetRHI());
			DrawUtility::ScreenRect(commandList);
		}

		RHIClearRenderTargets(commandList, EClearBits::Depth, nullptr, 0, FRHIZBuffer::FarPlane);

		RHISetBlendState(commandList, TStaticBlendState<>::GetRHI());
		RHISetDepthStencilState(commandList, TStaticDepthStencilState<>::GetRHI());

		if (mSelectedObjectId != INDEX_NONE)
		{
			auto const& obj = sceneData.objects[mSelectedObjectId];
			auto const& mat = sceneData.materials[obj.materialId];

			if ((mat.emissiveColor.r > 0.1f || mat.emissiveColor.g > 0.1f || mat.emissiveColor.b > 0.1f) && (obj.type == EObjectType::Quad || obj.type == EObjectType::Disc))
			{
				float visualDist = 4.0f; // Fixed distance for better visibility first
				// Map cosAngle from UI (interpreted as Cosine) to Angle
				float cosAngle = Math::Max(mat.emissiveAngle, 0.05f);
				float angle = Math::ACos(cosAngle);

				float radius = visualDist * Math::Tan(angle);								// Manual vertices are already shaped. World matrix only needs to move it to the object.
				Matrix4 world = Matrix4::Rotate(obj.rotation) * Matrix4::Translate(obj.pos);

				RHISetFixedShaderPipelineState(commandList, world * view.worldToClipRHI, LinearColor(1, 1, 0, 1.0f));
				RHISetRasterizerState(commandList, TStaticRasterizerState<ECullMode::None, EFillMode::Solid>::GetRHI());
				RHISetDepthStencilState(commandList, StaticDepthDisableState::GetRHI());
				
				// Draw wireframe cone manually using RenderRT (LineList)
				{
					int const numSides = 48;
					TArray<Vector3> lines;
					lines.reserve(numSides * 4);
					Vector3 tip(0, 0, 0);
					for (int i = 0; i < numSides; ++i)
					{
						float s, c, s1, c1;
						Math::SinCos(2 * Math::PI * i / numSides, s, c);
						Math::SinCos(2 * Math::PI * (i + 1) / numSides, s1, c1);
						Vector3 p0(radius * c, radius * s, visualDist);
						Vector3 p1(radius * c1, radius * s1, visualDist);
						
						// Circle segment
						lines.push_back(p0); lines.push_back(p1);
						// Side line
						if (i % 4 == 0) // Reduce side lines for clarity
						{
							lines.push_back(tip); lines.push_back(p0);
						}
					}
					TRenderRT<RTVF_XYZ>::Draw(commandList, EPrimitive::LineList, lines.data(), (int)lines.size());
				}

				RHISetDepthStencilState(commandList, TStaticDepthStencilState<>::GetRHI());
			}
		}
		RHISetRasterizerState(commandList, TStaticRasterizerState<>::GetRHI());

		if (mSelectedObjectId != INDEX_NONE)
		{
			auto const& obj = sceneData.objects[mSelectedObjectId];
			Quaternion gizmoRot = (mGizmoMode == EGizmoMode::Local || mGizmoType == EGizmoType::Scale) ? obj.rotation : Quaternion::Identity();
			drawGizmo(commandList, view, obj.pos, gizmoRot);
		}

		RHISetFixedShaderPipelineState(commandList, view.worldToClipRHI, LinearColor(1, 1, 1, 1));
		DrawUtility::AixsLine(commandList, 2.0f);


		RHIEndRender(false);

		GRenderTargetPool.freeUsedElement(colorRT);
		GRenderTargetPool.freeUsedElement(depthRT);

	}

	void PathTracingEditor::onViewportMouseEvent(MouseMsg const& msg)
	{
		static Vec2i oldPos = msg.getPos();
		if (msg.isRightDown())
		{
			float rotateSpeed = 0.01f;
			Vector2 off = rotateSpeed * Vector2(msg.getPos() - oldPos);
			mEditorCamera.rotateByMouse(off.x, off.y);
		}



		auto& sceneData = mContext->getSceneData();
		auto& renderer = mContext->getRenderer();


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

			auto const& obj = sceneData.objects[mSelectedObjectId];
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
				int hitId = pickObject(msg.getPos().x, msg.getPos().y);
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
			auto& obj = sceneData.objects[mSelectedObjectId];

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
					int metaIndex = INDEX_NONE;
					switch (obj.type)
					{
					case EObjectType::Sphere: metaIndex = 0; break;
					case EObjectType::Cube: metaIndex = mGizmoAxis; break;
					case EObjectType::Quad: metaIndex = mGizmoAxis; break;
					case EObjectType::Disc: metaIndex = mGizmoAxis; break;
					case EObjectType::Mesh: metaIndex = 1; break;
					}
					if (metaIndex != INDEX_NONE)
					{
						obj.meta[metaIndex] *= s;
					}
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

			mContext->notifyDataChanged(EDataActionType::ModifyObjectProperty);
		}

		oldPos = msg.getPos();
	}

	void PathTracingEditor::onViewportKeyEvent(unsigned key, bool isDown)
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
			auto& sceneData = mContext->getSceneData();

			if (key == EKeyCode::Delete)
			{
				if (mSelectedObjectId != INDEX_NONE)
				{
					sceneData.objects.erase(sceneData.objects.begin() + mSelectedObjectId);
					mSelectedObjectId = INDEX_NONE;
					mContext->notifyDataChanged(EDataActionType::RemoveObject);
					refreshDetailView();
				}
			}
			else if (key == EKeyCode::V && mbControlDown)
			{
				if (mSelectedObjectId != INDEX_NONE)
				{
					ObjectData copy = sceneData.objects[mSelectedObjectId];
					sceneData.objects.push_back(copy);
					mSelectedObjectId = sceneData.objects.size() - 1;
					mContext->notifyDataChanged(EDataActionType::AddObject);
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

	void PathTracingEditor::refreshDetailView()
	{
		if (mDetailView)
		{
			mDetailView->clearCategory();

			mDetailView->clearCategoryViews("Object");
			mDetailView->clearCategoryViews("Material");
			mDetailView->clearCategoryViews("MeshInfo");

			if (mSelectedObjectId != INDEX_NONE)
			{
				auto OnPropertyChange = [this](char const*)
				{
					mContext->notifyDataChanged(EDataActionType::ModifyObjectProperty);
				};

				auto& sceneData = mContext->getSceneData();
				auto& object = sceneData.objects[mSelectedObjectId];

				mDetailView->setCategory("Object");
				PropertyViewHandle hObj = mDetailView->addStruct(object);
				mDetailView->addCallback(hObj, OnPropertyChange);

				mDetailView->setCategory("Material");
				PropertyViewHandle hMat = mDetailView->addStruct(sceneData.materials[object.materialId]);
				mDetailView->addCallback(hMat, OnPropertyChange);

				if (sceneData.objects[mSelectedObjectId].type == EObjectType::Mesh)
				{
					int meshId = AsValue<int32>(sceneData.objects[mSelectedObjectId].meta.x);
					if (sceneData.meshInfos.isValidIndex(meshId))
					{

						mDetailView->setCategory("MeshInfo");
						PropertyViewHandle hMesh = mDetailView->addStruct(sceneData.meshInfos[meshId]);
						mDetailView->addCallback(hMesh, OnPropertyChange);
					}
				}
				mDetailView->clearCategory();
			}
		}
	}

	void PathTracingEditor::drawGizmo(RHICommandList& commandList, ViewInfo& view, Vector3 const& pos, Quaternion const& rotation)
	{
		if (mGizmoType == EGizmoType::None)
			return;

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

		auto GetAxisDir = [&](int idx)
		{
			Vector3 axis(0, 0, 0);
			axis[idx] = 1.0f;
			return rotation.rotate(axis);
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

			Matrix4 worldRotate = Matrix4::Rotate(rotation);

			world = Matrix4::Scale(scale) * worldRotate * Matrix4::Translate(pos + axisDir * 0.5 * length);

			LinearColor drawColor = (mGizmoAxis == axisIdx) ? LinearColor::White() : color;

			auto SetShaderParams = [&](Matrix4 const& xForm)
			{
				view.setupShader(commandList, *mPreviewVS);
				SET_SHADER_PARAM(commandList, *mPreviewVS, World, xForm);
				SET_SHADER_PARAM(commandList, *mPreviewPS, BaseColor, drawColor);
				SET_SHADER_PARAM(commandList, *mPreviewPS, EmissiveColor, 0.5 * drawColor);
				SET_SHADER_PARAM(commandList, *mPreviewPS, Roughness, 1.0f);
				SET_SHADER_PARAM(commandList, *mPreviewPS, Specular, 0.0f);
				SET_SHADER_PARAM(commandList, *mPreviewPS, SelectionAlpha, 0.0f);
			};

			SetShaderParams(world);
			mContext->getMesh(SimpleMeshId::Box).draw(commandList);

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
				mContext->getMesh(SimpleMeshId::Cone).draw(commandList);
			}
			else if (mGizmoType == EGizmoType::Rotate)
			{
				Quaternion rot = MakeAlignRotation(Vector3(0, 0, 1), axisDir);
				Matrix4 tipWorld = Matrix4::Scale(tipSize) * Matrix4::Rotate(rot) * Matrix4::Translate(tipPos);
				SetShaderParams(tipWorld);
				mContext->getMesh(SimpleMeshId::Doughnut2).draw(commandList);
			}

			else if (mGizmoType == EGizmoType::Scale)
			{
				Matrix4 tipWorld = Matrix4::Scale(tipSize * 0.5) * worldRotate * Matrix4::Translate(tipPos);
				SetShaderParams(tipWorld);
				mContext->getMesh(SimpleMeshId::Box).draw(commandList);
			}
		};

		drawAxis(GetAxisDir(0), LinearColor(1, 0, 0), 0);
		drawAxis(GetAxisDir(1), LinearColor(0, 1, 0), 1);
		drawAxis(GetAxisDir(2), LinearColor(0, 0, 1), 2);
	}

	int PathTracingEditor::pickObject(int x, int y)
	{
		if (x < 0 || y < 0 || x >= mEditorViewportSize.x || y >= mEditorViewportSize.y)
			return INDEX_NONE;

		if (!mPickingRT)
			return INDEX_NONE;


		RHIFlushCommand(RHICommandList::GetImmediateList());
		TArray<uint8> data;
		RHIReadTexture(static_cast<RHITexture2D&>(*mPickingRT->texture), ETexture::R32U, 0, data);

		if (data.empty())
			return INDEX_NONE;

		uint32* pIndices = (uint32*)data.data();
		uint32 index = pIndices[y * mEditorViewportSize.x + x];
		if (index == 0xffffffff)
			return INDEX_NONE;

		return (int)index;
	}
}

#endif