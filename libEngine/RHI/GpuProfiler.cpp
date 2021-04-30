#include "GpuProfiler.h"

#include <cstdarg>
#include "SystemPlatform.h"

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

	void GpuProfiler::releaseRHIResource()
	{
		mSamples.clear();

		if (mCore)
		{
			delete mCore;
			mCore = nullptr;
		}
	}

	void GpuProfiler::beginFrame()
	{
		mNumSampleUsed = 0;
		mCurLevel = 0;


		auto GetSystemNameString = [](RHISystemName name)
		{
			switch (name)
			{
			case RHISystemName::OpenGL: return "OpenGL";
			case RHISystemName::D3D11:  return "D3D11";
			case RHISystemName::D3D12: return "D3D12";
			case RHISystemName::Vulkan: return "Vulkan";
			}
			return "Unknown";
		};

		if( mCore )
		{
			mCore->beginFrame();
			mbStartSampling = true;
			InlineString< 512 > str;
			str.format("Frame : %s System", GetSystemNameString(GRHISystem->getName()));
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
					for( int i = 0; i < mNumSampleUsed; ++i )
					{
						GpuProfileSample* sample = mSamples[i].get();

						if( sample->timingHandle != RHI_ERROR_PROFILE_HANDLE )
						{
							uint64 time;
							while( !mCore->getTimingDuration(sample->timingHandle, time) )
							{
								SystemPlatform::Sleep(0);
							}
							sample->time = double(time) * mCycleToSecond;
						}
						else
						{
							sample->time = -1;
						}
					}
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
			mSamples.clear();
		}
	}

}//namespace Render


