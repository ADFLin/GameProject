#ifndef BitmapDC_h__
#define BitmapDC_h__

#include "PlatformConfig.h"

#if SYS_PLATFORM_WIN
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

	bool    isValid() const { return mhDC != NULL; }

	bool    initialize( HDC hDC , LPSTR file );
	bool    initialize( HDC hDC , HWND hWnd );
	bool    initialize( HDC hDC , int w , int h );
	bool    initialize( HDC hDC , BITMAPINFO* info , void** data = 0 );
	void    release();

	bool    bitBltFrom(HDC hDC, int x, int y);
	bool    bitBltTo( HDC hdc, int x = 0,int y = 0 );
	bool    bitBltTo( HDC hdc, int x ,int y , int sx , int sy , int w , int h );
	bool    bitBltTransparent( HDC hdc , COLORREF color , int x = 0,int y = 0 );
	HDC     getHandle() const { return mhDC; }
	HBITMAP getBitmap(){ return mhBmp ; }
	void    clear();
	int     getWidth() const { return mWidth; }
	int     getHeight()const { return mHeight; }

	operator HDC() { return mhDC; }


	bool   readPixels(void* pOutPxiels);

private:

	bool    constructBitmap(HDC hdc , int w , int h );
	void    cleanup();
	int     mWidth;
	int     mHeight;
	HDC     mhDC;
	HBITMAP mhBmp;
};
#endif


#endif // BitmapDC_h__
