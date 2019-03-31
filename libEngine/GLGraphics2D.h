#ifndef GLGraphics2D_h__
#define GLGraphics2D_h__

#include "RHI/Font.h"
#include "GLConfig.h"
#include "Math/Vector2.h"
#include "Graphics2DBase.h"
#include "Core/IntegerType.h"



#include <algorithm>
#include <cmath>
#include <vector>

typedef Math::Vector2 Vector2;
typedef Render::FontDrawer GLFont;
typedef Render::RHITexture2D GLTexture2D; 

class GLGraphics2D
{
public:
	GLGraphics2D();


	void  init( int w , int h );
	void  beginXForm();
	void  finishXForm();

	void  pushXForm(){ glPushMatrix(); }
	void  popXForm(){ glPopMatrix(); }

	void  identityXForm(){ glLoadIdentity(); }
	void  translateXForm( float ox , float oy ){  glTranslatef( ox , oy , 0 );  }
	void  rotateXForm( float angle ){  glRotatef( angle , 0 , 0 , 1 );  }
	void  scaleXForm( float sx , float sy  ){ glScalef( sx , sy , 1 ); }

	void  beginRender();
	void  endRender();

	void  enablePen( bool beE ){ mDrawPen = beE; }
	void  enableBrush( bool beE ){ mDrawBrush = beE; }

	void  setPen( Color3ub const& color )  
	{
		mColorPen = color;
	}
	void  setPen(Color3f const& color)
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
	void  setBrush( Color3ub const& color )                
	{  
		mColorBrush = color;
	}
	void  setBrush(Color3f const& color)
	{
		mColorBrush = color;
	}

	void  beginClip(Vec2i const& pos, Vec2i const& size)
	{
		glEnable(GL_SCISSOR_TEST);
		glScissor(pos.x, mHeight - pos.y - size.y , size.x,  size.y);
	}
	void  endClip()
	{
		glDisable(GL_SCISSOR_TEST);
	}

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
};

#endif // GLGraphics2D_h__
