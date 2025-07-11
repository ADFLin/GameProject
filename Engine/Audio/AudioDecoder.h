#pragma once
#ifndef AudioDecoder_H_CC85244B_CE53_48DB_9F8D_C1069449A8A9
#define AudioDecoder_H_CC85244B_CE53_48DB_9F8D_C1069449A8A9

#include "Audio/AudioTypes.h"

#include "DataStructure/Array.h"
#include "AudioStreamSource.h"


class WaveSampleBuffer
{
public:
	struct SampleData
	{
		TArray<uint8> data;
	};
	using SampleHandle = uint32;


	TArray< uint32 > mFreeSampleDataIndices;
	TArray< std::unique_ptr< SampleData > > mSampleDataList;

	SampleData* getSampleData(SampleHandle handle) { return mSampleDataList[handle].get(); }


	AudioStreamSample allocSamples(uint32 size)
	{
		AudioStreamSample result;
		result.handle = acquireSampleData();
		SampleData* sampleData = getSampleData(result.handle);
		sampleData->data.resize(size);
		result.data = sampleData->data.data();
		result.dataSize = size;
		return result;
	}

	SampleHandle acquireSampleData()
	{
		uint32 result;
		if (!mFreeSampleDataIndices.empty())
		{
			result = mFreeSampleDataIndices.back();
			mFreeSampleDataIndices.pop_back();
			return result;
		}

		result = (uint32)mSampleDataList.size();
		auto sampleData = std::make_unique<SampleData>();
		mSampleDataList.push_back(std::move(sampleData));
		return result;
	}

	void releaseSampleData(SampleHandle sampleHandle)
	{
		auto& sampleData = mSampleDataList[sampleHandle];
		sampleData->data.clear();
		mFreeSampleDataIndices.push_back(sampleHandle);
	}
};

#endif // AudioDecoder_H_CC85244B_CE53_48DB_9F8D_C1069449A8A9
