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
#include "FileSystem.h"
#include "RHI/OpenGLCommon.h"

namespace Bloxorz
{
	using namespace Render;

	Object::Object() 
		:mPos(0,0,0)
	{
		mNumBodies = 1;
		mBodiesPos[0] = Vec3i(0,0,0);
	}

	Object::Object(Object const& rhs) 
		:mPos( rhs.mPos )
		,mNumBodies( rhs.mNumBodies )
	{
		std::copy( rhs.mBodiesPos , rhs.mBodiesPos + mNumBodies , mBodiesPos );
	}

	void Object::addBody(Vec3i const& pos)
	{
		assert( mNumBodies < MaxBodyNum );
		assert( pos.x >= 0 && pos.y >=0 && pos.z >= 0 );
		mBodiesPos[ mNumBodies ] = pos;
		++mNumBodies;
	}

	void Object::move(Dir dir)
	{
		uint32 idxAxis = dir & 1;

		if ( isNegative( dir ) )
		{
			int maxOffset = 0;
			for( int i = 0 ; i < mNumBodies ; ++i )
			{
				Vec3i& bodyPos = mBodiesPos[i];
				assert( bodyPos.x >= 0 && bodyPos.y >=0 && bodyPos.z >= 0 );
				int offset = bodyPos.z;

				bodyPos.z        = bodyPos[ idxAxis ];
				bodyPos[idxAxis] = -offset;

				if ( maxOffset < offset )
					maxOffset = offset;
			}

			for( int i = 0 ; i < mNumBodies ; ++i )
			{
				Vec3i& bodyPos = mBodiesPos[i];
				bodyPos[idxAxis] += maxOffset;
			}

			mPos[idxAxis] -=  maxOffset + 1;
		}
		else
		{
			int maxOffset = 0;
			for( int i = 0 ; i < mNumBodies ; ++i )
			{
				Vec3i& bodyPos = mBodiesPos[i];
				assert( bodyPos.x >= 0 && bodyPos.y >=0 && bodyPos.z >= 0 );
				int offset = bodyPos[ idxAxis ];

				bodyPos[idxAxis] = bodyPos.z;
				bodyPos.z        = -offset;

				if ( maxOffset < offset )
					maxOffset = offset;
			}

			for( int i = 0 ; i < mNumBodies ; ++i )
			{
				Vec3i& bodyPos = mBodiesPos[i];
				bodyPos.z += maxOffset;
			}

			mPos[idxAxis] +=  maxOffset + 1;
		}
	}

	int Object::getMaxLocalPosX() const
	{
		int result = 0;
		for( int i = 0 ; i < mNumBodies ; ++i )
		{
			Vec3i const& bodyPos = mBodiesPos[i];
			if ( result < bodyPos.x )
				result = bodyPos.x;
		}
		return result;
	}

	int Object::getMaxLocalPosY() const
	{
		int result = 0;
		for( int i = 0 ; i < mNumBodies ; ++i )
		{
			Vec3i const& bodyPos = mBodiesPos[i];
			if ( result < bodyPos.y )
				result = bodyPos.y;
		}
		return result;
	}

	int Object::getMaxLocalPosZ() const
	{
		int result = 0;
		for( int i = 0 ; i < mNumBodies ; ++i )
		{
			Vec3i const& bodyPos = mBodiesPos[i];
			if ( result < bodyPos.z )
				result = bodyPos.z;
		}
		return result;
	}

	enum TileID
	{
		TILE_GOAL ,
	};

#define G 2

	int map[] = 
	{
		1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 
		1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0,
		1, 1, 1, 0, 0, 0, 1, 1, 1, 1, 0, 0,
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

	class RayTraceProgram : public GlobalShaderProgram
	{
		DECLARE_SHADER_PROGRAM(RayTraceProgram, Global);
	public:

		static char const* GetShaderFileName()
		{
			return "Shader/Game/RayTraceSDF";
		}

		static TArrayView< ShaderEntryInfo const > GetShaderEntries()
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

	template< bool bUseBuiltinScene , bool bDeferredRendering >
	class TRayTraceProgram : public RayTraceProgram
	{
		DECLARE_SHADER_PROGRAM(TRayTraceProgram, Global);
	public:
		static void SetupShaderCompileOption(ShaderCompileOption& option)
		{
			option.addDefine(SHADER_PARAM(USE_BUILTIN_SCENE), bUseBuiltinScene);
			option.addDefine(SHADER_PARAM(USE_DEFERRED_RENDERING), bDeferredRendering);
		}
	};

	IMPLEMENT_SHADER_PROGRAM_T(template<>, COMMA_SEPARATED(TRayTraceProgram< false, false>));
	IMPLEMENT_SHADER_PROGRAM_T(template<>, COMMA_SEPARATED(TRayTraceProgram< true, false>));
	IMPLEMENT_SHADER_PROGRAM_T(template<>, COMMA_SEPARATED(TRayTraceProgram< true, true>));


	class RayTraceLightingProgram : public RayTraceProgram
	{
		DECLARE_SHADER_PROGRAM(RayTraceLightingProgram, Global);
		using BassClass = RayTraceProgram;
	public:

		static char const* GetShaderFileName()
		{
			return "Shader/Game/RayTraceSDF";
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
			BIND_SHADER_PARAM(parameterMap, RenderTargetA);
			BIND_SHADER_PARAM(parameterMap, RenderTargetASampler);
			BIND_SHADER_PARAM(parameterMap, RenderTargetB);
			BIND_SHADER_PARAM(parameterMap, RenderTargetBSampler);
		}

		void setRenderTargets(RHICommandList& commandList, RHITexture2DRef textures[])
		{
			setTexture(commandList, mParamRenderTargetA, *textures[0],
				mParamRenderTargetASampler, TStaticSamplerState<>::GetRHI());
			setTexture(commandList, mParamRenderTargetB, *textures[1],
				mParamRenderTargetBSampler, TStaticSamplerState<>::GetRHI());
		}
		void clearRenderTargets(RHICommandList& commandList)
		{
			clearTexture(commandList, mParamRenderTargetA);
			clearTexture(commandList, mParamRenderTargetB);
		}

		DEFINE_SHADER_PARAM(RenderTargetA);
		DEFINE_SHADER_PARAM(RenderTargetASampler);
		DEFINE_SHADER_PARAM(RenderTargetB);
		DEFINE_SHADER_PARAM(RenderTargetBSampler);
	};
	IMPLEMENT_SHADER_PROGRAM(RayTraceLightingProgram);

	bool TestStage::onInit()
	{
		::Global::GUI().cleanupWidget();
		
		Vec2i screenSize = ::Global::GetScreenSize();

		mTweener.tweenValue< Easing::CLinear >( mSpot , 0.0f , 0.5f , 1.0f ).cycle();

		mObject.addBody( Vec3i(0,0,1) );
		mObject.addBody( Vec3i(0,0,2) );
		mObject.addBody( Vec3i(0,1,0) );
		mObject.addBody( Vec3i(0,1,1) );
		mObject.addBody( Vec3i(0,1,2) );

		mMap.resize( 12 , 20 );
		for( int i = 0 ; i < 12 * 20 ; ++i )
			mMap[i] = map[i];

		mLookPos.x = mObject.getPos().x + 1;
		mLookPos.y = mObject.getPos().y + 1;
		mLookPos.z = 0;

		mRotateAngle = 0;
		mMoveCur = DIR_NONE;
		mIsGoal = false;

		mCamera.lookAt(Vector3(10, 10, 10), Vector3(0, 0, 0), Vector3(0, 0, 1));

		DevFrame* frame = WidgetUtility::CreateDevFrame();
		FWidgetPropery::Bind(frame->addCheckBox(UI_ANY, "bUseRayTrace") , bUseRayTrace);
		FWidgetPropery::Bind(frame->addCheckBox(UI_ANY, "bUseSceneButin"), bUseSceneBuitin);
		FWidgetPropery::Bind(frame->addCheckBox(UI_ANY, "bUseDeferredRending"), bUseDeferredRending);
		FWidgetPropery::Bind(frame->addCheckBox(UI_ANY, "bFreeView"), bFreeView);
		FWidgetPropery::Bind(frame->addCheckBox(UI_ANY, "bMoveCamera"), bMoveCamera);
		restart();
		return true;
	}

	void TestStage::onEnd()
	{

	}

	void TestStage::configRenderSystem(ERenderSystem systenName, RenderSystemConfigs& systemConfigs)
	{
#if 1
		systemConfigs.screenWidth = 1080;
		systemConfigs.screenHeight = 720;
#else
		systemConfigs.screenWidth = 800;
		systemConfigs.screenHeight = 600;
#endif
	}


	bool TestStage::setupRenderSystem(ERenderSystem systemName)
{
		if (GRHISystem->getName() == RHISytemName::OpenGL)
		{
			glEnable(GL_POLYGON_OFFSET_LINE);
			glPolygonOffset(-0.4f, 0.2f);
		}


		Vec2i screenSize = ::Global::GetScreenSize();

		VERIFY_RETURN_FALSE(MeshBuild::CubeOffset(mCube, 0.5, Vector3(0.5, 0.5, 0.5)));
		VERIFY_RETURN_FALSE(mObjectBuffer.initializeResource(256, EStructuredBufferType::Buffer ));
		VERIFY_RETURN_FALSE(mMaterialBuffer.initializeResource(32, EStructuredBufferType::Buffer ));

		VERIFY_RETURN_FALSE(mFrameBuffer = RHICreateFrameBuffer());
		for (int i = 0; i < ARRAY_SIZE(mRenderBuffers); ++i)
		{
			VERIFY_RETURN_FALSE(mRenderBuffers[i] = RHICreateTexture2D(
				(i == 0 ) ? Texture::eRGBA32F : Texture::eFloatRGBA, screenSize.x, screenSize.y, 0, 1, 
				TCF_CreateSRV | TCF_RenderTarget));

			mFrameBuffer->addTexture(*mRenderBuffers[i]);
		}

		{
			MaterialData* pMaterial = mMaterialBuffer.lock();
			pMaterial[0].color = Color3f(1, 0, 0);
			pMaterial[0].shininess = 5;
			pMaterial[1].color = Color3f(0.5, 0.2, 0);
			pMaterial[1].shininess = 5;
			pMaterial[2].color = Color3f(1, 0.7, 0.3);
			pMaterial[2].shininess = 5;
			mMaterialBuffer.unlock();
		}
		{
			VERIFY_RETURN_FALSE(mMapTileBuffer.initializeResource(mMap.getRawDataSize(), EStructuredBufferType::Buffer));
			RegionManager manager( Vec2i( mMap.getSizeX() , mMap.getSizeY() ) );
			for (int j = 0; j < mMap.getSizeY(); ++j)
			{
				for (int i = 0; i < mMap.getSizeX(); ++i)
				{
					if (mMap.getData(i, j) != 1)
					{
						manager.productBlock(Vec2i(i, j), Vec2i(1, 1));
					}

				}
			}
			MapTileData* pData = mMapTileBuffer.lock();
			mNumMapTile = 0;
			std::string code;

			code += "SDFSceneOut SDFSceneBuiltin(float3 pos)\n";
			code += "{\n";
			code += "\tSDFSceneOut data; data.dist = 1e10; data.id = 0;\n";
			for (Region* region : manager.mRegionList)
			{
				Vector2 halfSize = 0.5 * Vector2(region->rect.getSize());
				Vector2 pos = Vector2( region->rect.getMin() ) + halfSize;
				pData->posAndSize = Vector4(pos.x, pos.y, halfSize.x, halfSize.y);
				++mNumMapTile;
				++pData;

				FixString<512> str;
				str.format("\tdata = SDF_Union(data, 0 , SDF_Box(pos - float3( %g , %g , -0.25), float3( %g ,  %g , 0.25)));\n", pos.x, pos.y, halfSize.x, halfSize.y);
				code += str;
			}

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

				FixString<512> str;
				str.format(objectCode, i);
				code += str;
			}
#endif

			code += "\treturn data;\n";
			code += "}\n";

			FileUtility::SaveFromBuffer("Shader/Game/SDFSceneBuiltin.sgc", code.data(), code.length());
			VERIFY_RETURN_FALSE(mProgRayTrace = ShaderManager::Get().getGlobalShaderT< COMMA_SEPARATED(TRayTraceProgram<false, false>) >());
			VERIFY_RETURN_FALSE(mProgRayTraceBuiltin = ShaderManager::Get().getGlobalShaderT< COMMA_SEPARATED(TRayTraceProgram<true,false>)>());
			VERIFY_RETURN_FALSE(mProgRayTraceDeferred = ShaderManager::Get().getGlobalShaderT<COMMA_SEPARATED(TRayTraceProgram<true, true>)>());
			VERIFY_RETURN_FALSE(mProgRayTraceLighting = ShaderManager::Get().getGlobalShaderT< RayTraceLightingProgram >());

		}


		return true;
	}

	void TestStage::preShutdownRenderSystem(bool bReInit /*= false*/)
	{
		mProgRayTrace = nullptr;
		mProgRayTraceBuiltin = nullptr;
		mProgRayTraceDeferred = nullptr;

		mFrameBuffer.release();
		for (int i = 0; i < ARRAY_SIZE(mRenderBuffers); ++i)
		{
			mRenderBuffers[i].release();
		}
		
		mCube.releaseRHIResource();
		mView.releaseRHIResource();
		mMapTileBuffer.releaseResources();
		mMaterialBuffer.releaseResources();
		mObjectBuffer.releaseResources();
	}


	void TestStage::restart()
	{

	}

	void TestStage::onUpdate(long time)
	{
		BaseClass::onUpdate( time );

		int frame = time / gDefaultTickTime;
		for( int i = 0 ; i < frame ; ++i )
			tick();

		updateFrame( frame );
	}

	void TestStage::updateFrame(int frame)
	{
		mTweener.update( float( frame * gDefaultTickTime ) / 1000.0f );
	}

	void TestStage::tick()
	{
		if ( mMoveCur == DIR_NONE && canInput() )
		{
			if ( InputManager::Get().isKeyDown( EKeyCode::A ) )
				requestMove( DIR_NX );
			else if ( InputManager::Get().isKeyDown( EKeyCode::D ) )
				requestMove( DIR_X ); 
			else if ( InputManager::Get().isKeyDown( EKeyCode::S ) )
				requestMove( DIR_NY );
			else if ( InputManager::Get().isKeyDown( EKeyCode::W ) )
				requestMove( DIR_Y );
		}
	}

	void TestStage::onRender(float dFrame)
	{
		RHICommandList& commandList = RHICommandList::GetImmediateList();
		RHISetFrameBuffer(commandList, nullptr);
		RHIClearRenderTargets(commandList, EClearBits::Color | EClearBits::Depth, &LinearColor(0,0,0,1), 1);	
		Vec2i screenSize = ::Global::GetScreenSize();
		RHISetViewport(commandList, 0, 0, screenSize.x, screenSize.y);
		float aspect = float(screenSize.x) / screenSize.y;
		Matrix4 projectionMatrix = PerspectiveMatrix(Math::Deg2Rad(45) , aspect, 0.01, 1000);

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


		RHISetDepthStencilState(commandList, TStaticDepthStencilState<>::GetRHI());
		RHISetBlendState(commandList, TStaticBlendState<>::GetRHI());

		//viewMatrix = LookAtMatrix(Vector3(10,10,10), Vector3(-1,-1,-1), Vector3(0, 0, 1));
		mView.setupTransform(viewMatrix, projectionMatrix);
		if (bUseRayTrace)
		{
			mStack.setIdentity();
			mObjectList.clear();
		}
		else
		{
			mStack.set(AdjProjectionMatrixForRHI(projectionMatrix));
			mStack.transform(viewMatrix);
		}

		if (!bUseRayTrace)
		{
			for (int i = 0; i < mMap.getSizeX(); ++i)
			{
				for (int j = 0; j < mMap.getSizeY(); ++j)
				{
					int data = mMap.getData(i, j);
					if (data == 0)
						continue;

					float h = 0.5;
					mStack.push();
					mStack.translate(Vector3(i, j, -h));

					mStack.scale(Vector3(1.0f, 1.0f, h));

					RHISetRasterizerState(commandList, TStaticRasterizerState<ECullMode::Back, EFillMode::Solid >::GetRHI());
					RHISetFixedShaderPipelineState(commandList, mStack.get(), (data == 1) ? LinearColor(1, 1, 1, 1) : LinearColor(0.9, mSpot, mSpot, 1));
					mCube.draw(commandList);

					RHISetRasterizerState(commandList, TStaticRasterizerState<ECullMode::Back, EFillMode::Wireframe >::GetRHI());
					RHISetFixedShaderPipelineState(commandList, mStack.get(), LinearColor(0, 0, 0, 1));
					mCube.draw(commandList);

					mStack.pop();
				}
			}
		}

		mStack.push();
		{
			Vec3i const& pos = mObject.getPos();
			mStack.translate(Vector3(pos));
			mStack.push();
			{
				Vector3 color = Vector3( 1, 1 , 0 );
				if ( mMoveCur != DIR_NONE )
				{
					mStack.translate(mRotatePos);
					mStack.rotate(Quaternion::Rotate(mRotateAxis, Math::Deg2Rad(mRotateAngle)));
					mStack.translate(-mRotatePos);
				}
				else if ( mIsGoal )
				{
					color = Vector3( 0.9 , mSpot , mSpot );
				}
				drawObjectBody( color );
			}
			mStack.pop();
		}
		mStack.pop();

		if (bUseRayTrace)
		{
			GPU_PROFILE("Ray Trace");
			if (mObjectList.size())
			{
				ObjectData* pData = mObjectBuffer.lock();
				memcpy(pData, mObjectList.data(), sizeof(ObjectData) * mObjectList.size());
				mObjectBuffer.unlock();
			}

			RayTraceProgram* progRayTrace;
			if (bUseDeferredRending)
			{
				RHISetFrameBuffer(commandList, mFrameBuffer);
				LinearColor clearColors[2] = { LinearColor(0, 0, 0, 0) , LinearColor(0, 0, 0, 0) };
				RHIClearRenderTargets(commandList, EClearBits::Color , clearColors , 2 );

				progRayTrace = mProgRayTraceDeferred;
			}
			else
			{
				progRayTrace = (bUseSceneBuitin) ? mProgRayTraceBuiltin : mProgRayTrace;
			}

			RHISetRasterizerState(commandList, TStaticRasterizerState<ECullMode::None >::GetRHI());
			RHISetBlendState(commandList, TStaticBlendState<>::GetRHI());
			RHISetDepthStencilState(commandList, StaticDepthDisableState::GetRHI());

			RHISetShaderProgram(commandList, progRayTrace->getRHIResource());
			mView.setupShader(commandList, *progRayTrace);
			SET_SHADER_PARAM(commandList, *progRayTrace, ObjectNum, (int)mObjectList.size());
			progRayTrace->setStructuredStorageBufferT< ObjectData >(commandList, *mObjectBuffer.getRHI());
			progRayTrace->setStructuredStorageBufferT< MaterialData >(commandList, *mMaterialBuffer.getRHI());
			if (!bUseSceneBuitin && !bUseDeferredRending)
			{
				SET_SHADER_PARAM(commandList, *progRayTrace, MapTileNum, int(mNumMapTile));
				progRayTrace->setStructuredStorageBufferT< MapTileData >(commandList, *mMapTileBuffer.getRHI());
			}
			DrawUtility::ScreenRect(commandList);

			if (bUseDeferredRending)
			{
				RHISetFrameBuffer(commandList, nullptr);
				RHISetShaderProgram(commandList, mProgRayTraceLighting->getRHIResource());
				mProgRayTraceLighting->setRenderTargets(commandList, mRenderBuffers);
				//SET_SHADER_PARAM(commandList, *mProgRayTraceLighting, ObjectNum, (int)mObjectList.size());
				mProgRayTraceLighting->setParam(commandList, SHADER_PARAM(ObjectNum), (int)mObjectList.size());
				mProgRayTraceLighting->setStructuredStorageBufferT< ObjectData >(commandList, *mObjectBuffer.getRHI());
				mProgRayTraceLighting->setStructuredStorageBufferT< MaterialData >(commandList, *mMaterialBuffer.getRHI());
				DrawUtility::ScreenRect(commandList);

				mProgRayTraceLighting->clearRenderTargets(commandList);
			}
		}

		RHISetFrameBuffer(commandList, nullptr);
		RHISetFixedShaderPipelineState(commandList, mView.worldToClip);
		DrawUtility::AixsLine(commandList);

		RHIGraphics2D& g = ::Global::GetRHIGraphics2D();

	}

	void TestStage::drawObjectBody( Vector3 const& color )
	{
		RHICommandList& commandList = RHICommandList::GetImmediateList();
		for( int i = 0 ; i < mObject.mNumBodies ; ++i )
		{
			Vec3i const& posBody = mObject.mBodiesPos[i];
			mStack.push();
			mStack.translate(Vector3(posBody));

			if (bUseRayTrace)
			{
				ObjectData object;
				Matrix4 worldToLocal;
				float det;
				mStack.get().inverse(worldToLocal, det);
				object.worldToLocal = worldToLocal;
				object.worldToLocal.translate(Vector3(-0.5));
				object.Type = 0;
				object.MatID = 3;
				object.typeParams = Vector4(1,1,1,0);
				mObjectList.push_back(object);
			}
			else
			{
				RHISetRasterizerState(commandList, TStaticRasterizerState<ECullMode::Back, EFillMode::Solid >::GetRHI());
				RHISetFixedShaderPipelineState(commandList, mStack.get(), LinearColor(color.x, color.y, color.z, 1));
				mCube.draw(commandList);

				RHISetRasterizerState(commandList, TStaticRasterizerState<ECullMode::Back, EFillMode::Wireframe >::GetRHI());
				RHISetFixedShaderPipelineState(commandList, mStack.get(), LinearColor(0, 0, 0, 1));
			}
			mCube.draw(commandList);
			mStack.pop();
		}
	}

	void TestStage::requestMove(Dir dir)
	{
		if ( mMoveCur != DIR_NONE )
			return;

		Object testObj = Object( mObject );
		testObj.move( dir );
		uint8 holeDirMask;
		State state = checkObjectState( testObj , holeDirMask );
		switch( state )
		{
		case eFall: return;
		case eGoal: mIsGoal = true; break;
		case eMove: mIsGoal = false; break;
		}
	
		mMoveCur = dir;
		switch( dir )
		{
		case DIR_X:
			mRotateAxis  = Vector3(0,1,0);
			mRotatePos   = Vector3(1+mObject.getMaxLocalPosX(),0,0);
			break;
		case DIR_NX:
			mRotateAxis  = Vector3(0,-1,0);
			mRotatePos   = Vector3(0,0,0);
			break;
		case DIR_Y:
			mRotateAxis  = Vector3(-1,0,0);
			mRotatePos   = Vector3(0,1+mObject.getMaxLocalPosY(),0);
			break;
		case DIR_NY:
			mRotateAxis  = Vector3(1,0,0);
			mRotatePos   = Vector3(0,0,0);
			break;
		}
		float time = 0.4f;
		mRotateAngle = 0;
		mTweener.tweenValue< Easing::OQuad >( mRotateAngle , 0.0f , 90.0f , time ).finishCallback( std::bind( &TestStage::moveObject , this ) );
		Vector3 to;
		to.x = testObj.getPos().x + 1;
		to.y = testObj.getPos().y + 1;
		to.z = 0;
		mTweener.tweenValue< Easing::OQuad >( mLookPos , mLookPos  , to , time );
	}

	void TestStage::moveObject()
	{
		mObject.move( mMoveCur );
		mMoveCur = DIR_NONE;
	}

	TestStage::State TestStage::checkObjectState( Object const& object , uint8& holeDirMask )
	{
		Vec2i posAcc = Vec2i::Zero();
		Vec2i holdPos[ Object::MaxBodyNum ];
		int numHold = 0;
		Vec3i const& posObj = object.getPos();

		bool isGoal = true;
		for( int i = 0 ; i < object.getBodyNum() ; ++i )
		{
			Vec3i const& posBody = object.getBodyLocalPos( i );

			posAcc += 2 * Vec2i( posBody.x , posBody.y );

			Vec2i pos;
			pos.x = posObj.x + posBody.x;
			pos.y = posObj.y + posBody.y;

			if ( mMap.checkRange( pos.x , pos.y ) )
			{
				int data =  mMap.getData( pos.x , pos.y );
				if ( data )
				{
					holdPos[ numHold ].x = 2 * posBody.x;
					holdPos[ numHold ].y = 2 * posBody.y;
					++numHold;

					if ( data != 2 )
					{
						isGoal = false;
					}
				}
			}
		}

		Vec2i centerPos;
		centerPos.x = floor( float( posAcc.x ) / object.getBodyNum() );
		if ( ( posAcc.x % object.getBodyNum() ) != 0 && ( centerPos.x % 2 ) == 1 )
			centerPos.x += 1;
		centerPos.y = floor( float( posAcc.y ) / object.getBodyNum() );
		if ( ( posAcc.y % object.getBodyNum() ) != 0 && ( centerPos.y % 2 ) == 1 )
			centerPos.y += 1;

		holeDirMask = ( BIT( DIR_X ) | BIT( DIR_Y ) | BIT( DIR_NX ) | BIT( DIR_NY ) );
		for( int i = 0 ; i < numHold ; ++i )
		{
			Vec2i const& pos = holdPos[i];

			if ( centerPos.x < pos.x )
				holeDirMask &= ~( BIT(DIR_X) );
			else if ( centerPos.x > pos.x )
				holeDirMask &= ~( BIT(DIR_NX) );
			else
			{
				holeDirMask &= ~( BIT(DIR_X) | BIT(DIR_NX) );
			}

			if ( centerPos.y < pos.y )
				holeDirMask &= ~( BIT(DIR_Y) );
			else if ( centerPos.y > pos.y )
				holeDirMask &= ~( BIT(DIR_NY) );
			else
			{
				holeDirMask &= ~( BIT(DIR_Y) | BIT(DIR_NY) );
			}
		}
		
		if ( holeDirMask != 0 )
			return eFall;

		if ( !isGoal )
			return eMove;

		return eGoal;
	}

	bool TestStage::onKey(KeyMsg const& msg)
	{
		if ( msg.isDown())
		{
			switch (msg.getCode())
			{
			case EKeyCode::R: restart(); break;
			}

			if ( !canInput() )
			{
				switch (msg.getCode())
				{
				case EKeyCode::W: mCamera.moveFront(1); break;
				case EKeyCode::S: mCamera.moveFront(-1); break;
				case EKeyCode::D: mCamera.moveRight(1); break;
				case EKeyCode::A: mCamera.moveRight(-1); break;
				case EKeyCode::Z: mCamera.moveUp(0.5); break;
				case EKeyCode::X: mCamera.moveUp(-0.5); break;
				}
			}
		}

		return BaseClass::onKey(msg);
	}

	bool TestStage::onMouse(MouseMsg const& msg)
	{
		if ( !BaseClass::onMouse( msg ) )
			return false;

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

		return false;
	}

	std::istream& operator >> ( std::istream& is , Vector3& vec )
	{
		is >> vec.x >> vec.y >> vec.z;
		return is;
	}

	std::istream& operator >> ( std::istream& is , Vector2& vec )
	{
		is >> vec.x >> vec.y;
		return is;
	}

	bool OBJFile::load(char const* path)
	{
#if 0
		std::ifstream fs( path );

		char buf[256];

		Vec3f      vec;
		GroupInfo* groupCur = NULL;

		int index = 0;

		while ( fs )
		{
			fs.getline( buf , sizeof(buf)/sizeof(char) );

			std::string decr;
			std::stringstream ss( buf );

			StringTokenizer

			ss >> decr;

			if ( decr == "v" )
			{
				Vec3f v;
				ss >> v;
				vertices.push_back( v );
			}
			else if ( decr == "vt" )
			{
				Vector2 v;
				ss >> v;
				UVs.push_back( v );
			}
			else if ( decr == "vn" )
			{
				Vec3f v;
				ss >> v;
				normals.push_back( v );
			}
			else if ( decr == "g" )
			{
				groupCur = new GroupInfo;
				groups.push_back( groupCur );
				groupCur->format;
				ss >> groupCur->name;
			}
			else if ( decr == "f" )
			{
				char sbuf[256];
				int v[3];
				for ( int i = 0 ; i < 3 ; ++ i )
				{
					ss >> sbuf;
					sscanf(  sbuf , "%d/%d/%d" , &v[0] , &v[1] , &v[2]);
					tri.v[i] = v[0];
				}
			}
			else if ( decr == "usemtl")
			{
				ss >> groupCur->matName;
			}
			else if ( decr == "mtllib" )
			{
				loadMatFile( buf + strlen("mtllib") + 1 );
			}
		}
#endif

		return true;
	}

}//namespace Bloxorz


REGISTER_STAGE2("Bloxorz Test", Bloxorz::TestStage, EStageGroup::Dev, 2);