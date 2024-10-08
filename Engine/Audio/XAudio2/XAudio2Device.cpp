#include "PlatformConfig.h"

#if SYS_PLATFORM_WIN

#include "XAudio2Device.h"
#include "LogSystem.h"
#include "ProfileSystem.h"
#include "PlatformThread.h"


float gStreamLockedDuration = 1;
bool XAudio2Device::initialize()
{
	Async([this]()
	{
		initializePlatform();
	});

	if( !BaseClass::initialize() )
		return false;

	return true;
}

void XAudio2Device::shutdown()
{
	BaseClass::shutdown();

	pMasterVoice.reset();
	pXAudio2.reset();

	if( bCoInitialized )
		CoUninitialize();
}

bool XAudio2Device::initializePlatform()
{
	TIME_SCOPE("initializePlatform");
	{
		TIME_SCOPE("CoInitializeEx");
		CHECK_RETURN(CoInitializeEx(NULL, COINIT_MULTITHREADED), false);
		bCoInitialized = true;
	}
	{
		TIME_SCOPE("XAudio2Create");
		CHECK_RETURN(XAudio2Create(&pXAudio2, 0, XAUDIO2_DEFAULT_PROCESSOR), false);
	}

	{
		TIME_SCOPE("CreateMasteringVoice");
		CHECK_RETURN(pXAudio2->CreateMasteringVoice(&pMasterVoice), false);
	}
	return true;
}

AudioSource* XAudio2Device::createSource()
{
	XAudio2Source* chennel = new XAudio2Source;
	chennel->mDevice = this;
	return chennel;
}

bool XAudio2Source::doInitialize(SoundInstance& instance)
{
	pSourceVoice.reset();

	instance.samplesPlayed = 0;
	mNextStreamSampleFrame = INDEX_NONE;
	mUsedSampleHandles.clear();

	WaveFormatInfo const& formatInfo = instance.soundwave->format;

	WAVEFORMATEX format = { 0 };
	format.wFormatTag = formatInfo.tag;
	format.nChannels = formatInfo.numChannels;
	format.nSamplesPerSec = formatInfo.sampleRate;
	format.wBitsPerSample = formatInfo.bitsPerSample;
	format.nAvgBytesPerSec = formatInfo.byteRate;
	format.nBlockAlign = formatInfo.blockAlign;
	format.cbSize = 0; //WAVE_FORMAT_PCM must be 0
	CHECK_RESULT_CODE(
		mDevice->pXAudio2->CreateSourceVoice(&pSourceVoice, &format, 0, 10.0f, this),
		LogWarning(0, "Can't Create SourceVoice");
		return false;
	);

	if( instance.soundwave->streamSource )
	{
		instance.soundwave->streamSource->seekSamplePosition(instance.startSamplePos);
		mNextStreamSampleFrame = instance.startSamplePos;
		if( !commitStreamingData(true) )
			return false;
	}
	else
	{
		XAUDIO2_BUFFER buffer = { 0 };
		buffer.Flags = XAUDIO2_END_OF_STREAM;
		buffer.pAudioData = (BYTE*)instance.soundwave->PCMData.data();
		buffer.AudioBytes = instance.soundwave->PCMData.size();
		buffer.PlayBegin = 0;
		buffer.PlayLength = 0;
		buffer.LoopBegin = 0;
		buffer.LoopCount = instance.activeSound->bLoop ? XAUDIO2_LOOP_INFINITE : 0;
		buffer.LoopLength = 0;
		buffer.pContext = this;
		CHECK_RETURN(pSourceVoice->SubmitSourceBuffer(&buffer), false);
	}

	CHECK_RETURN(pSourceVoice->Start(0), false);
	return true;
}


bool XAudio2Source::commitStreamingData( bool bInit )
{
	SoundInstance& instance = *mInstance;
	auto const& formatInfo = instance.soundwave->format;

	AudioStreamSample sample;
	bool bNeedFlush = bInit || mUsedSampleHandles.size() == 0;
	auto fetchResult = instance.soundwave->streamSource->generatePCMData(mNextStreamSampleFrame, sample , formatInfo.sampleRate / 16, bNeedFlush);

	if( fetchResult == EAudioStreamStatus::NoSample )
		return false;

	if(  fetchResult == EAudioStreamStatus::Error ||
	    (fetchResult == EAudioStreamStatus::Eof && sample.dataSize == 0))
	{
		mNextStreamSampleFrame = INDEX_NONE;
		pSourceVoice->Discontinuity();
		return false;
	}

	mUsedSampleHandles.push_back(sample.handle);

	if( instance.soundwave->bSaveStreamingPCMData && sample.dataSize )
	{
		instance.soundwave->PCMData.insert(instance.soundwave->PCMData.end(), sample.data, sample.data + sample.dataSize);
	}

	XAUDIO2_BUFFER buffer = { 0 };
	buffer.Flags = (fetchResult == EAudioStreamStatus::Eof) ? XAUDIO2_END_OF_STREAM : 0;
	buffer.pAudioData = (BYTE*)sample.data;
	buffer.AudioBytes = sample.dataSize;
	if( bInit )
	{
		buffer.LoopCount = instance.activeSound->bLoop ? XAUDIO2_LOOP_INFINITE : 0;
	}
	buffer.pContext = (void*)sample.handle;

	if (fetchResult == EAudioStreamStatus::Eof)
	{
		mNextStreamSampleFrame = INDEX_NONE;
	}
	else
	{
		mNextStreamSampleFrame += sample.dataSize / formatInfo.blockAlign;
	}

	CHECK_RETURN(pSourceVoice->SubmitSourceBuffer(&buffer), false);
	return true;
}

void XAudio2Source::update(float deltaT)
{
	pSourceVoice->SetVolume(mInstance->volume);
	pSourceVoice->SetFrequencyRatio(mInstance->pitch);


	XAUDIO2_VOICE_STATE state;
	pSourceVoice->GetState(&state);
	WaveFormatInfo const& formatInfo = mInstance->soundwave->format;

	mInstance->samplesPlayed = state.SamplesPlayed;
#if 0	
	static uint64 prevPlayed = 0;
	uint64 delta = mInstance->samplesPlayed - prevPlayed;
	LogMsg("SamplePlayed = %llu , DeltaPlayed = %llu , sampleRate = %d , Time = %lf", mInstance->samplesPlayed, delta, formatInfo.sampleRate, double(mInstance->samplesPlayed) / formatInfo.sampleRate);
	prevPlayed = state.SamplesPlayed;
#endif


	if( mInstance->soundwave->streamSource )
	{
		if ( mNextStreamSampleFrame != INDEX_NONE )
		{
			if( state.BuffersQueued < 4 )
			{
				commitStreamingData(false);
			}
		}
	}

	if( !mInstance->activeSound->bPlay2d )
	{


	}
}

void XAudio2Source::endPlay()
{
	if( mInstance->soundwave->isStreaming() )
	{
		for( uint32 handle : mUsedSampleHandles )
		{
			mInstance->soundwave->streamSource->releaseSampleData(handle);
		}
		mUsedSampleHandles.clear();
	}

	if( pSourceVoice )
	{
		pSourceVoice->Stop();
	}
}

void XAudio2Source::pause()
{
	if( pSourceVoice )
	{
		pSourceVoice->Stop(0);
	}
}


void XAudio2Source::OnVoiceProcessingPassStart(UINT32 BytesRequired)
{

}

void XAudio2Source::OnVoiceProcessingPassEnd()
{

}

void XAudio2Source::OnStreamEnd()
{
	if (!mInstance->activeSound->bLoop)
		mInstance->bPlaying = false;

	if (mInstance->soundwave->isStreaming())
	{
		for (uint32 handle : mUsedSampleHandles)
		{
			mInstance->soundwave->streamSource->releaseSampleData(handle);
		}
		mUsedSampleHandles.clear();
	}
}

void XAudio2Source::OnBufferStart(void* pBufferContext)
{

}

void XAudio2Source::OnBufferEnd(void* pBufferContext)
{
	if (mInstance->soundwave->isStreaming())
	{
		uint32 handle = (uint32)pBufferContext;
		if (mUsedSampleHandles.size())
		{
			mUsedSampleHandles.remove(handle);
			mInstance->soundwave->streamSource->releaseSampleData(handle);
		}
	}
	else
	{
		mInstance->bPlaying = false;
	}
}

void XAudio2Source::OnLoopEnd(void* pBufferContext)
{

}

void XAudio2Source::OnVoiceError(void* pBufferContext, HRESULT Error)
{

}
#endif