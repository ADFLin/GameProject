#ifndef BitmapDC_h__
#define BitmapDC_h__

#include "WindowsHeader.h"

class BitmapDC
{
public:
	BitmapDC();
	BitmapDC( HDC hDC, HWND hWnd );
	BitmapDC( HDC hDC, int w ,int h );
	BitmapDC( HDC hDC, WORD resID );
	BitmapDC( HDC hDC, LPSTR file );
	~BitmapDC();

	bool    initialize( HDC hDC , LPSTR file );
	bool    initialize( HDC hDC , HWND hWnd );
	bool    initialize( HDC hDC , int w , int h );
	bool    initialize( HDC hDC , BITMAPINFO* info , void** data = 0 );
	void    release();


	void    bitBlt( HDC hdc, int x = 0,int y = 0 );
	void    bitBlt( HDC hdc, int x ,int y , int sx , int sy , int w , int h );
	void    bitBltTransparent( HDC hdc , COLORREF color , int x = 0,int y = 0 );
	HDC     getDC() const { return mhDC; }
	HBITMAP getBitmap(){ return mhBmp ; }
	void    clear();
	int     getWidth() const { return mWidth; }
	int     getHeight()const { return mHeight; }

	operator HDC() { return mhDC; }

private:

	bool    constructBitmap(HDC hdc , int w , int h );
	void    cleanup();
	int     mWidth;
	int     mHeight;
	HDC     mhDC;
	HBITMAP mhBmp;
};

#endif // BitmapDC_h__
