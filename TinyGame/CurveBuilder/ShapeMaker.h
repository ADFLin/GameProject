#ifndef axShapeMaker_H
#define axShapeMaker_H

#include "Base.h"
#include "Color.h"
#include "FunctionParser.h"
#include "ColorMap.h"

#include <vector>
#include <exception>

namespace CB
{
	class ShapeFunBase;

	struct SampleParam;
	struct ShapeUpdateInfo;
	class  RenderData;

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
		FunctionParser& getParser() { return mParser; }

	private:
		void  setColor(float p, float* color);

		std::vector< Vector3 > mCacheNormal;
		std::vector< int >     mCacheCount;

		ValueType        mVarTime;
		ColorMap         mColorMap;
		FunctionParser   mParser;

	};

}//namespace CB


#endif //axShapeMaker_H
