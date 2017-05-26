#include "GpuProfiler.h"

#include "FixString.h"

namespace RenderGL
{

	void GLGpuTiming::start()
	{
		if( mStartHandle != 0 )
		{
			glDeleteQueries(1, &mStartHandle);
			mStartHandle = 0;
		}
		glGenQueries(1, &mStartHandle);
		glQueryCounter(mStartHandle, GL_TIMESTAMP);
	}

	void GLGpuTiming::end()
	{
		if( mEndHandle != 0 )
		{
			glDeleteQueries(1, &mEndHandle);
			mEndHandle = 0;
		}
		glGenQueries(1, &mEndHandle);
		glQueryCounter(mEndHandle, GL_TIMESTAMP);
	}

	bool GLGpuTiming::getTime(uint64& result)
	{
		GLuint isAvailable = GL_TRUE;
		glGetQueryObjectuiv(mStartHandle, GL_QUERY_RESULT_AVAILABLE, &isAvailable);
		if( isAvailable == GL_TRUE )
		{
			glGetQueryObjectuiv(mEndHandle, GL_QUERY_RESULT_AVAILABLE, &isAvailable);

			if( isAvailable == GL_TRUE )
			{
				GLuint64 startTimeStamp;
				glGetQueryObjectui64v(mStartHandle, GL_QUERY_RESULT, &startTimeStamp);
				GLuint64 endTimeStamp;
				glGetQueryObjectui64v(mEndHandle, GL_QUERY_RESULT, &endTimeStamp);

				result = endTimeStamp - startTimeStamp;
				return true;
			}
		}
		return false;
	}

	GpuProfiler::GpuProfiler()
	{
		numSampleUsed = 0;
	}

	void GpuProfiler::beginFrame()
	{
		numSampleUsed = 0;
		mCurLevel = 0;
	}

	void GpuProfiler::endFrame()
	{
		for( int i = 0; i < numSampleUsed; ++i )
		{
			GpuProfileSample* sample = mSamples[i].get();
			uint64 time;
			while( !sample->timing.getTime(time) )
			{


			}
			sample->time = time / 1000000.0;
		}
	}

	GpuProfileSample* GpuProfiler::startSample(char const* name)
	{
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
		++mCurLevel;
		++numSampleUsed;
		sample->timing.start();
		return sample;
	}

	void GpuProfiler::endSample(GpuProfileSample* sample)
	{
		sample->timing.end();
		--mCurLevel;
	}

	GpuProfileScope::GpuProfileScope(NoVA, char const* name)
	{
		sample = GpuProfiler::getInstance().startSample(name);
	}

	GpuProfileScope::GpuProfileScope(char const* format, ...)
	{
		FixString< 256 > name;
		va_list argptr;
		va_start(argptr, format);
		name.formatV(format, argptr);
		va_end(argptr);
		sample = GpuProfiler::getInstance().startSample(name);
	}

	GpuProfileScope::~GpuProfileScope()
	{
		GpuProfiler::getInstance().endSample(sample);
	}

}//namespace RenderGL


