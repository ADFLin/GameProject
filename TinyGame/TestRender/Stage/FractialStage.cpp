#include "Stage/TestStageHeader.h"

#include "RHI/RHIGraphics2D.h"

#include "RHI/RHICommand.h"
#include "RHI/ShaderManager.h"
#include "RHI/DrawUtility.h"

#include "CurveBuilder/ColorMap.h"

#include "SuperFractial/SelectRectBase.h"
#include "RHI/GpuProfiler.h"
#include "GameRenderSetup.h"

using namespace Render;

struct MandelbrotParam
{
	Vec2i viewSize;
	float rotationAngle;
	int   maxIteration;
	Vector2 center;
	float bailoutValue;
	float magnification;
	float stretch;

	float getRatioX() const { return getRatioY()* stretch; }
	float getRatioY() const { return 0.005 / magnification; }

	void  zoomInPos( Vec2i const& centerOffset , float zoomFactor, float angle)
	{
		center = getCoordinates(Vector2(centerOffset) + 0.5 * Vector2(viewSize));
		magnification /= zoomFactor;
		setRotationAngle(rotationAngle + angle);
	}

	void setRotationAngle(float angle)
	{
		rotationAngle = WrapZeroTo2PI( angle );
	}

	Vector2 getCoordinates( Vector2 pixelPos ) const
	{
		Vector2 len = (pixelPos - 0.5f * Vector2(viewSize)).mul(Vector2(getRatioX(), getRatioY()));
		float s, c;
		Math::SinCos(rotationAngle, s, c);
		return center + Vector2(c*len.x + s*len.y, -s*len.x + c*len.y);
	}

	void setDefalut()
	{
		rotationAngle = 0;
		maxIteration = 20000;
		center = Vector2(-0.5, 0);
		bailoutValue = 2048.0;
		magnification = 1.0;
		stretch = 1.0;
	}

};


class MandelbrotProgram : public GlobalShaderProgram
{
	DECLARE_SHADER_PROGRAM(MandelbrotProgram, Global);

	static int constexpr SizeX = 16;
	static int constexpr SizeY = 16;


	void bindParameters(ShaderParameterMap const& parameterMap)
	{
		BIND_SHADER_PARAM(parameterMap, CoordParam);
		BIND_SHADER_PARAM(parameterMap, CoordParam2);
		BIND_SHADER_PARAM(parameterMap, ViewSize);
		BIND_SHADER_PARAM(parameterMap, MaxIteration);
		BIND_SHADER_PARAM(parameterMap, ColorMapParam);
		BIND_SHADER_PARAM(parameterMap, ColorRWTexture);
		BIND_TEXTURE_PARAM(parameterMap, ColorMapTexture);
	}

	void setParameters(RHICommandList& commandList, MandelbrotParam const& param , RHITexture2D& colorTexture , RHITexture1D& colorMapTexture )
	{
		Vector4 rectParam;
		rectParam.x = param.center.x;
		rectParam.y = param.center.y;
		rectParam.z = param.center.x - 0.5 * param.viewSize.x * param.getRatioX();
		rectParam.w = param.center.y - 0.5 * param.viewSize.y * param.getRatioY();
		SET_SHADER_PARAM(commandList, *this, CoordParam, rectParam);

		Vector4 rectParam2;
		rectParam2.x = param.getRatioX();
		rectParam2.y = param.getRatioY();
		Math::SinCos( param.rotationAngle , rectParam2.w, rectParam2.z);

		SET_SHADER_PARAM(commandList, *this, CoordParam2, rectParam2);
		SET_SHADER_PARAM(commandList, *this, ViewSize, param.viewSize);
		SET_SHADER_PARAM(commandList, *this, MaxIteration, param.maxIteration);
		SET_SHADER_PARAM(commandList, *this, ColorMapParam, Vector4(1, 0, param.bailoutValue * param.bailoutValue, 0));

		setRWTexture(commandList, mParamColorRWTexture, colorTexture, AO_WRITE_ONLY);
		SET_SHADER_TEXTURE_AND_SAMPLER(commandList, *this, ColorMapTexture, colorMapTexture ,COMMA_SEPARATED(TStaticSamplerState< Sampler::eBilinear , Sampler::eWarp >::GetRHI()));
	}

	void clearParameters(RHICommandList& commandList)
	{
		clearRWTexture(commandList, mParamColorRWTexture);
	}

	static void SetupShaderCompileOption(ShaderCompileOption& option)
	{
		option.addDefine(SHADER_PARAM(SIZE_X), SizeX);
		option.addDefine(SHADER_PARAM(SIZE_Y), SizeY);
	}
	static char const* GetShaderFileName()
	{
		return "Shader/Game/Mandelbrot";
	}
	static TArrayView< ShaderEntryInfo const > GetShaderEntries()
	{
		static ShaderEntryInfo entries[] =
		{
			{ EShader::Compute , SHADER_ENTRY(MainCS) },
		};
		return entries;
	}

	DEFINE_SHADER_PARAM(CoordParam);
	DEFINE_SHADER_PARAM(CoordParam2);
	DEFINE_SHADER_PARAM(ViewSize);
	DEFINE_SHADER_PARAM(MaxIteration);
	DEFINE_SHADER_PARAM(ColorMapParam);
	DEFINE_SHADER_PARAM(ColorRWTexture);
	DEFINE_TEXTURE_PARAM(ColorMapTexture);

};

IMPLEMENT_SHADER_PROGRAM(MandelbrotProgram);


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


		RHISetBlendState(commandList, TStaticBlendState< CWM_RGBA, Blend::eOneMinusDestColor, Blend::eZero >::GetRHI());
		g.setBrush(Color3f(1, 1, 1));
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

		RHISetBlendState(commandList, TStaticBlendState<>::GetRHI());
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


	MandelbrotProgram* mProgMandelbrot = nullptr;
	RHITexture2DRef mTexture;
	RHITexture1DRef mColorMap;
	MandelbrotParam mParam;

	SelectRect mSelectRect;

	virtual bool onInit()
	{
		if( !BaseClass::onInit() )
			return false;

		::Global::GUI().cleanupWidget();

		return true;
	}

	virtual void postInit()
	{
		restart();
	}

	virtual bool setupRenderSystem()
	{
		VERIFY_RETURN_FALSE(mProgMandelbrot = ShaderManager::Get().getGlobalShaderT< MandelbrotProgram >(true));

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
		VERIFY_RETURN_FALSE(mColorMap = RHICreateTexture1D(Texture::eRGBA32F, 1024, 0, TCF_DefalutValue, colors));

		Vec2i screenSize = ::Global::GetScreenSize();
		mParam.viewSize = screenSize;
		VERIFY_RETURN_FALSE(mTexture = RHICreateTexture2D(Texture::eRGBA32F, screenSize.x, screenSize.y, 0, 1, TCF_CreateUAV | TCF_CreateSRV));

		return true;
	}
	virtual void preShutdownRenderSystem(bool bReInit = false)
	{
		mProgMandelbrot = nullptr;
		mColorMap.release();
		mTexture.release();
	}

	void updateTexture()
	{
		RHICommandList& commandList = RHICommandList::GetImmediateList();
		RHISetShaderProgram(commandList, mProgMandelbrot->getRHIResource());
		mProgMandelbrot->setParameters(commandList, mParam, *mTexture, *mColorMap );
		int nx = (mTexture->getSizeX() + MandelbrotProgram::SizeX - 1) / MandelbrotProgram::SizeX;
		int ny = (mTexture->getSizeY() + MandelbrotProgram::SizeY - 1) / MandelbrotProgram::SizeY;
		RHIDispatchCompute(commandList, nx, ny, 1);

		mProgMandelbrot->clearParameters(commandList);
	}

	virtual void onEnd()
	{
		BaseClass::onEnd();
	}

	void restart() 
	{
		mParam.setDefalut();
		mParam.magnification = 1.5;
		updateTexture();
	}
	void tick() {}
	void updateFrame(int frame) {}

	virtual void onUpdate(long time)
	{
		BaseClass::onUpdate(time);

		int frame = time / gDefaultTickTime;
		for( int i = 0; i < frame; ++i )
			tick();

		updateFrame(frame);
	}

	void onRender(float dFrame)
	{
		RHIGraphics2D& g = Global::GetRHIGraphics2D();

		Vec2i screenSize = ::Global::GetScreenSize();

		RHICommandList& commandList = RHICommandList::GetImmediateList();


		{
			GPU_PROFILE("Update Texture");
			updateTexture();
		}

		Matrix4 projectionMatrix = AdjProjectionMatrixForRHI(OrthoMatrix(0, screenSize.x, screenSize.y, 0, 1, -1));

		RHISetFrameBuffer(commandList, nullptr);
		RHIClearRenderTargets(commandList, EClearBits::All, &LinearColor(0.2, 0.2, 0.2, 1), 1);
		RHISetViewport(commandList, 0, 0, screenSize.x, screenSize.y);

		RHISetRasterizerState(commandList, TStaticRasterizerState< ECullMode::None >::GetRHI() );
		
		DrawUtility::DrawTexture(commandList, projectionMatrix, *mTexture, Vec2i(0, 0), screenSize);

		g.beginRender();
		mSelectRect.draw(g);
		FixString<246> str;
		str.format("%f %f", mPosOnMouse.x, mPosOnMouse.y);
		g.setTextColor(Color3f(1, 0, 0));
		RenderUtility::SetFont(g, FONT_S10);
		g.drawText(200, 10, str);
		if( mSelectRect.isEnable() )
		{
			str.format("%f %f", mPosRectCenter.x, mPosRectCenter.y);
			g.drawText(200, 10 + 15, str);
		}
		g.endRender();
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

		float angle = SRect.getRotationAngle();
		outInfo.angle = WrapZeroTo2PI(angle);
	}

	Vector2 mPosRectCenter;
	bool onMouse(MouseMsg const& msg)
	{
		if( !BaseClass::onMouse(msg) )
			return false;

		mPosOnMouse = mParam.getCoordinates(msg.getPos());

		if( !mSelectRect.procMouseMsg(msg) )
			return false;

		if ( mSelectRect.isEnable() )
		{
			ZoomInfo info;
			calcZoomInfo(info, mSelectRect, mParam, msg.onLeftDClick());
			mPosRectCenter = mParam.getCoordinates(Vector2(info.centerOffset)+ 0.5 * Vector2(mParam.viewSize));
			if( msg.onLeftDClick() || msg.onRightDClick() )
			{

				mSelectRect.enable(false);

				mParam.zoomInPos(info.centerOffset , info.zoomFactor, info.angle);
				updateTexture();
				return false;
			}
		}

		return true;
	}

	Vector2 mPosOnMouse;

	bool onKey(KeyMsg const& msg)
	{
		if( !msg.isDown() )
			return false;
		switch(msg.getCode())
		{
		case EKeyCode::R: restart(); break;
		}
		return false;
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


REGISTER_STAGE("Fractial Test", FractialTestStage, EStageGroup::FeatureDev);