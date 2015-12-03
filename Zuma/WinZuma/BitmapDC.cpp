#include "BitmapDC.h"

BitmapDC::BitmapDC(HDC hDC, HWND hWnd)
{
	RECT rect;

	::GetClientRect(hWnd,&rect);
	m_width = rect.right;
	m_height = rect.bottom;

	buildBitmap(hDC);
}

BitmapDC::BitmapDC(HDC hDC,int lx,int ly)
	:m_width(lx)
	,m_height(ly)
{
	buildBitmap(hDC);
}

BitmapDC::BitmapDC( HDC hDC,WORD resID )
{
	m_hDC = ::CreateCompatibleDC( hDC );
	m_hBmp = ::LoadBitmap( GetModuleHandle(0) , MAKEINTRESOURCE( resID ) );
	
	::SelectObject( m_hDC , m_hBmp );

	BITMAP bmp;
	GetObject( m_hBmp , sizeof(BITMAP) , &bmp );
	m_width = bmp.bmWidth;
	m_height = bmp.bmHeight;
}

BitmapDC::BitmapDC( HDC hDC , LPSTR fileName )
{
	m_hDC = ::CreateCompatibleDC( hDC );

	// Use LoadImage() to get the image loaded into a DIBSection
	m_hBmp = (HBITMAP)LoadImage( NULL, fileName, IMAGE_BITMAP, 0, 0,
		LR_CREATEDIBSECTION | LR_DEFAULTSIZE | LR_LOADFROMFILE );

	if( m_hBmp == NULL )
		return;

	::SelectObject( m_hDC , m_hBmp );


	BITMAP  bmp;
	GetObject( m_hBmp, sizeof(BITMAP), &bmp );

	m_width = bmp.bmWidth;
	m_height = bmp.bmHeight;

}

BitmapDC::~BitmapDC()
{
	DeleteObject(m_hBmp);
	DeleteDC(m_hDC);
}

void BitmapDC::buildBitmap(HDC hdc)
{
	m_hDC = CreateCompatibleDC(hdc);
	m_hBmp= CreateCompatibleBitmap(hdc,m_width,m_height);
	::SelectObject(m_hDC,m_hBmp);
}

void BitmapDC::clear()
{
	::PatBlt(m_hDC, 0 , 0 , m_width , m_height , BLACKNESS);
}

void BitmapDC::bitBlt( HDC hdc, int x ,int y )
{
	::BitBlt( hdc , x, y , m_width , m_height, m_hDC, 0,0,SRCCOPY );
}


