#include "SampleBase.h"


class CubeMapSample : public SampleBase
{
public:
	
	Light*    mainLight;
	Object*   cubeMapObj;
	Viewport* cubeMapViewport;
	Camera*   cubeMapCamera;
	Texture*  cubeMapTex;

	Object*   obj;

	static int const CubeMapSize = 512;
	bool onSetupSample()
	{
		mainLight = mMainScene->createLight( nullptr );
		mainLight->registerName( "mainLight" );
		mainLight->translate( Vector3(0,0,20) , CFTO_LOCAL );
		mainLight->setLightColor( Color4f(1,1,1) );

		cubeMapCamera = mMainScene->createCamera( nullptr );
		cubeMapCamera->setAspect( 1.0f );
		cubeMapCamera->setFov( Math::DegToRad(90) ) ;
		cubeMapViewport = mWorld->createViewport( 0 , 0 , CubeMapSize , CubeMapSize );

		Object* skyBox = mMainScene->createSkyBox( "hallCube" , 2000 , true );

		Material* mat = skyBox->getElement(0)->getMaterial();
		//skyBox->setRenderOption( CFRO_LIGHTING , FALSE );
		Texture* skyTex = mat->getTexture( 0 );

		cubeMapObj = mMainScene->createObject( nullptr );
		cubeMapObj->load( "ball.cw3" );
		cubeMapObj->addVertexNormal();
		mat = cubeMapObj->getElement(0)->getMaterial();

		cubeMapTex = mat->addRenderCubeMap( 0 , 1 , "cubemap" , CF_TEX_FMT_R5G6B5 , CubeMapSize , false );
		//mat->addTexture( 0 , 1 , skyTex );
		//mat->setDiffuse( Vector3(0,0,0) );
		mat->setShininess( 5 );
		ShaderEffect* shader = mat->addShaderEffect( "envMap" , "envMap" );

		shader->addParam( SP_WORLD              , "mWorld" );
		shader->addParam( SP_WVP                , "mWVP" );
		shader->addParam( SP_WORLD_INV          , "mWorldInv" );
		shader->addParam( SP_LIGHT_POS          , "mainLight" , "mainLightPosition" );
		shader->addParam( SP_LIGHT_DIFFUSE      , "mainLight" , "mainLightColor" );
		shader->addParam( SP_CAMERA_POS         , "camPosition" );
		shader->addParam( SP_MATERIAL_AMBIENT   , "amb" );
		shader->addParam( SP_MATERIAL_DIFFUSE   , "dif" );
		shader->addParam( SP_MATERIAL_SPECULAR  , "spe" );
		shader->addParam( SP_MATERIAL_SHINENESS , "power" );
		shader->addParam( SP_MATERIAL_TEXLAYER1 , "cubeTexMap" );

		obj = mMainScene->createObject( nullptr );
		//obj->load( "ball.cw3" );

		return true; 
	}

	 void onExitSample()
	 {

	 }

	long onUpdate( long time )
	{
		static float angle = 0.0f;

		SampleBase::onUpdate( time );
		//mainLight->setLocalPosition( 
		//	Vector3( 50 * Math::Cos( angle ) , 50 * Math::Sin( angle ) , 50 ) );

		//float radius = 200;
		//obj->setLocalPosition( 
		//	Vector3( radius * Math::Cos( angle ) , 0 , radius * Math::Sin( angle ) ) );

		angle += 0.001 * time;

		return time;
	}

	struct RotationData
	{
		AxisEnum rotateAxis;
		float    angle;
	};


	void onRenderScene()
	{
		static RotationData const cubeViewSetting[]=
		{
			CF_AXIS_Y ,  Math::DegToRad(90),
			CF_AXIS_Y , -Math::DegToRad(90),
			CF_AXIS_X , -Math::DegToRad(90),
			CF_AXIS_X ,  Math::DegToRad(90),
			CF_AXIS_Y , 0,
			CF_AXIS_Y , Math::DegToRad(180),
		};

		cubeMapObj->show( false );
		for( int i = 0 ; i < 6 ; ++i )
		{
			cubeMapViewport->setRenderTarget( cubeMapTex->getRenderTarget(i) , 0 );
			cubeMapCamera->rotate( 
				cubeViewSetting[i].rotateAxis , 
				cubeViewSetting[i].angle , 
				CFTO_REPLACE );
			mMainScene->render( cubeMapCamera , cubeMapViewport );
		}
		cubeMapObj->show( true );

		SampleBase::onRenderScene();
	}

	void onShowMessage()
	{
		SampleBase::onShowMessage();
		Vector3 pos = mMainCamera->getWorldPosition();
		pushMessage( "cam pos = %f %f %f" , pos.x , pos.y , pos.z );
	}


};

INVOKE_SAMPLE( CubeMapSample )
