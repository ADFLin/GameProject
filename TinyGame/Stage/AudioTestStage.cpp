#include "AudioTestStage.h"

#include "TestStageHeader.h"

#include "Platform/Windows/MediaFoundationHeader.h"
#include "Audio/XAudio2/XAudio2Device.h"
#include "Widget/WidgetUtility.h"

#include "SystemPlatform.h"
#include "Core/ScopeExit.h"


constexpr int16 MaxInt16 = 32767l;

double GTime = 0;




void CalcFreq(SoundWave& sound, float t, float dt, int freqOffset , std::vector< float >& outValues )
{
	float numData = 16384 * 4;//sound.format.sampleRate * dt;
	float idxStart = sound.format.numChannels * Math::FloorToInt(sound.format.sampleRate * t);

	std::vector< float > data(numData , 0.0f);

	assert(sound.format.bitsPerSample == 16);
	int16* pSampleData = (int16*)&sound.PCMData[idxStart];
	for( int i = 0; i < numData; ++i )
	{
		for( int n = 0; n < sound.format.numChannels; ++n )
		{
			data[i] += float(*pSampleData);
			++pSampleData;
		}
	}

	double time = SystemPlatform::GetHighResolutionTime();
	std::vector< Complex > outFeq( numData );
	DFFT::Transform(&data[0], numData, &outFeq[0]);
	time = SystemPlatform::GetHighResolutionTime() - time;
	GTime = time;
	LogMsg("FFT = %lf", time);

	int numGroup = ( numData - 1 )/ freqOffset + 1;
	outValues.resize(numGroup , 0.0f );

	for( int n = 0 ; n < numData; ++n )
	{
		int group = n / freqOffset;
		outValues[group] += outFeq[n].length();
	}
}

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


class IDataStreaming
{




};
class AudioTestStage : public StageBase
{
	typedef StageBase BaseClass;
public:
	AudioTestStage() {}

	enum
	{
		UI_AUDIO_VOLUME = BaseClass::NEXT_UI_ID ,
		UI_AUDIO_PITCH ,
	};

	AudioDevice* mAudioDevice;
	SoundWave    mSoundWave;

	AudioHandle  mAudioHandle;

	float prevRadius;
	float nextRadius;
	Vector2 prevPos;
	Vector2 nextPos;
	Vector2 curPos;
	float curRadius;
	bool bMoving = false;
	float moveDeltaT;
	int step = 0;

	virtual bool onInit()
	{
		if( !BaseClass::onInit() )
			return false;

		mAudioDevice = AudioDevice::Create();
		if( mAudioDevice == nullptr )
			return false;

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

		{
			if( !LoadWaveFile("Sounds/gem.mp3", mSoundWave.format, mSoundWave.PCMData) )
				return false;
		}
#endif

#endif

		std::vector< float > values;
		CalcFreq(mSoundWave, 0, 1, 200, values);
		mAudioHandle = mAudioDevice->playSound2D(&mSoundWave, 1.0 , false);

		::Global::GUI().cleanupWidget();

		DevFrame* frame = WidgetUtility::CreateDevFrame();

		//frame->addButton(UI_RESTART_GAME, "Restart");
		GSlider* slider;
		frame->addText("Volume");
		slider = frame->addSlider(UI_AUDIO_VOLUME);
		slider->setRange(0, 500);
		slider->setValue(100);
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

		frame->addButton("Reset pitch", [&,sliderPitch](int event, GWidget* ui)-> bool
		{
			sliderPitch->setValue(100);
			mAudioDevice->setAudioPitchMultiplier(mAudioHandle, 1);
			return false;
		});
		restart();
		return true;
	}

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

	virtual void onUpdate(long time)
	{
		BaseClass::onUpdate(time);

		mAudioDevice->update(float(time) / 1000);

		int frame = time / gDefaultTickTime;
		for( int i = 0; i < frame; ++i )
			tick();

		updateFrame(frame);
	}

	void onRender(float dFrame)
	{
		Graphics2D& g = Global::getGraphics2D();

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
				g.beginBlend(nextPos - Vector2(nextRadius, nextRadius), 2 * Vector2(nextRadius , nextRadius ) , 0.5 );
				g.drawCircle(nextPos, nextRadius);
				g.endBlend();
			}
		}

	}


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

	bool onMouse(MouseMsg const& msg)
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

	bool onKey(unsigned key, bool isDown)
	{
		if( !isDown )
			return false;
		switch( key )
		{
		case Keyboard::eR: restart(); break;
		case Keyboard::eB:
			mAudioHandle = mAudioDevice->playSound2D(&mSoundWave, 1.0, false);
			break;
		}
		return false;
	}


protected:
};



REGISTER_STAGE("Audio Test", AudioTestStage, EStageGroup::FeatureDev);