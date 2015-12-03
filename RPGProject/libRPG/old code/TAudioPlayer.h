#ifndef TAudioPlayer_h__
#define TAudioPlayer_h__

#include "common.h"
#include "TEntityBase.h"

class TPhyEntity;

class TAudioPlayer : public TEntityBase
	               , public Singleton< TAudioPlayer >
{
public:
	TAudioPlayer();
	void init( FnWorld& world , FnCamera& camera );
	AUDIOid play( char const* name ,  float volume  , DWORD mode = ONCE );

	OBJECTid play3D( Vec3D const& pos , char const* name , 
		             float volume , DWORD mode = ONCE );
	OBJECTid play3D( OBJECTid objID , char const* name , 
		             float volume , DWORD mode =  ONCE  );
	void  changeBGMusic( char const* name , bool beFade = false );
	float getBGMVolume() const { return m_BGMVolume; }
	void  setBGMVolume(float val) { m_BGMVolume = val; }
	float getGameVolume() const { return m_gameVolume; }
	void  setGameVolume(float val) { m_gameVolume = val; }

	void changeScene( FnScene& scene );

	void setDistFactor(float val);
	void setDopplerFactor(float val);
	void setRollOffFactor(float val);

private:
	enum SoundType
	{
		SOUND_3DAUDIO,
		SOUND_MADIA,
		SOUND_AUDIO,
	};

	void compute3DEffectThink();
	void changeBGMusicThink();

	OBJECTid m_cameraID;

	float   m_BGMVolume;
	float   m_gameVolume;

	FnWorld m_world;
	FnScene m_scene;

	int     m_chStep;
	float   m_curBGMVolume;
	float   m_dVolme;

	float   m_distFactor;
	float   m_dopplerFactor;
	float   m_rollOffFactor;

	
	AUDIOid m_chBgMusic;
	FnAudio m_bgMusic;
};

#endif // TAudioPlayer_h__