#include "WinGDIRenderSystem.h"

#include <cmath>

#if 1
#	include "ProfileSystem.h"
#	define  GDI_PROFILE( name )  PROFILE_ENTRY( name )
#else
#   define  GDI_PROFILE( name )
#endif

WinGdiGraphics2D::WinGdiGraphics2D( HDC hDC ) 
	:mBlendCount(0)
	,mNumStack(0)
	,mhDCRender( hDC )
	,mhDCTarget( hDC )
{
	mbPenManaged   = false;
	mbBrushManaged = false;
	mbFontManaged  = false;

	mhCurPen   = NULL;
	mhCurBrush = NULL;
	mhCurFont  = NULL;

	if ( mhDCTarget )
		::SetBkMode( mhDCTarget , TRANSPARENT );
}

void WinGdiGraphics2D::releaseReources()
{
	if( mbFontManaged )
	{
		::DeleteObject(mhCurFont);
		mhCurFont = NULL;
		mbFontManaged = false;
	}
	if( mbBrushManaged )
	{
		::DeleteObject(mhCurBrush);
		mhCurBrush = NULL;
		mbBrushManaged = false;
	}
	if( mbPenManaged )
	{
		::DeleteObject(mhCurPen);
		mhCurPen = NULL;
		mbPenManaged = false;
	}

	for( auto& pair : mCachedBrushMap )
	{
		::DeleteObject(pair.second);
	}
	mCachedBrushMap.clear();
	for( auto& pair : mCachedPenMap )
	{
		::DeleteObject(pair.second);
	}
	mCachedPenMap.clear();
}

void WinGdiGraphics2D::setPenImpl( HPEN hPen , bool beManaged  )
{
	if ( hPen != mhCurPen )
	{
		if( mbPenManaged )
			::DeleteObject(mhCurPen);

		::SelectObject(getRenderDC(), hPen);

		mhCurPen = hPen;
		mbPenManaged = beManaged;
	}
}

void WinGdiGraphics2D::setBrushImpl( HBRUSH hBrush , bool beManaged )
{
	if ( hBrush != mhCurBrush )
	{
		if( mbBrushManaged )
			::DeleteObject(mhCurBrush);

		::SelectObject(getRenderDC(), hBrush);
		mhCurBrush = hBrush;
		mbBrushManaged = beManaged;
	}
}


void WinGdiGraphics2D::setFontImpl( HFONT hFont , bool beManaged )
{
	if ( hFont != mhCurFont )
	{
		if( mbFontManaged )
			::DeleteObject(mhCurFont);

		::SelectObject(getRenderDC(), hFont);
		mhCurFont = hFont;
		mbFontManaged = beManaged;
	}
}


void WinGdiGraphics2D::setPen(Color3ub const& color, int width /*= 1 */)
{
	if( width == 1 )
	{
		HPEN hPen;
		auto iter = mCachedPenMap.find(color.toXBGR());
		if( iter != mCachedPenMap.end() )
		{
			hPen = iter->second;
		}
		else
		{
			hPen = ::CreatePen(PS_SOLID, width, color.toXBGR());
			mCachedPenMap.emplace(color.toXBGR(), hPen);
		}
		assert(hPen != NULL);
		setPenImpl(hPen, false);
	}
	else
	{
		setPenImpl(::CreatePen(PS_SOLID, width, color.toXBGR()), true);
	}
}

void WinGdiGraphics2D::setBrush(Color3ub const& color)
{

	HBRUSH hBrush;
	auto iter = mCachedBrushMap.find(color.toXBGR());
	if( iter != mCachedBrushMap.end() )
	{
		hBrush = iter->second;
	}
	else
	{
		hBrush = ::CreateSolidBrush(color.toXBGR());
		mCachedBrushMap.emplace(color.toXBGR(), hBrush);
	}
	assert(hBrush != NULL);
	setBrushImpl(hBrush, false);
}

void WinGdiGraphics2D::beginClip(Vec2i const& pos, Vec2i const& size)
{
	mhClipRegion = CreateRectRgn(pos.x, pos.y, pos.x + size.x, pos.y + size.y);
	::SelectClipRgn(getRenderDC(), mhClipRegion);
}

void WinGdiGraphics2D::endClip()
{
	::SelectClipRgn(getRenderDC(), NULL);
	::DeleteObject(mhClipRegion);
	mhClipRegion = NULL;
}

void WinGdiGraphics2D::beginBlend( Vec2i const& pos , Vec2i const& size , float alpha )
{
	assert( mBlendCount == 0 );

	if ( mBlendDC.getWidth() < size.x ||
		 mBlendDC.getHeight() < size.y )
	{
		mBlendDC.initialize( mhDCTarget , size.x , size.y );
	}

	mBlendPos   = pos;
	mBlendSize  = size;
	mBlendAlpha = alpha;

	::BitBlt( mBlendDC.getDC() , 0 , 0 , size.x , size.y , 
		      mhDCTarget , mBlendPos.x , mBlendPos.y ,SRCCOPY );

	::SetViewportOrgEx( mBlendDC.getDC() , -mBlendPos.x , -mBlendPos.y , NULL );

	::SelectObject( mBlendDC.getDC() , mhCurBrush );
	::SelectObject( mBlendDC.getDC() , mhCurPen );

	setRenderDC( mBlendDC.getDC() );

	++mBlendCount;
}

void WinGdiGraphics2D::endBlend()
{
	BLENDFUNCTION bfn = { 0 };
	bfn.AlphaFormat = 0;
	bfn.BlendOp     = AC_SRC_OVER;
	bfn.BlendFlags  = 0;
	bfn.SourceConstantAlpha = BYTE( 255 * mBlendAlpha );

	::SetViewportOrgEx( mBlendDC.getDC() , 0 , 0 , NULL );

	::AlphaBlend( mhDCTarget,  mBlendPos.x , mBlendPos.y  , mBlendSize.x , mBlendSize.y ,
		          mBlendDC.getDC(), 0 , 0 , mBlendSize.x , mBlendSize.y , bfn );

	--mBlendCount;

	setRenderDC( mhDCTarget );
}

void WinGdiGraphics2D::identityXForm()
{
	::ModifyWorldTransform( getRenderDC() , NULL , MWT_IDENTITY );
}

void WinGdiGraphics2D::translateXForm( float ox , float oy  )
{
	XFORM xform;
	xform.eM11 = 1;  xform.eM12 = 0;
	xform.eM21 = 0;  xform.eM22 = 1;
	xform.eDx  = ox ;  xform.eDy  = oy;

	::ModifyWorldTransform( getRenderDC() , &xform , MWT_LEFTMULTIPLY);
}

void WinGdiGraphics2D::rotateXForm( float angle )
{
	float c = cos( angle );
	float s = sin( angle );
	XFORM xform;

	xform.eM11 = c;  xform.eM12 = s;
	xform.eM21 = -s; xform.eM22 = c;
	xform.eDx  = 0 ; xform.eDy = 0;

	::ModifyWorldTransform( getRenderDC() , &xform , MWT_LEFTMULTIPLY );
}

void WinGdiGraphics2D::scaleXForm( float sx , float sy )
{

	XFORM xform;

	xform.eM11 = sx;  xform.eM12 = 0;
	xform.eM21 = 0;  xform.eM22 = sy;
	xform.eDx  = 0 ; xform.eDy = 0;

	::ModifyWorldTransform( getRenderDC() , &xform , MWT_LEFTMULTIPLY  );

}

void WinGdiGraphics2D::beginXForm()
{
	::SetGraphicsMode( getRenderDC(), GM_ADVANCED );
}

void WinGdiGraphics2D::finishXForm()
{
	identityXForm();
	::SetGraphicsMode( getRenderDC(), GM_COMPATIBLE );
}

void WinGdiGraphics2D::pushXForm()
{
	assert( mNumStack < MaxXFormStackNum );
	::GetWorldTransform( getRenderDC() , &mXFormStack[ mNumStack ] );
	++mNumStack;
}

void WinGdiGraphics2D::popXForm()
{
	assert( mNumStack > 0 );
	--mNumStack;
	::SetWorldTransform( getRenderDC() , &mXFormStack[ mNumStack ] );
}

void WinGdiGraphics2D::drawTexture( GdiTexture& texture , Vec2i const& pos )
{
	GDI_PROFILE( "WinGdiGraphics2D::drawTexture" )
	texture.mImpl.bitBlt( getRenderDC() , pos.x , pos.y );
}

void WinGdiGraphics2D::drawTexture( GdiTexture& texture , Vec2i const& pos , Color3ub const& color )
{
	GDI_PROFILE( "WinGdiGraphics2D::drawTexture" )
	texture.mImpl.bitBltTransparent( getRenderDC() , color.toXBGR() , pos.x , pos.y );
}

void WinGdiGraphics2D::drawTexture( GdiTexture& texture , Vec2i const& pos , Vec2i const& texPos , Vec2i const& texSize )
{
	GDI_PROFILE( "WinGdiGraphics2D::drawTexture" )
	::BitBlt( getRenderDC() , pos.x , pos.y , texSize.x , texSize.y , 
		texture.mImpl.getDC() , texPos.x , texPos.y , SRCCOPY );
}

void WinGdiGraphics2D::drawTexture( GdiTexture& texture , Vec2i const& pos , Vec2i const& texPos , Vec2i const& texSize , Color3ub const& color )
{
	GDI_PROFILE( "WinGdiGraphics2D::drawTexture" )
	::TransparentBlt( getRenderDC() , pos.x , pos.y , texSize.x , texSize.y , 
		texture.mImpl.getDC() , texPos.x , texPos.y , texSize.x , texSize.y , color.toXBGR()  );
}

void WinGdiGraphics2D::drawTexture(GdiTexture& texture, Vec2i const& pos, Vec2i const& size)
{
	GDI_PROFILE("WinGdiGraphics2D::drawTexture")
	::StretchBlt( getRenderDC(), pos.x, pos.y, size.x , size.y ,
				 texture.mImpl.getDC(), 0, 0, texture.getWidth(), texture.getHeight(), SRCCOPY);
}

void WinGdiGraphics2D::drawText( Vec2i const& pos , char const* str )
{
	::TextOut( getRenderDC() , pos.x , pos.y  , str , (int)strlen(str) );
}

void WinGdiGraphics2D::drawText( Vec2i const& pos , Vec2i const& size , char const* str , bool beClip /*= false */ )
{
	RECT rect;
	rect.left = pos.x;
	rect.top = pos.y;
	rect.right = pos.x + size.x;
	rect.bottom = pos.y + size.y;

	UINT format = DT_CENTER | DT_SINGLELINE | DT_VCENTER;
	if ( !beClip )
		format |= DT_NOCLIP;
	::DrawText( getRenderDC() , str , (int)strlen( str ), &rect , format  );
}

void WinGdiGraphics2D::setTextColor(Color3ub const& color)
{
	::SetTextColor( getRenderDC(), color.toXBGR() );
}

Vec2i WinGdiGraphics2D::calcTextExtentSize( char const* str , int num )
{
	SIZE rSize;
	::GetTextExtentPoint( getRenderDC() , str , num , &rSize );
	return Vec2i( rSize.cx , rSize.cy );
}

void WinGdiGraphics2D::setTargetDC( HDC hDC )
{
	mhDCTarget = hDC;
	if ( mhDCTarget )
		::SetBkMode( mhDCTarget , TRANSPARENT );
}

bool GdiTexture::create( HDC hDC , int width , int height , void** data )
{
	BITMAPINFO bmpInfo;
	memset( &bmpInfo , 0 , sizeof(BITMAPINFO) );

	bmpInfo.bmiHeader.biSize   = sizeof(BITMAPINFOHEADER);
	bmpInfo.bmiHeader.biWidth  = width;
	bmpInfo.bmiHeader.biHeight = height;
	bmpInfo.bmiHeader.biPlanes = 1;
	bmpInfo.bmiHeader.biBitCount = 32;
	bmpInfo.bmiHeader.biCompression = BI_RGB;

	return mImpl.initialize( hDC , &bmpInfo , data );
}

bool GdiTexture::createFromFile( HDC hDC , char const* path )
{
	return mImpl.initialize( hDC , (LPSTR)path );
}

uint8* GdiTexture::getRawData()
{
	BITMAP bmp;
	::GetObject( mImpl.getBitmap() , sizeof( bmp ) , &bmp );
	return (uint8*)bmp.bmBits;
}


WinGdiRenderSystem::WinGdiRenderSystem( HWND hWnd , HDC hDC ) 
	:mBufferDC( hDC , hWnd )
	,mGraphics( mBufferDC.getDC() )
	,mhDCWindow( hDC )
{

}


void WinGdiRenderSystem::beginRender()
{
	mBufferDC.clear();
	mGraphics.beginRender();
}

void WinGdiRenderSystem::endRender()
{
	mGraphics.endRender();
	mBufferDC.bitBlt( mhDCWindow );
}

HFONT WinGdiRenderSystem::createFont( int size , char const* faceName , bool beBold , bool beItalic )
{
	LOGFONT lf;

	lf.lfHeight = -(int)(fabs( ( float)10 * size *GetDeviceCaps( mhDCWindow ,LOGPIXELSY)/72)/10.0+0.5);
	lf.lfWeight        = 0;
	lf.lfEscapement    = 0;
	lf.lfOrientation   = 0;
	lf.lfWeight        = ( beBold ) ? FW_BOLD : FW_NORMAL ;
	lf.lfItalic        = beItalic;
	lf.lfUnderline     = FALSE;
	lf.lfStrikeOut     = FALSE;
	lf.lfCharSet       = CHINESEBIG5_CHARSET;
	lf.lfOutPrecision  = OUT_TT_PRECIS;
	lf.lfClipPrecision = CLIP_TT_ALWAYS;
	lf.lfQuality       = DEFAULT_QUALITY;
	lf.lfPitchAndFamily = DEFAULT_PITCH;
	strcpy_s( lf.lfFaceName , faceName );

	return CreateFontIndirect( &lf );
}

Vec2i WinGdiRenderSystem::getClientSize() const
{
	return Vec2i( mBufferDC.getWidth() , mBufferDC.getHeight() );
}
