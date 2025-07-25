#include "BloxorzStage.h"
#include "StageRegister.h"

#include "EasingFunction.h"
#include "GameGUISystem.h"
#include "InputManager.h"

#include "TileRegion.h"

#include "RHI/RHIGraphics2D.h"
#include "RHI/RHICommand.h"
#include "RHI/RHIGlobalResource.h"
#include "RHI/RHIType.h"
#include "RHI/ShaderManager.h"
#include "RHI/GpuProfiler.h"
#include "Renderer/MeshBuild.h"

#include "FileSystem.h"
#include "ConsoleSystem.h"
#include "Misc/Format.h"

namespace Bloxorz
{
	using namespace Render;

	Object::Object()
		:mPos(0, 0, 0)
	{
		mNumBodies = 1;
		mBodiesPos[0] = Vec3i(0, 0, 0);
	}

	Object::Object(Object const& rhs)
		:mPos(rhs.mPos)
		, mNumBodies(rhs.mNumBodies)
	{
		std::copy(rhs.mBodiesPos, rhs.mBodiesPos + mNumBodies, mBodiesPos);
	}

	void Object::initBody(Vec3i const& pos)
	{
		CHECK(pos.x >= 0 && pos.y >= 0 && pos.z >= 0);
		addBody(pos);
	}

	void Object::growBody(Vec3i const& pos)
	{
		Vec3i localPos = pos - mPos;
		addBody(localPos);
		if (localPos.x < 0 || localPos.y < 0 || localPos.z < 0)
		{
			Vec3i offset;
			offset.x = Math::Max(0, -localPos.x);
			offset.y = Math::Max(0, -localPos.y);
			offset.z = Math::Max(0, -localPos.z);
			for (int i = 0; i < mNumBodies; ++i)
			{
				mBodiesPos[i] += offset;
			}
			mPos -= offset;
		}
	}

	void Object::addBody(Vec3i const& pos)
	{
		CHECK(findBody(pos) == INDEX_NONE);
		CHECK(mNumBodies < MaxBodyNum);

		mBodiesPos[mNumBodies] = pos;
		++mNumBodies;
	}

	void Object::move(Dir dir)
	{
		uint32 idxAxis = dir & 1;

		if (isNegative(dir))
		{
			int maxOffset = 0;
			for (int i = 0; i < mNumBodies; ++i)
			{
				Vec3i& bodyPos = mBodiesPos[i];
				CHECK(bodyPos.x >= 0 && bodyPos.y >= 0 && bodyPos.z >= 0);
				int offset = bodyPos.z;

				bodyPos.z = bodyPos[idxAxis];
				bodyPos[idxAxis] = -offset;

				if (maxOffset < offset)
					maxOffset = offset;
			}

			for (int i = 0; i < mNumBodies; ++i)
			{
				Vec3i& bodyPos = mBodiesPos[i];
				bodyPos[idxAxis] += maxOffset;
			}

			mPos[idxAxis] -= maxOffset + 1;
		}
		else
		{
			int maxOffset = 0;
			for (int i = 0; i < mNumBodies; ++i)
			{
				Vec3i& bodyPos = mBodiesPos[i];
				CHECK(bodyPos.x >= 0 && bodyPos.y >= 0 && bodyPos.z >= 0);
				int offset = bodyPos[idxAxis];

				bodyPos[idxAxis] = bodyPos.z;
				bodyPos.z = -offset;

				if (maxOffset < offset)
					maxOffset = offset;
			}

			for (int i = 0; i < mNumBodies; ++i)
			{
				Vec3i& bodyPos = mBodiesPos[i];
				bodyPos.z += maxOffset;
			}

			mPos[idxAxis] += maxOffset + 1;
		}
	}

	int Object::getMaxLocalPosX() const
	{
		int result = 0;
		for (int i = 0; i < mNumBodies; ++i)
		{
			Vec3i const& bodyPos = mBodiesPos[i];
			if (result < bodyPos.x)
				result = bodyPos.x;
		}
		return result;
	}

	int Object::getMaxLocalPosY() const
	{
		int result = 0;
		for (int i = 0; i < mNumBodies; ++i)
		{
			Vec3i const& bodyPos = mBodiesPos[i];
			if (result < bodyPos.y)
				result = bodyPos.y;
		}
		return result;
	}

	int Object::getMaxLocalPosZ() const
	{
		int result = 0;
		for (int i = 0; i < mNumBodies; ++i)
		{
			Vec3i const& bodyPos = mBodiesPos[i];
			if (result < bodyPos.z)
				result = bodyPos.z;
		}
		return result;
	}

	enum TileID
	{
		TILE_NONE = 0,
		TILE_NORAML,
		TILE_GOAL,
		TILE_BLOCK,
		TILE_GROW,
	};

#define G TILE_GOAL
#define B TILE_BLOCK
#define W TILE_GROW
	int map[] =
	{
		1, 1, 1, W, 1, 1, 1, 1, 1, 1, 0, 0,
		1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0,
		1, 1, 1, B, 0, 0, 1, 1, 1, 1, 0, 0,
		1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0,
		1, 1, 1, 1, 1, 1, 1, 0, 1, 1, 0, 0,
		1, 1, 0, 1, 1, 1, 1, 1, 0, 1, 0, 0,
		1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0,
		1, 1, 1, 1, 1, 1, G, 1, 1, 1, 0, 0,
		1, 1, 1, 1, 1, 1, G, 1, 1, 1, 0, 0,
		1, 1, 0, 1, 1, 1, 1, 1, 1, 1, 0, 0,
		1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0,
		1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0,
		1, 1, 1, 0, 1, 1, 1, 0, 0, 1, 0, 0,
		1, 1, 1, 0, 1, 1, 1, 0, 0, 1, 0, 0,
		1, 1, 1, 0, 1, 1, 1, 0, 0, 1, 0, 0,
		1, 1, 1, 0, 1, 1, 1, 0, 0, 1, 0, 0,
		1, 1, 1, 0, 1, 1, 1, 0, 0, 1, 0, 0,
		1, 1, 1, 0, 1, 1, 1, 0, 0, 1, 0, 0,
		1, 1, 1, 0, 1, 1, 1, 0, 0, 1, 0, 0,
		1, 1, 1, 0, 1, 1, 1, 0, 0, 1, 0, 0,
	};
#undef G
#undef B
#undef W

	class RayTraceProgramBase : public GlobalShaderProgram
	{
	public:
		static char const* GetShaderFileName()
		{
			return "Shader/Game/RayTraceSDF";
		}

		void bindParameters(ShaderParameterMap const& parameterMap)
		{
			BIND_SHADER_PARAM(parameterMap, ObjectNum);
			BIND_SHADER_PARAM(parameterMap, MapTileNum);
		}

		DEFINE_SHADER_PARAM(ObjectNum);
		DEFINE_SHADER_PARAM(MapTileNum);
	};

	class RayTraceProgram : public RayTraceProgramBase
	{
		DECLARE_SHADER_PROGRAM(RayTraceProgram, Global);
	public:
		SHADER_PERMUTATION_TYPE_BOOL(UseBuiltinScene, SHADER_PARAM(USE_BUILTIN_SCENE));
		SHADER_PERMUTATION_TYPE_BOOL(UseDeferredRendering, SHADER_PARAM(USE_DEFERRED_RENDERING));
		using PermutationDomain = TShaderPermutationDomain<UseBuiltinScene, UseDeferredRendering>;

		static TArrayView< ShaderEntryInfo const > GetShaderEntries(PermutationDomain const& domain)
		{
			static ShaderEntryInfo const entries[] =
			{
				{ EShader::Vertex , SHADER_ENTRY(ScreenVS) },
				{ EShader::Pixel  , SHADER_ENTRY(BasePassPS) },
			};
			return entries;
		}

		void bindParameters(ShaderParameterMap const& parameterMap)
		{
			BIND_SHADER_PARAM(parameterMap, ObjectNum);
			BIND_SHADER_PARAM(parameterMap, MapTileNum);
		}

		DEFINE_SHADER_PARAM(ObjectNum);
		DEFINE_SHADER_PARAM(MapTileNum);
	};

	IMPLEMENT_SHADER_PROGRAM(RayTraceProgram);
	
	class RayTraceLightingProgram : public RayTraceProgramBase
	{
		DECLARE_SHADER_PROGRAM(RayTraceLightingProgram, Global);
		using BassClass = RayTraceProgramBase;
	public:

		static void SetupShaderCompileOption(ShaderCompileOption& option)
		{
			option.addDefine(SHADER_PARAM(USE_BUILTIN_SCENE), true);
			option.addDefine(SHADER_PARAM(USE_DEFERRED_RENDERING), true);
		}

		static TArrayView< ShaderEntryInfo const > GetShaderEntries()
		{
			static ShaderEntryInfo const entries[] =
			{
				{ EShader::Vertex , SHADER_ENTRY(ScreenVS) },
				{ EShader::Pixel  , SHADER_ENTRY(LightingPS) },
			};
			return entries;
		}

		void bindParameters(ShaderParameterMap const& parameterMap)
		{
			BassClass::bindParameters(parameterMap);
			BIND_TEXTURE_PARAM(parameterMap, RenderTargetA);
			BIND_TEXTURE_PARAM(parameterMap, RenderTargetB);
		}

		void setRenderTargets(RHICommandList& commandList, RHITexture2DRef textures[])
		{
			SET_SHADER_TEXTURE_AND_SAMPLER(commandList, *this, RenderTargetA, *textures[0], TStaticSamplerState<>::GetRHI());
			SET_SHADER_TEXTURE_AND_SAMPLER(commandList, *this, RenderTargetB, *textures[1], TStaticSamplerState<>::GetRHI());
		}

		void clearRenderTargets(RHICommandList& commandList)
		{
			clearTexture(commandList, mParamRenderTargetA);
			clearTexture(commandList, mParamRenderTargetB);
		}

		DEFINE_TEXTURE_PARAM(RenderTargetA);
		DEFINE_TEXTURE_PARAM(RenderTargetB);
	};
	IMPLEMENT_SHADER_PROGRAM(RayTraceLightingProgram);


	bool TestStage::onInit()
	{
		::Global::GUI().cleanupWidget();

		Vec2i screenSize = ::Global::GetScreenSize();

		mTweener.tweenValue< Easing::CLinear >(mSpot, 0.0f, 0.5f, 1.0f).cycle();

		//mObject.initBody(Vec3i(0, 0, 0));
		mObject.initBody(Vec3i(0, 0, 1));
#if 0
		mObject.initBody(Vec3i(0, 0, 1));
		mObject.initBody(Vec3i(0, 0, 2));
		mObject.initBody(Vec3i(0, 1, 0));
		mObject.initBody(Vec3i(0, 1, 1));
		mObject.initBody(Vec3i(0, 1, 2));

		mObject.initBody(Vec3i(0, 2, 0));
		mObject.initBody(Vec3i(0, 2, 1));
		mObject.initBody(Vec3i(0, 2, 2));
		mObject.initBody(Vec3i(0, 3, 0));
		mObject.initBody(Vec3i(0, 3, 1));
		mObject.initBody(Vec3i(0, 3, 2));
#endif

		mMap.resize(12, 20);
		for (int i = 0; i < 12 * 20; ++i)
			mMap[i] = map[i];

		mLookPos.x = mObject.getPos().x + 1;
		mLookPos.y = mObject.getPos().y + 1;
		mLookPos.z = 0;

		mRotateAngle = 0;
		mMoveCur = DIR_NONE;
		mIsGoal = false;

		mCamera.lookAt(Vector3(10, 10, 10), Vector3(0, 0, 0), Vector3(0, 0, 1));

		DevFrame* frame = WidgetUtility::CreateDevFrame();
		FWidgetProperty::Bind(frame->addCheckBox(UI_ANY, "bUseRayTrace"), bUseRayTrace);
		FWidgetProperty::Bind(frame->addCheckBox(UI_ANY, "bUseSceneButin"), bUseSceneBuitin);
		FWidgetProperty::Bind(frame->addCheckBox(UI_ANY, "bUseDeferredRending"), bUseDeferredRending);
		FWidgetProperty::Bind(frame->addCheckBox(UI_ANY, "bFreeView"), bFreeView);
		FWidgetProperty::Bind(frame->addCheckBox(UI_ANY, "bMoveCamera"), bMoveCamera);
		FWidgetProperty::Bind(frame->addCheckBox(UI_ANY, "bUseBloom"), bUseBloom);

		auto UpdateSceneEnvBufferFunc = [this](float) { updateSceneEnvBuffer(); };

		FWidgetProperty::Bind(frame->addSlider("Sun Intensity"), mSceneEnv.sunIntensity, 0, 10, 1, UpdateSceneEnvBufferFunc);
		FWidgetProperty::Bind(frame->addSlider("Fog Distance"), mSceneEnv.fogDistance, 0, 150, 1.3, UpdateSceneEnvBufferFunc);
		FWidgetProperty::Bind(frame->addSlider("Blur Radius Scale"), blurRadiusScale, 0, 30, 1);
		FWidgetProperty::Bind(frame->addSlider("Bloom Intensity"), mBloomIntensity, 0, 5, 1);
		FWidgetProperty::Bind(frame->addSlider("Bloom Threshold"), mBloomThreshold, -0.5, 2, 1);


		restart();
		return true;
	}

	void TestStage::onEnd()
	{
		IConsoleSystem::Get().unregisterCommandByName("ShowTexture");

		BaseClass::onEnd();
	}

	void TestStage::configRenderSystem(ERenderSystem systenName, RenderSystemConfigs& systemConfigs)
	{
		systemConfigs.bVSyncEnable = false;
#if 1
		systemConfigs.screenWidth = 1280;
		systemConfigs.screenHeight = 720;
#else
		systemConfigs.screenWidth = 800;
		systemConfigs.screenHeight = 600;
#endif
	}


	bool TestStage::setupRenderResource(ERenderSystem systemName)
	{
#if 0
		if (GRHISystem->getName() == RHISystemName::OpenGL)
		{
			glEnable(GL_POLYGON_OFFSET_LINE);
			glPolygonOffset(-0.4f, 0.2f);
		}
#endif

		ShaderHelper::Get().init();

		Vec2i screenSize = ::Global::GetScreenSize();

		VERIFY_RETURN_FALSE(FMeshBuild::CubeOffset(mCube, 0.5, Vector3(0.5, 0.5, 0.5)));
		VERIFY_RETURN_FALSE(FMeshBuild::CubeLineOffset(mCubeLine, 0.5, Vector3(0.5, 0.5, 0.5)));
		VERIFY_RETURN_FALSE(mObjectBuffer.initializeResource(256, EStructuredBufferType::Buffer));
		VERIFY_RETURN_FALSE(mMaterialBuffer.initializeResource(32, EStructuredBufferType::Buffer));
		VERIFY_RETURN_FALSE(mSceneEnvBuffer.initializeResource(1));
		updateSceneEnvBuffer();

		VERIFY_RETURN_FALSE(mFrameBuffer = RHICreateFrameBuffer());
		RasterizerStateInitializer initializer;
		initializer.fillMode = EFillMode::Wireframe;
		initializer.cullMode = ECullMode::Back;
		initializer.frontFace = EFrontFace::Default;
		initializer.bEnableScissor = false;
		initializer.bEnableMultisample = false;
		const float BiasScale = float((1 << 24) - 1);
		initializer.depthBias = 0.2f / BiasScale;
		initializer.slopeScaleDepthBias = -0.4f;
		mLineOffsetRSState = RHICreateRasterizerState(initializer);

		{
			MaterialData* pMaterial = mMaterialBuffer.lock();
			pMaterial[0].baseColor = Color3f(1, 0, 0);
			pMaterial[0].emissiveColor = Color3f(0, 0, 0);
			pMaterial[0].shininess = 5;
			pMaterial[1].baseColor = Color3f(0.5, 0.2, 0);
			pMaterial[1].emissiveColor = Color3f(0, 0, 0);
			pMaterial[1].shininess = 5;
			pMaterial[2].baseColor = Color3f(1, 0.7, 0.3);
			pMaterial[2].emissiveColor = Color3f(0, 0, 0);
			pMaterial[2].shininess = 5;
			pMaterial[3].baseColor = Color3f(0, 0, 0.1);
			pMaterial[3].emissiveColor = Color3f(10, 10, 0.1);
			pMaterial[3].shininess = 5;
			mMaterialBuffer.unlock();
		}
		{
			VERIFY_RETURN_FALSE(mMapTileBuffer.initializeResource(mMap.getRawDataSize(), EStructuredBufferType::Buffer));
			RegionManager manager(Vec2i(mMap.getSizeX(), mMap.getSizeY()));
			for (int j = 0; j < mMap.getSizeY(); ++j)
			{
				for (int i = 0; i < mMap.getSizeX(); ++i)
				{
					if (mMap.getData(i, j) == TILE_NONE)
					{
						manager.productBlock(Vec2i(i, j), Vec2i(1, 1));
					}
				}
			}
			MapTileData* pData = mMapTileBuffer.lock();
			mNumMapTile = 0;
			std::string code;

			std::vector<uint8> codeTemplate;
			if (!FFileUtility::LoadToBuffer("Shader/Game/SDFSceneTemplate.sgc", codeTemplate, true))
			{
				LogWarning(0, "Can't load SDFSceneTemplate file");
				return false;
			}

			std::string codeBlock;
			for (Region* region : manager.mRegionList)
			{
				Vector2 halfSize = 0.5 * Vector2(region->rect.getSize());
				Vector2 pos = Vector2(region->rect.getMin()) + halfSize;
				pData->posAndSize = Vector4(pos.x, pos.y, halfSize.x, halfSize.y);
				++mNumMapTile;
				++pData;

				InlineString<512> str;
				str.format("\tSDF_BOX(float3( %g , %g , -0.25), float3( %g ,  %g , 0.25));\r\n", pos.x, pos.y, halfSize.x, halfSize.y);
				codeBlock += str;
			}

			for (int j = 0; j < mMap.getSizeY(); ++j)
			{
				for (int i = 0; i < mMap.getSizeX(); ++i)
				{
					if (mMap.getData(i, j) == TILE_BLOCK)
					{
						Vector2 halfSize = 0.5 * Vector2(1,1);
						Vector2 pos = Vector2(i,j) + halfSize;
						pData->posAndSize = Vector4(pos.x, pos.y, halfSize.x, halfSize.y);
						++mNumMapTile;
						++pData;

						InlineString<512> str;
						str.format("\tSDF_BOX(float3( %g , %g , 0.25), float3( %g ,  %g , 0.25));\r\n", pos.x, pos.y, halfSize.x, halfSize.y);
						codeBlock += str;
					}
				}
			}

			Text::Format((char const*)codeTemplate.data(), TArrayView<std::string const>(&codeBlock, 1) , code);
			mMapTileBuffer.unlock();

#if 0
			for (int i = 0; i < mObject.mNumBodies; ++i)
			{
				char const* objectCode =
					"{\n"
					"ObjectData objectData = Objects[%d];\n"
					"float3 localPos = mul(objectData.worldToLocal, float4(pos, 1)).xyz;\n"
					"float objectdist = SDF_RoundBox(localPos.xyz, 0.4 * float3(1, 1, 1), 0.1);\n"
					"data = SDF_Union(data, objectData.objectParams.y, objectdist);\n"
					"}\n";

				InlineString<512> str;
				str.format(objectCode, i);
				code += str;
			}
#endif

			FFileUtility::SaveFromBuffer("Shader/Game/SDFSceneBuiltin.sgc", (uint8* const)code.data(), code.length());
			VERIFY_RETURN_FALSE(mProgRayTraceLighting = ShaderManager::Get().getGlobalShaderT< RayTraceLightingProgram >());
		}


		{

			mSceneRenderTargets.initializeRHI();

			Color4ub colors[] = { Color4ub::Black() , Color4ub::White() , Color4ub::White() , Color4ub::Black() };
			VERIFY_RETURN_FALSE(mGirdTexture = RHICreateTexture2D(ETexture::RGBA8, 2, 2, 0, 1, TCF_DefalutValue, colors));

			VERIFY_RETURN_FALSE(mProgFliter = ShaderManager::Get().getGlobalShaderT< FliterProgram >());
			VERIFY_RETURN_FALSE(mProgFliterAdd = ShaderManager::Get().getGlobalShaderT< FliterAddProgram >());
			VERIFY_RETURN_FALSE(mProgDownsample = ShaderManager::Get().getGlobalShaderT< DownsampleProgram >());
			VERIFY_RETURN_FALSE(mProgBloomSetup = ShaderManager::Get().getGlobalShaderT< BloomSetupProgram >());
			VERIFY_RETURN_FALSE(mProgTonemap = ShaderManager::Get().getGlobalShaderT< TonemapProgram >());

			mBloomFrameBuffer = RHICreateFrameBuffer();
		}


		return true;
	}

	void TestStage::preShutdownRenderSystem(bool bReInit /*= false*/)
	{
		mFrameBuffer.release();

		mCube.releaseRHIResource();
		mCubeLine.releaseRHIResource();
		mView.releaseRHIResource();
		mMapTileBuffer.releaseResource();
		mMaterialBuffer.releaseResource();
		mObjectBuffer.releaseResource();
		mSceneEnvBuffer.releaseResource();

		mSceneRenderTargets.releaseRHI();

		mGirdTexture.release();
		mBloomFrameBuffer.release();

		GRenderTargetPool.releaseRHI();

	}


	void TestStage::updateSceneEnvBuffer()
	{
		mSceneEnvBuffer.updateBuffer(mSceneEnv);
	}

	void TestStage::restart()
	{

	}

	void TestStage::onUpdate(GameTimeSpan deltaTime)
	{
		BaseClass::onUpdate(deltaTime);

		if (mMoveCur == DIR_NONE && canInput())
		{
			if (InputManager::Get().isKeyDown(EKeyCode::A))
				requestMove(DIR_X);
			else if (InputManager::Get().isKeyDown(EKeyCode::D))
				requestMove(DIR_NX);
			else if (InputManager::Get().isKeyDown(EKeyCode::S))
				requestMove(DIR_NY);
			else if (InputManager::Get().isKeyDown(EKeyCode::W))
				requestMove(DIR_Y);
		}
		mCamera.updatePosition(deltaTime);
		mTweener.update(deltaTime);
	}

	void TestStage::onRender(float dFrame)
	{
		RHICommandList& commandList = RHICommandList::GetImmediateList();

		GRenderTargetPool.freeAllUsedElements();

		RHISetFrameBuffer(commandList, nullptr);
		RHIClearRenderTargets(commandList, EClearBits::Color | EClearBits::Depth, &LinearColor(0, 0, 0, 1), 1);
		Vec2i screenSize = ::Global::GetScreenSize();
		RHISetViewport(commandList, 0, 0, screenSize.x, screenSize.y);
		float aspect = float(screenSize.x) / screenSize.y;
		Matrix4 projectionMatrix = PerspectiveMatrix(Math::DegToRad(45), aspect, 0.01, 1000);

		mCamRefPos = Vector3(-3, -7, 12);
		Vector3 posCam = mLookPos + mCamRefPos;
		Matrix4 viewMatrix = LookAtMatrix(posCam, mLookPos - posCam, Vector3(0, 0, 1));
		if (bFreeView)
		{
			viewMatrix = mCamera.getViewMatrix();
		}
		mView.setupTransform(viewMatrix, projectionMatrix);
		mView.rectSize = screenSize;
		mView.rectOffset = IntVector2(0, 0);


		mSceneRenderTargets.prepare(screenSize);
		mSceneRenderTargets.attachDepthTexture();

		RHISetDepthStencilState(commandList, TStaticDepthStencilState<>::GetRHI());
		RHISetBlendState(commandList, TStaticBlendState<>::GetRHI());

		//viewMatrix = LookAtMatrix(Vector3(10,10,10), Vector3(-1,-1,-1), Vector3(0, 0, 1));
		//mView.setupTransform(viewMatrix, projectionMatrix);

		CHECK(mStack.mStack.size() == 0);
		if (bUseRayTrace)
		{
			mStack.setIdentity();
			mObjectList.clear();

			for (int i = 0; i < mMap.getSizeX(); ++i)
			{
				for (int j = 0; j < mMap.getSizeY(); ++j)
				{
					int data = mMap.getData(i, j);
					if (data == TILE_NONE)
						continue;

					if (data == TILE_GROW)
					{
						mStack.push();
						Vec3i posBody = Vec3i(i, j, 0);
						mStack.translate(Vector3(posBody) + Vector3(0.5));
						ObjectData object;
						Matrix4 worldToLocal;
						float det;
						mStack.get().inverse(worldToLocal, det);
						object.worldToLocal = worldToLocal;
						//object.worldToLocal.translate(Vector3(-0.5));
						object.type = 0;
						object.matID = 2;
						object.typeParams = Vector4(1, 1, 1, 0);
						mObjectList.push_back(object);
						mStack.pop();
					}
				}
			}
		}
		else
		{
			RHISetFrameBuffer(commandList, mSceneRenderTargets.getFrameBuffer());
			RHIClearRenderTargets(commandList, EClearBits::Color | EClearBits::Depth, &LinearColor(0.2, 0.2, 0.2, 1), 1);

			mStack.set(viewMatrix * AdjustProjectionMatrixForRHI(projectionMatrix));

			RHISetFixedShaderPipelineState(commandList, mStack.get(), LinearColor(1, 1, 1, 1));
			DrawUtility::AixsLine(commandList, 100);

			auto GetColor = [&](int tileId) -> LinearColor
			{
				switch (tileId)
				{
				case TILE_GOAL: return LinearColor(0.9, mSpot, mSpot, 1);
				}
				return LinearColor(1, 1, 1, 1);
			};


			auto DrawBlock = [&](LinearColor const& color)
			{
				RHISetRasterizerState(commandList, TStaticRasterizerState<ECullMode::Back, EFillMode::Solid >::GetRHI());
				RHISetFixedShaderPipelineState(commandList, mStack.get(), color);
				mCube.draw(commandList);

				RHISetRasterizerState(commandList, *mLineOffsetRSState);
				RHISetFixedShaderPipelineState(commandList, mStack.get(), LinearColor(0, 0, 0, 1));
				mCube.draw(commandList);
			};

			for (int i = 0; i < mMap.getSizeX(); ++i)
			{
				for (int j = 0; j < mMap.getSizeY(); ++j)
				{
					int data = mMap.getData(i, j);
					if (data == TILE_NONE)
						continue;

					float h = 0.5;
					mStack.push();
					mStack.translate(Vector3(i, j, -h));
					mStack.scale(Vector3(1.0f, 1.0f, h));
					DrawBlock(GetColor(data));
					mStack.pop();


					if (data == TILE_BLOCK)
					{
						mStack.push();
						mStack.translate(Vector3(i, j, 0));
						mStack.scale(Vector3(1.0f, 1.0f, h));
						DrawBlock(LinearColor(1, 1, 1, 1));
						mStack.pop();
					}
					else if (data == TILE_GROW)
					{
						mStack.push();
						mStack.translate(Vector3(i, j, 0));
						DrawBlock(LinearColor(0, 1, 1, 1));
						mStack.pop();
					}
				}
			}
		}


		mStack.push();
		{
			Vec3i const& pos = mObject.getPos();
			mStack.translate(Vector3(pos));
			mStack.push();
			{
				Vector3 color = Vector3(1, 1, 0);
				if (mMoveCur != DIR_NONE)
				{
					mStack.translate(mRotatePos);
					mStack.rotate(Quaternion::Rotate(mRotateAxis, Math::DegToRad(mRotateAngle)));
					mStack.translate(-mRotatePos);
				}
				else if (mIsGoal)
				{
					color = Vector3(0.9, mSpot, mSpot);
				}
				drawObjectBody(color);
			}
			mStack.pop();
		}
		mStack.pop();

		if (bUseRayTrace)
		{

			GPU_PROFILE("Ray Trace");
			if (mObjectList.size())
			{
#if 1
				mObjectBuffer.updateBuffer(mObjectList);
#else
				ObjectData* pData = mObjectBuffer.lock();
				memcpy(pData, mObjectList.data(), sizeof(ObjectData) * mObjectList.size());
				mObjectBuffer.unlock();
#endif
			}

			RHITexture2DRef   renderBuffers[2];

			RenderTargetDesc desc;
			desc.size = screenSize;
			desc.format = ETexture::FloatRGBA;
			desc.creationFlags = TCF_CreateSRV;

			renderBuffers[0] = static_cast<RHITexture2D*>(GRenderTargetPool.fetchElement(desc)->texture.get());
			renderBuffers[1] = static_cast<RHITexture2D*>(GRenderTargetPool.fetchElement(desc)->texture.get());
			if (bUseDeferredRending)
			{
				mFrameBuffer->setTexture(0, *renderBuffers[0]);
				mFrameBuffer->setTexture(1, *renderBuffers[1]);
				RHISetFrameBuffer(commandList, mFrameBuffer);
				LinearColor clearColors[2] = { LinearColor(0, 0, 0, 0) , LinearColor(0, 0, 0, 0) };
				RHIClearRenderTargets(commandList, EClearBits::Color, clearColors, 2);
			}
			else
			{
				RHISetFrameBuffer(commandList, mSceneRenderTargets.getFrameBuffer());
				RHIClearRenderTargets(commandList, EClearBits::Color | EClearBits::Depth, &LinearColor(0, 0, 0, 1), 1);
			}


			RayTraceProgram::PermutationDomain permutationVector;
			permutationVector.set<RayTraceProgram::UseBuiltinScene>( bUseDeferredRending ? false : bUseSceneBuitin);
			permutationVector.set<RayTraceProgram::UseDeferredRendering>(bUseDeferredRending);


			RayTraceProgram* progRayTrace = ShaderManager::Get().getGlobalShaderT< RayTraceProgram >(permutationVector);
			RHISetRasterizerState(commandList, TStaticRasterizerState<ECullMode::None >::GetRHI());
			RHISetBlendState(commandList, TStaticBlendState<>::GetRHI());
			RHISetDepthStencilState(commandList, StaticDepthDisableState::GetRHI());

			RHISetShaderProgram(commandList, progRayTrace->getRHI());
			mView.setupShader(commandList, *progRayTrace);
			SET_SHADER_PARAM(commandList, *progRayTrace, ObjectNum, (int)mObjectList.size());
			progRayTrace->setStructuredStorageBufferT< ObjectData >(commandList, *mObjectBuffer.getRHI());
			progRayTrace->setStructuredStorageBufferT< MaterialData >(commandList, *mMaterialBuffer.getRHI());
			if (!bUseSceneBuitin && !bUseDeferredRending)
			{
				SET_SHADER_PARAM(commandList, *progRayTrace, MapTileNum, int(mNumMapTile));
				progRayTrace->setStructuredStorageBufferT< MapTileData >(commandList, *mMapTileBuffer.getRHI());
			}
			if (!bUseDeferredRending)
			{
				progRayTrace->setStructuredUniformBufferT< SceneEnvData >(commandList, *mSceneEnvBuffer.getRHI());
			}
			DrawUtility::ScreenRect(commandList);

			if (bUseDeferredRending)
			{
				RHISetFrameBuffer(commandList, mSceneRenderTargets.getFrameBuffer());
				RHISetShaderProgram(commandList, mProgRayTraceLighting->getRHI());
				mView.setupShader(commandList, *mProgRayTraceLighting);
				mProgRayTraceLighting->setRenderTargets(commandList, renderBuffers);
				//SET_SHADER_PARAM(commandList, *mProgRayTraceLighting, ObjectNum, (int)mObjectList.size());
				mProgRayTraceLighting->setParam(commandList, SHADER_PARAM(ObjectNum), (int)mObjectList.size());
				mProgRayTraceLighting->setStructuredStorageBufferT< ObjectData >(commandList, *mObjectBuffer.getRHI());
				mProgRayTraceLighting->setStructuredStorageBufferT< MaterialData >(commandList, *mMaterialBuffer.getRHI());
				mProgRayTraceLighting->setStructuredUniformBufferT< SceneEnvData >(commandList, *mSceneEnvBuffer.getRHI());
				DrawUtility::ScreenRect(commandList);

				mProgRayTraceLighting->clearRenderTargets(commandList);
			}
		}

		RHITexture2DRef postProcessRT;
		if (bUseBloom)
		{
			GPU_PROFILE("Bloom");

			RHISetRasterizerState(commandList, TStaticRasterizerState<ECullMode::None >::GetRHI());
			RHISetBlendState(commandList, TStaticBlendState<>::GetRHI());
			RHISetDepthStencilState(commandList, StaticDepthDisableState::GetRHI());

			RHITexture2DRef downsampleTextures[6];

			{
				GPU_PROFILE("Downsample");

				auto DownsamplePass = [this](RHICommandList& commandList, int index, RHITexture2D& sourceTexture) -> RHITexture2DRef
				{
					RenderTargetDesc desc;
					InlineString<128> str;
					str.format("Downsample(%d)", index);

					Vec2i size;
					size.x = (sourceTexture.getSizeX() + 1) / 2;
					size.y = (sourceTexture.getSizeY() + 1) / 2;
					desc.debugName = str;
					desc.size = size;
					desc.format = ETexture::FloatRGBA;
					desc.creationFlags = TCF_CreateSRV;
					RHITexture2DRef downsampleTexture = static_cast<RHITexture2D*>(GRenderTargetPool.fetchElement(desc)->texture.get());
					mBloomFrameBuffer->setTexture(0, *downsampleTexture);
					RHISetViewport(commandList, 0, 0, size.x, size.y);
					RHISetFrameBuffer(commandList, mBloomFrameBuffer);
					RHISetShaderProgram(commandList, mProgDownsample->getRHI());
					auto& sampler = TStaticSamplerState<ESampler::Bilinear, ESampler::Clamp, ESampler::Clamp >::GetRHI();
					SET_SHADER_TEXTURE_AND_SAMPLER(commandList, *mProgDownsample, Texture, sourceTexture, sampler);
					SET_SHADER_PARAM(commandList, *mProgDownsample, ExtentInverse, Vector2(1 / float(size.x), 1 / float(size.y)));

					DrawUtility::ScreenRect(commandList);
					return downsampleTexture;
				};

				RHITexture2DRef halfSceneTexture;
				{
					GPU_PROFILE("Downsample(0)");
					halfSceneTexture = DownsamplePass(commandList, 0, mSceneRenderTargets.getFrameTexture());
				}

				{
					GPU_PROFILE("BloomSetup");

					RenderTargetDesc desc;
					Vec2i size = Vec2i(halfSceneTexture->getSizeX(), halfSceneTexture->getSizeY());
					desc.debugName = "BloomSetup";
					desc.size = size;
					desc.format = ETexture::FloatRGBA;
					desc.creationFlags = TCF_CreateSRV;

					PooledRenderTargetRef bloomSetupRT = GRenderTargetPool.fetchElement(desc);

					mBloomFrameBuffer->setTexture(0, *bloomSetupRT->texture);
					RHISetViewport(commandList, 0, 0, size.x, size.y);
					RHISetFrameBuffer(commandList, mBloomFrameBuffer);
					RHISetShaderProgram(commandList, mProgBloomSetup->getRHI());

					auto& sampler = TStaticSamplerState<ESampler::Bilinear, ESampler::Clamp, ESampler::Clamp >::GetRHI();
					SET_SHADER_TEXTURE_AND_SAMPLER(commandList, *mProgBloomSetup, Texture, *halfSceneTexture, sampler);
					SET_SHADER_PARAM(commandList, *mProgBloomSetup, BloomThreshold, mBloomThreshold);

					DrawUtility::ScreenRect(commandList);

					downsampleTextures[0] = static_cast<RHITexture2D*>(bloomSetupRT->texture.get());
				}

				{

					for (int i = 1; i < ARRAY_SIZE(downsampleTextures); ++i)
					{
						GPU_PROFILE("Downsample(%d)", i);
						downsampleTextures[i] = DownsamplePass(commandList, i, *downsampleTextures[i - 1]);
					}
				}
			}

			{
				auto BlurPass = [this](RHICommandList& commandList, int index, RHITexture2D& fliterTexture, RHITexture2D& bloomTexture, LinearColor const& tint, float blurSize)
				{
					int numSamples = 0;
					float bloomRadius = blurSize * 0.01 * 0.5 * fliterTexture.getSizeX() * blurRadiusScale;

					auto& sampler = TStaticSamplerState<ESampler::Bilinear, ESampler::Clamp, ESampler::Clamp >::GetRHI();

					IntVector2 sizeH = IntVector2(fliterTexture.getSizeX(), fliterTexture.getSizeY()); ;

					InlineString<128> str;
					RenderTargetDesc desc;
					str.format("BlurH(%d)", index);
					desc.debugName = str;
					desc.size = sizeH;
					desc.format = ETexture::FloatRGBA;
					desc.creationFlags = TCF_CreateSRV;
					PooledRenderTargetRef blurXRT = GRenderTargetPool.fetchElement(desc);
					mBloomFrameBuffer->setTexture(0, *blurXRT->texture);
					{
						GPU_PROFILE(str);
						RHISetFrameBuffer(commandList, mBloomFrameBuffer);
						RHISetViewport(commandList, 0, 0, sizeH.x , sizeH.y);
						RHISetShaderProgram(commandList, mProgFliter->getRHI());
						SET_SHADER_TEXTURE_AND_SAMPLER(commandList, *mProgFliter, FliterTexture, fliterTexture, sampler);
						numSamples = generateFliterData(fliterTexture.getSizeX(), Vector2(1, 0), LinearColor(1,1,1,1), bloomRadius);
						mProgFliter->setParam(commandList, SHADER_PARAM(Weights), mWeightData.data(), MaxWeightNum);
						mProgFliter->setParam(commandList, SHADER_PARAM(UVOffsets), (Vector4*)mUVOffsetData.data(), MaxWeightNum / 2);
						mProgFliter->setParam(commandList, SHADER_PARAM(WeightNum), numSamples);
						DrawUtility::ScreenRect(commandList);

					}

					IntVector2 sizeV = IntVector2(fliterTexture.getSizeX(), fliterTexture.getSizeY()); ;

					str.format("BlurV(%d)", index);
					desc.debugName = str;
					PooledRenderTargetRef blurYRT = GRenderTargetPool.fetchElement(desc);
					mBloomFrameBuffer->setTexture(0, *blurYRT->texture);
					{
						GPU_PROFILE(str);
						RHISetFrameBuffer(commandList, mBloomFrameBuffer);
						RHISetViewport(commandList, 0, 0, sizeV.x, sizeV.y);
						RHISetShaderProgram(commandList, mProgFliterAdd->getRHI());

						SET_SHADER_TEXTURE_AND_SAMPLER(commandList, *mProgFliterAdd, FliterTexture, *blurXRT->texture, sampler);
						SET_SHADER_TEXTURE_AND_SAMPLER(commandList, *mProgFliterAdd, AddTexture, bloomTexture, sampler);
						numSamples = generateFliterData(fliterTexture.getSizeY(), Vector2(0, 1), tint, bloomRadius);
						mProgFliterAdd->setParam(commandList, SHADER_PARAM(Weights), mWeightData.data(), MaxWeightNum);
						mProgFliterAdd->setParam(commandList, SHADER_PARAM(UVOffsets), (Vector4*)mUVOffsetData.data(), MaxWeightNum / 2);
						mProgFliterAdd->setParam(commandList, SHADER_PARAM(WeightNum), numSamples);
						DrawUtility::ScreenRect(commandList);
					}

					return static_cast<RHITexture2D*>(blurYRT->texture.get());
				};

				struct BlurInfo
				{
					LinearColor tint;
					float size;
				};

				BlurInfo blurInfos[] =
				{
					{ LinearColor(0.3465f, 0.3465f, 0.3465f), 0.3f },
					{ LinearColor(0.138f, 0.138f, 0.138f), 1.0f },
					{ LinearColor(0.1176f, 0.1176f, 0.1176f), 2.0f },
					{ LinearColor(0.066f, 0.066f, 0.066f), 10.0f },
					{ LinearColor(0.066f, 0.066f, 0.066f), 30.0f },
					{ LinearColor(0.061f, 0.061f, 0.061f), 64.0f },
				};
				RHITexture2DRef bloomTexture = GBlackTexture2D;
				{
					GPU_PROFILE("Blur");
					float tintScale = mBloomIntensity / float(ARRAY_SIZE(downsampleTextures));

					
					for (int i = 0; i < ARRAY_SIZE(downsampleTextures); ++i)
					{
						int index = ARRAY_SIZE(downsampleTextures) - 1 - i;
						RHITexture2DRef fliterTexture = downsampleTextures[index];
						bloomTexture = BlurPass(commandList, i, *fliterTexture, *bloomTexture, Vector4(blurInfos[index].tint) * tintScale, blurInfos[index].size);
					}
				}

				{
					GPU_PROFILE("Tonemap");
					mSceneRenderTargets.swapFrameTexture();

					RHISetFrameBuffer(commandList, mSceneRenderTargets.getFrameBuffer());
					RHISetViewport(commandList, 0, 0, screenSize.x, screenSize.y);
					RHISetShaderProgram(commandList, mProgTonemap->getRHI());

					auto& sampler = TStaticSamplerState<ESampler::Bilinear, ESampler::Clamp, ESampler::Clamp >::GetRHI();
					SET_SHADER_TEXTURE_AND_SAMPLER(commandList, *mProgTonemap, BloomTexture, *bloomTexture, sampler);
					SET_SHADER_TEXTURE_AND_SAMPLER(commandList, *mProgTonemap, TextureInput0, mSceneRenderTargets.getPrevFrameTexture(), sampler);
					DrawUtility::ScreenRect(commandList);

				}

				postProcessRT = &mSceneRenderTargets.getFrameTexture();
			}

		}
		else
		{
			postProcessRT = &mSceneRenderTargets.getFrameTexture();
		}

		{
			GPU_PROFILE("CopyToBackBuffer");
			RHISetFrameBuffer(commandList, nullptr);
			RHISetViewport(commandList, 0, 0, screenSize.x , screenSize.y);
			RHISetDepthStencilState(commandList, StaticDepthDisableState::GetRHI());
			RHISetRasterizerState(commandList, TStaticRasterizerState<ECullMode::None >::GetRHI());

			ShaderHelper::Get().copyTextureToBuffer(commandList, *postProcessRT);
			//ShaderHelper::Get().copyTextureToBuffer(commandList, mSceneRenderTargets.getFrameTexture());
		}

		updateRenderTargetShow();

		RHISetFixedShaderPipelineState(commandList, AdjustProjectionMatrixForRHI(mView.worldToClip));
		//DrawUtility::AixsLine(commandList);

		RHIGraphics2D& g = ::Global::GetRHIGraphics2D();
	}

	void TestStage::drawObjectBody(Vector3 const& color)
	{
		RHICommandList& commandList = RHICommandList::GetImmediateList();
		for (int i = 0; i < mObject.mNumBodies; ++i)
		{
			Vec3i const& posBody = mObject.mBodiesPos[i];
			mStack.push();

			if (bUseRayTrace)
			{
				mStack.translate(Vector3(posBody) + Vector3(0.5));
				ObjectData object;
				Matrix4 worldToLocal;
				float det;
				mStack.get().inverse(worldToLocal, det);
				object.worldToLocal = worldToLocal;
				//object.worldToLocal.translate(Vector3(-0.5));
				object.type = 0;
				object.matID = 3;
				object.typeParams = Vector4(1, 1, 1, 0);
				mObjectList.push_back(object);
			}
			else
			{
				mStack.translate(Vector3(posBody));
				RHISetRasterizerState(commandList, TStaticRasterizerState<ECullMode::Back, EFillMode::Solid >::GetRHI());
				RHISetFixedShaderPipelineState(commandList, mStack.get(), LinearColor(color.x, color.y, color.z, 1));
				mCube.draw(commandList);

				RHISetRasterizerState(commandList, *mLineOffsetRSState);
				RHISetFixedShaderPipelineState(commandList, mStack.get(), LinearColor(0, 0, 0, 1));
				mCube.draw(commandList);
			}

			mStack.pop();
		}
	}

	void TestStage::requestMove(Dir dir)
	{
		if (mMoveCur != DIR_NONE)
			return;

		Object testObj = Object(mObject);
		testObj.move(dir);
		uint8 holeDirMask;
		State state = checkObjectState(testObj, holeDirMask);
		switch (state)
		{
		case State::eFall: return;
		case State::eGoal: mIsGoal = true; break;
		case State::eMove: mIsGoal = false; break;
		case State::eBlock: return;
		}

		mMoveCur = dir;
		switch (dir)
		{
		case DIR_X:
			mRotateAxis = Vector3(0, 1, 0);
			mRotatePos = Vector3(1 + mObject.getMaxLocalPosX(), 0, 0);
			break;
		case DIR_NX:
			mRotateAxis = Vector3(0, -1, 0);
			mRotatePos = Vector3(0, 0, 0);
			break;
		case DIR_Y:
			mRotateAxis = Vector3(-1, 0, 0);
			mRotatePos = Vector3(0, 1 + mObject.getMaxLocalPosY(), 0);
			break;
		case DIR_NY:
			mRotateAxis = Vector3(1, 0, 0);
			mRotatePos = Vector3(0, 0, 0);
			break;
		}
		float time = 0.4f;
		mRotateAngle = 0;
		mTweener.tweenValue< Easing::OQuad >(mRotateAngle, 0.0f, 90.0f, time).finishCallback(std::bind(&TestStage::moveObject, this));
		Vector3 to;
		to.x = testObj.getPos().x + 1;
		to.y = testObj.getPos().y + 1;
		to.z = 0;
		mTweener.tweenValue< Easing::OQuad >(mLookPos, mLookPos, to, time);
	}

	void TestStage::moveObject()
	{
		mObject.move(mMoveCur);
		mMoveCur = DIR_NONE;

		auto CheckGrow = [&](Vec2i const& tilePos)
		{
			if (!mMap.checkRange(tilePos))
				return;

			int& data = mMap.getData(tilePos.x, tilePos.y);
			if (data == TILE_GROW)
			{
				mObject.growBody(Vec3i(tilePos.x, tilePos.y, 0));
				data = TILE_NORAML;
			}
		};

		Vec3i const& posObj = mObject.getPos();
		for (int i = 0; i < mObject.MaxBodyNum; ++i)
		{
			Vec3i const& posBody = posObj + mObject.getBodyLocalPos(i);
			if (posBody.z == 0)
			{
				static constexpr Vec2i DirOffset[] = { Vec2i(1,0), Vec2i(-1, 0) , Vec2i(0, 1), Vec2i(0, -1) };
				for (int n = 0; n < 4; ++n)
				{
					Vec2i tilePos = Vec2i(posBody.x, posBody.y) + DirOffset[n];
					CheckGrow(tilePos);
				}
			}
			else if (posBody.z == 1)
			{
				Vec2i tilePos = Vec2i(posBody.x, posBody.y);
				CheckGrow(tilePos);
			}
		}
	}

	TestStage::State TestStage::checkObjectState(Object const& object, uint8& holeDirMask)
	{
		CHECK(Object::MaxBodyNum >= object.getBodyNum());

		Vec2i holdPos[Object::MaxBodyNum];
		int numHold = 0;
		Vec3i const& posObj = object.getPos();
		Vec2f posCOM = Vec2f::Zero();

		bool isGoal = true;
		for (int i = 0; i < object.getBodyNum(); ++i)
		{
			Vec3i const& posBody = posObj + object.getBodyLocalPos(i);

			posCOM += Vec2f(posBody.x, posBody.y) + Vec2f(0.5f, 0.5f);

			if (posBody.z == 0 && mMap.checkRange(posBody.x, posBody.y))
			{
				int data = mMap.getData(posBody.x, posBody.y);
				if (data)
				{

					if (data == TILE_BLOCK || data == TILE_GROW )
						return State::eBlock;

					holdPos[numHold].x = posBody.x;
					holdPos[numHold].y = posBody.y;
					++numHold;

					if (data != 2)
					{
						isGoal = false;
					}
				}
			}
		}

		posCOM /= float(object.getBodyNum());
		holeDirMask = (BIT(DIR_X) | BIT(DIR_Y) | BIT(DIR_NX) | BIT(DIR_NY));
		for (int i = 0; i < numHold; ++i)
		{
			Vec2i const& pos = holdPos[i];

			if (posCOM.x < pos.x)
				holeDirMask &= ~(BIT(DIR_X));
			else if (posCOM.x > pos.x + 1)
				holeDirMask &= ~(BIT(DIR_NX));
			else
			{
				holeDirMask &= ~(BIT(DIR_X) | BIT(DIR_NX));
			}

			if (posCOM.y < pos.y)
				holeDirMask &= ~(BIT(DIR_Y));
			else if (posCOM.y > pos.y + 1)
				holeDirMask &= ~(BIT(DIR_NY));
			else
			{
				holeDirMask &= ~(BIT(DIR_Y) | BIT(DIR_NY));
			}
		}

		if (holeDirMask != 0)
			return State::eFall;

		if (!isGoal)
			return State::eMove;

		return State::eGoal;
	}

	MsgReply TestStage::onKey(KeyMsg const& msg)
	{
		if (!canInput())
		{
			float baseImpulse = 500;
			switch (msg.getCode())
			{
			case EKeyCode::W: mCamera.moveForwardImpulse = msg.isDown() ? baseImpulse : 0; break;
			case EKeyCode::S: mCamera.moveForwardImpulse = msg.isDown() ? -baseImpulse : 0; break;
			case EKeyCode::D: mCamera.moveRightImpulse = msg.isDown() ? baseImpulse : 0; break;
			case EKeyCode::A: mCamera.moveRightImpulse = msg.isDown() ? -baseImpulse : 0; break;
			case EKeyCode::Z: mCamera.moveUp(0.5); break;
			case EKeyCode::X: mCamera.moveUp(-0.5); break;
			}
		}

		if (msg.isDown())
		{
			switch (msg.getCode())
			{
			case EKeyCode::R: restart(); break;
			}
		}

		return BaseClass::onKey(msg);
	}

	MsgReply TestStage::onMouse(MouseMsg const& msg)
	{
		if (bFreeView)
		{
			static Vec2i oldPos = msg.getPos();
			if (msg.onLeftDown())
			{
				oldPos = msg.getPos();
			}
			if (msg.onMoving() && msg.isLeftDown())
			{
				float rotateSpeed = 0.01;
				Vector2 off = rotateSpeed * Vector2(msg.getPos() - oldPos);
				mCamera.rotateByMouse(off.x, off.y);
				oldPos = msg.getPos();
			}
		}

		return BaseClass::onMouse(msg);
	}

}//namespace Bloxorz


REGISTER_STAGE_ENTRY("Bloxorz Test", Bloxorz::TestStage, EExecGroup::Dev, 2);