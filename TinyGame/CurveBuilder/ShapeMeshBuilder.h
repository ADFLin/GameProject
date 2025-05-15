#pragma once
#ifndef ShapeMeshBuilder_H_BCBBC6D7_771D_4995_9E5E_3122758EDA04
#define ShapeMeshBuilder_H_BCBBC6D7_771D_4995_9E5E_3122758EDA04

#include "ShapeCommon.h"
#include "FunctionParser.h"
#include "ColorMap.h"

#include "PlatformThread.h"

#include "RHI/ShaderCore.h"
#include "RHI/RHICommand.h"

#include <vector>
#include <exception>
#include <functional>




#define USE_PARALLEL_UPDATE 0

namespace CB
{
	class ShapeFuncBase;

	struct SampleParam;
	struct ShapeUpdateContext;
	class  RenderData;
	class  ShapeBase;

	class RealNanException : public std::exception
	{

	};



	struct GenParamsData
	{
		DECLARE_UNIFORM_BUFFER_STRUCT(GenParamsDataBlock);

		Vector2 delata;
		Vector2 offset;
		uint    gridCountU;
		uint    vertexCount;
		uint    vertexSize;
		uint    posOffset;
		float   time;
		Vector3 dummy;
	};


	class ShapeMeshBuilder : public IShapeMeshBuilder
	{
	public:
		ShapeMeshBuilder();

		void  updateCurveData(ShapeUpdateContext const& context, SampleParam const& paramS) override;
		void  updateSurfaceData(ShapeUpdateContext const& context, SampleParam const& paramU, SampleParam const& paramV) override;




		bool  parseFunction(ShapeFuncBase& func) override;
		void  setTime(float t) { mVarTime = t; }

		void initializeRHI();

		void releaseRHI();

	private:
		void  setColor(float p, float* color);

		void  updateSurfaceDataGPU(ShapeUpdateContext const& context, SampleParam const& paramU, SampleParam const& paramV);

		template< typename TSurfaceUVFunc >
		void updatePositionData_SurfaceUV(TSurfaceUVFunc* func, SampleParam const &paramU, SampleParam const &paramV, RenderData& data);
		template< typename TSurfaceXYFunc >
		void updatePositionData_SurfaceXY(TSurfaceXYFunc* func, SampleParam const &paramU, SampleParam const &paramV, RenderData& data);


		Render::TStructuredBuffer<GenParamsData> mGenParamBuffer;



		RealType        mVarTime;
		ColorMap        mColorMap;

#if USE_PARALLEL_UPDATE
		Mutex            mParserLock;
#endif
		FunctionParser   mParser;

	};

}//namespace CB


#endif // ShapeMeshBuilder_H_BCBBC6D7_771D_4995_9E5E_3122758EDA04
