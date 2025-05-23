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

	struct VertexGenParamsData
	{
		DECLARE_UNIFORM_BUFFER_STRUCT(VertexGenParamsDataBlock);

		Vector2 delata;
		Vector2 offset;
		uint32  gridCountU;
		uint32  vertexCount;
		uint32  vertexSize;
		uint32  posOffset;
		Color4f color;
		float   time;
		Vector3 dummy;
		
	};

	struct NormalGenParamsData
	{
		DECLARE_UNIFORM_BUFFER_STRUCT(NormalGenParamsDataBlock);
		uint32   totalCount;
		uint32   vertexSize;
		uint32   posOffset;
		uint32   normalOffset;
	};

	class ShapeMeshBuilder : public IShapeMeshBuilder
	{
	public:
		ShapeMeshBuilder();

		void  updateCurveData(ShapeUpdateContext const& context, SampleParam const& paramS) override;
		void  updateSurfaceData(ShapeUpdateContext const& context, SampleParam const& paramU, SampleParam const& paramV) override;

		bool  parseFunction(ShapeFuncBase& func) override;

		void  bindTime(RealType& time);

		void initializeRHI();

		void releaseRHI();

		SymbolTable& getSymbolDefine(){ return mParser.getSymbolDefine(); }

	private:
		void  setColor(float p, float* color);

		void  updateSurfaceDataGPU(ShapeUpdateContext const& context, SampleParam const& paramU, SampleParam const& paramV);

		bool  compileFuncShader(ShapeFuncBase& func);
		template< typename TSurfaceUVFunc >
		void updatePositionData_SurfaceUV(TSurfaceUVFunc* func, SampleParam const &paramU, SampleParam const &paramV, RenderData& data);
		template< typename TSurfaceXYFunc >
		void updatePositionData_SurfaceXY(TSurfaceXYFunc* func, SampleParam const &paramU, SampleParam const &paramV, RenderData& data);


		Render::TStructuredBuffer<VertexGenParamsData> mVertexGenParamBuffer;
		Render::TStructuredBuffer<NormalGenParamsData> mNoramlGenParamBuffer;

		class GenNormalCS* mShaderGenNormal;

		ColorMap         mColorMap;
		RealType*        mTimePtr;

#if USE_PARALLEL_UPDATE
		Mutex            mParserLock;
#endif
		FunctionParser   mParser;

	};

}//namespace CB


#endif // ShapeMeshBuilder_H_BCBBC6D7_771D_4995_9E5E_3122758EDA04
