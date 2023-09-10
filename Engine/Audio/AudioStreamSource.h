#ifndef AudioStreamSource_h__
#define AudioStreamSource_h__

#include "AudioTypes.h"

struct AudioStreamSample
{
	uint32 handle;
	uint8* data;
	int64  dataSize;
};

enum class EAudioStreamStatus
{
	Ok,
	Error,
	NoSample,
	Eof,
};

class IAudioStreamSource
{
public:
	virtual void  seekSamplePosition(int64 samplePos) = 0;
	virtual void  getWaveFormat(WaveFormatInfo& outFormat) = 0;
	virtual int64 getTotalSampleNum() = 0;
	virtual EAudioStreamStatus  generatePCMData(int64 samplePos, AudioStreamSample& outSample, int requiredMinSameleNum) = 0;
	virtual void  releaseSampleData(uint32 sampleHadle) {}
};

#endif // AudioStreamSource_h__
