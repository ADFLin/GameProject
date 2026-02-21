#include "GpuProfiler.h"

#include <cstdarg>
#include "SystemPlatform.h"
#include "Renderer/RenderThread.h"

namespace Render
{

	GpuProfiler::GpuProfiler()
	{
		mbStartSampling = false;
		mIndexWriteBuffer = 0;
		mIndexReadBuffer = 0;
		for (int i = 0; i < NUM_FRAME_BUFFER; ++i)
		{
			mBufferStatus[i] = EBufferStatus::Free;
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
		if (RenderThread::IsRunning() && !IsInRenderThread())
		{
			RenderThread::AddCommand("GpuProfiler::beginFrame", []()
			{
				GpuProfiler::Get().beginFrame();
			});
			return;
		}

		auto& frameData = getWriteData();
		frameData.numSampleUsed = 0;
		mSampleStack.clear();

		// Update completed results
		for (int i = 0; i < NUM_FRAME_BUFFER; ++i)
		{
			if (mBufferStatus[i] == EBufferStatus::Recorded)
			{
				readSamples(mFrameBuffers[i]);
				if (mFrameBuffers[i].numSampleUsed > 0 && mFrameBuffers[i].samples[0]->time != -1)
				{
					RWLock::WriteLocker locker(mIndexLock);
					mBufferStatus[i] = EBufferStatus::ResultReady;
				}
			}
		}
	
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
					sample->time = (double)time * mCycleToSecond;
					sample->startTime = (double)(start - frameStart) * mCycleToSecond;
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
		if (RenderThread::IsRunning() && !IsInRenderThread())
		{
			RenderThread::AddCommand("GpuProfiler::endFrame", []()
			{
				GpuProfiler::Get().endFrame();
			});
			return;
		}

		if( mCore )
		{
			if( mbStartSampling )
			{
				mbStartSampling = false;
				endSample(*mRootSample);
				mCore->endFrame();

				{
					RWLock::WriteLocker locker(mIndexLock);
					mBufferStatus[mIndexWriteBuffer] = EBufferStatus::Recorded;
					mIndexWriteBuffer = (mIndexWriteBuffer + 1) % NUM_FRAME_BUFFER;
				}
			}
		}
	}

	GpuProfileSample* GpuProfiler::startSample(char const* name)
	{
		if (RenderThread::IsRunning() && !IsInRenderThread())
		{
			std::string nameStr = name;
			RenderThread::AddCommand("GpuProfiler::startSample", [nameStr]()
			{
				GpuProfiler::Get().startSample(nameStr.c_str());
			});
			static GpuProfileSample GDummySample;
			return &GDummySample;
		}

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

		mSampleStack.push_back(sample);

		if ( sample->timingHandle != RHI_ERROR_PROFILE_HANDLE )
			mCore->startTiming(sample->timingHandle);
		return sample;
	}

	void GpuProfiler::endSample(GpuProfileSample& sample)
	{
		if (RenderThread::IsRunning() && !IsInRenderThread())
		{
			RenderThread::AddCommand("GpuProfiler::endSample", []()
			{
				GpuProfiler::Get().endSampleInternal();
			});
			return;
		}

		if( sample.timingHandle != RHI_ERROR_PROFILE_HANDLE )
			mCore->endTiming(sample.timingHandle);
		--mCurLevel;

		if (!mSampleStack.empty())
		{
			mSampleStack.pop_back();
		}
	}

	void GpuProfiler::endSampleInternal()
	{
		if (mSampleStack.empty())
			return;
		
		endSample(*mSampleStack.back());
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

		int checkIndex = (mIndexWriteBuffer + NUM_FRAME_BUFFER - 1) % NUM_FRAME_BUFFER;
		int readIndex = INDEX_NONE;
		for (int i = 0; i < NUM_FRAME_BUFFER - 1; ++i)
		{
			if (mBufferStatus[checkIndex] == EBufferStatus::ResultReady)
			{
				readIndex = checkIndex;
				break;
			}
			checkIndex = (checkIndex + NUM_FRAME_BUFFER - 1) % NUM_FRAME_BUFFER;
		}

		if (readIndex != INDEX_NONE)
		{
			mIndexReadBuffer = readIndex;
			return true;
		}

		mIndexLock.readUnlock();
		return false;
	}

	void GpuProfiler::endRead()
	{
		mIndexLock.readUnlock();
	}

	GpuProfileSample* GpuProfiler::getSample(int idx)
	{
		return getReadData().samples[idx].get();
	}

	int GpuProfiler::getSampleNum() const
	{
		return getReadData().numSampleUsed;
	}

#endif
}//namespace Render


