#include "RenderGLStage.h"
#include "RenderAssetId.h"

#include "RHI/GpuProfiler.h"
#include "RHI/MeshUtility.h"
#include "RHI/DrawUtility.h"
#include "RHI/RHICommand.h"
#include "RHI/ShaderManager.h"

#include "GameGUISystem.h"
#include "GLGraphics2D.h"
#include "FileSystem.h"
#include "Widget/WidgetUtility.h"
#include "PlatformThread.h"

#include "GL/wglew.h"
#include <inttypes.h>

/*
#TODO:
#Material
-Translucent Impl
-Tangent Y sign Fix

#Shader
-World Position reconstrcut
-ForwardShading With Material
-Shadow PCF
-PostProcess flow Register

-Merge with CFly Library !!
-SceneManager
-Game Client Type : Mesh -> Object Texture
-Use Script setup Scene and reload
-RHI Resource Manage
-Game Resource Manage ( Material Object )
*/




namespace Render
{
	RHITexture2D* CreateHeightMapRawFile(char const* path , Texture::Format format , Vec2i const& size )
	{
		std::vector< char > buffer;
		if( !FileUtility::LoadToBuffer(path, buffer) )
			return nullptr;

		uint32 byteNum = Texture::GetFormatSize(format) * size.x * size.y;
		if( byteNum != buffer.size() )
			return nullptr;

		RHITexture2D* texture = RHICreateTexture2D(format, size.x, size.y, 1, 1, TCF_DefalutValue , &buffer[0], 1);
		if( texture == nullptr )
			return nullptr;

		{
			GL_BIND_LOCK_OBJECT(*texture);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		}

		return texture;
	}

	IAssetProvider* SceneAsset::sAssetProvider = nullptr;

	SampleStage::SampleStage()
	{
		mPause = false;
		mbShowBuffer = true;

		SceneAsset::sAssetProvider = this;
	}

	SampleStage::~SampleStage()
	{

	}

	float dbg;




	bool SampleStage::onInit()
	{
		testCode();

		//::Global::GetDrawEngine().changeScreenSize(1600, 800);
		::Global::GetDrawEngine().changeScreenSize(1280, 720);

		VERIFY_RETURN_FALSE(Global::GetDrawEngine().initializeRHI(RHITargetName::OpenGL));

		wglSwapIntervalEXT(0);

		Vec2i screenSize = ::Global::GetDrawEngine().getScreenSize();


		if( !ShaderHelper::Get().init() )
			return false;


		LazyObjectManager::Get().registerDefalutObject< StaticMesh >(&mEmptyMesh);
		LazyObjectManager::Get().registerDefalutObject< Material >(GDefalutMaterial);

		if( !loadAssetResouse() )
			return false;

		VERIFY_RETURN_FALSE(SharedAssetData::loadCommonShader());

		VERIFY_RETURN_FALSE(ShaderManager::Get().loadMultiFile(mProgBump, "Shader/Game/Bump"));
		VERIFY_RETURN_FALSE(ShaderManager::Get().loadFileSimple(mProgParallax, "Shader/Game/Parallax"));
		VERIFY_RETURN_FALSE(ShaderManager::Get().loadFileSimple(mEffectSimple, "Shader/Game/Simple"));
		VERIFY_RETURN_FALSE(ShaderManager::Get().loadFileSimple(mProgPlanet, "Shader/Game/PlanetSurface"));

		VERIFY_RETURN_FALSE(mProgShadowVolume = ShaderManager::Get().getGlobalShaderT< ShadowVolumeProgram >());

		VERIFY_RETURN_FALSE(mSceneRenderTargets.initializeRHI(screenSize));
		VERIFY_RETURN_FALSE(mTechShadow.init());
		VERIFY_RETURN_FALSE(mTechDeferredShading.init(mSceneRenderTargets));
		VERIFY_RETURN_FALSE(mTechOIT.init(screenSize));
		VERIFY_RETURN_FALSE(mTechVolumetricLighing.init(screenSize));
		VERIFY_RETURN_FALSE(mSSAO.init(screenSize));
		VERIFY_RETURN_FALSE(mDOF.init(screenSize));

		ShaderEntryInfo const entryNames[] = 
		{ 
			Shader::eVertex , SHADER_ENTRY(MainVS) ,
			Shader::ePixel  , SHADER_ENTRY(MainPS) ,
			Shader::eGeometry , SHADER_ENTRY(MainGS) ,
		};
		VERIFY_RETURN_FALSE(ShaderManager::Get().loadFile(mProgSimpleSprite, "Shader/Game/SimpleSprite", entryNames));

		if( !mLayerFrameBuffer.create() )
			return false;

		mLayerFrameBuffer.setupTextureLayer(*mTechShadow.mShadowMap);

		{
			mSpritePosMesh.mInputLayoutDesc.addElement(0, Vertex::ATTRIBUTE0, Vertex::eFloat3);
			mSpritePosMesh.mInputLayoutDesc.addElement(0, Vertex::ATTRIBUTE1, Vertex::eFloat3);
			mSpritePosMesh.mInputLayoutDesc.addElement(0, Vertex::ATTRIBUTE2, Vertex::eFloat3);
			mSpritePosMesh.mType = EPrimitive::Points;

			int numVertex = 100000;
			std::vector< Vector3 > vtx;

			int numElemant = mSpritePosMesh.mInputLayoutDesc.getVertexSize() / sizeof(Vector3);
			vtx.resize(numVertex * numElemant);

			Vector3* v = &vtx[0];
			float boxSize = 100;
			for( int i = 0; i < numVertex; ++i )
			{
				*v = RandVector(boxSize * Vector3(-1, -1, -1), boxSize *  Vector3(1,1,1));
				*(v + 1) = 10 * RandVector();
				*(v + 2) = 10 * RandVector();
				v += numElemant;
			}
			if( !mSpritePosMesh.createRHIResource(&vtx[0], numVertex) )
				return false;
		}

		VERIFY_RETURN_FALSE(SharedAssetData::createSimpleMesh());

		VERIFY_RETURN_FALSE(mSimpleMeshs[SimpleMeshId::Sphere].generateVertexAdjacency());

		int TerrainTileNum = 1024;
		mTexTerrainHeight = CreateHeightMapRawFile("Terrain/heightmap.r16", Texture::eR16, Vec2i(TerrainTileNum + 1, TerrainTileNum + 1));
		if( !mTexTerrainHeight.isValid() )
			return false;

		ShaderCompileOption option;
		if( !ShaderManager::Get().loadFile(
			mProgTerrain, "Shader/Terrain",
			SHADER_ENTRY(MainVS), SHADER_ENTRY(MainPS),
			option, nullptr) )
			return false;


#define TEX_DIR "Texture/LancellottiChapel/"
#define TEX_SUB_NAME ".jpg"
		char const* paths[] =
		{
			TEX_DIR "posx" TEX_SUB_NAME,
			TEX_DIR "negx" TEX_SUB_NAME,
			TEX_DIR "posy" TEX_SUB_NAME,
			TEX_DIR "negy" TEX_SUB_NAME,
			TEX_DIR "posz" TEX_SUB_NAME,
			TEX_DIR "negz" TEX_SUB_NAME,
		};
#undef  TEX_DIR
#undef  TEX_SUB_NAME

		mTexSky = RHIUtility::LoadTextureCubeFromFile(paths);
		if ( !mTexSky.isValid() )
			return false;

		OpenGLCast::To(mTexSky)->bind();
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
		OpenGLCast::To(mTexSky)->unbind();

		//////////////////////////////////////

		mViewFrustum.mNear = 0.01;
		mViewFrustum.mFar  = 2000.0;
		mViewFrustum.mAspect = float(screenSize.x) / screenSize.y;
		mViewFrustum.mYFov   = Math::Deg2Rad( 60 / mViewFrustum.mAspect );

		if ( !createFrustum( mFrustumMesh , mViewFrustum.getPerspectiveMatrix() ) )
			return false;

		mCameraMove.target = &mCamStorage[0];
		mCameraMove.target->setPos( Vector3( -10 , 0 , 10 ) );
		mCameraMove.target->setViewDir(Vector3(0, 0, -1), Vector3(1, 0, 0));

		mAabb[0].min = Vector3(1, 1, 1);
		mAabb[0].max = Vector3(5, 4, 7);
		mAabb[1].min = Vector3(-5, -5, -5);
		mAabb[1].max = Vector3(3, 5, 2);
		mAabb[2].min = Vector3(2, -5, -5);
		mAabb[2].max = Vector3(5, -3, 15);

		mTime = 0;
		mRealTime = 0;
		mIdxTestChioce = 0;

		setupScene();

		glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_NICEST);


		RHICommandList& commandList = RHICommandList::GetImmediateList();
		RHISetRasterizerState(commandList, TStaticRasterizerState<>::GetRHI());
		RHISetDepthStencilState(commandList, TStaticDepthStencilState<>::GetRHI());

	
		::Global::GUI().cleanupWidget();

		using namespace std::placeholders;
#define ADD_TEST( NAME , FUN )\
	mSampleTests.push_back({NAME,std::bind(&SampleStage::FUN,this,_1,_2)})

		ADD_TEST("Sphere Plane", render_SpherePlane);
		ADD_TEST("Parallax Mapping", render_ParallaxMapping);
		ADD_TEST("PointLight shadow", renderTest2);
		ADD_TEST("Ray Test", render_RaycastTest);
		ADD_TEST("Deferred Lighting", render_DeferredLighting);
		ADD_TEST("Spite Test", render_Sprite);
		ADD_TEST("OIT Test", render_OIT);
		ADD_TEST("Terrain Test", render_Terrain);
		ADD_TEST("Shadow Volume", render_ShadowVolume);
#undef ADD_TEST

		auto devFrame = WidgetUtility::CreateDevFrame(Vec2i(150, 400));
		for ( int i = 0 ; i < (int)mSampleTests.size() ; ++i )
		{
			auto& test = mSampleTests[i];
			devFrame->addButton(UI_SAMPLE_TEST, test.name)->setUserData(i);
		}

		bInitialized = true;
		restart();
		return true;
	}

	void SampleStage::onInitFail()
	{
		if( mLoadingThread )
			mLoadingThread->kill();
		ShaderManager::Get().clearnupRHIResouse();
		::Global::GetDrawEngine().shutdownRHI();
	}

	void SampleStage::onEnd()
	{
		unregisterAllAsset();

		if( mLoadingThread )
			mLoadingThread->kill();
		ShaderManager::Get().clearnupRHIResouse();

		::Global::GetDrawEngine().shutdownRHI();
	}

	void SampleStage::testCode()
	{

		VectorCurveTrack track;
		track.mode = TrackMode::Oscillation;
		track.addKey(0, Vector3(0, 0, 0));
		track.addKey(20, Vector3(10, 20, 30));
		track.addKey(10, Vector3(5, 10, 15));
		
		Vector3 a = track.getValue(5);
		Vector3 b = track.getValue(-5);
		Vector3 c = track.getValue(25);
		Vector3 d = track.getValue(35);
		Vector3 e = track.getValue(40);
		Vector3 f = track.getValue(-20);

		{
			Vector3 n(1, 1, 0);
			float factor = n.normalize();
			ReflectMatrix m(n, 1.0 * factor);
			Vector3 v = Vector3(1, 0, 0) * m;
			int i = 1;
		}

	}

	void SampleStage::tick()
	{

		float deltaTime = float(gDefaultTickTime) / 10000;

		getScene(0).tick(deltaTime);

		mCameraMove.update(deltaTime);

		mRealTime += deltaTime;
		if ( mPause )
			return;

		mTime += gDefaultTickTime / 1000.0f;

		mPos.x = 10 * cos( mTime );
		mPos.y = 10 * sin( mTime );
		mPos.z = 0;

		mLights[0].dir.x = sin(mTime);
		mLights[0].dir.y = cos(mTime);
		mLights[0].dir.z = 0;
		mLights[0].pos = mTracks[1].getValue(mTime);
		mLights[0].dir = mTracks[0].getValue(mTime);
		for( int i = 1; i < 4; ++i )
		{
			mLights[i].pos = mTracks[i].getValue(mTime);
		}
		
		for( auto& modifier : mValueModifiers )
		{
			modifier->update(mTime);
		}

	}

	void SampleStage::unregisterAllAsset()
	{
		AssetManager& manager = ::Global::GetAssetManager();
		for( auto& asset : mMaterialAssets )
		{
			manager.unregisterViewer(&asset);
		}
		for( auto& asset : mSceneAssets )
		{
			manager.unregisterViewer(asset.get());
		}
		ShaderManager::Get().unregisterShaderAssets(manager);
	}

	void SampleStage::onRender(float dFrame)
	{
		if( !bInitialized )
			return;

		if( !mGpuSync.pervRender() )
			return;

		GameWindow& window = Global::GetDrawEngine().getWindow();

		RHICommandList& commandList = RHICommandList::GetImmediateList();


		assert(mCameraMove.target);
		mCameraView.gameTime = mTime;
		mCameraView.realTime = mRealTime;
		mCameraView.rectOffset = IntVector2(0, 0);
		mCameraView.rectSize = IntVector2(window.getWidth(), window.getHeight());
		
		mCameraView.setupTransform(mCameraMove.target->getViewMatrix() , mViewFrustum.getPerspectiveMatrix() );

		{
			GPU_PROFILE("Frame");
			glMatrixMode(GL_PROJECTION);
			glLoadMatrixf(mCameraView.viewToClip);
			glMatrixMode(GL_MODELVIEW);
			glLoadMatrixf(mCameraView.worldToView);

			glClearColor(0, 0, 0, 0);
			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

			RHISetViewport(commandList, mCameraView.rectOffset.x, mCameraView.rectOffset.y, mCameraView.rectSize.x, mCameraView.rectSize.y);
			//drawAxis( 20 );
			RHISetRasterizerState(commandList, TStaticRasterizerState<>::GetRHI());
			RHISetDepthStencilState(commandList, TStaticDepthStencilState<>::GetRHI());
			RHISetBlendState(commandList, TStaticBlendState<>::GetRHI());
			mSampleTests[mIdxTestChioce].renderFun(commandList, mCameraView);


			if ( 0 )
			{
				Vec2i screenSize = ::Global::GetDrawEngine().getScreenSize();
				Vec2i imageSize = IntVector2(200, -200);
				ViewportSaveScope vpScope(commandList);
				OrthoMatrix matProj(0, vpScope[2], 0, vpScope[3], -1, 1);
				MatrixSaveScope matScope(matProj);
				RHISetBlendState(commandList, TStaticBlendState<CWM_RGBA , Blend::eSrcAlpha , Blend::eOneMinusSrcAlpha >::GetRHI());
				RHISetRasterizerState(commandList, TStaticRasterizerState< ECullMode::None >::GetRHI());
				LinearColor color = LinearColor(0.2, 0.2, 0.2, 0.2);
				DrawUtility::DrawTexture(commandList, getTexture(TextureId::WaterMark).getRHI(), IntVector2( screenSize.x , 0 ) - imageSize - IntVector2(20,-20), imageSize , color );
				RHISetRasterizerState(commandList, TStaticRasterizerState<>::GetRHI());
				RHISetBlendState(commandList, TStaticBlendState<>::GetRHI());
			}

		}

		GLGraphics2D& g = ::Global::GetRHIGraphics2D();

		FixString< 512 > str;
		g.beginRender();


		char const* title = mSampleTests[mIdxTestChioce].name;

		int const offset = 15;
		int textX = 1000;
		int y = 10;
		str.format("%s lightCount = %d", title , renderLightCount);
		g.drawText(textX, y , str );
		str.format("CamPos = %.1f %.1f %.1f", mCameraView.worldPos.x, mCameraView.worldPos.y, mCameraView.worldPos.z);
		g.drawText(textX, y += offset, str);

		g.endRender();

		mGpuSync.postRender();
	}

	void SampleStage::render(RenderContext& context)
	{
		context.beginRender();
		renderScene(context);
		context.endRender();
	}

	void SampleStage::renderTranslucent(RenderContext& context)
	{
		
		context.beginRender();
		RHICommandList& commandList = context.getCommnadList();
		Matrix4 matWorld;
		if ( mIdxTestChioce != 4 )
		{
			StaticMesh& mesh = getMesh(MeshId::Sponza);
			matWorld = Matrix4::Identity();
			mesh.render( matWorld, context );
		}
		else
		{
			RHISetRasterizerState(commandList, TStaticRasterizerState<ECullMode::None>::GetRHI());
			if(1 )
			{
				matWorld = Matrix4::Rotate(Vector3(0, 0, -1), Math::Deg2Rad(45 + 180)) * Matrix4::Translate(Vector3(-6, 6, 4));

				Mesh& mesh = getMesh(MeshId::Teapot);
				context.setMaterial(getMaterial(MaterialId::MetelA));
				context.setWorld(matWorld);
				//mesh.draw(commandList, LinearColor(0.7, 0.7, 0.7) );
				
			}
			if (1)
			{
				StaticMesh& mesh = getMesh(MeshId::Lightning);
				matWorld = Matrix4::Scale(3) * Matrix4::Rotate(Vector3(0, 0, -1), Math::Deg2Rad(135 + 180)) * Matrix4::Translate(Vector3(-16, 0, 10));
				//matWorld = Matrix4::Identity();
				mesh.render(matWorld, context);
			}
			RHISetRasterizerState(commandList, TStaticRasterizerState<>::GetRHI());
		}
		context.endRender();
	}

	void SampleStage::onRemoveLight(SceneLight* light)
	{
		for ( auto iter = mValueModifiers.begin() ; iter != mValueModifiers.end() ; )
		{
			IValueModifier* modifier = (*iter).get();
			if( modifier->isHook(&light->info.pos) ||
			    modifier->isHook(&light->info.color) ||
			    modifier->isHook(&light->info.intensity) )
			{
				iter = mValueModifiers.erase(iter);
			}
			else
			{
				++iter;
			}
		}
	}

	void SampleStage::onRemoveObject(SceneObject* object)
	{
		
	}

	void SampleStage::render_SpherePlane(RHICommandList& commandList, ViewInfo& view)
	{
		//glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
		//glColor3f( 1 , 1 , 0 );

		//glTranslatef( 20 , 20 , 0 );

		{
			RHISetShaderProgram(commandList, mProgSphere->getRHIResource());
			view.setupShader(commandList, * mProgSphere);

			mProgSphere->setParameters(commandList, mLights[1].pos, 0.3, Color3f(1, 1, 1) );
			mSimpleMeshs[SimpleMeshId::SpherePlane].draw(commandList);

			mProgSphere->setParameters(commandList, Vector3(10, 3, 1.0f), 1.5f , Color3f(1, 1, 1) );
			mSimpleMeshs[SimpleMeshId::SpherePlane].draw(commandList);
		}

		float const off = 0.5;
		float xform[6][9] = 
		{
			{ 0,0,-1, 0,1,0, 1,0,0, } ,
			{ 0,0,1, 0,1,0, -1,0,0, } ,
			{ 1,0,0, 0,0,-1, 0,1,0, } ,
			{ 1,0,0, 0,0,1,  0,-1,0 } ,
			{ 1,0,0, 0,1,0,  0,0,1  } ,
			{ 1,0,0, 0,-1,0, 0,0,-1 } ,
		};
		glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

		RHISetShaderProgram(commandList, mProgPlanet.getRHIResource());
		mProgPlanet.setParam(commandList, SHADER_PARAM(planet.radius), 2.0f);
		mProgPlanet.setParam(commandList, SHADER_PARAM(planet.maxHeight), 0.1f);
		mProgPlanet.setParam(commandList, SHADER_PARAM(planet.baseHeight), 0.5f);

		float const TileBaseScale = 2.0f;
		float const TilePosMin = -1.0f;
		float const TilePosMax = 1.0f;
		mProgPlanet.setParam(commandList, SHADER_PARAM(tile.pos), TilePosMin, TilePosMin);
		mProgPlanet.setParam(commandList, SHADER_PARAM(tile.scale), TileBaseScale);

		for( int i = 0 ; i < 6 ; ++i )
		{
			mProgPlanet.setMatrix33(commandList, SHADER_PARAM(xformFace) , xform[i] );
			glPushMatrix();
			mSimpleMeshs[SimpleMeshId::Tile].draw(commandList);
			glPopMatrix();
		}
		glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
	}

	void SampleStage::render_ParallaxMapping(RHICommandList& commandList, ViewInfo& view)
	{
		if ( 1 )
		{
			RHISetShaderProgram(commandList, mProgParallax.getRHIResource());
			view.setupShader(commandList, mProgParallax);

#if 0
			mProgParallax.setTexture(commandList, SHADER_PARAM(texBase), getTexture(TextureId::RocksD).getRHI() );
			mProgParallax.setTexture(commandList, SHADER_PARAM(texNormal), getTexture(TextureId::RocksNH).getRHI() );
#else
			mProgParallax.setTexture(commandList, SHADER_PARAM(texBase), getTexture(TextureId::Base).getRHI() );
			mProgParallax.setTexture(commandList, SHADER_PARAM(texNormal), getTexture(TextureId::Normal).getRHI() );
#endif

			mProgParallax.setParam(commandList, SHADER_PARAM(GLight.worldPosAndRadius), Vector4(mLights[1].pos, mLights[1].radius));
			mSimpleMeshs[SimpleMeshId::Plane].draw(commandList);
		}
	}


	void SampleStage::renderTest2(RHICommandList& commandList, ViewInfo& view)
	{
		drawSky(commandList);
		visitLights([this, &view, &commandList](int index, LightInfo const& light)
		{
			if( bUseFrustumTest && !light.testVisible( view )  )
				return;

			mTechShadow.renderLighting(commandList, view, *this, light, index != 0);
		});
		showLights(commandList, view);
		mTechShadow.drawShadowTexture(commandList, mLights[mNumLightDraw -1].type , Vec2i(0, 0), 200 );
		//TechBase::drawCubeTexture(mTexSky);
	}

	void SampleStage::render_RaycastTest(RHICommandList& commandList, ViewInfo& view)
	{

		if ( mCameraMove.target != &mCamStorage[0] )
		{
			glPushMatrix();
			glMultMatrixf( mCamStorage[0].getTransform() );
			drawAxis(commandList , 1 );
			glColor3f( 1 , 1 , 1 );
			mFrustumMesh.draw(commandList);
			glPopMatrix();
		}

		glBegin(GL_LINES);
		glColor3f( 1 , 1 , 0 );
		glVertex3fv( &rayStart[0] );
		glVertex3fv( &rayEnd[0] );
		glEnd();


		RHISetShaderProgram(commandList, mEffectSimple.getRHIResource());

		view.setupShader(commandList, mEffectSimple);
#if 0
		glPushMatrix(); 
		glMultMatrixf( Matrix4::Rotate( Vector3(1,1,1) , Math::Deg2Rad(45) ) * Matrix4::Translate( Vector3(3,-3,7) ) );
		glColor3f( 0.3 , 0.3 , 1 );
		mBoxMesh.draw();
		glPopMatrix();
#endif

		for ( int i = 0 ; i < NumAabb ; ++i )
		{
			AABB& aabb = mAabb[i];
			glPushMatrix();
			Vector3 len = aabb.max - aabb.min;
			glTranslatef( aabb.min.x , aabb.min.y , aabb.min.z );
			glScalef( len.x , len.y , len.z );
			if ( mIsIntersected )
				glColor3f( 1 , 1 , 1 );
			else
				glColor3f( 0.3 , 0.3 , 0.3 );

			DrawUtility::CubeMesh(commandList);
			glPopMatrix();
		}

		RHISetShaderProgram(commandList, nullptr);

		if ( mIsIntersected )
		{
			glPointSize( 5 );
			glBegin( GL_POINTS );
			glColor3f( 1 , 0 , 0 );
			glVertex3fv( &mIntersectPos[0] );
			glEnd();
			glPointSize( 1 );
		}
	}

	bool bUseSSAO = true;
	bool bUseDOF = true;
	void SampleStage::render_DeferredLighting(RHICommandList& commandList, ViewInfo& view)
	{
		getScene(0).prepareRender(view);

		{
			GPU_PROFILE("BasePass");
			mTechDeferredShading.renderBassPass(commandList, view, *this );
		}
		if( bUseSSAO )
		{
			GPU_PROFILE("SSAO");
			mSSAO.render(commandList, view, mSceneRenderTargets);
		}

		if( 1 )
		{	
			GPU_PROFILE("Lighting");

			//RHISetRasterizerState(TStaticRasterizerState< ECullMode::None >::GetRHI());
			mTechDeferredShading.prevRenderLights(commandList, view);

			renderLightCount = 0;
			visitLights([this, &view, &commandList](int index, LightInfo const& light)
			{
				if( bUseFrustumTest && !light.testVisible(view) )
					return;

				++renderLightCount;
				
				ShadowProjectParam shadowProjectParam;
				shadowProjectParam.setupLight(light);
				mTechShadow.renderShadowDepth(commandList, view, *this, shadowProjectParam);

				mTechDeferredShading.renderLight(commandList, view, light, shadowProjectParam );
			});
		}

		if ( 1 )
		{
			std::vector< LightInfo > lights;
			visitLights([&lights](int index, LightInfo const& light)
			{
				lights.push_back(light);
			});
			GPU_PROFILE("VolumetricLighting");
			mTechVolumetricLighing.render(commandList, view, lights);
		}

		bUseDOF = false;

		if( bUseDOF)
		{
			GPU_PROFILE("DOF");
			mDOF.render(commandList, view, mSceneRenderTargets);
		}

		if ( 1 )
		{
			GPU_PROFILE("ShowLight");
			mSceneRenderTargets.attachDepthTexture();
			{
				GL_BIND_LOCK_OBJECT(mSceneRenderTargets.getFrameBuffer());
				showLights(commandList, view);
			}
			mSceneRenderTargets.detachDepthTexture();
		}

		if( 1 )
		{
			GPU_PROFILE("DrawTransparency");
			mSceneRenderTargets.attachDepthTexture();
			{
				GL_BIND_LOCK_OBJECT(mSceneRenderTargets.getFrameBuffer());
				mTechOIT.render(commandList, view, *this, &mSceneRenderTargets);
			}
			mSceneRenderTargets.detachDepthTexture();
		}

		if(1)
		{

			GPU_PROFILE("ShowBuffer");
			RHISetFrameBuffer(commandList, nullptr);

			ViewportSaveScope vpScope(commandList);
			OrthoMatrix matProj(0, vpScope[2], 0, vpScope[3], -1, 1);
			MatrixSaveScope matScope(matProj);

			Vec2i showSize;

			float gapFactor = 0.03;

			Vec2i size;
			size.x = vpScope.value[2] / 4;
			size.y = vpScope.value[3] / 4;

			Vec2i gapSize;
			gapSize.x = size.x * gapFactor;
			gapSize.y = size.y * gapFactor;

			Vec2i renderSize = size - 2 * gapSize;

			if( mbShowBuffer )
			{
				{
#if 0
					ViewportSaveScope vpScope;
					int w = vpScope[2] / 4;
					int h = vpScope[3] / 4;
					RHISetViewport(0, h, 3 * w, 3 * h);
#endif
					ShaderHelper::Get().copyTextureToBuffer(commandList, mSceneRenderTargets.getFrameTexture());
				}

				RHISetDepthStencilState(commandList, StaticDepthDisableState::GetRHI());
				mSceneRenderTargets.getGBuffer().drawTextures(commandList, size , gapSize );

				//mSSAO.drawSSAOTexture(commandList, Vec2i(0, 1).mul(size) + gapSize, renderSize);
#if 0
				DrawUtility::DrawTexture(commandList, *mTechOIT.mColorStorageTexture, Vec2i(0, 0), Vec2i(200, 200));
#else
				//DrawUtility::DrawTexture(commandList, getTexture(TextureId::MarioD).getRHI() , Vec2i(0, 0), Vec2i(200, 200));
#endif
			}
			else
			{
				ShaderHelper::Get().copyTextureToBuffer(commandList, mSceneRenderTargets.getFrameTexture());

				RHISetDepthStencilState(commandList, StaticDepthDisableState::GetRHI());
				mTechShadow.drawShadowTexture(commandList, mLights[mNumLightDraw - 1].type , Vec2i(0, 0), 200 );
			}
		}
	}

	void SampleStage::render_OIT(RHICommandList& commandList, ViewInfo& view)
	{
		glClearColor(0.7, 0.7, 0.7, 1);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		if( 1 )
		{
			StaticMesh& mesh = getMesh(MeshId::Dragon2);
			RHISetDepthStencilState(commandList, TStaticDepthStencilState<>::GetRHI());
			RHISetupFixedPipelineState(commandList, view.worldToClip, LinearColor(1, 1, 0));
			mesh.draw(commandList);
			mTechOIT.render(commandList, view, *this , nullptr );
		}
		else
		{
			mTechOIT.renderTest(commandList, view, mSceneRenderTargets, getMesh(MeshId::Havoc), getMaterial(MaterialId::Havoc));
		}	
	}


	void SampleStage::render_Terrain(RHICommandList& commandList, ViewInfo& view)
	{


		{
			RHISetShaderProgram(commandList, mProgTerrain.getRHIResource());
			view.setupShader(commandList, mProgTerrain);
			mProgTerrain.setParam(commandList, SHADER_PARAM(LocalToWorld), Matrix4::Identity() );
			mProgTerrain.setParam(commandList, SHADER_PARAM(TerrainScale), Vector3( 50 , 50 , 10 ) );
			mProgTerrain.setTexture(commandList, SHADER_PARAM(TextureHeight), *mTexTerrainHeight);
			mProgTerrain.setTexture(commandList, SHADER_PARAM(TextureBaseColor), getTexture(TextureId::Terrain).getRHI());
			mSimpleMeshs[SimpleMeshId::Terrain].draw(commandList);
		}


	}

	void SampleStage::render_Portal(RHICommandList& commandList, ViewInfo& view)
	{

	}

	class ShadowVolumeProgram : public GlobalShaderProgram
	{
		DECLARE_SHADER_PROGRAM(ShadowVolumeProgram, Global);
	public:
		static void SetupShaderCompileOption(ShaderCompileOption& option)
		{

		}
		static char const* GetShaderFileName()
		{
			return "Shader/ShadowVolume";
		}
		static TArrayView< ShaderEntryInfo const > GetShaderEntries()
		{

			static ShaderEntryInfo const entries[] =
			{
				{ Shader::eVertex , SHADER_ENTRY(MainVS) },
				{ Shader::eGeometry  , SHADER_ENTRY(MainGS) },
				{ Shader::ePixel  , SHADER_ENTRY(MainPS) },
			};

			return entries;
		}

	};

	IMPLEMENT_SHADER_PROGRAM(ShadowVolumeProgram);

	void SampleStage::render_ShadowVolume(RHICommandList& commandList, ViewInfo& view)
	{
		drawAxis(commandList, 10);



		{
			RHISetShaderProgram(commandList, mProgTerrain.getRHIResource());
			view.setupShader(commandList, mProgTerrain);
			mProgTerrain.setParam(commandList, SHADER_PARAM(LocalToWorld), Matrix4::Translate(-50,-50,-10));
			mProgTerrain.setParam(commandList, SHADER_PARAM(TerrainScale), Vector3(100, 100, 20));
			mProgTerrain.setTexture(commandList, SHADER_PARAM(TextureHeight), *mTexTerrainHeight);
			mProgTerrain.setTexture(commandList, SHADER_PARAM(TextureBaseColor), getTexture(TextureId::Terrain).getRHI());
			mSimpleMeshs[SimpleMeshId::Terrain].draw(commandList);
		}


		auto& mesh = *getMesh(MeshId::Mario).get();
		//auto& mesh = mSimpleMeshs[SimpleMeshId::Box];
		if( !mesh.generateVertexAdjacency() )
			return;


		Matrix4 localToWorld = Matrix4::Rotate( Vector3(0,0,1) , Math::Deg2Rad(mRealTime * 45));
		{
			RHISetupFixedPipelineState(commandList, localToWorld * view.worldToClip, LinearColor(0.2, 0.2, 0.2, 1));
			mesh.draw(commandList);
		}

		auto DrawMesh = [&]()
		{
			mProgShadowVolume->setParam(commandList, SHADER_PARAM(LocalToWorld), localToWorld);
			mesh.drawAdj(commandList, LinearColor(1, 0, 0, 1));
		};

		bool bVisualizeVolume = false;
		bool bUseDepthPass = false;
		bool bShowVolumeStencil = true;

		{

			GPU_PROFILE("Render Shadow Volume");

			glStencilMask(-1);
			glClearStencil(0);
			glClear(GL_STENCIL_BUFFER_BIT);

			RHISetRasterizerState(commandList, TStaticRasterizerState< ECullMode::None >::GetRHI());
			if( bVisualizeVolume )
			{
				RHISetBlendState(commandList, TStaticBlendState< CWM_RGBA, Blend::eOne, Blend::eOne >::GetRHI());
			}
			else
			{
				RHISetBlendState(commandList, TStaticBlendState<CWM_None>::GetRHI());
			}		

			if( bUseDepthPass )
			{
				RHISetDepthStencilState(commandList,
					TStaticDepthStencilSeparateState< false, ECompareFunc::Less, true,
						ECompareFunc::Always, Stencil::eKeep, Stencil::eKeep, Stencil::eDecrWarp,
						ECompareFunc::Always, Stencil::eKeep, Stencil::eKeep, Stencil::eIncrWarp
					>::GetRHI());
			}
			else
			{				
				RHISetDepthStencilState(commandList,
					TStaticDepthStencilSeparateState< false, ECompareFunc::Less, true,
						ECompareFunc::Always, Stencil::eKeep, Stencil::eDecrWarp, Stencil::eKeep,
						ECompareFunc::Always, Stencil::eKeep, Stencil::eIncrWarp, Stencil::eKeep
					>::GetRHI());
			}


#if 0
			Matrix4 prevProjectMatrix = view.viewToClip;
			float zn = PerspectiveMatrix::GetNearZ(prevProjectMatrix);
			Matrix4 newProjectMatrix = prevProjectMatrix;
			newProjectMatrix[10] = -1;
			newProjectMatrix[14] = -2 * zn;
			view.setupTransform(view.worldToView, newProjectMatrix);
#endif

			RHISetShaderProgram(commandList, mProgShadowVolume->getRHIResource());
			mProgShadowVolume->setParam(commandList, SHADER_PARAM(LightPos), mLights[0].pos);
			mProgShadowVolume->setParam(commandList, SHADER_PARAM(LightPos), Vector3(20, 10, 20));
			view.setupShader(commandList, *mProgShadowVolume);

			//glEnable(GL_POLYGON_OFFSET_FILL);
			//glPolygonOffset(-1.0, -1.0);
			//glLineWidth(8);
			DrawMesh();
			//glLineWidth(1);
			//glDisable(GL_POLYGON_OFFSET_FILL);
			RHISetRasterizerState(commandList, TStaticRasterizerState<>::GetRHI());
		}

		
		if ( bShowVolumeStencil  )
		{
			ViewportSaveScope vpScope(commandList);
			MatrixSaveScope matrixScopt( Matrix4::Identity() );
			RHISetRasterizerState(commandList, TStaticRasterizerState<ECullMode::None>::GetRHI());
			RHISetDepthStencilState(commandList, TStaticDepthStencilState<
				false , ECompareFunc::Always , 
				true , ECompareFunc::NotEqual , Stencil::eKeep , Stencil::eKeep , Stencil::eKeep , 0xff , 0xff 
			>::GetRHI() , 0x0 );
			RHISetBlendState(commandList, TStaticBlendState< CWM_RGBA , Blend::eOne , Blend::eOne >::GetRHI());
			glColor3f(0, 0, 0.8);
			DrawUtility::ScreenRect(commandList);
		}
		RHISetDepthStencilState(commandList, TStaticDepthStencilState<>::GetRHI());
		RHISetBlendState(commandList, TStaticBlendState<>::GetRHI());
	}

	void SampleStage::showLights(RHICommandList& commandList, ViewInfo& view)
	{
		RHISetShaderProgram( commandList , mProgSphere->getRHIResource());

		RHISetBlendState(commandList, TStaticBlendState<>::GetRHI());
		RHISetDepthStencilState(commandList, TStaticDepthStencilState<>::GetRHI());

		float radius = 0.15f;
		view.setupShader(commandList, *mProgSphere);
		mProgSphere->setParam(commandList, SHADER_PARAM(Primitive.worldToLocal), Matrix4::Identity());

		visitLights([this,&view, radius, &commandList](int index, LightInfo const& light)
		{
			if( bUseFrustumTest && !light.testVisible(view) )
				return;

			mProgSphere->setParameters(commandList, light.pos, radius, light.color);
			mSimpleMeshs[SimpleMeshId::SpherePlane].draw(commandList);
		});
	}


	void SampleStage::drawSky(RHICommandList& commandList)
	{
		Matrix4 mat;
		glGetFloatv( GL_MODELVIEW_MATRIX , mat );

		RHISetDepthStencilState(commandList, StaticDepthDisableState::GetRHI());

		glEnable( GL_TEXTURE_CUBE_MAP );
		OpenGLCast::To(mTexSky)->bind();

		glPushMatrix();

		mat.modifyTranslation( Vector3(0,0,0) );
		glLoadMatrixf( Matrix4::Rotate( Vector3(1,0,0), Math::Deg2Rad(90) ) * mat );
		mSimpleMeshs[SimpleMeshId::SkyBox].draw(commandList, LinearColor(1, 1, 1));
		glPopMatrix();

		OpenGLCast::To(mTexSky)->unbind();
		glDisable( GL_TEXTURE_CUBE_MAP );
	}

	void SampleStage::drawAxis(RHICommandList& commandList, float len)
	{

		struct Vertex
		{
			Vector3 pos;
			Vector3 color;
		};

		Vertex v[] =
		{
			{ Vector3(0,0,0), Vector3(1,0,0)  } ,{ Vector3(len,0,0),Vector3(1,0,0) },
			{ Vector3(0,0,0), Vector3(0,1,0)  } ,{ Vector3(0,len,0),Vector3(0,1,0) },
			{ Vector3(0,0,0), Vector3(0,0,1), } ,{ Vector3(0,0,len),Vector3(0,0,1) },
		};

		glLineWidth( 1.5 );
		TRenderRT< RTVF_XYZ_C >::Draw(commandList, EPrimitive::LineList, &v[0], 6);
		glLineWidth( 1 );
	}

	void SampleStage::render_Sprite(RHICommandList& commandList, ViewInfo& view)
	{
		Vector3 CubeFaceDir[] =
		{
			Vector3(1,0,0),Vector3(-1,0,0),
			Vector3(0,1,0),Vector3(0,-1,0),
			Vector3(0,0,1),Vector3(0,0,-1),
		};
		Vector3 CubeUpDir[] =
		{
			Vector3(0,-1,0),Vector3(0,-1,0),
			Vector3(0,0,1),Vector3(0,0,-1),
			Vector3(0,-1,0),Vector3(0,-1,0),
		};
		Matrix4 viewMatrix[6];

		Matrix4 viewproject = PerspectiveMatrix(Math::Deg2Rad(90), 1.0, 0.01, 500);
		for( int i = 0; i < 6; ++i )
		{
			viewMatrix[i] = LookAtMatrix(view.worldPos, CubeFaceDir[i], CubeUpDir[i]);
		};

		{
			GPU_PROFILE("DrawSprite");
			GL_BIND_LOCK_OBJECT(mLayerFrameBuffer);
			glClear(GL_COLOR_BUFFER_BIT);
			RHISetViewport(commandList, 0, 0, 2048, 2048);
			RHISetDepthStencilState(commandList, StaticDepthDisableState::GetRHI());
			RHISetBlendState(commandList, TStaticBlendState< CWM_RGBA, Blend::eOne, Blend::eOne >::GetRHI());
			
			RHISetShaderProgram(commandList, mProgSimpleSprite.getRHIResource());
			view.setupShader(commandList, mProgSimpleSprite);
			mProgSimpleSprite.setTexture(commandList, SHADER_PARAM(BaseTexture), getTexture(TextureId::Star).getRHI());
			mProgSimpleSprite.setParam(commandList, SHADER_PARAM(ViewMatrix), viewMatrix, 6);
			mProgSimpleSprite.setParam(commandList, SHADER_PARAM(ViewProjectMatrix), viewproject );
			mSpritePosMesh.draw(commandList);

		}

		if( 1 )
		{
			ViewportSaveScope vpScope(commandList);
			OrthoMatrix matProj(0, vpScope.value[2], 0, vpScope.value[3], -1, 1);
			MatrixSaveScope matScope(matProj);

			Vec2i showSize;

			float gapFactor = 0.03;

			Vec2i size;
			size.x = vpScope.value[2] / 4;
			size.y = vpScope.value[3] / 4;

			Vec2i gapSize;
			gapSize.x = size.x * gapFactor;
			gapSize.y = size.y * gapFactor;

			Vec2i renderSize = size - 2 * gapSize;

			if( mbShowBuffer )
			{
				RHISetDepthStencilState(commandList, StaticDepthDisableState::GetRHI());
				RHISetBlendState(commandList, TStaticBlendState<>::GetRHI());
				mTechShadow.drawShadowTexture(commandList, LightType::Point , Vec2i(0,0) , 400 );
				//mSceneRenderTargets.getGBuffer().drawTextures(size, gapSize);
			}
		}
	}

	bool SampleStage::createFrustum(Mesh& mesh, Matrix4 const& matProj)
	{
		float depth = 0.9999;
		Vector3 v[8] =
		{
			Vector3(1,1,depth),Vector3(1,-1,depth),Vector3(-1,-1,depth),Vector3(-1,1,depth),
			Vector3(1,1,-depth),Vector3(1,-1,-depth),Vector3(-1,-1,-depth),Vector3(-1,1,-depth),
		};

		Matrix4 matInv;
		float det;
		matProj.inverse(matInv, det);
		for( int i = 0; i < 8; ++i )
		{
			v[i] = TransformPosition(v[i], matInv);
		}

		int idx[24] =
		{
			0,1, 1,2, 2,3, 3,0,
			4,5, 5,6, 6,7, 7,4,
			0,4, 1,5, 2,6, 3,7,
		};
		mesh.mInputLayoutDesc.addElement(0, Vertex::ePosition, Vertex::eFloat3);
		if( !mesh.createRHIResource(v, 8, idx, 24, true) )
			return false;
		mesh.mType = EPrimitive::LineList;
		return true;
	}

	void SampleStage::calcViewRay(float x, float y)
	{
		Matrix4 matViewProj = mCameraMove.target->getViewMatrix() * mViewFrustum.getPerspectiveMatrix();
		Matrix4 matVPInv;
		float det;
		matViewProj.inverse(matVPInv, det);

		rayStart = TransformPosition(Vector3(x, y, -1), matVPInv);
		rayEnd = TransformPosition(Vector3(x, y, 1), matVPInv);
	}

	bool SampleStage::onKey(KeyMsg const& msg)
	{
		if ( !msg.isDown())
			return false;

		switch(msg.getCode())
		{
		case EKeyCode::R: restart(); break;
		case EKeyCode::W: mCameraMove.moveFront(); break;
		case EKeyCode::S: mCameraMove.moveBack(); break;
		case EKeyCode::D: mCameraMove.moveRight(); break;
		case EKeyCode::A: mCameraMove.moveLeft(); break;
		case EKeyCode::Z: mCameraMove.target->moveUp(0.5); break;
		case EKeyCode::X: mCameraMove.target->moveUp(-0.5); break;
		case EKeyCode::Add: mNumLightDraw = std::min(mNumLightDraw + 1, 4); break;
		case EKeyCode::Subtract: mNumLightDraw = std::max(mNumLightDraw - 1, 0); break;
		case EKeyCode::B: mbShowBuffer = !mbShowBuffer; break;
		case EKeyCode::F3: 
			mLineMode = !mLineMode;
			glPolygonMode( GL_FRONT_AND_BACK , mLineMode ? GL_LINE : GL_FILL );
			break;
		case EKeyCode::M:
			ShaderHelper::Get().reload();
			reloadMaterials();
			break;
		case EKeyCode::L:
			mLights[0].bCastShadow = !mLights[0].bCastShadow;
			break;
		case EKeyCode::K:
			mTechOIT.bUseBMA = !mTechOIT.bUseBMA;
			break;
		case EKeyCode::P: mPause = !mPause; break;
		case EKeyCode::F2:
			if (0)
			{
				switch( mIdxTestChioce )
				{
				case 0:
					ShaderManager::Get().reloadShader(*mProgSphere);
					ShaderManager::Get().reloadShader(mProgPlanet);
					break;
				case 1:
					ShaderManager::Get().reloadShader(mProgParallax);
					break;
				case 2: mTechShadow.reload(); break;
				case 4: mTechDeferredShading.reload(); mSSAO.reload(); break;
				case 5: ShaderManager::Get().reloadShader(mProgSimpleSprite); break;
				case 6: mTechOIT.reload(); break;
				}
			}
			else
			{
				ShaderManager::Get().reloadAll();
			}
			break;
		case EKeyCode::F5:
			if ( mCameraMove.target == &mCamStorage[0])
				mCameraMove.target = &mCamStorage[1];
			else
				mCameraMove.target = &mCamStorage[0];
			break;
		case EKeyCode::F6:
			mTechDeferredShading.boundMethod = (LightBoundMethod)( ( mTechDeferredShading.boundMethod + 1 ) % NumLightBoundMethod );
			break;
		case EKeyCode::F7:
			mTechDeferredShading.debugMode = (DeferredShadingTech::DebugMode)( ( mTechDeferredShading.debugMode + 1 ) % DeferredShadingTech::NumDebugMode );
			break;
		case EKeyCode::F8:
			bUseFrustumTest = !bUseFrustumTest;
			break;
		case EKeyCode::F4:
			break;
		case EKeyCode::Num1: mIdxTestChioce = 0; break;
		case EKeyCode::Num2: mIdxTestChioce = 1; break;
		case EKeyCode::Num3: mIdxTestChioce = 2; break;
		case EKeyCode::Num4: mIdxTestChioce = 3; break;
		case EKeyCode::Num5: mIdxTestChioce = 4; break;
		case EKeyCode::Num6: mIdxTestChioce = 5; break;
		case EKeyCode::Num7: mIdxTestChioce = 6; break;
		}
		return false;
	}

	bool SampleStage::onWidgetEvent(int event, int id, GWidget* ui)
	{
		switch( id )
		{
		case UI_SAMPLE_TEST:
			mIdxTestChioce = ui->getUserData();
			return false;
		}

		return BaseClass::onWidgetEvent(event, id, ui);
	}

	void SampleStage::reloadMaterials()
	{
		for( int i = 0; i < mMaterialAssets.size(); ++i )
		{
			mMaterialAssets[i].material->reload();
		}
	}

	bool SampleStage::onMouse(MouseMsg const& msg)
	{
		static Vec2i oldPos = msg.getPos();
		
		if ( msg.onLeftDown() )
		{
			oldPos = msg.getPos();
		}
		if ( msg.onMoving() && msg.isLeftDown() )
		{
			float rotateSpeed = 0.01;
			Vector2 off = rotateSpeed * Vector2( msg.getPos() - oldPos );
			mCameraMove.target->rotateByMouse( off.x , off.y );
			oldPos = msg.getPos();
		}

		if ( msg.onRightDown() )
		{
			Vec2i size = ::Global::GetDrawEngine().getScreenSize();
			Vec2i pos = msg.getPos();
			float x = float( 2 * pos.x ) / size.x - 1;
			float y = 1 - float( 2 * pos.y ) / size.y;
			calcViewRay( x , y );
			
			float tMin = 1;
			mIsIntersected = false;
			for( int i = 0 ; i < NumAabb ; ++i )
			{
				AABB& aabb = mAabb[i];
				float t = -1;
				if ( TestRayAABB( rayStart , rayEnd - rayStart , aabb.min , aabb.max , t ) )
				{
					mIsIntersected = true;
					if ( t < tMin )
					{
						tMin = t;
						mIntersectPos = rayStart + ( rayEnd - rayStart ) * t;
					}
				}
			}
		}

		if ( !BaseClass::onMouse( msg ) )
			return false;

		return true;
	}



	bool WaterTech::init()
	{
		if ( !mBuffer.create() )
			return false;
		if ( !MeshBuild::PlaneZ( mWaterMesh , 20 , 20 ) )
			return false;

		mReflectMap = RHICreateTexture2D(Texture::eRGB32F, MapSize, MapSize);
		if ( !mReflectMap.isValid() )
			return false;

		mBuffer.addTexture( *mReflectMap );
#if USE_DepthRenderBuffer
		RHIDepthRenderBufferRef depthBuf = new RHIDepthRenderBuffer;
		if ( ! depthBuf->create( MapSize , MapSize , Texture::eDepth32 ) )
			return false;
		mBuffer.setDepth( *depthBuf );
#endif
		return true;
	}


	bool GLGpuSync::pervRender()
	{
		if( !bUseFence )
			return true;

		GLbitfield flags = 0;

		if( loadingSync != nullptr )
		{
			GLbitfield flags = 0;
			glWaitSync(loadingSync, flags, GL_TIMEOUT_IGNORED);
		}
		return true;
	}

	void GLGpuSync::postRender()
	{
		if( bUseFence )
		{
			GLbitfield flags = 0;
			GLenum condition = GL_SYNC_GPU_COMMANDS_COMPLETE;
			renderSync = glFenceSync(condition, flags);
		}
	}

	bool GLGpuSync::pervLoading()
	{
		if( !bUseFence )
			return true;
		if( renderSync != nullptr )
		{
			GLbitfield flags = 0;
			glWaitSync(renderSync, flags, GL_TIMEOUT_IGNORED);
		}
		else
		{
			return false;
		}

		return true;
	}

	void GLGpuSync::postLoading()
	{
		if( bUseFence )
		{
			GLbitfield flags = 0;
			GLenum condition = GL_SYNC_GPU_COMMANDS_COMPLETE;
			loadingSync = glFenceSync(condition, flags);
		}
	}



}//namespace Render


REGISTER_STAGE2("Shader Test", Render::SampleStage, EStageGroup::FeatureDev, 10)



