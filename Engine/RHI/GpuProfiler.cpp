#include "GpuProfiler.h"

#include <cstdarg>
#include "SystemPlatform.h"

namespace Render
{

	GpuProfiler::GpuProfiler()
	{
		mbStartSampling = false;
	}

#if CORE_SHARE_CODE
	GpuProfiler& GpuProfiler::Get()
	{
		static GpuProfiler sInstance;
		return sInstance;
	}

	void GpuProfiler::releaseRHIResource()
	{
		for (int i = 0; i < ARRAY_SIZE(mFrameBuffers); ++i)
		{
			mFrameBuffers[i].clear();
		}

		if (mCore)
		{
			mCore->releaseRHI();
			delete mCore;
			mCore = nullptr;
		}
	}

	void GpuProfiler::beginFrame()
	{
		auto& frameData = getWriteData();
		frameData.numSampleUsed = 0;
	
		mCurLevel = 0;
		if( mCore )
		{
			mCore->beginFrame();
			mbStartSampling = true;
			InlineString< 512 > str;
			str.format("Frame : %s System", Literal::ToString(GRHISystem->getName()));
			mRootSample = startSample(str);
		}
	}


	void GpuProfiler::endFrame()
	{
		if( mCore )
		{
			if( mbStartSampling )
			{
				endSample(*mRootSample);
				if( mCore->endFrame() )
				{
					mCore->beginReadback();

					auto& frameData = getWriteData();

					for( int i = 0; i < frameData.numSampleUsed; ++i )
					{
						GpuProfileSample* sample = frameData.samples[i].get();

						if( sample->timingHandle != RHI_ERROR_PROFILE_HANDLE )
						{
							uint64 time;
							while( !mCore->getTimingDuration(sample->timingHandle, time) )
							{
								LogMsg("GpuProfiler Can't get sample");
								SystemPlatform::Sleep(0);
							}
							sample->time = double(time) * mCycleToSecond;
						}
						else
						{
							sample->time = -1;
						}
					}
					mCore->endReadback();
				}
				mbStartSampling = false;
			}
		}

		{
			RWLock::WriteLocker locker(mIndexLock);
			mIndexWriteBuffer = 1 - mIndexWriteBuffer;
		}

	}

	GpuProfileSample* GpuProfiler::startSample(char const* name)
	{
		if( !mbStartSampling  || mCore == nullptr)
			return nullptr;

		auto& frameData = getWriteData();

		GpuProfileSample* sample;
		if(frameData.numSampleUsed >= frameData.samples.size() )
		{
			frameData.samples.push_back(std::make_unique< GpuProfileSample>());
			sample = frameData.samples.back().get();
			sample->timingHandle = mCore->fetchTiming();
		}
		else
		{
			sample = frameData.samples[frameData.numSampleUsed].get();
		}

		sample->name = name;
		sample->level = mCurLevel;
		++mCurLevel;
		++frameData.numSampleUsed;
		if ( sample->timingHandle != RHI_ERROR_PROFILE_HANDLE )
			mCore->startTiming(sample->timingHandle);
		return sample;
	}

	void GpuProfiler::endSample(GpuProfileSample& sample)
	{
		if( sample.timingHandle != RHI_ERROR_PROFILE_HANDLE )
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
			for (int i = 0; i < ARRAY_SIZE(mFrameBuffers); ++i)
			{
				mFrameBuffers[i].clear();
			}
		}
	}

}//namespace Render


