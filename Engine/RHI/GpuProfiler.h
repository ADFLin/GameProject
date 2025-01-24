#pragma once
#ifndef GpuProfiler_H_5CF3071A_820F_435C_BC97_1975A2C6D546
#define GpuProfiler_H_5CF3071A_820F_435C_BC97_1975A2C6D546

#include "Singleton.h"
#include "Core/IntegerType.h"
#include "InlineString.h"
#include "DataStructure/Array.h"
#include "PlatformThread.h"

#include <memory>
#include <string>


namespace Render
{

	class RHIProfileCore
	{
	public:
		virtual ~RHIProfileCore(){}

		virtual void releaseRHI() = 0;

		virtual void beginFrame() = 0;
		virtual bool endFrame() = 0;
		virtual uint32 fetchTiming() = 0;

		virtual void startTiming(uint32 timingHandle) = 0;
		virtual void endTiming(uint32 timingHandle) = 0;
		virtual void beginReadback() {}
		virtual bool getTimingDuration(uint32 timingHandle, uint64& outDuration) = 0;
		virtual void endReadback() {}
		virtual double getCycleToMillisecond() = 0;
	};

#define RHI_ERROR_PROFILE_HANDLE uint32(-1)

	struct GpuProfileSample
	{
		std::string name;
		int idxGroup;
		int level;

		uint32 timingHandle;
		float time;
	};

	struct GpuProfiler
	{
	public:
		GpuProfiler();

		static CORE_API GpuProfiler& Get();

		CORE_API void beginFrame();
		CORE_API void endFrame();

		CORE_API GpuProfileSample* startSample(char const* name);
		CORE_API void endSample(GpuProfileSample& sample);


		CORE_API void releaseRHIResource();
		CORE_API void setCore(RHIProfileCore* core);

		CORE_API bool beginRead();
		CORE_API void endRead();

		GpuProfileSample* getSample(int idx) { return getReadData().samples[idx].get(); }
		int  getSampleNum() const { return getReadData().numSampleUsed; }

	private:
		struct FrameData
		{
			TArray< std::unique_ptr< GpuProfileSample > > samples;
			int    numSampleUsed = 0;

			void clear()
			{
				samples.clear();
				numSampleUsed = 0;
			}
		};
		FrameData const& getReadData() const { return mFrameBuffers[1 - mIndexWriteBuffer]; }
		FrameData& getWriteData(){ return mFrameBuffers[mIndexWriteBuffer]; }
		void readSamples(FrameData& frameData);

		FrameData mFrameBuffers[2];
		int       mIndexWriteBuffer;
		RWLock    mIndexLock;

		struct SampleGroup
		{
			std::string name;
			float time;
			int idxParent;
		};

		RHIProfileCore* mCore = nullptr;
		GpuProfileSample* mRootSample;
		bool   mbStartSampling = false;
		bool   mbCanRead = false;
		int    mCurLevel;
		int    mNumSampleUsed;
		double mCycleToSecond;
	};

	struct GpuProfileReadScope
	{
		GpuProfileReadScope()
		{
			GpuProfiler::Get().beginRead();
		}

		~GpuProfileReadScope()
		{
			GpuProfiler::Get().endRead();
		}
	};

	struct GpuProfileScope
	{
		GpuProfileScope(char const* name)
		{
			sample = GpuProfiler::Get().startSample(name);
		}

		template< class ...Args>
		GpuProfileScope(char const* format, Args&& ...args)
		{
			InlineString< 256 > name;
			name.format(format, std::forward< Args >(args)...);
			sample = GpuProfiler::Get().startSample(name);
		}

		~GpuProfileScope()
		{
			if (sample)
			{
				GpuProfiler::Get().endSample(*sample);
			}
		}

		GpuProfileSample* sample;
	};

}//namespace Render

#define GPU_PROFILE( name , ... ) Render::GpuProfileScope ANONYMOUS_VARIABLE(GPUProfile)( name , ##__VA_ARGS__);

#endif // GpuProfiler_H_5CF3071A_820F_435C_BC97_1975A2C6D546
