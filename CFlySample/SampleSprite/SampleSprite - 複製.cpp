#include "SampleBase.h"


class SpriteSample : public SampleBase
{
public:
	
	ILight*    mainLight;
	ISprite*  sprite;
	IObject*  obj;

	static int const CubeMapSize = 512;
	bool OnSetupSample()
	{
		mainLight = mMainScene->createLight( nullptr );
		mainLight->setName( "mainLight" );
		mainLight->translate( CFTO_LOCAL , Vector3(0,0,20) );
		mainLight->setLightColor( ColorKey(1,1,1) );

		sprite = mMainScene->createSprite( nullptr );
		sprite->setRectArea( nullptr , 64 , 64  , "Spell_Fire_FlameShock" , 0 , nullptr , false , 0 , 0 , 0 , CF_FILTER_POINT );
		sprite->setRectPosition( Vector3(0,0,0) );

		sprite->setRenderOption( CFRO_ALPHA_BLENGING , TRUE );
		sprite->setRenderOption( CFRO_SRC_BLEND , CF_BLEND_SRC_ALPHA );
		sprite->setRenderOption( CFRO_DEST_BLEND , CF_BLEND_INV_SRC_ALPHA );
		return true; 
	}

	 void OnExitSample()
	 {

	 }

	void OnUpdate( long time )
	{
		static float angle = 0.0f;

		SampleBase::OnUpdate( time );
		mainLight->setLocalPosition( 
			Vector3( 50 * Math::Cos( angle ) , 50 * Math::Sin( angle ) , 50 ) );


		angle += 0.001 * time;
	}



	void OnRenderScene()
	{
		mMainScene->render2D( mMainViewport );
	}

	void OnShowMessage()
	{
		SampleBase::OnShowMessage();
		Vector3 pos = mMainCamera->getWorldPos();
		pushMessage( "cam pos = %f %f %f" , pos.x , pos.y , pos.z );
	}


};

INVOKE_SAMPLE( SpriteSample )
