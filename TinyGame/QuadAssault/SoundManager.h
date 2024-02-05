#ifndef SoundManager_h__
#define SoundManager_h__

#include "Dependence.h"

#include "Audio/AudioDevice.h"

#include "HashString.h"
#include "RefCount.h"

struct SoundData
{
	HashString name;
	SoundWave  soundWave;
};

class Sound : public RefCountedObjectT<Sound>
{
public:
	Sound( SoundData* data )
		:mData( data )
	{

	}
	void play(float volumeMultiplier = 1.0f, bool bLoop = false)
	{
		mHandle = mDevice->playSound2D(mData->soundWave, volumeMultiplier, bLoop);
	}
	void stop() 
	{
		mDevice->stopSound(mHandle);

	}
	bool isPlaying()
	{
		return mDevice->isPlaying(mHandle);
	}

	SoundData* getData(){ return mData; }

private:
	friend class SoundManager;
	AudioDevice* mDevice = nullptr;
	AudioHandle mHandle = ERROR_AUDIO_HANDLE;
	SoundData*  mData;
};

using SoundPtr = TRefCountPtr<Sound>;

class SoundManager
{
public:	
	SoundManager();
	~SoundManager();

	bool initialize();

	SoundData* getData(char const* name );
	SoundPtr   addSound( char const* name , bool canRepeat = false );
	void       update( float dt );
	bool       loadSound(char const* name );
	void       cleanup();

private:
	AudioDevice* mDevice = nullptr;

	typedef std::vector< SoundPtr > SoundList;

	TArray< std::unique_ptr<SoundData> > mDataStorage;
	SoundList mSounds;
};


#endif // SoundManager_h__
