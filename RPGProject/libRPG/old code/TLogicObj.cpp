#include "TLogicObj.h"

#include "TNPCBase.h"
#include "THero.h"
#include "TGame.h"
#include "TLevel.h"
#include "TEntityManager.h"
#include "TResManager.h"
#include "TActorTemplate.h"
#include "TSlotSingalSystem.h"

BEGIN_SLOT_MAP_NOBASE( TLogicObj )
	DEF_SLOT( play )
	DEF_SLOT( active )
	DEF_SLOT( deactive )
	DEF_SLOT( setActive )
END_SLOT_MAP( TLogicObj )

void TLogicObj::play()
{
	if ( m_active )
	{
		doPlay();
	}
}

TLogicObj::TLogicObj()
{
	m_active = true;
}

TSpawnZone::TSpawnZone( Vec3D const& pos ) 
	:pos(pos)
{
	spawnRoleID = 3;
}

void TSpawnZone::doPlay()
{
	XForm trans;
	trans.setIdentity();
	trans.setOrigin( pos );
	TNPCBase* npc = new TNPCBase( spawnRoleID , trans );

	getLiveLevel()->addEntity( npc );
}

void TSpawnZone::setSpawnRoleID( unsigned id )
{
	TRoleData* data = TRoleManager::instance().getRoleData( id );
	if ( data )
		spawnRoleID = id;
}

void TChangeLevel::doPlay()
{
	g_Game->changeLevel( levelName.c_str() );
	getPlayer()->setPosition( playerPos );
}