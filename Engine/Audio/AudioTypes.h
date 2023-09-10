#pragma once
#ifndef AudioTypes_H_C1408838_30DF_43BA_AF4F_C8EFF43988B8
#define AudioTypes_H_C1408838_30DF_43BA_AF4F_C8EFF43988B8

#include "Core/IntegerType.h"

struct WaveFormatInfo
{
	uint16 tag;
	uint16 numChannels;
	uint32 sampleRate;
	uint32 byteRate;
	uint16 blockAlign;
	uint16 bitsPerSample;
};

#endif // AudioTypes_H_C1408838_30DF_43BA_AF4F_C8EFF43988B8
