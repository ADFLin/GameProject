#pragma once
#ifndef Surface_H_C89700BB_B3F2_4A03_A2F1_7AD4BE38ADA6
#define Surface_H_C89700BB_B3F2_4A03_A2F1_7AD4BE38ADA6

#include "Color.h"
#include "ShapeCommon.h"
#include "RenderData.h"

namespace CB
{
	class ShapeMeshBuilder;

	class ShapeVisitor;
	class ShapeFuncVisitor;


	class ShapeFuncBase;

	class ShapeBase
	{
	public:
		ShapeBase();
		virtual ~ShapeBase();

	public:

		bool               update(IShapeMeshBuilder& builder);
		void               visible(bool visable) { mbVisible = visable; }
		bool               isVisible() { return mbVisible; }
		ShapeFuncBase*     getFunction() const { return mShapeFunc; }
		void               setColor(Color4f const& color);
		Color4f const&     getColor() const { return mColor; }
		RenderData*        getRenderData() { return &mRenderData; }
		void               addUpdateBits(unsigned bits) { mUpdateBits |= bits; }

	public:
		virtual int        getShapeType() = 0;
		virtual ShapeBase* clone() = 0;
		virtual void       acceptVisit(ShapeVisitor& visitor) = 0;
	protected:
		virtual void       updateRenderData(IShapeMeshBuilder& builder, ShapeUpdateContext& context) = 0;


	protected:
		void    setFunctionInternal(ShapeFuncBase* FunData);
		ShapeBase(ShapeBase const& rhs);
	private:
		ShapeFuncBase*  mShapeFunc;
		bool           mbVisible;
		Color4f        mColor;
		RenderData     mRenderData;
		unsigned       mUpdateBits;

	};


	class SurfaceFunc;

	class Surface3D : public ShapeBase
	{
	public:
		Surface3D();
		~Surface3D();

		void setFunction(SurfaceFunc* func);
		int  getShapeType() override { return mCurType; }
		Surface3D*  clone() override;
		void updateRenderData(IShapeMeshBuilder& builder, ShapeUpdateContext& context) override;
		void acceptVisit(ShapeVisitor& visitor) override;

		void setDataSampleNum(int nu, int nv);
		SampleParam const& getParamU() const { return mParamU; }
		SampleParam const& getParamV() const { return mParamV; }
		void setRangeU(Range const& range) { mParamU.range = range; }
		void setRangeV(Range const& range) { mParamV.range = range; }

		float getMeshLineDensity() const { return mMeshLineDensity; }


		void setMeshLineDensity(float density) { mMeshLineDensity = density; addUpdateBits(RUF_COLOR); }

		void setIncrement(float incrementU, float incrementV);

		bool isShowMesh()   const { return mbShowMesh; }
		bool isShowNormal() const { return mbShowNormal; }
		bool isShowLine()   const { return mbShowLine; }

		void showMesh(bool beNeed) { mbShowMesh = beNeed; }
		void showNormal(bool beNeed) { mbShowNormal = beNeed; }
		void showLine(bool beNeed) { mbShowLine = beNeed; }


	protected:
		Surface3D(Surface3D const& rhs);
		int  mCurType;
		bool mbShowMesh;
		bool mbShowNormal;
		bool mbShowLine;

		float mMeshLineDensity;

		SampleParam  mParamU;
		SampleParam  mParamV;

	};



	class Curve3DFunc;

	class Curve3D : public ShapeBase
	{
	public:
		Curve3D();
		~Curve3D();

		Curve3D* clone() override;

		void setFunction(Curve3DFunc* func);

		int  getShapeType() override { return TYPE_CURVE_3D; }
		void setNumData(int n);

		SampleParam const& getParamS() const { return mParamS; }
		void setRangeS(Range const& range) { mParamS.range = range; }

		void acceptVisit(ShapeVisitor& visitor) override;

	protected:
		void updateRenderData(IShapeMeshBuilder& builder, ShapeUpdateContext& context) override;
		Curve3D(Curve3D const& rhs);


	private:
		SampleParam mParamS;
	};


	class ShapeVisitor
	{
	public:
		virtual void visit(Surface3D& surface) = 0;
		virtual void visit(Curve3D& curve) = 0;
	};

}//namespace CB

#endif // Surface_H_C89700BB_B3F2_4A03_A2F1_7AD4BE38ADA6
