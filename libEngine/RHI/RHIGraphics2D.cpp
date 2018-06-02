#include "RHIGraphics2D.h"
#include "Math/Base.h"



#include <cassert>
#include <algorithm>

namespace RenderGL
{
#if 1
#define DRAW_LINE_IMPL( EXPR )\
	mBuffer.clear();\
	EXPR;\
	drawLineBuffer();

#define DRAW_IMPL( EXPR )\
	mBuffer.clear();\
	EXPR;\
	drawPolygonBuffer();
#endif


	static inline int calcCircleSemgmentNum(int r)
	{
		return std::max(4 * (r / 2), 32);
	}


	RHIGraphics2D::RHIGraphics2D()
		:mWidth(1)
		, mHeight(1)
	{
		mFont = NULL;
		mWidthPen = 1;
		mDrawBrush = true;
		mDrawPen = true;
		mAlpha = 1.0;
		mColorKeyShader = 0;
		mColorBrush = Color3f(0, 0, 0);
		mColorPen = Color3f(0, 0, 0);
		mColorFont = Color3f(0, 0, 0);

		mDepth = 0;
		mDepthDelta = 0.01;
	}

	void RHIGraphics2D::init(int w, int h)
	{
		mWidth = w;
		mHeight = h;
	}

	void RHIGraphics2D::beginXForm()
	{

	}

	void RHIGraphics2D::finishXForm()
	{

	}

	void RHIGraphics2D::beginRender()
	{
		glPushAttrib(GL_ENABLE_BIT | GL_VIEWPORT_BIT);
		
		RHISetDepthStencilState(TStaticDepthStencilState<>::GetRHI());
		//glDisable(GL_DEPTH_TEST);
		glDisable(GL_CULL_FACE);

		RHISetViewport(0, 0, mWidth, mHeight);
		glMatrixMode(GL_PROJECTION);
		glPushMatrix();
		glLoadIdentity();
		glOrtho(0, mWidth, mHeight, 0, -1000, 1000);
		glMatrixMode(GL_MODELVIEW);
		glPushMatrix();
		glLoadIdentity();

		mDepth = 0;
	}

	void RHIGraphics2D::endRender()
	{
		flushCommand();

		glFlush();
		glMatrixMode(GL_MODELVIEW);
		glPopMatrix();
		glMatrixMode(GL_PROJECTION);
		glPopMatrix();
		glMatrixMode(GL_MODELVIEW);

		glPopAttrib();
	}

	void RHIGraphics2D::emitPolygonVertex(Vector2 pos[], int num)
	{
		mBuffer.insert(mBuffer.end(), pos, pos + num);
		for( int i = 0; i < num; ++i )
		{
			emitVertex(pos[i]);
		}
	}

	void RHIGraphics2D::emitPolygonVertex(Vec2i pos[], int num)
	{
		mBuffer.insert(mBuffer.end(), pos, pos + num);
	}

	void RHIGraphics2D::emintRectVertex(Vector2 const& p1, Vector2 const& p2)
	{
		emitVertex(p1);
		emitVertex(Vector2( p2.x, p1.y ) );
		emitVertex(p2);
		emitVertex(Vector2( p1.x, p2.y ));
	}

	void RHIGraphics2D::emitCircleVertex(float cx, float cy, float r, int numSegment)
	{
		float theta = 2 * 3.1415926 / float(numSegment);
		float c = cos(theta);//precalculate the sine and cosine
		float s = sin(theta);

		float x = r;//we start at angle = 0 
		float y = 0;

		for( int i = 0; i < numSegment; ++i )
		{
			Vector2 v;
			v.x = cx + x;
			v.y = cy + y;
			emitVertex(v);

			//apply the rotation matrix
			float temp = x;
			x = c * temp - s * y;
			y = s * temp + c * y;
		}
	}

	void RHIGraphics2D::emitEllipseVertex(float cx, float cy, float r, float yFactor, int numSegment)
	{
		float theta = 2 * 3.1415926 / float(numSegment);
		float c = cos(theta);//precalculate the sine and cosine
		float s = sin(theta);

		float x = r;//we start at angle = 0 
		float y = 0;
		for( int i = 0; i < numSegment; ++i )
		{
			Vector2 v;
			v.x = cx + x;
			v.y = cy + yFactor * y;
			emitVertex(v);
			float temp = x;
			x = c * temp - s * y;
			y = s * temp + c * y;
		}
	}

	void RHIGraphics2D::emitRoundRectVertex(Vector2 const& pos, Vector2 const& rectSize, Vector2 const& circleSize)
	{
		int numSegment = calcCircleSemgmentNum((circleSize.x + circleSize.y) / 2);
		float yFactor = float(circleSize.y) / circleSize.x;
		int num = numSegment / 4;

		float theta = 2 * Math::PI / float(numSegment);
		float c = cos(theta);
		float s = sin(theta);

		float cvn[4][2] =
		{
			float(pos.x + rectSize.x - circleSize.x) , float(pos.y + rectSize.y - circleSize.y) ,
			float(pos.x + circleSize.x) , float(pos.y + rectSize.y - circleSize.y) ,
			float(pos.x + circleSize.x) , float(pos.y + circleSize.y) ,
			float(pos.x + rectSize.x - circleSize.x) , float(pos.y + circleSize.y) ,
		};

		Vector2 v;
		float x, y;
		float cx = cvn[0][0];
		float cy = cvn[0][1];
		for( int n = 0; n < 4; ++n )
		{
			switch( n )
			{
			case 0: x = circleSize.x; y = 0; break;
			case 1: y = circleSize.x; x = 0; break;
			case 2: x = -circleSize.x; y = 0; break;
			case 3: y = -circleSize.x; x = 0; break;
			}
			for( int i = 0; i < num; ++i )
			{
				v.x = cx + x;
				v.y = cy + yFactor * y;
				emitVertex(v);

				float temp = x;
				x = c * temp - s * y;
				y = s * temp + c * y;
			}
			int next = (n == 3) ? 0 : n + 1;
			cx = cvn[next][0];
			cy = cvn[next][1];
			v.x = cx + x;
			v.y = cy + yFactor * y;
			emitVertex(v);
		}
	}


	void RHIGraphics2D::emitLineVertex(Vector2 const &p1, Vector2 const &p2)
	{
		emitVertex(p1);
		emitVertex(p2);
	}

	void RHIGraphics2D::drawPixel(Vector2 const& p, Color3ub const& color)
	{
		glBegin(GL_POINTS);
		glVertex2i(p.x, p.y);
		glEnd();
	}

	void RHIGraphics2D::drawRect(int left, int top, int right, int bottom)
	{
		Vector2 p1(left, top);
		Vector2 p2(right, bottom);
		DRAW_IMPL(emintRectVertex(p1, p2));
	}

	void RHIGraphics2D::drawRect(Vector2 const& pos, Vector2 const& size)
	{
		Vector2 p2 = pos + size;
		DRAW_IMPL(emintRectVertex(pos, p2));
	}

	void RHIGraphics2D::drawCircle(Vector2 const& center, float r)
	{
		int numSeg = std::max(2 * r * r, 4.0f);
		DRAW_IMPL(emitCircleVertex(center.x, center.y, r, calcCircleSemgmentNum(r)));
	}

	void RHIGraphics2D::drawEllipse(Vector2 const& center, Vector2 const& size)
	{
		float yFactor = float(size.y) / size.x;
		DRAW_IMPL(emitEllipseVertex(center.x, center.y, size.x, yFactor, calcCircleSemgmentNum((size.x + size.y) / 2)));
	}

	void RHIGraphics2D::drawPolygon(Vector2 pos[], int num)
	{
		DRAW_IMPL(emitPolygonVertex(pos, num));
	}

	void RHIGraphics2D::drawPolygon(Vec2i pos[], int num)
	{
		DRAW_IMPL(emitPolygonVertex(pos, num));
	}

	void RHIGraphics2D::drawLine(Vector2 const& p1, Vector2 const& p2)
	{
		DRAW_LINE_IMPL(emitLineVertex(p1, p2));
	}

	void RHIGraphics2D::drawRoundRect(Vector2 const& pos, Vector2 const& rectSize, Vector2 const& circleSize)
	{
		DRAW_IMPL(emitRoundRectVertex(pos, rectSize, circleSize / 2));
	}

	void RHIGraphics2D::setTextColor(Color3ub const& color)
	{
		mColorFont = color;
	}

	void RHIGraphics2D::drawText(Vector2 const& pos, char const* str)
	{
		if( !mFont )
			return;
		int fontSize = mFont->getSize();
		float ox = pos.x;
		float oy = pos.y + fontSize;
		drawTextImpl(ox, oy, str);
	}

	void RHIGraphics2D::drawText(Vector2 const& pos, Vector2 const& size, char const* str, bool beClip /*= false */)
	{
		if( !mFont )
			return;
		int fontSize = mFont->getSize();
		int strLen = strlen(str);
		float ox = pos.x + (size.x - (3 * fontSize / 4) * strLen) / 2;
		float oy = pos.y + fontSize + (size.y - fontSize) / 2;
		drawTextImpl(ox, oy, str);
	}

	void RHIGraphics2D::drawTextImpl(float  ox, float  oy, char const* str)
	{
		assert(mFont);
		flushCommand();
		glColor4f(mColorFont.r, mColorFont.g, mColorFont.b, mAlpha);
		mFont->draw( Vector2(ox,oy) , str );
	}

	void RHIGraphics2D::drawPolygonBuffer()
	{
		assert(!mBuffer.empty());

		mBatchedDrawer.addPolygon(
			&mBuffer[0], mBuffer.size(),
			mDrawBrush ? &Color4f(mColorBrush, mAlpha) : nullptr, mDepth,
			mDrawPen ? &Color4f(mColorPen, mAlpha) : nullptr , mDepth + mDepthDelta);
		mDepth += 2 * mDepthDelta;
		mBuffer.clear();
	}

	void RHIGraphics2D::drawLineBuffer()
	{
		if( mDrawPen )
		{
			mBatchedDrawer.addPolygonLine(&mBuffer[0], mBuffer.size(), Color4f(mColorPen, mAlpha), mDepth);
			mDepth += mDepthDelta;
		}
		mBuffer.clear();
	}

	void RHIGraphics2D::drawTexture(GLTexture& texture, Vector2 const& pos)
	{

	}

	void RHIGraphics2D::drawTexture(GLTexture& texture, Vector2 const& pos, Color3ub const& color)
	{

	}

	void RHIGraphics2D::drawTexture(GLTexture& texture, Vector2 const& pos, Vector2 const& texPos, Vector2 const& texSize)
	{

	}

	void RHIGraphics2D::drawTexture(GLTexture& texture, Vector2 const& pos, Vector2 const& texPos, Vector2 const& texSize, Color3ub const& color)
	{

	}

	void RHIGraphics2D::beginBlend(Vector2 const& pos, Vector2 const& size, float alpha)
	{
		flushCommand();
		RHISetBlendState(TStaticBlendState< CWM_RGBA , Blend::eSrcAlpha , Blend::eOneMinusSrcAlpha >::GetRHI());
		mAlpha = alpha;
	}

	void RHIGraphics2D::endBlend()
	{
		flushCommand();
		RHISetBlendState(TStaticBlendState<>::GetRHI());
		mAlpha = 1.0f;
	}

	void BatchedDrawer::flushCommond()
	{
		if( !mSurfaceVertices.empty() && !mSurfaceIndices.empty() )
		{
			MyRender::BindVertexPointer((uint8*)&mSurfaceVertices[0], MyRender::GetVertexSize());
			RHIDrawIndexedPrimitive(PrimitiveType::TriangleList, CVT_UInt, (int)&mSurfaceIndices[0] , mSurfaceIndices.size());
			MyRender::UnbindVertexPointer();

			mSurfaceVertices.clear();
			mSurfaceIndices.clear();
		}

		if( !mLinesVertices.empty() && !mLineIndices.empty() )
		{
			MyRender::BindVertexPointer((uint8*)&mLinesVertices[0], MyRender::GetVertexSize());
			RHIDrawIndexedPrimitive(PrimitiveType::LineList, CVT_UInt, (int)&mLineIndices[0], mLineIndices.size());
			MyRender::UnbindVertexPointer();

			mLinesVertices.clear();
			mLineIndices.clear();
		}
	}


	void BatchedDrawer::addPolygon(Vector2 v[], int numV, Color4f const* color, float depth, Color4f const* lineColor, float lineDepth)
	{
		if( color )
		{
			int baseIndex = mSurfaceVertices.size();
			addVerticesInternal(mSurfaceVertices, v, numV, depth, *color);
			addPolygonIndex(baseIndex, numV);
		}
		if ( lineColor )
		{
			int baseIndex = mLinesVertices.size();
			addVerticesInternal(mLinesVertices, v, numV, lineDepth, *lineColor);
			addPolygonLineIndex(baseIndex, numV);
		}
	}

	void BatchedDrawer::addPolygonLine(Vector2 v[], int numV, Color4f const& color, float depth)
	{
		int baseIndex = mLinesVertices.size();
		addVerticesInternal(mLinesVertices, v, numV, depth, color);
		addPolygonLineIndex(baseIndex, numV);
	}

	void BatchedDrawer::reverseColoredVertex(int num)
	{
		mSurfaceVertices.reserve(mSurfaceVertices.size() + num);
	}

	void BatchedDrawer::addPolygonIndex(int numV, int baseIndex)
	{
		if( numV >= 3 )
		{
			int startIndex = mSurfaceIndices.size();

			int numTri = numV - 2;
			mSurfaceIndices.resize(startIndex + 3 * numTri);
			int* pIdx = &mSurfaceIndices[startIndex];
			for( int i = 0; i < numTri; ++i )
			{
				pIdx[0] = baseIndex;
				pIdx[1] = baseIndex + i + 1;
				pIdx[2] = baseIndex + i + 2;
				pIdx += 3;
			}
		}
	}

	void BatchedDrawer::addPolygonLineIndex(int numV, int baseIndex)
	{
		if( numV >= 1 )
		{
			int startIndex = mLineIndices.size();
			mLineIndices.resize(startIndex + 2 * numV);
			int* pIdx = &mLineIndices[startIndex];
			for( int i = 0; i < numV; ++i )
			{
				pIdx[0] = baseIndex + i;
				pIdx[1] = baseIndex + i + 1;
				pIdx += 2;
			}
			*(pIdx - 1) = baseIndex;
		}
	}

	void BatchedDrawer::addVerticesInternal(std::vector< ColoredVertex >& buffer, Vector2 v[], int numV, float depth, Color4f const& color)
	{
		if( numV <= 0 )
			return;

		int baseIndex = buffer.size();
		buffer.resize(baseIndex + numV);
		for( int i = 0; i < numV; ++i )
		{
			ColoredVertex& vtx = buffer[baseIndex + i];
			vtx.pos = Vector3(v[i].x, v[i].y, depth);
			vtx.color = color;
		}
	}

}

