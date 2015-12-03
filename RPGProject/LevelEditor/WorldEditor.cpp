#include "WorldEditor.h"

#include "TActor.h"
#include "TMessageShow.h"
#include "TEntityManager.h"
#include "TResManager.h"
#include "TPhySystem.h"
#include "TEventSystem.h"
#include "TLevel.h"
#include "ModelEnum.h"
#include "UtilityFlyFun.h"
#include "TUISystem.h"
#include "profile.h"
#include "TActorTemplate.h"


class DevLog : public IMsgListener
{
public:
	DevLog()
	{
		addChannel( MSG_DEV );
		setLevel( 3 );

		pos = 0;
	}

	void receive( MsgChannel channel , char const* str )
	{
		m_str[ pos++ ] = str;
		pos = pos % 10;
	}

	void show( TMessageShow& msgShow )
	{
		for  ( int i = 0 ; i < 20 ; ++i )
		{
			String& str = m_str[( pos + i ) % 10];
			if ( !str.empty() )
				msgShow.push( str.c_str() );
		}
	}

	int pos;
	String m_str[20];
}g_devLog;


bool WorldEditor::OnInit( WORLDid wID  )
{
	if ( !TGame::OnInit( wID ) )
		return false;

	btDynamicsWorld* phyWorld = TPhySystem::instance().getDynamicsWorld();

	TRoleManager::instance().loadData( ROLE_DATA_PATH );
	getWorldEditor()->init( this );

	openScene("BT_LOWER_TEST");


	g_isEditMode = true;

	return true;
}

void WorldEditor::update( int skip )
{
	btDynamicsWorld* world = TPhySystem::instance().getDynamicsWorld();

	getWorldEditor()->update();

	camView.SetDirectionAlignment( MINUS_Z_AXIS , Y_AXIS );

	camView.SetPosition( (float*) &camControl.getPosition() );

	camView.SetDirection(  camControl.getViewDir() , 
		                   camControl.getAxisZDir() );

	TEventSystem::instance().updateFrame();

	if ( m_player )
		UF_SetTransform( colShapeObj , m_player->getTransform() );

	if ( m_Mode == MODE_RUN )
	{
		TEntityManager::instance().updateFrame();
		//g_Hero->keyHandle();
		g_GlobalVal.update();
		TProfileManager::incrementFrameCount();
	}
}

void WorldEditor::render( int skip )
{

	viewportScreen.Render( camView.Object() , TRUE , TRUE );

	float fps = updateFPS();

	TMessageShow msgShow( m_world );
	msgShow.start();
	msgShow.push( "Fps: %6.2f", fps );
	{
		Vec3D pos = camControl.getPosition();
		Vec3D dir = camControl.getViewDir();
		msgShow.push( "Cam pos = ( %.1f %.1f %.1f ) viewDir = ( %.2f %.2f %.2f )" ,
			pos.x() , pos.y() , pos.z() , dir.x() , dir.y() , dir.z() );
	}


	g_devLog.show( msgShow );

	//if ( EditData* data = m_objEdit.getSelectEditData())
	//{
	//	if ( TActor* actor = TActor::upCast( data->entity ) )
	//	{
	//		Vec3D  pos = actor->getPosition();
	//		msgShow.push( "Hit pos = ( %.1f %.1f %.1f ) " ,
	//			pos.x() , pos.y() ,pos.z()  );
	//		actor->getFlyActor().GetWorldPosition( pos );
	//		msgShow.push( "Hit pos = ( %.1f %.1f %.1f ) " ,
	//			pos.x() , pos.y() ,pos.z()  );
	//	}
	//}
	
	if ( m_player )
	{
		Vec3D dir = m_player->getFaceDir();
		msgShow.push( "viewDir = ( %.2f %.2f %.2f )" , dir.x() , dir.y() , dir.z() );
		m_player->getFlyActor().GetDirection( dir , NULL );
		msgShow.push( "viewDir = ( %.2f %.2f %.2f )" , dir.x() , dir.y() , dir.z() );
		FnObject obj; obj.Object( m_player->getFlyActor().GetBaseObject() );
		msgShow.push( "viewDir = ( %.2f %.2f %.2f )" , dir.x() , dir.y() , dir.z() );
	}

	if ( m_Mode == MODE_RUN )
	{
		TProfileIterator* profIter = TProfileManager::createIterator();

		profIter->getCurNode()->showAllChild( true );
		showProfileInfo( profIter , msgShow , NULL );
		TProfileManager::releaseIterator( profIter );
	}



	msgShow.finish();

	m_world.SwapBuffers();
}

void WorldEditor::openScene( char const* name )
{	
	FnLight lgt;
	lgt.Object( sceneLevel.CreateLight() );
	lgt.Translate(70.0f, -70.0f, 70.0f, REPLACE);
	lgt.SetColor(1.0f, 1.0f, 1.0f);

	camControl.setPosition( Vec3D( 500 , 500 , 500 ) );
	camControl.setLookPos( Vec3D( 0,0,0 ) );

	TLevel* preLevel = m_curLevel;
	TLevel* level = new TLevel( name );

	setCurLevel( level );
	getWorldEditor()->changeLevel( level );

	//level->getFlyTerrain().Show( TRUE );


	delete preLevel;

	if ( !loadLevelData() )
		getWorldEditor()->initEditData();
	
}
