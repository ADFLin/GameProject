#ifndef GLGraphics2D_h__
#define GLGraphics2D_h__

#include "RHI/Font.h"
#include "GLConfig.h"
#include "Math/Vector2.h"
#include "Graphics2DBase.h"
#include "Core/IntegerType.h"
#include "RHI/SimpleRenderState.h"


#include <algorithm>
#include <cmath>
#include <vector>
#include "Math/Matrix2.h"
#include "Math/Matrix4.h"


namespace Render
{
	using ::Math::Matrix2;
	struct RenderTransform2D
	{
		Matrix2 M;
		Vector2 P;

		static RenderTransform2D Identity() { return { Matrix2::Identity() , Vector2::Zero() }; }
		FORCEINLINE RenderTransform2D operator * (RenderTransform2D const& rhs) const
		{
			return { M * rhs.M , rhs.M.leftMul(P) + P };
		}

		FORCEINLINE Vector2 tranformPosition(Vector2 const& v) const
		{
			return P + M.leftMul(v);
		}

		FORCEINLINE Vector2 tranformVector(Vector2 const& v) const
		{
			return M.leftMul(v);
		}

		FORCEINLINE void applyTranslate(Vector2 const& offset)
		{
			P += offset;
		}

		FORCEINLINE void applyRotate(float angle)
		{
			M = M * Matrix2::Rotate(angle);
		}

		FORCEINLINE void applyScale(Vector2 const& scale)
		{
			M = M * Matrix2::Scale(scale);
		}

		FORCEINLINE Matrix4 toMatrix4() const
		{
			return Matrix4(
				M[0], M[1], 0, 0,
				M[2], M[3], 0, 0,
				   0,    0, 1, 0,
				 P.x,  P.y, 0, 1);
		}
	};


	struct TransformStack2D
	{

		void clear()
		{
			mStack.clear();
			mCurrent = RenderTransform2D::Identity();
		}

		FORCEINLINE void set(RenderTransform2D const& xform)
		{
			mCurrent = xform;
		}

		FORCEINLINE void setIdentity()
		{
			mCurrent = RenderTransform2D::Identity();
		}

		FORCEINLINE void transform(RenderTransform2D const& xform)
		{
			mCurrent = xform * mCurrent;
		}

		FORCEINLINE void translate(Vector2 const& offset)
		{
			mCurrent.applyTranslate(offset);
		}

		FORCEINLINE void rotate(float angle)
		{
			mCurrent.applyRotate(angle);
		}

		FORCEINLINE void scale(Vector2 const& scale)
		{
			mCurrent.applyScale(scale);
		}

		void push()
		{
			assert(!mStack.empty());
			mStack.push_back(mCurrent);
		}

		void pop()
		{
			assert(!mStack.empty());
			mCurrent = mStack.back();
			mStack.pop_back();
		}

		RenderTransform2D const& get() { return mCurrent; }

		RenderTransform2D mCurrent;
		std::vector< RenderTransform2D > mStack;
	};
}

using Vector2 = Math::Vector2;
using GLFont = Render::FontDrawer;
using GLTexture2D = Render::RHITexture2D;
using RenderTransform2D = Render::RenderTransform2D;
using TransformStack2D = Render::TransformStack2D;

class GLGraphics2D
{
public:
	GLGraphics2D();


	void  init( int w , int h );
	void  beginXForm();
	void  finishXForm();

	void  pushXForm();
	void  popXForm();

	void  identityXForm();
	void  translateXForm( float ox , float oy );
	void  rotateXForm( float angle );
	void  scaleXForm( float sx , float sy  );

	void  beginRender();
	void  endRender();

	void  enablePen( bool beE ){ mDrawPen = beE; }
	void  enableBrush( bool beE ){ mDrawBrush = beE; }

	void  setPen( Color3f const& color )
	{
		mColorPen = color;
	}
	void  setPen( Color3ub const& color , int width  )  
	{
		mColorPen = color;
		if ( mWidthPen != width )
		{
			glLineWidth( width );
			mWidthPen = width;
		}
	}

	void  setBrush(Color3f const& color)
	{
		mColorBrush = color;
	}

	void  beginClip(Vec2i const& pos, Vec2i const& size);
	void  endClip();

	void  beginBlend( Vector2 const& pos , Vector2 const& size , float alpha );
	void  endBlend();

	void  drawPixel  ( Vector2 const& p , Color3ub const& color );
	void  drawLine   ( Vector2 const& p1 , Vector2 const& p2 );

	

	void  drawRect   ( int left , int top , int right , int bottom );
	void  drawRect   ( Vector2 const& pos , Vector2 const& size );
	void  drawCircle ( Vector2 const& center , float r );
	void  drawEllipse( Vector2 const& center , Vector2 const& size );
	void  drawPolygon( Vector2 pos[] , int num );
	void  drawPolygon( Vec2i pos[] , int num );
	void  drawRoundRect( Vector2 const& pos , Vector2 const& rectSize , Vector2 const& circleSize );

	void  fillGradientRect(Vector2 const& posLT, Color3ub const& colorLT,
						   Vector2 const& posRB, Color3ub const& colorRB, bool bHGrad);


	void  drawTexture(GLTexture2D& texture, Vector2 const& pos, Color3ub const& color = Color3ub(255, 255, 255));
	void  drawTexture(GLTexture2D& texture, Vector2 const& pos, Vector2 const& size , Color3ub const& color = Color3ub(255,255,255) );
	void  drawTexture(GLTexture2D& texture, Vector2 const& pos, Vector2 const& texPos, Vector2 const& texSize, Color3ub const& color);
	void  drawTexture(GLTexture2D& texture, Vector2 const& pos, Vector2 const& size, Vector2 const& texPos , Vector2 const& texSize , Color3ub const& color );

	void  setFont( GLFont& font )
	{
		mFont = &font;
	}
	void  setTextColor(Color3ub const& color);
	void  drawText( Vector2 const& pos , char const* str );
	void  drawText( Vector2 const& pos , Vector2 const& size , char const* str , bool beClip = false );
	void  drawText( float x , float y , char const* str ){ drawText( Vector2(x,y) , str ); }

	

private:
	void emitLineVertex(Vector2 const &p1, Vector2 const &p2);
	void emintRectVertex( Vector2 const& p1 , Vector2 const& p2 );
	void emitPolygonVertex( Vector2 pos[] , int num );
	void emitPolygonVertex(Vec2i pos[] , int num);
	void emitCircleVertex(float cx, float cy, float r, int numSegment);
	void emitEllipseVertex(float cx, float cy, float r , float yFactor , int numSegment);
	void emitRoundRectVertex( Vector2 const& pos , Vector2 const& rectSize , Vector2 const& circleSize );
	void emitVertex( float x , float y );
	void emitVertex( float v[] );
	void drawTextImpl(float ox, float oy, char const* str);

	void drawPolygonBuffer();
	void drawLineBuffer();

	Render::RHICommandList& GetCommandList();
	int       mWidth;
	int       mHeight;

	Color3f   mColorPen;
	Color3f   mColorBrush;
	Color3f   mColorFont;
	float     mAlpha;

	unsigned  mWidthPen;
	bool      mDrawBrush;
	bool      mDrawPen;
	GLuint    mColorKeyShader;
	GLFont*   mFont;
	std::vector< float > mBuffer;
	void checkPipelineState();
	bool     mbResetPipelineState;
	Math::Matrix4   mBaseTransform;

	TransformStack2D mXFormStack;

	
};

#endif // GLGraphics2D_h__
