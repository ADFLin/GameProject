#ifndef AudioPlayer_h__
#define AudioPlayer_h__

#include "ZBase.h"
#include "ZResManager.h"
#include "ZEventHandler.h"

#define  ERROR_SOUND_ID -1

struct FMOD_SYSTEM;
struct FMOD_SOUND;
struct FMOD_CHANNEL;
struct FMOD_CHANNELGROUP;

namespace Zuma
{
	enum ResID;
	typedef unsigned SoundID;

	class SoundRes : public ResBase
	{
	public:
		SoundRes():ResBase( RES_SOUND ){ sound = NULL; }

		void release();
		FMOD_SOUND* sound;
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

		typedef FMOD_CHANNEL Channel;
		Channel*  getChannel( SoundID sID );
		unsigned  registerChannel( Channel* channel );

		float          mSoundVolume;
		float          mMusicVolume;
		FMOD_SYSTEM*   mFmodSys;
		FMOD_CHANNELGROUP* mMasterGroup;
		Channel*       mMusicChannels;
		Channel*       mChannels[ MaxNumChannel ];
		unsigned       mChannelSerial[ MaxNumChannel ];
	};


	SoundID  playSound   ( ResID id , float volume , bool beLoop , float freqFactor );
	SoundID  playSound   ( ResID id , float volume = 1.0f , bool beLoop = false );
	void     setSoundFreq( SoundID sID , float freqFactor );
	void     stopSound   ( SoundID sID );

}//namespace Zuma


#endif // AudioPlayer_h__
