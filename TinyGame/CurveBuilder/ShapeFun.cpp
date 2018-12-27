#include "ShapeFun.h"

#include "FunctionParser.h"


namespace CB
{
	void SurfaceXYFun::evalExpr(Vector3& out, float x, float y)
	{
		assert(isParsed());
		out.setValue(x, y, mExpr.eval(x,y));
	}

	bool SurfaceXYFun::parseExpression(FunctionParser& parser)
	{
		SymbolTable& table = parser.getSymbolDefine();
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

	SurfaceXYFun* SurfaceXYFun::clone()
	{
		return new SurfaceXYFun(*this);
	}

	void SurfaceXYFun::acceptVisit(ShapeFunVisitor& visitor)
	{
		visitor.visit(*this);
	}


	void SurfaceUVFun::evalExpr(Vector3& out, float u, float v)
	{
		assert(isParsed());
		out.setValue(mAixsExpr[0].eval(u,v), mAixsExpr[1].eval(u,v), mAixsExpr[2].eval(u,v));
	}

	bool SurfaceUVFun::parseExpression(FunctionParser& parser)
	{
		SymbolTable& table = parser.getSymbolDefine();
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

	bool SurfaceUVFun::isParsed()
	{
		return mAixsExpr[0].isParsed() && mAixsExpr[1].isParsed() && mAixsExpr[2].isParsed();
	}


	SurfaceUVFun* SurfaceUVFun::clone()
	{
		return new SurfaceUVFun(*this);
	}

	void SurfaceUVFun::acceptVisit(ShapeFunVisitor& visitor)
	{
		visitor.visit(*this);
	}


	Curve3DFun* Curve3DFun::clone()
	{
		return new Curve3DFun(*this);
	}

	void Curve3DFun::acceptVisit(ShapeFunVisitor& visitor)
	{
		visitor.visit(*this);
	}

	bool Curve3DFun::parseExpression(FunctionParser& parser)
	{
		bool bDynamic = false;
		SymbolTable& table = parser.getSymbolDefine();

		ValueLayout inputLayouts[] = { ValueLayout::Real };
		for( int i = 0; i < 3; ++i )
		{
			if( parser.parse(mCoordExpr[i] , ARRAY_SIZE(inputLayouts) , inputLayouts ) )
				bDynamic |= parser.isUsingVar("t");
		}

		setDynamic(bDynamic);
		return isParsed();
	}

	bool Curve3DFun::isParsed()
	{
		return mCoordExpr[0].isParsed() && mCoordExpr[1].isParsed() && mCoordExpr[2].isParsed();
	}

	void Curve3DFun::evalExpr(Vector3& out, float s)
	{
		assert(isParsed());
		out.setValue(mCoordExpr[0].eval(s), mCoordExpr[1].eval(s), mCoordExpr[2].eval(s));
	}

}//namespace CB