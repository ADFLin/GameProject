#include "ShapeFunction.h"

#include "FunctionParser.h"


namespace CB
{

	bool SurfaceXYFunc::parseExpression(FunctionParser& parser)
	{
		ValueLayout inputLayouts[] = { ValueLayout::Real , ValueLayout::Real };


		ParseResult parserResult;
		if (mbUseGPU)
		{
			if (parser.parse(mExpr, ARRAY_SIZE(inputLayouts), inputLayouts, parserResult))
			{
				mUsedInputMask = 0;
				mUsedInputMask |= parserResult.isUsingInput("x") ? BIT(0) : 0;
				mUsedInputMask |= parserResult.isUsingInput("y") ? BIT(1) : 0;
				setDynamic(parserResult.isUsingVar("t"));
			}
		}
		else
		{
			if (parser.compile(mExpr, ARRAY_SIZE(inputLayouts), inputLayouts, parserResult))
			{
				mUsedInputMask = 0;
				mUsedInputMask |= parserResult.isUsingInput("x") ? BIT(0) : 0;
				mUsedInputMask |= parserResult.isUsingInput("y") ? BIT(1) : 0;
				setDynamic(parserResult.isUsingVar("t"));
				bSupportSIMD = mExpr.getEvalResource<ExecutableCode>().isSupportSIMD();
			}
		}
		return isParsed();
	}

	SurfaceXYFunc* SurfaceXYFunc::clone()
	{
		return new SurfaceXYFunc(*this);
	}

	void SurfaceUVFunc::evalExpr(float u, float v, Vector3& out)
	{
		CHECK(isParsed());
		out.setValue(
			mAixsExpr[0].getEvalResource<ExecutableCode>().evalT<RealType>(u,v),
			mAixsExpr[1].getEvalResource<ExecutableCode>().evalT<RealType>(u,v), 
			mAixsExpr[2].getEvalResource<ExecutableCode>().evalT<RealType>(u,v)
		);
	}

	void SurfaceUVFunc::evalExpr(FloatVector const& u, FloatVector const& v, FloatVector& outX, FloatVector& outY, FloatVector& outZ)
	{
		CHECK(isParsed());
		outX = mAixsExpr[0].getEvalResource<ExecutableCode>().evalT<FloatVector>(u, v);
		outY = mAixsExpr[0].getEvalResource<ExecutableCode>().evalT<FloatVector>(u, v);
		outZ = mAixsExpr[0].getEvalResource<ExecutableCode>().evalT<FloatVector>(u, v);
	}

	bool SurfaceUVFunc::parseExpression(FunctionParser& parser)
	{
		bool bDynamic = false;
		mUsedInputMask = 0;
		ValueLayout inputLayouts[] = { ValueLayout::Real , ValueLayout::Real };
		ParseResult parseResult;
		if (mbUseGPU)
		{
			for (int i = 0; i < 3; ++i)
			{
				if (!parser.parse(mAixsExpr[i], ARRAY_SIZE(inputLayouts), inputLayouts, parseResult))
					return false;

				bDynamic |= parseResult.isUsingVar("t");
				mUsedInputMask |= parseResult.isUsingInput("u") ? BIT(0) : 0;
				mUsedInputMask |= parseResult.isUsingInput("v") ? BIT(1) : 0;
			}
		}
		else
		{
			for (int i = 0; i < 3; ++i)
			{
				if (!parser.compile(mAixsExpr[i], ARRAY_SIZE(inputLayouts), inputLayouts, parseResult))
					return false;

				bDynamic |= parseResult.isUsingVar("t");
				mUsedInputMask |= parseResult.isUsingInput("u") ? BIT(0) : 0;
				mUsedInputMask |= parseResult.isUsingInput("v") ? BIT(1) : 0;
			}
			bSupportSIMD = mAixsExpr[0].getEvalResource<ExecutableCode>().isSupportSIMD();
		}


		setDynamic(bDynamic);
		return isParsed();
	}

	bool SurfaceUVFunc::isParsed()
	{
		return mAixsExpr[0].isParsed() && mAixsExpr[1].isParsed() && mAixsExpr[2].isParsed();
	}


	SurfaceUVFunc* SurfaceUVFunc::clone()
	{
		return new SurfaceUVFunc(*this);
	}

	Curve3DFunc* Curve3DFunc::clone()
	{
		return new Curve3DFunc(*this);
	}

	bool Curve3DFunc::parseExpression(FunctionParser& parser)
	{
		bool bDynamic = false;
		ValueLayout inputLayouts[] = { ValueLayout::Real };
		ParseResult parseResult;
		for( int i = 0; i < 3; ++i )
		{
			if( parser.compile(mCoordExpr[i] , ARRAY_SIZE(inputLayouts) , inputLayouts, parseResult) )
				bDynamic |= parseResult.isUsingVar("t");
		}
		bSupportSIMD = mCoordExpr[0].getEvalResource<ExecutableCode>().isSupportSIMD();

		setDynamic(bDynamic);
		return isParsed();
	}

	bool Curve3DFunc::isParsed()
	{
		return mCoordExpr[0].isParsed() && mCoordExpr[1].isParsed() && mCoordExpr[2].isParsed();
	}

	void Curve3DFunc::evalExpr(Vector3& out, float s)
	{
		assert(isParsed());
		out.setValue(
			mCoordExpr[0].getEvalResource<ExecutableCode>().evalT<RealType>(s),
			mCoordExpr[1].getEvalResource<ExecutableCode>().evalT<RealType>(s),
			mCoordExpr[2].getEvalResource<ExecutableCode>().evalT<RealType>(s)
		);
	}

	NativeSurfaceXYFunc* NativeSurfaceXYFunc::clone()
	{
		return new NativeSurfaceXYFunc(*this);
	}

	NativeSurfaceUVFunc* NativeSurfaceUVFunc::clone()
	{
		return new NativeSurfaceUVFunc(*this);
	}



}//namespace CB