#pragma once
#ifndef WindowsProfileViewer_H_00146D2F_D5AF_4709_A904_7157B77CC4E4
#define WindowsProfileViewer_H_00146D2F_D5AF_4709_A904_7157B77CC4E4

#include "ProfileSystem.h"
#include "WindowsHeader.h"

class TMessageShow
{
public:
	typedef unsigned char BYTE;

	TMessageShow( HDC hDC );
	~TMessageShow();

	void shiftPos( int dx , int dy );
	void setPos( int x , int y );
	void setOffsetY( int y ){  height = y;  }
	void setColor( BYTE r, BYTE g , BYTE b , BYTE a = 255 )
	{  cr = r ; cg = g; cb = b ; ca = a;  }
	void start();
	void pushString( char const* str );
	void push( char const* format, ... );
	void finish(){}
	
private:

	static HFONT CreateFont( HDC hDC , int size , char const* faceName , bool beBold , bool beItalic );
	HDC      mhDC;
	HFONT    hFont;
	char     string[512];
	int      height;
	int      lineNun;
	BYTE     cr,cg,cb,ca;
	int      startX , startY;
};

class WindowsProfileViewer : public ProfileNodeVisitorT< WindowsProfileViewer >
{
public:
	WindowsProfileViewer( HDC hDC );

	void showText( int x , int y );
	void setTextColor( BYTE r, BYTE g , BYTE b , BYTE a = 255  );
public:
	void onRoot     ( SampleNode* node );
	void onNode     ( SampleNode* node , double parentTime );
	bool onEnterChild( SampleNode* node );
	void onEnterParent( SampleNode* node , int numChildren , double accTime );
private:
	int          mIdxChild;
	TMessageShow msgShow;
};

#endif // WindowsProfileViewer_H_00146D2F_D5AF_4709_A904_7157B77CC4E4