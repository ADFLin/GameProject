#ifndef SampleBase_h__
#define SampleBase_h__

#include "GameLoop.h"

#include "WindowsPlatform.h"
#include "WindowsMessageHander.h"

#include "CFlyHeader.h"


#define SAMPLE_DATA_DIR "../Data"
using namespace CFly;

int const g_ScreenWidth  = 800;
int const g_ScreenHeight = 600;

class CameraController
{


	CameraController( Camera* cam )
		:mCamera( cam )
	{






	}





	Camera* mCamera;


};

class SampleBase : public GameLoopT< SampleBase , WindowsPlatform > 
	             , public WinFrameT< SampleBase >
	             , public WindowsMessageHandlerT< SampleBase >
				 , public LogOutput
{
public:
	virtual bool onSetupSample(){ return true; }
	virtual void onExitSample(){}
	virtual bool onMouse( MouseMsg& msg );
	virtual bool onKey( unsigned key , bool isDown );
	virtual long onUpdate( long time ){ return time; }

	virtual void onRenderScene()
	{
		mMainScene->render( mMainCamera , mMainViewport );
	}
	virtual void onShowMessage();

	void  pushMessage( char const* format , ... );
	float getFPS(){ return m_fps; }
	void  createCoorditeAxis( float len );

	void onIdle( long time ){ onRender(); }

private:
	void onRender();
	bool onInit();
	void onEnd(){  onExitSample();  }


	template < class T , class PP >
	friend class GameLoopT;

protected:
	virtual void receiveLog( LogChannel channel , char const* str );

	String          mDevMsg[ 7 ];
	CFly::Camera*   mMainCamera;
	CFly::Viewport* mMainViewport;
	CFly::Scene*    mMainScene;
	CFly::World*    mWorld;

	Vec2i mLastDownPos;

protected:
	int        m_msgPos;
	float      m_fps;
	long       m_frameCount;

};

#define INVOKE_SAMPLE( SampleName ) \
	SampleBase* createSample(){  return new SampleName; }





#endif // SampleBase_h__