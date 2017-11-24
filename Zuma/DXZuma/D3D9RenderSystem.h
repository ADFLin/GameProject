#ifndef D3DRenderSystem_h__
#define D3DRenderSystem_h__

#include "IRenderSystem.h"

#include <d3d9.h>
#include <d3dx9.h>

namespace Zuma
{
	class DXTexture2D : public ITexture2D
	{
	public:
		DXTexture2D():m_d3dTexture(NULL){}

		void release();

		static DXTexture2D& castImpl( ITexture2D& tex )
		{
			return static_cast< DXTexture2D& >( tex );
		}

		static DXTexture2D const& castImpl(ITexture2D const& tex)
		{
			return static_cast<DXTexture2D const&>(tex);
		}


		bool createFromRawData( LPDIRECT3DDEVICE9 d3dDevice , unsigned char* data , int w , int h  );
		bool createFromFile(  LPDIRECT3DDEVICE9 d3dDevice , char const* path , D3DCOLOR colorKey );
		LPDIRECT3DTEXTURE9 getD3DTexture() const { return m_d3dTexture; }

	protected:
		LPDIRECT3DTEXTURE9 m_d3dTexture;

	};


	class D3D9RenderSystem : public IRenderSystem
	{
	public:
		D3D9RenderSystem();


		bool init( LPDIRECT3DDEVICE9 d3dDevice );

		virtual void  setColor( uint8 r , uint8 g , uint8 b , uint8 a );
		virtual void  translateWorld( float x , float y );
		virtual void  rotateWorld( float angle );
		virtual void  scaleWorld( float sx , float sy );
		virtual void  setWorldIdentity();
		virtual void  loadWorldMatrix( Vector2 const& pos , Vector2 const& dir );
		virtual void  pushWorldTransform();
		virtual void  popWorldTransform();

		virtual void  enableBlendImpl( bool beE );

	protected:

		virtual void  setupBlendFun( BlendEnum src , BlendEnum dst );


		void     updateWorldMatrix();
		D3DBLEND convBlendValue( BlendEnum value );

	public:

		virtual bool prevRender();
		virtual void postRender();

		void cleanup();

		virtual void  drawBitmap( ITexture2D const& tex , unsigned flag = 0 );
		virtual void  drawBitmap( ITexture2D const& tex , Vector2 const& texPos , Vector2 const& texSize, unsigned flag = 0 );
		virtual void  drawBitmapWithinMask( ITexture2D const& tex , ITexture2D const& mask , Vector2 const& pos , unsigned flag = 0 );


		virtual void  drawPolygon( Vector2 const pos[] , int num );

		void bindTexture( DWORD stage , ITexture2D const& tex )
		{
			getD3DDevice()->SetTexture( stage , getD3DTexture( tex ) );
		}


		struct CVertex
		{
			enum { FVF = ( D3DFVF_XYZ | D3DFVF_DIFFUSE| D3DFVF_TEX2 ) , };
			FLOAT x, y, z ; // The transformed position for the vertex
			D3DCOLOR color;
			FLOAT t1u , t1v;
			FLOAT t2u , t2v;
		};

		struct PVertex
		{
			enum { FVF = ( D3DFVF_XYZ | D3DFVF_DIFFUSE ), };
			FLOAT    x, y, z ;
			D3DCOLOR color;
		};


		virtual ITexture2D* createEmptyTexture(){ return new DXTexture2D; }
		virtual bool loadTexture( ITexture2D& texture , char const* path , char const* alphaPath );

		DXTexture2D* createTexture( char const* path , char const* alphaPath = NULL );
		virtual ITexture2D* createTextureRes( ImageResInfo& info );
		virtual ITexture2D* createFontTexture( ZFontLayer& layer );

		bool loadGIF( char const* path , TempRawData& data );
		bool loadGIFAlpha( char const* path , TempRawData& data );
		bool loadGIFAlpha( DXTexture2D& texture , char const* path );
		LPDIRECT3DDEVICE9 getD3DDevice() const { return mD3DDevice; }

		static LPDIRECT3DTEXTURE9 getD3DTexture( ITexture2D const& texture ){ return DXTexture2D::castImpl(texture).getD3DTexture(); }
		static int const MaxPolygonSize = 64;
		LPDIRECT3DDEVICE9 mD3DDevice;
		LPD3DXMATRIXSTACK mWorldMatStack;
		ID3DXSprite*      mD3DSprite;
		D3DXCOLOR         mCurColor;
		LPDIRECT3DVERTEXBUFFER9 mBmpVtxBuffer;
		LPDIRECT3DVERTEXBUFFER9 mPolyVtxBuffer;
	};


}//namespace Zuma


#endif // D3DRenderSystem_h__
