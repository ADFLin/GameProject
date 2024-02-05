#include "SoundManager.h"
#include "Base.h"
#include "DataPath.h"

#include <iostream>
#include "Audio/XAudio2/MFDecoder.h"
#include "FileSystem.h"

static int SoundPlayMaxNum = 16;

SoundManager::SoundManager()
{

}
SoundManager::~SoundManager()
{
	mDataStorage.clear();
	delete mDevice;
}


bool SoundManager::initialize()
{
	mDevice = AudioDevice::Create();
	if (mDevice == nullptr)
		return false;

	if (!mDevice->initialize())
		return false;
	
	return true;
}

SoundData* SoundManager::getData(char const* name)
{
	HashString findName = name;
	for(int i=0; i<mDataStorage.size(); i++)
	{
		if(mDataStorage[i]->name == findName )
		{
			return mDataStorage[i].get();
		}
	}

	if ( !loadSound(name) )
		return NULL;

	return mDataStorage.back().get();
}

bool SoundManager::loadSound(char const* name)
{
	String path = SOUND_DIR;
	path += name;

	std::unique_ptr<SoundData> soundData = std::make_unique< SoundData >();

	auto LoadRes = [&]()
	{
		char const* ext = FFileUtility::GetExtension(path.c_str());

		if (FCString::CompareIgnoreCase(ext, "ogg") == 0)
		{
			if (LoadOggFile(path.c_str(), soundData->soundWave.format, soundData->soundWave.PCMData))
				return true;

			return false;
		}

		if (FCString::CompareIgnoreCase(ext, "wav") == 0)
		{
			if (LoadWaveFile(path.c_str(), soundData->soundWave.format, soundData->soundWave.PCMData))
				return true;
		}

		if (!FMFDecodeUtil::LoadWaveFile(path.c_str(), soundData->soundWave.format, soundData->soundWave.PCMData))
		{
			LogWarning(0, "Audio file load fail : %s", path.c_str());
			return false;
		}

		return false;

	};
	if (!LoadRes())
		return false;


	soundData->name   = name;
	mDataStorage.push_back(std::move(soundData));
	QA_LOG("Sound loaded : %s", name);
	return true;
}

void SoundManager::cleanup()
{
	if (mDevice)
	{
		mDevice->shutdown();
	}
	for(Sound* sound : mSounds )	
	{
		if ( sound->isPlaying() )
			sound->stop();
	}
	mSounds.clear();
	mDataStorage.clear();
}

void SoundManager::update( float dt )
{
	if (mDevice)
	{
		mDevice->update(dt);
	}

	for( SoundList::iterator iter = mSounds.begin();
		iter != mSounds.end(); )
	{
		Sound* sound = *iter;
		if( !sound->isPlaying() )
		{
			iter = mSounds.erase( iter );				
		}
		else
		{
			++iter;
		}
	}
}

SoundPtr SoundManager::addSound( char const* name , bool canRepeat )
{
	if ( mSounds.size() >= SoundPlayMaxNum )
		return NULL;

	SoundData* data = getData( name );

	if ( data == NULL )
		return NULL;

	if ( !canRepeat )
	{
		for(int i=0; i<mSounds.size(); i++)
		{
			Sound* sound = mSounds[i];
			if( sound->getData() != data )
				continue;

			sound->stop();
			return sound;
		}
	}

	SoundPtr sound = new Sound( data );
	sound->mDevice = mDevice;
	mSounds.push_back(sound);		
	return sound;
}
