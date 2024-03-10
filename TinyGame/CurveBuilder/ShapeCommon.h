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

	class ShapeFuncVisitor;

	class ShapeFuncBase
	{
	public:
		virtual ~ShapeFuncBase() {}
		virtual bool   parseExpression(FunctionParser& parser) = 0;
		virtual bool   isParsed() = 0;
		virtual int    getFuncType() = 0;
		virtual void   acceptVisit(ShapeFuncVisitor& visitor) = 0;
		bool    isDynamic() { return mbDynamic; }
		virtual ShapeFuncBase* clone() = 0;
		uint8   getUsedInputMask() const { return mUsedInputMask; }

	protected:
		uint8   mUsedInputMask;
		void    setDynamic(bool bDynamic) { mbDynamic = bDynamic; }
	private:
		bool    mbDynamic;
	};

	enum RenderUpdateFlags
	{
		RUF_FUNCTION = 1 << 0,
		RUF_DATA_SAMPLE = 1 << 1,
		RUF_GEOM = 1 << 2,
		RUF_COLOR = 1 << 3,
		RUF_ALL_UPDATE_BIT = 0xffffffff,
	};

	struct ShapeUpdateContext
	{
		RenderData*    data;
		unsigned       flags;
		ShapeFuncBase* func;
		Color4f        color;
	};

	class IShapeMeshBuilder
	{
	public:
		virtual ~IShapeMeshBuilder() = default;
		virtual void updateCurveData(ShapeUpdateContext const& context, SampleParam const& paramS) = 0;
		virtual void updateSurfaceData(ShapeUpdateContext const& context, SampleParam const& paramU, SampleParam const& paramV) = 0;
		virtual bool parseFunction(ShapeFuncBase& func) = 0;
	};

}//namespace CB


#endif // ShapeCommon_H_24FD13AA_4CED_4EB8_AA01_B486F4798671
