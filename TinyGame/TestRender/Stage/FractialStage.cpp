#include "Stage/TestStageHeader.h"

#include "RHI/RHIGraphics2D.h"

#include "RHI/RHICommand.h"
#include "RHI/ShaderManager.h"
#include "RHI/DrawUtility.h"

#include "CurveBuilder/ColorMap.h"

#include "SuperFractial/SelectRectBase.h"
#include "RHI/GpuProfiler.h"
#include "GameRenderSetup.h"

#include "Math/BigFloat.h"

#include "TinyCore/GameGlobal.h"
#include "PropertySet.h"
#include <string>

using namespace Render;

typedef TBigFloat<60, 1> BigFloat;

struct TComplexBigFloat
{
	BigFloat r, i;

	void setZero() { r.setZero(); i.setZero(); }
	void setValue(double rv, double iv) { r.setValue(rv); i.setValue(iv); }

	// z = z^2 + c
	// (r + i*i)^2 + (cr + ci*i)
	// (r^2 - i^2 + cr) + (2*r*i + ci)*i
	void nextIteration(TComplexBigFloat const& c)
	{
		BigFloat r2 = r; r2.mul(r);
		BigFloat i2 = i; i2.mul(i);
		BigFloat ri = r; ri.mul(i);

		r = r2;
		r.sub(i2);
		r.add(c.r);

		i = ri;
		i.add(ri);
		i.add(c.i);
	}

	double getLength2() const
	{
		BigFloat r2 = r; r2.mul(r);
		BigFloat i2 = i; i2.mul(i);
		r2.add(i2);
		double res;
		r2.convertToDouble(res);
		return res;
	}
};

struct MandelbrotParam
{
	Vec2i viewSize;
	float rotationAngle;
	int   maxIteration;

	TVector2<double> center;
	TVector2<double> centerLow = TVector2<double>(0, 0);

	BigFloat centerFullR;
	BigFloat centerFullI;

	float bailoutValue;
	BigFloat magnification;
	double stretch = 1.0;
	std::string magnificationToStr() const
	{
		std::string str;
		BigFloat temp = magnification;
		temp.getString(str);
		return str;
	}
	void magnificationFromStr(char const* str)
	{
		magnification.setFromString(str);
	}

	static void DSAdd(double a, double al, double b, double bl, double& out, double& outL)
	{
		double s = a + b;
		double v = s - a;
		double e = (a - (s - v)) + (b - v);
		double e2 = e + (al + bl);
		out = s + e2;
		outL = e2 + (s - out);
	}

	static void DSMul(double a, double al, double b, double bl, double& out, double& outL)
	{
		const double split = 134217729.0;
		auto Split = [&](double val, double& hi, double& lo) {
			double con = val * split;
			hi = con - (con - val);
			lo = val - hi;
		};
		double ax, ay, bx, by;
		Split(a, ax, ay);
		Split(b, bx, by);
		double r1 = a * b;
		double r2 = ay * by - (((r1 - ax * bx) - ay * bx) - ax * by);
		r2 += (a * bl + al * b);
		out = r1 + r2;
		outL = r2 + (r1 - out);
	}

	TVector2<double> getRatio() const { 
		
		BigFloat val(0.005 * stretch);
		val.div(magnification);
		double rx;
		val.convertToDouble(rx);

		val.setValue(0.005);
		val.div(magnification);
		double ry;
		val.convertToDouble(ry);

		return TVector2<double>(rx, ry); 
	}

	void  zoomInPos(Vec2i const& centerOffset, float zoomFactor, float angle)
	{
		float s, c;
		Math::SinCos(rotationAngle, s, c);

		TVector2<double> pixelOffset = Vector2(centerOffset);
		double rx = c * pixelOffset.x + s * pixelOffset.y;
		double ry = -s * pixelOffset.x + c * pixelOffset.y;

		BigFloat dR(rx * 0.005 * stretch);
		dR.div(magnification);
		BigFloat dI(ry * 0.005);
		dI.div(magnification);

		centerFullR.add(dR);
		centerFullI.add(dI);

		BigFloat lowR; lowR.setValue(centerLow.x);
		centerFullR.add(lowR);
		BigFloat lowI; lowI.setValue(centerLow.y);
		centerFullI.add(lowI);
		centerLow = TVector2<double>(0, 0);

		centerFullR.convertToDouble(center.x);
		centerFullI.convertToDouble(center.y);

		BigFloat scale(1.0 / (double)zoomFactor);
		magnification.mul(scale);

		setRotationAngle(rotationAngle + angle);
	}

	void getBigFloatCoordinates(Vector2 pixelPos, BigFloat& outR, BigFloat& outI) const
	{
		TVector2<double> ratio = getRatio();
		TVector2<double> len = (TVector2<double>(pixelPos) - 0.5 * TVector2<double>(viewSize)).mul(ratio);
		float s, c;
		Math::SinCos(rotationAngle, s, c);
		TVector2<double> disp = TVector2<double>(c * len.x + s * len.y, -s * len.x + c * len.y);
		
		BigFloat dR, dI;
		dR.setValue(disp.x);
		dI.setValue(disp.y);
		
		outR = centerFullR; outR.add(dR);
		outI = centerFullI; outI.add(dI);
	}

	void setRotationAngle(float angle)
	{
		rotationAngle = WrapZeroTo2PI( angle );
	}

	TVector2<double> getCoordinates(Vector2 pixelPos) const
	{
		TVector2<double> len = (TVector2<double>(pixelPos) - 0.5 * TVector2<double>(viewSize)).mul(getRatio());
		float s, c;
		Math::SinCos(rotationAngle, s, c);
		return center + TVector2<double>(c*len.x + s*len.y, -s*len.x + c*len.y);
	}

	MandelbrotParam()
	{
		setDefalut();
	}

	void setDefalut()
	{
		rotationAngle = 0;
		maxIteration = 20000;
		center = TVector2<double>(-0.5, 0);
		centerLow = TVector2<double>(0, 0);
		bailoutValue = 2048.0;
		magnification.setValue(1.0);
		stretch = 1.0;
		centerFullR.setValue(-0.5);
		centerFullI.setZero();
	}
};


struct GPU_ALIGN MandelbrotParamData
{
	DECLARE_UNIFORM_BUFFER_STRUCT(MParamsBlock)

	TVector2<double> CoordParam;    // High
	TVector2<double> CoordParam2;   // High
	Vector4          ColorMapParam;
	TVector2<double> Rotate;
	IntVector2  ViewSize;
	int         MaxIteration;
	int         Padding;
	int         PaddingAlignment[40]; // Pad to 256 bytes for D3D12 CBV alignment

	void set(MandelbrotParam const& param)
	{
		CoordParam.x = param.center.x;
		CoordParam.y = param.center.y;

		TVector2<double> ratio = param.getRatio();
		CoordParam2.x = ratio.x;
		CoordParam2.y = ratio.y;

		float s, c;
		Math::SinCos(param.rotationAngle, s, c);
		Rotate.x = c;
		Rotate.y = s;

		ViewSize = param.viewSize;
		MaxIteration = param.maxIteration;
		ColorMapParam = Vector4(1, 0, param.bailoutValue * param.bailoutValue, 0);
	}
};


struct GPU_ALIGN MandelbrotParamData_Double
{
	DECLARE_UNIFORM_BUFFER_STRUCT(MParamsBlock)

	TVector2<double> CoordParam;    // High (Real, Imag)
	TVector2<double> CoordParam2;   // High (RatioX, RatioY)
	Vector4          ColorMapParam;
	TVector2<double> CoordParamLow; // Low  (Real, Imag)
	TVector2<double> CoordParam2Low;// Low  (RatioX, RatioY)
	TVector2<double> Rotate;        // Cos, Sin
	IntVector2  ViewSize;
	int         MaxIteration;
	int         Padding;
	int         PaddingAlignment[36];

	void set(MandelbrotParam const& param)
	{
		CoordParam.x = param.center.x;
		CoordParam.y = param.center.y;

		TVector2<double> ratio = param.getRatio();
		CoordParam2.x = ratio.x;
		CoordParam2.y = ratio.y;

		float s, c;
		Math::SinCos(param.rotationAngle, s, c);
		Rotate.x = c;
		Rotate.y = s;

		ViewSize = param.viewSize;
		MaxIteration = param.maxIteration;
		ColorMapParam = Vector4(1, 0, param.bailoutValue * param.bailoutValue, 0);

		CoordParamLow.x = param.centerLow.x;
		CoordParamLow.y = param.centerLow.y;
		CoordParam2Low = TVector2<double>(0, 0);
	}
};

struct GPU_ALIGN MandelbrotPerturbParamData
{
	DECLARE_UNIFORM_BUFFER_STRUCT(MParamsBlock)

	TVector2<double> CoordParam;     // Slot 0: Center High
	TVector2<double> CoordParam2;    // Slot 1: Ratio High
	Vector4          ColorMapParam;  // Slot 2
	TVector2<double> CoordParamLow;  // Slot 3: Center Low
	TVector2<double> CoordParam2Low; // Slot 4: Ratio Low
	TVector2<double> Rotate;         // Slot 5: Cos, Sin
	IntVector2       ViewSize;       // Slot 6
	int              MaxIteration;
	int              Padding;
	TVector2<double> RefCoord;       // Slot 7: Reference Point High (Display Only)
	TVector2<double> RefCoordLow;    // Slot 8
	TVector2<double> CenterToRefDelta_Rel; // Slot 9: Delta normalized to Viewport scale
	float  ZoomScale;  // Slot 10: The scale factor (1.0 / zoom) - 4 bytes
	float  MaxIter_f;  // 4 bytes
	int    PaddingAlignment[22]; // Total struct now: 168 + 22*4 = 256 bytes

	void set(MandelbrotParam const& param, BigFloat const& refR, BigFloat const& refI)
	{
		CoordParam.x = param.center.x;
		CoordParam.y = param.center.y;
		CoordParamLow.x = param.centerLow.x;
		CoordParamLow.y = param.centerLow.y;

		auto DSDiv = [&](double a, double b, double bl, double& outH, double& outL) {
			outH = a / b;
			outL = (a - outH * b - outH * bl) / b;
		};
		// High-precision Ratio Calculation using BigFloat
		{
			BigFloat bfRatio(0.005 * param.stretch);
			bfRatio.div(param.magnification);
			bfRatio.convertToDouble(CoordParam2.x);
			// We can't easily compute low part of ratio without more BigFloat ops, 
			// but for e308+ it is 0 anyway. Approximating 0 for low part.
			CoordParam2Low.x = 0; 

			bfRatio.setValue(0.005);
			bfRatio.div(param.magnification);
			bfRatio.convertToDouble(CoordParam2.y);
			CoordParam2Low.y = 0;
		}

		float s, c;
		Math::SinCos(param.rotationAngle, s, c);
		Rotate.x = c;
		Rotate.y = s;

		ViewSize = param.viewSize;
		MaxIteration = param.maxIteration;
		Padding = 0;

		double rH, rL;
		refR.convertToDouble(rH);
		RefCoord.x = rH;
		BigFloat dR = refR; dR.sub(BigFloat(rH)); dR.convertToDouble(rL);
		RefCoordLow.x = rL;

		refI.convertToDouble(rH);
		RefCoord.y = rH;
		BigFloat dI = refI; dI.sub(BigFloat(rH)); dI.convertToDouble(rL);
		RefCoordLow.y = rL;

		// CALCULATE ABSOLUTE DELTA
		// This is the key to preserving precision! We keep delta in world units (e.g. 10^-100)
		// rather than scaling it up to pixel units (1.0). Floating point mantissa 
		// remains precise for small numbers.
		BigFloat dx = param.centerFullR; dx.sub(refR);
		BigFloat dy = param.centerFullI; dy.sub(refI);

		double dx_abs, dy_abs;
		dx.convertToDouble(dx_abs);
		dy.convertToDouble(dy_abs);

		CenterToRefDelta_Rel.x = dx_abs;
		CenterToRefDelta_Rel.y = dy_abs;

		ZoomScale = 0; // Fallback for huge mag
		{
			BigFloat scale(1.0);
			scale.div(param.magnification);
			double dScale;
			scale.convertToDouble(dScale);
			ZoomScale = (float)dScale;
		}
		MaxIter_f = (float)param.maxIteration;
	
		ColorMapParam = Vector4(1, 0, (float)(param.bailoutValue * param.bailoutValue), 0);

		// CPU VERIFY:
		TVector2<double> cpuCoord = param.getCoordinates(Vector2(param.viewSize) * 0.5f);
		LogMsg("CPU Verify (Center Pixel): (%f, %f)", cpuCoord.x, cpuCoord.y);
		LogMsg("Shader Params: Center=(%f, %f), Ratio=(%f, %f), Rot=(%c=%f, s=%f)", 
			CoordParam.x + CoordParamLow.x, CoordParam.y + CoordParamLow.y,
			CoordParam2.x + CoordParam2Low.x, CoordParam2.y + CoordParam2Low.y,
			'c', Rotate.x, Rotate.y);
	}
};



class MandelbrotShader : public GlobalShader
{
	DECLARE_SHADER(MandelbrotShader, Global);

public:
	static int constexpr SizeX = 16;
	static int constexpr SizeY = 16;

	SHADER_PERMUTATION_TYPE_BOOL(UseNativeDouble, SHADER_PARAM(USE_NATIVE_DOUBLE));
	SHADER_PERMUTATION_TYPE_BOOL(UseDoubleSim, SHADER_PARAM(USE_DOUBLE_SIM));
	using PermutationDomain = TShaderPermutationDomain<UseNativeDouble, UseDoubleSim>;

	static void SetupShaderCompileOption(PermutationDomain const& domain, ShaderCompileOption& option)
	{
		option.addDefine(SHADER_PARAM(SIZE_X), SizeX);
		option.addDefine(SHADER_PARAM(SIZE_Y), SizeY);
	}

	void bindParameters(ShaderParameterMap const& parameterMap)
	{
		BIND_SHADER_PARAM(parameterMap, ColorMapParam);
		BIND_SHADER_PARAM(parameterMap, ColorRWTexture);
		BIND_TEXTURE_PARAM(parameterMap, ColorMapTexture);
	}

	void setParameters(RHICommandList& commandList, MandelbrotParam const& param, RHITexture2D& colorTexture, RHITexture1D& colorMapTexture)
	{
		setRWTexture(commandList, SHADER_MEMBER_PARAM(ColorRWTexture), colorTexture, 0, EAccessOp::WriteOnly);
		auto& samplerState = TStaticSamplerState< ESampler::Bilinear, ESampler::Wrap >::GetRHI();
		SET_SHADER_TEXTURE_AND_SAMPLER(commandList, *this, ColorMapTexture, colorMapTexture, samplerState);
		SET_SHADER_PARAM_VALUE(commandList, *this, ColorMapParam, Vector4(1, 0, param.bailoutValue * param.bailoutValue, 0));
	}

	void clearParameters(RHICommandList& commandList)
	{
		clearRWTexture(commandList, mParamColorRWTexture);
	}

	static char const* GetShaderFileName()
	{
		return "Shader/Game/Mandelbrot";
	}

	DEFINE_SHADER_PARAM(ColorMapParam);
	DEFINE_SHADER_PARAM(ColorRWTexture);
	DEFINE_TEXTURE_PARAM(ColorMapTexture);

};

IMPLEMENT_SHADER(MandelbrotShader , EShader::Compute , SHADER_ENTRY(MainCS));


class MandelbrotAnalysisShader : public GlobalShader
{
	DECLARE_SHADER(MandelbrotAnalysisShader, Global);
public:
	static void SetupShaderCompileOption(ShaderCompileOption& option) 
	{
		option.addDefine(SHADER_PARAM(SIZE_X), 16);
		option.addDefine(SHADER_PARAM(SIZE_Y), 16);
	}
	static char const* GetShaderFileName() { return "Shader/Game/MandelbrotPerturb"; }
	
	void bindParameters(ShaderParameterMap const& parameterMap)
	{
		BIND_SHADER_PARAM(parameterMap, ColorRWTexture);
		BIND_SHADER_PARAM(parameterMap, ResultBuffer);
		BIND_SHADER_PARAM(parameterMap, MParamsBlock);
	}

	void setParameters(RHICommandList& commandList, RHITexture2D& texture, RHIBuffer& resultBuffer, TStructuredBuffer< MandelbrotPerturbParamData >& paramsBuffer)
	{
		setRWTexture(commandList, SHADER_MEMBER_PARAM(ColorRWTexture), texture, 0, EAccessOp::ReadOnly);
		setStorageBuffer(commandList, SHADER_MEMBER_PARAM(ResultBuffer), resultBuffer, EAccessOp::ReadAndWrite);
		SetStructuredUniformBuffer(commandList, *this, paramsBuffer);
	}

	void clearParameters(RHICommandList& commandList)
	{
		clearRWTexture(commandList, mParamColorRWTexture);
	}
	
	DEFINE_SHADER_PARAM(ColorRWTexture);
	DEFINE_SHADER_PARAM(ResultBuffer);
	DEFINE_SHADER_PARAM(MParamsBlock);
};

IMPLEMENT_SHADER(MandelbrotAnalysisShader, EShader::Compute, SHADER_ENTRY(AnalysisCS));

class MandelbrotPerturbShader : public GlobalShader
{
	DECLARE_SHADER(MandelbrotPerturbShader, Global);

public:
	static int constexpr SizeX = 16;
	static int constexpr SizeY = 16;

	static void SetupShaderCompileOption(ShaderCompileOption& option)
	{
		option.addDefine(SHADER_PARAM(SIZE_X), SizeX);
		option.addDefine(SHADER_PARAM(SIZE_Y), SizeY);
	}

	void bindParameters(ShaderParameterMap const& parameterMap)
	{
		BIND_SHADER_PARAM(parameterMap, ColorRWTexture);
		BIND_TEXTURE_PARAM(parameterMap, ColorMapTexture);
		BIND_SHADER_PARAM(parameterMap, OrbitBuffer);
	}


	void setParameters(RHICommandList& commandList, MandelbrotParam const& param, RHITexture2D& colorTexture, RHITexture1D& colorMapTexture, RHIResource* orbitBuffer, TStructuredBuffer< MandelbrotPerturbParamData >& paramsBuffer)
	{
		setRWTexture(commandList, SHADER_MEMBER_PARAM(ColorRWTexture), colorTexture, 0, EAccessOp::WriteOnly);
		auto& samplerState = TStaticSamplerState< ESampler::Bilinear, ESampler::Wrap >::GetRHI();
		SET_SHADER_TEXTURE_AND_SAMPLER(commandList, *this, ColorMapTexture, colorMapTexture, samplerState);

		setStorageBuffer(commandList, SHADER_MEMBER_PARAM(OrbitBuffer), *static_cast<RHIBuffer*>(orbitBuffer), EAccessOp::ReadAndWrite);
		SetStructuredUniformBuffer(commandList, *this, paramsBuffer);
	}

	void clearParameters(RHICommandList& commandList)
	{
		clearRWTexture(commandList, mParamColorRWTexture);
	}

	static char const* GetShaderFileName()
	{
		return "Shader/Game/MandelbrotPerturb";
	}

	DEFINE_SHADER_PARAM(ColorRWTexture);
	DEFINE_TEXTURE_PARAM(ColorMapTexture);
	DEFINE_SHADER_PARAM(OrbitBuffer);
};

IMPLEMENT_SHADER(MandelbrotPerturbShader, EShader::Compute, SHADER_ENTRY(MainCS));


class SelectRect : public SelectRectOperationT< SelectRect >
{
public:
	SelectRect()
	{
		enable(false);
	}


	int    clickArea;
	Vec2i  clickPos;

	bool procMouseMsg(MouseMsg const& msg)
	{
		Vec2i pos = msg.getPos();
		mHitArea = hitAreaTest(pos.x, pos.y);
		changeCursor(mHitArea);

		if (msg.onLeftDown() && !isEnable())
		{
			clickArea = CORNER_RT;
			mHitArea = CORNER_RT;
			reset(pos.x, pos.y);
		}
		else if (msg.onLeftDown())
		{
			if (isEnable() && mHitArea == OUTSIDE)
			{
				enable(false);
				repaint();
			}

			if (isEnable())
			{
				clickArea = mHitArea;
				clickPos = getCenterPos();
				clickPos -= pos;
			}
			else
			{
				clickPos = pos;
				return false;
			}
		}
		else if (msg.onLeftUp())
		{
			clickArea = 0;
			clickPos = Vec2i(0, 0);
		}
		else if (msg.isLeftDown() && msg.isDraging())
		{
			if (clickArea & SIDE)
				resize(clickArea, pos.x, pos.y);
			else if (clickArea == INSIDE)
				moveTo(pos.x + clickPos.x, pos.y + clickPos.y);
			else if (clickArea == ROTEZONE)
				rotate(pos.x, pos.y);
		}

		return true;
	}

	void draw(RHIGraphics2D& g)
	{
		if (!mbEnable) return;

		RHICommandList& commandList = RHICommandList::GetImmediateList();

		Vector2 sizeHalf = 0.5 * Vector2(getSize());

		Vector2 points[6] =
		{
			sizeHalf,
			sizeHalf.mul(Vector2(1,-1)),
			sizeHalf.mul(Vector2(-1,-1)) ,
			sizeHalf.mul(Vector2(-1,1)) ,
			{0,-(sizeHalf.y + RoteLineLength) },
			{0,-sizeHalf.y },
		};

		for (int i = 0; i < 6; ++i)
		{
			points[i] = localToWorldPos(points[i]);
		}

		g.setBrush(Color3f(1, 1, 1));
		g.setBlendState(ESimpleBlendMode::InvDestColor);
		g.enablePen(false);
		auto DrawLine = [&](Vector2 const& p1, Vector2 const& p2)
		{
			Vector2 d = p1 - p2;
			d.normalize();
			Vector2 offset = 0.5 * Vector2(d.y, -d.x);
			Vector2 v[4] = { p1 + offset , p1 - offset , p2 - offset, p2 + offset };
			g.drawPolygon(v, 4);
		};

		for (int i = 0; i < 4; i++)
		{
			int j = (i + 1) % 4;
			DrawLine(points[i], points[j]);
		}
		DrawLine(points[4], points[5]);

		g.setBlendState(ESimpleBlendMode::None);
		g.enablePen(true);
		g.setPen(Color3f(0, 0, 0));
		g.setBrush(Color3f(1, 1, 1));

		for (int i = 0; i < 5; i++)
		{
			g.drawRect(points[i] - Vector2(2, 2), Vector2(5, 5));
		}

	}
	void repaint()
	{

	}
	void changeCursor(int location)
	{

	}
};


class FractialTestStage : public StageBase
	                    , public IGameRenderSetup
{
	using BaseClass = StageBase;
public:
	FractialTestStage() {}


	MandelbrotShader* mProgMandelbrot = nullptr;
	MandelbrotPerturbShader* mProgMandelbrotPerturb = nullptr;
	MandelbrotAnalysisShader* mProgAnalysis = nullptr;
	RHITexture2DRef mTexture;
	RHITexture1DRef mColorMap;
	MandelbrotParam mParam;

	struct AnalysisResult { uint32 x, y; };
	TStructuredBuffer< AnalysisResult > mAnalysisBuffer;

	std::vector<TVector2<double>> mReferenceOrbit;
	TStructuredBuffer< TVector2<double> > mOrbitBuffer;

	BigFloat mRefFullR;
	BigFloat mRefFullI;
	int      mRefIter = 0;

	int iterateBigFloat(BigFloat const& cr, BigFloat const& ci, int scanLimit)
	{
		BigFloat zr; zr.setZero();
		BigFloat zi; zi.setZero();
		BigFloat r2, i2, ni, temp;
		
		// Use a temporary bailout check (4.0)
		for (int i = 0; i < scanLimit; ++i)
		{
			r2 = zr; r2.mul(zr);
			i2 = zi; i2.mul(zi);
			
			// Check escape
			temp = r2; temp.add(i2);
			double len2;
			temp.convertToDouble(len2);
			if (len2 > (double)mParam.bailoutValue * mParam.bailoutValue) return i;

			// ni = 2*zr*zi + ci
			ni = zr; ni.mul(zi); ni.mul(2); ni.add(ci);

			// zr = r2 - i2 + cr
			zr = r2; zr.sub(i2); zr.add(cr);
			zi = ni;
		}
		return scanLimit;
	}

	void findBestReferencePoint(bool bForce = false)
	{
		// DISABLED DRIFT: Keep reference point at viewport center for calibration
		mRefFullR = mParam.centerFullR;
		mRefFullI = mParam.centerFullI;
		mRefIter = iterateBigFloat(mRefFullR, mRefFullI, mParam.maxIteration);
	}

	void updateReferenceOrbit(BigFloat const* pRefR = nullptr, BigFloat const* pRefI = nullptr)
	{
		if (pRefR && pRefI)
		{
			mRefFullR = *pRefR;
			mRefFullI = *pRefI;
		}
		else
		{
			// Always evaluate current center first to avoid "Sticky Reference" from distant points
			mRefFullR = mParam.centerFullR;
			mRefFullI = mParam.centerFullI;
		}

		mRefIter = iterateBigFloat(mRefFullR, mRefFullI, mParam.maxIteration);

		// Only look for a better point IF the center is not already at max detail
		if (mRefIter < mParam.maxIteration * 0.95f)
		{
			findBestReferencePoint(mRefIter < 10);
		}

		mReferenceOrbit.resize(mParam.maxIteration);
		TComplexBigFloat z, c;
		z.setZero();
		c.r = mRefFullR;
		c.i = mRefFullI;

		mRefIter = mParam.maxIteration;
		for(int i = 0; i < mParam.maxIteration; ++i)
		{
			double r, im;
			z.r.convertToDouble(r);
			z.i.convertToDouble(im);

			mReferenceOrbit[i] = TVector2<double>(r, im);
			
			// Calculate |Zn|^2 using double for bailout check
			if(r * r + im * im > (double)mParam.bailoutValue * mParam.bailoutValue)
			{
				mRefIter = i;
				// Maintain values for safety
				for(int j = i + 1; j < mParam.maxIteration; ++j) 
					mReferenceOrbit[j] = TVector2<double>(r, im);
				break;
			}
			z.nextIteration(c);
		}

		LogMsg("Orbit Calculated: Escape at %d / %d iterations", mRefIter, mParam.maxIteration);

		if(!mOrbitBuffer.isValid() || mOrbitBuffer.getElementNum() < mParam.maxIteration)
		{
			mOrbitBuffer.initializeResource(mParam.maxIteration, EStructuredBufferType::Buffer, false);
		}
		
		mOrbitBuffer.updateBuffer(MakeConstView(mReferenceOrbit));
	}

	struct FractalBookmark
	{
		std::string name;
		std::string centerR;
		std::string centerI;
		std::string magnification;
		float rotation;
	};
	TArray<FractalBookmark> mBookmarks;
	GListCtrl* mUIBookmarkList = nullptr;

	void saveBookmarks()
	{
		PropertySet& config = Global::GameConfig();
		char const* section = "FractalBookmarks";

		TArray<std::string> values;
		for (auto const& bm : mBookmarks)
		{
			std::string line = bm.name + ";" + bm.centerR + ";" + bm.centerI + ";" + bm.magnification + ";" + std::to_string(bm.rotation);
			values.push_back(line);
		}
		config.setStringValues("Bookmark", section, values);
		config.saveFile("Game.ini");
	}

	void loadBookmarks()
	{
		PropertySet& config = Global::GameConfig();
		char const* section = "FractalBookmarks";

		TArray<std::string> values;
		config.getStringValues("Bookmark", section, values);
		
		for (auto const& val : values)
		{
			FractalBookmark bm;
			size_t pos = 0;
			auto NextToken = [&](std::string& outToken)
			{
				size_t next = val.find(';', pos);
				if (next == std::string::npos)
				{
					outToken = val.substr(pos);
					pos = val.length();
					return false;
				}
				outToken = val.substr(pos, next - pos);
				pos = next + 1;
				return true;
			};
			
			std::string token;
			NextToken(bm.name);
			NextToken(bm.centerR);
			NextToken(bm.centerI);
			NextToken(bm.magnification);
			if (bm.magnification.empty()) continue; // Invalid
			
			// Rotation might be last
			NextToken(token);
			bm.rotation = (float)atof(token.c_str());

			mBookmarks.push_back(bm);
			if (mUIBookmarkList)
			{
				mUIBookmarkList->addItem(bm.name.c_str());
			}
		}
	}

	SelectRect mSelectRect;

	virtual bool onInit()
	{
		if( !BaseClass::onInit() )
			return false;
		
		::Global::GUI().cleanupWidget();

		DevFrame* frame = WidgetUtility::CreateDevFrame();
		FWidgetProperty::Bind(frame->addCheckBox(UI_ANY, "Native Double"), bUseNativeDouble, [this](bool bValue) { bNeedUpdateTexture = true; });
		FWidgetProperty::Bind(frame->addCheckBox(UI_ANY, "Double Sim"), bUseDoubleSim, [this](bool bValue) { bNeedUpdateTexture = true; });
		FWidgetProperty::Bind(frame->addCheckBox(UI_ANY, "Use Perturbation"), bUsePerturb, [this](bool bValue) { bNeedUpdateTexture = true; });

		frame->addText("Bookmarks:");
		mUIBookmarkList = frame->addListCtrl(UI_ANY, Vec2i(180, 200));
		mUIBookmarkList->onEvent = [this](int event, GWidget* widget)
		{
			if (event == EVT_LISTCTRL_DCLICK)
			{
				int index = mUIBookmarkList->getSelection();
				if (mBookmarks.isValidIndex(index))
				{
					auto& bm = mBookmarks[index];
					mParam.centerFullR.setFromString(bm.centerR.c_str());
					mParam.centerFullI.setFromString(bm.centerI.c_str());
					mParam.magnificationFromStr(bm.magnification.c_str());
					mParam.rotationAngle = bm.rotation;
					double r, i;
					mParam.centerFullR.convertToDouble(r);
					mParam.centerFullI.convertToDouble(i);
					mParam.center = TVector2<double>(r, i);
					mParam.centerLow = TVector2<double>(0, 0); // Approximation reset
					
					// Force update reference orbit immediately to match new view
					updateReferenceOrbit();
					bNeedUpdateTexture = true;
				}
			}
			return false;
		};

		GTextCtrl* mUIBookmarkName = frame->addTextCtrl("Bookmark Name:");
		frame->addButton("Add Bookmark", [this, mUIBookmarkName](int id, GWidget* widget)
		{
			FractalBookmark bm;
			mParam.centerFullR.getString(bm.centerR);
			mParam.centerFullI.getString(bm.centerI);
			bm.magnification = mParam.magnificationToStr();
			bm.rotation = mParam.rotationAngle;
			
			std::string inputName = mUIBookmarkName->getValue();
			if (!inputName.empty())
			{
				bm.name = inputName;
			}
			else
			{
				char nameBuf[64];
				sprintf(nameBuf, "%.2e", 1.0 / atof(bm.magnification.c_str()));
				bm.name = nameBuf;
			}

			mBookmarks.push_back(bm);
			mUIBookmarkList->addItem(bm.name.c_str());
			mUIBookmarkName->setValue("");
			saveBookmarks();
			return false;
		});

		frame->addButton("Remove Bookmark", [this](int id, GWidget* widget)
		{
			int index = mUIBookmarkList->getSelection();
			if (mBookmarks.isValidIndex(index))
			{
				mBookmarks.removeIndex(index);
				mUIBookmarkList->removeItem(index);
				saveBookmarks();
			}
			return false;
		});

		loadBookmarks();

		return true;
	}

	virtual void postInit()
	{
		restart();
	}

	virtual ERenderSystem getDefaultRenderSystem() override
	{
		return ERenderSystem::D3D11;
	}
	virtual bool setupRenderResource(ERenderSystem systemName) override
	{
		mProgMandelbrot = ShaderManager::Get().getGlobalShaderT< MandelbrotShader >();
		mProgMandelbrotPerturb = ShaderManager::Get().getGlobalShaderT< MandelbrotPerturbShader >();
		mProgAnalysis = ShaderManager::Get().getGlobalShaderT< MandelbrotAnalysisShader >();


		ColorMap colorMap(1024);
		colorMap.addPoint(0, Color3ub(0, 7, 100));
		colorMap.addPoint(640 / 4, Color3ub(32, 107, 203));
		colorMap.addPoint(1680 / 4, Color3ub(237, 255, 255));
		colorMap.addPoint(2570 / 4, Color3ub(255, 170, 0));
		colorMap.addPoint(3430 / 4, Color3ub(0, 2, 0));
		colorMap.setSmoothLine(true);
		colorMap.calcColorMap(true);
		Color4f colors[1024];
		for (int i = 0; i < ARRAY_SIZE(colors); ++i)
		{
			Color3f color;
			colorMap.getColor(float(i) / 1024, color);
			colors[i] = color;
		}
		VERIFY_RETURN_FALSE(mColorMap = RHICreateTexture1D(ETexture::RGBA32F, 1024, 0, 1, TCF_DefalutValue, colors));

		Vec2i screenSize = ::Global::GetScreenSize();
		mParam.viewSize = screenSize;
		VERIFY_RETURN_FALSE(mTexture = RHICreateTexture2D( TextureDesc::Type2D(ETexture::RGBA32F, screenSize.x, screenSize.y).Flags(TCF_CreateUAV | TCF_CreateSRV | TCF_DefalutValue)) );
		mTexture->setDebugName("RWTexture");


		mMParamsBuffer.initializeResource(1);
		mMParamsBufferDouble.initializeResource(1);
		mMParamsBufferPerturb.initializeResource(1);
		mAnalysisBuffer.initializeResource(2, EStructuredBufferType::RWBuffer, true);
		return true;
	}
	void debugRenderPerturbCPU()
	{
		LogMsg("--- START CPU PERTURB VERIFY (Double Precision) ---");
		
		TVector2<double> centerFull = mParam.center + mParam.centerLow;
		TVector2<double> ratioFull  = mParam.getRatio();
		
		double rH, rL, iH, iL;
		mRefFullR.convertToDouble(rH);
		BigFloat dR = mRefFullR; dR.sub(BigFloat(rH)); dR.convertToDouble(rL);
		mRefFullI.convertToDouble(iH);
		BigFloat dI = mRefFullI; dI.sub(BigFloat(iH)); dI.convertToDouble(iL);
		TVector2<double> refFull = TVector2<double>(rH, iH); // Use High part for sanity

		float s, c;
		Math::SinCos(mParam.rotationAngle, s, c);
		float bailoutSq = mParam.bailoutValue * mParam.bailoutValue;

		int midY = mParam.viewSize.y / 2;
		int samples[] = { 0, mParam.viewSize.x / 4, mParam.viewSize.x / 2, 3 * mParam.viewSize.x / 4, mParam.viewSize.x - 1 };

		for (int x : samples)
		{
			Vector2 pixelOffset = Vector2((float)x, (float)midY) - 0.5f * (Vector2)mParam.viewSize;
			TVector2<double> vRel = TVector2<double>(pixelOffset).mul(ratioFull);

			TVector2<double> p_offset;
			p_offset.x = c * vRel.x + s * vRel.y;
			p_offset.y = -s * vRel.x + c * vRel.y;

			TVector2<double> c_pixel = centerFull + p_offset;
			TVector2<double> dc_d = (centerFull - refFull) + p_offset;

			// NATIVE DOUBLE ITERATION (The real baseline)
			TVector2<double> z(0, 0);
			int iter = 0;
			for (iter = 0; iter < mParam.maxIteration; ++iter)
			{
				double nx = z.x * z.x - z.y * z.y + c_pixel.x;
				double ny = 2.0 * z.x * z.y + c_pixel.y;
				z.x = nx; z.y = ny;
				if (z.length2() > bailoutSq) break;
			}

			LogMsg("Pixel(%d,%d): c=(%.6f, %.6f) iter=%d %s", 
				x, midY, c_pixel.x, c_pixel.y, iter, (iter < mParam.maxIteration) ? "ESC" : "IN");
		}
		LogMsg("--- END CPU PERTURB VERIFY ---");
	}

	virtual void preShutdownRenderSystem(bool bReInit) override
	{
		mProgMandelbrot = nullptr;
		mColorMap.release();
		mTexture.release();
		mMParamsBuffer.releaseResource();
		mMParamsBufferDouble.releaseResource();
		mMParamsBufferPerturb.releaseResource();
	}

	TStructuredBuffer< MandelbrotParamData > mMParamsBuffer;
	TStructuredBuffer< MandelbrotParamData_Double > mMParamsBufferDouble;
	TStructuredBuffer< MandelbrotPerturbParamData > mMParamsBufferPerturb;
	
	bool bNeedUpdateTexture = false;
	bool bUseNativeDouble = false;
	bool bUseDoubleSim = false;
	bool bUsePerturb = false;

	void updateTexture()
	{
		RHICommandList& commandList = RHICommandList::GetImmediateList();

		if (bUsePerturb)
		{
			// Need to re-calculate orbit if view changed significantly or we forced an update
			// Currently we assume whenever bNeedUpdateTexture is true, we might need a new orbit
			// BUT, orbit calculation is expensive.
			// Let's assume if mReference is NOT valid for current view (e.g. Center moved far), we should update.
			// For now, let's trust the Explicit Calls to updateReferenceOrbit elsewhere,
			// OR add a basic check.
			
			if (mReferenceOrbit.empty() || mRefIter == 0)
			{
				updateReferenceOrbit();
			}
			LogMsg("Updating Perturbation Shader Params. RefIter=%d", mRefIter);
			debugRenderPerturbCPU();


			RHISetComputeShader(commandList, mProgMandelbrotPerturb->getRHI());

			auto data = mMParamsBufferPerturb.lock();
			if (data)
			{
				data->set(mParam, mRefFullR, mRefFullI);
				mMParamsBufferPerturb.unlock();
			}

			RHIResource* RWResource[] = { mTexture };
			RHIResourceTransition(commandList, RWResource, EResourceTransition::UAV);

			mProgMandelbrotPerturb->setParameters(commandList, mParam, *mTexture, *mColorMap, mOrbitBuffer.getRHI(), mMParamsBufferPerturb);
			RHIDispatchCompute(commandList, (mParam.viewSize.x + MandelbrotPerturbShader::SizeX - 1) / MandelbrotPerturbShader::SizeX, (mParam.viewSize.y + MandelbrotPerturbShader::SizeY - 1) / MandelbrotPerturbShader::SizeY, 1);
			mProgMandelbrotPerturb->clearParameters(commandList);

			// Run Analysis pass
			uint32 clearData[2] = { 0, 0 };
			RHIUpdateBuffer(*mAnalysisBuffer.getRHI(), 0, 2, clearData);


			RHISetComputeShader(commandList, mProgAnalysis->getRHI());
			mProgAnalysis->setParameters(commandList, *mTexture, *mAnalysisBuffer.getRHI(), mMParamsBufferPerturb);
			RHIDispatchCompute(commandList, (mParam.viewSize.x + 15) / 16, (mParam.viewSize.y + 15) / 16, 1);
			mProgAnalysis->clearParameters(commandList);

			// Readback result for NEXT frame
			uint32 result[2] = { 0, 0 };
			void* pData = RHILockBuffer(mAnalysisBuffer.getRHI(), ELockAccess::ReadOnly, 0, sizeof(result));
			if (pData)
			{
				std::copy((uint32*)pData, (uint32*)pData + 2, result);
				RHIUnlockBuffer(mAnalysisBuffer.getRHI());
			}

			if (result[0] > 0)
			{
				LogMsg("GPU Analysis: MaxIter found = %d (Current RefIter = %d)", result[0], mRefIter);
			}

			// Threshold for updating reference point to avoid "wild jumping"
			bool bRefImprovement = false;
			if (result[0] > 0)
			{
				// Use cached mRefIter to avoid heavy CPU BigFloat calculations
				if (result[0] > uint32(mRefIter * 1.01f) || (mRefIter < mParam.maxIteration && result[0] > uint32(mRefIter + 10)))
				{
					bRefImprovement = true;
				}
				else if (mRefIter < 500 && result[0] > mRefIter)
				{
				    // Force Update Strategy:
				    // If current reference is very poor (< 500 iters) but GPU found SOMETHING better,
				    // jump to it immediately. This helps break out of "early escape" bad states.
				    bRefImprovement = true;
				}
			}

			if (bRefImprovement)
			{
				int x = result[1] >> 16;
				int y = result[1] & 0xffff;
				BigFloat nextRefR, nextRefI;
				mParam.getBigFloatCoordinates(Vector2((float)x, (float)y), nextRefR, nextRefI);
				mRefIter = result[0];
				LogMsg("Refinement: Jumping to new Deep Point at (%d, %d) iter=%d", x, y, mRefIter);

				// Re-update reference using the refined point
				updateReferenceOrbit(&nextRefR, &nextRefI);
				bNeedUpdateTexture = true; 
			}

			RHIResourceTransition(commandList, RWResource, EResourceTransition::SRV);
		}
		else
		{
			MandelbrotShader::PermutationDomain domain;
			domain.set<MandelbrotShader::UseNativeDouble>(bUseNativeDouble);
			domain.set<MandelbrotShader::UseDoubleSim>(bUseDoubleSim);
			mProgMandelbrot = ShaderManager::Get().getGlobalShaderT< MandelbrotShader >(domain);

			RHISetComputeShader(commandList, mProgMandelbrot->getRHI());
			if (bUseDoubleSim || bUseNativeDouble)
			{
				auto data = mMParamsBufferDouble.lock();
				if (data)
				{
					data->set(mParam);
					mMParamsBufferDouble.unlock();
				}
				SetStructuredUniformBuffer(commandList, *mProgMandelbrot, mMParamsBufferDouble);

			}
			else
			{
				auto data = mMParamsBuffer.lock();
				if (data)
				{
					data->set(mParam);
					mMParamsBuffer.unlock();
				}
				SetStructuredUniformBuffer(commandList, *mProgMandelbrot, mMParamsBuffer);
			}

			RHIResource* RWResource[] = { mTexture };
			RHIResourceTransition(commandList, RWResource, EResourceTransition::UAV);
			mProgMandelbrot->setParameters(commandList, mParam, *mTexture, *mColorMap);
			int nx = (mTexture->getSizeX() + MandelbrotShader::SizeX - 1) / MandelbrotShader::SizeX;
			int ny = (mTexture->getSizeY() + MandelbrotShader::SizeY - 1) / MandelbrotShader::SizeY;
			RHIDispatchCompute(commandList, nx, ny, 1);

			mProgMandelbrot->clearParameters(commandList);
			RHIResourceTransition(commandList, RWResource, EResourceTransition::SRV);
		}
	}

	virtual void onEnd()
	{
		BaseClass::onEnd();
	}

	void restart() 
	{
		mParam.setDefalut();
		mParam.magnification = 1.0;
		mRefFullR = mParam.centerFullR;
		mRefFullI = mParam.centerFullI;
		mRefIter = 0;
		bNeedUpdateTexture = true;
		//updateTexture();
	}

	int mFrameIndex = 0;

	void onUpdate(GameTimeSpan deltaTime) override
	{
		BaseClass::onUpdate(deltaTime);
	}

	void onRender(float dFrame)
	{
		RHIGraphics2D& g = Global::GetRHIGraphics2D();

		Vec2i screenSize = ::Global::GetScreenSize();

		RHICommandList& commandList = RHICommandList::GetImmediateList();

		if (bNeedUpdateTexture)
		{
			GPU_PROFILE("Update Texture");
			bNeedUpdateTexture = false;
			updateTexture();
		}

		Matrix4 projectionMatrix = AdjustProjectionMatrixForRHI(OrthoMatrix(0, screenSize.x, screenSize.y, 0, 1, -1));

		RHISetFrameBuffer(commandList, nullptr);
		RHIClearRenderTargets(commandList, EClearBits::All, &LinearColor(0.2, 0.2, 0.2, 1), 1);
		RHISetViewport(commandList, 0, 0, screenSize.x, screenSize.y);

#if 1
		RHISetRasterizerState(commandList, TStaticRasterizerState< ECullMode::None >::GetRHI() );
		RHISetDepthStencilState(commandList, StaticDepthDisableState::GetRHI());

		Matrix4 projectMatrix = OrthoMatrix(0, screenSize.x, screenSize.y, 0, -1, 1);
		DrawUtility::DrawTexture(commandList, projectionMatrix, *mTexture, Vec2i(0, 0), screenSize);
#endif

#if 1
		g.beginRender();

		mSelectRect.draw(g);
		InlineString<512> str;
		std::string magStr;
		mParam.magnification.getString(magStr);
		// Find exponent part
		size_t expPos = magStr.find('E');
		if (expPos != std::string::npos)
		{
			std::string mantissa = magStr.substr(0, expPos);
			std::string exponent = magStr.substr(expPos + 1);

			// Keep only limited decimals for mantissa
			if (mantissa.length() > 5)
			{
				mantissa = mantissa.substr(0, 5);
			}

			str.format("POS: %f %f  ZOOM: %s E %s", (double)mPosOnMouse.x, (double)mPosOnMouse.y, mantissa.c_str(), exponent.c_str());
		}
		else
		{
			// For non-scientific notation small numbers
			if (magStr.length() > 10)
			{
				magStr = magStr.substr(0, 8) + "..";
			}
			str.format("POS: %f %f  ZOOM: %s", (double)mPosOnMouse.x, (double)mPosOnMouse.y, magStr.c_str());
		}
		g.setTextColor(Color3f(1, 0, 0));
		RenderUtility::SetFont(g, FONT_S10);
		g.drawText(200, 10, str);
		if( mSelectRect.isEnable() )
		{
			str.format("%f %f", mPosRectCenter.x, mPosRectCenter.y);
			g.drawText(200, 10 + 15, str);
		}

		g.endRender();
#endif
	}

	struct ZoomInfo
	{
		Vec2i   centerOffset;
		float   zoomFactor;
		float   angle;
	};

	static void calcZoomInfo(ZoomInfo& outInfo, SelectRectBase const& SRect , MandelbrotParam const& param , bool bZoomIn )
	{
		int w = param.viewSize.x;
		int h = param.viewSize.y;

	 	Vec2i pos = SRect.getCenterPos();
		outInfo.centerOffset = SRect.getCenterPos() - param.viewSize / 2;

		Vector2 mag = Vector2(param.viewSize).div(SRect.getSize());
		float magh = float(h) / SRect.getHeight();
		float magw = float(w) / SRect.getWidth();
		float zoom = std::max(magh, magw);
		if( bZoomIn )
			zoom = 1 / zoom;
		outInfo.zoomFactor = zoom;

		float angle = -SRect.getRotationAngle();
		outInfo.angle = WrapZeroTo2PI(angle);
	}

	Vector2 mPosRectCenter;
	MsgReply onMouse(MouseMsg const& msg)
	{
		mPosOnMouse = mParam.getCoordinates(msg.getPos());

		if( !mSelectRect.procMouseMsg(msg) )
			return MsgReply::Handled();

		if ( mSelectRect.isEnable() )
		{
			ZoomInfo info;
			calcZoomInfo(info, mSelectRect, mParam, msg.onLeftDClick());
			mPosRectCenter = mParam.getCoordinates(Vector2(info.centerOffset)+ 0.5 * Vector2(mParam.viewSize));
			if( msg.onLeftDClick() || msg.onRightDClick() )
			{

				mSelectRect.enable(false);

				mParam.zoomInPos(info.centerOffset , info.zoomFactor, info.angle);
				
				// Force orbit update by invalidating current one
				mReferenceOrbit.clear();
				mRefIter = 0;
				
				bNeedUpdateTexture = true;
				//updateTexture();
				return MsgReply::Handled();
			}
		}

		return BaseClass::onMouse(msg);
	}

	Vector2 mPosOnMouse;

	MsgReply onKey(KeyMsg const& msg)
	{
		if( msg.isDown() )
		{
			switch (msg.getCode())
			{
			case EKeyCode::R: restart(); break;
			}
		}
		return BaseClass::onKey(msg);
	}

	virtual bool onWidgetEvent(int event, int id, GWidget* ui) override
	{
		switch( id )
		{
		default:
			break;
		}

		return BaseClass::onWidgetEvent(event, id, ui);
	}



protected:
};


REGISTER_STAGE_ENTRY("Fractial Test", FractialTestStage, EExecGroup::FeatureDev, "Render|Math");