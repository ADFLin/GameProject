#include "SampleBase.h"

class SampleCoordTest : public SampleBase
{
public:

	struct Vertex
	{
		float v[3];
		float c[3];
	};
	bool onSetupSample()
	{
		createCoorditeAxis( 120 );

		mMainCamera->setLookAt( Vector3( 0,0,150 ) , Vector3(0,0,0), Vector3(0,1,0) );

		Vertex vtx[4] =
		{
			{ 0 , 0 , 0 , 0 , 0 , 1 } ,
			{ 100 , 0 , 0 , 1 , 0 , 0 } ,
			{ 100 , 100 , 0 , 1 , 1 , 1 } ,
			{ 0 , 100 , 0 , 0 , 1 , 0 } ,
		};

		int idx[6] = 
		{
			0 , 1 , 2 ,
			0 , 2 , 3 ,
		};

		Material* mat = mWorld->createMaterial(  0 , 0 , 0 , 1 , Color4f( 1 , 1 , 1 , 1 ) );
		ShaderEffect* shader = mat->addShaderEffect( "CoordTest" ,"CoordTest" );
		shader->addParam( SP_WVP   , "mWVP" );

		mat->setLightingColor( CFLC_EMISSIVE , CFMC_VERTEX_C1 );
		Object* obj = mMainScene->createObject();
		obj->createIndexedTriangle( mat , CFVT_XYZ_CF1 , (float*)vtx , 4 , idx , 2 );
		return true; 
	}

	void onExitSample()
	{

	}

	long onUpdate( long time )
	{
		SampleBase::onUpdate( time );
		return time;
	}

	void onRenderScene()
	{
		mMainScene->render( mMainCamera , mMainViewport );
	}

	void onShowMessage()
	{
		SampleBase::onShowMessage();
		Vector3 pos = mMainCamera->getWorldPosition();
		pushMessage( "cam pos = %f %f %f" , pos.x , pos.y , pos.z );
	}

};

INVOKE_SAMPLE( SampleCoordTest )
