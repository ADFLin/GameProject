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


	class SurfaceXYFunc : public SurfaceFunc
	{
	public:
		SurfaceXYFunc(bool bUseGPU)
			:mbUseGPU(bUseGPU)
		{
			bSupportSIMD = ExecutableCode::IsSupportSIMD && !bUseGPU;
		}
		virtual ~SurfaceXYFunc() {}
		int  getFuncType() override { return TYPE_SURFACE_XY; }
		EEvalType getEvalType() override { return mbUseGPU ? EEvalType::GPU : EEvalType::CPU; }
		bool parseExpression(FunctionParser& parser) override;
		bool isParsed() override { return mExpr.isParsed(); }
		void evalExpr(Vector3& out, float x, float y)
		{
			assert(isParsed());
			out.setValue(x, y, mExpr.GetEvalResource<ExecutableCode>().evalT<RealType>(x, y));
		}

		void evalExpr(FloatVector const& x, FloatVector const& y, FloatVector& outZ)
		{
			outZ = mExpr.GetEvalResource<ExecutableCode>().evalT<FloatVector>(x, y);
		}

		template< typename T>
		void evalExpr(TArrayView<T const> valueBuffers, T& outZ)
		{
			outZ = mExpr.GetEvalResource<ExecutableCode>().evalT<T>(valueBuffers);
		}

		void setExpr(std::string const& expr) { mExpr.setExprString(expr); }
		std::string const& getExprString() { return mExpr.getExprString(); }

		Render::Shader& getShaderResrouce() { return mExpr.GetEvalResource<Render::Shader>(); }

		SurfaceXYFunc* clone() override;
	private:
		friend class ShapeMeshBuilder;
		bool         mbUseGPU;
		Expression   mExpr;
	};


	class SurfaceUVFunc : public SurfaceFunc
	{
	public:
		SurfaceUVFunc(bool bUseGPU) :SurfaceFunc()
			,mbUseGPU(bUseGPU)
		{
			bSupportSIMD = ExecutableCode::IsSupportSIMD && !bUseGPU;
		}

		int  getFuncType() override { return TYPE_SURFACE_UV; }
		EEvalType getEvalType() override { return mbUseGPU ? EEvalType::GPU : EEvalType::CPU; }
		bool parseExpression(FunctionParser& parser) override;
		bool isParsed() override;
		void evalExpr(Vector3& out, float u, float v);
		void evalExpr(FloatVector const& u, FloatVector const& v, FloatVector& outX, FloatVector& outY, FloatVector& outZ);

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

		Render::Shader& getShaderResrouce(){ return mAixsExpr[0].GetEvalResource<Render::Shader>(); }
	private:
		friend class ShapeMeshBuilder;
		bool         mbUseGPU;
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
				(*static_cast<FuncType2>(mAixsExpr[0]))(u, v),
				(*static_cast<FuncType2>(mAixsExpr[1]))(u, v),
				(*static_cast<FuncType2>(mAixsExpr[2]))(u, v));
		}

		void evalExpr(FloatVector const& u, FloatVector const& v, FloatVector& outX, FloatVector& outY, FloatVector& outZ)
		{
			outX = (*static_cast<FloatVector(*)(FloatVector, FloatVector)>(mAixsExpr[0]))(u, v);
			outY = (*static_cast<FloatVector(*)(FloatVector, FloatVector)>(mAixsExpr[1]))(u, v);
			outZ = (*static_cast<FloatVector(*)(FloatVector, FloatVector)>(mAixsExpr[2]))(u, v);
		}

		NativeSurfaceUVFunc* clone() override;
	private:
		void* mAixsExpr[3];
	};


	class SFImplicitFunc : public SurfaceFunc
	{
	private:
		Expression mExpr;
	};

}//namespace CB

#endif // ShapeFun_H_883010A1_0C8F_40DA_A88C_32FBDE06AF1D