#include "WinGDIRenderSystem.h"
#include "LogSystem.h"
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
	mhClipRegion = NULL;

	if ( mhDCTarget )
		::SetBkMode( mhDCTarget , TRANSPARENT );
}

void WinGdiGraphics2D::releaseReources()
{
	releaseUsedResources();

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

void WinGdiGraphics2D::releaseUsedResources()
{
	mCurFont.release();
	mCurBrush.release();
	mCurPen.release();

	if ( mhClipRegion )
	{
		LogWarning(0, "Forget Call end clip!!");
		::DeleteObject(mhClipRegion);
		mhClipRegion = NULL;
	}
}

void WinGdiGraphics2D::setPenImpl( HPEN hPen , bool bManaged )
{
	if ( hPen != mCurPen )
	{
		mCurPen.set(hPen, bManaged);
		::SelectObject(getRenderDC(), hPen);
	}
}

void WinGdiGraphics2D::setBrushImpl( HBRUSH hBrush , bool bManaged )
{
	if ( hBrush != mCurBrush )
	{
		mCurBrush.set(hBrush, bManaged);
		::SelectObject(getRenderDC(), hBrush);
	}
}


void WinGdiGraphics2D::setFontImpl( HFONT hFont , bool bManaged )
{
	if ( hFont != mCurFont )
	{
		mCurFont.set(hFont, bManaged);
		::SelectObject(getRenderDC(), hFont);
	}
}


void WinGdiGraphics2D::setPen(Color3ub const& color, int width /*= 1 */)
{
	if( width == 1 )
	{
		HPEN hPen;
		auto iter = mCachedPenMap.find(color.toRGB());
		if( iter != mCachedPenMap.end() )
		{
			hPen = iter->second;
		}
		else
		{
			hPen = FWinGDI::CreatePen(color, width);
			mCachedPenMap.emplace(color.toRGB(), hPen);
		}
		assert(hPen != NULL);
		setPenImpl(hPen, false);
	}
	else
	{
		setPenImpl(FWinGDI::CreatePen(color, width), true);
	}
}

void WinGdiGraphics2D::setBrush(Color3ub const& color)
{

	HBRUSH hBrush;
	auto iter = mCachedBrushMap.find(color.toRGB());
	if( iter != mCachedBrushMap.end() )
	{
		hBrush = iter->second;
	}
	else
	{
		hBrush = FWinGDI::CreateBrush(color);
		mCachedBrushMap.emplace(color.toRGB(), hBrush);
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
	if (mhClipRegion == NULL)
	{
		LogWarning(0, "You no call beginClip before call endClip");
		return;
	}
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
		mBlendDC.release();
		mBlendDC.initialize( mhDCTarget , size.x , size.y );
		::SetBkMode(mBlendDC.getHandle(), TRANSPARENT);
	}

	mBlendPos   = pos;
	mBlendSize  = size;
	mBlendAlpha = alpha;

	::BitBlt( mBlendDC.getHandle() , 0 , 0 , size.x , size.y , 
		      mhDCTarget , mBlendPos.x , mBlendPos.y ,SRCCOPY );

	::SetViewportOrgEx( mBlendDC.getHandle() , -mBlendPos.x , -mBlendPos.y , NULL );

	::SelectObject( mBlendDC.getHandle(), mCurBrush );
	::SelectObject( mBlendDC.getHandle(), mCurPen );
	::SelectObject( mBlendDC.getHandle(), mCurFont );

	setRenderDC( mBlendDC.getHandle() );

	++mBlendCount;
}

void WinGdiGraphics2D::endBlend()
{
	BLENDFUNCTION bfn = { 0 };
	bfn.AlphaFormat = 0;
	bfn.BlendOp     = AC_SRC_OVER;
	bfn.BlendFlags  = 0;
	bfn.SourceConstantAlpha = BYTE( 255 * mBlendAlpha );

	::SetViewportOrgEx( mBlendDC.getHandle() , 0 , 0 , NULL );

	::AlphaBlend(mhDCTarget, mBlendPos.x, mBlendPos.y, mBlendSize.x, mBlendSize.y,
		mBlendDC.getHandle(), 0, 0, mBlendSize.x, mBlendSize.y, bfn);

	--mBlendCount;
	//FIX Some bug
	::SelectObject(mhDCTarget, mCurPen);
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
	float s, c;
	Math::SinCos(angle, s, c);
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
	texture.mImpl.bitBltTo( getRenderDC() , pos.x , pos.y );
}

void WinGdiGraphics2D::drawTexture( GdiTexture& texture , Vec2i const& pos , Color3ub const& colorKey)
{
	GDI_PROFILE( "WinGdiGraphics2D::drawTexture" )
	texture.mImpl.bitBltTransparent( getRenderDC() , colorKey.toRGB() , pos.x , pos.y );
}

void WinGdiGraphics2D::drawTexture( GdiTexture& texture , Vec2i const& pos , Vec2i const& texPos , Vec2i const& texSize )
{
	GDI_PROFILE( "WinGdiGraphics2D::drawTexture" )
	::BitBlt( getRenderDC() , pos.x , pos.y , texSize.x , texSize.y , 
		texture.mImpl.getHandle() , texPos.x , texPos.y , SRCCOPY );
}

void WinGdiGraphics2D::drawTexture( GdiTexture& texture , Vec2i const& pos , Vec2i const& texPos , Vec2i const& texSize , Color3ub const& colorKey)
{
	GDI_PROFILE( "WinGdiGraphics2D::drawTexture" )
	::TransparentBlt( getRenderDC() , pos.x , pos.y , texSize.x , texSize.y , 
		texture.mImpl.getHandle() , texPos.x , texPos.y , texSize.x , texSize.y , colorKey.toRGB()  );
}

void WinGdiGraphics2D::drawTexture(GdiTexture& texture, Vec2i const& pos, Vec2i const& size)
{
	GDI_PROFILE("WinGdiGraphics2D::drawTexture")
	::StretchBlt( getRenderDC(), pos.x, pos.y, size.x , size.y ,
				 texture.mImpl.getHandle(), 0, 0, texture.getWidth(), texture.getHeight(), SRCCOPY);
}

void WinGdiGraphics2D::drawText( Vec2i const& pos , char const* str )
{
	GDI_PROFILE("TextOut");
	::TextOut( getRenderDC() , pos.x , pos.y  , str , (int)strlen(str) );
}

void WinGdiGraphics2D::drawText( Vec2i const& pos , Vec2i const& size , char const* str , bool beClip /*= false */ )
{
	GDI_PROFILE("DrawText");
	RECT rect;
	rect.left = pos.x;
	rect.top = pos.y;
	rect.right = pos.x + size.x;
	rect.bottom = pos.y + size.y;

	UINT format = DT_CENTER | DT_SINGLELINE | DT_VCENTER;
	if ( !beClip )
		format |= DT_NOCLIP;
	::DrawText( getRenderDC() , str , (int)FCString::Strlen( str ), &rect , format  );
}

void WinGdiGraphics2D::setTextColor(Color3ub const& color)
{
	::SetTextColor( getRenderDC(), color.toRGB() );
}

Vec2i WinGdiGraphics2D::calcTextExtentSize( char const* str , int num )
{
	SIZE rSize;
	::GetTextExtentPoint( getRenderDC() , str , num , &rSize );
	return Vec2i( rSize.cx , rSize.cy );
}

void WinGdiGraphics2D::beginFrame()
{

}

void WinGdiGraphics2D::endFrame()
{
	releaseUsedResources();
}

void WinGdiGraphics2D::beginRender()
{

}

void WinGdiGraphics2D::endRender()
{
	assert(mBlendCount == 0);
	mhDCRender = mhDCTarget;

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


WinGDIRenderSystem::WinGDIRenderSystem( HWND hWnd , HDC hDC ) 
	:mBufferDC( hDC , hWnd )
	,mGraphics( mBufferDC.getHandle() )
	,mhDCWindow( hDC )
{

}


WinGDIRenderSystem::~WinGDIRenderSystem()
{
	mGraphics.releaseReources();
}

void WinGDIRenderSystem::beginRender()
{
	mBufferDC.clear();
	mGraphics.beginRender();
}

void WinGDIRenderSystem::endRender()
{
	mGraphics.endRender();
	mBufferDC.bitBltTo( mhDCWindow );
}

HFONT WinGDIRenderSystem::createFont(char const* faceName, int size , bool bBold , bool bItalic )
{
	return FWinGDI::CreateFont(mhDCWindow, faceName, size, bBold, bItalic );
}

Vec2i WinGDIRenderSystem::getClientSize() const
{
	return Vec2i( mBufferDC.getWidth() , mBufferDC.getHeight() );
}

HFONT FWinGDI::CreateFont(HDC hDC, char const* faceName, int size, bool bBold , bool bItalic , bool bUnderLine )
{
	LOGFONT lf;

	lf.lfHeight = -MulDiv(size, GetDeviceCaps(hDC, LOGPIXELSY), 72);
	lf.lfWeight = 0;
	lf.lfEscapement = 0;
	lf.lfOrientation = 0;
	lf.lfWeight = (bBold) ? FW_BOLD : FW_NORMAL;
	lf.lfItalic = bItalic;
	lf.lfUnderline = bUnderLine;
	lf.lfStrikeOut = FALSE;
	lf.lfCharSet = DEFAULT_CHARSET;
	lf.lfOutPrecision = OUT_TT_PRECIS;
	lf.lfClipPrecision = CLIP_DEFAULT_PRECIS;
	lf.lfQuality = ANTIALIASED_QUALITY;
	lf.lfPitchAndFamily = FF_DONTCARE | DEFAULT_PITCH;
	strcpy_s(lf.lfFaceName, faceName);

	return CreateFontIndirectA(&lf);
}

HBRUSH FWinGDI::CreateBrush(Color3ub const& color)
{
	return ::CreateSolidBrush(color.toRGB());
}

HPEN FWinGDI::CreatePen(Color3ub const& color, int width)
{
	return ::CreatePen(PS_SOLID, width, color.toRGB());
}
