#include "SampleBase.h"


class SpriteSample : public SampleBase
{
public:
	
	Light*   mainLight;
	Sprite*  sprite;
	Object*  obj;

	static int const CubeMapSize = 512;
	bool OnSetupSample()
	{
		mainLight = mMainScene->createLight( nullptr );
		mainLight->setName( "mainLight" );
		mainLight->translate( CFTO_LOCAL , Vector3(0,0,20) );
		mainLight->setLightColor( ColorKey(1,1,1) );

		sprite = mMainScene->createSprite( nullptr );
		sprite->createRectArea( 0 , 0 , 64 , 64  , "Spell_Fire_FlameShock" , 1 , 0 , nullptr , nullptr , CF_FILTER_POINT );

		sprite->setRenderOption( CFRO_ALPHA_BLENGING , TRUE );
		sprite->setRenderOption( CFRO_SRC_BLEND , CF_BLEND_SRC_ALPHA );
		sprite->setRenderOption( CFRO_DEST_BLEND , CF_BLEND_INV_SRC_ALPHA );

		obj = mMainScene->createObject( nullptr );

		struct Vertex
		{
			Vector3 pos;
			Vector3 color;
		} v[] =
		{
			{ Vector3( 0,0,0 ) , Vector3( 1,0,0) } ,
			{ Vector3( 10,0,0 ) , Vector3( 0,1,0) } ,
			{ Vector3( 10,10,0 ) , Vector3( 1,1,1) } ,
			{ Vector3( 0,10,0 ) , Vector3( 0,0,1) } ,
		};

		int idx[] = { 0 , 1 , 2 , 3 };
		Material* mat = mWorld->createMaterial( Vector3(1,1,1) );
		obj->createPrimitive( mat , CFPT_TRIANGLEFAN , CFVT_XYZ_CF1 , nullptr , 0  , v[0].pos , 4  , idx , 4  );
		//obj->createIndexedTriangle( nullptr , CFVT_XYZ_RGB , 0 , nullptr , 0 , 4 , v[0].pos , 2 , idx  );

		return true; 
	}

	 void OnExitSample()
	 {

	 }

	long OnUpdate( long time )
	{
		static float angle = 0.0f;

		SampleBase::OnUpdate( time );
		mainLight->setLocalPosition( 
			Vector3( 50 * Math::Cos( angle ) , 50 * Math::Sin( angle ) , 50 ) );

		angle += 0.001 * time;

		return time;
	}

	void OnRenderScene()
	{
		mMainScene->render(  mMainCamera , mMainViewport );
		//mMainScene->render2D( mMainViewport );
	}

	void OnShowMessage()
	{
		SampleBase::OnShowMessage();
		Vector3 pos = mMainCamera->getWorldPosition();
		pushMessage( "cam pos = %f %f %f" , pos.x , pos.y , pos.z );
	}


};

INVOKE_SAMPLE( SpriteSample )
