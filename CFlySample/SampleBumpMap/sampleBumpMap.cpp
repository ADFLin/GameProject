#include "SampleBase.h"

class BumpMapSample : public SampleBase
{
public:

	Light*    mainLight;
	Object*   obj;

	static int const CubeMapSize = 512;
	bool onSetupSample()
	{
		mainLight = mMainScene->createLight( nullptr );
		mainLight->registerName( "mainLight" );
		mainLight->translate( Vector3(0,0,20) , CFTO_LOCAL );
		mainLight->setLightColor( Color4f(1,1,1) );

		Material* mat = mWorld->createMaterial( 
			Color4f(0.3,0.3,0.3) , 
			Color4f(0.6,0.6,0.6) ,
			Color4f(0.8,0.8,0.8), 5 );

		mat->addTexture( 0 , 0 , "stones" );
		mat->addTexture( 0 , 1 , "stones_NM_height" );
		//mat->setDiffuse( Vector3(0,0,0) );

		mat->setShininess( 1.5f );

		ShaderEffect* shader = mat->addShaderEffect( "phong_bump" , "PhongBump" );
		//IShader* shader = mat->addShaderEffect( "phong" , "Phong" );

		shader->addParam( SP_WORLD , "mWorld" );
		shader->addParam( SP_WVP   , "mWVP" );
		shader->addParam( SP_WORLD_INV , "mWorldInv" );
		shader->addParam( SP_LIGHT_POS , "mainLight" , "mainLightPosition" );
		shader->addParam( SP_LIGHT_DIFFUSE , "mainLight" , "mainLightColor" );
		shader->addParam( SP_CAMERA_POS , "camPosition" );
		shader->addParam( SP_MATERIAL_AMBIENT , "amb" );
		shader->addParam( SP_MATERIAL_DIFFUSE , "dif" );
		shader->addParam( SP_MATERIAL_SPECULAR , "spe" );
		shader->addParam( SP_MATERIAL_SHINENESS , "power" );
		shader->addParam( SP_MATERIAL_TEXLAYER0 , "texMap" );
		shader->addParam( SP_MATERIAL_TEXLAYER1 , "bumpMap" );

		obj = mMainScene->createObject( nullptr );
		obj->createPlane( mat , 100 , 100 , nullptr );
		obj->addVertexNormal();

		return true; 
	}

	void onExitSample()
	{

	}

	long onUpdate( long time )
	{
		static float angle = 0.0f;

		SampleBase::onUpdate( time );
		mainLight->setLocalPosition( 
			Vector3( 50 * Math::Cos( angle ) , 50 * Math::Sin( angle ) , 50 ) );

		angle += 0.001 * time;

		return time;
	}



	void onRenderScene()
	{
		SampleBase::onRenderScene();
	}

	void onShowMessage()
	{
		SampleBase::onShowMessage();
		Vector3 pos = mMainCamera->getWorldPosition();
		pushMessage( "cam pos = %f %f %f" , pos.x , pos.y , pos.z );
	}


};

INVOKE_SAMPLE( BumpMapSample )