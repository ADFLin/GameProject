#pragma once
#ifndef ShapeFun_H_883010A1_0C8F_40DA_A88C_32FBDE06AF1D
#define ShapeFun_H_883010A1_0C8F_40DA_A88C_32FBDE06AF1D

#include "ShapeCommon.h"
#include "Expression.h"
#include "Math/SIMD.h"
#include "RHI/ShaderProgram.h"

#define USE_VALUE_INPUT 1

namespace CB
{

	class Curve3DFunc : public ShapeFuncBase
	{
	public:
		Curve3DFunc() :ShapeFuncBase() {}

		int  getFuncType() override { return TYPE_CURVE_3D; }
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
	private:

		Expression   mCoordExpr[3];
	};

	class SurfaceFunc : public ShapeFuncBase
	{
	public:
		bool   parseExpression(FunctionParser& parser){ return true; }
		bool   isParsed(){ return true; }
		int    getFuncType() override { return TYPE_SURFACE_XY; }
	};

	class NativeSurfaceXYFunc : public SurfaceFunc
	{
	public:
		NativeSurfaceXYFunc()
		{
			setDynamic(true);
			mUsedInputMask = BIT(0) | BIT(1);
		}
		virtual EEvalType getEvalType() { return EEvalType::Native; }

		template< typename TFunc >
		void setFunc(TFunc funcPtr)
		{
			mPtr = funcPtr;
			bSupportSIMD = std::is_same_v< Meta::FuncTraits<TFunc>::ResultType, FloatVector>;
		}
		int  getFuncType() override { return TYPE_SURFACE_XY; }
		void evalExpr(Vector3& out, float x, float y)
		{
			out.setValue(x, y, (float)((*static_cast<FuncType2>(mPtr))(x,y)));
		}

		void evalExpr(FloatVector const& x, FloatVector const& y, FloatVector& outZ)
		{
			outZ = (*static_cast<FloatVector (*)(FloatVector , FloatVector )>(mPtr))(x,y);
		}

		NativeSurfaceXYFunc* clone() override;
		void* mPtr;
		bool  bSupportSIMD = false;
	};


	class GPUSurfaceXYFunc : public SurfaceFunc
	{
	public:
		GPUSurfaceXYFunc()
		{
			bSupportSIMD = false;
		}
		virtual ~GPUSurfaceXYFunc() {}
		bool parseExpression(FunctionParser& parser) override;
		int  getFuncType() override { return TYPE_SURFACE_XY; }
		virtual EEvalType getEvalType() { return EEvalType::GPU; }
		bool isParsed() override
		{
			return mShader.isVaild();
		}

		void setExpr(std::string const& expr) 
		{
			mExpr = expr;
			mShader.releaseRHI();
		}
		std::string const& getExprString() { return mExpr; }
		GPUSurfaceXYFunc* clone() override;
	private:

		friend class ShapeMeshBuilder;
		std::string  mExpr;
		Render::ShaderProgram mShader;
	};

	class SurfaceXYFunc : public SurfaceFunc
	{
	public:
		SurfaceXYFunc():SurfaceFunc() 
		{
			bSupportSIMD = ExecutableCode::IsSupportSIMD;
			//bSupportSIMD = false;
		}
		virtual ~SurfaceXYFunc() {}

		int  getFuncType() override { return TYPE_SURFACE_XY; }
		bool parseExpression(FunctionParser& parser) override;
		bool isParsed() override { return mExpr.isParsed(); }
		void evalExpr(Vector3& out, float x, float y)
		{
			assert(isParsed());
			out.setValue(x, y, (float)mExpr.eval(x, y));
		}

		void evalExpr(FloatVector const& x, FloatVector const& y, FloatVector& outZ)
		{
			outZ = mExpr.eval(x, y);
		}

		void setExpr(std::string const& expr) { mExpr.setExprString(expr); }
		std::string const& getExprString() { return mExpr.getExprString(); }
		SurfaceXYFunc* clone() override;
	private:
		Expression   mExpr;
	};

	class SurfaceUVFunc : public SurfaceFunc
	{
	public:
		SurfaceUVFunc() :SurfaceFunc() {}

		int  getFuncType() override { return TYPE_SURFACE_UV; }
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
	private:
		Expression   mAixsExpr[3];
	};

	class NativeSurfaceUVFunc : public SurfaceFunc
	{
	public:
		NativeSurfaceUVFunc() :SurfaceFunc() {}

		int  getFuncType() override { return TYPE_SURFACE_UV; }
		bool parseExpression(FunctionParser& parser) override { return true; }
		bool isParsed() override { return true; }
		void evalExpr(Vector3& out, float u, float v)
		{
			out = Vector3(
				(*mAixsExpr[0])(u, v),
				(*mAixsExpr[1])(u, v),
				(*mAixsExpr[2])(u, v));
		}

		NativeSurfaceUVFunc* clone() override;
	private:
		FuncType2   mAixsExpr[3];
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
		virtual void visit(NativeSurfaceXYFunc& func) = 0;
		virtual void visit(NativeSurfaceUVFunc& func) = 0;
	};

}//namespace CB

#endif // ShapeFun_H_883010A1_0C8F_40DA_A88C_32FBDE06AF1D