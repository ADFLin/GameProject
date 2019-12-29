#include "SampleBase.h"
#include "UnitTest.h"
#include "CFPluginManager.h"
#include "Cw3FileLoader.h"

SampleBase* createSample();
static SampleBase* g_game = nullptr;
static HINSTANCE g_hInstance = NULL;
static PSTR      g_szCmdLine = nullptr;

int WINAPI WinMain (HINSTANCE hInstance, HINSTANCE hPrevInstance, PSTR szCmdLine, int iCmdShow)
{
	UT_RUN_TEST();

	g_hInstance = hInstance;
	g_szCmdLine = szCmdLine;

	g_game = createSample();
	g_game->setUpdateTime( 1000 / 30 );
	g_game->run();

	delete g_game;
	return 0;
}


bool SampleBase::initializeGame()
{
	m_frameCount = 0;
	m_fps = 0.0f;

	addChannel( LOG_DEV );

	if ( !WinFrame::create( "Sample Test" , g_ScreenWidth , g_ScreenHeight ,
		   WindowsMessageHandler::MsgProc , false ) )
		return false;

	if ( !CFly::initSystem() )
		return  false;
	mWorld =  CFly::createWorld( getHWnd() , g_ScreenWidth , g_ScreenHeight , 32 , false );

	CFly::PluginManager::Get().registerLinker( "CW3" , new Cw3FileLinker );

	if ( !mWorld )
		return false;

	mWorld->setDir( DIR_OBJECT , SAMPLE_DATA_DIR );
	mWorld->setDir( DIR_ACTOR  , SAMPLE_DATA_DIR"/NPC" );
	mWorld->setDir( DIR_TEXTURE, SAMPLE_DATA_DIR );
	mWorld->setDir( DIR_SHADER , SAMPLE_DATA_DIR"/Shader" );
	mMainViewport = mWorld->createViewport( 0 , 0 , g_ScreenWidth , g_ScreenHeight );
	mMainScene    = mWorld->createScene( 1 );
	mMainScene->setAmbientLight( Color4f(1,1,1) );

	mMainCamera = mMainScene->createCamera();
	mMainCamera->setAspect( float(  g_ScreenWidth ) / g_ScreenHeight );
	mMainCamera->setNear(5.0f);
	mMainCamera->setFar(100000.0f);

	return onSetupSample();
}

bool SampleBase::handleMouseEvent( MouseMsg const& msg )
{
	if ( msg.onLeftDown() )
	{
		mLastDownPos = msg.getPos();
	}
	else if ( msg.isDraging() && msg.isLeftDown() )
	{
		Quaternion q;
		q.setMatrix( mMainCamera->getLocalTransform() );
		Vector3 rot = q.getEulerZYX();

		rot.z += 0.01 *( mLastDownPos.x - msg.getPos().x );
		rot.x += 0.01 *( mLastDownPos.y - msg.getPos().y );

		Quaternion q2;
		q2.setEulerZYX( rot.z , rot.y , rot.x );

		Matrix4 mat;
		mat.setTransform( mMainCamera->getLocalPosition() , q2 );

		mMainCamera->setLocalTransform( mat );
		//mMainCamera->rotate( CF_AXIS_Y , 0.01 *( mLastDownPos.x - msg.getPos().x ) , CFTO_LOCAL );
		//mMainCamera->rotate( CF_AXIS_X , -0.01 *( mLastDownPos.y - msg.getPos().y ) , CFTO_LOCAL );
		mLastDownPos = msg.getPos();
	}
	return true;
}

bool SampleBase::handleKeyEvent(KeyMsg const& msg)
{
	float moveSpeed = 10;
	if ( !msg.isDown() )
		return false;

	switch( msg.getCode() )
	{
	case EKeyCode::Escape: setLoopOver( true ); break;
	case EKeyCode::W: mMainCamera->translate( Vector3( 0,0,-moveSpeed ) , CFTO_LOCAL ); break;
	case EKeyCode::S: mMainCamera->translate( Vector3( 0,0,moveSpeed ) , CFTO_LOCAL ); break;
	case EKeyCode::A: mMainCamera->translate( Vector3( -moveSpeed ,0 , 0 ) , CFTO_LOCAL ); break;
	case EKeyCode::D: mMainCamera->translate( Vector3( moveSpeed ,0 , 0 ) , CFTO_LOCAL ); break;
	}

	return true;
}

void SampleBase::handleGameRender()
{
	onRenderScene();

	static float fps = 0.0f;
	static long time = getMillionSecond();
	++m_frameCount;
	if ( m_frameCount > 50 )
	{
		m_fps = 1000.0f * ( m_frameCount ) / ( getMillionSecond() - time );
		time = getMillionSecond();
		m_frameCount = 0;
	}

	m_msgPos = 10;
	mWorld->beginMessage();
	onShowMessage();
	mWorld->endMessage();

	mWorld->swapBuffers();
}

void SampleBase::pushMessage( char const* format , ... )
{
	char str[256];

	va_list argptr;
	va_start( argptr, format );
	vsprintf_s( str , format , argptr );
	va_end( argptr );

	mWorld->showMessage( 10 , m_msgPos , str , 255 , 0 , 0 );
	m_msgPos += 15;
}

void SampleBase::createCoorditeAxis( float len )
{
	Object* obj = mMainScene->createObject( nullptr );

	Material* mat;

	Vector3 line[2];
	line[0] = Vector3(0,0,0);

	line[1] = Vector3( len ,0,0 );
	mat = mWorld->createMaterial( NULL , NULL , NULL , 0 , Color4f(1,0,0) );
	obj->createLines( mat ,CF_LINE_SEGMENTS , (float*) &line[0] , 2  );

	line[1] = Vector3(0,len,0);
	mat = mWorld->createMaterial( NULL , NULL , NULL , 0 , Color4f(0,1,0) );
	obj->createLines( mat ,CF_LINE_SEGMENTS , (float*) &line[0]  , 2 );

	line[1] = Vector3(0,0,len );
	mat = mWorld->createMaterial( NULL , NULL , NULL , 0 , Color4f(0,0,1) );
	obj->createLines( mat , CF_LINE_SEGMENTS , (float*) &line[0] , 2 );

	obj->setRenderOption( CFRO_LIGHTING , true );
}

void SampleBase::receiveLog( LogChannel channel , char const* str )
{
	static int idx = 0;
	mDevMsg[ idx ] = str;
	++idx;
	if ( idx >= 7 )
		idx = 0;
}

void SampleBase::onShowMessage()
{
	pushMessage( "fps= %f" , getFPS() );

	for( int i = 0 ; i < 7 ; ++i )
		pushMessage( mDevMsg[i].c_str() );
}
