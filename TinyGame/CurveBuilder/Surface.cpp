#include "Surface.h"

#include "FunctionParser.h"
#include "ShapeFunction.h"
#include "ProfileSystem.h"

#include <cassert>
#include <cmath>


namespace CB
{
	ShapeBase::ShapeBase()
		: mShapeFun(nullptr)
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

	void ShapeBase::setFunctionInternal(ShapeFuncBase* FunData)
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

	bool ShapeBase::update(IShapeMeshBuilder& builder)
	{
		if( mUpdateBit & RUF_FUNCTION )
		{
			TIME_SCOPE("Pares Shape Function");
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
			ShapeUpdateContext context;
			context.color = getColor();
			context.data  = &mRenderData;
			context.flag  = mUpdateBit;
			context.func   = getFunction();

			mUpdateBit = 0;
			updateRenderData(builder, context);

			return true;
		}
		return false;
	}


	Surface3D::Surface3D()
		:ShapeBase()
		, mbShowMesh(true)
		, mbShowNormal(false)
		, mbShowLine(true)
		, mMeshLineDensity(1.0f)
		, mParamU(Range(-10, 10), 0)
		, mParamV(Range(-10, 10), 0)

	{
		setDataSampleNum(100, 100);
	}

	Surface3D::Surface3D(Surface3D const& rhs)
		:ShapeBase(rhs)
		, mbShowMesh(rhs.mbShowMesh)
		, mbShowNormal(rhs.mbShowNormal)
		, mbShowLine(rhs.mbShowLine)
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

	void Surface3D::updateRenderData(IShapeMeshBuilder& builder, ShapeUpdateContext& context)
	{
		builder.updateSurfaceData(context, mParamU, mParamV);
	}

	void Surface3D::acceptVisit(ShapeVisitor& visitor)
	{
		visitor.visit(*this);;
	}

	void Surface3D::setFunction(SurfaceFunc* func)
	{
		setFunctionInternal(func);
		mCurType = (func) ? func->getFuncType() : TYPE_SURFACE;
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

	void Curve3D::setFunction(Curve3DFunc* func)
	{
		setFunctionInternal(func);
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

	void Curve3D::updateRenderData(IShapeMeshBuilder& builder, ShapeUpdateContext& context)
	{
		builder.updateCurveData(context, mParamS);
	}

	void Curve3D::acceptVisit(ShapeVisitor& visitor)
	{
		return visitor.visit(*this);
	}

}//namespace CB