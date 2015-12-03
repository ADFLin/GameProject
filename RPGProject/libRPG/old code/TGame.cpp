#include "TGame.h"
#include "TResManager.h"
#include "TPhySystem.h"
#include "TEntityManager.h"
#include "TEventSystem.h"
#include "TItemSystem.h"
#include "TAudioPlayer.h"
#include "TShadow.h"
#include "TFxPlayer.h"
#include "TUISystem.h"
#include "TSkill.h"
#include "TGameStage.h"
#include "TLevel.h"
#include "debug.h"

#include "TObjectEditor.h"
#include "THero.h"//player

GlobalVal g_GlobalVal(30);
TGame* g_Game = NULL;
bool   g_exitGame = false;


class GameErrorLog : public TMsgProxy
{
public:
	GameErrorLog()
	{
		setCurrentOutput( MSG_ERROR );
	}
	void receive( char const* str )
	{
		MessageBox( NULL , str , "Game Error" , MB_OK );
	}
};

GameErrorLog errorlog;

TGame::TGame() 
	:m_curLevel(NULL)
	,m_fps(0)
	,m_frame(0)
{
	g_Game = this;
	::FyUseHighZ( TRUE );

	m_curStage = new TEmptyStage;
	m_StageMap[ m_curStage->getStateType() ] = m_curStage;
	TEntityManager::instance().addEntity( m_curStage , false );
}

TGame::~TGame()
{
	TFxPlayer::instance().clear();
	TSkillLibrary::instance().clear();
	TItemManager::instance().clear();

}

bool TGame::OnInit( WORLDid wID )
{
	m_world.Object( wID );

	VIEWPORTid vID = m_world.CreateViewport( 0, 0, g_ScreenWidth , g_ScreenHeight );
	viewportScreen.Object( vID );

	viewportScreen.TurnFogOn( FALSE );
	viewportScreen.SetFogMode( LINEAR_FOG );
	viewportScreen.SetExpFogDensity( 1 );
	viewportScreen.SetLinearFogRange( 1000 , 6000 );
	viewportScreen.SetBackgroundColor( 0.7 , 0.7 ,0.7 , 0 );

	TResManager::instance().init( m_world );
	if ( !TResManager::instance().loadRes( RES_DATA_PATH ) )
	{
		ErrorMsg( "can't load Res" );
		return false;
	}

	if ( !TItemManager::instance().loadItemData(ITEM_DATA_PATH) )
	{
		ErrorMsg( "can't load Item Data in %s" , ITEM_DATA_PATH );
	}

	TSkillLibrary::instance().init();

	TUISystem::instance().init( m_world );
	
	camView.Object( TUISystem::instance().getBackScene().CreateCamera() );
	camView.SetNear(5.0f);
	camView.SetFar(100000.0f);
	camView.SetDirectionAlignment( MINUS_Z_AXIS , Y_AXIS );
	camView.SetDirection( Vec3D(1,0,0) , Vec3D( 0,0,1 ) );

	TAudioPlayer::instance().init( m_world , camView );

	TObjectEditor* editor = new TObjectEditor;
	editor->init( this );
	
	return true;
}

void TGame::update( int skip )
{
	TEntityManager::instance().updateFrame();
	TEventSystem::instance().updateFrame();
	
	getCurStage()->update();

	camView.SetWorldPosition( (float*) &camControl.getPosition()  );

	camView.SetDirection(   camControl.getViewDir() , 
		                    camControl.getAxisZDir() );
	g_GlobalVal.update();
}

void TGame::setCurLevel( TLevel* level )
{
	assert( level != NULL );

	TLevel* prevLevel = m_curLevel;
	if ( prevLevel )
		prevLevel->OnLeaveLevel( level );

	TResManager::instance().setCurScene( level->getFlyScene() );

	sceneLevel.Object( level->getFlyScene().Object() );
	camView.ChangeScene( level->getFlyScene().Object() );
	TAudioPlayer::instance().changeScene( level->getFlyScene() );
	TShadowManager::instance().changeLevel( level );
	
	m_curLevel = level;
	m_curLevel->OnEnterLevel( prevLevel );

	//for ( int i = 0 ; i < g_EditData.size() ; ++i )
	//{
	//	delete g_EditData[i];
	//}
	//g_EditData.clear();

}

void TGame::changeLevel( char const* levelName )
{
	TLevel* level = new TLevel( levelName );

	TEntityManager::instance().addEntity( level );
	setCurLevel( level );

	loadLevelData();

	TMainStage* stage = (TMainStage*) m_StageMap[ GS_MAIN ];
	stage->OnChangeLevel( level );
}

void TGame::showLevelObj( bool isShow )
{
	std::vector< OBJECTid >& lvObjVec = getCurLevel()->getLevelObjVec();
	for( int i = 0 ; i < lvObjVec.size() ; ++i )
	{
		FnObject obj; obj.Object( lvObjVec[i] );
		obj.Show( isShow );
	}
}

float TGame::updateFPS()
{
	if ( m_frame == 0 ) 
		FyTimerReset(0);

	if ( m_frame / 10*10 == m_frame ) 
	{
		float curTime = FyTimerCheckTime(0);
		m_fps = m_frame/curTime;
	}

	++m_frame;

	if ( m_frame >= 1000) 
		m_frame = 0;

	return m_fps;
}

void TGame::changeScreenSize( int x , int y )
{
	m_world.Resize( x , y );
	viewportScreen.SetSize( x , y );
	camView.SetAspect( float( x ) / y );
	TUISystem::instance().changetScreenSize( x, y );
	//camView.SetScreenRange( 0 , 0 , x , y );
}

bool TGame::changeStage( StageType stateType , bool beReset /*= false */ )
{
	StageMap::iterator iter = m_StageMap.find( stateType );
	if ( iter != m_StageMap.end() )
	{
		
		TEntityManager::instance().removeEntity( m_curStage , false );

		TGameStage* oldStage = m_curStage;
		m_curStage = iter->second;

		if ( beReset )
		{
			oldStage->clear();
			m_curStage->reset();
		}

		TEntityManager::instance().addEntity( m_curStage , false );
		return true;
	}
	return false;
}

void TGame::addStage( TGameStage* stage )
{
	assert( stage->getStateType() != m_curStage->getStateType() );

	delete m_StageMap[ stage->getStateType() ];
	m_StageMap[ stage->getStateType() ] = stage;
}

bool TGame::OnMessage( char c )
{
	return getCurStage()->OnMessage( c );
}

bool TGame::OnMessage( unsigned key , bool isDown )
{
	return getCurStage()->OnMessage( key , isDown );
}

bool TGame::OnMessage( MouseMsg& msg )
{
	return getCurStage()->OnMessage( msg );
}

void TGame::exit()
{
	g_exitGame = true;
}

Vec3D TGame::computeScreenPos( Vec3D const& v )
{
	Vec3D out;

	viewportScreen.ComputeScreenPosition(
		camView.Object() , out , (float*)&v[0] ,
		PHYSICAL_SCREEN , TRUE );

	return out;
}

Vec3D TGame::computeHitRayDir( TVec2D const& hitPos )
{
	Vec3D pos;

	viewportScreen.ConvertScreenToWorldPos( 
		camView.Object() , hitPos.x , hitPos.y , pos , NULL , Z_AXIS  );

	Vec3D dir = pos - camControl.getPosition();
	DevMsg( 5 , "ray dir = %.2f %.2f %.2f" , dir.x() , dir.y() , dir.z() );

	return  dir;
}

void TGame::render3DScene()
{
	TShadowManager::instance().render();
	viewportScreen.Render( camView.Object() , TRUE , TRUE );
}

void TGame::renderUI()
{
	TUISystem::instance().render( viewportScreen );
}

bool TGame::loadLevelData()
{
	TString const& path = getCurLevel()->getMapDir();
	TString const& name = getCurLevel()->getMapName();
	TString file = path + "/" + name + ".oe" ;

	return TObjectEditor::loadEditData( file.c_str() , getCurLevel() );
}

FnWorld&   UF_GetWorld()
{
	return g_Game->getWorld();
}

FnViewport& UF_GetScreenViewport()
{
	return g_Game->getScreenViewport();
}

FnMaterial UF_CreateMaterial()
{
	FnMaterial mat;
	mat.Object( UF_GetWorld().CreateMaterial( NULL, NULL, NULL,1, NULL) );
	return mat;
}
