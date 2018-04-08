#include "TinyGamePCH.h"
#include "RenderGLStage.h"
#include "RenderGLStageAssetID.h"

#include "GpuProfiler.h"

#include "GLUtility.h"
#include "DrawUtility.h"
#include "RHICommand.h"
#include "ShaderCompiler.h"

#include "GameGUISystem.h"
#include "GLGraphics2D.h"
#include "FileSystem.h"
#include "WidgetUtility.h"
#include "Thread.h"

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




namespace RenderGL
{
	bool LoadHeightMapRawFile(RHITexture2D& texture, char const* path , Texture::Format format , Vec2i const& size )
	{
		std::vector< char > buffer;
		if( !FileUtility::LoadToBuffer(path, buffer) )
			return false;

		uint32 byteNum = Texture::GetFormatSize(format) * size.x * size.y;
		if( byteNum != buffer.size() )
			return false;


		if( !texture.create(format, size.x, size.y, &buffer[0] , 1) )
			return false;

		texture.bind();
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		texture.unbind();

		return true;
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

		//::Global::getDrawEngine()->changeScreenSize(1600, 800);
		::Global::getDrawEngine()->changeScreenSize(1280, 720);

		if ( !Global::getDrawEngine()->startOpenGL() )
			return false;

		if( !mAssetManager.init() )
			return false;

		Vec2i screenSize = ::Global::getDrawEngine()->getScreenSize();

		if( !InitGlobalRHIResource() )
			return false;

		if( !ShaderHelper::Get().init() )
			return false;


		LazyObjectManager::Get().registerDefalutObject< StaticMesh >(&mEmptyMesh);
		LazyObjectManager::Get().registerDefalutObject< Material >(GDefalutMaterial);

		if( !loadAssetResouse() )
			return false;

		if ( !ShaderManager::Get().loadFileSimple(mProgSphere,"Shader/Game/Sphere" ) )
			return false;
		if ( !ShaderManager::Get().loadFileSimple(mEffectSphereSM,"Shader/Game/Sphere" , "#define USE_SHADOW_MAP 1" ) )
			return false;
		if( !ShaderManager::Get().loadMultiFile(mProgBump , "Shader/Game/Bump") )
			return false;
		if( !ShaderManager::Get().loadFileSimple(mProgParallax,"Shader/Game/Parallax") )
			return false;
		if( !ShaderManager::Get().loadFileSimple(mEffectSimple,"Shader/Game/Simple") )
			return false;
		if( !ShaderManager::Get().loadFileSimple(mProgPlanet, "Shader/Game/PlanetSurface") )
			return false;

		if( !mSceneRenderTargets.init(screenSize) )
			return false;

		if( !mShadowTech.init() )
			return false;

		if( !mDefferredShadingTech.init(mSceneRenderTargets) )
			return false;

		if( !mOITTech.init(screenSize) )
			return false;

		if( !mSSAO.init(screenSize) )
			return false;


		char const* entryNames[] = { SHADER_ENTRY(MainVS) , SHADER_ENTRY(MainPS) , SHADER_ENTRY(MainGS) };
		if( !ShaderManager::Get().loadFile(
			mProgSimpleSprite, "Shader/SimpleSprite",
			BIT(Shader::eVertex) | BIT(Shader::ePixel) | BIT(Shader::eGeometry),
			entryNames ) )
			return false;

		if( !mLayerFrameBuffer.create() )
			return false;

		mLayerFrameBuffer.addTextureLayer(*mShadowTech.mShadowMap);

		{
			mSpritePosMesh.mDecl.addElement(Vertex::ATTRIBUTE0, Vertex::eFloat3);
			mSpritePosMesh.mDecl.addElement(Vertex::ATTRIBUTE1, Vertex::eFloat3);
			mSpritePosMesh.mDecl.addElement(Vertex::ATTRIBUTE2, Vertex::eFloat3);
			mSpritePosMesh.mType = PrimitiveType::ePoints;

			int numVertex = 100000;
			std::vector< Vector3 > vtx;

			int numElemant = mSpritePosMesh.mDecl.getVertexSize() / sizeof(Vector3);
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
			if( !mSpritePosMesh.createBuffer(&vtx[0], numVertex) )
				return false;
		}

		if( !MeshBuild::PlaneZ(mSimpleMeshs[SimpleMeshId::Plane], 10, 1) ||
		   !MeshBuild::Tile(mSimpleMeshs[SimpleMeshId::Tile], 64, 1.0f) ||
		   !MeshBuild::UVSphere(mSimpleMeshs[SimpleMeshId::Sphere], 2.5, 60, 60) ||
		   //!MeshUtility::createUVSphere(mSimpleMeshs[ SimpleMeshId::Sphere ], 2.5, 4, 4) ||
		   !MeshBuild::IcoSphere(mSimpleMeshs[SimpleMeshId::Sphere2], 2.5, 4) ||
		   !MeshBuild::Cube(mSimpleMeshs[SimpleMeshId::Box]) ||
		   !MeshBuild::SkyBox(mSimpleMeshs[SimpleMeshId::SkyBox]) ||
		   !MeshBuild::Doughnut(mSimpleMeshs[SimpleMeshId::Doughnut], 2, 1, 60, 60) ||
		   !MeshBuild::SimpleSkin(mSimpleMeshs[SimpleMeshId::SimpleSkin], 5, 2.5, 20, 20) )
			return false;

		int const TerrainTileNum = 1024;
		if( !MeshBuild::Tile(mSimpleMeshs[SimpleMeshId::Terrain], 1024, 1.0, false) )
			return false;
		
		mTexTerrainHeight = RHICreateTexture2D();
		if( !LoadHeightMapRawFile(*mTexTerrainHeight, "Terrain/heightmap.r16", Texture::eR16, Vec2i( TerrainTileNum + 1 , TerrainTileNum + 1 )) )
			return false;

		ShaderCompileOption option;
		option.version = 430;
		if( !ShaderManager::Get().loadFile(
			mProgTerrain, "Shader/Terrain",
			SHADER_ENTRY(MainVS), SHADER_ENTRY(MainPS),
			option, nullptr) )
			return false;

		Vector3 v[] =
		{
			Vector3(1,1,0),
			Vector3(-1,1,0) ,
			Vector3(-1,-1,0),
			Vector3(1,-1,0),
		};
		int   idx[6] = { 0 , 1 , 2 , 0 , 2 , 3 };
		mSimpleMeshs[SimpleMeshId::SpherePlane].mDecl.addElement(Vertex::ePosition, Vertex::eFloat3);
		if( !mSimpleMeshs[SimpleMeshId::SpherePlane].createBuffer(&v[0], 4, &idx[0], 6, true) )
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

		mTexSky = new RHITextureCube;
		if ( !mTexSky->loadFile( paths ) )
			return false;

		mTexSky->bind();
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
		mTexSky->unbind();

		//////////////////////////////////////
		wglSwapIntervalEXT(0);
		mViewFrustum.mNear = 0.01;
		mViewFrustum.mFar  = 800.0;
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

		glEnable( GL_CULL_FACE );
		glEnable( GL_DEPTH_TEST );

		glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_NICEST);

		::Global::GUI().cleanupWidget();

		using namespace std::placeholders;
#define ADD_TEST( NAME , FUN )\
	mSampleTests.push_back({NAME,std::bind(&SampleStage::FUN,this,_1)})

		ADD_TEST("Sphere Plane", renderTest0);
		ADD_TEST("Parallax Mapping", renderTest1);
		ADD_TEST("PointLight shadow", renderTest2);
		ADD_TEST("Ray Test", renderTest3);
		ADD_TEST("Deferred Lighting", renderTest4);
		ADD_TEST("Spite Test", renderTest5);
		ADD_TEST("OIT Test", renderTest6);
		ADD_TEST("Terrain Test", renderTerrain);
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
		Global::getDrawEngine()->stopOpenGL(true);
	}

	void SampleStage::onEnd()
	{
		if( mLoadingThread )
			mLoadingThread->kill();
		ShaderManager::Get().clearnupRHIResouse();
		Global::getDrawEngine()->stopOpenGL(true);
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
		mAssetManager.tick();

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

	void SampleStage::onRender(float dFrame)
	{
		if( !bInitialized )
			return;

		if( !mGpuSync.pervRender() )
			return;

		GpuProfiler::Get().beginFrame();
		GameWindow& window = Global::getDrawEngine()->getWindow();

		ViewInfo view;
		assert(mCameraMove.target);
		view.gameTime = mTime;
		view.realTime = mRealTime;
		view.setupTransform(mCameraMove.target->getViewMatrix() , mViewFrustum.getPerspectiveMatrix() );

		{
			GPU_PROFILE("Frame");
			glMatrixMode(GL_PROJECTION);
			glLoadMatrixf(view.viewToClip);

			glMatrixMode(GL_MODELVIEW);

			glMatrixMode(GL_MODELVIEW);
			glLoadMatrixf(view.worldToView);

			glClearColor(0, 0, 0, 0);
			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

			glViewport(0, 0, window.getWidth(), window.getHeight());

			//drawAxis( 20 );
			
			mSampleTests[mIdxTestChioce].renderFun(view);
		}

		GpuProfiler::Get().endFrame();

		GLGraphics2D& g = ::Global::getGLGraphics2D();

		FixString< 512 > str;
		Vector3 v;
		g.beginRender();

		char const* title = mSampleTests[mIdxTestChioce].name;

		int const offset = 15;
		int textX = 1000;
		int y = 10;
		str.format("%s lightCount = %d", title , renderLightCount);
		g.drawText(textX, y , str );
		str.format("CamPos = %.1f %.1f %.1f", view.worldPos.x, view.worldPos.y, view.worldPos.z);
		g.drawText(textX, y += offset, str);
		for( int i = 0; i < GpuProfiler::Get().getSampleNum(); ++i )
		{
			GpuProfileSample* sample = GpuProfiler::Get().getSample(i);
			str.format("%.2lf => %s", sample->time, sample->name.c_str());
			g.drawText(textX + 10 * sample->level , y += offset, str);

		}
		g.endRender();

		mGpuSync.postRender();
	}

	void SampleStage::render( RenderContext& context)
	{
		context.beginRender();
		renderScene(context);
		context.endRender();
	}

	void SampleStage::renderTranslucent( RenderContext& context)
	{
		context.beginRender();

		Matrix4 matWorld;
		if ( mIdxTestChioce != 4 )
		{
			StaticMesh& mesh = getMesh(MeshId::Sponza);
			glColor3f(0.7, 0.7, 0.7);
			matWorld = Matrix4::Identity();
			mesh.render(matWorld, context , true );
		}
		else
		{
			glDisable(GL_CULL_FACE);
			if(1 )
			{
				
				matWorld = Matrix4::Rotate(Vector3(0, 0, -1), Math::Deg2Rad(45 + 180)) * Matrix4::Translate(Vector3(-6, 6, 4));

				Mesh& mesh = getMesh(MeshId::Teapot);
				context.setupShader(getMaterial(MaterialId::MetelA));
				glPushMatrix();
				context.setWorld(matWorld);
				glColor3f(0.7, 0.7, 0.7);
				mesh.draw(true);
				glPopMatrix();

				
			}
			if (1)
			{
				StaticMesh& mesh = getMesh(MeshId::Lightning);
				glColor3f(0.7, 0.7, 0.7);
				matWorld = Matrix4::Scale(3) * Matrix4::Rotate(Vector3(0, 0, -1), Math::Deg2Rad(135 + 180)) * Matrix4::Translate(Vector3(-16, 0, 10));
				//matWorld = Matrix4::Identity();
				mesh.render(matWorld, context , true );
			}

			glEnable(GL_CULL_FACE);
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

	void SampleStage::renderTest0(ViewInfo& view)
	{
		//glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
		//glColor3f( 1 , 1 , 0 );

		//glTranslatef( 20 , 20 , 0 );

		{
			GL_BIND_LOCK_OBJECT(mProgSphere);

			view.setupShader(mProgSphere);
			mProgSphere.setParam(SHADER_PARAM(Sphere.radius), 0.3f);
			mProgSphere.setParam(SHADER_PARAM(Sphere.localPos), mLights[1].pos);
			mSimpleMeshs[ SimpleMeshId::SpherePlane ].draw();

			mProgSphere.setParam(SHADER_PARAM(Sphere.radius), 1.5f);
			mProgSphere.setParam(SHADER_PARAM(Sphere.localPos), Vector3(10, 3, 1.0f));
			mSimpleMeshs[SimpleMeshId::SpherePlane].draw();
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

		GL_BIND_LOCK_OBJECT(mProgPlanet);

		mProgPlanet.setParam(SHADER_PARAM(planet.radius), 2.0f);
		mProgPlanet.setParam(SHADER_PARAM(planet.maxHeight), 0.1f);
		mProgPlanet.setParam(SHADER_PARAM(planet.baseHeight), 0.5f);

		float const TileBaseScale = 2.0f;
		float const TilePosMin = -1.0f;
		float const TilePosMax = 1.0f;
		mProgPlanet.setParam(SHADER_PARAM(tile.pos), TilePosMin, TilePosMin);
		mProgPlanet.setParam(SHADER_PARAM(tile.scale), TileBaseScale);

		for( int i = 0 ; i < 6 ; ++i )
		{
			mProgPlanet.setMatrix33(SHADER_PARAM(xformFace) , xform[i] );
			glPushMatrix();
			mSimpleMeshs[SimpleMeshId::Tile].draw();
			glPopMatrix();
		}
		glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
	}

	void SampleStage::renderTest1(ViewInfo& view)
	{
		//glColor3f( 1 , 1 , 1 );

		glEnable( GL_TEXTURE_2D );
		if ( 1 )
		{
			GL_BIND_LOCK_OBJECT(mProgParallax);
			//glTranslatef( 5 , 0 , 0 );
			//float m[16];
			//glGetFloatv( GL_MODELVIEW_MATRIX , m );

			//mProgParallax.setMatrix44( "matMV" , m );
			view.setupShader(mProgParallax);

#if 0
			mProgParallax.setTexture(SHADER_PARAM(texBase), getTexture(TextureId::RocksD), 0);
			mProgParallax.setTexture(SHADER_PARAM(texNormal), getTexture(TextureId::RocksNH), 1);
#else
			mProgParallax.setTexture(SHADER_PARAM(texBase), getTexture(TextureId::Base).getRHI() ? *getTexture(TextureId::Base).getRHI() : *GDefaultMaterialTexture2D , 0);
			mProgParallax.setTexture(SHADER_PARAM(texNormal), getTexture(TextureId::Normal).getRHI() ? *getTexture(TextureId::Normal).getRHI() : *GDefaultMaterialTexture2D , 1);
#endif

			mProgParallax.setParam(SHADER_PARAM(GLight.worldPosAndRadius), Vector4(mLights[1].pos, mLights[1].radius));
			mSimpleMeshs[SimpleMeshId::Plane].draw();
		}

		glDisable( GL_TEXTURE_2D );
	}


	void SampleStage::renderTest2(ViewInfo& view)
	{
		drawSky();
		visitLights([this, &view](int index, LightInfo const& light)
		{
			if( bUseFrustumTest && !light.testVisible( view )  )
				return;

			mShadowTech.renderLighting(view, *this, light, index != 0);
		});
		showLight(view);
		mShadowTech.drawShadowTexture( mLights[mNumLightDraw -1].type , Vec2i(0, 0), 200 );
		//TechBase::drawCubeTexture(mTexSky);
	}

	void SampleStage::renderTest3(ViewInfo& view)
	{

		if ( mCameraMove.target != &mCamStorage[0] )
		{
			glPushMatrix();
			glMultMatrixf( mCamStorage[0].getTransform() );
			drawAxis( 1 );
			glColor3f( 1 , 1 , 1 );
			mFrustumMesh.draw();
			glPopMatrix();
		}

		glBegin(GL_LINES);
		glColor3f( 1 , 1 , 0 );
		glVertex3fv( &rayStart[0] );
		glVertex3fv( &rayEnd[0] );
		glEnd();

		mEffectSimple.bind();

		view.setupShader(mEffectSimple);
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

			DrawUtility::CubeMesh();
			glPopMatrix();
		}

		mEffectSimple.unbind();

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

	void SampleStage::renderTest4(ViewInfo& view)
	{
		getScene(0).prepareRender(view);

		{
			GPU_PROFILE("BasePass");
			mDefferredShadingTech.renderBassPass(view, *this );
		}
		if( bUseSSAO )
		{
			GPU_PROFILE("SSAO");
			mSSAO.render(view, mSceneRenderTargets);
		}
		if( 1 )
		{	
			GPU_PROFILE("Lighting");

			mDefferredShadingTech.prevRenderLights(view);

			renderLightCount = 0;
			visitLights([this, &view](int index, LightInfo const& light)
			{
				if( bUseFrustumTest && !light.testVisible(view) )
					return;

				++renderLightCount;

				glEnable(GL_DEPTH_TEST);
				ShadowProjectParam shadowProjectParam;
				shadowProjectParam.setupLight(light);
				mShadowTech.renderShadowDepth(view, *this, shadowProjectParam);
				glDisable(GL_DEPTH_TEST);

				mDefferredShadingTech.renderLight(view, light, shadowProjectParam );
			});
		}

		{
			GPU_PROFILE("ShowLight");
			mSceneRenderTargets.getFrameBuffer().setDepth(mSceneRenderTargets.getDepthTexture());
			{
				GL_BIND_LOCK_OBJECT(mSceneRenderTargets.getFrameBuffer());
				showLight(view);
			}
			mSceneRenderTargets.getFrameBuffer().removeDepthBuffer();
		}

		if( 1 )
		{
			GPU_PROFILE("DrawTransparency");
			mSceneRenderTargets.getFrameBuffer().setDepth(mSceneRenderTargets.getDepthTexture());
			{
				GL_BIND_LOCK_OBJECT(mSceneRenderTargets.getFrameBuffer());
				mOITTech.render(view, *this, &mSceneRenderTargets);
			}
			mSceneRenderTargets.getFrameBuffer().removeDepthBuffer();
		}

		if(1)
		{
			GPU_PROFILE("ShowBuffer");

			ViewportSaveScope vpScope;
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
					glViewport(0, h, 3 * w, 3 * h);
#endif
					ShaderHelper::Get().copyTextureToBuffer(mSceneRenderTargets.getRenderFrameTexture());
				}



				glDisable(GL_DEPTH_TEST);



				mSceneRenderTargets.getGBuffer().drawTextures( size , gapSize );

				mSSAO.drawSSAOTexture(Vec2i(0, 1).mul(size) + gapSize, renderSize);

				ShaderHelper::DrawTexture(*mOITTech.mColorStorageTexture, Vec2i(0, 0), Vec2i(200, 200));
				glEnable(GL_DEPTH_TEST);
			}
			else
			{
				ShaderHelper::Get().copyTextureToBuffer(mSceneRenderTargets.getRenderFrameTexture());

				glDisable(GL_DEPTH_TEST);
				mShadowTech.drawShadowTexture(mLights[mNumLightDraw - 1].type , Vec2i(0, 0), 200 );
				glEnable(GL_DEPTH_TEST);
			}
		}
	}

	void SampleStage::renderTest6(ViewInfo& view)
	{
		glClearColor(0.7, 0.7, 0.7, 1);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		if( 1 )
		{
			glEnable(GL_DEPTH_TEST);

			StaticMesh& mesh = getMesh(MeshId::Dragon2);

			glPushMatrix();
			glLoadMatrixf(view.worldToView);
			glColor3f(1, 1, 0);
			mesh.draw(false);
			glPopMatrix();
			mOITTech.render(view, *this , nullptr );
		}
		else
		{
			mOITTech.renderTest(view, mSceneRenderTargets, getMesh(MeshId::Havoc), getMaterial(MaterialId::Havoc));
		}
		
	}


	void SampleStage::renderTerrain(ViewInfo& view)
	{
		{
			GL_BIND_LOCK_OBJECT(mProgTerrain);

			view.setupShader(mProgTerrain);

			
			mProgTerrain.setParam( SHADER_PARAM(LocalToWorld), Matrix4::Identity() );
			mProgTerrain.setParam( SHADER_PARAM(TerrainScale), Vector3( 50 , 50 , 10 ) );
			mProgTerrain.setTexture( SHADER_PARAM(TextureHeight), *mTexTerrainHeight);
			mProgTerrain.setTexture(SHADER_PARAM(TextureBaseColor), *getTexture(TextureId::Terrain).getRHI());
			mSimpleMeshs[SimpleMeshId::Terrain].draw();
		}
	}

	void SampleStage::showLight(ViewInfo& view)
	{
		GL_BIND_LOCK_OBJECT(mProgSphere);

		float radius = 0.15f;
		view.setupShader(mProgSphere);
		mProgSphere.setParam(SHADER_PARAM(Primitive.worldToLocal), Matrix4::Identity());
		mProgSphere.setParam(SHADER_PARAM(Sphere.radius), radius);

		visitLights([this,&view, radius](int index, LightInfo const& light)
		{
			if( bUseFrustumTest && !light.testVisible(view) )
				return;

			mProgSphere.setParam(SHADER_PARAM(Sphere.localPos), light.pos);
			mProgSphere.setParam(SHADER_PARAM(Sphere.baseColor), light.color);
			mSimpleMeshs[SimpleMeshId::SpherePlane].draw();
		});
	}


	void SampleStage::drawSky()
	{
		Matrix4 mat;
		glGetFloatv( GL_MODELVIEW_MATRIX , mat );

		glDisable( GL_DEPTH_TEST );
		glDepthMask( GL_FALSE );
		glEnable( GL_TEXTURE_CUBE_MAP );
		mTexSky->bind();

		glPushMatrix();

		mat.modifyTranslation( Vector3(0,0,0) );
		glLoadMatrixf( Matrix4::Rotate( Vector3(1,0,0), Math::Deg2Rad(90) ) * mat );
		glColor3f( 1 , 1 , 1 );
		mSimpleMeshs[SimpleMeshId::SkyBox].draw();
		glPopMatrix();

		mTexSky->unbind();
		glDisable( GL_TEXTURE_CUBE_MAP );
		glDepthMask( GL_TRUE );
		glEnable( GL_DEPTH_TEST );
	}

	void SampleStage::drawAxis(float len)
	{	
		glLineWidth( 1.5 );
		glBegin( GL_LINES );
		glColor3f(1,0,0);glVertex3f(0,0,0);glVertex3f(len,0,0);
		glColor3f(0,1,0);glVertex3f(0,0,0);glVertex3f(0,len,0);
		glColor3f(0,0,1);glVertex3f(0,0,0);glVertex3f(0,0,len);
		glEnd();
		glLineWidth( 1 );
	}

	void SampleStage::renderTest5(ViewInfo& view)
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
			glViewport(0, 0, 2048, 2048);
			glDisable(GL_DEPTH_TEST);
			glEnable(GL_BLEND);
			glBlendFunc(GL_ONE, GL_ONE);
			GL_BIND_LOCK_OBJECT(mProgSimpleSprite);
			view.setupShader(mProgSimpleSprite);
			mProgSimpleSprite.setTexture(SHADER_PARAM(BaseTexture), *getTexture(TextureId::Star).getRHI());
			mProgSimpleSprite.setParam(SHADER_PARAM(ViewMatrix), viewMatrix, 6);
			mProgSimpleSprite.setParam(SHADER_PARAM(ViewProjectMatrix), viewproject );
			mSpritePosMesh.draw(true);
			glDisable(GL_BLEND);
			glEnable(GL_DEPTH_TEST);
		}

		if( 1 )
		{
			ViewportSaveScope vpScope;
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
				glDisable(GL_DEPTH_TEST);
				mShadowTech.drawShadowTexture(LightType::Point , Vec2i(0,0) , 400 );
				//mSceneRenderTargets.getGBuffer().drawTextures(size, gapSize);
				glEnable(GL_DEPTH_TEST);
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
		mesh.mDecl.addElement(Vertex::ePosition, Vertex::eFloat3);
		if( !mesh.createBuffer(v, 8, idx, 24, true) )
			return false;
		mesh.mType = PrimitiveType::eLineList;
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

	bool SampleStage::onKey(unsigned key , bool isDown)
	{
		if ( !isDown )
			return false;

		switch( key )
		{
		case Keyboard::eR: restart(); break;
		case 'W': mCameraMove.moveFront(); break;
		case 'S': mCameraMove.moveBack(); break;
		case 'D': mCameraMove.moveRight(); break;
		case 'A': mCameraMove.moveLeft(); break;
		case 'Z': mCameraMove.target->moveUp(0.5); break;
		case 'X': mCameraMove.target->moveUp(-0.5); break;
		case Keyboard::eADD: mNumLightDraw = std::min(mNumLightDraw + 1, 4); break;
		case Keyboard::eSUBTRACT: mNumLightDraw = std::max(mNumLightDraw - 1, 0); break;
		case Keyboard::eB: mbShowBuffer = !mbShowBuffer; break;
		case Keyboard::eF3: 
			mLineMode = !mLineMode;
			glPolygonMode( GL_FRONT_AND_BACK , mLineMode ? GL_LINE : GL_FILL );
			break;
		case Keyboard::eM:
			ShaderHelper::Get().reload();
			reloadMaterials();
			break;
		case Keyboard::eL:
			mLights[0].bCastShadow = !mLights[0].bCastShadow;
			break;
		case Keyboard::eK:
			mOITTech.bUseBMA = !mOITTech.bUseBMA;
			break;
		case Keyboard::eP: mPause = !mPause; break;
		case Keyboard::eF2:
			if (0)
			{
				switch( mIdxTestChioce )
				{
				case 0:
					ShaderManager::Get().reloadShader(mProgSphere);
					ShaderManager::Get().reloadShader(mProgPlanet);
					break;
				case 1:
					ShaderManager::Get().reloadShader(mProgParallax);
					break;
				case 2: mShadowTech.reload(); break;
				case 4: mDefferredShadingTech.reload(); mSSAO.reload(); break;
				case 5: ShaderManager::Get().reloadShader(mProgSimpleSprite); break;
				case 6: mOITTech.reload(); break;
				}
			}
			else
			{
				ShaderManager::Get().reloadAll();
			}
			break;
		case Keyboard::eF5:
			if ( mCameraMove.target == &mCamStorage[0])
				mCameraMove.target = &mCamStorage[1];
			else
				mCameraMove.target = &mCamStorage[0];
			break;
		case Keyboard::eF6:
			mDefferredShadingTech.boundMethod = (LightBoundMethod)( ( mDefferredShadingTech.boundMethod + 1 ) % NumLightBoundMethod );
			break;
		case Keyboard::eF7:
			mDefferredShadingTech.debugMode = (DefferredShadingTech::DebugMode)( ( mDefferredShadingTech.debugMode + 1 ) % DefferredShadingTech::NumDebugMode );
			break;
		case Keyboard::eF8:
			bUseFrustumTest = !bUseFrustumTest;
			break;
		case Keyboard::eF4:
			break;
		case Keyboard::eNUM1: mIdxTestChioce = 0; break;
		case Keyboard::eNUM2: mIdxTestChioce = 1; break;
		case Keyboard::eNUM3: mIdxTestChioce = 2; break;
		case Keyboard::eNUM4: mIdxTestChioce = 3; break;
		case Keyboard::eNUM5: mIdxTestChioce = 4; break;
		case Keyboard::eNUM6: mIdxTestChioce = 5; break;
		case Keyboard::eNUM7: mIdxTestChioce = 6; break;
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
			Vec2i size = ::Global::getDrawEngine()->getScreenSize();
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

		mReflectMap = new RHITexture2D;
		if ( !mReflectMap->create( Texture::eRGB32F , MapSize , MapSize ) )
			return false;

		mBuffer.addTexture( *mReflectMap );
		RHIDepthRenderBufferRef depthBuf = new RHIDepthRenderBuffer;
		if ( ! depthBuf->create( MapSize , MapSize , Texture::eDepth32 ) )
			return false;
		mBuffer.setDepth( *depthBuf );

		return true;
	}


	bool GLGpuSync::pervRender()
	{
		if( !bUseFence )
			return true;

		GLbitfield flags = 0;

		if( loadingSync != NULL )
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
		if( renderSync != NULL )
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



}//namespace RenderGL



