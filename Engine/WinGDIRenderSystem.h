#ifndef RenderSystemWinGDI_h__
#define RenderSystemWinGDI_h__

#include "WindowsHeader.h"
#include "Core/IntegerType.h"

#include "Graphics2DBase.h"
#include "BitmapDC.h"

#include <unordered_map>
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

	BitmapDC& getContext() { return mImpl; }

private:

	friend class WinGdiGraphics2D;
	friend class WinGDIRenderSystem;
	BitmapDC mImpl;
};


class WinGdiGraphics2D
{
public:
	WinGdiGraphics2D( HDC hDC = NULL );


	void  releaseReources();

	void  beginXForm();
	void  finishXForm();

	void  pushXForm();
	void  popXForm();
	void  identityXForm();
	void  translateXForm( float ox , float oy );
	void  rotateXForm( float angle );
	void  scaleXForm( float sx , float sy  );

	void  setPen( Color3ub const& color , int width = 1 );
	void  setPen( HPEN hPen , bool bManaged = false )      {  setPenImpl( hPen , bManaged ); }
	void  setBrush( Color3ub const& color );
	void  setBrush( HBRUSH hBrush , bool bManaged = false ){  setBrushImpl( hBrush , bManaged ); }
	void  setFont( HFONT hFont , bool bManaged = false )   {  setFontImpl( hFont , bManaged );  }

	void  beginClip(Vec2i const& pos, Vec2i const& size);
	void  endClip();
	void  beginBlend( Vec2i const& pos , Vec2i const& size , float alpha );
	void  endBlend();

	void  drawPixel  ( Vec2i const& p , Color3ub const& color ){  ::SetPixel( mhDCRender , p.x , p.y , color.toRGB() ); }
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

	void  drawTexture( GdiTexture& texture, Vec2i const& pos );
	void  drawTexture( GdiTexture& texture, Vec2i const& pos, Vec2i const& size);
	void  drawTexture( GdiTexture& texture, Vec2i const& pos, Color3ub const& colorKey );
	void  drawTexture( GdiTexture& texture, Vec2i const& pos, Vec2i const& texPos, Vec2i const& texSize );
	void  drawTexture( GdiTexture& texture, Vec2i const& pos, Vec2i const& texPos, Vec2i const& texSize, Color3ub const& colorKey );
	
	void  setTextColor(Color3ub const& color);
	void  drawText( Vec2i const& pos , char const* str );
	void  drawText( int x , int y , char const* str ){ drawText( Vec2i(x,y) , str ); }
	void  drawText( Vec2i const& pos , Vec2i const& size , char const* str , bool beClip = false );

	Vec2i calcTextExtentSize( char const* str , int num );

	void  beginFrame();
	void  endFrame();
	void  beginRender();
	void  endRender();

private:

	void  releaseUsedResources();

	void  setPenImpl( HPEN hPen , bool bManaged );
	void  setBrushImpl( HBRUSH hBrush , bool bManaged );
	void  setFontImpl( HFONT hFont , bool bManaged );

	static int const MaxXFormStackNum = 32;
	XFORM    mXFormStack[ MaxXFormStackNum ];
	int      mNumStack;

	template< class T >
	struct TGDIObject
	{
		TGDIObject() = default;
		
		~TGDIObject()
		{
			assert(handle == NULL);
		}

		void set(T newHandle, bool bInManaged)
		{
			release();
			handle = newHandle;
			bManaged = bInManaged;
		}

		void release()
		{
			if ( handle )
			{
				if( bManaged )
				{
					::DeleteObject(handle);
				}
				handle = NULL;
			}
		}
		operator T() { return handle; }

		T    handle = NULL;
		bool bManaged;

	};

	TGDIObject<HBRUSH> mCurBrush;
	TGDIObject<HPEN>   mCurPen;
	TGDIObject<HFONT>  mCurFont;

	unsigned mBlendCount;
	float    mBlendAlpha;
	Vec2i    mBlendPos;
	Vec2i    mBlendSize;
	BitmapDC mBlendDC;

	HDC      mhDCTarget;
	HDC      mhDCRender;
	HRGN     mhClipRegion;

	std::unordered_map< uint32, HPEN > mCachedPenMap;
	std::unordered_map< uint32, HBRUSH > mCachedBrushMap;

};

class FWinGDI
{
public:
	static HFONT CreateFont(HDC hDC, char const* faceName, int size, bool bBold = false, bool bItalic = false, bool bUnderLine = false);
	static HBRUSH CreateBrush(Color3ub const& color);
	static HPEN CreatePen(Color3ub const& color, int width);

};

class WinGDIRenderSystem
{
public:
	WinGDIRenderSystem( HWND hWnd , HDC hDC );
	~WinGDIRenderSystem();
	typedef WinGdiGraphics2D Graphics2D;

	Vec2i       getClientSize() const;
	Graphics2D& getGraphics2D(){ return mGraphics; }
	HDC         getWindowDC(){ return mhDCWindow; }
	void        beginRender();
	void        endRender();
 	HFONT       createFont(char const* faceName, int size, bool bBold = false, bool bItalic = false);
	BitmapDC    mBufferDC;
	Graphics2D  mGraphics;
	HDC         mhDCWindow;

};

#endif // RenderSystemWinGDI_h__
