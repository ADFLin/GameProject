#ifndef RHIGraphics2D_h__
#define RHIGraphics2D_h__



#include "Graphics2DBase.h"
#include "GLCommon.h"
#include "Core/IntegerType.h"

#include "RHICommand.h"
#include "DrawUtility.h"

#include <algorithm>
#include <cmath>
#include <vector>

#include "GLGraphics2D.h"

namespace RenderGL
{
	class BatchedDrawer
	{
	public:

		typedef TRenderRT< RTVF_XYZ_CA > MyRender;

		void flushCommond();
		void addPolygon(Vector2 v[], int numV, Color4f const* color, float depth,  Color4f const* lineColor ,float lineDepth );
		void addPolygonLine(Vector2 v[], int numV, Color4f const& color, float depth);
		void reverseColoredVertex(int num);
		
		void addPolygonIndex( int numV , int baseIndex);
		void addPolygonLineIndex(int numV, int baseIndex);

		struct ColoredVertex
		{
			Vector3  pos;
			Color4f  color;
		};

		static void addVerticesInternal(std::vector< ColoredVertex >& buffer ,Vector2 v[], int numV, float depth, Color4f const& color);


		std::vector< ColoredVertex > mSurfaceVertices;
		std::vector< ColoredVertex > mLinesVertices;
		std::vector< int32 >         mSurfaceIndices;
		std::vector< int32 >         mLineIndices;

		struct TexVertex : ColoredVertex
		{
			float uv[2];
		};

		std::vector< TexVertex > mTexVertices;
		std::vector< int32 >     mTexSurfaceIndices;
	};

	class RHIGraphics2D
	{
	public:
		RHIGraphics2D();


		void  init(int w, int h);
		void  beginXForm();
		void  finishXForm();

		void  pushXForm() { glPushMatrix(); }
		void  popXForm() { glPopMatrix(); }

		void  identityXForm() { glLoadIdentity(); }
		void  translateXForm(float ox, float oy) { glTranslatef(ox, oy, 0); }
		void  rotateXForm(float angle) { glRotatef(angle, 0, 0, 1); }
		void  scaleXForm(float sx, float sy) { glScalef(sx, sy, 1); }

		void  beginRender();
		void  endRender();

		void  enablePen(bool beE) { mDrawPen = beE; }
		void  enableBrush(bool beE) { mDrawBrush = beE; }

		void  setPen(Color3ub const& color)
		{
			mColorPen = color;
		}
		void  setPen(Color3f const& color)
		{
			mColorPen = color;
		}
		void  setPen(Color3ub const& color, int width)
		{
			mColorPen = color;
			if( mWidthPen != width )
			{
				glLineWidth(width);
				mWidthPen = width;
			}
		}
		void  setBrush(Color3ub const& color)
		{
			mColorBrush = color;
		}
		void  setBrush(Color3f const& color)
		{
			mColorBrush = color;
		}

		void  beginClip(Vec2i const& pos, Vec2i const& size)
		{
			RHISetScissorRect(true, pos.x, pos.y, size.x, size.y);
		}
		void  endClip()
		{
			RHISetScissorRect(false);
		}

		void  beginBlend(Vector2 const& pos, Vector2 const& size, float alpha);
		void  endBlend();

		void  drawPixel(Vector2 const& p, Color3ub const& color);
		void  drawLine(Vector2 const& p1, Vector2 const& p2);



		void  drawRect(int left, int top, int right, int bottom);
		void  drawRect(Vector2 const& pos, Vector2 const& size);
		void  drawCircle(Vector2 const& center, float r);
		void  drawEllipse(Vector2 const& center, Vector2 const& size);
		void  drawPolygon(Vector2 pos[], int num);
		void  drawPolygon(Vec2i pos[], int num);
		void  drawRoundRect(Vector2 const& pos, Vector2 const& rectSize, Vector2 const& circleSize);

		void  drawTexture(GLTexture& texture, Vector2 const& pos);
		void  drawTexture(GLTexture& texture, Vector2 const& pos, Color3ub const& color);
		void  drawTexture(GLTexture& texture, Vector2 const& pos, Vector2 const& texPos, Vector2 const& texSize);
		void  drawTexture(GLTexture& texture, Vector2 const& pos, Vector2 const& texPos, Vector2 const& texSize, Color3ub const& color);

		void  setFont(FontDrawer& font)
		{
			mFont = &font;
		}
		void  setTextColor(Color3ub const& color);
		void  drawText(Vector2 const& pos, char const* str);
		void  drawText(Vector2 const& pos, Vector2 const& size, char const* str, bool beClip = false);
		void  drawText(float x, float y, char const* str) { drawText(Vector2(x, y), str); }



	private:
		void emitLineVertex(Vector2 const &p1, Vector2 const &p2);
		void emintRectVertex(Vector2 const& p1, Vector2 const& p2);
		void emitPolygonVertex(Vector2 pos[], int num);
		void emitPolygonVertex(Vec2i pos[], int num);
		void emitCircleVertex(float cx, float cy, float r, int numSegment);
		void emitEllipseVertex(float cx, float cy, float r, float yFactor, int numSegment);
		void emitRoundRectVertex(Vector2 const& pos, Vector2 const& rectSize, Vector2 const& circleSize);
		void emitVertex(Vector2 const& v)
		{
			mBuffer.push_back(v);
		}
		void emitVertex(float v[]);
		void drawTextImpl(float ox, float oy, char const* str);

		void drawPolygonBuffer();
		void drawLineBuffer();

		void   flushCommand()
		{
			mBatchedDrawer.flushCommond();
			mDepth = 0;
		}
		int       mWidth;
		int       mHeight;

		float     mDepth;
		float     mDepthDelta;
		BatchedDrawer mBatchedDrawer;
		Color3f   mColorPen;
		Color3f   mColorBrush;
		Color3f   mColorFont;
		float     mAlpha;

		unsigned  mWidthPen;
		bool      mDrawBrush;
		bool      mDrawPen;
		GLuint    mColorKeyShader;
		FontDrawer*   mFont;
		std::vector< Vector2 > mBuffer;
	};


}


#endif // RHIGraphics2D_h__

