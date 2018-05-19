#include "ZumaPCH.h"
#include "AudioPlayer.h"

#include "fmod.hpp"
#include "fmod_errors.h"


#define FMOD_CHECK( FUN )\
	{\
	FMOD_RESULT  result = FUN;\
	ERRCHECK(result);\
}\

namespace Zuma
{

	static AudioPlayer* gAudioPlayer = NULL;

	static FMOD_RESULT F_CALLBACK ChannelCallBack(
		FMOD_CHANNEL *channel,
		FMOD_CHANNEL_CALLBACKTYPE type,
		void *commanddata1, void *commanddata2 )
	{
		switch( type )
		{
		case FMOD_CHANNEL_CALLBACKTYPE_END:
			break;

		}

		return FMOD_OK;
	}

	static void ERRCHECK(FMOD_RESULT result)
	{
		if (result != FMOD_OK)
		{
			assert( 0 );
			printf("FMOD error! (%d) %s\n", result, FMOD_ErrorString(result));
			::exit(-1);
		}
	}

	void SoundRes::release()
	{
		FMOD_CHECK( FMOD_Sound_Release( sound ) );
		sound = NULL;
	}

	AudioPlayer::AudioPlayer()
	{
		mFmodSys = NULL;

		gAudioPlayer = this;
		mSoundVolume = 0.5f;

		std::fill_n( mChannels , MaxNumChannel , (FMOD_CHANNEL*)0 );
	}

	AudioPlayer::~AudioPlayer()
	{
		cleunup();
	}

	bool AudioPlayer::init()
	{
		unsigned int      version;

		FMOD_CHECK( FMOD_System_Create( &mFmodSys ) );
		FMOD_CHECK( FMOD_System_GetVersion( mFmodSys , &version ) );

		if (version < FMOD_VERSION)
		{
			LogError("Error!  You are using an old version of FMOD %08x.  This program requires %08x\n", version, FMOD_VERSION);
			return false;
		}

		FMOD_CHECK( FMOD_System_Init( mFmodSys , MaxNumChannel , FMOD_INIT_NORMAL, NULL ) );
		return true;
	}

	void AudioPlayer::cleunup()
	{
		if ( mFmodSys )
		{
			FMOD_CHECK( FMOD_System_Close( mFmodSys ) );
			FMOD_CHECK( FMOD_System_Release( mFmodSys ) );
			mFmodSys = NULL;
		}
	}


	ResBase* AudioPlayer::createResource( ResID id , ResInfo& info )
	{
		SoundResInfo& soundInfo = static_cast< SoundResInfo& >( info );
		SoundRes* res = new SoundRes;

		FMOD_CHECK( FMOD_System_CreateSound( mFmodSys , info.path.c_str() , FMOD_DEFAULT , 0 , &res->sound ) );

		res->volume = soundInfo.volume;

		return res;
	}


	SoundID AudioPlayer::playSound( ResID id , float volume , bool beLoop )
	{
		SoundRes* res = getSound( id );
		if ( res == NULL )
			return ERROR_SOUND_ID;

		FMOD_CHANNEL* channel;
		FMOD_System_PlaySound( mFmodSys , FMOD_CHANNEL_FREE , res->sound , false , &channel );
		FMOD_Channel_SetVolume( channel , volume * res->volume * mSoundVolume );
		FMOD_Channel_SetCallback( channel ,ChannelCallBack );

		if ( beLoop )
			FMOD_Channel_SetMode( channel ,FMOD_LOOP_NORMAL );

		return registerChannel( channel );
	}

	SoundID AudioPlayer::playSound( ResID id , float volume , bool beLoop , float freqFactor )
	{
		SoundRes* res = getSound( id );
		if ( res == NULL )
			return ERROR_SOUND_ID;

		FMOD_CHANNEL* channel;
		FMOD_System_PlaySound( mFmodSys , FMOD_CHANNEL_FREE , res->sound , false , &channel );
		FMOD_Channel_SetVolume( channel , volume * res->volume * mSoundVolume );
		FMOD_Channel_SetCallback( channel ,ChannelCallBack );

		float freq;
		FMOD_Channel_GetFrequency( channel , &freq );
		FMOD_Channel_SetFrequency( channel , freqFactor * freq );

		if ( beLoop )
			FMOD_Channel_SetMode( channel ,FMOD_LOOP_NORMAL );

		return registerChannel( channel );
	}

	SoundRes* AudioPlayer::getSound( ResID id )
	{
		ResBase* res = getResource( id );
		assert( res == NULL || res->getType() == RES_SOUND );
		return ( SoundRes* ) res;
	}

	void AudioPlayer::update( unsigned time )
	{
		FMOD_CHECK( FMOD_System_Update( mFmodSys ) );

		for( unsigned i = 0 ; i < MaxNumChannel ; ++i )
		{
			if ( mChannels[ i ] == NULL )
				continue;

			FMOD_BOOL isPlaying;
			if ( FMOD_Channel_IsPlaying( mChannels[i] , &isPlaying ) == FMOD_OK )
			{
				if ( !isPlaying )
					mChannels[i] = NULL;
			}
		}
	}

	void AudioPlayer::stopSound( SoundID sID )
	{
		if ( sID == ERROR_SOUND_ID )
			return;

		Channel* channel = getChannel( sID );
		if ( !channel )
			return;

		FMOD_CHECK( FMOD_Channel_Stop( channel ) );
	}

	void AudioPlayer::setSoundFreq( SoundID sID , float freqFactor )
	{
		if ( sID == ERROR_SOUND_ID )
			return;

		Channel* channel = getChannel( sID );
		if ( !channel )
			return;

		float freq;
		FMOD_Channel_GetFrequency( channel , &freq );
		FMOD_Channel_SetFrequency( channel , freqFactor * freq );
	}



	namespace
	{
		union SSID
		{
			struct
			{
				unsigned channel : 8;
				unsigned serial  :24;
			};
			unsigned  value;
		};

		unsigned gSerialNumber = 0;
	}



	unsigned AudioPlayer::registerChannel( Channel* channel )
	{

		for( unsigned i = 0 ; i < MaxNumChannel ; ++i )
		{
			if ( mChannels[ i ] )
				continue;

			mChannels[i] = channel;
			mChannelSerial[i] = gSerialNumber;
			++gSerialNumber;

			SSID id;
			id.channel = i;
			id.serial  = gSerialNumber;
			return id.value;

		}
		return ERROR_SOUND_ID;
	}

	AudioPlayer::Channel* AudioPlayer::getChannel( unsigned sID )
	{
		SSID id;
		id.value = sID;
		if ( mChannelSerial[ id.channel ] != id.serial )
			return NULL;
		return mChannels[ id.channel ];
	}


	bool AudioPlayer::onEvent( int evtID , ZEventData const& data , ZEventHandler* sender )
	{
		return true;
	}

	SoundID playSound( ResID id , float volume , bool beLoop )
	{
		return gAudioPlayer->playSound( id , volume , beLoop );
	}

	SoundID playSound( ResID id , float volume , bool beLoop , float freqFactor )
	{
		return gAudioPlayer->playSound( id , volume , beLoop ,freqFactor );
	}

	void stopSound( SoundID sID )
	{
		gAudioPlayer->stopSound( sID );
	}

}//namespace Zuma
