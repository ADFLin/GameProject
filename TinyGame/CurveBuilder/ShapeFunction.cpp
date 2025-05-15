#include "ShapeFunction.h"

#include "FunctionParser.h"


namespace CB
{

	bool SurfaceXYFunc::parseExpression(FunctionParser& parser)
	{
		ValueLayout inputLayouts[] = { ValueLayout::Real , ValueLayout::Real };
		if( parser.parse(mExpr , ARRAY_SIZE(inputLayouts) , inputLayouts ) )
		{
			mUsedInputMask = 0;
			mUsedInputMask |= parser.isUsingInput("x") ? BIT(0) : 0;
			mUsedInputMask |= parser.isUsingInput("y") ? BIT(1) : 0;
			setDynamic(parser.isUsingVar("t"));
		}
		return isParsed();
	}

	SurfaceXYFunc* SurfaceXYFunc::clone()
	{
		return new SurfaceXYFunc(*this);
	}

	bool GPUSurfaceXYFunc::parseExpression(FunctionParser& parser)
	{
		ValueLayout inputLayouts[] = { ValueLayout::Real , ValueLayout::Real };
		ParseResult parserResult;
		if (parser.parse(mExpr.c_str(), ARRAY_SIZE(inputLayouts), inputLayouts, parserResult))
		{
			mUsedInputMask = 0;
			mUsedInputMask |= parserResult.isUsingInput("x") ? BIT(0) : 0;
			mUsedInputMask |= parserResult.isUsingInput("y") ? BIT(1) : 0;
			setDynamic(parserResult.isUsingVar("t"));
			return true;
		}

		return false;
	}

	CB::GPUSurfaceXYFunc* GPUSurfaceXYFunc::clone()
	{
		return new GPUSurfaceXYFunc(*this);
	}

	void SurfaceUVFunc::evalExpr(Vector3& out, float u, float v)
	{
		assert(isParsed());
		out.setValue(mAixsExpr[0].eval(u,v), mAixsExpr[1].eval(u,v), mAixsExpr[2].eval(u,v));
	}

	bool SurfaceUVFunc::parseExpression(FunctionParser& parser)
	{
		bool bDynamic = false;

		ValueLayout inputLayouts[] = { ValueLayout::Real , ValueLayout::Real };	
		for( int i = 0; i < 3; ++i )
		{
			if( !parser.parse(mAixsExpr[i], ARRAY_SIZE(inputLayouts), inputLayouts) )
				return false;

			bDynamic |= parser.isUsingVar("t");
		}

		mUsedInputMask = 0;
		mUsedInputMask |= parser.isUsingInput("u") ? BIT(0) : 0;
		mUsedInputMask |= parser.isUsingInput("v") ? BIT(1) : 0;
		
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
		for( int i = 0; i < 3; ++i )
		{
			if( parser.parse(mCoordExpr[i] , ARRAY_SIZE(inputLayouts) , inputLayouts ) )
				bDynamic |= parser.isUsingVar("t");
		}

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
		out.setValue(mCoordExpr[0].eval(s), mCoordExpr[1].eval(s), mCoordExpr[2].eval(s));
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