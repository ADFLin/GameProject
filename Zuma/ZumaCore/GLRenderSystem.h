#ifndef GLRenderSystem_h__
#define GLRenderSystem_h__

#include "ZBase.h"
#include "IRenderSystem.h"

#include "Thread.h"

#include <gl\gl.h>
#include <gl\glu.h>

namespace Zuma
{
	class GLTexture : public ITexture2D
	{
	public:
		static GLuint const ErrorID = ( GLuint )-1;
		GLTexture():id( ErrorID  ){}
		GLuint   id;

		void bind() const { glBindTexture( GL_TEXTURE_2D , id ); }

		void release();
		static GLTexture& castImpl( ITexture2D& tex ){ return static_cast< GLTexture& >( tex ) ; }
		static GLTexture const& castImpl(ITexture2D const& tex) { return static_cast< GLTexture const& >(tex); }
		bool createFromRawData( unsigned char* data , int w , int h , bool beAlpha );
	};

	inline void glVertexVec2D( Vec2D const& v ){  glVertex3f( v.x , v.y , 0.0f ); }


	class GLRenderSystem : public IRenderSystem

	{
	public:
		GLRenderSystem();
		~GLRenderSystem();

		bool init( HWND hWnd , HDC hDC , HGLRC hRC );

		virtual void  setColor( uint8 r , uint8 g , uint8 b , uint8 a );
		virtual void  loadWorldMatrix( Vec2D const& pos , Vec2D const& dir ){  setMatrix( pos , dir.x , dir.y );  }
		virtual void  translateWorld( float x , float y ){  glTranslatef( x , y , 0.0f );  }
		virtual void  rotateWorld( float angle ){  glRotatef( angle , 0.0f , 0.0f , 1.0f );  }
		virtual void  scaleWorld( float sx , float sy ){  glScalef( sx , sy , 1.0f );  }
		virtual void  setWorldIdentity(){  glLoadIdentity();  }
		virtual void  pushWorldTransform(){  glPushMatrix();  }
		virtual void  popWorldTransform(){  glPopMatrix();  }

		virtual void  drawBitmap( ITexture2D const& tex , unsigned flag );
		virtual void  drawBitmap( ITexture2D const& tex ,Vec2D const& texPos , Vec2D const& texSize , unsigned flag = 0 );
		virtual void  drawBitmapWithinMask( ITexture2D const& tex , ITexture2D const& mask , Vec2D const& pos , unsigned flag = 0 );


		virtual void  drawPolygon( Vec2D const pos[] , int num );

		void   drawBitmap( GLTexture const& tex , GLTexture const& mask );
		void   drawBitmap( GLTexture const& tex , Vec2D const& texPos , Vec2D const& texSize , 
			GLTexture const& mask , Vec2D const& maskPos , Vec2D const& maskSize );

		virtual ITexture2D* createEmptyTexture(){ return new GLTexture; } 
		virtual bool  loadTexture( ITexture2D& texture , char const* path , char const* alphaPath );

	protected:
		bool     loadTexture( ITexture2D& texture , ImageResInfo& imgInfo );

		virtual  ITexture2D* createTextureRes( ImageResInfo& imgInfo );
		virtual  ITexture2D* createFontTexture( ZFontLayer& layer );

		static GLenum convBlendValue( BlendEnum value );
		void  enableBlendImpl( bool beB );
		void  setupBlendFun( BlendEnum src , BlendEnum dst );
		void  setMatrix( Vec2D const& pos , float c , float s );

	public:
		bool setContext( HGLRC hRC );
		HDC  getHDC(){ return mhDC; }

		bool    mNeedSweepBuffer;
	protected:
		virtual void prevLoadResource();
		virtual void postLoadResource();
		virtual bool prevRender();
		virtual void postRender();
		void resize();

		Mutex   mMutexGL;
		int     mScreenWidth;
		int     mScreenHeight;
		
		HGLRC   mhRCCur;
		HWND    mhWnd;
		HDC     mhDC;
		HGLRC   mhRCTex;
		HGLRC   mhRCDraw;
		bool    mEnableBlend;
	public:

		bool  loadAlphaGIF( GLTexture& tex , char const* path );
		bool  loadGIF( GLTexture& tex , char const* path );
		bool  loadGIF( GLTexture& tex , char const* path , char const* alphaPath );
		bool  loadImage( GLTexture& tex , char const *imgPath , char const* alphaPath );
	};

}//namespace Zuma


#endif // GLRenderSystem_h__