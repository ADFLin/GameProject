#pragma once
#ifndef GpuProfiler_H_5CF3071A_820F_435C_BC97_1975A2C6D546
#define GpuProfiler_H_5CF3071A_820F_435C_BC97_1975A2C6D546

#include "GL/glew.h"
#include "GLConfig.h"
#include "Singleton.h"

#include <unordered_map>

namespace GL
{
	struct GLGpuTiming
	{
		GLGpuTiming()
		{
			mStartHandle = 0;
			mEndHandle = 0;
		}

		void start();
		void end();
		bool getTime(uint64& result);

		GLuint mStartHandle;
		GLuint mEndHandle;
	};

	struct GpuProfileSample
	{
		std::string name;
		int idxGroup;
		int level;
		GLGpuTiming timing;
		float time;
	};

	struct GpuProfiler : public SingletonT< GpuProfiler >
	{
	public:
		GpuProfiler()
		{
			numSampleUsed = 0;
		}
		void beginFrame()
		{
			numSampleUsed = 0;
			mCurLevel = 0;

		}
		void endFrame()
		{
			for( int i = 0; i < numSampleUsed; ++i )
			{
				GpuProfileSample* sample = mSamples[i];
				uint64 time;
				while( !sample->timing.getTime(time) )
				{


				}
				sample->time = time / 1000000.0;
			}
		}

		GpuProfileSample* startSample(char const* name);

		struct SampleGroup
		{
			std::string name;
			float time;
			int idxParent;
		};

		void endSample(GpuProfileSample* sample);
		std::vector< GpuProfileSample* > mSamples;
		int mCurLevel;
		int numSampleUsed;
	};


	struct GpuProfileScope
	{
		struct NoVA {};
		GpuProfileScope(NoVA, char const* name);
		GpuProfileScope(char const* format, ...);
		~GpuProfileScope();

		GpuProfileSample* sample;
	};

}//namespace RenderGL

#define GPU_PROFILE( name ) GL::GpuProfileScope ANONYMOUS_VARIABLE(GPUProfile)( GL::GpuProfileScope::NoVA() , name );
#define GPU_PROFILE_VA( name , ... ) GL::GpuProfileScope ANONYMOUS_VARIABLE(GPUProfile)( name , __VA_ARGS__);

#endif // GpuProfiler_H_5CF3071A_820F_435C_BC97_1975A2C6D546
