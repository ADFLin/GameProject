#ifndef GLGraphics2D_h__
#define GLGraphics2D_h__

#include "GLConfig.h"

#include "Graphics2DBase.h"
#include "IntegerType.h"

#include <algorithm>
#include <cmath>
#include <vector>

typedef TVector2<float> Vec2f;

class GLTexture
{

	
};


class GLFont
{	
public:
	GLFont(){  mSize = 0; mIdBaseList = 0;  }
	~GLFont();
#ifdef SYS_PLATFORM_WIN
	explicit GLFont( int size , char const* faceName , HDC hDC ){ buildBaseImage( size , faceName , hDC ); }
	void create( int size , char const* faceName , HDC hDC);
private:
	void buildBaseImage( int size , char const* faceName , HDC hDC );
#endif
public:
	void release();
	void printf( char const* fmt, ...);
	void print( char const* str );
	int  getSize() const{ return mSize; }
private:
	GLuint	mIdBaseList;
	int     mSize;
};

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

	void  setPen( ColorKey3 const& color )  
	{
		mColorPen = color;
	}
	void  setPen( ColorKey3 const& color , int width  )  
	{
		mColorPen = color;
		if ( mWidthPen != width )
		{
			glLineWidth( width );
			mWidthPen = width;
		}
	}
	void  setBrush( ColorKey3 const& color )                
	{  
		mColorBrush = color;
	}

	void  beginBlend( Vec2f const& pos , Vec2f const& size , float alpha );
	void  endBlend();

	void  drawPixel  ( Vec2f const& p , ColorKey3 const& color );
	void  drawLine   ( Vec2f const& p1 , Vec2f const& p2 );

	

	void  drawRect   ( int left , int top , int right , int bottom );
	void  drawRect   ( Vec2f const& pos , Vec2f const& size );
	void  drawCircle ( Vec2f const& center , int r );
	void  drawEllipse( Vec2f const& center , Vec2f const& size );
	void  drawPolygon( Vec2f pos[] , int num );
	void  drawPolygon( Vec2i pos[] , int num );
	void  drawRoundRect( Vec2f const& pos , Vec2f const& rectSize , Vec2f const& circleSize );

	void  drawTexture( GLTexture& texture , Vec2f const& pos );
	void  drawTexture( GLTexture& texture , Vec2f const& pos , ColorKey3 const& color );
	void  drawTexture( GLTexture& texture , Vec2f const& pos , Vec2f const& texPos , Vec2f const& texSize );
	void  drawTexture( GLTexture& texture , Vec2f const& pos , Vec2f const& texPos , Vec2f const& texSize , ColorKey3 const& color );

	void  setFont( GLFont& font )
	{
		mFont = &font;
	}
	void  setTextColor( uint8 r , uint8 g, uint8 b );
	void  drawText( Vec2f const& pos , char const* str );
	void  drawText( Vec2f const& pos , Vec2f const& size , char const* str , bool beClip = false );
	void  drawText( float x , float y , char const* str ){ drawText( Vec2f(x,y) , str ); }

	

private:
	void emitLineVertex(Vec2f const &p1, Vec2f const &p2);
	void emintRectVertex( Vec2f const& p1 , Vec2f const& p2 );
	void emitPolygonVertex( Vec2f pos[] , int num );
	void emitPolygonVertex(Vec2i pos[] , int num);
	void emitCircleVertex(float cx, float cy, float r, int numSegment);
	void emitEllipseVertex(float cx, float cy, float r , float yFactor , int numSegment);
	void emitRoundRectVertex( Vec2f const& pos , Vec2f const& rectSize , Vec2f const& circleSize );
	void emitVertex( float x , float y );
	void emitVertex( float v[] );
	void drawTextImpl(float ox, float oy, char const* str);
	int       mWidth;
	int       mHeight;

	ColorKey3 mColorPen;
	ColorKey3 mColorBrush;
	ColorKey3 mColorFont;
	uint8     mAlpha;
	unsigned  mWidthPen;
	bool      mDrawBrush;
	bool      mDrawPen;
	GLuint    mColorKeyShader;
	GLFont*   mFont;

	std::vector< float > mBuffer;
};

#endif // GLGraphics2D_h__
