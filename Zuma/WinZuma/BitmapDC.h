#ifndef BitmapDC_h__
#define BitmapDC_h__

#include <Windows.h>

class BitmapDC
{
public:
	BitmapDC( HDC hDC, HWND hWnd );
	BitmapDC( HDC hDC,int lx,int ly);
	BitmapDC( HDC hDC,WORD resID );
	BitmapDC( HDC hDC , LPSTR file );
	~BitmapDC();

	void bitBlt( HDC hdc, int x = 0,int y = 0);
	HDC getDC() const { return m_hDC; }
	void clear();
	HBITMAP getBitmap(){ return m_hBmp ; }
	int getWidth(){ return m_width; }
	int getHeight(){ return m_height; }

private:
	void buildBitmap(HDC hdc);
	int  m_width;
	int  m_height;

	HDC  m_hDC;
	HBITMAP m_hBmp;
};

#endif // BitmapDC_h__
