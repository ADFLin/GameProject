#pragma once
#ifndef AudioDevice_H_FE470721_C44B_48EC_A5C1_65B0EBE25937
#define AudioDevice_H_FE470721_C44B_48EC_A5C1_65B0EBE25937

#include "Core/IntegerType.h"

#include <unordered_map>
#include <vector>
#include <cassert>

typedef uint32 AudioHandle;
AudioHandle const ERROR_AUDIO_HANDLE = AudioHandle(0);


struct WaveFormatInfo
{
	uint16 tag;
	uint16 numChannels;
	uint32 sampleRate;
	uint32 byteRate;
	uint16 blockAlign;
	uint16 bitsPerSample;
};


bool LoadWaveFile(char const* path, WaveFormatInfo& waveInfo, std::vector< uint8 >& outSampleData);

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

	std::vector< SoundInstance* > playingInstance;

	bool isPlaying() const
	{
		return !playingInstance.empty();
	}

	void removeInstance(SoundInstance* instance)
	{
		auto iter = std::find(playingInstance.begin(), playingInstance.end(), instance);
		if (iter != playingInstance.end())
			playingInstance.erase(iter);
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

	std::vector< LoadedChunk > loadedChunks;
	uint32 dataPos;
	std::vector< uint8 > buffer;
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

	SoundInstance()
	{
		bUsed = false;
		bPlaying = false;
		sourceId = -1;
		soundwave = nullptr;
		activeSound = nullptr;
		usageFrame = 0;
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
	std::vector< SoundDSPData > mDataStack;
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

	bool loadFromWaveFile(char const* path)
	{
		if( !LoadWaveFile(path, format, PCMData) )
			return false;
		return true;
	}

	virtual void process(SoundDSP& dsp, uint64 hash) override
	{
		dsp.addInstance(hash, *this);
	}

	float getDurtion()
	{
		return float( PCMData.size() ) / format.byteRate;
	}


	void fillStreamingData()
	{



	}
	bool bStreaming = false;

	WaveFormatInfo format;
	std::vector< uint8 > PCMData;
};

class StreamingSoundWave : public SoundWave
{



};

class AudioSource
{
public:
	bool initialize(SoundInstance& instance);

	virtual bool doInitialize(SoundInstance& instance) { return true; }
	virtual void update( float deltaT ) {}
	virtual void stop() {}
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

	AudioHandle playSound2D(SoundBase* sound, float volumeMultiplier = 1.0f , bool bLoop = false );
	AudioHandle playSound3D(SoundBase* sound, float volumeMultiplier = 1.0f , bool bLoop = false );

	void setAudioVolumeMultiplier(AudioHandle handle , float value);
	void setAudioPitchMultiplier(AudioHandle handle, float value);

	void update( float deltaT );

	SoundInstance* registerInstance(uint64 hash, SoundWave& soundwave);

	SoundInstance* findInstance(uint64 hash);

	virtual AudioSource* createSource();

	int fetchIdleSource();


	void stopAllSound();

	ActiveSound* getActiveSound(AudioHandle handle);

protected:
	friend class SoundDSP;

	uint32 mSourceUsageFrame = 0;
	ActiveSound* createAtiveSound();
	void destroyActiveSound(ActiveSound* activeSound);

	std::vector< AudioSource* > mAudioSources;
	std::vector< int > mIdleSources;
	int mMaxChannelNum = 10;

	uint32 mNextHandleId = 1;
	std::vector< ActiveSound* > mActiveSounds;
	std::unordered_map< uint64, SoundInstance* > mPlayingInstances;
	std::unordered_map< uint32, ActiveSound* >   mActiveSoundMap;
};




#endif // AudioDevice_H_FE470721_C44B_48EC_A5C1_65B0EBE25937