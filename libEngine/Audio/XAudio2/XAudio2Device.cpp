#include "XAudio2Device.h"

#include "LogSystem.h"

float gStreamLockedDurtion = 1;
bool XAudio2Device::initialize()
{


	CHECK_RETRUN(CoInitializeEx(NULL , COINIT_MULTITHREADED ), false);
	bCoInitialized = true;

	CHECK_RETRUN(XAudio2Create(&pXAudio2, 0, XAUDIO2_DEFAULT_PROCESSOR), false);

	CHECK_RETRUN(pXAudio2->CreateMasteringVoice(&pMasterVoice), false);

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
	mNextStreamSampleFrame = -1;
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
	CHECK_RETRUN(mDevice->pXAudio2->CreateSourceVoice(&pSourceVoice, &format, 0, 2.0f, &mDevice->sourceCallback), false);

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
		buffer.pAudioData = (BYTE*)&instance.soundwave->PCMData[0];
		buffer.AudioBytes = instance.soundwave->PCMData.size();
		buffer.PlayBegin = 0;
		buffer.PlayLength = 0;
		buffer.LoopBegin = 0;
		buffer.LoopCount = instance.activeSound->bLoop ? XAUDIO2_LOOP_INFINITE : 0;
		buffer.LoopLength = 0;
		buffer.pContext = this;
		CHECK_RETRUN(pSourceVoice->SubmitSourceBuffer(&buffer), false);
	}

	CHECK_RETRUN(pSourceVoice->Start(0), false);
	return true;
}


bool XAudio2Source::commitStreamingData( bool bInit )
{
	SoundInstance& instance = *mInstance;
	auto const& formatInfo = instance.soundwave->format;

	AudioStreamSample sample;
	auto fetchResult = instance.soundwave->streamSource->generatePCMData(mNextStreamSampleFrame, sample , formatInfo.sampleRate / 32 );

	if( fetchResult == EAudioStreamStatus::NoSample )
		return false;

	if( fetchResult == EAudioStreamStatus::Error )
	{
		mNextStreamSampleFrame = -1;
		return false;
	}

	if( sample.handle != -1 )
	{
		mUsedSampleHandles.push_back(sample.handle);
	}

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
	buffer.pContext = this;
	CHECK_RETRUN(pSourceVoice->SubmitSourceBuffer(&buffer), false);
	if( fetchResult == EAudioStreamStatus::Eof )
	{
		mNextStreamSampleFrame = -1;
	}
	else
	{
		mNextStreamSampleFrame += sample.dataSize / ( formatInfo.numChannels * formatInfo.bitsPerSample / 8 );
	}	
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
		if ( mNextStreamSampleFrame != -1 )
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
void XAudio2Source::notifyBufferStart()
{

}
void XAudio2Source::notifyBufferEnd()
{
	if( mInstance->soundwave->isStreaming() )
	{
		if ( mUsedSampleHandles.size() )
		{
			uint32 handle = mUsedSampleHandles.front();
			mUsedSampleHandles.erase(mUsedSampleHandles.begin());
			mInstance->soundwave->streamSource->releaseSampleData(handle);
		}
		else
		{
			mInstance->bPlaying = false;
		}
	}
}

void XAudio2Source::noitfyStreamEnd()
{
	if ( !mInstance->activeSound->bLoop )
		mInstance->bPlaying = false;
}

void XAudio2SourceCallback::OnVoiceProcessingPassStart(UINT32 BytesRequired)
{

}

void XAudio2SourceCallback::OnVoiceProcessingPassEnd()
{

}

void XAudio2SourceCallback::OnStreamEnd()
{

}

void XAudio2SourceCallback::OnBufferStart(void* pBufferContext)
{
	XAudio2Source* source = static_cast<XAudio2Source*>(pBufferContext);
	source->notifyBufferStart();
}

void XAudio2SourceCallback::OnBufferEnd(void* pBufferContext)
{
	XAudio2Source* source = static_cast<XAudio2Source*>(pBufferContext);
	source->notifyBufferEnd();
}

void XAudio2SourceCallback::OnLoopEnd(void* pBufferContext)
{

}

void XAudio2SourceCallback::OnVoiceError(void* pBufferContext, HRESULT Error)
{

}
