#include "GpuProfiler.h"

#include "FixString.h"
#include <cstdarg>

namespace Render
{

	GpuProfiler::GpuProfiler()
	{
		numSampleUsed = 0;
		bStartSampling = false;
	}

#if CORE_SHARE_CODE
	GpuProfiler& GpuProfiler::Get()
	{
		static GpuProfiler sInstance;
		return sInstance;
	}
#endif

	void GpuProfiler::beginFrame()
	{
		numSampleUsed = 0;
		mCurLevel = 0;

		if( mCore )
		{
			mCore->beginFrame();
			bStartSampling = true;
		}
	}


	void GpuProfiler::endFrame()
	{
		if( mCore )
		{
			mCore->endFrame();

			if ( bStartSampling )
			{
				for( int i = 0; i < numSampleUsed; ++i )
				{
					GpuProfileSample* sample = mSamples[i].get();
					uint64 time;
					while( !mCore->getTimingDurtion(sample->timingHandle, time) )
					{

					}
					sample->time = double(time) * cycleToSecond;
				}

				bStartSampling = false;
			}
		}
	}

	void GpuProfiler::setCore(RHIProfileCore* core)
	{
		assert(!bStartSampling);
		mCore = core;
		if( mCore )
		{
			cycleToSecond = mCore->getCycleToSecond();
		}
		else
		{
			mSamples.clear();
		}
	}

	GpuProfileSample* GpuProfiler::startSample(char const* name)
	{
		if( !bStartSampling )
			return nullptr;

		assert(mCore);
		GpuProfileSample* sample;
		if( numSampleUsed >= mSamples.size() )
		{
			mSamples.push_back(std::make_unique< GpuProfileSample>());
			sample = mSamples.back().get();		
		}
		else
		{
			sample = mSamples[numSampleUsed].get();
		}
		sample->name = name;
		sample->level = mCurLevel;
		sample->timingHandle = mCore->fetchTiming();
		++mCurLevel;
		++numSampleUsed;
		mCore->startTiming(sample->timingHandle);
		return sample;
	}

	void GpuProfiler::endSample(GpuProfileSample& sample)
	{
		mCore->endTiming(sample.timingHandle);
		--mCurLevel;
	}

	GpuProfileScope::GpuProfileScope(NoVA, char const* name)
	{
		sample = GpuProfiler::Get().startSample(name);
	}

	GpuProfileScope::GpuProfileScope(char const* format, ...)
	{
		FixString< 256 > name;
		va_list argptr;
		va_start(argptr, format);
		name.formatVA(format, argptr);
		va_end(argptr);
		sample = GpuProfiler::Get().startSample(name);
	}

	GpuProfileScope::~GpuProfileScope()
	{
		if ( sample )
			GpuProfiler::Get().endSample(*sample);
	}

}//namespace Render


