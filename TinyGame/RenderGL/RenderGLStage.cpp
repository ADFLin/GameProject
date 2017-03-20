#include "TinyGamePCH.h"
#include "RenderGLStage.h"
#include "RenderGLStageAssetID.h"

#include "GpuProfiler.h"

#include "GLUtility.h"
#include "GLDrawUtility.h"
#include "ShaderCompiler.h"

#include "GameGUISystem.h"
#include "GLGraphics2D.h"
#include "FileSystem.h"
#include "WidgetUtility.h"

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

	SampleStage::SampleStage()
	{
		mPause = false;
		mbShowBuffer = true;
	}

	SampleStage::~SampleStage()
	{

	}

	bool SampleStage::onInit()
	{
		{
			Vector3 n(1, 1, 0);
			float factor = n.normalize();
			ReflectMatrix m(n, 1.0 * factor);
			Vector3 v = Vector3(1, 0, 0) * m;
			int i = 1;
		}

		//::Global::getDrawEngine()->changeScreenSize(1600, 800);
		::Global::getDrawEngine()->changeScreenSize(1280, 720);

		if ( !Global::getDrawEngine()->startOpenGL() )
			return false;

		Vec2i screenSize = ::Global::getDrawEngine()->getScreenSize();


		if( !ShaderHelper::getInstance().init() )
			return false;

		if( !loadAssetResouse() )
			return false;

		if( !MeshUtility::createPlaneZ(mPlane, 10, 3) ||
		   !MeshUtility::createTile(mMesh, 64, 1.0f) ||
		   !MeshUtility::createUVSphere(mSphereMesh, 2.5, 60, 60) ||
		   //!MeshUtility::createUVSphere(mSphereMesh, 2.5, 4, 4) ||
		   !MeshUtility::createIcoSphere(mSphereMesh2, 2.5, 4) ||
		   !MeshUtility::createCube(mBoxMesh) ||
		   !MeshUtility::createSkyBox(mSkyMesh) ||
		   !MeshUtility::createDoughnut(mDoughnutMesh, 2, 1, 60, 60) )
			return false;

		Vector3 v[] =
		{ 
			Vector3( 1,1,0 ), 
			Vector3( -1,1,0 ) ,
			Vector3( -1,-1,0 ),
			Vector3( 1,-1,0 ),
		};
		int   idx[6] = { 0 , 1 , 2 , 0 , 2 , 3 };
		mSpherePlane.mDecl.addElement( Vertex::ePosition , Vertex::eFloat3 );

		if( !ShaderManager::getInstance().loadFile(mProgPlanet,"Shader/PlanetSurface") )
			return false;

		if ( !mSpherePlane.create( &v[0] , 4  , &idx[0] , 6 , true ) )
			return false;

		if ( !ShaderManager::getInstance().loadFile(mProgSphere,"Shader/Sphere" ) )
			return false;
		if ( !ShaderManager::getInstance().loadFile(mEffectSphereSM,"Shader/Sphere" , "#define USE_SHADOW_MAP 1" ) )
			return false;

		if( !ShaderManager::getInstance().loadMultiFile(mProgBump , "Shader/Bump") )
			return false;
		if( !ShaderManager::getInstance().loadFile(mProgParallax,"Shader/Parallax") )
			return false;

		if( !ShaderManager::getInstance().loadFile(mEffectSimple,"Shader/Simple") )
			return false;

		if( !mSceneRenderTargets.init(screenSize) )
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

		if ( !mShadowTech.init() )
			return false;

		if( !mDefferredLightingTech.init(mSceneRenderTargets) )
			return false;

		if( !mSSAO.init(screenSize) )
			return false;


		//////////////////////////////////////
		wglSwapIntervalEXT(0);
		mViewFrustum.mNear = 0.01;
		mViewFrustum.mFar  = 800.0;
		mViewFrustum.mAspect = float(screenSize.x) / screenSize.y;
		mViewFrustum.mYFov   = Math::Deg2Rad( 60 / mViewFrustum.mAspect );

		if ( !createFrustum( mFrustumMesh , mViewFrustum.getPerspectiveMatrix() ) )
			return false;

		mCamera = &mCamStorage[0];

		mCamera->setPos( Vector3( -10 , 0 , 10 ) );
		mCamera->setViewDir(Vector3(0, 0, -1), Vector3(1, 0, 0));

		mAabb[0].min = Vector3(1, 1, 1);
		mAabb[0].max = Vector3(5, 4, 7);
		mAabb[1].min = Vector3(-5, -5, -5);
		mAabb[1].max = Vector3(3, 5, 2);
		mAabb[2].min = Vector3(2, -5, -5);
		mAabb[2].max = Vector3(5, -3, 15);

		mTime = 0;
		mRealTime = 0;
		mIdxChioce = 0;

		setupScene();

		glEnable( GL_CULL_FACE );
		glEnable( GL_DEPTH_TEST );

		glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_NICEST);

		::Global::GUI().cleanupWidget();


		WidgetUtility::createDevFrame();

		bInitialized = true;
		restart();
		return true;
	}

	void SampleStage::onInitFail()
	{
		Global::getDrawEngine()->stopOpenGL();
	}

	void SampleStage::onEnd()
	{
		Global::getDrawEngine()->stopOpenGL();
	}

	void SampleStage::onRender(float dFrame)
	{
		if( !bInitialized )
			return;

		if( !mGpuSync.pervRender() )
			return;

		GpuProfiler::getInstance().beginFrame();
		GameWindow& window = Global::getDrawEngine()->getWindow();

		ViewInfo view;
		assert(mCamera);	
		view.gameTime = mTime;
		view.realTime = mRealTime;
		view.setupTransform(mCamera->getViewMatrix() , mViewFrustum.getPerspectiveMatrix() );

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
			switch( mIdxChioce )
			{
			case 0: renderTest0(view); break;
			case 1: renderTest1(view); break;
			case 2: renderTest2(view); break;
			case 3: renderTest3(view); break;
			case 4: renderTest4(view); break;
			}
		}

		GpuProfiler::getInstance().endFrame();

		GLGraphics2D& g = ::Global::getGLGraphics2D();

		FixString< 512 > str;
		Vector3 v;
		g.beginRender();

		char const* title = "Unknown";
		switch( mIdxChioce )
		{
		case 0: title = "Sphere Plane"; break;
		case 1: title = "Parallax Mapping"; break;
		case 2: title = "Point Light shadow"; break;
		case 3: title = "Ray Test"; break;
		case 4: title = "Deferred Lighting"; break;
		}
		int const offset = 15;
		int y = 10;
		str.format("%s lightCount = %d", title , renderLightCount);
		g.drawText( 200 , y , str );
		str.format("CamPos = %.1f %.1f %.1f", view.worldPos.x, view.worldPos.y, view.worldPos.z);
		g.drawText(200, y += offset, str);
		for( int i = 0; i < GpuProfiler::getInstance().numSampleUsed; ++i )
		{
			GpuProfileSample* sample = GpuProfiler::getInstance().mSamples[i];
			str.format("%.2lf => %s", sample->time, sample->name.c_str());
			g.drawText(200 + 10 * sample->level , y += offset, str);

		}
		g.endRender();

		mGpuSync.postRender();
	}

	void SampleStage::render(ViewInfo& view, RenderParam& param)
	{
		param.beginRender( view );
		renderScene(param);
		param.endRender();
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
			mSpherePlane.draw();

			mProgSphere.setParam(SHADER_PARAM(Sphere.radius), 1.5f);
			mProgSphere.setParam(SHADER_PARAM(Sphere.localPos), Vector3(10, 3, 1.0f));
			mSpherePlane.draw();
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
			mMesh.draw();
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
			mPlane.draw();
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
		mShadowTech.drawShadowTexture( mLights[mNumLightDraw -1].type );
		//TechBase::drawCubeTexture(mTexSky);
	}

	void SampleStage::renderTest3(ViewInfo& view)
	{

		if ( mCamera != &mCamStorage[0] )
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

			DrawUtiltiy::CubeMesh();
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

	void SampleStage::renderTest4(ViewInfo& view)
	{
		{
			GPU_PROFILE("BasePass");
			mDefferredLightingTech.renderBassPass(view, *this );
		}
		if( 1 )
		{
			GPU_PROFILE("SSAO");
			mSSAO.render(view, mSceneRenderTargets);
		}
		if( 1 )
		{	
			GPU_PROFILE("Lighting");

			mDefferredLightingTech.prevRenderLights(view);

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

				mDefferredLightingTech.renderLight(view, light, shadowProjectParam );
			});
		}

		{
			GPU_PROFILE("ShowLigh");
			mSceneRenderTargets.getFrameBuffer().setDepth(mSceneRenderTargets.getDepthTexture());
			{
				GL_BIND_LOCK_OBJECT(mSceneRenderTargets.getFrameBuffer());
				showLight(view);
			}
			mSceneRenderTargets.getFrameBuffer().removeDepthBuffer();
		}


		if(1)
		{
			GPU_PROFILE("ShowBuffer");

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
				{
					ViewportSaveScope vpScope;
					int w = vpScope[2] / 4;
					int h = vpScope[3] / 4;
					//glViewport(0, h, 3 * w, 3 * h);
					ShaderHelper::getInstance().copyTextureToBuffer(mSceneRenderTargets.getRenderFrameTexture());
				}

				glDisable(GL_DEPTH_TEST);
				mSceneRenderTargets.getGBuffer().drawTextures( size , gapSize );

				mSSAO.drawSSAOTexture(Vec2i(0, 1).mul(size) + gapSize, renderSize);
				glEnable(GL_DEPTH_TEST);
			}
			else
			{
				ShaderHelper::getInstance().copyTextureToBuffer(mSceneRenderTargets.getRenderFrameTexture());

				glDisable(GL_DEPTH_TEST);
				mShadowTech.drawShadowTexture(mLights[mNumLightDraw - 1].type);
				glEnable(GL_DEPTH_TEST);
			}
		}
	}

	void SampleStage::showLight(ViewInfo& view)
	{
		GL_BIND_LOCK_OBJECT(mProgSphere);

		float radius = 0.15f;
		view.setupShader(mProgSphere);
		mProgSphere.setParam(SHADER_PARAM(VertexFactoryParams.worldToLocal), Matrix4::Identity());
		mProgSphere.setParam(SHADER_PARAM(Sphere.radius), radius);

		visitLights([this,&view, radius](int index, LightInfo const& light)
		{
			if( bUseFrustumTest && !light.testVisible(view) )
				return;

			mProgSphere.setParam(SHADER_PARAM(Sphere.localPos), light.pos);
			mProgSphere.setParam(SHADER_PARAM(Sphere.baseColor), light.color);
			mSpherePlane.draw();
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
		mSkyMesh.draw();
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
		if( !mesh.create(v, 8, idx, 24, true) )
			return false;
		mesh.mType = Mesh::eLineList;
		return true;
	}

	void SampleStage::calcViewRay(float x, float y)
	{
		Matrix4 matViewProj = mCamera->getViewMatrix() * mViewFrustum.getPerspectiveMatrix();
		Matrix4 matVPInv;
		float det;
		matViewProj.inverse(matVPInv, det);

		rayStart = TransformPosition(Vector3(x, y, -1), matVPInv);
		rayEnd = TransformPosition(Vector3(x, y, 1), matVPInv);
	}

	void SampleStage::tick()
	{
		mRealTime += float(gDefaultTickTime) / 10000;
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
		

	}

	bool SampleStage::onKey(unsigned key , bool isDown)
	{
		if ( !isDown )
			return false;

		switch( key )
		{
		case Keyboard::eR: restart(); break;
		case 'W': mCamera->moveFront( 0.5 ); break;
		case 'S': mCamera->moveFront( -0.5 ); break;
		case 'D': mCamera->moveRight( 0.5 ); break;
		case 'A': mCamera->moveRight( -0.5 ); break;
		case 'Z': mCamera->moveUp( 0.5 ); break;
		case 'X': mCamera->moveUp( -0.5 ); break;
		case Keyboard::eADD: mNumLightDraw = std::min(mNumLightDraw + 1, 4); break;
		case Keyboard::eSUBTRACT: mNumLightDraw = std::max(mNumLightDraw - 1, 0); break;
		case Keyboard::eB: mbShowBuffer = !mbShowBuffer; break;
		case Keyboard::eF3: 
			mLineMode = !mLineMode;
			glPolygonMode( GL_FRONT_AND_BACK , mLineMode ? GL_LINE : GL_FILL );
			break;
		case Keyboard::eM:
			ShaderHelper::getInstance().reload();
			reloadMaterials();
			break;
		case Keyboard::eL:
			mLights[0].bCastShadow = !mLights[0].bCastShadow;
			break;
		case Keyboard::eP: mPause = !mPause; break;
		case Keyboard::eF2:
			switch( mIdxChioce )
			{
			case 0: 
				ShaderManager::getInstance().reloadShader( mProgSphere ); 
				ShaderManager::getInstance().reloadShader (mProgPlanet ); 
				break;
			case 1: 
				ShaderManager::getInstance().reloadShader(mProgParallax); 
				break;
			case 2: mShadowTech.reload(); break;
			case 4: mDefferredLightingTech.reload(); mSSAO.reload(); break;
			}
			
			break;
		case Keyboard::eF5:
			if ( mCamera == &mCamStorage[0])
				mCamera = &mCamStorage[1];
			else
				mCamera = &mCamStorage[0];
			break;
		case Keyboard::eF6:
			mDefferredLightingTech.boundMethod = (LightBoundMethod)( ( mDefferredLightingTech.boundMethod + 1 ) % NumLightBoundMethod );
			break;
		case Keyboard::eF7:
			mDefferredLightingTech.debugMode = (DefferredLightingTech::DebugMode)( ( mDefferredLightingTech.debugMode + 1 ) % DefferredLightingTech::NumDebugMode );
			break;
		case Keyboard::eF8:
			bUseFrustumTest = !bUseFrustumTest;
			break;
		case Keyboard::eF4:
			break;
		case Keyboard::eNUM1: mIdxChioce = 0; break;
		case Keyboard::eNUM2: mIdxChioce = 1; break;
		case Keyboard::eNUM3: mIdxChioce = 2; break;
		case Keyboard::eNUM4: mIdxChioce = 3; break;
		case Keyboard::eNUM5: mIdxChioce = 4; break;
		}
		return false;
	}

	void SampleStage::reloadMaterials()
	{
		for( int i = 0; i < mMaterials.size(); ++i )
		{
			mMaterials[i].reload();
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
			Vec2f off = rotateSpeed * Vec2f( msg.getPos() - oldPos );
			mCamera->rotateByMouse( off.x , off.y );
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
				if ( testRayAABB( rayStart , rayEnd - rayStart , aabb.min , aabb.max , t ) )
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
		if ( !MeshUtility::createPlaneZ( mWaterMesh , 20 , 20 ) )
			return false;

		mReflectMap = new RHITexture2D;
		if ( !mReflectMap->create( Texture::eRGB32F , MapSize , MapSize ) )
			return false;

		mBuffer.addTexture( *mReflectMap , false );
		DepthRenderBuffer depthBuf;
		if ( ! depthBuf.create( MapSize , MapSize , Texture::eDepth32 ) )
			return false;
		mBuffer.setDepth( depthBuf , true );

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



