#pragma once
#ifndef ShapeCommon_H_24FD13AA_4CED_4EB8_AA01_B486F4798671
#define ShapeCommon_H_24FD13AA_4CED_4EB8_AA01_B486F4798671

#include "Base.h"
#include "Color.h"

namespace CB
{
	class RenderData;
	class FunctionParser;

	enum ShapeType
	{
		TYPE_SURFACE = 0,
		TYPE_SURFACE_XY,
		TYPE_SURFACE_UV,

		TYPE_CURVE_2D,
		TYPE_CURVE_3D,
	};

	inline bool isSurface(int type)
	{
		return type == TYPE_SURFACE ||
			type == TYPE_SURFACE_XY ||
			type == TYPE_SURFACE_UV;
	}

	struct Range
	{
		Range(float min = 0.0f, float max = 0.0f)
			:Min(min), Max(max)
		{
		}

		float Min;
		float Max;
	};

	struct SampleParam
	{
		SampleParam() :range(), numData(0) {}
		SampleParam(Range const& range, int num)
			:range(range), numData(num)
		{
		}

		Range range;
		int   numData;

		int   getNumData() const { return numData; }
		float getRangeMax() const { return range.Max; }
		float getRangeMin() const { return range.Min; }
		void  setIncrement(float increment) { numData = int((range.Max - range.Min) / increment); }
		float getIncrement() const { return (range.Max - range.Min) / numData; }
	};

	class ShapeFunVisitor;

	class ShapeFunBase
	{
	public:
		virtual ~ShapeFunBase() {}
		virtual bool   parseExpression(FunctionParser& parser) = 0;
		virtual bool   isParsed() = 0;
		virtual int    getFunType() = 0;
		virtual void   acceptVisit(ShapeFunVisitor& visitor) = 0;
		bool    isDynamic() { return mbDynamic; }
		virtual ShapeFunBase* clone() = 0;

	protected:
		void    setDynamic(bool bDynamic) { mbDynamic = bDynamic; }
	private:
		bool   mbDynamic;
	};

	enum RenderUpdateFlag
	{
		RUF_FUNCTION = 1 << 0,
		RUF_DATA_SAMPLE = 1 << 1,
		RUF_GEOM = 1 << 2,
		RUF_COLOR = 1 << 3,
		RUB_ALL_UPDATE_BIT = 0xffffffff,
	};

	struct ShapeUpdateInfo
	{
		RenderData*   data;
		unsigned      flag;
		ShapeFunBase* fun;
		Color4f       color;
	};

}//namespace CB


#endif // ShapeCommon_H_24FD13AA_4CED_4EB8_AA01_B486F4798671
