#include "SampleBase.h"


class ActorSample : public SampleBase
{
public:
	
	ILight*    mainLight;
	IActor*    actor;
	IObject*   plane;

	static int const CubeMapSize = 512;
	bool OnSetupSample()
	{
		mainLight = m_mainScene->createLight( nullptr );
		mainLight->setName( "mainLight" );
		mainLight->translate( CFTO_LOCAL , Vector3(0,0,20) );
		mainLight->setLightColor( ColorKey(1,1,1) );

		return true; 
	}

	 void OnExitSample()
	 {

	 }

	void OnUpdate( long time )
	{
		static float angle = 0.0f;

		SampleBase::OnUpdate( time );
		mainLight->setLocalPostion( 
			Vector3( 50 * Math::Cos( angle ) , 50 * Math::Sin( angle ) , 50 ) );


		angle += 0.001 * time;
	}



	void OnRenderScene()
	{
		SampleBase::OnRenderScene();
	}

	void OnShowMessage()
	{
		SampleBase::OnShowMessage();
		Vector3 pos = m_mainCamera->getWorldPos();
		pushMessage( "cam pos = %f %f %f" , pos.x , pos.y , pos.z );
	}


};

INVOKE_SAMPLE( ActorSample )
