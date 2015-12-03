#include "SampleBase.h"

class SampleCoordTest : public SampleBase
{
public:
	bool onSetupSample()
	{
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
		mMainScene->render2D( mMainViewport );
	}

	void onShowMessage()
	{
		SampleBase::onShowMessage();
		Vector3 pos = mMainCamera->getWorldPosition();
		pushMessage( "cam pos = %f %f %f" , pos.x , pos.y , pos.z );
	}

};

INVOKE_SAMPLE( SampleCoordTest )
