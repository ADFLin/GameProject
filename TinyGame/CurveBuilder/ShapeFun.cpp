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
		DefineTable& table = parser.getDefineTable();
		if( parser.parse(mExpr , 2) )
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
		DefineTable& table = parser.getDefineTable();
		bool beDynamic = false;

		for( int i = 0; i < 3; ++i )
		{
			if( parser.parse(mExpr[i] , 2) )
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
		DefineTable& table = parser.getDefineTable();

		for( int i = 0; i < 3; ++i )
		{
			if( parser.parse(mExpr[i] , 1) )
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