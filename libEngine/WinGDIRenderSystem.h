#ifndef RenderSystemWinGDI_h__
#define RenderSystemWinGDI_h__

#include "Win32Header.h"
#include "IntegerType.h"

#include "Graphics2DBase.h"
#include "BitmapDC.h"


#include <cassert>

class GdiTexture
{
public:
	GdiTexture(){}
	bool  create( HDC hDC , int width , int height , void** data );
	bool  createFromFile( HDC hDC , char const* path );

	Vec2i getSize()   const { return Vec2i( getWidth() , getHeight() ); }
	int   getWidth()  const { return mImpl.getWidth(); }
	int   getHeight() const { return mImpl.getHeight(); }
	uint8* getRawData();

private:

	friend class WinGdiGraphics2D;
	friend class WinGdiRenderSystem;
	BitmapDC mImpl;
};


class WinGdiGraphics2D
{
public:
	WinGdiGraphics2D( HDC hDC = NULL );

	void  beginXForm();
	void  finishXForm();

	void  pushXForm();
	void  popXForm();
	void  identityXForm();
	void  translateXForm( float ox , float oy );
	void  rotateXForm( float angle );
	void  scaleXForm( float sx , float sy  );

	void  setPen( ColorKey3 const& color , int width = 1 )  {  _setPenImpl( ::CreatePen( PS_SOLID , width , color) , true );  }
	void  setPen( HPEN hPen , bool beManaged = false )      {  _setPenImpl( hPen , beManaged ); }
	void  setBrush( ColorKey3 const& color )                {  _setBrushImpl( ::CreateSolidBrush( color ) , true );  }
	void  setBrush( HBRUSH hBrush , bool beManaged = false ){  _setBrushImpl( hBrush , beManaged ); }
	void  setFont( HFONT hFont , bool beManaged = false )   {  _setFontImpl( hFont , beManaged );  }


	void  beginBlend( Vec2i const& pos , Vec2i const& size , float alpha );
	void  endBlend();

	void  drawPixel  ( Vec2i const& p , ColorKey3 const& color ){  ::SetPixel( mhDCRender , p.x , p.y , color ); }
	void  drawLine   ( Vec2i const& p1 , Vec2i const& p2 )    {  ::MoveToEx( mhDCRender ,p1.x ,p1.y , NULL ); ::LineTo( mhDCRender,p2.x ,p2.y); }
	void  drawRect   ( int left , int top , int right , int bottom ){ ::Rectangle( mhDCRender , left , top , right , bottom );  }
	void  drawRect   ( Vec2i const& pos , Vec2i const& size ) {  ::Rectangle( mhDCRender , pos.x , pos.y , pos.x + size.x , pos.y + size.y );  }
	void  drawCircle ( Vec2i const& center , int r )          {  ::Ellipse( mhDCRender , center.x - r , center.y - r, center.x + r , center.y + r );  }
	void  drawEllipse( Vec2i const& pos , Vec2i const& size ) {  ::Ellipse( mhDCRender , pos.x , pos.y , pos.x + size.x , pos.y + size.y );  }
	void  drawRoundRect( Vec2i const& pos , Vec2i const& rectSize , Vec2i const& circleSize ){  ::RoundRect( mhDCRender , pos.x , pos.y , pos.x + rectSize.x , pos.y +rectSize.y  , circleSize.x ,circleSize.y );  }
	void  drawPolygon( Vec2i pos[] , int num )         {  ::Polygon( mhDCRender , (POINT*)( &pos[0] ) , num );  }

	HDC   getRenderDC(){ return mhDCRender; }
	void  setRenderDC( HDC hDC ){  mhDCRender = hDC;  }
	void  setTargetDC( HDC hDC );
	HDC   getTargetDC(){ return mhDCTarget; }

	void  drawTexture( GdiTexture& texture , Vec2i const& pos );
	void  drawTexture( GdiTexture& texture , Vec2i const& pos , ColorKey3 const& color );
	void  drawTexture( GdiTexture& texture , Vec2i const& pos , Vec2i const& texPos , Vec2i const& texSize );
	void  drawTexture( GdiTexture& texture , Vec2i const& pos , Vec2i const& texPos , Vec2i const& texSize , ColorKey3 const& color );
	
	void  setTextColor( uint8 r , uint8 g, uint8 b );
	void  drawText( Vec2i const& pos , char const* str );
	void  drawText( int x , int y , char const* str ){ drawText( Vec2i(x,y) , str ); }
	void  drawText( Vec2i const& pos , Vec2i const& size , char const* str , bool beClip = false );

	Vec2i calcTextExtentSize( char const* str , int num );

	void  beginRender()
	{ 

	}
	void  endRender()
	{
		assert( mBlendCount == 0 );
		mhDCRender = mhDCTarget;
	}

private:
	void  _setPenImpl( HPEN hPen , bool beManaged );
	void  _setBrushImpl( HBRUSH hBrush , bool beManaged );
	void  _setFontImpl( HFONT hFont , bool beManaged );

	static int const MaxXFormStackNum = 5;
	XFORM    mXFormStack[ MaxXFormStackNum ];
	int      mNumStack;

	bool     mbManagedBrush;
	bool     mbManagedPen;
	bool     mbManagedFont;

	HBRUSH   mhCurBrush;
	HPEN     mhCurPen;
	HFONT    mhCurFont;

	unsigned mBlendCount;
	float    mBlendAlpha;
	Vec2i    mBlendPos;
	Vec2i    mBlendSize;
	BitmapDC mBlendDC;

	HDC      mhDCTarget;
	HDC      mhDCRender;

};

class WinGdiRenderSystem
{
public:
	WinGdiRenderSystem( HWND hWnd , HDC hDC );

	typedef WinGdiGraphics2D Graphics2D;

	Vec2i       getClientSize() const;
	Graphics2D& getGraphics2D(){ return mGraphics; }
	HDC         getWindowDC(){ return mhDCWindow; }
	void        beginRender();
	void        endRender();
	HFONT       createFont( int size , char const* faceName , bool beBold = false , bool beItalic = false );

	BitmapDC    mBufferDC;
	Graphics2D  mGraphics;
	HDC         mhDCWindow;

};

#endif // RenderSystemWinGDI_h__
