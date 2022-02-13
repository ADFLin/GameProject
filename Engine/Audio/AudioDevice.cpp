#include "AudioDevice.h"

#include "MarcoCommon.h"
#include <cassert>

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

AudioHandle AudioDevice::playSound2D(SoundBase* sound, float volumeMultiplier , bool bLoop)
{
	ActiveSound* activeSound = createAtiveSound();
	activeSound->sound = sound;
	activeSound->volumeMultiplier = volumeMultiplier;
	activeSound->bLoop = bLoop;
	activeSound->bFinished = false;
	activeSound->bPlay2d = true;

	return activeSound->handle;
}

AudioHandle AudioDevice::playSound3D(SoundBase* sound, float volumeMultiplier , bool bLoop)
{
	ActiveSound* activeSound = createAtiveSound();
	activeSound->sound = sound;
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

void AudioDevice::update( float deltaT )
{
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
			if( instance->sourceId != -1 )
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
			if( instance->sourceId == -1 )
			{
				instance->bPlaying = false;
				instance->sourceId = fetchIdleSource(*instance);

				if( instance->sourceId != -1 )
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
				if( instance->sourceId != -1 )
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

AudioSource* AudioDevice::createSource()
{
	return nullptr;
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
		int channelId = mAudioSources.size();
		mAudioSources.push_back(createSource());
		return channelId;
	}

	return -1;
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

bool LoadWaveFile(char const* path, WaveFormatInfo& waveInfo, std::vector< uint8 >& outSampleData)
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
