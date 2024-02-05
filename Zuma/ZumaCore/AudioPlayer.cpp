#include "ZumaPCH.h"
#include "AudioPlayer.h"

#include "Audio/XAudio2/MFDecoder.h"
#include "Core/ScopeGuard.h"
#include "FileSystem.h"

namespace Zuma
{

	static AudioPlayer* GAudioPlayer = NULL;

	void SoundRes::release()
	{

	}

	AudioPlayer::AudioPlayer()
	{
		GAudioPlayer = this;
		mSoundVolume = 0.5f;
	}

	AudioPlayer::~AudioPlayer()
	{
		cleunup();
	}

	bool AudioPlayer::init()
	{
		mDevice = AudioDevice::Create();
		if (mDevice == nullptr)
		{
			return false;
		}
		return true;
	}

	void AudioPlayer::cleunup()
	{
		delete mDevice;
		mDevice = nullptr;
	}

	bool loadResource(SoundRes* res, SoundResInfo const& info)
	{

		if (info.flag & SND_FMT_OGG)
		{
			if (LoadOggFile(info.path.c_str(), res->soundWave.format, res->soundWave.PCMData))
				return true;

			return true;
		}
		else if (info.flag & SND_FMT_WAV)
		{
			if (LoadWaveFile(info.path.c_str(), res->soundWave.format, res->soundWave.PCMData))
				return true;
		}

		if (!FMFDecodeUtil::LoadWaveFile(info.path.c_str(), res->soundWave.format, res->soundWave.PCMData))
		{
			LogWarning(0, "Audio file load fail : %s", info.path.c_str());
			return false;
		}

		return true;
	}

	ResBase* AudioPlayer::createResource(ResID id, ResInfo& info)
	{
		SoundResInfo& soundInfo = static_cast<SoundResInfo&>(info);
		SoundRes* res = new SoundRes;
		loadResource(res, soundInfo);
		res->volume = soundInfo.volume;
		return res;
	}

	SoundID AudioPlayer::playSound( ResID id , float volume , bool beLoop )
	{
		SoundRes* res = getSound( id );
		if ( res == NULL )
			return ERROR_SOUND_ID;

		return mDevice->playSound2D(res->soundWave, res->volume * volume, beLoop);
	}

	SoundID AudioPlayer::playSound( ResID id , float volume , bool beLoop , float freqFactor )
	{
		SoundRes* res = getSound( id );
		if ( res == NULL )
			return ERROR_SOUND_ID;

		SoundID handle = mDevice->playSound2D(res->soundWave, res->volume * volume, beLoop);
		mDevice->setAudioPitchMultiplier(handle, freqFactor);
		return handle;
	}

	SoundRes* AudioPlayer::getSound( ResID id )
	{
		ResBase* res = getResource( id );
		assert( res == NULL || res->getType() == RES_SOUND );
		return ( SoundRes* ) res;
	}

	void AudioPlayer::update( unsigned time )
	{
		if (mDevice == nullptr)
			return;

		mDevice->update(float(time) / 1000.0f);
	}

	void AudioPlayer::stopSound( SoundID sID )
	{
		if ( sID == ERROR_SOUND_ID )
			return;

		mDevice->stopSound(sID);
	}

	void AudioPlayer::setSoundFreq( SoundID sID , float freqFactor )
	{
		if ( sID == ERROR_SOUND_ID )
			return;

		mDevice->setAudioPitchMultiplier(sID, freqFactor);
	}

	bool AudioPlayer::onEvent( int evtID , ZEventData const& data , ZEventHandler* sender )
	{
		return true;
	}

	SoundID playSound( ResID id , float volume , bool beLoop )
	{
		return GAudioPlayer->playSound( id , volume , beLoop );
	}

	SoundID playSound( ResID id , float volume , bool beLoop , float freqFactor )
	{
		return GAudioPlayer->playSound( id , volume , beLoop ,freqFactor );
	}

	void setSoundFreq(SoundID sID, float freqFactor)
	{
		GAudioPlayer->setSoundFreq(sID, freqFactor);
	}

	void stopSound(SoundID sID)
	{
		GAudioPlayer->stopSound( sID );
	}

}//namespace Zuma
