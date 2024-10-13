#include "AudioTestStage.h"

#include "Stage/TestStageHeader.h"

#include "Audio/AudioDecoder.h"
#include "Audio/XAudio2/XAudio2Device.h"
#include "Audio/XAudio2/MFDecoder.h"

#include "Widget/WidgetUtility.h"

#include "Core/ScopeGuard.h"
#include "BitUtility.h"
#include "Algo/FFT.h"
#include "ProfileSystem.h"


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


	std::vector< Complex > outFeq(samplesToRead);
	{
		PROFILE_ENTRY("FFT Transform");
		static FFT::Context context;
		if (context.sampleSize != samplesToRead)
		{
			context.set(samplesToRead);
		}
		FFT::Transform(context, &data[0], samplesToRead, outFeq.data());
	}
	
#if 0
	std::vector< Complex > outFeqKiss(samplesToRead);
	{
		PROFILE_ENTRY("FFT Transform Kiss");
		FFT::TransformKiss(&data[0], samplesToRead, outFeqKiss.data());
	}
#endif

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
		mFormat.byteRate = mFormat.blockAlign * mFormat.sampleRate;
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

	virtual EAudioStreamStatus generatePCMData(int64 samplePos, AudioStreamSample& outSample, int minSampleFrameRequired, bool bNeedFlush) override
	{
		outSample = mSampleBuffer.allocSamples(minSampleFrameRequired * mFormat.blockAlign);

		float dt = float(minSampleFrameRequired) / mFormat.sampleRate;
		int16* pData = (int16*)outSample.data;
		for( int i = 0; i < minSampleFrameRequired; ++i )
		{
			float t = double(samplePos + i) / mFormat.sampleRate;
			float value = mWaveGenerator(t);
			pData[i] = int16( float( MaxInt16 ) * Math::Clamp< float>(value , -1 , 1 ) );
		}

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

	virtual void onUpdate(GameTimeSpan deltaTime);

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

	MsgReply onMouse(MouseMsg const& msg);

	MsgReply onKey(KeyMsg const& msg);


protected:
};

REGISTER_STAGE_ENTRY("Audio Test", AudioTestStage, EExecGroup::FeatureDev);

bool AudioTestStage::onInit()
{
	TIME_SCOPE("AudioTestStage Init");

	if( !BaseClass::onInit() )
		return false;

	FFT::PrintIndex(16);

	{
		TIME_SCOPE("AudioDevice Create");
		mAudioDevice = AudioDevice::Create();
		if (mAudioDevice == nullptr)
			return false;
	}
	{
		TIME_SCOPE("MFStartup");
		HRESULT hr = MFStartup(MF_VERSION);
	}

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
		if( !FMFDecodeUtil::LoadWaveFile("Sounds/gem2.mp3", mSoundWave.format, mSoundWave.PCMData) )
			return false;
	}
#else
	{
		TIME_SCOPE("Init Stream Source");
		mAudioStreamSource.initialize("Sounds/gem2.mp3");
		//mSoundWave.setupStream(&mSineWaveStreamSource);
		mSoundWave.setupStream(&mAudioStreamSource, true);
	}
#endif
#endif

#endif


	mAudioHandle = mAudioDevice->playSound2D(mSoundWave, 0.3, false);

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

	frame->addText("Pos");
	auto sliderPos = frame->addSlider(UI_ANY);
	sliderPos->setRange(0, 100);
	sliderPos->setValue(0);
	sliderPos->onEvent = [&](int event, GWidget* ui)-> bool
	{
		int value = GUI::CastFast<GSlider>(ui)->getValue();
		mAudioStreamSource.setCurrentTime(double(value) / 100.0 * mAudioStreamSource.getDuration());
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
	FWidgetProperty::Bind(frame->addSlider(UI_ANY), mSineWaveStreamSource.mWaveGenerator.freq, 20, 15100, 2);


	restart();
	return true;
}

void AudioTestStage::onUpdate(GameTimeSpan deltaTime)
{
	BaseClass::onUpdate(deltaTime);

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

		if( CalcFrequencySpectrum(*soundInstance->soundwave, timePos, 1024 * 8, mNumFreqGroup, mFreqValues) )
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

	if (bMoving)
	{
		moveDeltaT += deltaTime;
		float factor = moveDeltaT / 3;

		if (factor > 1)
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

MsgReply AudioTestStage::onKey(KeyMsg const& msg)
{
	if(msg.isDown())
	{
		switch (msg.getCode())
		{
		case EKeyCode::R: restart(); break;
		case EKeyCode::B:
			mAudioDevice->stopAllSound();
			mAudioHandle = mAudioDevice->playSound2D(mSoundWave, 1.0, false);
			break;
		case EKeyCode::Z:
			mSoundWave.PCMData.clear();
			break;
		}
	}
	return BaseClass::onKey(msg);
}

MsgReply AudioTestStage::onMouse(MouseMsg const& msg)
{
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
	return BaseClass::onMouse(msg);
}
