#ifndef axShapeMaker_H
#define axShapeMaker_H

#include "Base.h"
#include "Color.h"
#include "FunctionParser.h"
#include "ColorMap.h"

#include "Thread.h"
#include "AsyncWork.h"

#include <vector>
#include <exception>
#include <functional>

#define USE_PARALLEL_UPDATE 1

namespace CB
{
	class ShapeFunBase;

	struct SampleParam;
	struct ShapeUpdateInfo;
	class  RenderData;
	class  ShapeBase;

	class RealNanException : public std::exception
	{

	};

	class ShapeMaker
	{
	public:
		ShapeMaker();

		void            updateCurveData(ShapeUpdateInfo const& info, SampleParam const& paramS);
		void            updateSurfaceData(ShapeUpdateInfo const& info, SampleParam const& paramU, SampleParam const& paramV);
		void            setTime(float t) { mVarTime = t; }
#if USE_PARALLEL_UPDATE
		auto            lockParser() { return MakeLockedObjectHandle(mParser, &mParserLock); }
		void            addUpdateWork( std::function<void()> fun );
		void            waitUpdateDone();
#else
		FunctionParser& getParser() { return mParser; }
#endif
	private:
		void  setColor(float p, float* color);

		ValueType        mVarTime;
		ColorMap         mColorMap;

#if USE_PARALLEL_UPDATE
		std::unique_ptr< QueueThreadPool > mUpdateThreadPool;
		Mutex            mParserLock;
#endif
		FunctionParser   mParser;

	};

}//namespace CB


#endif //axShapeMaker_H
