#include "TinyGamePCH.h"
#include "RenderGLStage.h"
#include "RenderGLStageAssetID.h"

#include "GameGUISystem.h"
#include "GLUtility.h"
#include "GLDrawUtility.h"
#include "GLGraphics2D.h"
#include "FileSystem.h"

#include "WidgetUtility.h"

#include "GL/wglew.h"


/*
#TODO:
-Tangent Matrix Bug
-Translucent Impl
-Shader Complier
-Game Client Type : Mesh -> Object Texture
-ForwardShading With Material
-Shadow PCF
-PostProcess flow Register
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

		::Global::getDrawEngine()->changeScreenSize(1600, 800);

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
		   !MeshUtility::createIcoSphere(mSphereMesh2, 2.5, 3) ||
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

		if( !mProgPlanet.loadFromSingleFile("Shader/PlanetSurface") )
			return false;

		if ( !mSpherePlane.createBuffer( &v[0] , 4  , &idx[0] , 6 , true ) )
			return false;

		if ( !mProgSphere.loadFromSingleFile( "Shader/Sphere" ) )
			return false;
		if ( !mEffectSphereSM.loadFromSingleFile( "Shader/Sphere" , "#define USE_SHADOW_MAP 1" ) )
			return false;

		if( !mProgBump.loadFromFile("Shader/Bump") )
			return false;
		if( !mProgParallax.loadFromSingleFile("Shader/Parallax") )
			return false;

		if( !mEffectSimple.loadFromSingleFile("Shader/Simple") )
			return false;


		if( !initFrameBuffer(screenSize) )
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

		if ( !mTexSky.loadFile( paths ) )
			return false;

		mTexSky.bind();
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
		mTexSky.unbind();

		if ( !mShadowTech.init() )
			return false;
		if( !mGBufferParamData.init(screenSize) )
			return false;
		if( !mDefferredLightingTech.init( mGBufferParamData ) )
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

		buildLights();

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

	void SampleStage::onRender(float dFrame)
	{
		if( !bInitialized )
			return;

		if( !mGpuSync.pervRender() )
			return;

		GameWindow& window = Global::getDrawEngine()->getWindow();

		ViewInfo view;
		assert(mCamera);	
		view.gameTime = mTime;
		view.realTime = mRealTime;
		view.setupTransform(mCamera->getViewMatrix() , mViewFrustum.getPerspectiveMatrix() );

		glMatrixMode(GL_PROJECTION);
		glLoadMatrixf( view.viewToClip );

		glMatrixMode(GL_MODELVIEW);

		glMatrixMode( GL_MODELVIEW );
		glLoadMatrixf( view.worldToView );

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

		GLGraphics2D& g = ::Global::getGLGraphics2D();

		FixString< 256 > str;
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
		g.drawText( 200 , 10 , title );
		g.endRender();

		mGpuSync.postRender();
	}

	void SampleStage::drawSky()
	{
		Matrix4 mat;
		glGetFloatv( GL_MODELVIEW_MATRIX , mat );

		glDisable( GL_DEPTH_TEST );
		glDepthMask( GL_FALSE );
		glEnable( GL_TEXTURE_CUBE_MAP );
		mTexSky.bind();

		glPushMatrix();

		mat.modifyTranslation( Vector3(0,0,0) );
		glLoadMatrixf( Matrix4::Rotate( Vector3(1,0,0), Math::Deg2Rad(90) ) * mat );
		glColor3f( 1 , 1 , 1 );
		mSkyMesh.draw();
		glPopMatrix();

		mTexSky.unbind();
		glDisable( GL_TEXTURE_CUBE_MAP );
		glDepthMask( GL_TRUE );
		glEnable( GL_DEPTH_TEST );
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

			view.bindParam(mProgSphere);
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
			view.bindParam(mProgParallax);

#if 0
			mProgParallax.setTexture(SHADER_PARAM(texBase), getTexture(TextureId::RocksD), 0);
			mProgParallax.setTexture(SHADER_PARAM(texNormal), getTexture(TextureId::RocksNH), 1);
#else
			mProgParallax.setTexture(SHADER_PARAM(texBase), getTexture(TextureId::Base), 0);
			mProgParallax.setTexture(SHADER_PARAM(texNormal), getTexture(TextureId::Normal), 1);
#endif

			mProgParallax.setParam(SHADER_PARAM(GLight.worldPosAndRadius), Vector4(mLights[1].pos, mLights[1].radius));
			mPlane.draw();
		}

		glDisable( GL_TEXTURE_2D );
	}


	void SampleStage::renderTest2(ViewInfo& view)
	{
		drawSky();
		auto fun = [this,&view](int index , LightInfo const& light)
		{
			mShadowTech.renderLighting(view, *this, light, index != 0);
		};
		visitLights(fun);
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

		view.bindParam(mEffectSimple);
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
		mDefferredLightingTech.renderBassPass( view , *this , getRenderFrameTexture() );
		
		{	
			glBlendFunc(GL_ONE, GL_ONE);
			auto fun = [this, &view](int index , LightInfo const& light)
			{
				glEnable(GL_DEPTH_TEST);
				ShadowProjectParam shadowProjectParam;
				shadowProjectParam.setupLight(light);
				mShadowTech.renderShadowDepth(view, *this, shadowProjectParam);
				glDisable(GL_DEPTH_TEST);
				{
					glEnable(GL_BLEND);
					GL_BIND_LOCK_OBJECT(mFrameBuffer);
					mDefferredLightingTech.renderLight(view, light, shadowProjectParam);
					glDisable(GL_BLEND);
				}
			};
			visitLights(fun);
		}
		
		mFrameBuffer.setDepth(mGBufferParamData.depthTexture);
		{
			GL_BIND_LOCK_OBJECT(mFrameBuffer);
			showLight( view);
		}
		mFrameBuffer.clearDepth();

		if ( mbShowBuffer )
		{
			{
				ViewportSaveScope vpScope;
				int w = vpScope[2] / 4;
				int h = vpScope[3] / 4;
				glViewport(0, h, 3 * w, 3 * h);
				ShaderHelper::getInstance().copyTextureToBuffer(getRenderFrameTexture());
			}

			glDisable(GL_DEPTH_TEST);
			mGBufferParamData.drawBuffer();
			glEnable(GL_DEPTH_TEST);
		}
		else
		{
			ShaderHelper::getInstance().copyTextureToBuffer(getRenderFrameTexture());

			glDisable(GL_DEPTH_TEST);
			mShadowTech.drawShadowTexture(mLights[mNumLightDraw - 1].type);
			glEnable(GL_DEPTH_TEST);
		}

	}

	void SampleStage::showLight(ViewInfo& view)
	{
		GL_BIND_LOCK_OBJECT(mProgSphere);
		view.bindParam(mProgSphere);
		mProgSphere.setParam(SHADER_PARAM(VertexFactoryParams.worldToLocal), Matrix4::Identity());
		mProgSphere.setParam(SHADER_PARAM(Sphere.radius), 0.2f);

		auto fun = [this](int index ,LightInfo const& light)
		{
			mProgSphere.setParam(SHADER_PARAM(Sphere.localPos), light.pos);
			mSpherePlane.draw();
		};
		visitLights(fun);
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
		case Keyboard::eP: mPause = !mPause; break;
		case Keyboard::eF2:
			switch( mIdxChioce )
			{
			case 0: mProgSphere.reload(); mProgPlanet.reload(); break;
			case 1: mProgParallax.reload(); break;
			case 2: mShadowTech.reload(); break;
			case 4: mDefferredLightingTech.reload(); break;
			}
			
			break;
		case Keyboard::eF5:
			if ( mCamera == &mCamStorage[0])
				mCamera = &mCamStorage[1];
			else
				mCamera = &mCamStorage[0];
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
			mMaterials[i]->reload();
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

	bool SampleStage::initFrameBuffer(Vec2i const& size)
	{
		mIdxRenderFrameTexture = 0;
		for( int i = 0; i < 2; ++i )
		{
			if( !mFrameTextures[i].create(Texture::eRGBA32F, size.x, size.y) )
				return false;
		}
		if( !mFrameBuffer.create() )
			return false;
		mFrameBuffer.addTexture(getRenderFrameTexture());
		return true;
	}

	bool ShadowDepthTech::init()
	{
		if ( !mShadowBuffer.create() )
			return false;

		if ( !mShadowMap.create( Texture::eRGBA32F , ShadowTextureSize , ShadowTextureSize ) )
			return false;


		mShadowMap.bind();
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
		mShadowMap.unbind();

		if( !mShadowMap2.create(Texture::eRGBA32F, ShadowTextureSize, ShadowTextureSize) )
			return false;

		if( !mCascadeTexture.create(Texture::eRGBA32F, CascadeTextureSize * CascadedShadowNum, CascadeTextureSize) )
			return false;

		int sizeX = Math::Max(CascadedShadowNum * CascadeTextureSize, ShadowTextureSize);
		int sizeY = Math::Max(CascadeTextureSize, ShadowTextureSize);
		
		if( !depthBuffer1.create(sizeX, sizeY, Texture::eDepth32F) )
			return false;
		if( !depthBuffer2.create(ShadowTextureSize, ShadowTextureSize, Texture::eDepth32F) )
			return false;

		mShadowMap2.bind();
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		mShadowMap2.unbind();

		mShadowBuffer.setDepth( depthBuffer1 );
		mShadowBuffer.addTexture(mShadowMap, Texture::eFaceX );
		//mBuffer.addTexture( mShadowMap2 );
		//mBuffer.addTexture( mShadowMap , Texture::eFaceX , false );
#if 0
		if (!mProgShadowDepth[0].loadFromSingleFile("Shader/ShadowMap" , "#define SHADOW_LIGHT_TYPE LIGHTTYPE_SPOT") )
			return false;
		if( !mProgShadowDepth[0].loadFromSingleFile("Shader/ShadowMap", "#define SHADOW_LIGHT_TYPE LIGHTTYPE_SPOT") )
			return false;
		if( !mProgShadowDepth[0].loadFromSingleFile("Shader/ShadowMap", "#define SHADOW_LIGHT_TYPE LIGHTTYPE_SPOT") )
			return false;

		if ( !mProgLighting.loadFromSingleFile( "Shader/ShadowLighting") )
			return false;
#endif

		depthParam[0] = 0.001;
		depthParam[1] = 100;

		return true;
	}

	void ShadowDepthTech::drawShadowTexture( LightType type )
	{
		ViewportSaveScope vpScope;

		Matrix4 porjectMatrix = OrthoMatrix(0, vpScope[2], 0, vpScope[3], -1, 1);

		MatrixSaveScope matrixScopt(porjectMatrix);

		int length = 200;
		switch( type )
		{
		case LightType::Spot:
			ShaderHelper::drawTexture(mShadowMap2, Vec2i(0, 0), Vec2i(length, length) );
			break;
		case LightType::Directional:
			ShaderHelper::drawTexture(mCascadeTexture, Vec2i(0, 0), Vec2i(length * CascadedShadowNum, length) );
			break;
		case LightType::Point:
			ShaderHelper::drawCubeTexture(mShadowMap, Vec2i(0, 0), length / 2 );
		default:
			break;
		}
	}

	void ShadowDepthTech::renderLighting(ViewInfo& view, SceneRender& scene, LightInfo const& light , bool bMultiple)
	{

		ShadowProjectParam shadowPrjectParam;
		shadowPrjectParam.setupLight(light);
		renderShadowDepth( view , scene , shadowPrjectParam );

		GL_BIND_LOCK_OBJECT(mProgLighting);
		//mProgLighting.setTexture(SHADER_PARAM(texSM) , mShadowMap2 , 0 );

		view.bindParam(mProgLighting);
		light.bindGlobalParam(mProgLighting);
	
		//mProgLighting.setParam(SHADER_PARAM(worldToLightView) , worldToLightView );
		mProgLighting.setTexture(SHADER_PARAM(ShadowTextureCube), mShadowMap);
		mProgLighting.setParam(SHADER_PARAM(DepthParam) , depthParam[0] , depthParam[1] );

		mEffectCur = &mProgLighting;
		if ( bMultiple )
		{
			glDepthFunc( GL_EQUAL );
			glEnable( GL_BLEND );
			glBlendFunc( GL_ONE , GL_ONE );
			scene.render( view , *this );
			glDisable( GL_BLEND );
			glDepthFunc( GL_LEQUAL);
		}
		else
		{
			scene.render( view , *this );
		}
		mEffectCur = nullptr;
	}

	static Vector3 GetUpDir(Vector3 const& dir)
	{
		Vector3 result = dir.cross(Vector3(0, 0, 1));
		if( dir.y * dir.y > 0.99998 )
			return Vector3(1, 0, 0);
		return dir.cross(Vector3(0, 0, 1)).cross(dir);
	}

	void ShadowDepthTech::renderShadowDepth(ViewInfo& view, SceneRender& scene ,  ShadowProjectParam& shadowProjectParam )
	{
		//glDisable( GL_CULL_FACE );
		LightInfo const& light = *shadowProjectParam.light;

		shadowParam = shadowProjectParam.shadowParam;

		ViewInfo lightView;
		lightView = view;

#if !USE_MATERIAL_SHADOW
		GL_BIND_LOCK_OBJECT(mProgShadowDepth);
		mEffectCur = &mProgShadowDepth[LIGHTTYPE_POINT];
		mEffectCur->setParam(SHADER_PARAM(DepthParam), depthParam[0], depthParam[1]);
#endif
		Matrix4 baisMatrix(
			0.5, 0, 0, 0,
			0, 0.5, 0, 0,
			0, 0, 0.5, 0,
			0.5, 0.5, 0.5, 1);
		//baisMatrix = Matrix4::Identity();
		Matrix4  worldToLight;
		Matrix4 shadowProject;
		if( light.type == LightType::Directional )
		{
			shadowProjectParam.numCascade = CascadedShadowNum;

			Vector3 farViewPos = (Vector4(0, 0, 1, 1) * view.clipToView).dividedVector();
			Vector3 nearViewPos = (Vector4(0, 0, -1, 1) * view.clipToView).dividedVector();
			float renderMaxDist = std::max( -mCascadeMaxDist - nearViewPos.z , farViewPos.z );

			determineCascadeSplitDist(nearViewPos.z, renderMaxDist, CascadedShadowNum, 0.5, shadowProjectParam.cacadeDepth);
			
			auto GetDepth = [&view](float value) ->float
			{
				Vector3 pos = (Vector4(0, 0, value, 1) * view.viewToClip).dividedVector();
				return pos.z;
			};

			shadowProjectParam.shadowTexture = &mCascadeTexture;
			mShadowBuffer.setDepth(depthBuffer1);
			mShadowBuffer.setTexture(0, mCascadeTexture);
			mShadowBuffer.bind();

			glClearDepth(1);
			glClearColor(1, 1, 1, 1.0);
			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
			for( int i = 0; i < CascadedShadowNum; ++i )
			{
				float delata = 1.0f / CascadedShadowNum;
				float depthParam[2];

				if( i == 0 )
					depthParam[0] = GetDepth(nearViewPos.z);
				else
					depthParam[0] = GetDepth(shadowProjectParam.cacadeDepth[i-1]);
				depthParam[1] = GetDepth(shadowProjectParam.cacadeDepth[i]);

				calcCascadeShadowProjectInfo(view, light, depthParam, worldToLight, shadowProject);

				MatrixSaveScope matScope(shadowProject);

				ViewportSaveScope vpScope;
				glViewport(i*CascadeTextureSize, 0, CascadeTextureSize, CascadeTextureSize);

				lightView.setupTransform(worldToLight, shadowProject);

				Matrix4 corpMatrx = Matrix4(
					delata , 0 , 0 , 0 ,
					0 , 1 , 0 , 0,
					0 , 0 , 1 , 0,
					i * delata , 0 , 0 , 1);
				mShadowMatrix = worldToLight * shadowProject * baisMatrix;

				shadowProjectParam.shadowMatrix[i] = mShadowMatrix * corpMatrx;
				
				glLoadMatrixf(worldToLight);
				scene.render(lightView, *this);	
			}
			mShadowBuffer.unbind();
		}
		else if( light.type == LightType::Spot )
		{
			shadowProject = PerspectiveMatrix(Math::Deg2Rad(2.0 * Math::Min(89.99 , light.spotAngle.y)), 1.0, 0.01, light.radius);
			MatrixSaveScope matScope(shadowProject);

			worldToLight = LookAtMatrix(light.pos, light.dir, GetUpDir(light.dir));
			mShadowMatrix = worldToLight * shadowProject * baisMatrix;
			shadowProjectParam.shadowMatrix[0] = mShadowMatrix;

			shadowProjectParam.shadowTexture = &mShadowMap2;
			mShadowBuffer.setDepth(depthBuffer2);
			mShadowBuffer.setTexture(0, mShadowMap2);
			GL_BIND_LOCK_OBJECT(mShadowBuffer);
			
			ViewportSaveScope vpScope;
			glViewport(0, 0, ShadowTextureSize, ShadowTextureSize);
			glEnable(GL_DEPTH_TEST);
			glClearDepth(1);
			glClearColor(1, 1, 1, 1.0);
			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

			lightView.setupTransform(worldToLight, shadowProject);
			glLoadMatrixf(worldToLight);
			scene.render(lightView, *this);

		}
		else if( light.type == LightType::Point )
		{
			shadowProject = PerspectiveMatrix(Math::Deg2Rad(90), 1.0, 0.01, light.radius);
			MatrixSaveScope matScope(shadowProject);

			Matrix4 baisMatrix(
				1, 0, 0, 0,
				0, 1, 0, 0,
				0, 0, 0.5, 0,
				0, 0, 0.5, 1);
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

			shadowProjectParam.shadowTexture = &mShadowMap;

			ViewportSaveScope vpScope;
			glViewport(0, 0, ShadowTextureSize, ShadowTextureSize);
			mShadowBuffer.setDepth(depthBuffer2);
			for( int i = 0; i < 6; ++i )
			{
				mShadowBuffer.setTexture(0, mShadowMap, Texture::Face(i));
				GL_BIND_LOCK_OBJECT(mShadowBuffer);
				glEnable(GL_DEPTH_TEST);
				glClearDepth(1);
				glClearColor(1, 1, 1, 1.0);
				glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
				worldToLight = LookAtMatrix(light.pos, CubeFaceDir[i], CubeUpDir[i]);
				mShadowMatrix = worldToLight * shadowProject * baisMatrix;
				shadowProjectParam.shadowMatrix[i] = mShadowMatrix;

				lightView.setupTransform(worldToLight, shadowProject);
				glLoadMatrixf(worldToLight);
				scene.render(lightView, *this);
			}

		}

	}


	void ShadowDepthTech::determineCascadeSplitDist(float nearDist, float farDist, int numCascade, float lambda, float outDist[])
	{
		for( int i = 1; i <= numCascade; ++i )
		{
			float Clog = nearDist * Math::Pow(farDist / nearDist, float(i) / numCascade);
			float Cuni = nearDist + (farDist - nearDist) * (float(i) / numCascade);
			outDist[i-1] = Math::Lerp(Clog, Cuni, lambda);
		}
	}


	void ShadowDepthTech::calcCascadeShadowProjectInfo(ViewInfo &view, LightInfo const &light, float depthParam[2], Matrix4 &worldToLight, Matrix4 &shadowProject)
	{
		Vector3 boundPos[8];
		int idx = 0;
		for( int i = -1; i <= 1; i += 2 )
		{
			for( int j = -1; j <= 1; j += 2 )
			{
				for( int k = 0; k < 2; ++k )
				{
					Vector4 v(i, j, depthParam[k], 1);
					v = v * view.clipToWorld;
					boundPos[idx] = v.dividedVector();
					++idx;
				}
			}
		}
		Vector3 zAxis = normalize(-light.dir);
		Vector3 viewDir = view.getViewForwardDir();
		Vector3 xAxis = viewDir.cross(zAxis);
		if( xAxis.length2() < 1e-4 )
		{
			xAxis = view.getViewRightDir();
		}
		xAxis.normalize();
		Vector3 yAxis = zAxis.cross(xAxis);
		yAxis.normalize();
		float MaxLen = 10000000;
		Vector3 Vmin = Vector3(MaxLen, MaxLen, MaxLen);
		Vector3 Vmax = -Vmin;
		for( int i = 0; i < 8; ++i )
		{
			Vector3 v;
			v.x = xAxis.dot(boundPos[i]);
			v.y = yAxis.dot(boundPos[i]);
			v.z = zAxis.dot(boundPos[i]);
			Vmin.min(v);
			Vmax.max(v);
		}

		Vector3 Vmid = 0.5 * (Vmax + Vmin);
		Vector3 viewPos = Vmid.x * xAxis + Vmid.y * yAxis;
		Vector3 fBoundVolume = Vmax - Vmin;

		viewPos = Vector3::Zero();
		worldToLight = LookAtMatrix(viewPos, light.dir, yAxis);
		shadowProject = OrthoMatrix( Vmin.x, Vmax.x , Vmin.y, Vmax.y, -mCascadeMaxDist / 2 , mCascadeMaxDist / 2);
	}

	bool WaterTech::init()
	{
		if ( !mBuffer.create() )
			return false;
		if ( !MeshUtility::createPlaneZ( mWaterMesh , 20 , 20 ) )
			return false;
		if ( !mReflectMap.create( Texture::eRGB32F , MapSize , MapSize ) )
			return false;

		mBuffer.addTexture( mReflectMap , false );
		DepthRenderBuffer depthBuf;
		if ( ! depthBuf.create( MapSize , MapSize , Texture::eDepth32 ) )
			return false;
		mBuffer.setDepth( depthBuf , true );

		return true;
	}


	bool DefferredLightingTech::init(GBufferParamData& inGBufferParamData)
	{
		mGBufferParamData = &inGBufferParamData;

		GameWindow& gameWindow = ::Global::getDrawEngine()->getWindow();

		Vec2i size = ::Global::getDrawEngine()->getScreenSize();

		if( !mBuffer.create() )
			return false;

		mBuffer.addTexture(mGBufferParamData->textures[0]);
		for( int i = 0; i < GBufferParamData::NumBuffer; ++i )
		{
			mBuffer.addTexture(mGBufferParamData->textures[i]);
		}

		mBuffer.setDepth(mGBufferParamData->depthTexture);

		if( !mProgLighting[(int)LightType::Point].loadFromSingleFile(
			"Shader/DeferredLighting",
			SHADER_ENTRY(ScreenVS), SHADER_ENTRY(LightingPassPS), 
			"#define DEFERRED_LIGHT_TYPE LIGHTTYPE_POINT") )
			return false;

		if( !mProgLighting[(int)LightType::Spot].loadFromSingleFile(
			"Shader/DeferredLighting",
			SHADER_ENTRY(ScreenVS), SHADER_ENTRY(LightingPassPS),
			"#define DEFERRED_LIGHT_TYPE LIGHTTYPE_SPOT") )
			return false;

		if( !mProgLighting[(int)LightType::Directional].loadFromSingleFile(
			"Shader/DeferredLighting",
			SHADER_ENTRY(ScreenVS), SHADER_ENTRY(LightingPassPS),
			"#define DEFERRED_LIGHT_TYPE LIGHTTYPE_DIRECTIONAL") )
			return false;

		return true;
	}

	void DefferredLightingTech::renderBassPass(ViewInfo& view, SceneRender& scene , Texture2DRHI& frameTexture )
	{
		glEnable(GL_DEPTH_TEST);
		mBuffer.setTexture( 0 , frameTexture );
		GL_BIND_LOCK_OBJECT(mBuffer);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		scene.render(view, *this);
	}

	void DefferredLightingTech::renderLight(ViewInfo& view, LightInfo const& light , ShadowProjectParam const& shadowProjectParam)
	{
		glDisable(GL_DEPTH_TEST);
		{
			//MatrixSaveScope matrixScope(Matrix4::Identity());
			ShaderProgram& pargam = mProgLighting[(int)light.type];
			GL_BIND_LOCK_OBJECT(pargam);

			mGBufferParamData->bindParam(pargam, true );
			shadowProjectParam.bindParam(pargam);
			view.bindParam(pargam);
			light.bindGlobalParam(pargam);

			DrawUtiltiy::ScreenRect();

			glBindTexture(GL_TEXTURE_CUBE_MAP, 0);
		}
		glEnable(GL_DEPTH_TEST);
	}

	bool GBufferParamData::init(Vec2i const& size)
	{
		for( int i = 0; i < NumBuffer; ++i )
		{
			if( !textures[i].create(Texture::eRGBA32F, size.x, size.y) )
				return false;
		}

		if( !depthTexture.create(Texture::eDepth32F, size.x, size.y) )
			return false;

		return true;
	}

	void GBufferParamData::bindParam(ShaderProgram& program, bool bUseDepth )
	{
		if( bUseDepth )
			program.setTexture(SHADER_PARAM(FrameDepthTexture), depthTexture);

		program.setTexture( SHADER_PARAM(GBufferTextureA), textures[BufferA]);
		program.setTexture( SHADER_PARAM(GBufferTextureB), textures[BufferB]);
		program.setTexture( SHADER_PARAM(GBufferTextureC), textures[BufferC]);
		program.setTexture( SHADER_PARAM(GBufferTextureD), textures[BufferD]);

	}

	void GBufferParamData::drawBuffer()
	{
		ViewportSaveScope vpScope;
		OrthoMatrix matProj(0, vpScope.value[2], 0, vpScope.value[3], -1, 1);

		MatrixSaveScope matScope(matProj);

		int width = vpScope.value[2] / 4;
		int height = vpScope.value[3] / 4;

		drawTexture(0 * width, 0 * height, width, height, BufferA);
		drawTexture(1 * width, 0 * height, width, height, BufferB);
		drawTexture(2 * width, 0 * height, width, height, BufferC);
		drawTexture(3 * width, 3 * height, width, height, BufferD, Vector4(1,0,0,0));
		drawTexture(3 * width, 2 * height, width, height, BufferD, Vector4(0,1,0,0));
		drawTexture(3 * width, 1 * height, width, height, BufferD, Vector4(0,0,1,0));
		{
			ViewportSaveScope vpScope;
			glViewport(3 * width, 0 * height, width, height);
			float valueFactor[2] = { 255 , 0 };
			ShaderHelper::getInstance().mapTextureColorToBuffer(textures[BufferD], Vector4(0, 0, 0, 1) , valueFactor );
		}
		//renderDepthTexture(width , 3 * height, width, height);
	}

	void GBufferParamData::drawDepthTexture(int x, int y, int width, int height)
	{
		//PROB
		glLoadIdentity();
		glEnable(GL_TEXTURE_2D);
		glBindTexture(GL_TEXTURE_2D, depthTexture.mHandle);
		glColor3f(1, 1, 1);
		DrawUtiltiy::Rect(x, y, width, height);
		glBindTexture(GL_TEXTURE_2D, 0);
		glDisable(GL_TEXTURE_2D);
	}

	void GBufferParamData::drawTexture(int x, int y, int width, int height, int idxBuffer )
	{
		ShaderHelper::drawTexture(textures[idxBuffer], Vec2i(x, y), Vec2i(width, height) );
	}

	void GBufferParamData::drawTexture(int x, int y, int width, int height, int idxBuffer, Vector4 const& colorMask)
	{
		ViewportSaveScope vpScope;
		glViewport(x, y, width, height);
		ShaderHelper::getInstance().copyTextureMaskToBuffer(textures[idxBuffer], colorMask);
	}

	bool ShaderHelper::init()
	{
		if( !mProgCopyTexture.loadFromSingleFile(
			"Shader/CopyTexture", 
			SHADER_ENTRY(CopyTextureVS), SHADER_ENTRY(CopyTexturePS) ) )
			return false;

		if( !mProgCopyTextureMask.loadFromSingleFile(
			"Shader/CopyTexture", 
			SHADER_ENTRY(CopyTextureVS), SHADER_ENTRY(CopyTextureMaskPS) ) )
			return false;

		if( !mProgMappingTextureColor.loadFromSingleFile(
			"Shader/CopyTexture", 
			SHADER_ENTRY(CopyTextureVS) , SHADER_ENTRY(MappingTextureColorPS) ) )
			return false;

		return true;
	}

	void ShaderHelper::drawCubeTexture(TextureCubeRHI& texCube , Vec2i const& pos , int length )
	{
		glEnable(GL_TEXTURE_CUBE_MAP);
		GL_BIND_LOCK_OBJECT(texCube);
		glColor3f(1, 1, 1);
		int offset = 10;

		if ( length == 0 )
			length = 100;

		glPushMatrix();
		glTranslatef(pos.x, pos.y, 0);
		glTranslatef(1 * length, 1 * length, 0);
		glBegin(GL_QUADS); //x
		glTexCoord3f(1, -1, -1); glVertex2f(0, 0);
		glTexCoord3f(1, 1, -1);  glVertex2f(length, 0);
		glTexCoord3f(1, 1, 1);  glVertex2f(length, length);
		glTexCoord3f(1, -1, 1);  glVertex2f(0, length);
		glEnd();
		glTranslatef(2 * length, 0, 0);
		glBegin(GL_QUADS); //-x
		glTexCoord3f(-1, 1, -1); glVertex2f(0, 0);
		glTexCoord3f(-1, -1, -1);  glVertex2f(length, 0);
		glTexCoord3f(-1, -1, 1); glVertex2f(length, length);
		glTexCoord3f(-1, 1, 1);  glVertex2f(0, length);
		glEnd();
		glTranslatef(-1 * length, 0, 0);
		glBegin(GL_QUADS); //y
		glTexCoord3f(1, 1, -1); glVertex2f(0, 0);
		glTexCoord3f(-1, 1, -1);glVertex2f(length, 0);
		glTexCoord3f(-1, 1, 1); glVertex2f(length, length);
		glTexCoord3f(1, 1, 1);  glVertex2f(0, length);
		glEnd();
		glTranslatef(-2 * length, 0, 0);
		glBegin(GL_QUADS); //-y
		glTexCoord3f(-1, -1, -1); glVertex2f(0, 0);
		glTexCoord3f(1, -1, -1); glVertex2f(length, 0);
		glTexCoord3f(1, -1, 1); glVertex2f(length, length);
		glTexCoord3f(-1, -1, 1); glVertex2f(0, length);
		glEnd();
		glTranslatef(1 * length, 1 * length, 0);
		glBegin(GL_QUADS); //z
		glTexCoord3f(1, -1, 1); glVertex2f(0, 0);
		glTexCoord3f(1, 1, 1); glVertex2f(length, 0);
		glTexCoord3f(-1, 1, 1);  glVertex2f(length, length);
		glTexCoord3f(-1, -1, 1);  glVertex2f(0, length);
		glEnd();
		glTranslatef(0 * length, -2 * length, 0);
		glBegin(GL_QUADS); //-z
		glTexCoord3f(-1, -1, -1);  glVertex2f(0, 0);
		glTexCoord3f(-1, 1, -1);  glVertex2f(length, 0);
		glTexCoord3f(1, 1, -1);  glVertex2f(length, length);
		glTexCoord3f(1, -1, -1); glVertex2f(0, length);
		glEnd();

		glPopMatrix();
		glDisable(GL_TEXTURE_CUBE_MAP);
	}


	void ShaderHelper::drawTexture(Texture2DRHI& texture, Vec2i const& pos, Vec2i const& size)
	{
		glLoadIdentity();
		glEnable(GL_TEXTURE_2D);
		texture.bind();
		glColor3f(1, 1, 1);
		DrawUtiltiy::Rect(pos.x, pos.y, size.x, size.y);
		texture.unbind();
		glDisable(GL_TEXTURE_2D);
	}

	void ShaderHelper::copyTextureToBuffer(Texture2DRHI& copyTexture)
	{
		GL_BIND_LOCK_OBJECT(mProgCopyTexture);
		mProgCopyTexture.setTexture(SHADER_PARAM(CopyTexture), copyTexture);
		DrawUtiltiy::ScreenRect();
	}

	void ShaderHelper::copyTextureMaskToBuffer(Texture2DRHI& copyTexture, Vector4 const& colorMask)
	{
		GL_BIND_LOCK_OBJECT(mProgCopyTextureMask);
		mProgCopyTextureMask.setTexture(SHADER_PARAM(CopyTexture), copyTexture);
		mProgCopyTextureMask.setParam(SHADER_PARAM(ColorMask), colorMask);
		DrawUtiltiy::ScreenRect();
	}

	void ShaderHelper::mapTextureColorToBuffer(Texture2DRHI& copyTexture, Vector4 const& colorMask, float valueFactor[2])
	{
		GL_BIND_LOCK_OBJECT(mProgMappingTextureColor);
		mProgMappingTextureColor.setTexture(SHADER_PARAM(CopyTexture), copyTexture);
		mProgMappingTextureColor.setParam(SHADER_PARAM(ColorMask), colorMask);
		mProgMappingTextureColor.setParam(SHADER_PARAM(ValueFactor), valueFactor[0], valueFactor[1]);
		DrawUtiltiy::ScreenRect();
	}

	void ShaderHelper::copyTexture(Texture2DRHI& destTexture, Texture2DRHI& srcTexture)
	{
		static FrameBuffer frameBuffer;
		static bool bInit = false;
		if( bInit == false )
		{
			frameBuffer.create();
			frameBuffer.addTexture(destTexture);
			bInit = true;
		}
		else
		{
			frameBuffer.setTexture(0, destTexture);
		}

		frameBuffer.bind();
		copyTextureToBuffer(srcTexture);
		frameBuffer.unbind();
	}

	void ShaderHelper::reload()
	{
		mProgCopyTexture.reload();
		mProgCopyTextureMask.reload();
		mProgMappingTextureColor.reload();
	}

	bool Material::loadInternal()
	{
		std::string path("Shader/Material/");
		path += mName;
		path += ".glsl";

		std::vector< char > materialCode;
		if( !FileUtility::LoadToBuffer(path.c_str(), materialCode) )
			return false;

		if( !mShader.loadFromSingleFile(
			"Shader/DeferredBasePass",
			SHADER_ENTRY(BassPassVS), SHADER_ENTRY(BasePassPS), &materialCode[0]) )
			return false;

		if( !mShadowShader.loadFromSingleFile(
			"Shader/ShadowDepthRender",
			SHADER_ENTRY(MainVS), SHADER_ENTRY(MainPS), &materialCode[0]) )
			return false;

		return true;
	}

	void Material::bindShaderParam(ShaderProgram& shader)
	{
		for( int i = 0; i < (int)mParams.size(); ++i )
		{
			ParamSlot& param = mParams[i];

			if( param.offset == -1 )
				continue;

			switch( param.type )
			{
			case ParamType::eTexture2DRHI:
				{
					TextureBaseRHI* texture = reinterpret_cast<TextureBaseRHI*>(&mParamDataStorage[param.offset]);
					shader.setTexture(param.name, *(Texture2DRHI*)texture);
				}
				break;
			case ParamType::eTexture3DRHI:
				{
					TextureBaseRHI* texture = reinterpret_cast<TextureBaseRHI*>(&mParamDataStorage[param.offset]);
					shader.setTexture(param.name, *(Texture3DRHI*)texture);
				}
				break;
			case ParamType::eTextureCubeRHI:
				{
					TextureBaseRHI* texture = reinterpret_cast<TextureBaseRHI*>(&mParamDataStorage[param.offset]);
					shader.setTexture(param.name, *(TextureCubeRHI*)texture);
				}
				break;
			case ParamType::eTextureDepthRHI:
				{
					TextureBaseRHI* texture = reinterpret_cast<TextureBaseRHI*>(&mParamDataStorage[param.offset]);
					shader.setTexture(param.name, *(TextureDepthRHI*)texture);
				}
				break;

			case ParamType::eMatrix4:
				shader.setParam(param.name, *reinterpret_cast<Matrix4*>(&mParamDataStorage[param.offset]));
				break;
			case ParamType::eMatrix3:
				shader.setParam(param.name, *reinterpret_cast<Matrix3*>(&mParamDataStorage[param.offset]));
				break;
			case ParamType::eVector:
				shader.setParam(param.name, *reinterpret_cast<Vector4*>(&mParamDataStorage[param.offset]));
				break;
			case ParamType::eScale:
				shader.setParam(param.name, *reinterpret_cast<float*>(&mParamDataStorage[param.offset]));
				break;
			}
		}
	}

	Material::ParamSlot& Material::fetchParam(char const* name, ParamType type)
	{
		HashString hName = name;
		for( int i = 0; i < mParams.size(); ++i )
		{
			if( mParams[i].name == hName && mParams[i].type == type )
				return mParams[i];
		}

		ParamSlot param;
		param.name = hName;
		param.type = type;
		int typeSize = 0;
		switch( type )
		{
		case ParamType::eTexture2DRHI:
		case ParamType::eTexture3DRHI:
		case ParamType::eTextureCubeRHI:
		case ParamType::eTextureDepthRHI:
			typeSize = sizeof(TextureBaseRHI*); 
			break;
		case ParamType::eMatrix4: typeSize = 16 * sizeof(float); break;
		case ParamType::eMatrix3: typeSize = 9 * sizeof(float); break;
		case ParamType::eVector:  typeSize = 4 * sizeof(float); break;
		case ParamType::eScale:   typeSize = 1 * sizeof(float); break;
		}

		if( typeSize )
		{
			param.offset = mParamDataStorage.size();
			mParamDataStorage.resize(mParamDataStorage.size() + typeSize, 0);
		}
		else
		{
			param.offset = -1;
		}

		mParams.push_back(param);
		return mParams.back();
	}

	bool GpuSync::pervRender()
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

	void GpuSync::postRender()
	{
		if( bUseFence )
		{
			GLbitfield flags = 0;
			GLenum condition = GL_SYNC_GPU_COMMANDS_COMPLETE;
			renderSync = glFenceSync(condition, flags);
		}
	}

	bool GpuSync::pervLoading()
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

	void GpuSync::postLoading()
	{
		if( bUseFence )
		{
			GLbitfield flags = 0;
			GLenum condition = GL_SYNC_GPU_COMMANDS_COMPLETE;
			loadingSync = glFenceSync(condition, flags);
		}
	}

}//namespace RenderGL



