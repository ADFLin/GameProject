#pragma once
#ifndef GpuProfiler_H_5CF3071A_820F_435C_BC97_1975A2C6D546
#define GpuProfiler_H_5CF3071A_820F_435C_BC97_1975A2C6D546

#include "Singleton.h"
#include "Core/IntegerType.h"

#include <unordered_map>
#include <memory>

namespace Render
{

	class RHIProfileCore
	{
	public:
		virtual ~RHIProfileCore(){}

		virtual void beginFrame() = 0;
		virtual void endFrame() = 0;
		virtual uint32 fetchTiming() = 0;

		virtual void startTiming(uint32 timingHandle) = 0;
		virtual void endTiming(uint32 timingHandle) = 0;
		virtual bool getTimingDurtion(uint32 timingHandle, uint64& outDurtion) = 0;
		virtual double getCycleToSecond() = 0;
	};

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

		void beginFrame();
		void endFrame();

		int  getSampleNum() { return numSampleUsed; }
		void setCore(RHIProfileCore* core);

		GpuProfileSample* getSample(int idx) { return mSamples[idx].get(); }
		GpuProfileSample* startSample(char const* name);
		void endSample(GpuProfileSample& sample);

		struct SampleGroup
		{
			std::string name;
			float time;
			int idxParent;
		};
		
		std::vector< std::unique_ptr< GpuProfileSample > > mSamples;

		RHIProfileCore* mCore = nullptr;
		bool bStartSampling = false;
		int  mCurLevel;
		int  numSampleUsed;
		double cycleToSecond;
	};


	struct GpuProfileScope
	{
		struct NoVA {};
		GpuProfileScope(NoVA, char const* name);
		GpuProfileScope(char const* format, ...);
		~GpuProfileScope();

		GpuProfileSample* sample;
	};

}//namespace Render

#define GPU_PROFILE( name ) Render::GpuProfileScope ANONYMOUS_VARIABLE(GPUProfile)( Render::GpuProfileScope::NoVA() , name );
#define GPU_PROFILE_VA( name , ... ) Render::GpuProfileScope ANONYMOUS_VARIABLE(GPUProfile)( name , __VA_ARGS__);

#endif // GpuProfiler_H_5CF3071A_820F_435C_BC97_1975A2C6D546
