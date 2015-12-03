#ifndef WinDrawEngine_h__
#define WinDrawEngine_h__

class WinDrawEngine : public IRenderSystem
{
public:
	void     init( HWND hWnd  );
	void     drawBall( ZBallBase& ball );
	void     drawBall( ZConBall& ball );

	void     beginRender();
	void     endRender();

	HDC      m_hDC;
	HDC      m_hDCWnd;


	void     drawPath( std::vector< Vec2D >& path ){}
	void     setMatrix( int ox , int oy , float c , float s )
	{
		SetGraphicsMode(m_hDC, GM_ADVANCED);

		XFORM  xForm;
		xForm.eM11 = (FLOAT) c; 
		xForm.eM12 = (FLOAT) s; 
		xForm.eM21 = (FLOAT) -s; 
		xForm.eM22 = (FLOAT) c; 
		xForm.eDx  = (FLOAT) ox; 
		xForm.eDy  = (FLOAT) oy; 

		SetWorldTransform(m_hDC, &xForm); 
	}

	void  setNormalMatrix()

	{
		XFORM  xForm;
		xForm.eM11 = (FLOAT) 1.0; 
		xForm.eM12 = (FLOAT) 0; 
		xForm.eM21 = (FLOAT) 0; 
		xForm.eM22 = (FLOAT) 1.0; 
		xForm.eDx  = (FLOAT) 0.0; 
		xForm.eDy  = (FLOAT) 0.0;

		SetWorldTransform(m_hDC, &xForm); 
	}

	void      drawBitmap( int x , int y , BitmapDC& bmp , int px , int py , BitmapDC& mask );
	BitmapDC* loadGIF(char const* path );
	void      drawFrog( Vec2D const& pos , Vec2D const& dir );

	COLORREF   ColorRGB[ ZColorNum ];
	BitmapDC*  tempDC;
	BitmapDC*  frogBmp;
	BitmapDC*  frogMaskBmp;
	BitmapDC*  ballBmp[ ZColorNum ];
	BitmapDC*  ballMaskBmp;
	BitmapDC*  ballShadow;

};




void WinDrawEngine::drawBall( ZBallBase& ball )
{
	Vec2D const& pos = ball.getPos();
	Vec2D const& dir = ball.getDir();

	setMatrix( pos.x , pos.y , dir.x  , dir.y );

	HBRUSH hBr = ::CreateSolidBrush( ColorRGB[ball.getColor()] );
	::SelectObject( m_hDC , hBr );
	::Ellipse( m_hDC , 0 - g_ZBallRaidus ,  0 - g_ZBallRaidus , 
		0 + g_ZBallRaidus ,  0 + g_ZBallRaidus );

	HPEN hpen = ::CreatePen( PS_SOLID , 1 , ColorRGB[ball.getColor()]  );
	::MoveToEx( m_hDC , 0, 0 , NULL );
	::LineTo( m_hDC , g_ZBallRaidus , 0 );
	::DeleteObject( hpen );
	::DeleteObject( hBr );

	BitmapDC* bmpDC = ballBmp[ball.getColor()];

	drawBitmap( -16 , -16 , *bmpDC  , 0 , 0 , *ballMaskBmp );


	setNormalMatrix();


}

void WinDrawEngine::drawBall( ZConBall& ball )
{
	Vec2D const& pos = ball.getPos();
	Vec2D const& dir = ball.getDir();

	setMatrix( pos.x , pos.y , dir.y , -dir.x );

	HBRUSH hBr = ::CreateSolidBrush( ColorRGB[ball.getColor()] );
	::SelectObject( m_hDC , hBr );
	::Ellipse( m_hDC , 0 - g_ZBallRaidus ,  0 - g_ZBallRaidus , 
		0 + g_ZBallRaidus ,  0 + g_ZBallRaidus );

	HPEN hpen = ::CreatePen( PS_SOLID , 1 , ColorRGB[ball.getColor()]  );
	::MoveToEx( m_hDC , 0, 0 , NULL );
	::LineTo( m_hDC , g_ZBallRaidus , 0 );
	::DeleteObject( hpen );
	::DeleteObject( hBr );

	int const totalFrame = 47;
	int frame = int( 0.5 * ball.getPathPos()) % totalFrame;
	if ( frame < 0 )
		frame += totalFrame;

	BitmapDC* bmpDC = ballBmp[ball.getColor()];

	drawBitmap( -16 , -16 , *bmpDC , 0 , frame * bmpDC->getWidth() , *ballMaskBmp );

	setNormalMatrix();
}

void WinDrawEngine::drawBitmap( int x , int y , BitmapDC& bmp , int px , int py , BitmapDC& mask )
{
	BitmapDC tempDC( m_hDC , mask.getWidth() , mask.getHeight() );

	::BitBlt( tempDC.getDC()  , 0 , 0 , mask.getWidth() , mask.getHeight() ,
		mask.getDC()  , 0 , 0  , NOTSRCCOPY );

	::BitBlt( m_hDC , x , y , mask.getWidth() , mask.getHeight() ,
		tempDC.getDC() ,  0 , 0 , SRCAND  );


	::BitBlt( tempDC.getDC()  , 0 , 0 , tempDC.getWidth() , tempDC.getHeight() ,
		bmp.getDC()  , px , py  , SRCCOPY );

	::BitBlt( tempDC.getDC()  , 0 , 0 , tempDC.getWidth() , tempDC.getWidth() ,
		mask.getDC()  , 0 , 0  , SRCAND );

	::BitBlt( m_hDC , x , y , tempDC.getWidth() , tempDC.getHeight() ,
		tempDC.getDC() ,  0 , 0 , SRCPAINT );
}

void WinDrawEngine::init( HWND hWnd )
{
	m_hDCWnd = ::GetDC( hWnd );

#define BALL_DATA( id ,  _frame , _color , _path )\
	ColorRGB[ id ] = _color;\
	ballBmp[ id ]  = loadGIF( _path );

	BALL_DATA( Red    , 50 , RGB( 255 , 0 , 0 ) , "images/baballred.gif" )
	BALL_DATA( Green  , 50 , RGB( 0 , 255 , 0 ) , "images/baballgreen.gif" )
	BALL_DATA( Blue   , 50 , RGB( 0 , 0 , 255 ) , "images/baballblue.gif" )
	BALL_DATA( Yellow , 50 , RGB( 255 , 255 , 0 ), "images/baballyellow.gif")
	BALL_DATA( Purple , 50 , RGB( 255 , 0 , 255 ), "images/baballpurple.gif")
	BALL_DATA( White  , 50 , RGB( 255 , 255 , 255 ), "images/baballwhite.gif")

#undef BALL_DATA




	ballMaskBmp = loadGIF("images/_ballalpha.gif");
	frogMaskBmp = loadGIF("images/_smallfrogonpad.gif");

	frogBmp    = loadGIF("images/smallfrogonpad.gif");
}

void WinDrawEngine::drawFrog( Vec2D const& pos , Vec2D const& dir )
{
	setMatrix( pos.x , pos.y , dir.y , -dir.x );

	drawBitmap( -frogBmp->getWidth()/2 , -frogBmp->getHeight()/2 ,
		*frogBmp , 0 , 0 , *frogMaskBmp );
}

BitmapDC* WinDrawEngine::loadGIF(char const* path )
{
	C_ImageSet imgSet;
	imgSet.LoadGIF( (char*)path );
	C_Image* img = imgSet.img[0];
	BitmapDC* bmpDC = new BitmapDC( m_hDCWnd , img->Width , img->Height );
	img->GDIPaint( bmpDC->getDC() , 0 , 0 );

	return bmpDC;
}


#endif // WinDrawEngine_h__