#ifndef GdiRenderSystem_h__
#define GdiRenderSystem_h__

namespace Zuma
{

	class GdiRenderSystem : public IRenderSystem
	{
	public:
		virtual void  setColor( uint8 r , uint8 g , uint8 b , uint8 a = 255 ) = 0;
		//virtual void  drawBitmap( ITexture2D& tex , Vec2D const& texPos , Vec2D const& texSize ,
		//							ITexture2D& mask , Vec2D const& maskPos , Vec2D const& maskSize ,
		//                          unsigned flag );
		//virtual void  drawBitmap( ITexture2D& tex , ITexture2D& mask , unsigned flag );
		virtual void  drawBitmap( ITexture2D const& tex , unsigned flag = 0 ) = 0;
		virtual void  drawBitmap( ITexture2D const& tex , Vector2 const& texPos , Vector2 const& texSize, unsigned flag = 0 ) = 0;
		virtual void  drawBitmapWithinMask( ITexture2D const& tex , ITexture2D const& mask , Vector2 const& pos , unsigned flag = 0 ) = 0;
		virtual void  drawPolygon( Vector2 const pos[] , int num ) = 0;

		virtual void  loadWorldMatrix( Vector2 const& pos , Vector2 const& dir )
		{

		}

		virtual void  translateWorld( float x , float y ){  getGraphics().translateXForm( Vector2( x , y ) );  }
		virtual void  rotateWorld( float angle ){  getGraphics().rotateXForm( angle );  }
		virtual void  scaleWorld( float sx , float sy ){  getGraphics().scaleXForm( sx , sy );  }
		virtual void  setWorldIdentity()  {  getGraphics().identityXForm();  }
		virtual void  pushWorldTransform(){  getGraphics().pushXForm();  }
		virtual void  popWorldTransform() {  getGraphics().popXForm();  }

		virtual bool  loadTexture( ITexture2D& texture , char const* imagePath , char const* alphaPath ) = 0;
		virtual ITexture2D* createFontTexture( ZFontLayer& layer ) = 0;
		virtual ITexture2D* createTextureRes( ImageResInfo& imgInfo ) = 0;
		virtual ITexture2D* createEmptyTexture() = 0;

	protected:
		virtual bool prevRender(){}
		virtual void postRender(){}

		virtual void  enableBlendImpl( bool beE ) = 0;
		virtual void  setupBlendFun( BlendEnum src , BlendEnum dst ) = 0;


		WinGdiGraphics2D& getGraphics(){ return *mGraphics; }
		WinGdiGraphics2D* mGraphics;


		uint32 mColor;
	};




}//namespace Zuma

#endif // GdiRenderSystem_h__
