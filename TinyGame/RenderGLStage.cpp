#include "TinyGamePCH.h"
#include "RenderGLStage.h"

#include "GameGUISystem.h"
#include "GLUtility.h"
#include "GLDrawUtility.h"
#include "GLGraphics2D.h"

#include "WidgetUtility.h"

#include "GL/wglew.h"
namespace RenderGL
{
	SampleStage::SampleStage()
	{
		mPause = false;

	}

	SampleStage::~SampleStage()
	{

	}

	bool SampleStage::onInit()
	{
	
		if ( !Global::getDrawEngine()->startOpenGL() )
			return false;

		GameWindow& window = Global::getDrawEngine()->getWindow();

		Vec2i screenSize = ::Global::getDrawEngine()->getScreenSize();

		::Global::GUI().cleanupWidget();
		{
			Vector3 n(1,1,0);
			float factor = n.normalize();
			ReflectMatrix m( n , 1.0 * factor );
			Vector3 v = Vector3(1,0,0) * m;
			int i = 1;
		}


		if ( glewInit() != GLEW_OK )
			return false;

		wglSwapIntervalEXT(0);

		if ( !MeshUtility::createPlaneZ( mPlane , 10 , 3 ) ||
			 !MeshUtility::createTile( mMesh , 64 , 1.0f ) ||
			 !MeshUtility::createUVSphere( mSphereMesh , 2.5 , 60 , 60 ) ||
			 !MeshUtility::createIcoSphere( mSphereMesh2 , 2.5 , 3 ) ||
			 !MeshUtility::createCube( mBoxMesh ) || 
			 !MeshUtility::createSkyBox( mSkyMesh ) || 
			 !MeshUtility::createDoughnut( mDoughnutMesh , 2 , 1 , 60 , 60 ) )
			return false;

		if ( !mProgPlanet.loadFromSingleFile( "Shader/PlanetSurface" ) )
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

		if ( !mSpherePlane.createBuffer( &v[0] , 4  , &idx[0] , 6 , true ) )
			return false;

		if ( !mProgSphere.loadFromSingleFile( "Shader/Sphere" ) )
			return false;
		if ( !mEffectSphereSM.loadFromSingleFile( "Shader/Sphere" , "#define USE_SHADOW_MAP 1" ) )
			return false;
		{



#if 0
			if ( !mTex.loadFile( "Texture/rocks.png") )
				return false;
			if ( !mTexNormal.loadFile( "Texture/rocks_NM_height.tga"))
				return false;
			//if ( !mTexNormal.loadFile( "Texture/CircleN.tga"))
				//return false;
#else
			if ( !mTexBase.loadFile( "Texture/tile1.png") )
				return false;
			if ( !mTexNormal.loadFile( "Texture/tile1.tga"))
				return false;
#endif
			if ( !mProgBump.loadFromFile( "Shader/Bump") )
				return false;
			if ( !mProgParallax.loadFromSingleFile( "Shader/Parallax") )
				return false;

			if ( !mEffectSimple.loadFromSingleFile( "Shader/Simple") )
				return false;

		}

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
		if( !mDefferredLightingTech.init() )
			return false;

		mViewFrustum.mNear = 0.1;
		mViewFrustum.mFar  = 1000.0;
		mViewFrustum.mAspect = float(screenSize.x) / screenSize.y;
		mViewFrustum.mYFov   = Math::Deg2Rad( 60 / mViewFrustum.mAspect );

		if ( !createFrustum( mFrustumMesh , mViewFrustum.getPerspectiveMatrix() ) )
			return false;

		mAabb[0].min = Vector3(1,1,1);
		mAabb[0].max = Vector3(5,4,7);
		mAabb[1].min = Vector3(-5,-5,-5);
		mAabb[1].max = Vector3(3,5,2);
		mAabb[2].min = Vector3(2,-5,-5);
		mAabb[2].max = Vector3(5,-3,15);

		mCamera = &mCamStorage[0];

		mCamera->setPos( Vector3( 0 , -10 , 2 ) );
		mTime = 0;
		mIdxChioce = 0;



		glEnable( GL_CULL_FACE );
		glEnable( GL_DEPTH_TEST );

		glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_NICEST);

		WidgetUtility::createDevFrame();

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

	void SampleStage::drawAxis( float len )
	{	
		glLineWidth( 1.5 );
		glBegin( GL_LINES );
		glColor3f(1,0,0);glVertex3f(0,0,0);glVertex3f(len,0,0);
		glColor3f(0,1,0);glVertex3f(0,0,0);glVertex3f(0,len,0);
		glColor3f(0,0,1);glVertex3f(0,0,0);glVertex3f(0,0,len);
		glEnd();
		glLineWidth( 1 );
	}

	bool gUseMatrix = true;
	void SampleStage::onRender(float dFrame)
	{
		GameWindow& window = Global::getDrawEngine()->getWindow();

		glMatrixMode(GL_PROJECTION);
		
		if ( gUseMatrix )
		{
			glLoadMatrixf( mViewFrustum.getPerspectiveMatrix() );
		}
		else
		{
			glLoadIdentity();
			//glOrtho(0, window.getWidth() , 0 , window.getHeight() , -10000 , 100000 );
			gluPerspective( Math::Rad2Deg( mViewFrustum.mYFov ) , mViewFrustum.mAspect , mViewFrustum.mNear , mViewFrustum.mFar );
		}

		glMatrixMode(GL_MODELVIEW);


		glClearColor( 0 , 0 , 0 , 0 );
		glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );

		glMatrixMode( GL_MODELVIEW );
		glLoadMatrixf( mCamera->getViewMatrix() );

		//drawAxis( 20 );

		ViewInfo view;
		view.camera = mCamera;
		view.matView = mCamera->getViewMatrix();
		float det;
		view.matView.inverse(view.matViewInv, det);

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

	void SampleStage::render(ViewInfo& view, RenderParam& param)
	{
		renderScene(param);
	}

	void SampleStage::copyTexture(Texture2D& destTexture, Texture2D& srcTexture)
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
			frameBuffer.setTexture(0 , destTexture);
		}
		
		frameBuffer.bind();
		copyTextureToBuffer(srcTexture);
		frameBuffer.unbind();
	}

	void SampleStage::copyTextureToBuffer(Texture2D& copyTexture)
	{
		mProgCopyTexture.bind();
		mProgCopyTexture.setTexture("CopyTexture", copyTexture);
		DrawUtiltiy::ScreenRect();
		mProgCopyTexture.unbind();
	}

	void SampleStage::renderScene(RenderParam& param)
	{

		Matrix4 matWorld;

		glPushMatrix();
		matWorld = Matrix4::Translate( Vector3(0,0,7) );
		param.setWorld( matWorld  );
		glColor3f( 1 , 1 , 0 );
		mSphereMesh2.draw();
		glPopMatrix();


		glPushMatrix();
		matWorld = Matrix4::Rotate( Vector3(1,1,1) , Math::Deg2Rad(45) ) * Matrix4::Translate( Vector3(3,-3,7) );
		param.setWorld( matWorld  );
		glColor3f( 0.3 , 0.3 , 1 );
		mBoxMesh.draw();
		glPopMatrix();

		glPushMatrix();
		matWorld = Matrix4::Translate( Vector3(-3,3,5) );
		param.setWorld( matWorld  );
		glColor3f( 0.3 , 0.3 , 1 );
		mBoxMesh.draw();
		glPopMatrix();

		glPushMatrix();
		matWorld = Matrix4::Scale( 1 ) * Matrix4::Translate( Vector3(1,3,11) );
		param.setWorld( matWorld );
		glColor3f( 1 , 1 , 0 );
		mDoughnutMesh.draw();
		glPopMatrix();

		glPushMatrix();
		matWorld = Matrix4::Scale( 1 ) * Matrix4::Translate( Vector3(6,6,5) );
		param.setWorld( matWorld );
		glColor3f( 1 , 1 , 0 );
		mBoxMesh.draw();
		glPopMatrix();


		glPushMatrix();
		matWorld = Matrix4::Scale( 10.0 ) * Matrix4::Translate( Vector3(0,0,0) );
		param.setWorld( matWorld  );
		glColor3f( 1 , 1 , 0 );
		mPlane.draw();
		glPopMatrix();


		float scale = 1.5;
		float len = 10 * scale + 1;
		glPushMatrix();
		matWorld = Matrix4::Rotate( Vector3(1,0,0) , Math::Deg2Rad( 90 ) ) * Matrix4::Scale( scale ) * Matrix4::Translate( Vector3(0,len,len) );
		param.setWorld( matWorld );
		glColor3f( 0.5 , 1 , 0 );
		mPlane.draw();
		glPopMatrix();


		glPushMatrix();
		matWorld = Matrix4::Rotate( Vector3(1,0,0) , Math::Deg2Rad( -90 ) ) * Matrix4::Scale( scale ) * Matrix4::Translate( Vector3(0,-len,len) );
		param.setWorld( matWorld );
		glColor3f( 1 , 0.5 , 1 );
		mPlane.draw();
		glPopMatrix();

		glPushMatrix();
		matWorld = Matrix4::Rotate( Vector3(0,1,0) , Math::Deg2Rad( 90 ) ) * Matrix4::Scale( scale ) * Matrix4::Translate( Vector3(-len,0,len) );
		param.setWorld( matWorld );
		glColor3f( 1 , 1 , 0.5 );
		mPlane.draw();
		glPopMatrix();

		glPushMatrix();
		matWorld = Matrix4::Rotate( Vector3(0,1,0) , Math::Deg2Rad( -90 ) ) * Matrix4::Scale( scale ) * Matrix4::Translate( Vector3(len,0,len) );
		param.setWorld( matWorld );
		glColor3f( 1 , 1 , 1 );
		mPlane.draw();
		glPopMatrix();

		glPushMatrix();
		matWorld = Matrix4::Rotate( Vector3(0.5,1,0) , Math::Deg2Rad( -80 ) ) * Matrix4::Scale( 3 ) * Matrix4::Translate( Vector3( 0.5 * len ,0, len) );
		param.setWorld( matWorld );
		glColor3f( 1 , 1 , 1 );
		mBoxMesh.draw();
		glPopMatrix();

		//glPushMatrix();
		//mProgSphere.bind();
		//mProgSphere.setParam( "sphere.radius" , 2.0f );
		//mProgSphere.setParam( "sphere.localPos" , Vector3(5,5,5) );
		//mSpherePlane.draw();
		//mProgSphere.unbind();
		//glPopMatrix();

		//glFlush();
	}

	void SampleStage::renderTest0(ViewInfo& view)
	{
		//glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
		//glColor3f( 1 , 1 , 0 );

		//glTranslatef( 20 , 20 , 0 );

		{
			GL_BIND_LOCK_OBJECT(mProgSphere);
			mProgSphere.setParam("gParam.viewPos", mCamera->getPos());

			mProgSphere.setParam("sphere.radius", 0.3f);
			mProgSphere.setParam("sphere.localPos", mLightPos[0]);
			mSpherePlane.draw();

			mProgSphere.setParam("sphere.radius", 1.5f);
			mProgSphere.setParam("sphere.localPos", Vector3(10, 3, 1.0f));
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

		mProgPlanet.setParam("planet.radius", 2.0f);
		mProgPlanet.setParam("planet.maxHeight", 0.1f);
		mProgPlanet.setParam("planet.baseHeight", 0.5f);

		float const TileBaseScale = 2.0f;
		float const TilePosMin = -1.0f;
		float const TilePosMax = 1.0f;
		mProgPlanet.setParam("tile.pos", TilePosMin, TilePosMin);
		mProgPlanet.setParam("tile.scale", TileBaseScale);

		for( int i = 0 ; i < 6 ; ++i )
		{
			mProgPlanet.setMatrix33( "xformFace" , xform[i] );
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
		GL_BIND_LOCK_OBJECT(mProgParallax);
		//glTranslatef( 5 , 0 , 0 );
		//float m[16];
		//glGetFloatv( GL_MODELVIEW_MATRIX , m );

		//mProgParallax.setMatrix44( "matMV" , m );
		mProgParallax.setTexture( "texBase" , mTexBase , 0 );
		mProgParallax.setTexture( "texNormal" , mTexNormal, 1 );

		mProgParallax.setParam( "viewPos" , mCamera->getPos() );
		mProgParallax.setParam( "lightPos" , mLightPos[0] );
		mPlane.draw();

		glDisable( GL_TEXTURE_2D );
	}


	void SampleStage::renderTest2(ViewInfo& view)
	{
		drawSky();
		
		Vector3 color[] = { Vector3(1,0.3,0) , Vector3(0.3,1,0) , Vector3(0.3,0.5,1) };
		int num = 3;
		for( int i = 0 ; i < num ; ++i )
		{
			LightInfo light;
			light.pos = mLightPos[i];
			light.color = color[i];
			mShadowTech.renderLighting( view , *this , light , i != 0 );
		}

		showLight(num);

		mShadowTech.drawShadowTexture();
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
		mEffectSimple.setMatrix44( "gParam.matViewInv" , mCamera->getTransform() );
		mEffectSimple.setMatrix44( "gParam.matView" , mCamera->getViewMatrix() );

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
		mDefferredLightingTech.renderBassPass( view , *this );

		
		Vector3 color[] = { Vector3(1,0.3,0) , Vector3(0.3,1,0) , Vector3(0.3,0.5,1) };
		int num = 3;

		{
			
			for( int i = 0; i < num; ++i )
			{
				if( i == 1 )
				{
					glEnable(GL_BLEND);
					glBlendFunc(GL_ONE, GL_ONE);
				}
				LightInfo light;
				light.pos = mLightPos[i];
				light.color = color[i];
				mShadowTech.renderShadowDepth(view, *this, light);
				{
					GL_BIND_LOCK_OBJECT(mFrameBuffer);
					mDefferredLightingTech.renderLight(view, light , mShadowTech.mShadowMap , mShadowTech.depthParam );
				}
				
			}
		}
		
		glDisable(GL_BLEND);
		mFrameBuffer.setDepth(mDefferredLightingTech.mDepthTexture);
		{
			GL_BIND_LOCK_OBJECT(mFrameBuffer);
			showLight(num);
		}
		mFrameBuffer.clearDepth();

		{
			ViewportSaveScope vpScope;
			int w = vpScope[2] / 4;
			int h = vpScope[3] / 4;
			glViewport(w, 0 , 3 * w, 3 * h);
			copyTextureToBuffer(getRenderFrameTexture());
		}

		glDisable(GL_DEPTH_TEST);
		mDefferredLightingTech.renderBuffer();
		glEnable(GL_DEPTH_TEST);
	}

	void SampleStage::tick()
	{
		if ( mPause )
			return;

		mTime += gDefaultTickTime / 1000.0f;

		mPos.x = 10 * cos( mTime );
		mPos.y = 10 * sin( mTime );
		mPos.z = 0;

		mLightPos[0].x = 5 * cos( 0.3 * mTime );
		mLightPos[0].y = 5 * sin( 0.3 *mTime );
		mLightPos[0].z = 3;

		mLightPos[1].x = 5 * cos( 0.7 * mTime );
		mLightPos[1].z = 8 + 5 * sin( 0.7 *mTime );
		mLightPos[1].y = 3;

		mLightPos[2].y = 6 * cos( mTime );
		mLightPos[2].z = 8 + 6 * sin( mTime );
		mLightPos[2].x = 3;

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
		case Keyboard::eF3: 
			mLineMode = !mLineMode;
			glPolygonMode( GL_FRONT_AND_BACK , mLineMode ? GL_LINE : GL_FILL );
			break;
		case Keyboard::eP: mPause = !mPause; break;
		case Keyboard::eF2:
			switch( mIdxChioce )
			{
			case 0: mProgSphere.reload(); mProgPlanet.reload(); break;
			case 1: mProgParallax.reload(); break;
			case 2: mShadowTech.reload(); break;
			}
			
			break;
		case Keyboard::eF5:
			if ( mCamera == &mCamStorage[0])
				mCamera = &mCamStorage[1];
			else
				mCamera = &mCamStorage[0];
			break;
		case Keyboard::eF4:
			gUseMatrix = !gUseMatrix; 
			break;
		case Keyboard::eNUM1: mIdxChioce = 0; break;
		case Keyboard::eNUM2: mIdxChioce = 1; break;
		case Keyboard::eNUM3: mIdxChioce = 2; break;
		case Keyboard::eNUM4: mIdxChioce = 3; break;
		case Keyboard::eNUM5: mIdxChioce = 4; break;
		}
		return false;
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
			Vec2i size =  ::Global::getDrawEngine()->getScreenSize();
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

		if( !mProgCopyTexture.loadFromSingleFile(
			"Shader/CopyTexture", "CopyTextureVS", "CopyTextureFS") )
			return false;
	}

	bool ShadowDepthTech::init()
	{
		if ( !mShadowBuffer.create() )
			return false;

		if ( !mShadowMap2.create( Texture::eRGBA16F , MapSize , MapSize ) )
			return false;
		if ( !mShadowMap.create( Texture::eRGBA32F , MapSize , MapSize ) )
			return false;

		mShadowMap.bind();
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
		mShadowMap.unbind();

		DepthRenderBuffer depthBuffer;
		if ( !depthBuffer.create( MapSize , MapSize , Texture::eDepth32F ) )
			return false;

		mShadowBuffer.setDepth( depthBuffer , true );
		mShadowBuffer.addTexture(mShadowMap, Texture::eFaceX );
		//mBuffer.addTexture( mShadowMap2 );
		//mBuffer.addTexture( mShadowMap , Texture::eFaceX , false );

		if ( !mProgShadowDepth.loadFromSingleFile( "Shader/ShadowMap") )
			return false;
		if ( !mProgLighting.loadFromSingleFile( "Shader/ShadowLighting") )
			return false;

		depthParam[0] = 0.001;
		depthParam[1] = 100;

		return true;
	}

	void ShadowDepthTech::drawShadowTexture()
	{
		int vp[4];
		glGetIntegerv( GL_VIEWPORT , vp );

		glMatrixMode( GL_PROJECTION );
		glPushMatrix();
		glLoadIdentity();
		glOrtho(0 , vp[2] , 0 , vp[3] , -1 , 1 );
		glMatrixMode( GL_MODELVIEW );
		glPushMatrix();
		glLoadIdentity();
#if 0
		glEnable( GL_TEXTURE_2D );
		mShadowMap2.bind();
		//glBindTexture( GL_TEXTURE_2D , mBuffer.getTextureHandle( 0 ) );
		glBegin( GL_QUADS );
		glColor3f( 1,1,1 );
		int offset = 10;
		int length = 200;
		glTexCoord2f( 0 , 0 );glVertex2f( 0 , 0 );
		glTexCoord2f( 1 , 0 );glVertex2f( length , 0 );
		glTexCoord2f( 1 , 1 );glVertex2f( length , length);
		glTexCoord2f( 0 , 1 );glVertex2f( 0 , length );
		glEnd();
		mShadowMap2.unbind();
		glDisable( GL_TEXTURE_2D );

#else
		glEnable( GL_TEXTURE_CUBE_MAP );
		mShadowMap.bind();
		//glBindTexture( GL_TEXTURE_2D , mBuffer.getTextureHandle( 0 ) );

		glColor3f( 1,1,1 );
		int offset = 10;
		int length = 100;

		glTranslatef( 2 * length , 1 * length  , 0 );	
		glBegin( GL_QUADS ); //x
		glTexCoord3f( 1 ,-1 , 1 );glVertex2f( 0 , 0 );
		glTexCoord3f( 1 ,-1 , -1 );glVertex2f( length , 0 );
		glTexCoord3f( 1 , 1 , -1 );glVertex2f( length , length);
		glTexCoord3f( 1 , 1 , 1 );glVertex2f( 0 , length );
		glEnd();
		glTranslatef( - 2 * length , 0 , 0 );
		glBegin( GL_QUADS ); //-x
		glTexCoord3f( -1 ,-1 , -1 );glVertex2f( 0 , 0 );
		glTexCoord3f( -1 ,-1 , 1 );glVertex2f( length , 0 );
		glTexCoord3f( -1 ,1 , 1 );glVertex2f( length , length);
		glTexCoord3f( -1 , 1 ,-1 );glVertex2f( 0 , length );
		glEnd();
		glTranslatef( 1 * length , 1 * length , 0 );
		glBegin( GL_QUADS ); //y
		glTexCoord3f( -1 , 1 ,1 );glVertex2f( 0 , 0 );
		glTexCoord3f( 1 , 1 ,1 );glVertex2f( length , 0 );
		glTexCoord3f( 1 , 1 ,-1 );glVertex2f( length , length);
		glTexCoord3f( -1 , 1 ,-1 );glVertex2f( 0 , length );
		glEnd();
		glTranslatef( 0 * length , -2 * length , 0 );
		glBegin( GL_QUADS ); //-y
		glTexCoord3f( -1 , -1 ,-1 );glVertex2f( 0 , 0 );
		glTexCoord3f( 1 , -1 ,-1 );glVertex2f( length , 0 );
		glTexCoord3f( 1 , -1 ,1 );glVertex2f( length , length);
		glTexCoord3f( -1 , -1 , 1 );glVertex2f( 0 , length );
		glEnd();
		glTranslatef( 0 * length , 1 * length , 0 );
		glBegin( GL_QUADS ); //z
		glTexCoord3f( -1 , -1 , 1);glVertex2f( 0 , 0 );
		glTexCoord3f( 1 , -1 , 1);glVertex2f( length , 0 );
		glTexCoord3f( 1 , 1 , 1);glVertex2f( length , length);
		glTexCoord3f( -1 , 1 , 1);glVertex2f( 0 , length );
		glEnd();
		glTranslatef( 2 * length , 0 * length , 0 );
		glBegin( GL_QUADS ); //-z
		glTexCoord3f( 1 , -1 , -1);glVertex2f( 0 , 0 );
		glTexCoord3f( -1 , -1 , -1);glVertex2f( length , 0 );
		glTexCoord3f( -1 , 1 , -1);glVertex2f( length , length);
		glTexCoord3f(  1 , 1 , -1);glVertex2f( 0 , length );
		glEnd();
		mShadowMap.unbind();
		glDisable( GL_TEXTURE_CUBE_MAP );

#endif

		glMatrixMode( GL_MODELVIEW );
		glPopMatrix();
		glMatrixMode( GL_PROJECTION );
		glPopMatrix();
		glMatrixMode( GL_MODELVIEW );
	}

	void ShadowDepthTech::renderLighting( ViewInfo& view, SceneRender& scene, LightInfo const& light , bool bMultiple )
	{
		renderShadowDepth( view , scene , light );

		GL_BIND_LOCK_OBJECT(mProgLighting);
		//mProgLighting.setTexture( "texSM" , mShadowMap2 , 0 );
		mProgLighting.setTexture( "texSM" , mShadowMap , 0 );
		mProgLighting.setParam("lightPos" , light.pos );
		mProgLighting.setParam( "lightColor" , light.color );
		mProgLighting.setParam( "matLightView" , matLightView );
		mProgLighting.setParam( "gParam.viewPos" , view.camera->getPos() );
		mProgLighting.setParam( "depthParam" , depthParam[0] , depthParam[1] );

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
	}

	void ShadowDepthTech::renderShadowDepth(ViewInfo& view, SceneRender& scene , LightInfo const& light )
	{
		//glDisable( GL_CULL_FACE );
		ViewportSaveScope vpScope;
		glViewport(0, 0, MapSize, MapSize);

		Matrix4 matLightProject = PerspectiveMatrix(Math::Deg2Rad(90), 1.0, depthParam[0], depthParam[1]);
		MatrixSaveScope matScope( matLightProject );

		GL_BIND_LOCK_OBJECT(mProgShadowDepth);
		mEffectCur = &mProgShadowDepth;
		mProgShadowDepth.setParam("depthParam", depthParam[0], depthParam[1]);

#if 0
		mShadowBuffer.setTexture( 0 , mShadowMap2 );
		mShadowBuffer.bind();
		//glClearBufferfv( );
		glClearColor( 1 , 1 , 1 , 1.0 );
		glClearDepth( 1 );
		glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );


		matLightView = LookAtMatrix( light.pos , light.pos + Vector3(0,0,-1) , Vector3(1,0,0) );
		glLoadMatrixf( matLightView );
		fun( *this );
#else

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

		glClearColor( 1 , 1 , 1 , 1.0 );
		for( int i = 0 ; i < 6 ; ++i )
		{
			mShadowBuffer.setTexture( 0 , mShadowMap , Texture::Face( i ) );
			GL_BIND_LOCK_OBJECT(mShadowBuffer);
			glEnable( GL_DEPTH_TEST );
			glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );
			LookAtMatrix mat( light.pos, CubeFaceDir[i] , CubeUpDir[i] );
			glLoadMatrixf( mat );
			scene.render( view , *this );
		}
		glClearColor( 0 , 0 , 0 , 1.0 );
		matLightView = Matrix4::Translate( -light.pos );
#endif

	}


	void TechBase::drawCubeTexture(TextureCube& texCube)
	{
		ViewportSaveScope vpScope;

		Matrix4 matProj = OrthoMatrix(0, vpScope[2], 0, vpScope[3], -1, 1);
		MatrixSaveScope matScope( matProj );


		glEnable( GL_TEXTURE_CUBE_MAP );

		GL_BIND_LOCK_OBJECT(texCube);
		//glBindTexture( GL_TEXTURE_2D , mBuffer.getTextureHandle( 0 ) );

		glColor3f( 1,1,1 );
		int offset = 10;
		int length = 100;

		glTranslatef( 2 * length , 1 * length  , 0 );	
		glBegin( GL_QUADS ); //x
		glTexCoord3f( 1 ,-1 , 1 );glVertex2f( 0 , 0 );
		glTexCoord3f( 1 ,-1 , -1 );glVertex2f( length , 0 );
		glTexCoord3f( 1 , 1 , -1 );glVertex2f( length , length);
		glTexCoord3f( 1 , 1 , 1 );glVertex2f( 0 , length );
		glEnd();
		glTranslatef( - 2 * length , 0 , 0 );
		glBegin( GL_QUADS ); //-x
		glTexCoord3f( -1 ,-1 , -1 );glVertex2f( 0 , 0 );
		glTexCoord3f( -1 ,-1 , 1 );glVertex2f( length , 0 );
		glTexCoord3f( -1 ,1 , 1 );glVertex2f( length , length);
		glTexCoord3f( -1 , 1 ,-1 );glVertex2f( 0 , length );
		glEnd();
		glTranslatef( 1 * length , 1 * length , 0 );
		glBegin( GL_QUADS ); //y
		glTexCoord3f( -1 , 1 ,1 );glVertex2f( 0 , 0 );
		glTexCoord3f( 1 , 1 ,1 );glVertex2f( length , 0 );
		glTexCoord3f( 1 , 1 ,-1 );glVertex2f( length , length);
		glTexCoord3f( -1 , 1 ,-1 );glVertex2f( 0 , length );
		glEnd();
		glTranslatef( 0 * length , -2 * length , 0 );
		glBegin( GL_QUADS ); //-y
		glTexCoord3f( -1 , -1 ,-1 );glVertex2f( 0 , 0 );
		glTexCoord3f( 1 , -1 ,-1 );glVertex2f( length , 0 );
		glTexCoord3f( 1 , -1 ,1 );glVertex2f( length , length);
		glTexCoord3f( -1 , -1 , 1 );glVertex2f( 0 , length );
		glEnd();
		glTranslatef( 0 * length , 1 * length , 0 );
		glBegin( GL_QUADS ); //z
		glTexCoord3f( -1 , -1 , 1);glVertex2f( 0 , 0 );
		glTexCoord3f( 1 , -1 , 1);glVertex2f( length , 0 );
		glTexCoord3f( 1 , 1 , 1);glVertex2f( length , length);
		glTexCoord3f( -1 , 1 , 1);glVertex2f( 0 , length );
		glEnd();
		glTranslatef( 2 * length , 0 * length , 0 );
		glBegin( GL_QUADS ); //-z
		glTexCoord3f( 1 , -1 , -1);glVertex2f( 0 , 0 );
		glTexCoord3f( -1 , -1 , -1);glVertex2f( length , 0 );
		glTexCoord3f( -1 , 1 , -1);glVertex2f( length , length);
		glTexCoord3f(  1 , 1 , -1);glVertex2f( 0 , length );
		glEnd();
		glDisable( GL_TEXTURE_CUBE_MAP );
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


	bool DefferredLightingTech::init()
	{
		GameWindow& gameWindow = ::Global::getDrawEngine()->getWindow();

		Vec2i size = ::Global::getDrawEngine()->getScreenSize();

		if( !mBuffer.create() )
			return false;

		for( int i = 0; i < NumBuffer; ++i )
		{
			if( !mGTextures[i].create(Texture::eRGBA32F, size.x, size.y) )
				return false;
			mBuffer.addTexture(mGTextures[i]);
		}

		if( !mDepthTexture.create(Texture::eDepth32F, size.x, size.y) )
			return false;

		mBuffer.setDepth(mDepthTexture);

		if( !mBassPass.loadFromSingleFile(
			"Shader/DeferredLighting",
			"BassPassVS", "BasePassFS", "#define BASS_PASS\n") )
			return false;

		if( !mLightingPass.loadFromSingleFile(
			"Shader/DeferredLighting",
			"LightingPassVS", "LightingPassFS", "") )
			return false;

		return true;
	}

	void DefferredLightingTech::renderBassPass(ViewInfo& view, SceneRender& scene)
	{
		glEnable(GL_DEPTH_TEST);
		GL_BIND_LOCK_OBJECT(mBuffer);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		GL_BIND_LOCK_OBJECT(mBassPass);
		view.setParam(mBassPass);
		scene.render(view, *this);
	}

	void DefferredLightingTech::renderLight(ViewInfo& view, LightInfo const& light , TextureCube& texShadow , float shadowParam[2] )
	{
		glDisable(GL_DEPTH_TEST);
		{
			GL_BIND_LOCK_OBJECT(mLightingPass);
			mLightingPass.setParam("lightPos", light.pos);
			mLightingPass.setParam("lightColor", light.color);
			mLightingPass.setParam("viewPos", view.camera->getPos());
			mLightingPass.setTexture("GBufferTextureA", mGTextures[BufferA], 0);
			mLightingPass.setTexture("GBufferTextureB", mGTextures[BufferB], 1);
			mLightingPass.setTexture("GBufferTextureC", mGTextures[BufferC], 2);
			mLightingPass.setTexture("texSM", texShadow, 4);
			mLightingPass.setParam("depthParam", shadowParam[0], shadowParam[1]);
			DrawUtiltiy::ScreenRect();
		}
		glEnable(GL_DEPTH_TEST);
	}

	void DefferredLightingTech::renderBuffer()
	{
		ViewportSaveScope vpScope;
		OrthoMatrix matProj(0, vpScope.value[2], 0, vpScope.value[3], -1, 1);

		MatrixSaveScope matScope(matProj);

		int width = vpScope.value[2] / 4;
		int height = vpScope.value[3] / 4;

		renderTexture(0, 0 * height, width, height, 0);
		renderTexture(0, 1 * height, width, height, 1);
		renderTexture(0, 2 * height, width, height, 2);
		renderTexture(0, 3 * height, width, height, 3);
		//renderDepthTexture(width , 3 * height, width, height);
	}

	void DefferredLightingTech::renderDepthTexture(int x, int y, int width, int height)
	{
		//PROB
		glLoadIdentity();
		glTranslatef(x, y, 0);
		glEnable(GL_TEXTURE_2D);
		glBindTexture( GL_TEXTURE_2D , mDepthTexture.mHandle );
		glColor3f(1, 1, 1);
		DrawUtiltiy::Rect(width, height);
		glBindTexture(GL_TEXTURE_2D, 0);
		glDisable(GL_TEXTURE_2D);
	}

	void DefferredLightingTech::renderTexture(int x, int y, int width, int height, int idxBuffer)
	{
		glLoadIdentity();
		glTranslatef(x, y, 0);
		glEnable(GL_TEXTURE_2D);
		mGTextures[idxBuffer].bind();
		glColor3f(1, 1, 1);
		DrawUtiltiy::Rect(width, height);
		mGTextures[idxBuffer].unbind();
		glDisable(GL_TEXTURE_2D);
	}

}//namespace RenderGL



