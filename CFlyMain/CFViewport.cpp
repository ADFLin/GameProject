#include "CFlyPCH.h"
#include "CFViewport.h"

namespace CFly
{

	Viewport::Viewport( RenderTarget* renderTarget , int x , int y , int w , int h )
		:mRenderTarget( renderTarget )
		,mBGColor(0,0,0)
	{
		mIdxTarget = 0;

#ifdef CF_RENDERSYSTEM_D3D9
		mD3dVP.X = x;
		mD3dVP.Y = y;
		mD3dVP.Width = w;
		mD3dVP.Height = h;
		mD3dVP.MinZ = -1.0f;
		mD3dVP.MaxZ = 1.0f;
#else
		mOrg.x = x;
		mOrg.y = y;
		mRect.x = w;
		mRect.y = h;
#endif
	}


	void Viewport::release()
	{
		delete this;
	}

	void Viewport::getSize( int* w , int* h )
	{
		if ( w ) *w = mD3dVP.Width;
		if ( h ) *h = mD3dVP.Height;
	}

	void Viewport::setSize( int w , int h )
	{
#ifdef CF_RENDERSYSTEM_D3D9
		mD3dVP.Width = w;
		mD3dVP.Height = h;
#else
		mRect.x = w;
		mRect.y = h;
#endif
	}


	void Viewport::setRenderTarget( RenderTarget* target , int idx = 0 )
	{
		mIdxTarget    = idx;
		mRenderTarget = target;
	}

	void Viewport::setOrigin( int x , int y )
	{
#ifdef CF_RENDERSYSTEM_D3D9
		mD3dVP.X = x;
		mD3dVP.Y = y;
#else
		mOrg.x = x;
		mOrg.y = y;
#endif
	}


#ifdef CF_RENDERSYSTEM_D3D9
	RenderTarget::RenderTarget( IDirect3DSurface9* colorSurface ) 
		:mDepthSurface(nullptr)
		,mColorSurface(colorSurface)
	{

	}

	RenderTarget::~RenderTarget()
	{
		if ( mColorSurface )
			mColorSurface->Release();
		if ( mDepthSurface )
			mDepthSurface->Release();
	}
	HDC RenderTarget::getColorBufferDC()
	{
		HDC hDC = NULL;
		mColorSurface->GetDC( &hDC );
		return hDC;
	}
#elif defined CF_USE_OPENGL




#endif


}//namespace CFly