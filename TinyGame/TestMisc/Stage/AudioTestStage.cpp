#include "AudioTestStage.h"

#include "Stage/TestStageHeader.h"

#include "Platform/Windows/MediaFoundationHeader.h"
#include "Audio/XAudio2/XAudio2Device.h"
#include "Widget/WidgetUtility.h"

#include "SystemPlatform.h"
#include "Core/ScopeExit.h"
#include "BitUtility.h"
#include "Algo/FFT.h"



constexpr int16 MaxInt16 = 32767l;


float GetFFTInValue(const int16 SampleValue, const int16 sampleIndex, const int16 sampleCount)
{
	float FFTValue = SampleValue;
	//Hann window
	FFTValue *= 0.5f * (1 - Math::Cos(2 * PI * sampleIndex / (sampleCount - 1)));

	return FFTValue;
}

bool CalcFrequencySpectrum(SoundWave& sound, float t, int samplesToRead, int numFreqGroup , std::vector< float >& outValues )
{

	int   firstSample = Math::FloorToInt(sound.format.sampleRate * t);
	int   lastSample = firstSample + samplesToRead;

	int   sampleCount = 8 * sound.PCMData.size() / sound.format.bitsPerSample;
	firstSample = Math::Min(sampleCount, firstSample);
	lastSample = Math::Min(sampleCount, lastSample);

	samplesToRead = lastSample - firstSample;

	if( samplesToRead <= 0 )
		return false;

	int pot = FBitUtility::NextNumberOfPow2(samplesToRead);
	firstSample = Math::Max(0, firstSample - (pot - samplesToRead) / 2);
	samplesToRead = pot;
	lastSample = firstSample + samplesToRead;
	if( lastSample > sampleCount )
	{
		firstSample = lastSample - samplesToRead;
	}
	if( firstSample < 0 )
	{
		return false;
	}
	int numChannel = sound.format.numChannels;

	std::vector< float > data(samplesToRead, 0.0f);
	const int16* samplePtr = reinterpret_cast<const int16*>(sound.PCMData.data()) + numChannel * firstSample;

	for( int32 indexFreqSample = 0; indexFreqSample < samplesToRead; ++indexFreqSample )
	{
		for( int32 indexChannel = 0; indexChannel < numChannel; ++indexChannel )
		{
			data[indexFreqSample] += GetFFTInValue(*samplePtr, indexFreqSample, samplesToRead);
			++samplePtr;
		}
	}

	double time = SystemPlatform::GetHighResolutionTime();
	std::vector< Complex > outFeq(samplesToRead);
	FFT::Transform(&data[0], samplesToRead, &outFeq[0]);
	time = SystemPlatform::GetHighResolutionTime() - time;
	LogMsg("FFT = %lf , %d ", time, firstSample);


	int numSampleInGroup = samplesToRead / ( 2 * numFreqGroup );
	int numSampleRes = samplesToRead % ( 2 * numFreqGroup );

	outValues.resize(numFreqGroup, 0.0f);
	int indexFreqSample = 0;
	for( int indexGroup = 0; indexGroup < numFreqGroup; ++indexGroup )
	{
		int numFreqSamples = numSampleInGroup;
		if( numSampleRes > 0 )
			++numSampleRes;
		--numSampleRes;
		float sampleFreqSum = 0;
		for( int i = 0; i < numFreqSamples; ++i )
		{
			float PostScaledR = outFeq[indexFreqSample].r * 2.f / samplesToRead;
			float PostScaledI = outFeq[indexFreqSample].i * 2.f / samplesToRead;
			float val = 10.f * ( Math::LogX(10.f, 10 + Math::Square(PostScaledR) + Math::Square(PostScaledI)) - 1 );
			++indexFreqSample;
			sampleFreqSum += val;
		}
		outValues[indexGroup] = sampleFreqSum / numFreqSamples;
	}

	return true;
}

struct WavePattern_Triangle
{
	float amplitude;
	float freq;
	float operator()(float t) const
	{
		float x = Math::Frac(t * freq);
		float value = (x > 0.5) ? 1 - x : x;
		value = 4 * value - 1;
		return amplitude * value;
	}
};


struct WavePattern_Sine
{
	float amplitude;
	float freq;
	float operator()(float t) const
	{
		float x = t * freq;
		float value = -Math::Sin(3 * Math::PI * x) / 4 + Math::Sin( Math::PI * x) / 4 + Math::Sqrt(3) * Math::Cos( Math::PI * x) / 2;
		return value * amplitude;
		//float value =  Math::Pow(Math::Sin(Math::PI * x), 3) + Math::Sin(Math::PI*(x + 2.0 / 3.0));
		return amplitude * Math::Sin(2 * Math::PI * t * freq);
	}
};

class WaveSampleBuffer
{
public:
	struct SampleData
	{
		std::vector<uint8> data;
	};
	std::vector< uint32 > mFreeSampleDataIndices;
	std::vector< std::unique_ptr< SampleData > > mSampleDataList;
	SampleData* getSampleData(uint32 idx) { return mSampleDataList[idx].get(); }

	uint32 fetchSampleData()
	{
		uint32 result;
		if( mFreeSampleDataIndices.size() )
		{
			result = mFreeSampleDataIndices.back();
			mFreeSampleDataIndices.pop_back();
			return result;
		}

		result = mSampleDataList.size();
		auto sampleData = std::make_unique<SampleData>();
		mSampleDataList.push_back(std::move(sampleData));
		return result;

	}

	void releaseSampleData(uint32 sampleHadle)
	{
		auto& sampleData = mSampleDataList[sampleHadle];
		sampleData->data.clear();
		mFreeSampleDataIndices.push_back(sampleHadle);
	}
};

class ProceduralWaveStreamSource : public IAudioStreamSource
{
public:

	WaveFormatInfo mFormat;
	ProceduralWaveStreamSource()
	{
		mWaveGenerator.amplitude = 0.25;
		mWaveGenerator.freq = 1000;

		mFormat.tag = WAVE_FORMAT_PCM;
		mFormat.numChannels = 1;
		mFormat.sampleRate = 4 * 44100 + 44100/2;
		mFormat.bitsPerSample = 8 * sizeof(int16);
		mFormat.blockAlign = mFormat.numChannels * mFormat.bitsPerSample / 8;
		mFormat.byteRate = mFormat.bitsPerSample * mFormat.sampleRate * mFormat.numChannels / 8;
	}

	virtual void seekSamplePosition(int64 samplePos) override
	{

	}

	virtual void getWaveFormat(WaveFormatInfo& outFormat) override
	{
		outFormat = mFormat;
	}


	virtual int64 getTotalSampleNum() override
	{
		return 100000 * mFormat.sampleRate;
	}

	virtual EAudioStreamStatus generatePCMData(int64 samplePos, AudioStreamSample& outSample, int requiredMinSameleNum) override
	{
		outSample.handle = mSampleBuffer.fetchSampleData();
		WaveSampleBuffer::SampleData* sampleData = mSampleBuffer.getSampleData(outSample.handle);
		sampleData->data.resize( requiredMinSameleNum * mFormat.byteRate / mFormat.sampleRate );

		float dt = float( requiredMinSameleNum ) / mFormat.sampleRate;
		int16* pData = (int16*) sampleData->data.data();
		for( int i = 0; i < requiredMinSameleNum; ++i )
		{
			float t = double(samplePos + i) / mFormat.sampleRate;
			float value = mWaveGenerator(t);
			pData[i] = int16( float( MaxInt16 ) * Math::Clamp< float>(value , -1 , 1 ) );
		}
		outSample.data = sampleData->data.data();
		outSample.dataSize = sampleData->data.size();

		return EAudioStreamStatus::Ok;
	}

	virtual void releaseSampleData(uint32 sampleHadle) override
	{
		mSampleBuffer.releaseSampleData(sampleHadle);
	}

	WavePattern_Sine mWaveGenerator;
	WaveSampleBuffer mSampleBuffer;
};


void GenerateSineWave(float duration, int numSamplePerSec, float waveFreq, std::vector< uint8 >& outSampleData)
{
	int const numByte = sizeof(int16);
	int const numSample = Math::FloorToInt(numSamplePerSec * duration) + 1;
	outSampleData.resize(numSample * numByte);

	float const factor = 2 * Math::PI * waveFreq * duration / numSample;

	int16* pData = (int16*)&outSampleData[0];
	for( int i = 0; i < numSample; ++i )
	{
		int16 value = int16(MaxInt16  * Math::Sin(factor * i));
		*pData = value;
		++pData;
	}
}

void GenerateRectWave(float duration, int numSamplePerSec, float waveFreq, std::vector< uint8 >& outSampleData)
{
	int const numByte = sizeof(int16);
	int const numSample = Math::FloorToInt(numSamplePerSec * duration) + 1;
	outSampleData.resize(numSample * numByte);

	int16* pData = (int16*)&outSampleData[0];
	for( int i = 0; i < numSample; ++i )
	{
		float t = i * duration / numSample;
		float x = Math::Frac(t * waveFreq);
		float value = (x > 0.5) ? 1 : -1;
		*pData = int16(MaxInt16  * value);
		++pData;
	}
}

void GenerateTriangleWave(float duration, int numSamplePerSec, float waveFreq, std::vector< uint8 >& outSampleData)
{
	int const numByte = sizeof(int16);
	int const numSample = Math::FloorToInt(numSamplePerSec * duration) + 1;
	outSampleData.resize(numSample * numByte);

	int16* pData = (int16*)&outSampleData[0];
	for( int i = 0; i < numSample; ++i )
	{
		float t = i * duration / numSample;
		float x = Math::Frac(t * waveFreq);
		float value = (x > 0.5) ? 1 - x : x;
		value = 4 * value - 1;
		*pData = int16(MaxInt16  * value );
		++pData;
	}
}

static bool ConfigureAudioStream(
	IMFSourceReader *pReader,   // Pointer to the source reader.
	IMFMediaType **ppPCMAudio   // Receives the audio format.
)
{
	// Select the first audio stream, and deselect all other streams.
	CHECK_RETRUN(pReader->SetStreamSelection(MF_SOURCE_READER_ALL_STREAMS, FALSE), false);

	CHECK_RETRUN(pReader->SetStreamSelection(MF_SOURCE_READER_FIRST_AUDIO_STREAM, TRUE), false);

	TComPtr< IMFMediaType > pPartialType;
	// Create a partial media type that specifies uncompressed PCM audio.
	CHECK_RETRUN(MFCreateMediaType(&pPartialType), false);

	CHECK_RETRUN(pPartialType->SetGUID(MF_MT_MAJOR_TYPE, MFMediaType_Audio), false);
	CHECK_RETRUN(pPartialType->SetGUID(MF_MT_SUBTYPE, MFAudioFormat_PCM), false);

	// Set this type on the source reader. The source reader will
	// load the necessary decoder.
	CHECK_RETRUN(pReader->SetCurrentMediaType(MF_SOURCE_READER_FIRST_AUDIO_STREAM, NULL, pPartialType), false);

	TComPtr< IMFMediaType > pUncompressedAudioType;
	// Get the complete uncompressed format.
	CHECK_RETRUN(pReader->GetCurrentMediaType(MF_SOURCE_READER_FIRST_AUDIO_STREAM, &pUncompressedAudioType), false);

	// Ensure the stream is selected.
	CHECK_RETRUN(pReader->SetStreamSelection(MF_SOURCE_READER_FIRST_AUDIO_STREAM, TRUE), false);

	// Return the PCM format to the caller.

	*ppPCMAudio = pUncompressedAudioType;
	(*ppPCMAudio)->AddRef();
	return true;
}

static void  FillData(WaveFormatInfo& formatInfo, WAVEFORMATEX const& format)
{
	formatInfo.tag = format.wFormatTag;
	formatInfo.numChannels = format.nChannels;
	formatInfo.sampleRate = format.nSamplesPerSec;
	formatInfo.bitsPerSample = format.wBitsPerSample;
	formatInfo.byteRate = format.nAvgBytesPerSec;
	formatInfo.blockAlign = format.nBlockAlign;
}

int64 GetSourceDuration(IMFSourceReader *pReader)
{
	PROPVARIANT DurationAttrib;
	{
		const HRESULT Result = pReader->GetPresentationAttribute(MF_SOURCE_READER_MEDIASOURCE, MF_PD_DURATION, &DurationAttrib);
		if( FAILED(Result) )
		{
			return 0;
		}
	}
	const int64 Duration = (DurationAttrib.vt == VT_UI8) ? (int64)DurationAttrib.uhVal.QuadPart : 0;
	::PropVariantClear(&DurationAttrib);
	return Duration;
}

static bool WriteWaveFile(IMFSourceReader *pReader, WaveFormatInfo& outWaveFormat, std::vector<uint8>& outSampleData)
{


	HRESULT hr = S_OK;
	TComPtr< IMFMediaType > pAudioType;    // Represents the PCM audio format.
										   // Configure the source reader to get uncompressed PCM audio from the source file.
	if( !ConfigureAudioStream(pReader, &pAudioType) )
		return false;

	DWORD cbHeader = 0;         // Size of the WAVE file header, in bytes.
	DWORD cbAudioData = 0;      // Total bytes of PCM audio data written to the file.
	DWORD cbMaxAudioData = 0;

	{
		WAVEFORMATEX *pWavFormat = NULL;
		UINT32 cbFormat = 0;
		// Convert the PCM audio format into a WAVEFORMATEX structure.
		CHECK_RETRUN(MFCreateWaveFormatExFromMFMediaType(pAudioType, &pWavFormat, &cbFormat), false);
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
		for( ;;)
		{
			DWORD dwFlags = 0;
			TComPtr<IMFSample> pSample;
			// Read the next sample.
			hr = pReader->ReadSample(MF_SOURCE_READER_FIRST_AUDIO_STREAM, 0, NULL, &dwFlags, NULL, &pSample);

			if( FAILED(hr) ) { break; }

			if( dwFlags & MF_SOURCE_READERF_CURRENTMEDIATYPECHANGED )
			{
				printf("Type change - not supported by WAVE file format.\n");
				return false;
			}
			if( dwFlags & MF_SOURCE_READERF_ENDOFSTREAM )
			{
				printf("End of input file.\n");
				break;
			}

			if( pSample == nullptr )
			{
				printf("No sample\n");
				continue;
			}

			// Get a pointer to the audio data in the sample.
			TComPtr<IMFMediaBuffer> pBuffer;
			CHECK_RETRUN(pSample->ConvertToContiguousBuffer(&pBuffer), false);

			DWORD cbBuffer = 0;
			BYTE *pAudioData = NULL;
			{
				CHECK_RETRUN(pBuffer->Lock(&pAudioData, NULL, &cbBuffer), false);
				ON_SCOPE_EXIT{ pBuffer->Unlock(); };
				outSampleData.insert(outSampleData.end(), pAudioData, pAudioData + cbBuffer);
			}
			// Update running total of audio data.
			cbAudioData += cbBuffer;
		}
	}

	return true;
}

static bool LoadWaveFile(char const* path, WaveFormatInfo& outWaveFormat, std::vector<uint8>& outSampleData)
{
	MediaFoundationScope MFScope;
	if( !MFScope.bInit )
		return false;

	const size_t cSize = strlen(path) + 1;
	wchar_t wPath[MAX_PATH];
	mbstowcs(wPath, path, cSize);

	TComPtr< IMFSourceReader  > pReader;
	// Create the source reader to read the input file.
	CHECK_RETRUN(MFCreateSourceReaderFromURL(wPath, NULL, &pReader), false);

	// Write the WAVE file.
	if( !WriteWaveFile(pReader, outWaveFormat, outSampleData) )
		return false;

	return true;
}

class MFAudioStreamSource : public IAudioStreamSource
{
public:

	bool initialize(char const* path)
	{
		const size_t cSize = strlen(path) + 1;
		wchar_t wPath[MAX_PATH];
		mbstowcs(wPath, path, cSize);
		CHECK_RETRUN(MFCreateSourceReaderFromURL(wPath, NULL, &mSourceReader), false);

		HRESULT hr = S_OK;
		TComPtr< IMFMediaType > pAudioType;    // Represents the PCM audio format.
											   // Configure the source reader to get uncompressed PCM audio from the source file.
		if( !ConfigureAudioStream(mSourceReader, &pAudioType) )
			return false;

		DWORD cbHeader = 0;         // Size of the WAVE file header, in bytes.
		DWORD cbAudioData = 0;      // Total bytes of PCM audio data written to the file.
		DWORD cbMaxAudioData = 0;

		{
			WAVEFORMATEX *pWavFormat = NULL;
			UINT32 cbFormat = 0;
			// Convert the PCM audio format into a WAVEFORMATEX structure.
			CHECK_RETRUN(MFCreateWaveFormatExFromMFMediaType(pAudioType, &pWavFormat, &cbFormat), false);
			FillData(mWaveFormat, *pWavFormat);
			CoTaskMemFree(pWavFormat);
		}

		int64 duration = GetSourceDuration(mSourceReader);
		mTotalSampleNum = mWaveFormat.sampleRate * duration  / 10000000;

		return true;
	}

	int64 mTotalSampleNum;
	WaveFormatInfo mWaveFormat;
	TComPtr< IMFSourceReader  > mSourceReader;


	virtual void seekSamplePosition(int64 samplePos) override
	{
		int64 duration = 10000000 * samplePos / mWaveFormat.byteRate;
		PROPVARIANT var;
		HRESULT hr = InitPropVariantFromInt64(duration, &var);
		hr = mSourceReader->SetCurrentPosition(GUID_NULL, var);
		PropVariantClear(&var);

	}
	virtual void getWaveFormat(WaveFormatInfo& outFormat) override
	{
		outFormat = mWaveFormat;
	}


	virtual int64 getTotalSampleNum() override
	{
		return mTotalSampleNum;
	}
#define USE_COM_BUFFER 0

	virtual EAudioStreamStatus generatePCMData(int64 samplePos, AudioStreamSample& outSample, int requiredMinSameleNum) override
	{
		HRESULT hr = S_OK;

		EAudioStreamStatus result = EAudioStreamStatus::Ok;
		// Get audio samples from the source reader.

		int genertatedSampleNum = 0;

		uint32 idxSampleData = fetchSampleData();
		auto& sampleData = mSampleDataList[idxSampleData];
		assert(sampleData->data.empty());
		for(;;)
		{
			DWORD dwFlags = 0;
			TComPtr<IMFSample> pSample;
			// Read the next sample.
			hr = mSourceReader->ReadSample(MF_SOURCE_READER_FIRST_AUDIO_STREAM, 0, NULL, &dwFlags, NULL, &pSample);

			if( FAILED(hr) )
			{
				result = EAudioStreamStatus::Error;
				break;
			}

			if( dwFlags & MF_SOURCE_READERF_CURRENTMEDIATYPECHANGED )
			{
				LogWarning(0, "Type change - not supported by WAVE file format.\n");
				return EAudioStreamStatus::Error;
				break;
			}
			if( dwFlags & MF_SOURCE_READERF_ENDOFSTREAM )
			{
				result = EAudioStreamStatus::Eof;
				break;
			}

			if( pSample == nullptr )
			{
				result = EAudioStreamStatus::NoSample;
				break;
			}

			TComPtr<IMFMediaBuffer> buffer;
			hr = pSample->ConvertToContiguousBuffer(&buffer);
			if( FAILED(hr) )
			{
				result = EAudioStreamStatus::Error;
				break;
			}

			DWORD cbBuffer = 0;
			BYTE *pAudioData = NULL;

			hr = buffer->Lock(&pAudioData, NULL, &cbBuffer);
			assert((cbBuffer % mWaveFormat.bitsPerSample) == 0);
			sampleData->data.insert( sampleData->data.begin() , pAudioData, pAudioData + cbBuffer);

			buffer->Unlock();

			genertatedSampleNum += cbBuffer * mWaveFormat.sampleRate / mWaveFormat.byteRate;

			if ( genertatedSampleNum > requiredMinSameleNum )
				break;
		}

		switch( result )
		{
		case EAudioStreamStatus::Ok:
			break;
		case EAudioStreamStatus::Error:
			releaseSampleData(idxSampleData);
			break;
		case EAudioStreamStatus::NoSample:
		case EAudioStreamStatus::Eof:
			if( sampleData->data.empty() )
			{
				releaseSampleData(idxSampleData);
			}
			break;
		}

		if ( !sampleData->data.empty() )
		{
			outSample.handle = idxSampleData;
			outSample.data = sampleData->data.data();
			outSample.dataSize = sampleData->data.size();
		}
		else
		{

			outSample.handle = -1;
			outSample.data = nullptr;
			outSample.dataSize = 0;
		}

		return result;
	}


	struct SampleData
	{
		std::vector<uint8> data;
	};
	std::vector< uint32 > mFreeSampleDataIndices;
	std::vector< std::unique_ptr< SampleData > > mSampleDataList;

	uint32 fetchSampleData()
	{
		uint32 result;
		if( mFreeSampleDataIndices.size() )
		{
			result = mFreeSampleDataIndices.back();
			mFreeSampleDataIndices.pop_back();
			return result;
		}

		result = mSampleDataList.size();
		auto sampleData = std::make_unique<SampleData>();
		mSampleDataList.push_back(std::move(sampleData));
		return result;

	}

	void releaseSampleData(uint32 sampleHadle)
	{
		auto& sampleData = mSampleDataList[sampleHadle];
		sampleData->data.clear();
		mFreeSampleDataIndices.push_back(sampleHadle);
	}


};

class AudioTestStage : public StageBase
{
	typedef StageBase BaseClass;
public:
	AudioTestStage() {}

	enum
	{
		UI_AUDIO_VOLUME = BaseClass::NEXT_UI_ID,
		UI_AUDIO_PITCH,
	};

	AudioDevice* mAudioDevice;
	SoundWave    mSoundWave;

	AudioHandle  mAudioHandle;
	GSlider* mTimeWidget;
	float prevRadius;
	float nextRadius;
	Vector2 prevPos;
	Vector2 nextPos;
	Vector2 curPos;
	float curRadius;
	bool bMoving = false;
	float moveDeltaT;
	int step = 0;

	ProceduralWaveStreamSource  mSineWaveStreamSource;
	MFAudioStreamSource         mAudioStreamSource;
	virtual bool onInit();

	virtual void onEnd()
	{
		mAudioDevice->shutdown();

		BaseClass::onEnd();
	}

	void restart()
	{
		bMoving = false;
		step = 0;
	}

	void tick()
	{
		if( bMoving )
		{
			moveDeltaT += float(gDefaultTickTime) / 1000;
			float factor = moveDeltaT / 3;

			if( factor > 1 )
			{

				bMoving = false;
				curPos = prevPos = nextPos;
				curRadius = prevRadius = nextRadius;
			}
			else
			{
				curRadius = Math::LinearLerp(prevRadius, nextRadius, factor);
				curPos = Math::LinearLerp(prevPos, nextPos, factor);
			}


		}
	}
	void updateFrame(int frame) {}

	float mTime = 0;
	int   mNumFreqGroup = 50;
	float mScale = 5;
	std::vector< float > mFreqValues;
	struct HistroyData
	{
		float value;
		float vel;
		float lastUpdateTime;
	};
	std::vector< HistroyData > mHistoryDatas;

	virtual void onUpdate(long time);

	virtual void onRender(float dFrame) override;


	virtual bool onWidgetEvent(int event, int id, GWidget* ui) override
	{
		switch( id )
		{
		case UI_RESTART_GAME:
			{
				restart();

			}
			return false;
		default:
			break;
		}

		return BaseClass::onWidgetEvent(event, id, ui);
	}

	bool onMouse(MouseMsg const& msg);

	bool onKey(KeyMsg const& msg);


protected:
};

REGISTER_STAGE("Audio Test", AudioTestStage, EStageGroup::FeatureDev);

bool AudioTestStage::onInit()
{
	if( !BaseClass::onInit() )
		return false;

	mAudioDevice = AudioDevice::Create();
	if( mAudioDevice == nullptr )
		return false;

	HRESULT hr = MFStartup(MF_VERSION);
#if 0
	if( !mSoundWave.loadFromWaveFile("Sounds/Test.wav") )
	{
		return false;
	}
#else
#if 0
	{
		WaveFormatInfo& format = mSoundWave.format;
		format.tag = WAVE_FORMAT_PCM;
		format.numChannels = 1;
		format.sampleRate = 8000;
		format.bitsPerSample = sizeof(int16) * 8;
		format.byteRate = (format.bitsPerSample * format.sampleRate) / 8;
		format.blockAlign = (format.numChannels * format.bitsPerSample) / 8;
		GenerateTriangleWave(3, format.sampleRate, 500, mSoundWave.PCMData);
	}
#else

#if 0
	{
		if( !LoadWaveFile("Sounds/gem2.mp3", mSoundWave.format, mSoundWave.PCMData) )
			return false;
	}
#else
	mAudioStreamSource.initialize("Sounds/gem2.mp3");
	mSoundWave.bSaveStreamingPCMData = true;
	mSoundWave.setupStream(&mSineWaveStreamSource);
	//mSoundWave.setupStream(&mAudioStreamSource);
#endif
#endif

#endif


	mAudioHandle = mAudioDevice->playSound2D(&mSoundWave, 0.3, false);

	::Global::GUI().cleanupWidget();

	DevFrame* frame = WidgetUtility::CreateDevFrame();

	//frame->addButton(UI_RESTART_GAME, "Restart");
	GSlider* slider;
	frame->addText("Volume");
	slider = frame->addSlider(UI_AUDIO_VOLUME);
	slider->setRange(0, 500);
	slider->setValue(10);
	slider->onEvent = [&](int event, GWidget* ui)-> bool
	{
		int value = GUI::CastFast<GSlider>(ui)->getValue();
		mAudioDevice->setAudioVolumeMultiplier(mAudioHandle, float(value) / 100);
		return false;
	};

	frame->addText("Pitch");
	auto sliderPitch = frame->addSlider(UI_AUDIO_PITCH);
	sliderPitch->setRange(0, 200);
	sliderPitch->setValue(100);
	sliderPitch->onEvent = [&](int event, GWidget* ui)-> bool
	{
		int value = GUI::CastFast<GSlider>(ui)->getValue();
		mAudioDevice->setAudioPitchMultiplier(mAudioHandle, float(value) / 100);
		return false;
	};

	frame->addButton("Reset pitch", [&, sliderPitch](int event, GWidget* ui)-> bool
	{
		sliderPitch->setValue(100);
		mAudioDevice->setAudioPitchMultiplier(mAudioHandle, 1);
		return false;
	});


	mTimeWidget = frame->addSlider(UI_ANY);
	mTimeWidget->setRange(0, 1000);

	frame->addText("Freq");
	WidgetPropery::Bind(frame->addSlider(UI_ANY), mSineWaveStreamSource.mWaveGenerator.freq, 20, 15100, 2);


	restart();
	return true;
}

void AudioTestStage::onUpdate(long time)
{
	BaseClass::onUpdate(time);

	float deltaTime = float(time) / 1000;
	mAudioDevice->update(deltaTime);
	mTime += deltaTime;
	ActiveSound* sound = mAudioDevice->getActiveSound(mAudioHandle);
	if( sound && !sound->playingInstances.empty() )
	{

		SoundInstance* soundInstance = sound->playingInstances[0];
		auto const& format = soundInstance->soundwave->format;
		double timePos = double(soundInstance->samplesPlayed) / format.sampleRate;

		//LogMsg("%lld", soundInstance->samplesPlayed);
		mTimeWidget->setValue(timePos / soundInstance->soundwave->getDuration() * (mTimeWidget->getMaxValue() - mTimeWidget->getMinValue()));

		if( CalcFrequencySpectrum(*soundInstance->soundwave, timePos, 1024 * 4, mNumFreqGroup, mFreqValues) )
		{
			if( mHistoryDatas.size() != mFreqValues.size() )
			{
				HistroyData data;
				data.value = 0;
				data.lastUpdateTime = mTime;
				mHistoryDatas.resize(mFreqValues.size(), data);
			}

			for( int i = 0; i < mHistoryDatas.size(); ++i )
			{
				HistroyData& data = mHistoryDatas[i];
				float curValue = mScale * mFreqValues[i];
				if( data.value < curValue )
				{
					data.value = curValue;
					data.lastUpdateTime = mTime;
					data.vel = 0;
				}
				else if( (mTime - data.lastUpdateTime) > 0.05 )
				{
					data.vel -= 200 * deltaTime;
					data.value += deltaTime * data.vel;
				}
			}
		}
		else
		{
			mFreqValues.clear();
		}
	}

	int frame = time / gDefaultTickTime;
	for( int i = 0; i < frame; ++i )
		tick();

	updateFrame(frame);
}

void AudioTestStage::onRender(float dFrame)
{
	Graphics2D& g = Global::GetGraphics2D();

	if( step > 0 )
	{
		RenderUtility::SetBrush(g, EColor::Null);
		RenderUtility::SetPen(g, EColor::Blue);
		g.drawCircle(curPos, curRadius);
		if( step > 1 && bMoving )
		{
			RenderUtility::SetBrush(g, EColor::Null);
			RenderUtility::SetPen(g, EColor::Red);
			g.drawCircle(nextPos, nextRadius);

			RenderUtility::SetPen(g, EColor::Null);
			RenderUtility::SetBrush(g, EColor::Red, COLOR_LIGHT);
			g.beginBlend(nextPos - Vector2(nextRadius, nextRadius), 2 * Vector2(nextRadius, nextRadius), 0.5);
			g.drawCircle(nextPos, nextRadius);
			g.endBlend();
		}
	}
	RenderUtility::SetBrush(g, EColor::Yellow);
	RenderUtility::SetPen(g, EColor::Black);

	Vector2 org = Vector2(200, 500);
	for( int i = 0; i < mFreqValues.size(); ++i )
	{
		Vector2 pos = org + Vector2(i * 5, 0);
		Vector2 size = Vector2(5, mScale * mFreqValues[i]);
		g.drawRect(pos - Vector2(0, int(size.y)), size);
	}

	float FallDownDelay = 0.5;
	for( int i = 0; i < mHistoryDatas.size(); ++i )
	{
		HistroyData& data = mHistoryDatas[i];
		Vector2 pos = org + Vector2(i * 5, -int(data.value));
		Vector2 size = Vector2(5, 4);
		g.drawRect(pos - Vector2(0, int(size.y)), size);
	}
	RenderUtility::SetBrush(g, EColor::Yellow);
	RenderUtility::SetPen(g, EColor::Black);
	g.drawRect(org, Vector2(mNumFreqGroup * 5, 10));
}

bool AudioTestStage::onKey(KeyMsg const& msg)
{
	if( !msg.isDown() )
		return false;
	switch(msg.getCode())
	{
	case EKeyCode::R: restart(); break;
	case EKeyCode::B:
		mAudioDevice->stopAllSound();
		mAudioHandle = mAudioDevice->playSound2D(&mSoundWave, 1.0, false);
		break;
	case EKeyCode::Z:
		mSoundWave.PCMData.clear();
		break;
	}
	return false;
}

bool AudioTestStage::onMouse(MouseMsg const& msg)
{
	if( !BaseClass::onMouse(msg) )
		return false;

	if( msg.onLeftDown() && !bMoving )
	{
		if( step == 0 )
		{
			curRadius = prevRadius = 300;
			curPos = prevPos = msg.getPos();
		}
		else
		{
			nextPos = msg.getPos();
			nextRadius = 0.5 * prevRadius;
			bMoving = true;
			moveDeltaT = 0;
		}
		++step;
	}
	return true;
}
