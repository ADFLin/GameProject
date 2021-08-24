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
	friend class WinGdiRenderSystem;
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
	void  setPen( HPEN hPen , bool beManaged = false )      {  setPenImpl( hPen , beManaged ); }
	void  setBrush( Color3ub const& color );
	void  setBrush( HBRUSH hBrush , bool beManaged = false ){  setBrushImpl( hBrush , beManaged ); }
	void  setFont( HFONT hFont , bool beManaged = false )   {  setFontImpl( hFont , beManaged );  }

	void  beginClip(Vec2i const& pos, Vec2i const& size);
	void  endClip();
	void  beginBlend( Vec2i const& pos , Vec2i const& size , float alpha );
	void  endBlend();

	void  drawPixel  ( Vec2i const& p , Color3ub const& color ){  ::SetPixel( mhDCRender , p.x , p.y , color.toXBGR() ); }
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
	void  drawTexture( GdiTexture& texture, Vec2i const& pos, Vec2i const& size);
	void  drawTexture( GdiTexture& texture , Vec2i const& pos , Color3ub const& colorKey );
	void  drawTexture( GdiTexture& texture , Vec2i const& pos , Vec2i const& texPos , Vec2i const& texSize );
	void  drawTexture( GdiTexture& texture , Vec2i const& pos , Vec2i const& texPos , Vec2i const& texSize , Color3ub const& colorKey );
	
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

	void  setPenImpl( HPEN hPen , bool beManaged );
	void  setBrushImpl( HBRUSH hBrush , bool beManaged );
	void  setFontImpl( HFONT hFont , bool beManaged );

	static int const MaxXFormStackNum = 5;
	XFORM    mXFormStack[ MaxXFormStackNum ];
	int      mNumStack;

	template< class T >
	struct TGdiObject
	{
		TGdiObject()
		{
			handle = NULL;
		}
		~TGdiObject()
		{
			assert(handle == NULL);
		}

		void set(T newHandle, bool bInManaged)
		{
			if ( handle && bManaged )
				::DeleteObject(handle);

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
		T    handle;
		bool bManaged;
	};

	TGdiObject<HBRUSH> mCurBrush;
	TGdiObject<HPEN>   mCurPen;
	TGdiObject<HFONT>  mCurFont;


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
