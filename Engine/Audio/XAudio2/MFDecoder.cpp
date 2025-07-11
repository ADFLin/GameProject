#include "MFDecoder.h"

#include "CString.h"
#include "Core/ScopeGuard.h"
#include "LogSystem.h"

#include <cguid.h>
#include <propvarutil.h>

bool FMFDecodeUtil::ConfigureAudioStream(IMFSourceReader *pReader, IMFMediaType **ppPCMAudio)
{
	// Select the first audio stream, and deselect all other streams.
	CHECK_RETURN(pReader->SetStreamSelection(MF_SOURCE_READER_ALL_STREAMS, FALSE), false);

	CHECK_RETURN(pReader->SetStreamSelection(MF_SOURCE_READER_FIRST_AUDIO_STREAM, TRUE), false);

	TComPtr< IMFMediaType > pPartialType;
	// Create a partial media type that specifies uncompressed PCM audio.
	CHECK_RETURN(MFCreateMediaType(&pPartialType), false);

	CHECK_RETURN(pPartialType->SetGUID(MF_MT_MAJOR_TYPE, MFMediaType_Audio), false);
	CHECK_RETURN(pPartialType->SetGUID(MF_MT_SUBTYPE, MFAudioFormat_PCM), false);

	// Set this type on the source reader. The source reader will
	// load the necessary decoder.
	CHECK_RETURN(pReader->SetCurrentMediaType(MF_SOURCE_READER_FIRST_AUDIO_STREAM, NULL, pPartialType), false);

	TComPtr< IMFMediaType > pUncompressedAudioType;
	// Get the complete uncompressed format.
	CHECK_RETURN(pReader->GetCurrentMediaType(MF_SOURCE_READER_FIRST_AUDIO_STREAM, &pUncompressedAudioType), false);

	// Ensure the stream is selected.
	CHECK_RETURN(pReader->SetStreamSelection(MF_SOURCE_READER_FIRST_AUDIO_STREAM, TRUE), false);

	// Return the PCM format to the caller.

	*ppPCMAudio = pUncompressedAudioType;
	(*ppPCMAudio)->AddRef();
	return true;
}

void FMFDecodeUtil::FillData(WaveFormatInfo& formatInfo, WAVEFORMATEX const& format)
{
	formatInfo.tag = format.wFormatTag;
	formatInfo.numChannels = format.nChannels;
	formatInfo.sampleRate = format.nSamplesPerSec;
	formatInfo.bitsPerSample = format.wBitsPerSample;
	formatInfo.byteRate = format.nAvgBytesPerSec;
	formatInfo.blockAlign = format.nBlockAlign;
}

int64 FMFDecodeUtil::GetSourceDuration(IMFSourceReader *pReader)
{
	PROPVARIANT DurationAttrib;
	{
		const HRESULT Result = pReader->GetPresentationAttribute(MF_SOURCE_READER_MEDIASOURCE, MF_PD_DURATION, &DurationAttrib);
		if (FAILED(Result))
		{
			return 0;
		}
	}
	const int64 Duration = (DurationAttrib.vt == VT_UI8) ? (int64)DurationAttrib.uhVal.QuadPart : 0;
	::PropVariantClear(&DurationAttrib);
	return Duration;
}

bool FMFDecodeUtil::WriteWaveFile(IMFSourceReader *pReader, WaveFormatInfo& outWaveFormat, TArray<uint8>& outSampleData)
{
	HRESULT hr = S_OK;
	TComPtr< IMFMediaType > pAudioType;    // Represents the PCM audio format.
										   // Configure the source reader to get uncompressed PCM audio from the source file.
	if (!ConfigureAudioStream(pReader, &pAudioType))
		return false;

	DWORD cbHeader = 0;         // Size of the WAVE file header, in bytes.
	DWORD cbAudioData = 0;      // Total bytes of PCM audio data written to the file.
	DWORD cbMaxAudioData = 0;

	{
		WAVEFORMATEX *pWavFormat = NULL;
		UINT32 cbFormat = 0;
		// Convert the PCM audio format into a WAVEFORMATEX structure.
		CHECK_RETURN(MFCreateWaveFormatExFromMFMediaType(pAudioType, &pWavFormat, &cbFormat), false);
		FillData(outWaveFormat, *pWavFormat);
		CoTaskMemFree(pWavFormat);
	}

	int64 duration = GetSourceDuration(pReader);
	int64 dataSize = outWaveFormat.byteRate * duration;
	LogMsg("%lld", dataSize);

	{
		HRESULT hr = S_OK;
		DWORD cbAudioData = 0;

		// Get audio samples from the source reader.
		for (;;)
		{
			DWORD dwFlags = 0;
			TComPtr<IMFSample> pSample;
			// Read the next sample.
			hr = pReader->ReadSample(MF_SOURCE_READER_FIRST_AUDIO_STREAM, 0, NULL, &dwFlags, NULL, &pSample);

			if (FAILED(hr)) { break; }

			if (dwFlags & MF_SOURCE_READERF_CURRENTMEDIATYPECHANGED)
			{
				printf("Type change - not supported by WAVE file format.\n");
				return false;
			}
			if (dwFlags & MF_SOURCE_READERF_ENDOFSTREAM)
			{
				printf("End of input file.\n");
				break;
			}

			if (pSample == nullptr)
			{
				printf("No sample\n");
				continue;
			}

			// Get a pointer to the audio data in the sample.
			TComPtr<IMFMediaBuffer> pBuffer;
			CHECK_RETURN(pSample->ConvertToContiguousBuffer(&pBuffer), false);

			DWORD cbBuffer = 0;
			BYTE *pAudioData = NULL;
			{
				CHECK_RETURN(pBuffer->Lock(&pAudioData, NULL, &cbBuffer), false);
				ON_SCOPE_EXIT{ pBuffer->Unlock(); };
				outSampleData.insert(outSampleData.end(), pAudioData, pAudioData + cbBuffer);
			}
			// Update running total of audio data.
			cbAudioData += cbBuffer;
		}
	}

	return true;
}

bool FMFDecodeUtil::LoadWaveFile(char const* path, WaveFormatInfo& outWaveFormat, TArray<uint8>& outSampleData)
{
	MediaFoundationScope MFScope;
	if (!MFScope.bInit)
		return false;

	const size_t cSize = FCString::Strlen(path) + 1;
	wchar_t wPath[MAX_PATH];
	mbstowcs(wPath, path, cSize);

	TComPtr< IMFSourceReader  > pReader;
	// Create the source reader to read the input file.
	CHECK_RETURN(MFCreateSourceReaderFromURL(wPath, NULL, &pReader), false);

	// Write the WAVE file.
	if (!WriteWaveFile(pReader, outWaveFormat, outSampleData))
		return false;

	return true;
}

bool MFAudioStreamSource::initialize(char const* path)
{
	const size_t cSize = FCString::Strlen(path) + 1;
	CHECK_RETURN(MFCreateSourceReaderFromURL(FCString::CharToWChar(path).c_str(), NULL, &mSourceReader), false);

	HRESULT hr = S_OK;
	TComPtr< IMFMediaType > pAudioType;    // Represents the PCM audio format.
										   // Configure the source reader to get uncompressed PCM audio from the source file.
	if (!FMFDecodeUtil::ConfigureAudioStream(mSourceReader, &pAudioType))
		return false;

	DWORD cbHeader = 0;         // Size of the WAVE file header, in bytes.
	DWORD cbAudioData = 0;      // Total bytes of PCM audio data written to the file.
	DWORD cbMaxAudioData = 0;

	{
		WAVEFORMATEX *pWavFormat = NULL;
		UINT32 cbFormat = 0;
		// Convert the PCM audio format into a WAVEFORMATEX structure.
		CHECK_RETURN(MFCreateWaveFormatExFromMFMediaType(pAudioType, &pWavFormat, &cbFormat), false);
		FMFDecodeUtil::FillData(mWaveFormat, *pWavFormat);
		CoTaskMemFree(pWavFormat);
	}

	int64 duration = FMFDecodeUtil::GetSourceDuration(mSourceReader);
	mTotalSampleNum = mWaveFormat.sampleRate * duration / SecondTimeUnit;

	return true;
}

void MFAudioStreamSource::setCurrentTime(double time)
{
	int64 timeInt = time * SecondTimeUnit;
	PROPVARIANT var;
	HRESULT hr = InitPropVariantFromInt64(timeInt, &var);
	hr = mSourceReader->SetCurrentPosition(GUID_NULL, var);
	PropVariantClear(&var);
}

void MFAudioStreamSource::seekSamplePosition(int64 samplePos)
{
	int64 duration = SecondTimeUnit * samplePos / mWaveFormat.byteRate;
	PROPVARIANT var;
	HRESULT hr = InitPropVariantFromInt64(duration, &var);
	hr = mSourceReader->SetCurrentPosition(GUID_NULL, var);
	PropVariantClear(&var);
}

void MFAudioStreamSource::getWaveFormat(WaveFormatInfo& outFormat)
{
	outFormat = mWaveFormat;
}

int64 MFAudioStreamSource::getTotalSampleNum()
{
	return mTotalSampleNum;
}

EAudioStreamStatus MFAudioStreamSource::generatePCMData(int64 samplePos, AudioStreamSample& outSample, int minSampleFrameRequired, bool bNeedFlush)
{
	HRESULT hr = S_OK;

	EAudioStreamStatus result = EAudioStreamStatus::Ok;
	// Get audio samples from the source reader.

	int genertatedSampleFrame = 0;

	WaveSampleBuffer::SampleHandle sampleHandle = mSampleBuffer.acquireSampleData();
	WaveSampleBuffer::SampleData* sampleData = mSampleBuffer.getSampleData(sampleHandle);
	assert(sampleData->data.empty());
	for (;;)
	{
		DWORD dwFlags = 0;
		TComPtr<IMFSample> pSample;
		// Read the next sample.
		LONGLONG TimeStamp;
		hr = mSourceReader->ReadSample(MF_SOURCE_READER_FIRST_AUDIO_STREAM, 0, NULL, &dwFlags, &TimeStamp, &pSample);

		if (FAILED(hr))
		{
			result = EAudioStreamStatus::Error;
			break;
		}

		if (dwFlags & MF_SOURCE_READERF_CURRENTMEDIATYPECHANGED)
		{
			LogWarning(0, "Type change - not supported by WAVE file format.\n");
			return EAudioStreamStatus::Error;
			break;
		}
		if (dwFlags & MF_SOURCE_READERF_ENDOFSTREAM)
		{
			result = EAudioStreamStatus::Eof;
			break;
		}

		if (pSample == nullptr)
		{
			result = EAudioStreamStatus::NoSample;
			break;
		}

		LONGLONG sampleTime;
		HRESULT hrA = pSample->GetSampleTime(&sampleTime);
		LONGLONG sampleDuration;
		HRESULT hrB = pSample->GetSampleDuration(&sampleDuration);

		auto ReadBuffer = [&](TComPtr<IMFMediaBuffer>& buffer)
		{
			DWORD cbBuffer = 0;
			BYTE *pAudioData = NULL;

			hr = buffer->Lock(&pAudioData, NULL, &cbBuffer);
			assert((cbBuffer % mWaveFormat.bitsPerSample) == 0);
			sampleData->data.append(pAudioData, pAudioData + cbBuffer);
			buffer->Unlock();
			genertatedSampleFrame += cbBuffer / mWaveFormat.blockAlign;
		};

#if 0
		TComPtr<IMFMediaBuffer> buffer;
		hr = pSample->ConvertToContiguousBuffer(&buffer);
		if (FAILED(hr))
		{
			result = EAudioStreamStatus::Error;
			break;
		}
		ReadBuffer(buffer);
#else
		DWORD bufferCount = 0;
		pSample->GetBufferCount(&bufferCount);

		for (int indexBuffer = 0; indexBuffer < bufferCount; ++indexBuffer)
		{
			TComPtr<IMFMediaBuffer> buffer;
			pSample->GetBufferByIndex(indexBuffer, &buffer);
			ReadBuffer(buffer);
		}
#endif

		if (genertatedSampleFrame > minSampleFrameRequired)
			break;
	}

	switch (result)
	{
	case EAudioStreamStatus::Ok:
		break;
	case EAudioStreamStatus::Error:
		mSampleBuffer.releaseSampleData(sampleHandle);
		break;
	case EAudioStreamStatus::NoSample:
	case EAudioStreamStatus::Eof:
		if (sampleData->data.empty())
		{
			mSampleBuffer.releaseSampleData(sampleHandle);
		}
		break;
	}

	if (!sampleData->data.empty())
	{
		outSample.handle = sampleHandle;
		outSample.data = sampleData->data.data();
		outSample.dataSize = sampleData->data.size();
	}
	else
	{
		outSample.handle = INDEX_NONE;
		outSample.data = nullptr;
		outSample.dataSize = 0;
	}

	return result;
}
