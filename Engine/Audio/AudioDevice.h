#pragma once
#ifndef AudioDevice_H_FE470721_C44B_48EC_A5C1_65B0EBE25937
#define AudioDevice_H_FE470721_C44B_48EC_A5C1_65B0EBE25937

#include "AudioTypes.h"
#include "AudioStreamSource.h"

#include "DataStructure/Array.h"

#include <unordered_map>
#include <cassert>
#include <string>



typedef uint32 AudioHandle;
AudioHandle const ERROR_AUDIO_HANDLE = AudioHandle(0);


bool LoadWaveFile(char const* path, WaveFormatInfo& waveInfo, TArray< uint8 >& outSampleData);
bool LoadOggFile(char const* path, WaveFormatInfo& outWaveFormat, TArray<uint8>& outSampleData);

class SoundWave;
class SoundBase;
struct ActiveSound;
struct SoundInstance;

struct ActiveSound
{
	AudioHandle handle;
	SoundBase*  sound;

	float       volumeMultiplier;
	float       pitchMultiplier;
	bool        bFinished;
	bool        bPlay2d;
	bool        bLoop;

	TArray< SoundInstance* > playingInstances;

	bool isPlaying() const
	{
		return !playingInstances.empty();
	}

	void removeInstance(SoundInstance* instance)
	{
		auto iter = std::find(playingInstances.begin(), playingInstances.end(), instance);
		if (iter != playingInstances.end())
			playingInstances.erase(iter);
	}

	ActiveSound()
	{
		volumeMultiplier = 1.0f;
		pitchMultiplier = 1.0f;
	}
};

struct SoundWaveStreamingData
{


	struct LoadedChunk
	{
		uint32 size;
		uint32 dataPos;
		uint32 samplePos;
	};

	TArray< LoadedChunk > loadedChunks;
	uint32 dataPos;
	TArray< uint8 > buffer;
};

struct SoundInstance
{
	int  sourceId;
	ActiveSound* activeSound;
	SoundWave*   soundwave;

	uint32       usageFrame;
	bool         bUsed;
	bool         bPlaying;

	float        pitch;
	float        volume;

	uint64       startSamplePos;
	uint64       samplesPlayed;
	SoundInstance()
	{
		startSamplePos = 0;
		bUsed = false;
		bPlaying = false;
		sourceId = INDEX_NONE;
		soundwave = nullptr;
		activeSound = nullptr;
		usageFrame = 0;
		samplesPlayed = 0;
	}
};

class AudioDevice;


struct SoundDSPData
{
	float pitch;
	float volume;

	SoundDSPData()
	{
		volume = 1.0f;
		pitch = 1.0f;
	}
};

class SoundDSP
{
public:

	SoundDSP()
	{


	}
	void pushData()
	{
		mDataStack.push_back(mData);
	}
	void popData() 
	{
		mData = mDataStack.back();
		mDataStack.pop_back();
	}
	void addInstance(uint64 hash, SoundWave& soundwave);


	SoundDSPData mData;
	AudioDevice* mDevice;
	ActiveSound* mActiveSound;
	TArray< SoundDSPData > mDataStack;
};


class SoundBase
{
public:
	virtual ~SoundBase() {}
	virtual void process(SoundDSP& dsp, uint64 hash) = 0;

	static uint64 makeChildHash(uint64 hash, int index)
	{

		return hash;
	}
};

class SoundWave : public SoundBase
{
public:

	bool loadFromWaveFile(char const* path , bool bStreaming = false )
	{
		if( bStreaming )
		{

		}
		else
		{
			if( !LoadWaveFile(path, format, PCMData) )
				return false;

			bPCMDataCompleted = true;
		}
		return true;
	}

	void setupStream(IAudioStreamSource* inStreamSource, bool bCacheData = false)
	{
		streamSource = inStreamSource;
		streamSource->getWaveFormat(format);
		bSaveStreamingPCMData = bCacheData;
		PCMData.clear();
	}

	virtual void process(SoundDSP& dsp, uint64 hash) override
	{
		dsp.addInstance(hash, *this);
	}

	float getDuration()
	{
		if( streamSource )
			return float( streamSource->getTotalSampleNum() ) / format.sampleRate;

		return float( PCMData.size() ) / format.byteRate;
	}

	bool isStreaming() const { return streamSource != nullptr; }


	std::string assertPath;
	WaveFormatInfo format;

	bool bSaveStreamingPCMData = false;
	bool bPCMDataCompleted = false;
	TArray< uint8 > PCMData;

	IAudioStreamSource* streamSource = nullptr;
};

class AudioSource
{
public:


	bool initialize(SoundInstance& instance);

	virtual bool doInitialize(SoundInstance& instance) { return true; }
	virtual void update( float deltaT ) 
	{


	}
	virtual void endPlay() {}
	virtual void pause() {}

protected:
	SoundInstance* mInstance;
};



class AudioDevice
{
public:

	static AudioDevice* Create();

	virtual bool initialize();
	virtual void shutdown();

	AudioHandle playSound2D(SoundBase& sound, float volumeMultiplier = 1.0f , bool bLoop = false );
	AudioHandle playSound3D(SoundBase& sound, float volumeMultiplier = 1.0f , bool bLoop = false );

	void setAudioVolumeMultiplier(AudioHandle handle , float value);
	void setAudioPitchMultiplier(AudioHandle handle, float value);

	void update( float deltaT );

	SoundInstance* registerInstance(uint64 hash, SoundWave& soundwave);

	SoundInstance* findInstance(uint64 hash);

	virtual AudioSource* createSource() = 0;

	int fetchIdleSource(SoundInstance& instance);

	bool isPlaying(AudioHandle handle);
	void stopSound(AudioHandle handle);
	void stopAllSound();

	ActiveSound* getActiveSound(AudioHandle handle);

protected:
	friend class SoundDSP;

	uint32 mSourceUsageFrame = 0;
	ActiveSound* createAtiveSound();
	void destroyActiveSound(ActiveSound* activeSound);

	TArray< AudioSource* > mAudioSources;
	TArray< int > mIdleSources;
	int mMaxChannelNum = 128;

	uint32 mNextHandleId = 1;
	TArray< ActiveSound* > mActiveSounds;
	std::unordered_map< uint64, SoundInstance* > mPlayingInstances;
	std::unordered_map< uint32, ActiveSound* >   mActiveSoundMap;
};




#endif // AudioDevice_H_FE470721_C44B_48EC_A5C1_65B0EBE25937