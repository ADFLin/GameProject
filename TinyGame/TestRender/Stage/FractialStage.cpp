#include "Stage/TestStageHeader.h"
#include "CurveBuilder/ColorMap.h"

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
#include <functional>

#include "TinyCore/GameWidget.h"
#include "ProfileSystem.h"
#include "Async/AsyncWork.h"
#include "SystemPlatform.h"
#include "Random.h"
#include <atomic>


using namespace Render;

typedef TBigFloat<16, 1> BigFloat;

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

	struct BigRatio { BigFloat x, y; };
	BigRatio getBigRatio() const
	{
		BigRatio res;
		res.x.setValue(0.005 * stretch);
		res.x.div(magnification);
		res.y.setValue(0.005);
		res.y.div(magnification);
		return res;
	}

	TVector2<double> getRatio() const
	{
		BigRatio ratio = getBigRatio();
		double rx, ry;
		ratio.x.convertToDouble(rx);
		ratio.y.convertToDouble(ry);
		return TVector2<double>(rx, ry);
	}

	void  zoomInPos(Vec2i const& centerOffset, float zoomFactor, float angle)
	{
		Vector2 targetPixel = Vector2(centerOffset) + 0.5f * Vector2(viewSize);
		getBigFloatCoordinates(targetPixel, centerFullR, centerFullI);

		BigFloat scale(1.0 / (double)zoomFactor);
		magnification.mul(scale);

		setRotationAngle(rotationAngle + angle);
		syncCenterPart();
	}

	void getBigFloatCoordinates(Vector2 pixelPos, BigFloat& outR, BigFloat& outI) const
	{
		BigRatio ratio = getBigRatio();

		// len = (pixel - 0.5 * size) * ratio
		BigFloat dx, dy;
		dx.setValue((double)pixelPos.x - 0.5 * viewSize.x);
		dx.mul(ratio.x);
		dy.setValue((double)pixelPos.y - 0.5 * viewSize.y);
		dy.mul(ratio.y);

		float s, c;
		Math::SinCos(rotationAngle, s, c);

		// dispX = c*dx + s*dy, dispY = -s*dx + c*dy
		BigFloat dispX = dx; dispX.mul(BigFloat(c));
		BigFloat tempY = dy; tempY.mul(BigFloat(s));
		dispX.add(tempY);

		BigFloat dispY = dy; dispY.mul(BigFloat(c));
		BigFloat tempX = dx; tempX.mul(BigFloat(s));
		dispY.sub(tempX);

		outR = centerFullR; outR.add(dispX);
		outI = centerFullI; outI.add(dispY);
	}

	void getPixelPos(BigFloat const& posR, BigFloat const& posI, Vector2& outPixel) const
	{
		BigFloat dR = posR; dR.sub(centerFullR);
		BigFloat dI = posI; dI.sub(centerFullI);

		float s, c;
		Math::SinCos(rotationAngle, s, c);

		// Inverse rotation: dr = c*dR - s*dI, di = s*dR + c*dI
		BigFloat dr = dR; dr.mul(BigFloat(c));
		BigFloat tempI = dI; tempI.mul(BigFloat(s));
		dr.sub(tempI);

		BigFloat di = dR; di.mul(BigFloat(s));
		BigFloat tempR = dI; tempR.mul(BigFloat(c));
		di.add(tempR);

		BigRatio ratio = getBigRatio();
		dr.div(ratio.x);
		di.div(ratio.y);

		double outX, outY;
		dr.convertToDouble(outX);
		di.convertToDouble(outY);

		outPixel.x = (float)(outX + 0.5 * viewSize.x);
		outPixel.y = (float)(outY + 0.5 * viewSize.y);
	}

	void setRotationAngle(float angle)
	{
		rotationAngle = WrapZeroTo2PI(angle);
	}

	void syncCenterPart()
	{
		double rH, iH;
		centerFullR.convertToDouble(rH);
		centerFullI.convertToDouble(iH);
		center.x = rH;
		center.y = iH;

		BigFloat dR = centerFullR; dR.sub(BigFloat(rH));
		BigFloat dI = centerFullI; dI.sub(BigFloat(iH));
		dR.convertToDouble(centerLow.x);
		dI.convertToDouble(centerLow.y);
	}

	TVector2<double> getCoordinates(Vector2 pixelPos) const
	{
		// Use BigFloat for intermediate result even if we return double
		BigFloat r, i;
		getBigFloatCoordinates(pixelPos, r, i);
		double dr, di;
		r.convertToDouble(dr);
		i.convertToDouble(di);
		return TVector2<double>(dr, di);
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
		bailoutValue = 8.0;
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
		ColorMapParam = Vector4(1, 0, (float)(param.bailoutValue * param.bailoutValue), 0);
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
	TVector2<double> CenterToRefDelta_Rel; // Slot 9: Delta High
	TVector2<double> CenterToRefDelta_Rel_Low; // Slot 10: Delta Low
	float  ZoomScale;  // Slot 11: offset 176
	float  MaxIter_f;  // offset 180
	float  PaddingScale[2]; // Explicit padding to reach 192 (16-byte alignment)
	TVector2<double> InvNormalizationScale; // Slot 12: offset 192
	int    PaddingAlignment[12];


	void set(MandelbrotParam const& param, BigFloat const& refR, BigFloat const& refI)
	{
		auto GetHighLow = [](BigFloat const& bf, double& h, double& l)
		{
			bf.convertToDouble(h);
			BigFloat temp = bf; temp.sub(BigFloat(h));
			temp.convertToDouble(l);
		};

		// Use BigFloat precision for center coordinates
		GetHighLow(param.centerFullR, CoordParam.x, CoordParamLow.x);
		GetHighLow(param.centerFullI, CoordParam.y, CoordParamLow.y);

		auto DSDiv = [&](double a, double b, double bl, double& outH, double& outL) {
			outH = a / b;
			outL = (a - outH * b - outH * bl) / b;
		};

		// High-precision Ratio Calculation using BigFloat
		BigFloat bfRatioX(0.005 * param.stretch);
		bfRatioX.div(param.magnification);
		GetHighLow(bfRatioX, CoordParam2.x, CoordParam2Low.x);

		BigFloat bfRatioY(0.005);
		bfRatioY.div(param.magnification);
		GetHighLow(bfRatioY, CoordParam2.y, CoordParam2Low.y);

		float s, c;
		Math::SinCos(param.rotationAngle, s, c);
		Rotate.x = c;
		Rotate.y = s;

		ViewSize = param.viewSize;
		MaxIteration = param.maxIteration;
		Padding = 0;

		double refRh, refRl, refIh, refIl;
		refR.convertToDouble(refRh);
		RefCoord.x = refRh;
		BigFloat dRh = refR; dRh.sub(BigFloat(refRh)); dRh.convertToDouble(refRl);
		RefCoordLow.x = refRl;

		refI.convertToDouble(refIh);
		RefCoord.y = refIh;
		BigFloat dIh = refI; dIh.sub(BigFloat(refIh)); dIh.convertToDouble(refIl);
		RefCoordLow.y = refIl;

		// CALCULATE ABSOLUTE DELTA (High-precision split)
		BigFloat dx = param.centerFullR; dx.sub(refR);
		BigFloat dy = param.centerFullI; dy.sub(refI);

		GetHighLow(dx, CenterToRefDelta_Rel.x, CenterToRefDelta_Rel_Low.x);
		GetHighLow(dy, CenterToRefDelta_Rel.y, CenterToRefDelta_Rel_Low.y);

		ZoomScale = 0; // Fallback for huge mag
		{
			BigFloat scale(1.0);
			scale.div(param.magnification);
			double dScale;
			scale.convertToDouble(dScale);
			ZoomScale = (float)dScale;
		}
		MaxIter_f = (float)param.maxIteration;

		// Calculate high-precision 1/R for GPU
		// R = (1/mag) * (viewSize.x * 0.5) => 1/R = mag * 2.0 / viewSize.x
		BigFloat bfInvR = param.magnification;
		bfInvR.mul(BigFloat(2.0 / (double)param.viewSize.x));
		GetHighLow(bfInvR, InvNormalizationScale.x, InvNormalizationScale.y);


		ColorMapParam = Vector4(1, 0, (float)(param.bailoutValue * param.bailoutValue), 0);

		// CPU VERIFY:
		TVector2<double> cpuCoord = param.getCoordinates(Vector2(param.viewSize) * 0.5f);
		//LogMsg("CPU Verify (Center Pixel): (%f, %f)", cpuCoord.x, cpuCoord.y);
		//LogMsg("Shader Params: Center=(%f, %f), Ratio=(%f, %f), Rot=(%c=%f, s=%f)", 
		//	CoordParam.x + CoordParamLow.x, CoordParam.y + CoordParamLow.y,
		//	CoordParam2.x + CoordParam2Low.x, CoordParam2.y + CoordParam2Low.y,
		//	'c', Rotate.x, Rotate.y);
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

IMPLEMENT_SHADER(MandelbrotShader, EShader::Compute, SHADER_ENTRY(MainCS));


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
		setRWTexture(commandList, SHADER_MEMBER_PARAM(ColorRWTexture), texture, 0, EAccessOp::ReadAndWrite);
		setStorageBuffer(commandList, SHADER_MEMBER_PARAM(ResultBuffer), resultBuffer, EAccessOp::ReadAndWrite);
		SetStructuredUniformBuffer(commandList, *this, paramsBuffer);
	}

	void clearParameters(RHICommandList& commandList)
	{
		clearRWTexture(commandList, mParamColorRWTexture);
		clearBuffer(commandList, mParamResultBuffer, EAccessOp::ReadAndWrite);
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
		BIND_SHADER_PARAM(parameterMap, ColorRWTexture);
		BIND_TEXTURE_PARAM(parameterMap, ColorMapTexture);
		BIND_SHADER_PARAM(parameterMap, OrbitBuffer);
	}


	void setParameters(RHICommandList& commandList, MandelbrotParam const& param, RHITexture2D& colorTexture, RHITexture1D& colorMapTexture, RHIResource* orbitBuffer, TStructuredBuffer< MandelbrotPerturbParamData >& paramsBuffer)
	{
		setRWTexture(commandList, SHADER_MEMBER_PARAM(ColorRWTexture), colorTexture, 0, EAccessOp::WriteOnly);
		auto& samplerState = TStaticSamplerState< ESampler::Bilinear, ESampler::Wrap >::GetRHI();
		SET_SHADER_TEXTURE_AND_SAMPLER(commandList, *this, ColorMapTexture, colorMapTexture, samplerState);

		setStorageBuffer(commandList, SHADER_MEMBER_PARAM(OrbitBuffer), *static_cast<RHIBuffer*>(orbitBuffer), EAccessOp::ReadOnly);
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


struct FractalBookmark
{
	std::string name;
	std::string centerR;
	std::string centerI;
	std::string magnification;
	float rotation;
	Render::RHITexture2DRef thumbnail;
};

class PurpleButton : public GButton
{
public:
	using GButton::GButton;
	void onRender() override
	{
		Vec2i pos = getWorldPos();
		Vec2i size = getSize();
		IGraphics2D& g = Global::GetIGraphics2D();

		RenderUtility::SetPen(g, EColor::Null);
		Color3ub baseColor = Color3ub(45, 30, 85); // Deep Purple Base
		Color3ub glowColor = Color3ub(120, 80, 220); // Bright Purple Accent

		if (mState == BS_PRESS)
			g.setBrush(Color3ub(Math::Min(255, (int)baseColor.r + 20), Math::Min(255, (int)baseColor.g + 20), Math::Min(255, (int)baseColor.b + 20)));
		else if (mState == BS_HIGHLIGHT)
			g.setBrush(Color3ub(Math::Min(255, (int)baseColor.r + 10), Math::Min(255, (int)baseColor.g + 10), Math::Min(255, (int)baseColor.b + 10)));
		else
			g.setBrush(baseColor);

		g.drawRoundRect(pos, size, Vec2i(6, 6));

		// Draw subtle border
		RenderUtility::SetBrush(g, EColor::Null);
		g.setPen(glowColor, 1);
		g.drawRoundRect(pos, size, Vec2i(6, 6));

		g.setTextColor(Color3ub(220, 200, 255));
		RenderUtility::SetFont(g, FONT_S10);
		g.drawText(pos, size, mTitle.c_str(), EHorizontalAlign::Center);
	}
};

class BookmarkItemUI : public GPanel
{
public:
	BookmarkItemUI(FractalBookmark const& data, Vec2i const& pos, Vec2i const& size, GWidget* parent)
		:GPanel(UI_ANY, pos, size, parent)
		, mData(data)
	{
		mState = BS_NORMAL;
		mbSelected = false;
		// Dedicated Apply Button
		int btnHeight = 25; // Compact button to clear text
		int btnWidth = 50;
		mApplyBtn = new PurpleButton(UI_ANY, Vec2i(size.x - btnWidth - 10, 5), Vec2i(btnWidth, btnHeight), this);
		mApplyBtn->setTitle("GO");
	}

	void onResize(Vec2i const& size) override
	{
		if (mApplyBtn)
		{
			int btnHeight = 25;
			int btnWidth = 50;
			mApplyBtn->setPos(Vec2i(size.x - btnWidth - 10, 5));
		}
	}

	void setSelected(bool bSelected) { mbSelected = bSelected; }

	void onMouse(bool beInside) override
	{
		mState = beInside ? BS_HIGHLIGHT : BS_NORMAL;
	}

	MsgReply onMouseMsg(MouseMsg const& msg) override
	{
		if (msg.onLeftDown())
		{
			// Notify parent list to select this item
			auto* pList = static_cast<GWidgetListT<FractalBookmark, BookmarkItemUI>*>(getParent());
			if (pList)
			{
				for (int i = 0; i < (int)pList->getItemNum(); ++i)
				{
					if (pList->getItemWidget(i) == this)
					{
						pList->setSelection(i);
						break;
					}
				}
			}
		}
		return GPanel::onMouseMsg(msg);
	}

	void onRender() override
	{
		IGraphics2D& g = Global::GetIGraphics2D();
		Vec2i pos = getWorldPos();
		Vec2i size = getSize();

		// Background based on state
		RenderUtility::SetPen(g, EColor::Null);
		if (mbSelected)
			g.setBrush(Color3ub(55, 45, 80)); // Selected Purple
		else if (mState == BS_HIGHLIGHT)
			g.setBrush(Color3ub(45, 45, 55));
		else
			g.setBrush(Color3ub(35, 35, 45));

		g.drawRoundRect(pos + Vec2i(5, 2), size - Vec2i(10, 4), Vec2i(8, 8));

		// Accent border on hover or selection
		if (mbSelected || mState == BS_HIGHLIGHT)
		{
			RenderUtility::SetBrush(g, EColor::Null);
			if (mbSelected)
				g.setPen(Color3ub(160, 110, 255), 2); // Brighter thicker border for selection
			else
				g.setPen(Color3ub(120, 80, 220), 1);
			g.drawRoundRect(pos + Vec2i(5, 2), size - Vec2i(10, 4), Vec2i(8, 8));
		}

		// Thumbnail Square
		g.setBrush(Color3ub(15, 15, 20));
		g.setPen(Color3ub(60, 60, 80), 1);
		g.drawRoundRect(pos + Vec2i(15, 15), Vec2i(50, 50), Vec2i(4, 4));

		if (mData.thumbnail)
		{
			if (g.isUseRHI())
			{
				g.setBrush(Color3ub::White());
				RHIGraphics2D& gRHI = g.getImpl<RHIGraphics2D>();
				gRHI.drawTexture(*mData.thumbnail, pos + Vec2i(15, 15), Vec2i(50, 50));
			}
		}
		else
		{
			g.setBrush(Color3ub(120, 80, 220));
			g.drawCircle(pos + Vec2i(15 + 25, 15 + 25), 8);
		}

		// Text Rendering
		int textStartX = 75;
		int buttonEdgeX = size.x - 70;
		int fullWidth = size.x - textStartX - 10;
		int titleWidth = buttonEdgeX - textStartX; // 50px Button + 10px Margin + 10px Padding

		if (titleWidth > 0)
		{
			// Title
			g.setTextColor(Color3ub(255, 255, 255));
			RenderUtility::SetFont(g, FONT_S10);
			g.drawText(pos + Vec2i(textStartX, 6), Vec2i(titleWidth, 20), mData.name.c_str(), EHorizontalAlign::Left);

			// Details Row 1 - Position
			g.setTextColor(Color3ub(180, 180, 190));
			RenderUtility::SetFont(g, FONT_S8);
			auto Shorten = [](std::string const& s) { return s.length() > 7 ? s.substr(0, 7) : s; };
			std::string posInfo = "X: " + Shorten(mData.centerR) + "  Y: " + Shorten(mData.centerI);
			g.drawText(pos + Vec2i(textStartX, 35), Vec2i(fullWidth, 14), posInfo.c_str(), EHorizontalAlign::Left);

			// Details Row 2 - Rotation & Magnification
			g.setTextColor(Color3ub(140, 140, 160));
			double magVal = std::atof(mData.magnification.c_str());
			char buf[64];
			snprintf(buf, sizeof(buf), "R: %.1f  M: %.3g", mData.rotation, magVal);
			g.drawText(pos + Vec2i(textStartX, 56), Vec2i(fullWidth, 14), buf, EHorizontalAlign::Left);
		}
	}

	void setData(FractalBookmark const& data) { mData = data; }
	FractalBookmark const& getData() const { return mData; }
	FractalBookmark& getDataRef() { return mData; }
	GButton* getApplyBtn() { return mApplyBtn; }

private:
	FractalBookmark mData;
	ButtonState mState;
	bool mbSelected;
	PurpleButton* mApplyBtn;
};

struct GradientKey
{
	float time;
	Color3f color;
	bool operator<(GradientKey const& other) const { return time < other.time; }
};

class GradientEditorPanel : public GFrame
{
public:
	std::function<void(TArray<GradientKey> const&, int)> onGradientChanged;

	GradientEditorPanel(int id, Vec2i const& pos, Vec2i const& size, GWidget* parent)
		:GFrame(id, pos, size, parent)
	{
		setRenderType(GPanel::eRectType);
		setAlpha(0.95f);
		// setTitle("Color Gradient"); // Not supported

		// Close Button
		GButton* closeBtn = new PurpleButton(UI_ANY, Vec2i(size.x - 25, 5), Vec2i(20, 20), this);
		closeInfoBtn = closeBtn;
		closeBtn->setTitle("X");
		closeBtn->onEvent = [this](int, GWidget*) { show(false); return false; };

		int yCtx = 45 + GradientHeight + HandleAreaHeight + 5;
		int margin = 10;
		int labelWidth = 30;
		int valWidth = 40;
		int sliderWidth = size.x - 2 * margin - labelWidth - valWidth - 10;

		// Generic Slider Creator
		auto CreateSlider = [&](int min, int max, std::function<void(float)> onChange) -> GSlider*
		{
			// Slider (Always 0-1000 range for consistent thumb size)
			// Position offset by labelWidth
			GSlider* slider = new GSlider(UI_ANY, Vec2i(margin + labelWidth + 5, yCtx), sliderWidth, true, this);
			slider->setRange(0, 1000);

			slider->onEvent = [onChange, slider](int evt, GWidget*) {
				if (evt == EVT_SLIDER_CHANGE)
				{
					onChange((float)slider->getValue());
				}
				return true;
			};
			yCtx += 25;
			return slider;
		};

		mRotSlider = CreateSlider(0, 1024, [this](float val) {
			int rotVal = (int)(val * 1024 / 1000.0f);
			mRotation = rotVal;
			notifyChange();
		});

		yCtx += 10; // Gap between Global and Local properties

		mPosSlider = CreateSlider(0, 1000, [this](float val) {
			float timeVal = val / 1000.0f;
			if (mSelection != -1) {
				mKeys[mSelection].time = timeVal;
				sortKeysAndNotify();
			}
		});

		mRSlider = CreateSlider(0, 255, [this](float val) { updateColorFromSliders(); });
		mGSlider = CreateSlider(0, 255, [this](float val) { updateColorFromSliders(); });
		mBSlider = CreateSlider(0, 255, [this](float val) { updateColorFromSliders(); });

		// Initial Data
		mKeys.push_back({ 0.0f, Color3f(0,0,0.2) });
		mKeys.push_back({ 0.2f, Color3f(0,0,1) });
		mKeys.push_back({ 0.4f, Color3f(0,1,1) });
		mKeys.push_back({ 0.6f, Color3f(1,1,1) });
		mKeys.push_back({ 0.8f, Color3f(1,0.5,0) });
		mKeys.push_back({ 1.0f, Color3f(0,0,0) });
	}

	void onRender() override
	{
		GFrame::onRender(); // Draw background/title

		IGraphics2D& g = Global::GetIGraphics2D();
		Vec2i pos = getWorldPos();
		Vec2i size = getSize();

		// Header Bar
		RenderUtility::SetPen(g, EColor::Null);
		g.setBrush(Color3ub(35, 35, 45));
		g.drawRect(pos, Vec2i(size.x, 35));
		g.setBrush(Color3ub(120, 80, 220));
		g.drawRect(pos + Vec2i(0, 33), Vec2i(size.x, 2));

		g.setTextColor(Color3ub(220, 220, 240));
		RenderUtility::SetFont(g, FONT_S12);
		g.drawText(pos + Vec2i(15, 8), "COLOR EDITOR");

		int gradY = 45;
		int handleY = gradY + GradientHeight;

		// 1. Draw Gradient
		// Simple linear interpolation drawing
		int contentW = size.x - 20;
		int startX = pos.x + 10;

		// Sort keys just in case (though we keep them sorted)
		TArray<GradientKey> sorted = mKeys;
		std::sort(sorted.begin(), sorted.end());

		if (sorted.empty()) return;


		if (mIsGradientDirty || !mGradientTex.isValid())
		{
			updateGradientTexture();
		}

		if (mGradientTex.isValid() && g.isUseRHI())
		{
			RHIGraphics2D& gRHI = g.getImpl<RHIGraphics2D>();
			gRHI.setBrush(Color3f(1, 1, 1));
			gRHI.drawTexture(*mGradientTex, Vec2i(startX, pos.y + gradY), Vec2i(contentW, GradientHeight));
		}
		else
		{
			// Fallback if RHI not available (unlikely)
			g.setBrush(Color3ub(100, 100, 100));
			g.drawRect(Vec2i(startX, pos.y + gradY), Vec2i(contentW, GradientHeight));
		}

		// 2. Draw Handles
		float rotOffset = (float)mRotation / 1024.0f;
		for (int i = 0; i < (int)mKeys.size(); ++i)
		{
			float vTime = mKeys[i].time - rotOffset;
			while (vTime < 0) vTime += 1.0f;
			while (vTime >= 1.0f) vTime -= 1.0f;

			int hx = startX + (int)(vTime * contentW);
			int hy = pos.y + handleY;

			Vec2i tri[3] = { Vec2i(hx, hy), Vec2i(hx - 5, hy + 10), Vec2i(hx + 5, hy + 10) };
			Vector2 vTri[3] = { Vector2(tri[0]), Vector2(tri[1]), Vector2(tri[2]) };

			g.setPen(Color3ub(200, 200, 220));
			g.setBrush(i == mSelection ? Color3ub(255, 255, 0) : Color3ub(50, 50, 50));
			g.drawPolygon(vTri, 3);
		}

		// 3. Draw Labels and Values Manually
		auto DrawSliderInfo = [&](GSlider* slider, char const* label, char const* valStr)
		{
			RenderUtility::SetFont(g, FONT_S8);
			Vec2i sPos = slider->getWorldPos();
			Vec2i sSize = slider->getSize();
			int labelW = 40;
			int valW = 50;

			// Label (Right aligned)
			g.setTextColor(Color3ub(200, 200, 220));
			g.drawText(Vec2i(sPos.x - labelW - 5, sPos.y), Vec2i(labelW, sSize.y), label, EHorizontalAlign::Right);

			// Value (Left aligned)
			g.drawText(Vec2i(sPos.x + sSize.x + 5, sPos.y), Vec2i(valW, sSize.y), valStr, EHorizontalAlign::Left);
		};

		DrawSliderInfo(mRotSlider, "Rot", std::to_string(mRotation).c_str());
		if (mSelection != -1 && mSelection < mKeys.size())
		{
			DrawSliderInfo(mPosSlider, "Pos", FStringConv::From(mKeys[mSelection].time));
			DrawSliderInfo(mRSlider, "R", std::to_string((int)(mKeys[mSelection].color.r * 255)).c_str());
			DrawSliderInfo(mGSlider, "G", std::to_string((int)(mKeys[mSelection].color.g * 255)).c_str());
			DrawSliderInfo(mBSlider, "B", std::to_string((int)(mKeys[mSelection].color.b * 255)).c_str());
		}
		else
		{
			// Draw disabled state or last values?
			DrawSliderInfo(mPosSlider, "Pos", "-");
			DrawSliderInfo(mRSlider, "R", "-");
			DrawSliderInfo(mGSlider, "G", "-");
			DrawSliderInfo(mBSlider, "B", "-");
		}
	}

	MsgReply onMouseMsg(MouseMsg const& msg) override
	{
		Vec2i localPos = msg.getPos() - getWorldPos();
		int gradY = 45;
		int handleY = gradY + GradientHeight;
		int contentW = getSize().x - 20;
		int startX = 10;
		float rotOffset = (float)mRotation / 1024.0f;

		if (msg.onLeftDown())
		{
			// Check Handle Hit (Based on visual position)
			int bestIdx = -1;
			int hitDist = 8;
			for (int i = 0; i < (int)mKeys.size(); ++i)
			{
				float vTime = mKeys[i].time - rotOffset;
				while (vTime < 0) vTime += 1.0f;
				while (vTime >= 1.0f) vTime -= 1.0f;

				int hx = startX + (int)(vTime * contentW);
				int hy = handleY + 5;
				if (std::abs(localPos.x - hx) < hitDist && std::abs(localPos.y - hy) < 10)
				{
					bestIdx = i;
					break;
				}
			}

			if (bestIdx != -1)
			{
				mSelection = bestIdx;
				mbDragging = true;
				updateUIFromSelection();
				return MsgReply::Handled();
			}
			else if (localPos.y > gradY && localPos.y < handleY + 15 && localPos.x >= startX && localPos.x <= startX + contentW)
			{
				// Add new key
				if (msg.onLeftDClick())
				{
					float t = (float)(localPos.x - startX) / contentW + rotOffset;
					while (t >= 1.0f) t -= 1.0f;
					t = Math::Clamp(t, 0.0f, 1.0f);

					Color3f col = SampleColor(mKeys, t);
					mKeys.push_back({ t, col });
					sortKeysAndNotify();
					updateUIFromSelection();
					return MsgReply::Handled();
				}
			}
		}
		else if (msg.onLeftUp())
		{
			mbDragging = false;
		}
		else if (msg.isLeftDown() && msg.isDraging() && mbDragging && mSelection != -1)
		{
			float t = (float)(localPos.x - startX) / contentW + rotOffset;
			while (t < 0) t += 1.0f;
			while (t >= 1.0f) t -= 1.0f;
			mKeys[mSelection].time = Math::Clamp(t, 0.0f, 1.0f);

			sortKeysAndNotify();
			updateUIFromSelection();
			return MsgReply::Handled();
		}
		else if (msg.onRightDown())
		{
			// Delete handle on right click
			if (mKeys.size() > 2)
			{
				for (int i = 0; i < (int)mKeys.size(); ++i)
				{
					float vTime = mKeys[i].time - rotOffset;
					while (vTime < 0) vTime += 1.0f;
					while (vTime >= 1.0f) vTime -= 1.0f;
					int hx = startX + (int)(vTime * contentW);
					if (std::abs(localPos.x - hx) < 8 && std::abs(localPos.y - (handleY + 5)) < 10)
					{
						mKeys.erase(mKeys.begin() + i);
						mSelection = -1;
						notifyChange();
						return MsgReply::Handled();
					}
				}
			}
		}

		if (GFrame::onMouseMsg(msg).isHandled()) return MsgReply::Handled();
		return MsgReply::Unhandled();
	}

private:
	Color3f SampleColor(TArray<GradientKey>& keys, float t)
	{
		if (keys.empty()) return Color3f(0, 0, 0);
		if (keys.size() == 1) return keys[0].color;

		// Assuming keys are sorted
		// 1. Regular intervals
		for (size_t i = 0; i < keys.size() - 1; ++i)
		{
			if (t >= keys[i].time && t < keys[i + 1].time)
			{
				float alpha = (t - keys[i].time) / (keys[i + 1].time - keys[i].time);
				return Math::Lerp(keys[i].color, keys[i + 1].color, alpha);
			}
		}

		// 2. Wrap-around gap (between back and front)
		float dist = (1.0f - keys.back().time) + keys.front().time;
		if (dist > 1e-6f)
		{
			float alpha;
			if (t >= keys.back().time)
				alpha = (t - keys.back().time) / dist;
			else // t < keys.front().time
				alpha = (t + (1.0f - keys.back().time)) / dist;

			return Math::Lerp(keys.back().color, keys.front().color, alpha);
		}

		return keys.back().color;
	}

	void updateUIFromSelection()
	{
		mRotSlider->setValue(mRotation * 1000 / 1024);

		if (mSelection != -1 && mSelection < mKeys.size())
		{
			mPosSlider->setValue(mKeys[mSelection].time * 1000);

			// RGB sliders use 0-1000 scale now, so map 0-1 float directly to 0-1000
			int r = mKeys[mSelection].color.r * 1000;
			mRSlider->setValue(r);

			int g = mKeys[mSelection].color.g * 1000;
			mGSlider->setValue(g);

			int b = mKeys[mSelection].color.b * 1000;
			mBSlider->setValue(b);
		}
	}

	void updateColorFromSliders()
	{
		if (mSelection != -1)
		{
			// Map 0-1000 slider to 0-1 float
			mKeys[mSelection].color = Color3f(
				mRSlider->getValue() / 1000.0f,
				mGSlider->getValue() / 1000.0f,
				mBSlider->getValue() / 1000.0f
			);

			notifyChange();
		}
	}

	void sortKeysAndNotify()
	{
		GradientKey selectedKey;
		bool bHasSelection = (mSelection != -1 && mSelection < (int)mKeys.size());
		if (bHasSelection)
		{
			selectedKey = mKeys[mSelection];
		}

		std::sort(mKeys.begin(), mKeys.end());

		if (bHasSelection)
		{
			mSelection = -1;
			for (int i = 0; i < (int)mKeys.size(); ++i)
			{
				if (std::abs(mKeys[i].time - selectedKey.time) < 1e-5f &&
					mKeys[i].color.r == selectedKey.color.r &&
					mKeys[i].color.g == selectedKey.color.g &&
					mKeys[i].color.b == selectedKey.color.b)
				{
					mSelection = i;
					break;
				}
			}
		}
		notifyChange();
	}

	void notifyChange()
	{
		mIsGradientDirty = true;
		if (onGradientChanged) onGradientChanged(mKeys, mRotation);
	}

	void updateGradientTexture()
	{
		int const TexWidth = 256;
		TArray<Color4ub> colors;
		colors.resize(TexWidth);

		TArray<GradientKey> sorted = mKeys;
		std::sort(sorted.begin(), sorted.end());

		float rotOffset = (float)mRotation / 1024.0f;

		for (int i = 0; i < TexWidth; ++i)
		{
			float t = (float)i / (TexWidth - 1);
			t += rotOffset;
			t -= floor(t); // Wrap 0-1

			Color3f lColor = SampleColor(sorted, t);
			Color3ub c(lColor);
			colors[i] = Color4ub(c.r, c.g, c.b, 255);
		}

		mGradientTex = RHICreateTexture2D(TextureDesc::Type2D(ETexture::RGBA8, TexWidth, 1).Flags(TCF_DefalutValue), colors.data());
		mIsGradientDirty = false;
	}

	TArray<GradientKey> mKeys;
	int mSelection = -1;
	int mRotation = 0;
	bool mbDragging = false;

	Render::RHITexture2DRef mGradientTex;
	bool mIsGradientDirty = true;

	GSlider* mRotSlider;
	GSlider* mPosSlider;
	GSlider* mRSlider;
	GSlider* mGSlider;
	GSlider* mBSlider;
	GButton* closeInfoBtn;

	static const int GradientHeight = 40;
	static const int HandleAreaHeight = 20;
};

class FractalBookmarkPanel : public GFrame
{
public:
	TArray<FractalBookmark> mBookmarks;
	std::function<FractalBookmark()> onFetchCurrentBookmark;
	std::function<void(FractalBookmark const&)> onApplyBookmark;
	std::function<Render::RHITexture2DRef()> onCaptureThumbnail;

	GWidgetListT<FractalBookmark, BookmarkItemUI>* mUIBookmarkList;
	GTextCtrl* mUIBookmarkName;

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
			if (!NextToken(bm.name)) continue;
			if (!NextToken(bm.centerR)) continue;
			if (!NextToken(bm.centerI)) continue;
			if (!NextToken(bm.magnification)) continue;

			// Rotation might be last (NextToken returns false but fills token)
			NextToken(token);
			if (!token.empty())
				bm.rotation = (float)atof(token.c_str());

			mBookmarks.push_back(bm);
			auto* item = mUIBookmarkList->addItem(bm);
			item->getApplyBtn()->onEvent = [this, item](int event, GWidget* widget)
			{
				if (event == EVT_BUTTON_CLICK)
				{
					onApplyBookmark(item->getData());
					if (onCaptureThumbnail)
					{
						auto& bmRef = item->getDataRef();
						bmRef.thumbnail = onCaptureThumbnail();
					}
				}
				return false;
			};
		}
	}

	FractalBookmarkPanel(int id, Vec2i const& pos, Vec2i const& size, GWidget* parent)
		:GFrame(id, pos, size, parent)
	{
		setRenderType(GPanel::eRectType);
		setAlpha(0.95f);

		show(false); // Hidden by default

		int const TitleHeight = 35;
		int const Margin = 10;
		int const ButtonHeight = 30;
		int const SaveButtonWidth = 80;

		// Close Button
		GButton* closeInfo = new PurpleButton(UI_ANY, Vec2i(size.x - 25, (TitleHeight - 20) / 2), Vec2i(20, 20), this);
		closeInfo->setTitle("X");
		closeInfo->onEvent = [this](int, GWidget*) { show(false); return false; };

		int yCtx = TitleHeight + 10;

		// Input & Save Row
		mUIBookmarkName = new GTextCtrl(UI_ANY, Vec2i(Margin, yCtx), size.x - 2 * Margin - SaveButtonWidth - 5, this);
		mUIBookmarkName->setSize(Vec2i(size.x - 2 * Margin - SaveButtonWidth - 5, ButtonHeight));

		PurpleButton* addBtn = new PurpleButton(UI_ANY, Vec2i(size.x - Margin - SaveButtonWidth, yCtx), Vec2i(SaveButtonWidth, ButtonHeight), this);
		addBtn->setTitle("SAVE");
		addBtn->onEvent = [this](int id, GWidget* widget)
		{
			if (onFetchCurrentBookmark)
			{
				FractalBookmark bm = onFetchCurrentBookmark();
				std::string inputName = mUIBookmarkName->getValue();
				if (!inputName.empty())
					bm.name = inputName;
				else
				{
					char nameBuf[64];
					sprintf(nameBuf, "%.2e", 1.0 / atof(bm.magnification.c_str()));
					bm.name = nameBuf;
				}

				mBookmarks.push_back(bm);
				auto* item = mUIBookmarkList->addItem(bm);
				item->getApplyBtn()->onEvent = [this, item](int event, GWidget* widget)
				{
					if (event == EVT_BUTTON_CLICK)
					{
						onApplyBookmark(item->getData());
						if (onCaptureThumbnail)
						{
							auto& bmRef = item->getDataRef();
							bmRef.thumbnail = onCaptureThumbnail();
						}
					}
					return false;
				};
				mUIBookmarkName->setValue("");
				saveBookmarks();
			}
			return false;
		};

		yCtx += ButtonHeight + 10;

		// Bottom Delete Button
		int const RemoveBtnHeight = 25;
		int removeBtnY = size.y - Margin - RemoveBtnHeight;
		GButton* removeBtn = new GButton(UI_ANY, Vec2i(Margin, removeBtnY), Vec2i(size.x - 2 * Margin, RemoveBtnHeight), this);
		removeBtn->setTitle("Delete Selected Location");
		removeBtn->onEvent = [this](int id, GWidget* widget)
		{
			int selectedIndex = mUIBookmarkList->getSelection();
			if (selectedIndex != INDEX_NONE && selectedIndex < (int)mBookmarks.size())
			{
				mBookmarks.erase(mBookmarks.begin() + selectedIndex);
				mUIBookmarkList->removeAllItems();

				// Rebuild list
				for (auto const& bm : mBookmarks)
				{
					auto* item = mUIBookmarkList->addItem(bm);
					item->getApplyBtn()->onEvent = [this, item](int event, GWidget* widget)
					{
						if (event == EVT_BUTTON_CLICK)
						{
							onApplyBookmark(item->getData());
							if (onCaptureThumbnail)
							{
								auto& bmRef = item->getDataRef();
								bmRef.thumbnail = onCaptureThumbnail();
							}
						}
						return false;
					};
				}
				saveBookmarks();
			}
			return false;
		};

		// List (Fills remaining space)
		int listHeight = removeBtnY - yCtx - 10;
		auto* listCtrl = new GWidgetListT<FractalBookmark, BookmarkItemUI>(UI_ANY, Vec2i(0, yCtx), Vec2i(size.x, listHeight), this);
		listCtrl->setItemHeight(80);
		mUIBookmarkList = listCtrl;
		loadBookmarks();
	}

	void onRender() override
	{
		Vec2i pos = getWorldPos();
		Vec2i size = getSize();
		IGraphics2D& g = Global::GetIGraphics2D();

		// Main Background
		RenderUtility::SetPen(g, EColor::Null);
		g.setBrush(Color3ub(25, 25, 30));
		g.drawRect(pos, size);

		// Header Bar
		g.setBrush(Color3ub(35, 35, 45));
		g.drawRect(pos, Vec2i(size.x, 35));

		// Header Accent Line
		g.setBrush(Color3ub(120, 80, 220));
		g.drawRect(pos + Vec2i(0, 33), Vec2i(size.x, 2));

		// Title Text
		g.setTextColor(Color3ub(220, 220, 240));
		RenderUtility::SetFont(g, FONT_S12);
		g.drawText(pos + Vec2i(15, 8), "BOOKMARKS");

		// Border
		RenderUtility::SetBrush(g, EColor::Null);
		g.setPen(Color3ub(60, 60, 80), 1);
		g.drawRect(pos, size);
	}

};



int CalcIteration(BigFloat const& cr, BigFloat const& ci, MandelbrotParam const& param)
{
	double bailoutSquare = param.bailoutValue * param.bailoutValue;
	BigFloat zr; zr.setZero();
	BigFloat zi; zi.setZero();
	BigFloat r2, i2, ni, temp;

	// Use a temporary bailout check (4.0)
	for (int i = 0; i < param.maxIteration; ++i)
	{
		r2 = zr; r2.mul(zr);
		i2 = zi; i2.mul(zi);

		// Check escape
		temp = r2; temp.add(i2);
		double len2;
		temp.convertToDouble(len2);
		if (len2 > bailoutSquare) return i;

		// ni = 2*zr*zi + ci
		ni = zr; ni.mul(zi); ni.mul(2); ni.add(ci);

		// zr = r2 - i2 + cr
		zr = r2; zr.sub(i2); zr.add(cr);
		zi = ni;
	}
	return param.maxIteration;
}


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

	// OrbitPoint now includes Zn, An (1st derivative), and Bn (2nd derivative)
	struct OrbitPoint
	{
		TVector2<double> h; TVector2<double> l;   // Zn
		TVector2<double> ah; TVector2<double> al; // An
		TVector2<double> bh; TVector2<double> bl; // Bn
	};
	TArray<OrbitPoint> mReferenceOrbit;
	TStructuredBuffer< OrbitPoint > mOrbitBuffer;

	BigFloat mRefFullR;
	BigFloat mRefFullI;
	int      mRefIter = 0;

	QueueThreadPool mPool;
	std::atomic<bool> mbOptimizationRunning = { false };
	struct OptimizationResult
	{
		BigFloat bestR;
		BigFloat bestI;
		int maxIter;
		bool bValid = false;
	};
	OptimizationResult mPendingResult;
	Mutex mResultMutex;
	std::atomic<int> mNumPendingTasks = { 0 };
	OptimizationResult mBestFoundInRange;
	int  mLastAnalysisMaxIter = -1;
	int  mStableMaxIterCount = 0;
	bool mbAnalysisActive = true;
	struct OrbitUpdateResult
	{
		TArray<OrbitPoint> orbitData;
		BigFloat refR, refI;
		int refIter;
		uint32 requestID;
		bool bValid = false;
	};
	OrbitUpdateResult mPendingOrbitResult;
	std::atomic<uint32> mParamVersion = { 0 };
	std::atomic<int> mTargetRefIter = { 0 };
	std::atomic<bool> mbOrbitCalculating = { false };
	Mutex mOrbitMutex;


	void findBestReferencePoint(bool bForce = false)
	{
		// DISABLED DRIFT: Keep reference point at viewport center for calibration
		mRefFullR = mParam.centerFullR;
		mRefFullI = mParam.centerFullI;
		mRefIter = CalcIteration(mRefFullR, mRefFullI, mParam);
	}

	void updateReferenceOrbit(BigFloat const* pRefR = nullptr, BigFloat const* pRefI = nullptr, int expectedIter = 0)
	{
		// If it's an AI-suggested optimization, check if it's better than current target
		if (expectedIter > 0 && expectedIter <= mTargetRefIter)
			return;

		mTargetRefIter = expectedIter;
		uint32 currentID = ++mParamVersion;
		mbOrbitCalculating = true;

		BigFloat targetRefR, targetRefI;
		if (pRefR && pRefI)
		{
			targetRefR = *pRefR;
			targetRefI = *pRefI;
		}
		else
		{
			targetRefR = mParam.centerFullR;
			targetRefI = mParam.centerFullI;
		}

		// Capture parameters needed for calculation
		MandelbrotParam localParam = mParam;

		mPool.addFunctionWork([this, targetRefR, targetRefI, localParam, currentID]()
		{
			TIME_SCOPE("updateReferenceOrbit_Background");

			OrbitUpdateResult result;
			result.refR = targetRefR;
			result.refI = targetRefI;
			result.requestID = currentID;
			result.refIter = CalcIteration(targetRefR, targetRefI, localParam);

			result.orbitData.resize(localParam.maxIteration + 1);

			TComplexBigFloat z, c;
			z.setZero();
			c.r = targetRefR;
			c.i = targetRefI;

			for (int i = 0; i <= localParam.maxIteration; ++i)
			{
				if ((i % 100 == 0) && (mParamVersion != currentID)) break;

				double r0, r1, r2, r3;
				double i0, i1, i2, i3;

				auto Extract4 = [](BigFloat const& val, double& d0, double& d1, double& d2, double& d3)
				{
					BigFloat temp = val;
					temp.convertToDouble(d0);
					temp.sub(BigFloat(d0));
					temp.convertToDouble(d1);
					temp.sub(BigFloat(d1));
					temp.convertToDouble(d2);
					temp.sub(BigFloat(d2));
					temp.convertToDouble(d3);
				};

				Extract4(z.r, r0, r1, r2, r3);
				Extract4(z.i, i0, i1, i2, i3);

				result.orbitData[i] = {
					TVector2<double>(r0, i0), TVector2<double>(r1, i1),
					TVector2<double>(r2, i2), TVector2<double>(r3, i3),
					TVector2<double>(0, 0),    TVector2<double>(0, 0)
				};

				BigFloat magSqBF = z.r; magSqBF.mul(z.r);
				BigFloat tempI = z.i; tempI.mul(z.i);
				magSqBF.add(tempI);
				magSqBF.sub(BigFloat(localParam.bailoutValue * localParam.bailoutValue));

				double rb0, rb1;
				magSqBF.convertToDouble(rb0);
				BigFloat tempRes = magSqBF; tempRes.sub(BigFloat(rb0));
				tempRes.convertToDouble(rb1);
				result.orbitData[i].bh = TVector2<double>(rb0, rb1);

				if (magSqBF > BigFloat(0.0))
				{
					result.refIter = i;
					for (int j = i + 1; j < localParam.maxIteration + 1; ++j)
						result.orbitData[j] = result.orbitData[i];
					break;
				}
				z.nextIteration(c);
			}

			if (mParamVersion == currentID)
			{
				result.bValid = true;
				Mutex::Locker locker(mOrbitMutex);
				mPendingOrbitResult = std::move(result);
			}
		});
	}

	RHITexture2DRef captureThumbnail()
	{
		int const size = 128;
		RHITexture2DRef thumbnail = RHICreateTexture2D(TextureDesc::Type2D(ETexture::RGBA8, size, size).Flags(TCF_RenderTarget | TCF_CreateSRV));
		if (!thumbnail)
			return nullptr;

		RHICommandList& commandList = RHICommandList::GetImmediateList();
		RHIFrameBufferRef frameBuffer = RHICreateFrameBuffer();
		frameBuffer->addTexture(*thumbnail);

		// Snapshot the current full-res fractal texture into the small thumbnail
		RHISetFrameBuffer(commandList, frameBuffer);
		RHISetViewport(commandList, 0, 0, size, size);

		Matrix4 projectionMatrix = AdjustProjectionMatrixForRHI(OrthoMatrix(0, size, size, 0, 1, -1));
		if (mTexture)
		{
			DrawUtility::DrawTexture(commandList, projectionMatrix, *mTexture, Vec2i(0, 0), Vec2i(size, size));
		}

		RHISetFrameBuffer(commandList, nullptr);
		return thumbnail;
	}



	SelectRect mSelectRect;

	virtual bool onInit()
	{
		if (!BaseClass::onInit())
			return false;

		::Global::GUI().cleanupWidget();

		Vec2i screenSize = ::Global::GetScreenSize();
		DevFrame* frame = WidgetUtility::CreateDevFrame();
		FWidgetProperty::Bind(frame->addCheckBox(UI_ANY, "Native Double"), bUseNativeDouble, [this](bool bValue) { bNeedUpdateTexture = true; });
		FWidgetProperty::Bind(frame->addCheckBox(UI_ANY, "Double Sim"), bUseDoubleSim, [this](bool bValue) { bNeedUpdateTexture = true; });
		FWidgetProperty::Bind(frame->addCheckBox(UI_ANY, "Use Perturbation"), bUsePerturb, [this](bool bValue) { bNeedUpdateTexture = true; });

		GradientEditorPanel* gradientPanel = new GradientEditorPanel(UI_ANY, Vec2i(10, 300), Vec2i(300, 250), nullptr);
		::Global::GUI().addWidget(gradientPanel);
		gradientPanel->show(false);

		frame->addButton("Color Editor", [gradientPanel](int, GWidget*) {
			gradientPanel->show(true);
			gradientPanel->setTop();
			return false;
		});

		gradientPanel->onGradientChanged = [this](TArray<GradientKey> const& keys, int rotation) {
			ColorMap colorMap(1024);
			for (auto const& key : keys)
				colorMap.addPoint(int(key.time * ColorMap::CPPosRange), key.color);

			colorMap.setRotion(rotation);
			colorMap.setSmoothLine(true);
			colorMap.calcColorMap(true);

			Color4f colors[1024];
			for (int i = 0; i < ARRAY_SIZE(colors); ++i)
			{
				Color3f color;
				colorMap.getColor(float(i) / 1024, color);
				colors[i] = color;
			}
			mColorMap = RHICreateTexture1D(ETexture::RGBA32F, 1024, 0, 1, TCF_DefalutValue, colors);
			bNeedUpdateTexture = true;
		};

		FractalBookmarkPanel* bookmarkPanel = new FractalBookmarkPanel(UI_ANY, Vec2i(screenSize.x - 260 - 10, 120), Vec2i(260, 480), nullptr);
		bookmarkPanel->setRenderType(GPanel::eRectType);
		::Global::GUI().addWidget(bookmarkPanel);

		frame->addButton("Open Bookmarks", [bookmarkPanel](int, GWidget*) {
			bookmarkPanel->show(true);
			bookmarkPanel->setTop();
			return false;
		});

		bookmarkPanel->onApplyBookmark = [this](FractalBookmark const& bm)
		{
			mParam.centerFullR.setFromString(bm.centerR.c_str());
			mParam.centerFullI.setFromString(bm.centerI.c_str());
			mParam.magnificationFromStr(bm.magnification.c_str());
			mParam.rotationAngle = bm.rotation;

			mParam.syncCenterPart();
			notifyParamChanged();

			updateReferenceOrbit();
		};

		bookmarkPanel->onFetchCurrentBookmark = [this]()
		{
			FractalBookmark bm;
			mParam.centerFullR.getString(bm.centerR);
			mParam.centerFullI.getString(bm.centerI);
			bm.magnification = mParam.magnificationToStr();
			bm.rotation = mParam.rotationAngle;
			bm.thumbnail = captureThumbnail();
			return bm;
		};

		bookmarkPanel->onCaptureThumbnail = [this]()
		{
			return captureThumbnail();
		};


		return true;
	}

	virtual void postInit()
	{
		restart();
	}

	virtual ERenderSystem getDefaultRenderSystem() override
	{
		return ERenderSystem::D3D12;
	}
	virtual bool setupRenderResource(ERenderSystem systemName) override
	{
		mPool.init(SystemPlatform::GetProcessorNumber());
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
		VERIFY_RETURN_FALSE(mTexture = RHICreateTexture2D(TextureDesc::Type2D(ETexture::RGBA32F, screenSize.x, screenSize.y).Flags(TCF_CreateUAV | TCF_CreateSRV | TCF_DefalutValue)));
		mTexture->setDebugName("RWTexture");


		mMParamsBuffer.initializeResource(1);
		mMParamsBufferDouble.initializeResource(1);
		mMParamsBufferPerturb.initializeResource(1);
		mAnalysisBuffer.initializeResource(mParam.viewSize.x + 2, EStructuredBufferType::RWBuffer, true);
		return true;
	}

	void debugRenderPerturbCPU()
	{
		LogMsg("--- START CPU PERTURB VERIFY (Double Precision) ---");

		TVector2<double> centerFull = mParam.center + mParam.centerLow;
		TVector2<double> ratioFull = mParam.getRatio();

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

	uint32 mGpuCenterIter;

	void verifyPerturbation()
	{
		struct DD
		{
			double hi, lo;
			DD(double h = 0, double l = 0) : hi(h), lo(l) {}
		};

		auto DSAdd = [](DD a, DD b) -> DD
		{
			double s = a.hi + b.hi; double t = s - a.hi;
			double e = (a.hi - (s - t)) + (b.hi - t);
			e += (a.lo + b.lo);
			double s2 = s + e; double e2 = e + (s - s2);
			return DD(s2, e2);
		};

		auto DSMul = [](DD a, DD b) -> DD
		{
			const double split = 134217729.0;
			double cona = a.hi * split; double ax = cona - (cona - a.hi); double ay = a.hi - ax;
			double conb = b.hi * split; double bx = conb - (conb - b.hi); double by = b.hi - bx;
			double r1 = a.hi * b.hi;
			double r2 = ay * by - (((r1 - ax * bx) - ay * bx) - ax * by);
			r2 += (a.hi * b.lo + a.lo * b.hi);
			double s2 = r1 + r2; double e2 = r2 + (r1 - s2);
			return DD(s2, e2);
		};

		auto DSSub = [&](DD a, DD b) -> DD { return DSAdd(a, DD(-b.hi, -b.lo)); };

		auto DSComplexMul = [&](DD ar, DD ai, DD br, DD bi, DD& outR, DD& outI)
		{
			DD rr = DSMul(ar, br); DD ii = DSMul(ai, bi);
			DD ri = DSMul(ar, bi); DD ir = DSMul(ai, br);
			outR = DSSub(rr, ii); outI = DSAdd(ri, ir);
		};

		int midY = mParam.viewSize.y / 2;
		int midX = mParam.viewSize.x / 2;

		if (mReferenceOrbit.empty()) return;

		// 1. Calculate Truth (BigFloat)
		BigFloat cR_Pixel, cI_Pixel;
		mParam.getBigFloatCoordinates(Vector2((float)midX, (float)midY), cR_Pixel, cI_Pixel);
		int trueIter = CalcIteration(cR_Pixel, cI_Pixel, mParam);

		// 2. Simulate Perturbation (CPU DD)
		DD dzR(0, 0), dzI(0, 0);
		DD dcR, dcI;
		{
			BigFloat dR = cR_Pixel; dR.sub(mRefFullR);
			BigFloat dI = cI_Pixel; dI.sub(mRefFullI);
			double dh, dl;
			dR.convertToDouble(dh); BigFloat t = dR; t.sub(BigFloat(dh)); t.convertToDouble(dl);
			dcR = DD(dh, dl);
			dI.convertToDouble(dh); t = dI; t.sub(BigFloat(dh)); t.convertToDouble(dl);
			dcI = DD(dh, dl);
		}

		int simIter = 0;
		for (simIter = 0; simIter < mParam.maxIteration && simIter < mReferenceOrbit.size(); ++simIter)
		{
			OrbitPoint const& OP = mReferenceOrbit[simIter];
			DD zRefR(OP.h.x, OP.l.x);
			DD zRefI(OP.h.y, OP.l.y);
			DD dzSqR, dzSqI;
			DSComplexMul(dzR, dzI, dzR, dzI, dzSqR, dzSqI);

			DD twoZRefDzR, twoZRefDzI;
			DSComplexMul(zRefR, zRefI, dzR, dzI, twoZRefDzR, twoZRefDzI);

			twoZRefDzR = DSAdd(twoZRefDzR, twoZRefDzR);
			twoZRefDzI = DSAdd(twoZRefDzI, twoZRefDzI);

			dzR = DSAdd(DSAdd(twoZRefDzR, dzSqR), dcR);
			dzI = DSAdd(DSAdd(twoZRefDzI, dzSqI), dcI);

			if (simIter + 1 < mReferenceOrbit.size())
			{
				OrbitPoint const& Next = mReferenceOrbit[simIter + 1];
				DD fullR = DSAdd(DD(Next.h.x, Next.l.x), dzR);
				DD fullI = DSAdd(DD(Next.h.y, Next.l.y), dzI);
				if (fullR.hi*fullR.hi + fullI.hi*fullI.hi > mParam.bailoutValue) { simIter++; break; }
			}
		}

		LogMsg("--- PIXEL(%d,%d) CROSS-CHECK ---", midX, midY);
		LogMsg("  1. BIGFLOAT TRUTH   : %d", trueIter);
		LogMsg("  2. CPU SIM (DD)     : %d  (Diff vs Truth: %d)", simIter, (simIter - trueIter));
		LogMsg("  3. GPU REAL RESULT  : %d  (Diff vs Sim: %d)", mGpuCenterIter, (int(mGpuCenterIter) - simIter));

		if (mGpuCenterIter == 0) LogMsg("  WARNING: GPU iteration is 0. Check Analysis Pass binding!");
	}

	void cancelOptimization()
	{
		mParamVersion.fetch_add(1, std::memory_order_relaxed);
		mPool.cencelAllWorks();
		mbOptimizationRunning = false;
	}

	void notifyParamChanged()
	{
		cancelOptimization();
		mReferenceOrbit.clear();
		mRefIter = 0;
		bNeedUpdateTexture = true;
		mbAnalysisActive = true;
		mLastAnalysisMaxIter = -1;
		mStableMaxIterCount = 0;
		mTargetRefIter = 0;
	}

	void optimizeReferenceCPU()
	{
		if (mbOptimizationRunning)
			return;

		if (mReferenceOrbit.empty()) return;

		mbOptimizationRunning = true;

		Vector2 refPixelPos;
		mParam.getPixelPos(mRefFullR, mRefFullI, refPixelPos);

		int numTasks = SystemPlatform::GetProcessorNumber();
		int pointsPerTask = 50; // Roughly 50 points per task chunk

		mNumPendingTasks = numTasks;
		mBestFoundInRange.maxIter = 0;
		mBestFoundInRange.bValid = false;

		// Capture parameters needed for calculation
		MandelbrotParam localParam = mParam;
		int currentRefIter = mRefIter;
		uint32 requestID = mParamVersion.load(std::memory_order_relaxed);

		for (int t = 0; t < numTasks; ++t)
		{
			bool bIsLocalRefine = (t < numTasks / 2);

			mPool.addFunctionWork([this, localParam, currentRefIter, requestID, refPixelPos, bIsLocalRefine, t, pointsPerTask]()
			{
				TIME_SCOPE(bIsLocalRefine ? "optimizeReferenceCPU_Local" : "optimizeReferenceCPU_Global");
				int maxFoundIter = 0;
				int bestX = -1, bestY = -1;

				Random::Microsoft randGen(SystemPlatform::GetTickCount() + t * 777);

				for (int i = 0; i < pointsPerTask; ++i)
				{
					if ((i % 10 == 0) && (mParamVersion.load(std::memory_order_relaxed) != requestID)) break;

					int x, y;
					if (bIsLocalRefine)
					{
						// Mode 1: Local Refinement (Radius 200)
						x = (int)refPixelPos.x + (randGen.rand() % 401 - 200);
						y = (int)refPixelPos.y + (randGen.rand() % 401 - 200);
					}
					else
					{
						// Mode 2: Global Random Sampling
						x = randGen.rand() % localParam.viewSize.x;
						y = randGen.rand() % localParam.viewSize.y;
					}

					if (x < 0 || x >= localParam.viewSize.x || y < 0 || y >= localParam.viewSize.y)
						continue;

					BigFloat cR_Pixel, cI_Pixel;
					localParam.getBigFloatCoordinates(Vector2((float)x, (float)y), cR_Pixel, cI_Pixel);
					int trueIter = CalcIteration(cR_Pixel, cI_Pixel, localParam);

					if (trueIter > maxFoundIter)
					{
						maxFoundIter = trueIter;
						bestX = x; bestY = y;
					}
				}

				bool bIDValid = (mParamVersion.load(std::memory_order_relaxed) == requestID);
				if (bIDValid && bestX != -1)
				{
					// Global update
					Mutex::Locker locker(mResultMutex);
					if (maxFoundIter > mBestFoundInRange.maxIter)
					{
						mBestFoundInRange.maxIter = maxFoundIter;
						localParam.getBigFloatCoordinates(Vector2((float)bestX, (float)bestY), mBestFoundInRange.bestR, mBestFoundInRange.bestI);
						mBestFoundInRange.bValid = true;
					}
				}

				// Check if we are the last task
				if (--mNumPendingTasks == 0)
				{
					if (bIDValid)
					{
						Mutex::Locker locker(mResultMutex);
						if (mBestFoundInRange.bValid && mBestFoundInRange.maxIter > currentRefIter + 20)
						{
							mPendingResult = mBestFoundInRange;
						}
						else
						{
							mbOptimizationRunning = false;
						}
					}
					else
					{
						mbOptimizationRunning = false;
					}
				}
			});
		}
	}

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
				if (!mbOrbitCalculating)
				{
					updateReferenceOrbit();
				}
				// Skip perturbation if orbit is not ready
				return;
			}

			//LogMsg("--- STARTING GPU/CPU COMPARISON ---");
			//verifyPerturbation();

			// Select shader permutation based on double precision settings
			MandelbrotPerturbShader::PermutationDomain perturbDomain;
			perturbDomain.set<MandelbrotPerturbShader::UseNativeDouble>(bUseNativeDouble);
			perturbDomain.set<MandelbrotPerturbShader::UseDoubleSim>(bUseDoubleSim);
			mProgMandelbrotPerturb = ShaderManager::Get().getGlobalShaderT< MandelbrotPerturbShader >(perturbDomain);

			RHISetComputeShader(commandList, mProgMandelbrotPerturb->getRHI());

			{
				auto data = mMParamsBufferPerturb.lock();
				if (data)
				{
					data->set(mParam, mRefFullR, mRefFullI);
					mMParamsBufferPerturb.unlock();
				}
			}

			RHIResource* RWResource[] = { mTexture };
			RHIResourceTransition(commandList, RWResource, EResourceTransition::UAV);

			mProgMandelbrotPerturb->setParameters(commandList, mParam, *mTexture, *mColorMap, mOrbitBuffer.getRHI(), mMParamsBufferPerturb);
			RHIDispatchCompute(commandList, (mParam.viewSize.x + MandelbrotPerturbShader::SizeX - 1) / MandelbrotPerturbShader::SizeX, (mParam.viewSize.y + MandelbrotPerturbShader::SizeY - 1) / MandelbrotPerturbShader::SizeY, 1);
			mProgMandelbrotPerturb->clearParameters(commandList);

			// CRITICAL: Add UAV barrier to ensure MainCS writes are complete before AnalysisCS reads
			RHIResource* RWResourcesForBarrier[] = { mTexture };
			RHIResourceTransition(commandList, RWResourcesForBarrier, EResourceTransition::UAVBarrier);

			// Run Analysis pass only if reference hasn't reached max iteration and analysis is active
			if (mbAnalysisActive && mRefIter < mParam.maxIteration)
			{
				// Run Analysis pass
				uint32 clearData[2] = { 0, 0 };
				// Corrected: Clear 8 bytes (two uint32)
				RHIUpdateBuffer(*mAnalysisBuffer.getRHI(), 0, 8, clearData);

				RHISetComputeShader(commandList, mProgAnalysis->getRHI());
				mProgAnalysis->setParameters(commandList, *mTexture, *mAnalysisBuffer.getRHI(), mMParamsBufferPerturb);
				int groupsX = (mParam.viewSize.x + 15) / 16;
				int groupsY = (mParam.viewSize.y + 15) / 16;
				RHIDispatchCompute(commandList, groupsX, groupsY, 1);
				mProgAnalysis->clearParameters(commandList);

				// Readback results
				uint32 resultBuffer[128];
				memset(resultBuffer, 0, sizeof(resultBuffer));

				void* pData = RHILockBuffer(mAnalysisBuffer.getRHI(), ELockAccess::ReadOnly, 0, sizeof(resultBuffer));
				if (pData)
				{
					std::copy((uint32*)pData, (uint32*)pData + 128, resultBuffer);
					RHIUnlockBuffer(mAnalysisBuffer.getRHI());
				}

				uint32 maxIter = resultBuffer[0];
				uint32 packedCoord = resultBuffer[1];

				// Stability Check: If maxIter is the same for many frames, disable analysis
				if ((int)maxIter == mLastAnalysisMaxIter)
				{
					mStableMaxIterCount++;
					if (mStableMaxIterCount > 60)
					{
						LogMsg("Analysis: System stabilized at MaxIter %u. Disabling background optimization.", maxIter);
						mbAnalysisActive = false;
					}
				}
				else
				{
					mLastAnalysisMaxIter = (int)maxIter;
					mStableMaxIterCount = 0;
				}

				// Debug: Show what Analysis found
				static int analysisFrame = 0;
				//if (analysisFrame++ % 60 == 0)
				{
					LogMsg("Analysis: maxIter=%u, coord=(%d,%d), RefIter=%d",
						maxIter, (packedCoord >> 16), (packedCoord & 0xffff), mRefIter);
				}

				// Auto-Refinement: If GPU finds a deeper point, jump there!
				if (maxIter > mRefIter + 10)
				{
					int targetX = (packedCoord >> 16);
					int targetY = (packedCoord & 0xffff);

					if (targetX >= 0 && targetX < mParam.viewSize.x && targetY >= 0 && targetY < mParam.viewSize.y)
					{
						LogMsg("GPU Refinement: Found deeper point at (%d, %d) Iter: %d -> %d",
							targetX, targetY, mRefIter, maxIter);

						BigFloat bestR, bestI;
						mParam.getBigFloatCoordinates(Vector2((float)targetX, (float)targetY), bestR, bestI);
						updateReferenceOrbit(&bestR, &bestI, (int)maxIter);
						bNeedUpdateTexture = true;

						// Reset stability when finding a better point
						mLastAnalysisMaxIter = -1;
						mStableMaxIterCount = 0;
					}
				}
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
		mPool.waitAllWorkComplete();
		BaseClass::onEnd();
	}

	void restart()
	{
		mParam.setDefalut();
		mParam.magnification = 1.0;
		mRefFullR = mParam.centerFullR;
		mRefFullI = mParam.centerFullI;
		notifyParamChanged();
	}

	int mFrameIndex = 0;

	void onUpdate(GameTimeSpan deltaTime) override
	{
		BaseClass::onUpdate(deltaTime);
		bNeedUpdateTexture = true;

		{
			OrbitUpdateResult result;
			{
				Mutex::Locker locker(mOrbitMutex);
				if (mPendingOrbitResult.bValid)
				{
					result = std::move(mPendingOrbitResult);
					mPendingOrbitResult.bValid = false;
				}
			}

			if (result.bValid)
			{
				mReferenceOrbit = std::move(result.orbitData);
				mRefFullR = result.refR;
				mRefFullI = result.refI;
				mRefIter = result.refIter;
				mbOrbitCalculating = false;

				if (!mOrbitBuffer.isValid() || mOrbitBuffer.getElementNum() < mReferenceOrbit.size())
				{
					mOrbitBuffer.initializeResource((uint32)mReferenceOrbit.size(), EStructuredBufferType::Buffer, false);
				}
				mOrbitBuffer.updateBuffer(MakeConstView(mReferenceOrbit));

				bNeedUpdateTexture = true;
				LogMsg("Async Orbit Updated: Iter %d", mRefIter);
			}
		}

		{
			OptimizationResult result;
			{
				Mutex::Locker locker(mResultMutex);
				if (mPendingResult.bValid)
				{
					result = mPendingResult;
					mPendingResult.bValid = false;
				}
			}

			if (result.bValid)
			{
				LogMsg("CPU Optimization: Switching Reference (Background Task Outcome) to Iter %d", result.maxIter);
				updateReferenceOrbit(&result.bestR, &result.bestI, result.maxIter);
				mbOptimizationRunning = false;
				bNeedUpdateTexture = true;
			}
		}

		if (bUsePerturb && mRefIter < mParam.maxIteration)
		{
			static int optimizationFrame = 0;
			if (optimizationFrame++ % 30 == 0)
			{
				optimizeReferenceCPU();
			}
		}
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
		RHISetRasterizerState(commandList, TStaticRasterizerState< ECullMode::None >::GetRHI());
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

		int y = 10;
		g.setTextColor(Color3f(1, 0, 0));
		RenderUtility::SetFont(g, FONT_S10);
		g.drawText(200, y, str); y += 15;
		if (bUsePerturb)
		{
			str.format("  RefIter: %d", mRefIter);
			g.drawText(200, y, str); y += 15;
		}

		if (mSelectRect.isEnable())
		{
			str.format("%f %f", mPosRectCenter.x, mPosRectCenter.y);
			g.drawText(200, y, str); y += 15;
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

	static void calcZoomInfo(ZoomInfo& outInfo, SelectRectBase const& SRect, MandelbrotParam const& param, bool bZoomIn)
	{
		int w = param.viewSize.x;
		int h = param.viewSize.y;

		Vec2i pos = SRect.getCenterPos();
		outInfo.centerOffset = SRect.getCenterPos() - param.viewSize / 2;

		Vector2 mag = Vector2(param.viewSize).div(SRect.getSize());
		float magh = float(h) / SRect.getHeight();
		float magw = float(w) / SRect.getWidth();
		float zoom = std::max(magh, magw);
		if (bZoomIn)
			zoom = 1 / zoom;
		outInfo.zoomFactor = zoom;

		float angle = -SRect.getRotationAngle();
		outInfo.angle = WrapZeroTo2PI(angle);
	}

	Vector2 mPosRectCenter;
	MsgReply onMouse(MouseMsg const& msg)
	{
		mPosOnMouse = mParam.getCoordinates(msg.getPos());

		if (!mSelectRect.procMouseMsg(msg))
			return MsgReply::Handled();

		if (mSelectRect.isEnable())
		{
			ZoomInfo info;
			calcZoomInfo(info, mSelectRect, mParam, msg.onLeftDClick());
			mPosRectCenter = mParam.getCoordinates(Vector2(info.centerOffset) + 0.5 * Vector2(mParam.viewSize));
			if (msg.onLeftDClick() || msg.onRightDClick())
			{

				mSelectRect.enable(false);

				mParam.zoomInPos(info.centerOffset, info.zoomFactor, info.angle);
				notifyParamChanged();
				return MsgReply::Handled();
			}
		}

		return BaseClass::onMouse(msg);
	}

	Vector2 mPosOnMouse;

	MsgReply onKey(KeyMsg const& msg)
	{
		if (msg.isDown())
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
		switch (id)
		{
		default:
			break;
		}

		return BaseClass::onWidgetEvent(event, id, ui);
	}



protected:
};


REGISTER_STAGE_ENTRY("Fractial Test", FractialTestStage, EExecGroup::FeatureDev, "Render|Math");