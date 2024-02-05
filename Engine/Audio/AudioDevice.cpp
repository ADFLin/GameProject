#include "AudioDevice.h"

#include "MarcoCommon.h"

#include "ProfileSystem.h"
#include "Core/ScopeGuard.h"

#include <cassert>

#include "minivorbis/minivorbis.h"

void SoundDSP::addInstance(uint64 hash, SoundWave& soundwave)
{
	SoundInstance* instance = mDevice->findInstance(hash);

	if( instance == nullptr )
	{
		instance = mDevice->registerInstance(hash, soundwave);
		instance->activeSound = mActiveSound;
		if( instance == nullptr )
			return;
	}
	instance->usageFrame = mDevice->mSourceUsageFrame;
	instance->bUsed = true;
	instance->volume = mData.volume * mActiveSound->volumeMultiplier;
	instance->pitch = mData.pitch * mActiveSound->pitchMultiplier;
}

bool AudioDevice::initialize()
{
	return true;
}

void AudioDevice::shutdown()
{
	stopAllSound();
	for( auto channel : mAudioSources )
	{
		delete channel;
	}
	mAudioSources.clear();
}

AudioHandle AudioDevice::playSound2D(SoundBase& sound, float volumeMultiplier, bool bLoop)
{
	ActiveSound* activeSound = createAtiveSound();
	activeSound->sound = &sound;
	activeSound->volumeMultiplier = volumeMultiplier;
	activeSound->bLoop = bLoop;
	activeSound->bFinished = false;
	activeSound->bPlay2d = true;

	return activeSound->handle;
}

AudioHandle AudioDevice::playSound3D(SoundBase& sound, float volumeMultiplier, bool bLoop)
{
	ActiveSound* activeSound = createAtiveSound();
	activeSound->sound = &sound;
	activeSound->volumeMultiplier = volumeMultiplier;
	activeSound->bLoop = bLoop;
	activeSound->bFinished = false;
	activeSound->bPlay2d = false;

	return activeSound->handle;
}

void AudioDevice::setAudioVolumeMultiplier(AudioHandle handle, float value)
{
	ActiveSound* activeSound = getActiveSound(handle);
	if( activeSound )
	{
		activeSound->volumeMultiplier = value;
	}
}

void AudioDevice::setAudioPitchMultiplier(AudioHandle handle, float value)
{
	ActiveSound* activeSound = getActiveSound(handle);
	if( activeSound )
	{
		activeSound->pitchMultiplier = value;
	}
}

bool AudioDevice::isPlaying(AudioHandle handle)
{
	ActiveSound* activeSound = getActiveSound(handle);
	if (activeSound)
	{
		return activeSound->isPlaying();
	}
	return false;
}

void AudioDevice::update( float deltaT )
{
	PROFILE_ENTRY("AudioDevice Update");

	++mSourceUsageFrame;
	if( mSourceUsageFrame == 0 )
	{
		for( auto pair : mPlayingInstances )
		{
			pair.second->usageFrame = 0;
		}
	}

	for( auto iter = mActiveSoundMap.begin(); iter != mActiveSoundMap.end(); ++iter)
	{
		ActiveSound* activeSound = iter->second;

		SoundDSP dsp;
		dsp.mDevice = this;
		dsp.mActiveSound = activeSound;
		activeSound->sound->process(dsp, (intptr_t)activeSound);
	}

	for( auto iter = mPlayingInstances.begin(); iter != mPlayingInstances.end();)
	{
		SoundInstance* instance = iter->second;

		if( instance->usageFrame != mSourceUsageFrame || instance->activeSound == nullptr )
		{
			if( instance->sourceId != INDEX_NONE )
			{
				mAudioSources[instance->sourceId]->endPlay();
				mIdleSources.push_back(instance->sourceId);
			}

			if ( instance->activeSound )
				instance->activeSound->removeInstance(instance);

			delete instance;

			iter = mPlayingInstances.erase(iter);
		}
		else
		{
			if( instance->sourceId == INDEX_NONE)
			{
				instance->bPlaying = false;
				instance->sourceId = fetchIdleSource(*instance);

				if( instance->sourceId != INDEX_NONE)
				{
					if( mAudioSources[instance->sourceId]->initialize(*instance) )
					{
						instance->bPlaying = true;
						instance->activeSound->playingInstances.push_back(instance);
					}
				}
			}

			if( instance->bPlaying )
			{
				mAudioSources[instance->sourceId]->update(deltaT);
				++iter;
			}
			else
			{
				if( instance->sourceId != INDEX_NONE)
				{
					mIdleSources.push_back(instance->sourceId);
				}

				instance->activeSound->removeInstance(instance);
				delete instance;

				iter = mPlayingInstances.erase(iter);
			}

		}
	}


	for( auto iter = mActiveSoundMap.begin(); iter != mActiveSoundMap.end(); )
	{
		ActiveSound* activeSound = iter->second;
		if( !activeSound->isPlaying() )
		{
			destroyActiveSound(activeSound);
			iter = mActiveSoundMap.erase(iter);
		}
		else
		{
			++iter;
		}
	}
}

SoundInstance* AudioDevice::registerInstance(uint64 hash, SoundWave& soundwave)
{
	assert(findInstance(hash) == nullptr);
	SoundInstance* instance = new SoundInstance;
	instance->soundwave = &soundwave;
	auto pair = mPlayingInstances.insert({ hash , instance });

	return pair.first->second;
}

SoundInstance* AudioDevice::findInstance(uint64 hash)
{
	auto iter = mPlayingInstances.find(hash);
	if( iter == mPlayingInstances.end() )
		return nullptr;

	return iter->second;
}

int AudioDevice::fetchIdleSource(SoundInstance& instance)
{
	if( !mIdleSources.empty() )
	{
		int channelId = mIdleSources.back();
		mIdleSources.pop_back();
		return channelId;
	}

	if( mAudioSources.size() < mMaxChannelNum )
	{
		AudioSource* source = createSource();
		if (source)
		{
			int channelId = mAudioSources.size();
			mAudioSources.push_back(createSource());
			return channelId;
		}
		else
		{
			LogWarning(0, "Can't Create Audio Source");
		}
	}
	return INDEX_NONE;
}

void AudioDevice::stopSound(AudioHandle handle)
{
	if (handle == ERROR_AUDIO_HANDLE)
		return;

	auto iter = mActiveSoundMap.find(handle);
	if (iter != mActiveSoundMap.end())
	{
		ActiveSound* activeSound = iter->second;
		destroyActiveSound(activeSound);
		mActiveSoundMap.erase(iter);
	}
}

void AudioDevice::stopAllSound()
{
	for( auto iter = mActiveSoundMap.begin(); iter != mActiveSoundMap.end(); )
	{
		ActiveSound* activeSound = iter->second;
		destroyActiveSound(activeSound);

		iter = mActiveSoundMap.erase(iter);
	}
}

ActiveSound* AudioDevice::getActiveSound(AudioHandle handle)
{
	if( handle == ERROR_AUDIO_HANDLE )
		return nullptr;
	auto iter = mActiveSoundMap.find(handle);
	if( iter != mActiveSoundMap.end() )
		return iter->second;

	return nullptr;
}

ActiveSound* AudioDevice::createAtiveSound()
{
	ActiveSound* activeSound = new ActiveSound;
	activeSound->handle = mNextHandleId++;

	mActiveSoundMap.insert({ activeSound->handle, activeSound });

	return activeSound;
}

void AudioDevice::destroyActiveSound(ActiveSound* activeSound)
{
	//mActiveSoundMap.erase(activeSound->handle);

	if( !activeSound->bFinished )
	{
		for( auto soundInstance : activeSound->playingInstances )
		{
			soundInstance->activeSound = nullptr;
		}
	}

	delete activeSound;
}


#include <fstream>

bool LoadWaveFile(char const* path, WaveFormatInfo& waveInfo, TArray< uint8 >& outSampleData)
{
	struct WaveChunkHeader
	{
		uint32 id;
		uint32 size;
		uint32 format;
	};

	struct SubChunkHeader
	{
		uint32 id;
		uint32 size;
	};


	std::ifstream fs(path, std::ios::binary);
	if( !fs.is_open() )
		return false;

	WaveChunkHeader chunkHeader;
	fs.read((char*)&chunkHeader, sizeof(chunkHeader));

	if( chunkHeader.id != MAKE_MAGIC_ID('R', 'I', 'F', 'F') )
		return false;

	if( chunkHeader.size < (4 + sizeof(WaveFormatInfo) + 2 * sizeof(SubChunkHeader)) || chunkHeader.format != MAKE_MAGIC_ID('W', 'A', 'V', 'E') )
		return false;

	SubChunkHeader subChunk;
	fs.read((char*)&subChunk, sizeof(subChunk));
	if( subChunk.id != MAKE_MAGIC_ID('f', 'm', 't', 32) || subChunk.size < sizeof(WaveFormatInfo) )
		return false;

	fs.read((char*)&waveInfo, sizeof(waveInfo));

	fs.seekg(subChunk.size - sizeof(WaveFormatInfo), std::ios::cur);
	fs.read((char*)&subChunk, sizeof(subChunk));
	if( subChunk.id != MAKE_MAGIC_ID('d', 'a', 't', 'a') )
		return false;


	outSampleData.resize(subChunk.size);
	fs.read((char*)&outSampleData[0], outSampleData.size());
	if( fs.bad() )
		return false;

	return true;
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
#define WAVE_FORMAT_PCM 1

	outWaveFormat.tag = WAVE_FORMAT_PCM;
	outWaveFormat.numChannels = info->channels;
	outWaveFormat.sampleRate = info->rate;
	outWaveFormat.bitsPerSample = 8 * sizeof(int16);
	outWaveFormat.byteRate = outWaveFormat.bitsPerSample * outWaveFormat.sampleRate * outWaveFormat.numChannels / 8;
	outWaveFormat.blockAlign = outWaveFormat.numChannels * outWaveFormat.bitsPerSample / 8;

	/* Read the entire sound stream. */
	unsigned char buf[4096];
	for (;;)
	{
		int section = 0;
		long bytes = ov_read(&vorbis, (char*)buf, sizeof(buf), 0, sizeof(int16), std::is_signed_v<int16>, &section);
		if (bytes <= 0) /* end of file or error */
			break;

		outSampleData.append(buf, buf + bytes);
	}

	return true;
}


#include "PlatformConfig.h"

#if SYS_PLATFORM_WIN
#include "XAudio2/XAudio2Device.h"
#endif

AudioDevice* AudioDevice::Create()
{
#if SYS_PLATFORM_WIN
	AudioDevice* device = new XAudio2Device;
	if( !device->initialize() )
	{
		delete device;
		return nullptr;
	}
	return device;
#else
	return nullptr;
#endif
}

bool AudioSource::initialize(SoundInstance& instance)
{
	mInstance = &instance;
	return doInitialize(instance);
}
