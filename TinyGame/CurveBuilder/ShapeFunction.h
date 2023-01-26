#pragma once
#ifndef ShapeFun_H_883010A1_0C8F_40DA_A88C_32FBDE06AF1D
#define ShapeFun_H_883010A1_0C8F_40DA_A88C_32FBDE06AF1D

#include "ShapeCommon.h"
#include "Expression.h"

#define USE_VALUE_INPUT 1

namespace CB
{

	class Curve3DFunc : public ShapeFuncBase
	{
	public:
		Curve3DFunc() :ShapeFuncBase() {}

		int  getFunType() override { return TYPE_CURVE_3D; }
		bool parseExpression(FunctionParser& parser) override;
		bool isParsed() override;
		void evalExpr(Vector3& out, float s);
		void setExpr(int axis, std::string const& expr)
		{
			assert(axis >= 0 && axis < 3);
			mCoordExpr[axis].setExprString(expr);
		}
		std::string const& getExprString(int axis)
		{
			assert(axis >= 0 && axis < 3);
			return mCoordExpr[axis].getExprString();
		}

		Curve3DFunc* clone() override;
		void acceptVisit(ShapeFuncVisitor& visitor) override;
	private:

		Expression   mCoordExpr[3];
	};

	class SurfaceFunc : public ShapeFuncBase
	{

	};

	class SurfaceXYFunc : public SurfaceFunc
	{
	public:
		SurfaceXYFunc():SurfaceFunc() {}
		virtual ~SurfaceXYFunc() {}

		int  getFunType() override { return TYPE_SURFACE_XY; }
		bool parseExpression(FunctionParser& parser) override;
		bool isParsed() override { return mExpr.isParsed(); }
		void evalExpr(Vector3& out, float x, float y)
		{
			assert(isParsed());
			out.setValue(x, y, mExpr.eval(x, y));
		}
		void setExpr(std::string const& expr) { mExpr.setExprString(expr); }
		std::string const& getExprString() { return mExpr.getExprString(); }

		void acceptVisit(ShapeFuncVisitor& visitor) override;
		SurfaceXYFunc* clone() override;
	private:
		Expression   mExpr;
	};

	class SurfaceUVFunc : public SurfaceFunc
	{
	public:
		SurfaceUVFunc() :SurfaceFunc() {}

		int  getFunType() override { return TYPE_SURFACE_UV; }
		bool parseExpression(FunctionParser& parser) override;
		bool isParsed() override;
		void evalExpr(Vector3& out, float u, float v);
		void setExpr(int axis, std::string const& expr)
		{
			assert(axis >= 0 && axis < 3);
			mAixsExpr[axis].setExprString(expr);
		}
		std::string const& getExprString(int axis)
		{
			assert(axis >= 0 && axis < 3);
			return mAixsExpr[axis].getExprString();
		}
		SurfaceUVFunc* clone() override;
		void acceptVisit(ShapeFuncVisitor& visitor) override;
	private:
		Expression   mAixsExpr[3];

	};

	class SFImplicitFun : public SurfaceFunc
	{
	private:
		Expression mExpr;
	};

	class ShapeFuncVisitor
	{
	public:
		virtual void visit(SurfaceUVFunc& func) = 0;
		virtual void visit(SurfaceXYFunc& func) = 0;
		virtual void visit(Curve3DFunc& func) = 0;
	};

}//namespace CB

#endif // ShapeFun_H_883010A1_0C8F_40DA_A88C_32FBDE06AF1D