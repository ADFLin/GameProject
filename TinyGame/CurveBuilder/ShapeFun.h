#pragma once
#ifndef ShapeFun_H_883010A1_0C8F_40DA_A88C_32FBDE06AF1D
#define ShapeFun_H_883010A1_0C8F_40DA_A88C_32FBDE06AF1D

#include "ShapeCommon.h"
#include "Expression.h"

#define USE_VALUE_INPUT 1

namespace CB
{

	class Curve3DFun : public ShapeFunBase
	{
	public:
		Curve3DFun() :ShapeFunBase() {}

		int  getFunType() { return TYPE_CURVE_3D; }
		bool parseExpression(FunctionParser& parser);
		bool isParsed();
		void evalExpr(Vector3& out, float s);
		void setExpr(int axis, string const& expr)
		{
			assert(axis >= 0 && axis < 3);
			mCoordExpr[axis].setExprString(expr);
		}
		string const& getExprString(int axis)
		{
			assert(axis >= 0 && axis < 3);
			return mCoordExpr[axis].getExprString();
		}

		virtual Curve3DFun* clone();
		virtual void acceptVisit(ShapeFunVisitor& visitor);
	private:

		Expression   mCoordExpr[3];
	};

	class SurfaceFun : public ShapeFunBase
	{

	};

	class SurfaceXYFun : public SurfaceFun
	{
	public:
		SurfaceXYFun():SurfaceFun() {}
		virtual ~SurfaceXYFun() {}

		int  getFunType() { return TYPE_SURFACE_XY; }
		bool parseExpression(FunctionParser& parser);
		bool isParsed() { return mExpr.isParsed(); }
		void evalExpr(Vector3& out, float x, float y);
		void setExpr(string const& expr) { mExpr.setExprString(expr); }
		string const& getExprString() { return mExpr.getExprString(); }

		virtual void acceptVisit(ShapeFunVisitor& visitor);
		virtual SurfaceXYFun* clone();
	private:
		Expression   mExpr;
	};

	class SurfaceUVFun : public SurfaceFun
	{
	public:
		SurfaceUVFun() :SurfaceFun() {}

		int  getFunType() { return TYPE_SURFACE_UV; }
		bool parseExpression(FunctionParser& parser);
		bool isParsed();
		void evalExpr(Vector3& out, float u, float v);
		void setExpr(int axis, string const& expr)
		{
			assert(axis >= 0 && axis < 3);
			mAixsExpr[axis].setExprString(expr);
		}
		string const& getExprString(int axis)
		{
			assert(axis >= 0 && axis < 3);
			return mAixsExpr[axis].getExprString();
		}
		virtual SurfaceUVFun* clone();
		virtual void acceptVisit(ShapeFunVisitor& visitor);
	private:
		Expression   mAixsExpr[3];

	};

	class SFImplicitFun : public SurfaceFun
	{
	private:
		Expression mExpr;
	};

	class ShapeFunVisitor
	{
	public:
		virtual void visit(SurfaceUVFun& fun) = 0;
		virtual void visit(SurfaceXYFun& fun) = 0;
		virtual void visit(Curve3DFun& fun) = 0;
	};

}//namespace CB

#endif // ShapeFun_H_883010A1_0C8F_40DA_A88C_32FBDE06AF1D