#ifndef CFViewport_h__
#define CFViewport_h__

#include "CFBase.h"

#include "TVector2.h"

namespace CFly
{
	class RenderTarget : public RefObjectT< RenderTarget >
	{
	public:
		void destroyThis(){ delete this; }

#ifdef CF_RENDERSYSTEM_D3D9
		RenderTarget( IDirect3DSurface9* colorSurface );
		virtual ~RenderTarget();
		HDC           getColorBufferDC();
		IDirect3DSurface9* getColorSurface(){ return mColorSurface; }
	protected:
		IDirect3DSurface9* mDepthSurface;
		IDirect3DSurface9* mColorSurface;
		friend class RenderSystem;
#elif defined CF_USE_OPENGL




#endif


	};

	class Viewport : public Entity
	{
	public:
		Viewport( RenderTarget* renderTarget , int x , int y , int w , int h );

		void release();
		void getSize( int* w , int* h );
		void setSize( int w , int h );
		void setOrigin( int x , int y );
		

		void setBackgroundColor(float r , float g , float b , float a = 1.0f)
		{
			mBGColor = Color4f( r, g , b , a );
		}
		Color4f const& getBackgroundColor(){ return mBGColor; }

		RenderTarget* getRenderTarget(){ return mRenderTarget; }
		int           getRenderTargetIndex(){ return mIdxTarget; }
		void          setRenderTarget( RenderTarget* target , int idx );


	private:
		RenderTarget::RefPtr mRenderTarget;
		int                  mIdxTarget;
		Color4f              mBGColor;


#ifdef CF_RENDERSYSTEM_D3D9
		D3DVIEWPORT9         mD3dVP;
#else
		TVector2< int >      mOrg;
		TVector2< int >      mRect;
#endif
		friend class RenderSystem;
	};

	DEFINE_ENTITY_TYPE( Viewport , ET_VIEWPORT )

}//namespace CFly

#endif // CFViewport_h__