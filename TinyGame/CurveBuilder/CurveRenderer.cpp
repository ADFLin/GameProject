#include "CurveRenderer.h"

#include "Surface.h"
#include "RenderData.h"

#include "RHI/DrawUtility.h"
#include "RHI/ShaderCompiler.h"
#include "RHI/RHICommand.h"

namespace CB
{
	using namespace RenderGL;

	class CurveMeshProgram : public GlobalShaderProgram
	{
		DECLARE_GLOBAL_SHADER(CurveMeshProgram)

		static void SetupShaderCompileOption(ShaderCompileOption&) {}
		static char const* GetShaderFileName()
		{
			return "Shader/Game/CurveMesh";
		}


		void bindParameters()
		{

		}

		void setParameters()
		{

		}
	};

	template< bool bUseOIT >
	class TCurveMeshProgram : public CurveMeshProgram
	{
		DECLARE_GLOBAL_SHADER(TCurveMeshProgram)

		static ShaderEntryInfo const* GetShaderEntries()
		{
			static ShaderEntryInfo const enties[] =
			{
				{ Shader::eVertex ,  SHADER_ENTRY(MainVS) },
				{ Shader::ePixel ,  SHADER_ENTRY(MainPS) },
				{ Shader::eEmpty ,  nullptr },
			};
			return enties;
		}

		static void SetupShaderCompileOption(ShaderCompileOption& option) 
		{
			option.addDefine(SHADER_PARAM(USE_OIT), bUseOIT );
		}
	};

	class MeshNormalVisualizeProgram : public CurveMeshProgram
	{
		DECLARE_GLOBAL_SHADER(MeshNormalVisualizeProgram)

		static void SetupShaderCompileOption(ShaderCompileOption&) {}
		static char const* GetShaderFileName()
		{
			return "Shader/Game/MeshNormalVisualize";
		}
		static ShaderEntryInfo const* GetShaderEntries()
		{
			static ShaderEntryInfo const enties[] =
			{
				{ Shader::eVertex ,  SHADER_ENTRY(MainVS) },
				{ Shader::eGeometry ,  SHADER_ENTRY(MainGS) },
				{ Shader::ePixel ,  SHADER_ENTRY(MainPS) },
				{ Shader::eEmpty ,  nullptr },
			};
			return enties;
		}

		void bindParameters()
		{
			mParamNormalLength.bind(*this, SHADER_PARAM(NormalLength));
			mParamNormalColor.bind(*this, SHADER_PARAM(NormalColor));
			mParamDensityAndSize.bind(*this, SHADER_PARAM(DensityAndSize));
		}

		void setParameters( Vector4 const& inColor , float inNormalLength , int inDensity , int inSize )
		{
			setParam(mParamNormalLength, inNormalLength);
			if ( mParamNormalColor.isBound() )
				setParam(mParamNormalColor, inColor);
			setParam(mParamDensityAndSize, IntVector2( inDensity , inSize ) );
		}

		ShaderParameter mParamDensityAndSize;
		ShaderParameter mParamNormalLength;
		ShaderParameter mParamNormalColor;
	};


	IMPLEMENT_GLOBAL_SHADER_T(template<>, TCurveMeshProgram<false>);
	IMPLEMENT_GLOBAL_SHADER_T(template<>, TCurveMeshProgram<true>);
	IMPLEMENT_GLOBAL_SHADER(MeshNormalVisualizeProgram);

	CurveRenderer::CurveRenderer()
	{

		setAxisRange(0, Range(-10, 10));
		setAxisRange(1, Range(-10, 10));
		setAxisRange(2, Range(-10, 10));

		mAxis[0].numData = 10;
		mAxis[1].numData = 10;
		mAxis[2].numData = 10;
	}

	bool CurveRenderer::initialize( Vec2i const& screenSize )
	{
		mProgCurveMesh = ShaderManager::Get().getGlobalShaderT< TCurveMeshProgram<false> >(true);
		if( mProgCurveMesh == nullptr )
			return false;
		mProgCurveMeshOIT = ShaderManager::Get().getGlobalShaderT< TCurveMeshProgram<true> >(true);
		if( mProgCurveMeshOIT == nullptr )
			return false;
		mProgMeshNormalVisualize = ShaderManager::Get().getGlobalShaderT< MeshNormalVisualizeProgram >(true);
		if( mProgMeshNormalVisualize == nullptr )
			return false;
		if( !mOITTech.init(screenSize) )
			return false;
		return true;
	}

	void CurveRenderer::beginRender()
	{
		mTranslucentDraw.clear();

		//#TODO : REMOVE
		glMatrixMode(GL_PROJECTION);
		glLoadMatrixf(mViewInfo.viewToClip);
		glMatrixMode(GL_MODELVIEW);
		glLoadMatrixf( mViewInfo.worldToView );
	}

	void CurveRenderer::endRender()
	{
		if( !mTranslucentDraw.empty() )
		{
			auto DrawFun = [this]()
			{
				for( auto& fun : mTranslucentDraw )
				{
					fun();
				}
			};
			mOITTech.renderInternal(mViewInfo, DrawFun, nullptr);
		}
	}



	void CurveRenderer::drawSurface(Surface3D& surface)
	{
		if( !surface.getFunction()->isParsed() )
			return;

		if( surface.needDrawLine() )
		{
			glEnable(GL_POLYGON_OFFSET_FILL);
			glPolygonOffset(1, 1);

			Color4f const& surfaceColor = surface.getColor();
			Color4f const color = Color4f(1 - surfaceColor.r, 1 - surfaceColor.g, 1 - surfaceColor.b);
			RHISetupFixedPipelineState(mViewInfo.worldToView, mViewInfo.viewToClip);
			drawMeshLine( surface , color );

			glDisable(GL_POLYGON_OFFSET_FILL);
		}

		if( surface.needDrawMesh() )
		{
			if( surface.getColor().a < 1.0 )
			{
				auto DrawFun = [this, &surface]()
				{
					GL_BIND_LOCK_OBJECT(*mProgCurveMeshOIT);
					mViewInfo.setupShader(*mProgCurveMeshOIT);
					mOITTech.setupShader(*mProgCurveMeshOIT);

					RHISetRasterizerState(TStaticRasterizerState<ECullMode::None>::GetRHI());					
					drawMesh(surface);
				};
				mTranslucentDraw.push_back(DrawFun);

			}
			else
			{
				GL_BIND_LOCK_OBJECT(*mProgCurveMesh);
				mViewInfo.setupShader(*mProgCurveMesh);
				drawMesh(surface);
			}
		}
		if( surface.needDrawNormal() || 1)
		{
			drawMeshNormal(surface, 0.1);
		}
	}
	template< bool bHaveNormal >
	struct LineDrawer
	{
		typedef TRenderRT< bHaveNormal ? RTVF_XYZ_CA_N : RTVF_XYZ_CA > MyRender;

		static void Draw(Surface3D& surface , LinearColor const& color)
		{
			RenderData* data = surface.getRenderData();
			assert(data);
			int    numDataU = surface.getParamU().getNumData();
			int    numDataV = surface.getParamV().getNumData();

			int d = int(1.0f / surface.getMeshLineDensity());
			d = 1;
			int nx = numDataU / d;
			int ny = numDataV / d;
			MyRender::BindVertexArray(data->getVertexData(), data->getVertexSize() , &color );
			for( int i = 0; i < ny; ++i )
			{
				glDrawArrays(GL_LINE_STRIP, numDataU * i, numDataU);
			}
			MyRender::UnbindVertexArray();

			MyRender::BindVertexArray(data->getVertexData(), data->getVertexSize(), &color);
			for( int i = 0; i < nx; ++i )
			{
				glDrawArrays(GL_LINE_STRIP, numDataU * i, numDataU);
			}
			MyRender::UnbindVertexArray();
		}
	};

	void CurveRenderer::drawMeshLine(Surface3D& surface , LinearColor const& color )
	{
		if( surface.getMeshLineDensity() < 1e-6 )
			return;

		RenderData* data = surface.getRenderData();
		assert(data);
#if 0
		if( data->getNormalOffset() != -1 )
		{
			LineDrawer<true>::Draw(surface , color);
		}
		else
		{
			LineDrawer<false>::Draw(surface , color);
		}


#else

		Vector3* pPositionData = (Vector3*)( data->getVertexData() + data->getPositionOffset());

		int    numDataU = surface.getParamU().getNumData();
		int    numDataV = surface.getParamV().getNumData();

		int d = int(1.0f / surface.getMeshLineDensity());
		int nx = numDataU / d;
		int ny = numDataV / d;

		glColor4fv(color);
		glEnableClientState(GL_VERTEX_ARRAY);
		glVertexPointer(3, GL_FLOAT, data->getVertexSize() , pPositionData);
		for( int n = 0; n < nx; ++n )
			glDrawArrays(GL_LINE_STRIP, numDataV * n * d, numDataV);
		for( int n = 0; n < ny; ++n )
		{
			glBegin(GL_LINE_STRIP);
			for( int i = 0; i < numDataU; ++i )
				glArrayElement(numDataV * i + n * d);
			glEnd();
		}
		glDisableClientState(GL_VERTEX_ARRAY);
#endif
	}

	void CurveRenderer::drawMesh(Surface3D& surface)
	{
		RenderData* data = surface.getRenderData();
		assert(data);

		uint8* vertexData = data->getVertexData();
		assert(vertexData);

		int const stride = data->getVertexSize();
		if( data->getNormalOffset() != -1 )
		{
			TRenderRT< RTVF_XYZ_CA_N >::DrawIndexedShader(PrimitiveType::TriangleList, vertexData, data->getVertexNum(), data->getIndexData(), data->getIndexNum() , data->getVertexSize());
		}
		else
		{
			TRenderRT< RTVF_XYZ_CA >::DrawIndexedShader(PrimitiveType::TriangleList, vertexData, data->getVertexNum(), data->getIndexData(), data->getIndexNum(), data->getVertexSize());
		}
	}


	void CurveRenderer::drawAxis()
	{
		glBegin(GL_LINES);

		glColor3f(1, 0, 0);
		glVertex3f(mAxis[0].range.Min, mAxis[1].range.Min, mAxis[2].range.Min);
		glVertex3f(mAxis[0].range.Max, mAxis[1].range.Min, mAxis[2].range.Min);

		glColor3f(0, 1, 0);
		glVertex3f(mAxis[0].range.Min, mAxis[1].range.Min, mAxis[2].range.Min);
		glVertex3f(mAxis[0].range.Min, mAxis[1].range.Max, mAxis[2].range.Min);

		glColor3f(0, 0, 1);
		glVertex3f(mAxis[0].range.Min, mAxis[1].range.Min, mAxis[2].range.Min);
		glVertex3f(mAxis[0].range.Min, mAxis[1].range.Min, mAxis[2].range.Max);

		glEnd();
		glBegin(GL_LINES);

		glColor3f(1, 0, 0);
		glVertex3f(mAxis[0].range.Min, 0, 0);
		glVertex3f(mAxis[0].range.Max, 0, 0);

		glColor3f(0, 1, 0);
		glVertex3f(0, mAxis[1].range.Min, 0);
		glVertex3f(0, mAxis[1].range.Max, 0);

		glColor3f(0, 0, 1);
		glVertex3f(0, 0, mAxis[2].range.Min);
		glVertex3f(0, 0, mAxis[2].range.Max);

		glEnd();

		glEnable(GL_LINE_STIPPLE);
		glLineStipple(1, 0xAAAA);  /*  dotted  */
		glColor3f(0, 0, 0);

		glPushMatrix();
		glTranslatef(0, 0, mAxis[2].getRangeMin());
		drawCoordText(mAxis[0], DRAW_MAX, 0.5, mAxis[1], DRAW_MAX, 1);
		drawCoordinates(mAxis[0], mAxis[1]);
		glPopMatrix();

		glPushMatrix();
		glTranslatef(mAxis[0].getRangeMin(), 0, 0);
		glRotatef(-90, 0, 1, 0);
		drawCoordText(mAxis[2], DRAW_MAX, 0.5, mAxis[1], DRAW_MAX, 0.5);
		drawCoordinates(mAxis[2], mAxis[1]);
		glPopMatrix();

		glPushMatrix();
		glTranslatef(0, mAxis[1].getRangeMin(), 0);
		glRotatef(90, 1, 0, 0);
		drawCoordText(mAxis[0], DRAW_MAX, 0.5, mAxis[2], DRAW_MAX, 1);
		drawCoordinates(mAxis[0], mAxis[2]);
		glPopMatrix();

		glDisable(GL_LINE_STIPPLE);

	}


	void CurveRenderer::drawVertexPoint(ShapeBase& shape)
	{
		RenderData* data = shape.getRenderData();
		assert(data);
		TRenderRT< RTVF_XYZ_CA >::Draw(PrimitiveType::Points, data->getVertexData(), data->getVertexNum(), data->getVertexSize());
	}


	void CurveRenderer::drawMeshNormal(Surface3D& surface , float length )
	{
		RenderData* data = surface.getRenderData();
		assert(data);

		if( data->getNormalOffset() == -1 )
			return;

		int d = std::max(1, int(1.0f / surface.getMeshLineDensity()));
		GL_BIND_LOCK_OBJECT(*mProgMeshNormalVisualize);
		mProgMeshNormalVisualize->setParameters( Vector4(1,0,0,1) , length , d , surface.getParamU().getNumData());
		mViewInfo.setupShader(*mProgMeshNormalVisualize);
		TRenderRT< RTVF_XYZ_CA_N >::DrawShader(PrimitiveType::Points, data->getVertexData(), data->getVertexNum(), data->getVertexSize());

	}

	void CurveRenderer::drawCoordinates(SampleParam const& axis1, SampleParam const& axis2)
	{
		float d1 = axis1.getIncrement();
		float d2 = axis2.getIncrement();

		glBegin(GL_LINES);
		float x1 = axis1.getRangeMin();
		for( int i = 0; i < axis1.getNumData(); ++i )
		{
			x1 += d1;
			glVertex2f(x1, axis2.getRangeMin());
			glVertex2f(x1, axis2.getRangeMax());
		}

		float x2 = axis2.getRangeMin();
		for( int i = 0; i < axis2.getNumData(); ++i )
		{
			x2 += d2;
			glVertex2f(axis1.getRangeMin(), x2);
			glVertex2f(axis1.getRangeMax(), x2);
		}
		glEnd();
	}

	void CurveRenderer::drawCoordText(SampleParam const& axis1, int drawMode1, float offest1,
								   SampleParam const& axis2, int drawMode2, float offest2)
	{
		char temp[32];
		if( drawMode1 != DRAW_NONE )
		{
			float d1 = axis1.getIncrement();
			float u1 = axis2.getRangeMax();
			if( drawMode1 == DRAW_MIN ) u1 = axis2.getRangeMin();

			float x1 = axis1.getRangeMin();
			for( int i = 0; i < axis1.getNumData() + 1; ++i )
			{
				sprintf(temp, "%.2f", x1);
				glRasterPos2f(x1, u1 + offest1);
				//m_Font.Render(temp);
				x1 += d1;
			}

		}

		if( drawMode2 != DRAW_NONE )
		{
			float d2 = axis2.getIncrement();
			float u2 = axis1.getRangeMax();
			if( drawMode2 == DRAW_MIN ) u2 = axis1.getRangeMin();

			float x2 = axis2.getRangeMin();
			for( int i = 0; i < axis2.getNumData() + 1; ++i )
			{
				sprintf(temp, "%.2f", x2);
				glRasterPos2f(u2 + offest2, x2);
				//m_Font.Render(temp);
				x2 += d2;
			}
		}

	}

	void CurveRenderer::reloadShaer()
	{
		ShaderManager::Get().reloadShader(*mProgCurveMesh);
		ShaderManager::Get().reloadShader(*mProgCurveMeshOIT);
		ShaderManager::Get().reloadShader(*mProgMeshNormalVisualize);
	}

	void CurveRenderer::drawShape(ShapeBase& shape)
	{
		class DarwShapeVisitor : public ShapeVisitor
		{
		public:
			CurveRenderer& drawer;
			DarwShapeVisitor(CurveRenderer& drawer_) :drawer(drawer_) {}
			void visit(Surface3D& surface) { drawer.drawSurface(surface); }
			void visit(Curve3D& curve) { drawer.drawCurve3D(curve); }
		};

		DarwShapeVisitor visitor(*this);
		shape.acceptVisit(visitor);
	}

	void CurveRenderer::drawCurve3D(Curve3D& curve)
	{
		RenderData* data = curve.getRenderData();
		assert(data);
		glLineWidth(2);
		TRenderRT< RTVF_XYZ_CA >::Draw(PrimitiveType::LineStrip, data->getVertexData(), data->getVertexNum(), data->getVertexSize());
		glLineWidth(1);
	}

}//namespace CB