#ifndef axShapeMaker_H
#define axShapeMaker_H

#include "Base.h"
#include "Color.h"
#include "FunctionParser.h"
#include "ColorMap.h"

#include "Thread.h"

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

	class ShapeMeshBuilder
	{
	public:
		ShapeMeshBuilder();

		void            updateCurveData(ShapeUpdateInfo const& info, SampleParam const& paramS);
		void            updateSurfaceData(ShapeUpdateInfo const& info, SampleParam const& paramU, SampleParam const& paramV);
		void            setTime(float t) { mVarTime = t; }
		bool            parseFunction(ShapeFunBase& func);

	private:
		void  setColor(float p, float* color);

		RealType        mVarTime;
		ColorMap        mColorMap;

#if USE_PARALLEL_UPDATE
		Mutex            mParserLock;
#else
		std::vector< Vector3 > mCacheNormal;
		std::vector< int >     mCacheCount;
#endif
		FunctionParser   mParser;

	};

}//namespace CB


#endif //axShapeMaker_H
