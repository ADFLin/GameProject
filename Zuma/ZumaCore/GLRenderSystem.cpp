#include "ZumaPCH.h"
#include "GLRenderSystem.h"

#include "ZLevel.h"
#include "ZLevelManager.h"

#include "ResID.h"
#include "ZFont.h"
#include "Image/GIFLoader.h"

#include <olectl.h>


namespace Zuma
{

	GLRenderSystem::GLRenderSystem()
	{
		mNeedSweepBuffer = true;
	}

	GLRenderSystem::~GLRenderSystem()
	{
		wglDeleteContext( mhRCLoad );
	}

	bool GLRenderSystem::init( HWND hWnd , HDC hDC , HGLRC hRC )
	{
		mhWnd    = hWnd;
		mhDC     = hDC;
		mhRCLoad  = wglCreateContext( hDC );
		mhRCDraw = hRC;
		wglShareLists( mhRCDraw, mhRCLoad );

		glDisable( GL_DEPTH_TEST );
		glClearColor( 0.3f , 0.3f , 0.3f , 0 );

		mEnableBlend = false;
		enableBlend( true );


		glEnable( GL_TEXTURE_2D );
		glHint (GL_PERSPECTIVE_CORRECTION_HINT, GL_NICEST);
		resize();

		unsigned char const* version = glGetString( GL_VERSION );
		return true;
	}


	bool GLRenderSystem::loadAlphaGIF( GLTexture& tex , char const* path )
	{
		TempRawData data;
		if ( !LoadGIFImage( path , &TempRawData::GIFCreateAlpha , &data ) )
			return false;

		tex.createFromRawData(  data.buffer , data.w , data.h , true );
		return true;
	}



	bool GLRenderSystem::loadGIF( GLTexture& tex , char const* path )
	{
		TempRawData data;
		if ( !LoadGIFImage( path , &TempRawData::GIFCreateRGB , &data ) )
			return false;

		if ( ! tex.createFromRawData(  data.buffer , data.w , data.h , false ) )
			return false;

		return true;
	}



	bool GLRenderSystem::loadGIF( GLTexture& tex , char const* path , char const* alphaPath  )
	{
		TempRawData data;
		if ( !LoadGIFImage( path , &TempRawData::GIFCreateRGB , &data ) )
			return false;
		if ( !LoadGIFImage( alphaPath , &TempRawData::GIFFillAlpha , &data ) )
			return false;

		if (!tex.createFromRawData(data.buffer, data.w, data.h, true))
			return false;
		
		return true;
	}

	struct ImageInfo
	{
		int w;
		int h;

	};

	bool GLRenderSystem::loadImage( GLTexture& tex , char const *imgPath , char const* alphaPath )
	{
		bool result = false;

		char  szPath[MAX_PATH+1];										// Full Path To Picture
		if (strstr(imgPath, "http://"))									// If PathName Contains http:// Then...
		{
			strcpy(szPath, imgPath);										// Append The PathName To szPath
		}
		else																// Otherwise... We Are Loading From A File
		{
			GetCurrentDirectory(MAX_PATH, szPath);							// Get Our Working Directory
			strcat(szPath, "\\");											// Append "\" After The Working Directory
			strcat(szPath, imgPath);										// Append The PathName
		}
		OLECHAR		wszPath[MAX_PATH+1];									// Full Path To Picture (WCHAR)
		MultiByteToWideChar(CP_ACP, 0, szPath, -1, wszPath, MAX_PATH);		// Convert From ASCII To Unicode

		IPicture *pPicture = NULL;												// IPicture Interface
		HRESULT hr = OleLoadPicturePath(wszPath, 0, 0, 0, IID_IPicture, (void**)&pPicture);

		if(FAILED(hr))														// If Loading Failed
			return false;													// Return False

		HDC	hdcTemp = CreateCompatibleDC(GetDC(0));						    // Create The Windows Compatible Device Context
		if(!hdcTemp)														// Did Creation Fail?
		{
			pPicture->Release();											// Decrements IPicture Reference Count
			return false;													// Return False (Failure)
		}


		long		lWidth , lHeight;			                        // Width , Height  In Logical Units										
		pPicture->get_Width(&lWidth);										// Get IPicture Width (Convert To Pixels)
		pPicture->get_Height(&lHeight);										// Get IPicture Height (Convert To Pixels)
		long lWidthPixels = MulDiv(lWidth, GetDeviceCaps(hdcTemp, LOGPIXELSX), 2540); // Width In Pixels
		long lHeightPixels = MulDiv(lHeight, GetDeviceCaps(hdcTemp, LOGPIXELSY), 2540); // Height In Pixels

		GLint		glMaxTexDim ;											// Holds Maximum Texture Size
		glGetIntegerv(GL_MAX_TEXTURE_SIZE, &glMaxTexDim);					// Get Maximum Texture Size Supported
		// Resize Image To Closest Power Of Two
		//if (lWidthPixels <= glMaxTexDim) // Is Image Width Less Than Or Equal To Cards Limit
		//	lWidthPixels = 1 << (int)floor((log((double)lWidthPixels)/log(2.0f)) + 0.5f);
		//else  // Otherwise  Set Width To "Max Power Of Two" That The Card Can Handle
		//	lWidthPixels = glMaxTexDim;

		//if (lHeightPixels <= glMaxTexDim) // Is Image Height Greater Than Cards Limit
		//	lHeightPixels = 1 << (int)floor((log((double)lHeightPixels)/log(2.0f)) + 0.5f);
		//else  // Otherwise  Set Height To "Max Power Of Two" That The Card Can Handle
		//	lHeightPixels = glMaxTexDim;

		//	Create A Temporary Bitmap
		BITMAPINFO	bi = {0};												// The Type Of Bitmap We Request
		DWORD		*pBits = 0;												// Pointer To The Bitmap Bits

		bi.bmiHeader.biSize			= sizeof(BITMAPINFOHEADER);				// Set Structure Size
		bi.bmiHeader.biBitCount		= 32;									// 32 Bit
		bi.bmiHeader.biWidth		= lWidthPixels;							// Power Of Two Width
		bi.bmiHeader.biHeight		= -lHeightPixels;						// Make Image Top Up (Positive Y-Axis)
		bi.bmiHeader.biCompression	= BI_RGB;								// RGB Encoding
		bi.bmiHeader.biPlanes		= 1;									// 1 Bitplane


		//	Creating A Bitmap This Way Allows Us To Specify Color Depth And Gives Us Imediate Access To The Bits
		HBITMAP    hbmpTemp = CreateDIBSection(hdcTemp, &bi, DIB_RGB_COLORS, (void**)&pBits, 0, 0); // Holds The Bitmap Temporarily

		if( !hbmpTemp )														// Did Creation Fail?
		{
			DeleteDC(hdcTemp);												// Delete The Device Context
			pPicture->Release();											// Decrements IPicture Reference Count
			return false;													// Return False (Failure)
		}

		SelectObject(hdcTemp, hbmpTemp);									// Select Handle To Our Temp DC And Our Temp Bitmap Object

		// Render The IPicture On To The Bitmap
		pPicture->Render(hdcTemp, 0, 0, lWidthPixels, lHeightPixels, 0, lHeight, lWidth, -lHeight, 0);

		// Convert From BGR To RGB Format And Add An Alpha Value Of 255
		for(long i = 0; i < lWidthPixels * lHeightPixels; i++)				// Loop Through All Of The Pixels
		{
			BYTE* pPixel	= (BYTE*)(&pBits[i]);							// Grab The Current Pixel
			BYTE  temp		= pPixel[0];									// Store 1st Color In Temp Variable (Blue)
			pPixel[0]		= pPixel[2];									// Move Red Value To Correct Position (1st)
			pPixel[2]		= temp;											// Move Temp Value To Correct Blue Position (3rd)
			pPixel[3]	    = 0;
		}

		if ( alphaPath )
		{
			TempRawData data;
			data.w = lWidthPixels;
			data.h = lHeightPixels;
			data.buffer = (unsigned char*)pBits;

			if ( !LoadGIFImage( alphaPath , &TempRawData::GIFFillAlpha , &data ) )
			{
				data.buffer = NULL;
				goto cleanup;
			}

			data.buffer = NULL;
		}

		
		if ( !tex.createFromRawData(  ( unsigned char* )pBits , lWidthPixels, lHeightPixels ,  alphaPath != NULL ) )
			return false;

		result = true;

cleanup:
		DeleteObject( hbmpTemp );
		DeleteDC( hdcTemp );
		pPicture->Release();										    // Decrements IPicture Reference Count
		return result;												    // Return True (All Good)
	}

	void GLRenderSystem::drawBitmap( GLTexture const& tex  , Vector2 const& texPos , Vector2 const& texSize ,
		GLTexture const& mask , Vector2 const& maskPos , Vector2 const& maskSize )
	{

		float tx = texPos.x / tex.getWidth() ;
		float ty = texPos.y / tex.getHeight() ;
		float tw = tx + texSize.x / tex.getWidth() ;
		float th = ty + texSize.y / tex.getHeight() ;

		float mtx = maskPos.x / tex.getWidth() ;
		float mty = maskPos.y / tex.getHeight() ;
		float mtw = mtx + maskSize.x / tex.getWidth() ;
		float mth = mty + maskSize.y / tex.getHeight() ;

		glActiveTexture( GL_TEXTURE0 );
		tex.bind();
		glEnable( GL_TEXTURE_2D );
		glTexEnvi( GL_TEXTURE_ENV , GL_TEXTURE_ENV_MODE, GL_REPLACE );
		//glTexEnvf (GL_TEXTURE_ENV, GL_COMBINE_RGB , GL_REPLACE );

		glActiveTexture( GL_TEXTURE1 );
		mask.bind();
		glEnable( GL_TEXTURE_2D );
		glTexEnvi( GL_TEXTURE_ENV , GL_TEXTURE_ENV_MODE, GL_COMBINE );

		glTexEnvi (GL_TEXTURE_ENV , GL_COMBINE_RGB , GL_REPLACE );
		glTexEnvi( GL_TEXTURE_ENV , GL_SOURCE0_RGB , GL_PREVIOUS  );
		glTexEnvi( GL_TEXTURE_ENV , GL_OPERAND0_RGB , GL_SRC_COLOR );

		glTexEnvi (GL_TEXTURE_ENV , GL_COMBINE_ALPHA , GL_REPLACE );
		glTexEnvi( GL_TEXTURE_ENV , GL_SOURCE0_ALPHA , GL_TEXTURE );
		glTexEnvi( GL_TEXTURE_ENV , GL_OPERAND0_ALPHA , GL_SRC_ALPHA );

		glBegin( GL_QUADS );
		glMultiTexCoord2f( GL_TEXTURE0, tx , ty );
		glMultiTexCoord2f( GL_TEXTURE1, mtx , mty );
		glVertex3f( 0  , 0  , 0 );
		glMultiTexCoord2f( GL_TEXTURE0, tw , ty );
		glMultiTexCoord2f( GL_TEXTURE1, mtw , mty );
		glVertex3f( texSize.x  , 0 , 0 );
		glMultiTexCoord2f( GL_TEXTURE0, tw , th );
		glMultiTexCoord2f( GL_TEXTURE1, mtw , mth );
		glVertex3f( texSize.x  , texSize.y , 0 );
		glMultiTexCoord2f( GL_TEXTURE0, tx , th );
		glMultiTexCoord2f( GL_TEXTURE1, mtx , mth );
		glVertex3f( 0  , texSize.y , 0 );
		glEnd();

		glActiveTexture( GL_TEXTURE1 );
		glDisable( GL_TEXTURE_2D );

		glActiveTexture( GL_TEXTURE0 );
		glDisable( GL_TEXTURE_2D );

		enableBlendImpl( false );
	}

	void GLRenderSystem::drawBitmap( ITexture2D const& tex , unsigned flag )
	{
		GLTexture const& texture = GLTexture::castImpl( tex );
		setupBlend( texture.ownAlpha() , flag );

		glPushMatrix();

		glEnable( GL_TEXTURE_2D );
		texture.bind();

		glTexEnvi( GL_TEXTURE_ENV , GL_TEXTURE_ENV_MODE, GL_MODULATE );

		if ( flag & TBF_OFFSET_CENTER )
			translateWorld( -0.5 * tex.getWidth() , -0.5 * tex.getHeight() );

		glBegin( GL_QUADS );
		glTexCoord2f( 0 , 0 );glVertex3f( 0  , 0  , 0 );
		glTexCoord2f( 1 , 0 );glVertex3f( tex.getWidth()  , 0 , 0 );
		glTexCoord2f( 1 , 1 );glVertex3f( tex.getWidth()  , tex.getHeight() , 0 );
		glTexCoord2f( 0 , 1 );glVertex3f( 0  , tex.getHeight() , 0 );
		glEnd();

		glPopMatrix();
		enableBlendImpl( false );

	}

	void GLRenderSystem::drawBitmap( ITexture2D const& tex ,Vector2 const& texPos , Vector2 const& texSize , unsigned flag )
	{
		GLTexture const& texture = GLTexture::castImpl( tex );

		setupBlend( texture.ownAlpha() ,  flag );

		glEnable( GL_TEXTURE_2D );
		texture.bind();

		glTexEnvi( GL_TEXTURE_ENV , GL_TEXTURE_ENV_MODE, GL_MODULATE );

		float tx = texPos.x / tex.getWidth();
		float ty = texPos.y / tex.getHeight();
		float tw = tx + texSize.x  / tex.getWidth();
		float th = ty + texSize.y / tex.getHeight();

		glPushMatrix();

		if ( flag & TBF_OFFSET_CENTER )
			translateWorld( -0.5f * texSize.x , -0.5 * texSize.y );

		glBegin( GL_QUADS );
		glTexCoord2f( tx , ty );glVertex3f( 0          , 0         , 0 );
		glTexCoord2f( tw , ty );glVertex3f( texSize.x  , 0         , 0 );
		glTexCoord2f( tw , th );glVertex3f( texSize.x  , texSize.y , 0 );
		glTexCoord2f( tx , th );glVertex3f( 0          , texSize.y , 0 );
		glEnd();

		glPopMatrix();

		enableBlendImpl( false );
	}


	void GLRenderSystem::drawBitmapWithinMask( ITexture2D const& tex ,  ITexture2D const& mask , Vector2 const& pos , unsigned flag )
	{

		GLTexture const& texG =  GLTexture::castImpl( tex );
		GLTexture const& maskG = GLTexture::castImpl( mask );
		setupBlend( true , flag );

		float tx = pos.x / tex.getWidth();
		float ty = pos.y / tex.getHeight();

		float tw = tx + float(mask.getWidth()) / tex.getWidth();
		float th = ty + float(mask.getHeight()) / tex.getHeight();

		glActiveTexture( GL_TEXTURE0 );
		texG.bind();

		glEnable( GL_TEXTURE_2D );
		glTexEnvi( GL_TEXTURE_ENV , GL_TEXTURE_ENV_MODE, GL_REPLACE );

		glActiveTexture( GL_TEXTURE1 );
		maskG.bind();

		glEnable( GL_TEXTURE_2D );
		glTexEnvi( GL_TEXTURE_ENV , GL_TEXTURE_ENV_MODE, GL_COMBINE );

		glTexEnvi (GL_TEXTURE_ENV , GL_COMBINE_RGB , GL_REPLACE );
		glTexEnvi( GL_TEXTURE_ENV , GL_SOURCE0_RGB , GL_PREVIOUS  );
		glTexEnvi( GL_TEXTURE_ENV , GL_OPERAND0_RGB , GL_SRC_COLOR );

		glTexEnvi (GL_TEXTURE_ENV , GL_COMBINE_ALPHA , GL_REPLACE );
		glTexEnvi( GL_TEXTURE_ENV , GL_SOURCE0_ALPHA , GL_TEXTURE );
		glTexEnvi( GL_TEXTURE_ENV , GL_OPERAND0_ALPHA , GL_SRC_ALPHA );

		glPushMatrix();

		if ( flag & TBF_OFFSET_CENTER )
			glTranslatef( -0.5f *mask.getWidth() , -0.5f * mask.getHeight() , 0 );

		glBegin( GL_QUADS );
		glMultiTexCoord2f( GL_TEXTURE0, tx , ty );
		glMultiTexCoord2f( GL_TEXTURE1, 0 , 0 );
		glVertex3f( 0  , 0  , 0 );
		glMultiTexCoord2f( GL_TEXTURE0, tw , ty );
		glMultiTexCoord2f( GL_TEXTURE1, 1 , 0 );
		glVertex3f( mask.getWidth()  , 0 , 0 );
		glMultiTexCoord2f( GL_TEXTURE0, tw , th );
		glMultiTexCoord2f( GL_TEXTURE1, 1 , 1 );
		glVertex3f( mask.getWidth()  , mask.getHeight() , 0 );
		glMultiTexCoord2f( GL_TEXTURE0, tx , th );
		glMultiTexCoord2f( GL_TEXTURE1, 0 , 1 );
		glVertex3f( 0  , mask.getHeight() , 0 );
		glEnd();

		glPopMatrix();

		glActiveTexture( GL_TEXTURE1 );
		glDisable( GL_TEXTURE_2D );

		glActiveTexture( GL_TEXTURE0 );
		glDisable( GL_TEXTURE_2D );

		enableBlendImpl( false );

	}

	void GLRenderSystem::setMatrix( Vector2 const& pos , float c , float s )
	{
		static float m[16] =
		{
			1, 0 ,0, 0,
			0, 1, 0, 0,
			0, 0, 1, 0,
			0, 0, 0, 1,
		};

		m[0] = c;  m[1] = s;
		m[4] = -s; m[5] = c;
		m[12] = pos.x ; m[13] = pos.y;

		glLoadMatrixf( m );
	}

	bool GLRenderSystem::prevRender()
	{

		glClearColor(1, 0, 0, 1);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );
		glUseProgram(0);

		glMatrixMode( GL_PROJECTION );
		glLoadIdentity();
		glOrtho( 0 , mScreenWidth ,  mScreenHeight , 0  , -100 , 100 );
		glMatrixMode( GL_MODELVIEW );

		glColor4ub( 255, 255,255,255);
		glLoadIdentity();
		gluLookAt( 0 , 0 , 10.0 , 0 , 0 , 0, 0 , 1 , 0 );

		glDisable( GL_DEPTH_TEST );
		glDisable( GL_CULL_FACE );

		return true;
	}

	void GLRenderSystem::postRender()
	{
#if 0
		glLoadIdentity();
		glColor3f( 1 , 0 , 0 );
		glBegin( GL_LINE_STRIP );
		glVertex3f( 0 , 0  , 0 );
		glVertex3f( 100 , 100  , 0 );
		glEnd();
#endif

		::glFlush();
		if ( mNeedSweepBuffer )
		{
			::SwapBuffers( getHDC() );
		}
	}

	void GLRenderSystem::resize()
	{
		RECT rect;

		::GetClientRect( mhWnd , & rect );

		mScreenWidth = rect.right - rect.left;
		mScreenHeight = rect.bottom - rect.top;

		glMatrixMode( GL_PROJECTION );
		glLoadIdentity();
		glOrtho( 0 , mScreenWidth ,  mScreenHeight , 0  , -100 , 100 );
		glMatrixMode( GL_MODELVIEW );
	}

	bool GLRenderSystem::loadTexture( ITexture2D& texture , ImageResInfo& imgInfo )
	{
		GLTexture& texGL = GLTexture::castImpl( texture );

		texGL.col = imgInfo.numCol;
		texGL.row = imgInfo.numRow;

		if ( imgInfo.flag & IMG_FMT_GIF )
		{
			switch( imgInfo.flag &  IMG_ALPHA  )
			{
			case IMG_ALPHA_NONE:
				loadGIF( texGL , imgInfo.path.c_str() );
				break;
			case IMG_ALPHA_NORMAL:
			case IMG_ALPHA_GRID:
				loadGIF( texGL , imgInfo.path.c_str()  , imgInfo.pathAlpha.c_str() );
				break;
			case IMG_ALPHA_ONLY:
				loadAlphaGIF( texGL , imgInfo.pathAlpha.c_str() );
				break;
			}
		}
		else if ( ( imgInfo.flag & IMG_FMT_JPG ) ||
			( imgInfo.flag & IMG_FMT_BMP ) )
		{
			switch( imgInfo.flag &  IMG_ALPHA  )
			{
			case IMG_ALPHA_NONE:
				loadImage( texGL , imgInfo.path.c_str() , NULL );
				break;
			case IMG_ALPHA_NORMAL:
				loadImage( texGL , imgInfo.path.c_str()  , imgInfo.pathAlpha.c_str() );
				break;
			case IMG_ALPHA_GRID:
				assert( 0 );
				break;
			}
		}

		return true;
	}

	bool GLRenderSystem::loadTexture( ITexture2D& texture , char const* path , char const* alphaPath )
	{
		ImageResInfo info;
		info.numCol = 1;
		info.numRow = 1;
		info.flag   = 0;

		bool haveImage = path && *path != '\0';
		if ( haveImage )
		{
			if ( strstr( path , ".gif") )
				info.flag |= IMG_FMT_GIF;
			else if ( strstr( path , ".jpg") )
				info.flag |= IMG_FMT_JPG;
			else if ( strstr( path , ".bmp") )
				info.flag |= IMG_FMT_BMP;

			info.path = path;
		}

		if ( alphaPath && *alphaPath != '\0' )
		{
			if ( haveImage )
				info.flag |= IMG_ALPHA_NORMAL;
			else
				info.flag |= IMG_ALPHA_ONLY | IMG_FMT_GIF;

			info.pathAlpha = alphaPath;
		}
		else info.flag |= IMG_ALPHA_NONE;

		return loadTexture( texture , info );
	}

	ITexture2D* GLRenderSystem::createTextureRes( ImageResInfo& imgInfo )
	{
		GLTexture* tex = new GLTexture;

		if ( !loadTexture( *tex , imgInfo ) )
		{
			LogWarning( 0 , "Can't create GL Texture" );
			delete tex;
			return NULL;
		}

		return tex;

	}

	void GLRenderSystem::enableBlendImpl( bool beB )
	{
		//if ( m_enableBlend == beB )
		//	return;
		if ( beB )
			glEnable( GL_BLEND );
		else
			glDisable( GL_BLEND );

		mEnableBlend = beB;
	}



	GLenum GLRenderSystem::convBlendValue( BlendEnum value )
	{
		switch ( value )
		{
		case BLEND_ONE :         return GL_ONE;
		case BLEND_ZERO :        return GL_ZERO;
		case BLEND_SRCALPHA:     return GL_SRC_ALPHA;
		case BLEND_DSTALPHA:     return GL_DST_ALPHA;
		case BLEND_INV_SRCALPHA: return GL_ONE_MINUS_SRC_ALPHA;
		case BLEND_INV_DSTALPHA: return GL_ONE_MINUS_DST_ALPHA;
		case BLEND_SRCCOLOR:     return GL_SRC_COLOR;
		case BLEND_DSTCOLOR:     return GL_DST_COLOR;
		case BLEND_INV_SRCCOLOR: return GL_ONE_MINUS_SRC_COLOR;
		case BLEND_INV_DSTCOLOR: return GL_ONE_MINUS_DST_COLOR;
		default:
			assert(0);
		}

		return GL_ONE;
	}

	void GLRenderSystem::setupBlendFun( BlendEnum src , BlendEnum dst )
	{
		glBlendFunc( convBlendValue( src ) , convBlendValue( dst ) );
	}


	ITexture2D* GLRenderSystem::createFontTexture( ZFontLayer& layer )
	{
		GLTexture* tex = new GLTexture;

		if ( layer.imagePath.empty() )
		{
			loadAlphaGIF( *tex , layer.alhpaPath.c_str() );
		}
		else
		{
			loadGIF( *tex , layer.imagePath.c_str() , layer.alhpaPath.c_str() );
		}

		return tex;
	}

	void GLRenderSystem::setColor( uint8 r , uint8 g , uint8 b , uint8 a /*= 255 */ )
	{
		glColor4ub( r , g , b , a );
	}

	void GLRenderSystem::drawPolygon( Vector2 const pos[] , int num )
	{
		glDisable( GL_TEXTURE_2D );
		glBegin( GL_POLYGON );
		for( int i = 0 ; i < num ; ++i )
		{
			glVertexVec2D( pos[i] );
		}
		glEnd();
		glEnable( GL_TEXTURE_2D );
	}

	void GLRenderSystem::prevLoadResource()
	{
		wglMakeCurrent(mhDC,  mhRCLoad);
	}

	void GLRenderSystem::postLoadResource()
	{
		wglMakeCurrent(nullptr, nullptr);
	}

	bool GLTexture::createFromRawData( unsigned char* data , int w , int h , bool beAlpha )
	{
		
		release();

		glGenTextures( 1 , &id );

		if ( id == ErrorID )
			return false;
		glBindTexture( GL_TEXTURE_2D , id  );

		glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR );
		glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR );
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w , h , 0, GL_RGBA, GL_UNSIGNED_BYTE, data );

		mWidth     = w;
		mHeight    = h;
		mOwnAlpha  = beAlpha;
		return true;
	}

	void GLTexture::release()
	{
		if ( id != - 1 )
		{
			glDeleteTextures( 1 , &id );
			id = ErrorID;
		}
	}

}//namespace Zuma
