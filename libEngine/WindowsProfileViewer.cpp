#include "WindowsProfileViewer.h"

#include <math.h>
#include <stdio.h>

WindowsProfileViewer::WindowsProfileViewer( HDC hDC )
	:msgShow( hDC )
{

}

void WindowsProfileViewer::onRoot( SampleNode* node )
{
	double time_since_reset = ProfileSystem::Get().getTimeSinceReset();
	msgShow.push( "--- Profiling: %s (total running time: %.3f ms) ---" , 
		node->getName() , time_since_reset );
}

void WindowsProfileViewer::onNode( SampleNode* node , double parentTime )
{
	msgShow.push( "|-> %d -- %s (%.2f %%) :: %.3f ms / frame (%d calls)",
		++mIdxChild , node->getName() ,
		parentTime > CLOCK_EPSILON ? ( node->getTotalTime()  / parentTime ) * 100 : 0.f ,
		node->getTotalTime()  / (double)ProfileSystem::Get().getFrameCountSinceReset() ,
		node->getTotalCalls() );
}

bool WindowsProfileViewer::onEnterChild( SampleNode* node )
{
	mIdxChild = 0;

	msgShow.shiftPos( 20 , 0 );

	return true;
}

void WindowsProfileViewer::onEnterParent( SampleNode* node , int numChildren , double accTime )
{
	if ( numChildren )
	{
		double time;
		if ( node->getParent() != NULL )
			time = node->getTotalTime();
		else
			time = ProfileSystem::Get().getTimeSinceReset();

		double delta = time - accTime;
		msgShow.push( "|-> %s (%.3f %%) :: %.3f ms / frame", "Other",
			// (min(0, time_since_reset - totalTime) / time_since_reset) * 100);
			( time > CLOCK_EPSILON ) ? ( delta / time * 100 ) : 0.f  , 
			delta / (double)ProfileSystem::Get().getFrameCountSinceReset() );
		msgShow.push( "-------------------------------------------------" );
	}

	msgShow.shiftPos( -20 , 0 );
}

void WindowsProfileViewer::showText( int x , int y )
{
	msgShow.setPos( x , y );
	msgShow.start();
	visitNodes();
	msgShow.finish();
}

void WindowsProfileViewer::setTextColor( BYTE r, BYTE g , BYTE b , BYTE a /*= 255 */ )
{
	msgShow.setColor( r , g , b , a );
}

TMessageShow::TMessageShow( HDC hDC )
{
	mhDC  = hDC;
	hFont = CreateFont( hDC , 8 , TEXT("�s�ө���") , false , false );

	height = 15;
	setPos( 20 , 20 );
	setColor( 255 , 255 , 0 , 255 );
}

TMessageShow::~TMessageShow()
{
	::DeleteObject( hFont );
}

HFONT TMessageShow::CreateFont( HDC hDC , int size , char const* faceName , bool beBold , bool beItalic )
{
	LOGFONT lf;

	lf.lfHeight = -(int)(fabs( ( float)10 * size *GetDeviceCaps( hDC ,LOGPIXELSY)/72)/10.0+0.5);
	lf.lfWeight = 0;
	lf.lfEscapement = 0;
	lf.lfOrientation = 0;
	lf.lfWeight = ( beBold ) ? FW_BOLD : FW_NORMAL ;
	lf.lfItalic = beItalic;
	lf.lfUnderline = FALSE;
	lf.lfStrikeOut = FALSE;
	lf.lfCharSet = CHINESEBIG5_CHARSET;
	lf.lfOutPrecision = OUT_TT_PRECIS;
	lf.lfClipPrecision = CLIP_TT_ALWAYS;
	lf.lfQuality = DEFAULT_QUALITY;
	lf.lfPitchAndFamily = DEFAULT_PITCH;
	strcpy_s( lf.lfFaceName , faceName );

	return CreateFontIndirect( &lf );
}

void TMessageShow::shiftPos( int dx , int dy )
{
	startX += dx;
	startY += dy;
}

void TMessageShow::setPos( int x , int y )
{
	startX = x ; startY = y;
}

void TMessageShow::push( char const* format, ... )
{
	va_list argptr;
	va_start( argptr, format );
	vsprintf_s( string , format , argptr );
	va_end( argptr );
	pushString( string );
}

void TMessageShow::pushString( char const* str )
{
	::TextOut( mhDC , startX , startY + lineNun * height , str  , (int)strlen(str) );
	++lineNun;
}

void TMessageShow::start()
{
	lineNun = 0;
	::SelectObject( mhDC , hFont );
	::SetTextColor( mhDC , RGB( cr , cg , cb ) );
}
