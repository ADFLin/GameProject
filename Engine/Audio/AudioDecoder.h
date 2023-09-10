#pragma once
#ifndef AudioDecoder_H_CC85244B_CE53_48DB_9F8D_C1069449A8A9
#define AudioDecoder_H_CC85244B_CE53_48DB_9F8D_C1069449A8A9

#include "Audio/AudioTypes.h"

#include "DataStructure/Array.h"


class WaveSampleBuffer
{
public:
	struct SampleData
	{
		TArray<uint8> data;
	};
	TArray< uint32 > mFreeSampleDataIndices;
	TArray< std::unique_ptr< SampleData > > mSampleDataList;

	SampleData* getSampleData(uint32 idx) { return mSampleDataList[idx].get(); }

	uint32 fetchSampleData()
	{
		uint32 result;
		if (!mFreeSampleDataIndices.empty())
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

#endif // AudioDecoder_H_CC85244B_CE53_48DB_9F8D_C1069449A8A9
