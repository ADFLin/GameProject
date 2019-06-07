#include "Surface.h"

#include "FunctionParser.h"
#include "ShapeMeshBuilder.h"
#include "ShapeFun.h"

#include <cassert>
#include <cmath>

namespace CB
{
	ShapeBase::ShapeBase()
		:mShapeFun(NULL)
		, mbVisible(true)
		, mUpdateBit(RUF_ALL_UPDATE_BIT)
		, mColor(Color4f(1, 0, 0, 0.5f))
		, mRenderData()
	{

	}

	ShapeBase::ShapeBase(ShapeBase const& rhs)
		:mColor(rhs.mColor)
		, mbVisible(rhs.mbVisible)
		, mRenderData()
	{
		mShapeFun = rhs.mShapeFun->clone();
		mUpdateBit = RUF_ALL_UPDATE_BIT;
	}

	ShapeBase::~ShapeBase()
	{
		delete mShapeFun;
	}

	void ShapeBase::setFunctionInternal(ShapeFunBase* FunData)
	{
		delete mShapeFun;
		mShapeFun = FunData;
		mUpdateBit |= RUF_FUNCTION;
	}

	void ShapeBase::setColor(Color4f const& color)
	{
		mColor = color;
		addUpdateBit(RUF_COLOR);
	}

	bool ShapeBase::update(ShapeMeshBuilder& builder)
	{
		if( mUpdateBit & RUF_FUNCTION )
		{
			mUpdateBit &= ~RUF_FUNCTION;
			if( !getFunction()->isParsed() )
			{
				if( !builder.parseFunction(*getFunction()) )
					return false;
				mUpdateBit |= RUF_GEOM;
			}
		}

		if( !getFunction()->isParsed() )
			return false;

		if( getFunction()->isDynamic() && isVisible() )
		{
			mUpdateBit |= RUF_GEOM;
		}

		if( mUpdateBit )
		{
			ShapeUpdateInfo info;
			info.color = getColor();
			info.data  = &mRenderData;
			info.flag  = mUpdateBit;
			info.fun   = getFunction();

			mUpdateBit = 0;
			updateRenderData(info, builder);

			return true;
		}
		return false;
	}


	Surface3D::Surface3D()
		:ShapeBase()
		, mNeedDrawMesh(true)
		, mNeedDrawNormal(false)
		, mNeedDrawLine(true)
		, mMeshLineDensity(1.0f)
		, mParamU(Range(-10, 10), 0)
		, mParamV(Range(-10, 10), 0)

	{
		setDataSampleNum(100, 100);
	}

	Surface3D::Surface3D(Surface3D const& rhs)
		:ShapeBase(rhs)
		, mNeedDrawMesh(rhs.mNeedDrawMesh)
		, mNeedDrawNormal(rhs.mNeedDrawNormal)
		, mNeedDrawLine(rhs.mNeedDrawLine)
		, mMeshLineDensity(rhs.mMeshLineDensity)
		, mParamU(rhs.mParamU)
		, mParamV(rhs.mParamV)
		, mCurType(rhs.mCurType)
	{
		setDataSampleNum(mParamU.numData, mParamV.numData);
	}

	Surface3D::~Surface3D()
	{

	}


	void Surface3D::setDataSampleNum(int nu, int nv)
	{
		int newNumData = nu * nv;
		mParamU.numData = nu;
		mParamV.numData = nv;
		addUpdateBit(RUF_DATA_SAMPLE);
	}

	Surface3D* Surface3D::clone()
	{
		Surface3D* surface = new Surface3D(*this);
		return surface;
	}

	void Surface3D::setIncrement(float incrementU, float incrementV)
	{
		mParamU.setIncrement(incrementU);
		mParamV.setIncrement(incrementV);
		setDataSampleNum(mParamU.getNumData(), mParamV.getNumData());
	}


	void Surface3D::updateRenderData(ShapeUpdateInfo& info, ShapeMeshBuilder& builder)
	{
		builder.updateSurfaceData(info, mParamU, mParamV);
	}



	void Surface3D::acceptVisit(ShapeVisitor& visitor)
	{
		visitor.visit(*this);;
	}

	void Surface3D::setFunction(SurfaceFun* fun)
	{
		setFunctionInternal(fun);
		mCurType = (fun) ? fun->getFunType() : TYPE_SURFACE;
	}



	Curve3D::Curve3D()
		:ShapeBase()
		, mParamS(Range(-10, 10), 0)
	{
		setNumData(100);
	}

	Curve3D::Curve3D(Curve3D const& rhs)
		:ShapeBase(rhs)
		, mParamS(rhs.mParamS)
	{
		setNumData(mParamS.numData);
	}


	Curve3D::~Curve3D()
	{

	}

	void Curve3D::setFunction(Curve3DFun* fun)
	{
		setFunctionInternal(fun);
	}


	void Curve3D::setNumData(int n)
	{
		mParamS.numData = n;
		addUpdateBit(RUF_DATA_SAMPLE);
	}

	Curve3D* Curve3D::clone()
	{
		Curve3D* curve = new Curve3D(*this);
		return curve;
	}

	void Curve3D::updateRenderData(ShapeUpdateInfo& info, ShapeMeshBuilder& builder)
	{
		builder.updateCurveData(info, mParamS);
	}


	void Curve3D::acceptVisit(ShapeVisitor& visitor)
	{
		return visitor.visit(*this);
	}

}//namespace CB