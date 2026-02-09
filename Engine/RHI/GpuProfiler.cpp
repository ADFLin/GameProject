#include "GpuProfiler.h"

#include <cstdarg>
#include "SystemPlatform.h"

namespace Render
{

	GpuProfiler::GpuProfiler()
	{
		mbStartSampling = false;
		mIndexWriteBuffer = 0;
		mIndexReadBuffer = 0;
		for (int i = 0; i < NUM_FRAME_BUFFER; ++i)
		{
			mBufferStatus[i] = false;
		}
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
			str.format("Frame : %s", Literal::ToString(GRHISystem->getName()));
			mRootSample = startSample(str);
		}
	}

	void GpuProfiler::readSamples(FrameData& frameData)
	{
		PROFILE_FUNCTION();

		mCycleToSecond = mCore->getCycleToMillisecond();
		mCore->beginReadback();
		uint64 frameStart = 0;
		for (int i = 0; i < frameData.numSampleUsed; ++i)
		{
			GpuProfileSample* sample = frameData.samples[i].get();
			if (sample->timingHandle != RHI_ERROR_PROFILE_HANDLE)
			{
				uint64 time, start;
				if (mCore->getTimingDuration(sample->timingHandle, time, start))
				{
					if (i == 0) frameStart = start;
					sample->time = double(time) * mCycleToSecond;
					sample->startTime = double(start - frameStart) * mCycleToSecond;
				}
				else
				{
					sample->time = -1;
					sample->startTime = 0;
				}
			}
			else
			{
				sample->time = -1;
				sample->startTime = 0;
			}
		}
		mCore->endReadback();
	}

	void GpuProfiler::endFrame()
	{
		if( mCore )
		{
			if( mbStartSampling )
			{
				mbStartSampling = false;
				endSample(*mRootSample);
				mCore->endFrame();

				{
					RWLock::WriteLocker locker(mIndexLock);
					mBufferStatus[mIndexWriteBuffer] = true;
					mIndexWriteBuffer = (mIndexWriteBuffer + 1) % NUM_FRAME_BUFFER;
				}
			}
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

	bool GpuProfiler::beginRead()
	{
		mIndexLock.readLock();
		// Find the most recent buffer that is ready, but at least 2 frames old to avoid stalling
		int readIndex = (mIndexWriteBuffer + NUM_FRAME_BUFFER - 2) % NUM_FRAME_BUFFER;
		if (mBufferStatus[readIndex])
		{
			mIndexReadBuffer = readIndex;
			readSamples(mFrameBuffers[readIndex]);
		}
		return true;
	}

	void GpuProfiler::endRead()
	{
		mIndexLock.readUnlock();
	}

#endif
}//namespace Render


