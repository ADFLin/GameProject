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
		out.setValue(mExpr[0].eval(u,v), mExpr[1].eval(u,v), mExpr[2].eval(u,v));
	}

	bool SurfaceUVFun::parseExpression(FunctionParser& parser)
	{
		SymbolTable& table = parser.getSymbolDefine();
		bool beDynamic = false;

		ValueLayout inputLayouts[] = { ValueLayout::Real , ValueLayout::Real };
		
		for( int i = 0; i < 3; ++i )
		{
			if( parser.parse(mExpr[i], ARRAY_SIZE(inputLayouts), inputLayouts) )
				beDynamic |= parser.isUsingVar("t");
		}

		setDynamic(beDynamic);
		return isParsed();
	}

	bool SurfaceUVFun::isParsed()
	{
		return mExpr[0].isParsed() && mExpr[1].isParsed() && mExpr[2].isParsed();
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
		bool beDynamic = false;
		SymbolTable& table = parser.getSymbolDefine();

		ValueLayout inputLayouts[] = { ValueLayout::Real };
		for( int i = 0; i < 3; ++i )
		{
			if( parser.parse(mExpr[i] , ARRAY_SIZE(inputLayouts) , inputLayouts ) )
				beDynamic |= parser.isUsingVar("t");
		}

		setDynamic(beDynamic);
		return isParsed();
	}

	bool Curve3DFun::isParsed()
	{
		return mExpr[0].isParsed() && mExpr[1].isParsed() && mExpr[2].isParsed();
	}

	void Curve3DFun::evalExpr(Vector3& out, float s)
	{
		assert(isParsed());
		out.setValue(mExpr[0].eval(s), mExpr[1].eval(s), mExpr[2].eval(s));
	}

}//namespace CB