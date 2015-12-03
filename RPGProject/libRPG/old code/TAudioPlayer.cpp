#include "TAudioPlayer.h"

#include "TResManager.h"
#include "TEntityManager.h"
#include "ConsoleSystem.h"

TAudioPlayer::TAudioPlayer()
{
	setGlobal( true );

	m_BGMVolume  = 0.8;
	m_gameVolume = 1.0;

	m_distFactor = 0.1f ;
	m_dopplerFactor = 0.001f; 
	m_rollOffFactor = 0.001f;

	addThink( (fnThink)&TAudioPlayer::compute3DEffectThink , THINK_LOOP , 1.0 );

	ConCommand( "s_distFactor" , &TAudioPlayer::setDistFactor , this );
	ConCommand( "s_dopplerFactor" , &TAudioPlayer::setDopplerFactor , this );
	ConCommand( "s_rollOffFactor" , &TAudioPlayer::setRollOffFactor , this );
}

void TAudioPlayer::init( FnWorld& world , FnCamera& camera )
{
	m_cameraID = camera.Object();
	m_world.Object( world.Object() );
	TEntityManager::instance().addEntity( this );
}

void TAudioPlayer::changeBGMusic( char const* name , bool beFade )
{
	if ( beFade )
	{
		m_world.DeleteAudio( m_chBgMusic );
		m_chBgMusic = TResManager::instance().cloneAudio( name );

		int index = addThink( (fnThink)&TAudioPlayer::changeBGMusicThink );
		setNextThink( index , g_GlobalVal.nextTime );

		m_chStep = 0;
		if ( m_bgMusic.Object() != FAILED_ID )
			m_curBGMVolume = m_BGMVolume;
		else
			m_curBGMVolume = 0;

		m_dVolme = - m_BGMVolume * g_GlobalVal.frameTime / 3;
	}
	else
	{
		m_world.DeleteAudio( m_bgMusic.Object() );
		m_bgMusic.Object( TResManager::instance().cloneAudio( name ) );
		m_bgMusic.SetVolume( m_BGMVolume );
		m_bgMusic.Play( LOOP );
	}
}

void TAudioPlayer::changeBGMusicThink()
{
	m_curBGMVolume += m_dVolme;

	if ( m_chStep == 0 )
	{
		if ( m_curBGMVolume <= 0.2 * m_BGMVolume )
		{
			m_chStep = 1;
			m_curBGMVolume = 0.2 * m_BGMVolume;

			m_world.DeleteAudio( m_bgMusic.Object() );
			m_bgMusic.Object( m_chBgMusic );
			m_chBgMusic = FAILED_ID;

			m_bgMusic.SetVolume( 0 );
			m_bgMusic.Play( LOOP );

			m_dVolme =  m_BGMVolume * g_GlobalVal.frameTime / 2;
		}
		
		setCurContextNextThink( g_GlobalVal.nextTime );
	}
	else if ( m_chStep == 1 )
	{
		if ( m_curBGMVolume < m_BGMVolume )
			setCurContextNextThink( g_GlobalVal.nextTime );
	}

	m_bgMusic.SetVolume( m_curBGMVolume );
}

AUDIOid TAudioPlayer::play( char const* name , float volume , DWORD mode /*= ONCE */ )
{
	FnAudio audio;
	audio.Object( TResManager::instance().cloneAudio( name ) );
	audio.SetVolume( volume * m_gameVolume );
	audio.Play( mode );

	return audio.Object();
}

OBJECTid TAudioPlayer::play3D( OBJECTid objID , char const* name , float volume , DWORD mode /*= ONCE */ )
{
	Fn3DAudio audio;

	audio.Object( TResManager::instance().clone3DAudio( name ) );

	assert( audio.GetType() == AUDIO );

	//audio.SetDistanceRange( 0 , 1000 );
	
	audio.SetParent( objID );
	audio.Translate( 0,0,0, REPLACE );
	audio.SetVolume( volume * m_gameVolume );
	audio.Set3DEffect( m_cameraID );
	audio.Play( mode );
	m_scene.Commit3DAudio();
	
	return audio.Object();
}

OBJECTid TAudioPlayer::play3D( Vec3D const& pos , char const* name , float volume , DWORD mode /*= ONCE */ )
{
	Fn3DAudio audio;

	audio.Object( TResManager::instance().clone3DAudio( name ) );

	assert( audio.GetType() == AUDIO );

	//audio.SetDistanceRange( 0 , 1000 );

	audio.SetVolume( volume * m_gameVolume );
	audio.SetWorldPosition( (float*)&pos[0] );
	audio.Set3DEffect( m_cameraID );
	audio.Play( mode );
	
	return audio.Object();

}

void TAudioPlayer::changeScene( FnScene& scene )
{
	m_scene.Object( scene.Object() );
	m_scene.Set3DAudioParameters( 
		m_distFactor , m_dopplerFactor , m_rollOffFactor );
	m_scene.Commit3DAudio();
}

void TAudioPlayer::compute3DEffectThink()
{
	m_scene.Commit3DAudio();
}

void TAudioPlayer::setDistFactor( float val )
{
	m_distFactor = val; 
	m_scene.Set3DAudioParameters( 
		m_distFactor , m_dopplerFactor , m_rollOffFactor );
}

void TAudioPlayer::setDopplerFactor( float val )
{
	m_dopplerFactor = val;
	m_scene.Set3DAudioParameters( 
		m_distFactor , m_dopplerFactor , m_rollOffFactor );
}

void TAudioPlayer::setRollOffFactor( float val )
{
	m_rollOffFactor = val; 
	m_scene.Set3DAudioParameters( 
		m_distFactor , m_dopplerFactor , m_rollOffFactor );
}

AUDIOid  UG_PlaySound( char const* name , float volume , DWORD mode   )
{
	return TAudioPlayer::instance().play( name , volume , mode );
}
OBJECTid UG_PlaySound3D( OBJECTid objID , char const* name , float volume , DWORD mode )
{
	return TAudioPlayer::instance().play3D( objID , name , volume , mode );
}
OBJECTid UG_PlaySound3D( Vec3D const& pos , char const* name , float volume , DWORD mode  )
{
	return TAudioPlayer::instance().play3D( pos , name , volume , mode );
}
