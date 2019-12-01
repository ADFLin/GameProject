#include "SampleBase.h"


class IShadowMap
{
public:
};

class IObjectEffect
{

};

class IMaterialEffect
{
public:


};

class ISceneEffect
{
public:





};

class IShadowTechnique : public ISceneEffect
{
public:
	virtual void addLight( Light* light );
};

class IShadowMapTechnique : public IShadowTechnique
{

	IShadowMapTechnique ( Scene* scene )
		:m_scene( scene )
	{

		init();
	}
	virtual void addLight( Light* light );


	void init()
	{
		World* world = m_scene->getWorld();

		m_viewCamera = m_scene->createCamera( nullptr );
		m_viewCamera->registerName("viewCamera");
		m_viewCamera->setFov( Math::Deg2Rad(90) );

		m_viewCamera->setFar( 1000.0f );
		m_viewCamera->setNear( 1.0f );


		m_material = world->createMaterial();
		//m_material->addRenderTarget( 0 , 0 , "ShadowMap" , CF_TEX_FMT_R32F , viewport , false );
		ShaderEffect* shader = m_material->addShaderEffect( "ShadowMap" , "ShadowMap" );

		shader->addParam( SP_WVP   , "mWVP" );
		shader->addParam( SP_WORLD , "mWorld" );
		shader->addParam( SP_WORLD_VIEW , "mWorldView" );
		shader->addParam( SP_CAMERA_Z_FAR , "g_ZFar" );
		shader->addParam( SP_CAMERA_Z_NEAR , "g_ZNear" );

		shader->addParam( SP_MATERIAL_TEXLAYER0 , "shadowMap" );
		shader->addParam( SP_MATERIAL_DIFFUSE , "matDif" );
		shader->addParam( SP_CAMERA_POS , "g_camPos" );
		shader->addParam( SP_MATERIAL_DIFFUSE , "matDif" );


		char const* camName = m_viewCamera->getName();
		shader->addParam( SP_CAMERA_POS , camName , "g_lightPos" );
		shader->addParam( SP_CAMERA_VIEW , camName , "g_viewMatrix" );
		shader->addParam( SP_CAMERA_Z_FAR , camName , "g_viewZFar" );
		shader->addParam( SP_CAMERA_Z_NEAR , camName , "g_viewZNear" );

	}


	struct LightShadowData
	{
		Texture::RefCountPtr  ShadowMap;
		Viewport*        viewport;
	};
	Material::RefCountPtr m_material;
	Camera*          m_viewCamera;
	Scene*           m_scene;
};

class ProjTextureSample : public SampleBase
{
public:
	
	Light*    mainLight;

	static int const BallNum = 9;
	Object*   ball[BallNum];
	Object*   plane;
	Camera*   viewCamera;
	Material::RefCountPtr SMMaterial;
	Viewport* SMViewport;


	Viewport* showSMViewport;

	static int const CubeMapSize = 512;
	bool onSetupSample()
	{
		mainLight = mMainScene->createLight( nullptr );
		mainLight->registerName( "mainLight" );
		mainLight->translate( Vector3(0 ,100 ,50 ) , CFTO_LOCAL );
		mainLight->setLightColor( Color4f(0.3,0.3,0.3) );


		{
			viewCamera = mMainScene->createCamera( nullptr );
			viewCamera->registerName("viewCamera");
			viewCamera->setFov( Math::Deg2Rad(120) );
			viewCamera->rotate( CF_AXIS_X , Math::Deg2Rad(90) , CFTO_LOCAL );

			viewCamera->setFar( 500.0f );
			viewCamera->setNear( 1.0f );
		}

		Texture* texShadowMap;
		{

			SMViewport = mWorld->createViewport( 0 , 0 , 512 , 512 );
			SMMaterial = mWorld->createMaterial();
			texShadowMap = SMMaterial->addRenderTarget( 0 , 0 , "ShadowMap" , CF_TEX_FMT_RGB32 , SMViewport , true );
			ShaderEffect* shader = SMMaterial->addShaderEffect( "ShadowMap" , "ShadowMap" );

			shader->addParam( SP_WVP   , "mWVP" );
			shader->addParam( SP_WORLD , "mWorld" );
			shader->addParam( SP_WORLD_VIEW , "mWorldView" );
			shader->addParam( SP_CAMERA_Z_FAR , "g_ZFar" );
			shader->addParam( SP_CAMERA_Z_NEAR , "g_ZNear" );
			shader->addParam( SP_CAMERA_VIEW , "viewCamera" , "g_viewMatrix" );
			shader->addParam( SP_CAMERA_Z_FAR , "viewCamera" , "g_viewZFar" );
			shader->addParam( SP_CAMERA_Z_NEAR , "viewCamera" , "g_viewZNear" );
			shader->addParam( SP_MATERIAL_TEXLAYER0 , "shadowMap" );
			shader->addParam( SP_MATERIAL_DIFFUSE , "matDif" );
			shader->addParam( SP_CAMERA_POS , "g_camPos" );
			shader->addParam( SP_CAMERA_POS , "viewCamera" , "g_lightPos" );
			shader->addParam( SP_MATERIAL_DIFFUSE , "matDif" );

			showSMViewport = mWorld->createViewport( 0 , 0 , 200 , 200 );
		}
		{
			plane = mMainScene->createObject( nullptr );
			Material* mat = mWorld->createMaterial( Vector3(0.3,0.3,0.3) , Vector3(0.3,0.3,0.3) );
			mat->addTexture( 0 , 0 , "0016" );
			
			//IShaderEffect* shader = mat->addShaderEffect( "projTexture" , "projTexture" );
			//shader->addParam( SP_WVP   , "mWVP" );
			//shader->addParam( SP_WORLD , "mWorld" );
			//shader->addParam( SP_CAMERA_VIEW , "viewCamera" , "g_viewMatrix" );
			//shader->addParam( SP_MATERIAL_TEXLAYER0 , "projTex" );
			
			plane->createPlane( mat ,  1000 , 1000 , 0 , 5 , 5, Vector3(0,1,0) , Vector3(1,0,0) );
			
			plane->translate( Vector3(0,-100 ,0 ) , CFTO_LOCAL );

			for( int i = 0 ; i < BallNum ; ++i )
			{
				ball[i] = mMainScene->createObject( nullptr );
				ball[i]->load( "box.cw3" );
				ball[i]->addVertexNormal();
				int layer = i / 9;
				int idx  =  i % 9;
				ball[i]->setLocalPosition( 40 * Vector3( idx / 3  , layer , idx % 3 ) - Vector3(40,40,40));
			}
		}

		scene2 = mWorld->createScene(1);

		Material* mat = mWorld->createMaterial();
		Sprite* obj = scene2->createSprite();
		
		mat->addTexture(0,0, texShadowMap );
		//obj->setRectArea( nullptr , 100 , 100 , texShadowMap );
		obj->createPlane( mat , 200 , 200 , nullptr , 1 , 1 , Vector3(0,0,1) );
		obj->translate( Vector3(100,100,0) , CFTO_REPLACE );

		return true; 
	}

	Scene* scene2;
	Viewport* viewport2;

	 void onExitSample()
	 {
		 SMMaterial = nullptr;

	 }

	 bool handleKeyEvent( unsigned key , bool isDown )
	 {
		 if ( !isDown )
			 return false;

		 switch( key )
		 {
		 case Keyboard::eO: viewCamera->rotate( CF_AXIS_X , Math::Deg2Rad(1) , CFTO_LOCAL);  break;
		 case Keyboard::eP: viewCamera->rotate( CF_AXIS_X , Math::Deg2Rad(-1) , CFTO_LOCAL );  break;
		 }

		 return SampleBase::handleKeyEvent( key , isDown );
	 }


	long onUpdate( long time )
	{
		static float angle = 0.0f;

		float const radius = 100;

		SampleBase::onUpdate( time );

		mainLight->setLocalPosition( 
			Vector3( radius * Math::Cos( angle ) , 100 , radius * Math::Sin( angle ) ) );

		viewCamera->setLocalPosition(
			Vector3( radius * Math::Cos( angle ) , 100 , radius * Math::Sin( angle ) ) );

		angle += 0.002 * time;

		Vector3 pos = viewCamera->getWorldPosition();

		return time;
	}


	void renderShadowMap( Object* obj , bool beC )
	{
		Material::RefCountPtr mat = obj->getElement()->getMaterial();
		obj->getElement()->setMaterial( SMMaterial );
		mMainScene->renderObject( obj , viewCamera , SMViewport , (beC) ? CFRF_DEFULT : 0 );
		obj->getElement()->setMaterial( mat );
	}

	void renderScene( Object* obj , bool beC )
	{
		Color4f color;
		Material::RefCountPtr mat = plane->getElement()->getMaterial();
		color = mat->getDiffuse();
		SMMaterial->setDiffuse( color );
		obj->getElement()->setMaterial( SMMaterial );
		mMainScene->renderObject( obj , mMainCamera , mMainViewport , (beC) ? CFRF_DEFULT : 0 );
		obj->getElement()->setMaterial( mat );
	}


	void onRenderScene()
	{
		SMMaterial->setTechnique( "ShadowMap" );
		renderShadowMap( plane , true );
		for( int i = 0 ; i < BallNum ; ++i )
			renderShadowMap( ball[i] ,false );
		
		SMMaterial->setTechnique( "RenderScene" );
		renderScene( plane , true );
		for( int i = 0 ; i < BallNum ; ++i )
			renderScene( ball[i] ,false );

		scene2->render2D( mMainViewport , 0 );
		
		//SampleBase::OnRenderScene();
	}

	void onShowMessage()
	{
		SampleBase::onShowMessage();
		Vector3 pos = mMainCamera->getWorldPosition();
		pushMessage( "cam pos = %f %f %f" , pos.x , pos.y , pos.z );
		pos = viewCamera->getWorldPosition();
		pushMessage( "light pos = %f %f %f" , pos.x , pos.y , pos.z );
	}


};

INVOKE_SAMPLE( ProjTextureSample )
