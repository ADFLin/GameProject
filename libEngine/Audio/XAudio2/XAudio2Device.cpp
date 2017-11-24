#include "XAudio2Device.h"



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

	WaveFormatInfo const& formatInfo = instance.soundwave->format;

	WAVEFORMATEX format = { 0 };
	format.wFormatTag = formatInfo.tag;
	format.nChannels = formatInfo.numChannels;
	format.nSamplesPerSec = formatInfo.sampleRate;
	format.wBitsPerSample = formatInfo.bitsPerSample;
	format.nAvgBytesPerSec = formatInfo.byteRate;
	format.nBlockAlign = formatInfo.blockAlign;
	format.cbSize = 0; //WAVE_FORMAT_PCM must be 0
	CHECK_RETRUN( mDevice->pXAudio2->CreateSourceVoice(&pSourceVoice, &format, 0, 2.0f, &mDevice->sourceCallback ) , false );

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

	CHECK_RETRUN( pSourceVoice->SubmitSourceBuffer(&buffer) , false );
	CHECK_RETRUN( pSourceVoice->Start(0) , false );
	return true;
}


void XAudio2Source::update(float deltaT)
{
	pSourceVoice->SetVolume(mInstance->volume);
	pSourceVoice->SetFrequencyRatio(mInstance->pitch);

	if( !mInstance->activeSound->bPlay2d )
	{


	}
}

void XAudio2Source::stop()
{
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

void XAudio2Source::notifyPlayEnd()
{
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

}

void XAudio2SourceCallback::OnBufferEnd(void* pBufferContext)
{
	XAudio2Source* source = static_cast<XAudio2Source*>(pBufferContext);
	source->notifyPlayEnd();
}

void XAudio2SourceCallback::OnLoopEnd(void* pBufferContext)
{

}

void XAudio2SourceCallback::OnVoiceError(void* pBufferContext, HRESULT Error)
{

}
