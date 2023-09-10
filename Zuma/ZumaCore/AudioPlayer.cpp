#include "ZumaPCH.h"
#include "AudioPlayer.h"

#include "Audio/XAudio2/MFDecoder.h"
#include "minivorbis/minivorbis.h"
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

	bool LoadOggFile(char const* path, WaveFormatInfo& outWaveFormat, TArray<uint8>& outSampleData)
	{
		FILE* fp = fopen(path, "rb");
		if (!fp)
		{
			LogWarning(0, "Failed to open file '%s'.", path);
			return false;
		}

		ON_SCOPE_EXIT
		{
			fclose(fp);
		};
		/* Open sound stream. */
		OggVorbis_File vorbis;
		if (ov_open_callbacks(fp, &vorbis, NULL, 0, OV_CALLBACKS_DEFAULT) != 0) 
		{
			LogWarning(0, "Invalid Ogg file '%s'.", path);
			return false;
		}

		ON_SCOPE_EXIT
		{
			/* Close sound file */
			ov_clear(&vorbis);
		};

		/* Print sound information. */
		vorbis_info* info = ov_info(&vorbis, -1);
		//LogMsg("Ogg file %d Hz, %d channels, %d kbit/s.\n", info->rate, info->channels, info->bitrate_nominal / 1024);

		outWaveFormat.tag = WAVE_FORMAT_PCM;
		outWaveFormat.numChannels = info->channels;
		outWaveFormat.sampleRate  = info->rate;
		outWaveFormat.bitsPerSample = 8 * sizeof(int16);
		outWaveFormat.byteRate = outWaveFormat.bitsPerSample * outWaveFormat.sampleRate * outWaveFormat.numChannels / 8;
		outWaveFormat.blockAlign = outWaveFormat.numChannels * outWaveFormat.bitsPerSample / 8;

		/* Read the entire sound stream. */
		unsigned char buf[4096];
		for(;;)
		{
			int section = 0;
			long bytes = ov_read(&vorbis,(char*)buf, sizeof(buf), 0, sizeof(int16), std::is_signed_v<int16>, &section);
			if (bytes <= 0) /* end of file or error */
				break;

			outSampleData.append(buf, buf + bytes);
		}

		return true;
	}


	bool loadResource(SoundRes* res, char const* path)
	{
		char const* ext = FFileUtility::GetExtension(path);

		if (FCString::CompareIgnoreCase(ext, "ogg") == 0)
		{
			if (LoadOggFile(path, res->soundWave.format, res->soundWave.PCMData))
				return true;

			return true;
		}
		else if (FCString::CompareIgnoreCase(ext, "wav") == 0)
		{
			if (LoadWaveFile(path, res->soundWave.format, res->soundWave.PCMData))
				return true;
		}

		if (!FMFDecodeUtil::LoadWaveFile(path, res->soundWave.format, res->soundWave.PCMData))
		{
			LogWarning(0, "Audio file load fail : %s", path);
			return false;
		}

		return true;
	}

	ResBase* AudioPlayer::createResource(ResID id, ResInfo& info)
	{
		SoundResInfo& soundInfo = static_cast<SoundResInfo&>(info);
		SoundRes* res = new SoundRes;
		loadResource(res, info.path.c_str());
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
