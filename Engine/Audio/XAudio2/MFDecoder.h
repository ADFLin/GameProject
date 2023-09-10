#pragma once
#ifndef MFDecoder_H_3A100EA1_A4F0_49D9_ABCF_20C0C2D956A5
#define MFDecoder_H_3A100EA1_A4F0_49D9_ABCF_20C0C2D956A5

#include "Audio/AudioDecoder.h"
#include "Audio/AudioStreamSource.h"

#include "Platform/Windows/ComUtility.h"
#include "Platform/Windows/MediaFoundationHeader.h"

class FMFDecodeUtil
{
public:

	static bool ConfigureAudioStream(
		IMFSourceReader *pReader,   // Pointer to the source reader.
		IMFMediaType **ppPCMAudio   // Receives the audio format.
	);

	static void  FillData(WaveFormatInfo& formatInfo, WAVEFORMATEX const& format);

	static int64 GetSourceDuration(IMFSourceReader *pReader);

	static bool WriteWaveFile(IMFSourceReader *pReader, WaveFormatInfo& outWaveFormat, TArray<uint8>& outSampleData);

	static bool LoadWaveFile(char const* path, WaveFormatInfo& outWaveFormat, TArray<uint8>& outSampleData);
};


class MFAudioStreamSource : public IAudioStreamSource
{
public:

	bool initialize(char const* path);

	static constexpr int64 SecondTimeUnit = 10000000LL;
	int64 mTotalSampleNum;
	WaveFormatInfo mWaveFormat;
	TComPtr< IMFSourceReader  > mSourceReader;
	WaveSampleBuffer mSampleBuffer;

	double getDuration()
	{
		return double(mTotalSampleNum) / mWaveFormat.sampleRate;
	}

	void setCurrentTime(double time);

	virtual void seekSamplePosition(int64 samplePos) override;

	virtual void getWaveFormat(WaveFormatInfo& outFormat) override;

	virtual int64 getTotalSampleNum() override;
#define USE_COM_BUFFER 0

	virtual EAudioStreamStatus generatePCMData(int64 samplePos, AudioStreamSample& outSample, int requiredMinSameleNum) override;

	void releaseSampleData(uint32 sampleHadle) override
	{
		mSampleBuffer.releaseSampleData(sampleHadle);
	}

};

#endif // MFDecoder_H_3A100EA1_A4F0_49D9_ABCF_20C0C2D956A5