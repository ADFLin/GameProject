#pragma once
#ifndef Surface_H_C89700BB_B3F2_4A03_A2F1_7AD4BE38ADA6
#define Surface_H_C89700BB_B3F2_4A03_A2F1_7AD4BE38ADA6

#include "Color.h"
#include "ShapeCommon.h"
#include "RenderData.h"

namespace CB
{
	class ShapeMaker;

	class ShapeVisitor;
	class ShapeFunVisitor;


	class ShapeFunBase;

	class ShapeBase
	{
	public:
		ShapeBase();
		virtual ~ShapeBase();

	public:

		bool               parseFunction(FunctionParser& parser);
		bool               update(ShapeMaker& surfaceMaker);
		void               visible(bool visable) { mbVisible = visable; }
		bool               isVisible() { return mbVisible; }
		ShapeFunBase*      getFunction() const { return mShapeFun; }
		void               setColor(Color4f const& color);
		Color4f const&     getColor() const { return mColor; }
		RenderData*        getRenderData() { return &mRenderData; }
		void               addUpdateBit(unsigned bit) { mUpdateBit |= bit; }

	public:
		virtual int        getShapeType() = 0;
		virtual ShapeBase* clone() = 0;
		virtual void       acceptVisit(ShapeVisitor& visitor) = 0;
	protected:
		virtual void       updateRenderData(ShapeUpdateInfo& info, ShapeMaker& maker) = 0;


	protected:
		void    setFunctionInternal(ShapeFunBase* FunData);
		ShapeBase(ShapeBase const& rhs);
	private:
		ShapeFunBase*  mShapeFun;
		bool           mbVisible;
		Color4f        mColor;
		RenderData     mRenderData;
		unsigned       mUpdateBit;

	};


	class SurfaceFun;

	class Surface3D : public ShapeBase
	{
	public:
		Surface3D();
		~Surface3D();

		void setFunction(SurfaceFun* fun);
		int  getShapeType() { return mCurType; }
		virtual Surface3D*  clone();
		virtual void updateRenderData(ShapeUpdateInfo& info, ShapeMaker& maker);
		virtual void acceptVisit(ShapeVisitor& visitor);

		void setDataSampleNum(int nu, int nv);
		SampleParam const& getParamU() const { return mParamU; }
		SampleParam const& getParamV() const { return mParamV; }
		void setRangeU(Range const& range) { mParamU.range = range; }
		void setRangeV(Range const& range) { mParamV.range = range; }

		float getMeshLineDensity() const { return mMeshLineDensity; }


		void setMeshLineDensity(float density) { mMeshLineDensity = density; addUpdateBit(RUF_COLOR); }

		void setIncrement(float incrementU, float incrementV);

		bool needDrawMesh()   const { return mNeedDrawMesh; }
		bool needDrawNormal() const { return mNeedDrawNormal; }
		bool needDrawLine()   const { return mNeedDrawLine; }

		void setDrawMesh(bool beNeed) { mNeedDrawMesh = beNeed; }
		void setDrawNormal(bool beNeed) { mNeedDrawNormal = beNeed; }
		void setDrawLine(bool beNeed) { mNeedDrawLine = beNeed; }


	protected:
		Surface3D(Surface3D const& rhs);
		int  mCurType;
		bool mNeedDrawMesh;
		bool mNeedDrawNormal;
		bool mNeedDrawLine;

		float mMeshLineDensity;

		SampleParam  mParamU;
		SampleParam  mParamV;

	};



	class Curve3DFun;

	class Curve3D : public ShapeBase
	{
	public:
		Curve3D();
		~Curve3D();

		virtual Curve3D*  clone();

		void setFunction(Curve3DFun* fun);

		int  getShapeType() { return TYPE_CURVE_3D; }
		void setNumData(int n);

		SampleParam const& getParamS() const { return mParamS; }
		void setRangeS(Range const& range) { mParamS.range = range; }

		virtual void acceptVisit(ShapeVisitor& visitor);

	protected:
		virtual void updateRenderData(ShapeUpdateInfo& info, ShapeMaker& maker);
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
