#ifndef AudioPlayer_h__
#define AudioPlayer_h__

#include "ZBase.h"
#include "ZResManager.h"
#include "ZEventHandler.h"

#include "Audio/AudioDevice.h"

namespace Zuma
{
	enum ResID;

	using SoundID = AudioHandle;

#define  ERROR_SOUND_ID ERROR_AUDIO_HANDLE

	class SoundRes : public ResBase
	{
	public:
		SoundRes():ResBase( RES_SOUND )
		{ 
			volume = 1.0;
		}

		void release();

		SoundWave   soundWave;
		float       volume;
	};

	class AudioPlayer : public ResLoader
		              , public ZEventHandler
	{
	public:
		AudioPlayer();
		~AudioPlayer();

		bool      init();
		void      cleunup();
		void      update( unsigned time );
		ResBase*  createResource( ResID id , ResInfo& info );
		SoundRes* getSound( ResID id );


		SoundID   playSound( ResID id , float volume , bool beLoop = false );
		SoundID   playSound( ResID id , float volume , bool beLoop , float freqFactor );

		void      stopSound( SoundID sID );
		void      setSoundFreq( SoundID sID , float freqFactor );
		static int const MaxNumChannel = 32;

		float     getSoundVolume() const { return mSoundVolume; }
		void      setSoundVolume( float val ) { mSoundVolume = val; }
		float     getMusicVolume() const { return mMusicVolume; }
		void      setMusicVolume( float val ) { mMusicVolume = val; }

		bool      onEvent( int evtID , ZEventData const& data , ZEventHandler* sender );


		float          mSoundVolume;
		float          mMusicVolume;

		AudioDevice*   mDevice = nullptr;

	};


	SoundID  playSound   ( ResID id , float volume , bool beLoop , float freqFactor );
	SoundID  playSound   ( ResID id , float volume = 1.0f , bool beLoop = false );
	void     setSoundFreq( SoundID sID , float freqFactor );
	void     stopSound   ( SoundID sID );

}//namespace Zuma


#endif // AudioPlayer_h__
