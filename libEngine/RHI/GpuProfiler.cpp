#include "GpuProfiler.h"

#include "FixString.h"
#include <cstdarg>

namespace Render
{

	GpuProfiler::GpuProfiler()
	{
		mNumSampleUsed = 0;
		mbStartSampling = false;
	}

#if CORE_SHARE_CODE
	GpuProfiler& GpuProfiler::Get()
	{
		static GpuProfiler sInstance;
		return sInstance;
	}

	void GpuProfiler::beginFrame()
	{
		mNumSampleUsed = 0;
		mCurLevel = 0;

		if( mCore )
		{
			mCore->beginFrame();
			mbStartSampling = true;
			mRootSample = startSample("Frame");
		}
	}


	void GpuProfiler::endFrame()
	{
		if( mCore )
		{
			endSample(*mRootSample);
			mCore->endFrame();

			if ( mbStartSampling )
			{
				for( int i = 0; i < mNumSampleUsed; ++i )
				{
					GpuProfileSample* sample = mSamples[i].get();
					uint64 time;
					while( !mCore->getTimingDurtion(sample->timingHandle, time) )
					{

					}
					sample->time = double(time) * mCycleToSecond;
				}

				mbStartSampling = false;
			}
		}
	}

	GpuProfileSample* GpuProfiler::startSample(char const* name)
	{
		if( !mbStartSampling )
			return nullptr;

		assert(mCore);
		GpuProfileSample* sample;
		if( mNumSampleUsed >= mSamples.size() )
		{
			mSamples.push_back(std::make_unique< GpuProfileSample>());
			sample = mSamples.back().get();
			sample->timingHandle = mCore->fetchTiming();
		}
		else
		{
			sample = mSamples[mNumSampleUsed].get();
		}

		sample->name = name;
		sample->level = mCurLevel;
		++mCurLevel;
		++mNumSampleUsed;
		mCore->startTiming(sample->timingHandle);
		return sample;
	}

	void GpuProfiler::endSample(GpuProfileSample& sample)
	{
		mCore->endTiming(sample.timingHandle);
		--mCurLevel;
	}

#endif

	void GpuProfiler::setCore(RHIProfileCore* core)
	{
		assert(!mbStartSampling);
		mCore = core;
		if( mCore )
		{
			mCycleToSecond = mCore->getCycleToMillisecond();
		}
		else
		{
			mSamples.clear();
		}
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


