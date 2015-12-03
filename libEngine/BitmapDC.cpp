#include "BitmapDC.h"

BitmapDC::BitmapDC(HDC hDC, HWND hWnd)
{
	RECT rect;
	::GetClientRect(hWnd,&rect);
	buildBitmap(hDC , rect.right - rect.left , rect.bottom - rect.top );
}

BitmapDC::BitmapDC(HDC hDC, int w, int h )
{
	buildBitmap( hDC, w, h);
}

BitmapDC::BitmapDC( HDC hDC,WORD resID )
{
	mhDC  = ::CreateCompatibleDC( hDC );
	mhBmp = ::LoadBitmap( GetModuleHandle( NULL ) , MAKEINTRESOURCE( resID ) );
	
	::SelectObject( mhDC , mhBmp );

	BITMAP bmp;
	GetObject( mhBmp , sizeof(BITMAP) , &bmp );
	mWidth = bmp.bmWidth;
	mHeight = bmp.bmHeight;
}

BitmapDC::BitmapDC( HDC hDC , LPSTR fileName )
{
	mhDC = ::CreateCompatibleDC( hDC );

	// Use LoadImage() to get the image loaded into a DIBSection
	mhBmp = (HBITMAP)LoadImage( NULL, fileName, IMAGE_BITMAP, 0, 0,
		LR_CREATEDIBSECTION | LR_DEFAULTSIZE | LR_LOADFROMFILE );

	if( mhBmp == NULL )
		return;

	::SelectObject( mhDC , mhBmp );


	BITMAP  bmp;
	GetObject( mhBmp, sizeof(BITMAP), &bmp );

	mWidth  = bmp.bmWidth;
	mHeight = bmp.bmHeight;

}

BitmapDC::BitmapDC()
{
	mhDC = NULL;
	mhBmp = NULL;
	mWidth = mHeight = 0;
}

BitmapDC::~BitmapDC()
{
	if ( mhDC )
		::DeleteObject(mhBmp);
	if ( mhBmp )
		::DeleteDC(mhDC);
}

bool BitmapDC::buildBitmap(HDC hdc , int w , int h)
{
	mWidth  = w;
	mHeight = h;
	mhDC = CreateCompatibleDC(hdc);
	if ( mhDC == NULL )
		return false;

	mhBmp = CreateCompatibleBitmap(hdc,mWidth,mHeight);

	if ( mhBmp == NULL )
	{
		::DeleteDC( mhDC );
		mhDC = NULL;
		return false;
	}
	::SelectObject(mhDC,mhBmp);
	return true;
}

void BitmapDC::clear()
{
	::PatBlt(mhDC, 0 , 0 , mWidth , mHeight , BLACKNESS);
}

void BitmapDC::bitBlt( HDC hdc, int x ,int y )
{
	::BitBlt( hdc , x, y , mWidth , mHeight, mhDC, 0,0,SRCCOPY );
}

void BitmapDC::bitBlt( HDC hdc, int x ,int y , int sx , int sy , int w , int h )
{
	::BitBlt( hdc , x, y , w , h , mhDC, sx , sy ,SRCCOPY );
}

bool BitmapDC::create( HDC hDC , int w , int h )
{
	if ( mhBmp )
		::DeleteObject(mhBmp);
	if ( mhDC )
		::DeleteDC(mhDC);

	if ( !buildBitmap( hDC , w , h ) )
		return false;

	return true;
}

bool BitmapDC::create( HDC hDC, HWND hWnd )
{
	RECT rect;
	::GetClientRect(hWnd,&rect);
	return create( hDC , rect.right - rect.left , rect.bottom - rect.top );
}

bool BitmapDC::create( HDC hDC , BITMAPINFO* info , void** data )
{
	cleanup();

	mhDC = ::CreateCompatibleDC( hDC );

	if ( mhDC == NULL )
		return false;

	mWidth  = info->bmiHeader.biWidth;
	mHeight = info->bmiHeader.biHeight;

	void* pixel;
	mhBmp = ::CreateDIBSection( hDC , info , DIB_RGB_COLORS , (void**)&pixel , NULL , 0 );
	if ( data )
		*data = pixel;

	if ( mhBmp == NULL )
	{
		::DeleteDC( mhDC );
		mhDC = NULL;
		return false;
	}

	::SelectObject(mhDC,mhBmp);

	return true;
}

bool BitmapDC::create( HDC hDC, LPSTR file )
{
	cleanup();

	mhDC = ::CreateCompatibleDC( hDC );
	if ( mhDC == NULL )
		return false;

	// Use LoadImage() to get the image loaded into a DIBSection
	mhBmp = (HBITMAP)::LoadImage( NULL, file , IMAGE_BITMAP, 0, 0,
		LR_CREATEDIBSECTION | LR_DEFAULTSIZE | LR_LOADFROMFILE );

	if( mhBmp == NULL )
		return false;

	::SelectObject( mhDC , mhBmp );

	BITMAP  bmp;
	::GetObject( mhBmp, sizeof(BITMAP), &bmp );

	mWidth  = bmp.bmWidth;
	mHeight = bmp.bmHeight;
	return true;
}

void BitmapDC::bitBltTransparent( HDC hdc , COLORREF color , int x /*= 0*/,int y /*= 0 */ )
{
	::TransparentBlt( hdc , x , y , mWidth , mHeight, mhDC, 0,0 , mWidth , mHeight , color );
}

void BitmapDC::cleanup()
{
	if ( mhBmp )
	{
		::DeleteObject(mhBmp);
		mhBmp = NULL;
	}
	if ( mhDC )
	{
		::DeleteDC(mhDC);
		mhDC = NULL;
	}
}



