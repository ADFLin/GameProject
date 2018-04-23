#pragma once
#ifndef CurveRenderer_H_519622D3_874D_45A6_A5B1_DA76C86476A1
#define CurveRenderer_H_519622D3_874D_45A6_A5B1_DA76C86476A1

#include "ShapeCommon.h"

#include <cassert>
#include <cmath>
#include <vector>

#include "gl/glew.h"
//#include <gl/wglew.h>
//#include "FTGL/FTGLPixmapFont.h"

#include "RenderGL/GLCommon.h"
#include "RenderGL/RenderContext.h"
#include "RenderGL/SceneRenderer.h"

namespace CB
{
	using namespace RenderGL;

	class ShapeBase;
	class Surface3D;
	class Curve3D;

	enum
	{
		DRAW_MAX,
		DRAW_MIN,
		DRAW_NONE,
	};

	class CurveRenderer
	{
	public:
		CurveRenderer();

		bool initialize( Vec2i const& screenSize );

		void drawVertexPoint(ShapeBase& shape);
		void drawShape( ShapeBase& shape);

		void drawSurface(Surface3D& surface);
		void drawCurve3D(Curve3D& curve);

		void showData(int nx, int ny);

		void drawMesh(Surface3D& surface);
		void drawMeshLine(Surface3D& surface);
		void drawMeshNormal(Surface3D& surface , float length );


		void drawAxis();

		void setAxisRange(int axis, Range const& range)
		{
			assert(axis >= 0 && axis < 3);
			mAxis[axis].range = range;
		}
		Range const& getAxisRange(int axis) const
		{
			assert(axis >= 0 && axis < 3);
			return mAxis[axis].range;
		}
		void drawCoordinates(SampleParam const& axis1, SampleParam const& axis2);
		void drawCoordText(SampleParam const& axis1, int drawMode1, float offest1,
						   SampleParam const& axis2, int drawMode2, float offest2);

		ViewInfo& getViewInfo() { return mViewInfo; }

		void reloadShaer();
	private:

		std::vector< unsigned > mIndexArray;
		SampleParam mAxis[3];
		ViewInfo mViewInfo;
		class CurveMeshProgram* mProgCurveMesh;
		class CurveMeshProgram* mProgCurveMeshOIT;

		OITTechnique mOITTech;
		//FTGLPixmapFont m_Font;
	};

}//namespace CB


#endif // CurveRenderer_H_519622D3_874D_45A6_A5B1_DA76C86476A1