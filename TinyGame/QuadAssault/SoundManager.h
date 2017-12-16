#ifndef SoundManager_h__
#define SoundManager_h__

#include "Dependence.h"

#include "HashString.h"
struct SoundData
{
	HashString name;
#if USE_SFML
	sf::SoundBuffer buffer;
#endif
};

class Sound
{
public:
	Sound( SoundData* data )
		:mData( data )
	{
#if USE_SFML
		mSoundImpl.setBuffer( mData->buffer );
#endif
	}
#if USE_SFML
	void play(){ mSoundImpl.play(); }
	void stop(){ mSoundImpl.stop(); }
	bool isPlaying()
	{ 
		return mSoundImpl.getStatus() == sf::Sound::Playing; 
	}
#else
	void play() { }
	void stop() { }
	bool isPlaying()
	{
		return false;
	}
#endif
	SoundData* getData(){ return mData; }

private:
#if USE_SFML
	sf::Sound  mSoundImpl;
#endif
	SoundData* mData;
};

class SoundManager
{
public:	
	SoundManager();
	~SoundManager();

	SoundData* getData(char const* name );
	Sound*     addSound( char const* name , bool canRepeat = false );
	void       update( float dt );
	bool       loadSound(char const* name );
	void       cleanup();

private:
	typedef std::vector< Sound* > SoundList;

	std::vector<SoundData> mDataStorage;
	SoundList mSounds;
};


#endif // SoundManager_h__
