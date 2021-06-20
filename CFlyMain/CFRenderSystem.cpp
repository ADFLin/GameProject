#include "CFlyPCH.h"
#include "CFRenderSystem.h"

#include "CFBuffer.h"
#include "CFMaterial.h"
#include "CFLight.h"


namespace CFly
{
	static D3DBLEND             sBlendModeMap[ CF_BLEND_MODE_NUM ];
	static RenderOptionMap      sRenderOptionMap[ CFRO_OPTION_NUM ];
	static D3DTEXTUREFILTERTYPE sFilterModeMap[ CF_FILTER_MODE_NUM ];
	IDirect3D9* RenderSystem::s_d3d = nullptr;

	bool RenderSystem::init( HWND hWnd , int w , int h , bool fullscreen , TextureFormat backBufferFormat )
	{
		mhDestWnd     = hWnd;
		mFullScreen   = fullscreen;
		mBufferWidth  = w;
		mBufferHeight = h;

		D3DPRESENT_PARAMETERS d3dpp;
		_setDefaultPresentParam( d3dpp );
		d3dpp.hDeviceWindow  = hWnd;
		if ( backBufferFormat != CF_TEX_FMT_DEFAULT )
			d3dpp.BackBufferFormat = D3DTypeMapping::convert( backBufferFormat );
		
		if( FAILED( s_d3d->CreateDevice(
			D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, hWnd ,
			//D3DCREATE_SOFTWARE_VERTEXPROCESSING, 
			//D3DCREATE_PUREDEVICE | D3DCREATE_HARDWARE_VERTEXPROCESSING , 
			D3DCREATE_MIXED_VERTEXPROCESSING ,
			&d3dpp , &mD3dDevice
			) ) )
		{
			return false;
		}

		return true;
	}

	void RenderSystem::_setDefaultPresentParam( D3DPRESENT_PARAMETERS& d3dpp )
	{
		D3DDISPLAYMODE d3ddm;
		s_d3d->GetAdapterDisplayMode( D3DADAPTER_DEFAULT, &d3ddm );

		ZeroMemory( &d3dpp, sizeof( d3dpp ) );
		d3dpp.BackBufferWidth      = mBufferWidth;
		d3dpp.BackBufferHeight     = mBufferHeight;
		d3dpp.Windowed             = !mFullScreen;
		d3dpp.SwapEffect           = D3DSWAPEFFECT_DISCARD;
		d3dpp.BackBufferFormat     = d3ddm.Format;
		d3dpp.EnableAutoDepthStencil = TRUE;
		d3dpp.AutoDepthStencilFormat = getDepthStencilFormat();
		d3dpp.PresentationInterval   = D3DPRESENT_INTERVAL_IMMEDIATE;
		
	}

	bool RenderSystem::initSystem()
	{
		s_d3d = ::Direct3DCreate9( D3D_SDK_VERSION );
		if ( s_d3d == NULL )
			return false;

		if ( !D3DTypeMapping::init() )
			return false;

		return true;
	}

	D3DFORMAT RenderSystem::getDepthStencilFormat()
	{
		return D3DFMT_D24S8;
	}

	void RenderSystem::renderPrimitive( EPrimitive type , unsigned numEle , unsigned numVertices , IndexBuffer* indexBuffer )
	{
		HRESULT hr;
		CF_PROFILE("renderPrimitive");

		D3DPRIMITIVETYPE d3dPrimitiveType = D3DTypeMapping::convert( type );
		if ( indexBuffer )
		{
			mD3dDevice->SetIndices( indexBuffer->getSystemImpl() );
			hr = mD3dDevice->DrawIndexedPrimitive( d3dPrimitiveType , 0 , 0 , numVertices, 0 , numEle  );
		}
		else
		{
			hr = mD3dDevice->DrawPrimitive( d3dPrimitiveType , 0 , numEle );
		}
	}


	void RenderSystem::setProjectMatrix( Matrix4 const& proj )
	{
		mMatProject = proj;

		getD3DDevice()->SetTransform( D3DTS_PROJECTION , 
			reinterpret_cast< D3DMATRIX const* >( &mMatProject ) );
	}

	void RenderSystem::setViewMatrix( Matrix4 const& view )
	{
		mMatView = view;
		mMatViewRS = view;
#if 1
		mMatViewRS(0,2) = -mMatViewRS(0,2);
		mMatViewRS(1,2) = -mMatViewRS(1,2);
		mMatViewRS(2,2) = -mMatViewRS(2,2);
		mMatViewRS(3,2) = -mMatViewRS(3,2);
#endif
		getD3DDevice()->SetTransform( D3DTS_VIEW , 
			reinterpret_cast< D3DMATRIX const* >( &mMatViewRS ) );
	}

	void RenderSystem::setWorldMatrix( Matrix4 const& world )
	{
		mMatWorld = world;
		getD3DDevice()->SetTransform( D3DTS_WORLD , 
			reinterpret_cast< D3DMATRIX const* >( &mMatWorld ) );
	}

	void RenderSystem::setWorldMatrix( int idx , Matrix4 const& world )
	{
		getD3DDevice()->SetTransform( D3DTS_WORLDMATRIX(idx ) , 
			reinterpret_cast< D3DXMATRIX const* >( &world ) );
	}

	void RenderSystem::setupDefaultOption( unsigned renderOptionBit )
	{
		for( int i = 0 ; i < CFRO_OPTION_NUM && renderOptionBit ; ++i )
		{
			unsigned bit = 1 << i;
			if ( renderOptionBit & bit )
			{
				mD3dDevice->SetRenderState( 
					sRenderOptionMap[i].d3dRS , 
					sRenderOptionMap[i].defaultValue );
				renderOptionBit -= bit;
			}
		}
	}

	void RenderSystem::setupOption( DWORD* renderOption , unsigned renderOptionBit )
	{
		for( int i = 0 ; renderOptionBit && i < CFRO_OPTION_NUM ; ++i )
		{
			if ( renderOptionBit & 0x01 )
			{
				D3DRENDERSTATETYPE rsType = sRenderOptionMap[i].d3dRS;
				DWORD val = renderOption[i];
				mD3dDevice->SetRenderState( rsType , val );
			}
			renderOptionBit >>= 1;
		}
	}

	RenderWindow* RenderSystem::createRenderWindow( HWND hWnd )
	{
		RenderWindow* window = new RenderWindow( this , hWnd , mBufferWidth , mBufferHeight , true );
		return window;
	}

	void RenderSystem::setFillMode( RenderMode mode )
	{
		DWORD fillmode = D3DFILL_SOLID;
		switch ( mode )
		{
		case CFRM_POINT:     fillmode = D3DFILL_POINT; break;
		case CFRM_WIREFRAME: fillmode = D3DFILL_WIREFRAME; break;
		}
		mD3dDevice->SetRenderState( D3DRS_FILLMODE , fillmode );
	}

	void RenderSystem::setupStream( int idx , VertexBuffer& buffer )
	{

	}

	void RenderSystem::setAmbientColor( Color4f const& color )
	{
		mD3dDevice->SetRenderState( D3DRS_AMBIENT , color.toARGB() );
	}

	void RenderSystem::setLighting( bool beE )
	{
		mD3dDevice->SetRenderState( D3DRS_LIGHTING , beE );
	}

	void RenderSystem::setLight( int idx , bool bE )
	{
		mD3dDevice->LightEnable( idx , bE );
	}

	void RenderSystem::setupLight( int idx , Light& light )
	{
		D3DLIGHT9& d3dLight = light.mD3dLight;

		Vector3 pos = light.getWorldPosition();
		Vector3 dir = light.getLightDir();

		d3dLight.Position.x = pos.x;
		d3dLight.Position.y = pos.y;
		d3dLight.Position.z = pos.z;

		d3dLight.Direction.x = dir.x;
		d3dLight.Direction.y = dir.y;
		d3dLight.Direction.z = dir.z;

		mD3dDevice->SetLight( idx , &d3dLight );
	}


	void RenderSystem::setMaterialColor( Color4f const& amb , Color4f const& dif , Color4f const& spe , Color4f const& emi , float power )
	{
		D3DMATERIAL9    d3dMaterial;
		d3dMaterial.Ambient  = D3DTypeMapping::convert( amb );
		d3dMaterial.Diffuse  = D3DTypeMapping::convert( dif );
		d3dMaterial.Specular = D3DTypeMapping::convert( spe );
		d3dMaterial.Emissive = D3DTypeMapping::convert( emi );
		d3dMaterial.Power    = power;
		HRESULT hr = mD3dDevice->SetMaterial( &d3dMaterial );

	}

	void RenderSystem::setupViewport( Viewport& vp )
	{
		mD3dDevice->SetViewport( &vp.mD3dVP );
	}

	void RenderSystem::setRenderTarget( RenderTarget& rt , int idxTarget )
	{
		IDirect3DSurface9* prevSurface;
		mD3dDevice->GetRenderTarget( idxTarget , &prevSurface );
		if ( prevSurface != rt.mColorSurface )
		{
			HRESULT hr = mD3dDevice->SetRenderTarget( idxTarget  , rt.mColorSurface );
			if ( hr != D3D_OK )
			{
			}
		}

		if ( rt.mDepthSurface )
		{
			HRESULT hr = mD3dDevice->SetDepthStencilSurface( rt.mDepthSurface );
			if ( hr != D3D_OK )
			{

			}
		}
	}

	void RenderSystem::setVertexBlend( VertexType vertexType )
	{
		DWORD const BlendMatrixMap[] = { D3DVBF_0WEIGHTS , D3DVBF_1WEIGHTS , D3DVBF_2WEIGHTS , D3DVBF_3WEIGHTS };

		if ( vertexType & ( CFV_BLENDWEIGHT | CFV_BLENDINDICES ) )
		{
			int numBlendWeight = ( vertexType & CFV_BLENDWEIGHT ) ? CFVF_BLENDWEIGHTSIZE_GET( vertexType ) : 0;
			mD3dDevice->SetRenderState( D3DRS_VERTEXBLEND , BlendMatrixMap[ numBlendWeight ] );
		}
		else
		{
			mD3dDevice->SetRenderState( D3DRS_VERTEXBLEND , D3DVBF_DISABLE );
		}

		if ( vertexType & CFV_BLENDINDICES )
		{
			mD3dDevice->SetRenderState( D3DRS_INDEXEDVERTEXBLENDENABLE , TRUE );
		}
		else
		{
			mD3dDevice->SetRenderState( D3DRS_INDEXEDVERTEXBLENDENABLE , FALSE );
		}
	}

	void RenderSystem::setupTexAdressMode( int idxSampler , TextureType texType ,  TexAddressMode mode[] )
	{
		D3DDevice* d3dDevice = getD3DDevice();

		DWORD modeD3D[3] = { D3DTADDRESS_WRAP , D3DTADDRESS_WRAP , D3DTADDRESS_WRAP };
		for( int i = 0 ; i < 3 ; ++i )
		{
			switch( mode[i] )
			{
			case CFAM_WRAP   : modeD3D[i] = D3DTADDRESS_WRAP; break;
			case CFAM_MIRROR : modeD3D[i] = D3DTADDRESS_MIRROR; break;
			case CFAM_CLAMP  : modeD3D[i] = D3DTADDRESS_CLAMP; break;
			case CFAM_BORDER : modeD3D[i] = D3DTADDRESS_BORDER; break;
			case CFAM_MIRROR_ONCE : modeD3D[i] = D3DTADDRESS_MIRRORONCE; break;
			}
		}

		switch( texType )
		{
		case CFT_TEXTURE_1D:
			d3dDevice->SetSamplerState( idxSampler , D3DSAMP_ADDRESSU , modeD3D[0] );
			break;
		case CFT_TEXTURE_2D:
			d3dDevice->SetSamplerState( idxSampler , D3DSAMP_ADDRESSU , modeD3D[0] );
			d3dDevice->SetSamplerState( idxSampler , D3DSAMP_ADDRESSV , modeD3D[1] );
			break;
		case CFT_TEXTURE_CUBE_MAP:
		case CFT_TEXTURE_3D:
			d3dDevice->SetSamplerState( idxSampler , D3DSAMP_ADDRESSU , modeD3D[0] );
			d3dDevice->SetSamplerState( idxSampler , D3DSAMP_ADDRESSV , modeD3D[1] );
			d3dDevice->SetSamplerState( idxSampler , D3DSAMP_ADDRESSW , modeD3D[2] );
			break;
		}
	}

	void RenderSystem::setupTexture(int idxSampler , Texture* texture , int idxCoord )
	{
		D3DDevice* d3dDevice = getD3DDevice();
		d3dDevice->SetTextureStageState( idxSampler , D3DTSS_TEXCOORDINDEX , idxCoord );
		d3dDevice->SetTexture( idxSampler , texture->mTex2D );
	}

	HDC RenderSystem::getBackBufferDC()
	{
		D3DDevice* d3dDevice = getD3DDevice();
		IDirect3DSurface9 * pSurface = 0;
		if ( d3dDevice->GetBackBuffer(0, 0, D3DBACKBUFFER_TYPE_MONO, &pSurface ) != D3D_OK )
			return NULL;
		HDC hDC;
		pSurface->GetDC( &hDC );
		return hDC;
	}

	DWORD D3DTypeMapping::getDefaultValue( RenderOption option )
	{
		return sRenderOptionMap[ option ].defaultValue;
	}

	D3DPRIMITIVETYPE D3DTypeMapping::convert( EPrimitive type )
	{
		return (D3DPRIMITIVETYPE )type;
	}

	D3DTEXTUREFILTERTYPE D3DTypeMapping::convert( FilterMode fiter )
	{
		return sFilterModeMap[ fiter ];
	}

	D3DCULL D3DTypeMapping::convert( CullFace cullface )
	{
		switch( cullface )
		{
		case CF_CULL_NONE:  return D3DCULL_NONE; break;
		case CF_CULL_CCW:   return D3DCULL_CCW; break;
		case CF_CULL_CW:    return D3DCULL_CW; break;
		}
		return D3DCULL_FORCE_DWORD;
	}

	D3DBLEND D3DTypeMapping::convert( BlendMode blendMode )
	{
		return sBlendModeMap[ blendMode ];
	}

	D3DFORMAT D3DTypeMapping::convert( TextureFormat format )
	{
		D3DFORMAT d3dFormat = D3DFMT_UNKNOWN;
		switch ( format )
		{
		case CF_TEX_FMT_ARGB32: d3dFormat = D3DFMT_A8R8G8B8; break;
		case CF_TEX_FMT_RGB32 : d3dFormat = D3DFMT_X8R8G8B8; break;
		case CF_TEX_FMT_RGB24 : d3dFormat = D3DFMT_R8G8B8; break;
		case CF_TEX_FMT_R5G6B5: d3dFormat = D3DFMT_R5G6B5; break;
		case CF_TEX_FMT_R32F:   d3dFormat = D3DFMT_R32F; break;
		default:
			assert(0);
		}
		return d3dFormat;
	}

	D3DCOLORVALUE const& D3DTypeMapping::convert( Color4f const& color )
	{
		return *reinterpret_cast< D3DCOLORVALUE const* >( &color );
	}

	bool D3DTypeMapping::init()
	{
		sBlendModeMap[ CF_BLEND_ZERO ] = D3DBLEND_ZERO;
		sBlendModeMap[ CF_BLEND_ONE ]  = D3DBLEND_ONE;
		sBlendModeMap[ CF_BLEND_SRC_COLOR ]= D3DBLEND_SRCCOLOR;
		sBlendModeMap[ CF_BLEND_INV_SRC_COLOR ]= D3DBLEND_INVSRCCOLOR;
		sBlendModeMap[ CF_BLEND_SRC_ALPHA ] = D3DBLEND_SRCALPHA;
		sBlendModeMap[ CF_BLEND_INV_SRC_ALPHA ]= D3DBLEND_INVSRCALPHA;
		sBlendModeMap[ CF_BLEND_DEST_ALPHA ] = D3DBLEND_DESTALPHA;
		sBlendModeMap[ CF_BLEND_INV_DEST_ALPHA ] = D3DBLEND_INVDESTALPHA;
		sBlendModeMap[ CF_BLEND_DEST_COLOR ] = D3DBLEND_DESTCOLOR;
		sBlendModeMap[ CF_BLEND_INV_DESTCOLOR ] = D3DBLEND_INVDESTCOLOR;

		sFilterModeMap[ CF_FILTER_NONE  ] = D3DTEXF_NONE;
		sFilterModeMap[ CF_FILTER_POINT ] = D3DTEXF_POINT; 
		sFilterModeMap[ CF_FILTER_LINEAR  ] = D3DTEXF_LINEAR;
		sFilterModeMap[ CF_FILTER_ANISOTROPIC ] = D3DTEXF_ANISOTROPIC;
		sFilterModeMap[ CF_FILTER_FLAT_CUBIC ] = D3DTEXF_PYRAMIDALQUAD;
		sFilterModeMap[ CF_FILTER_GAUSSIAN_CUBIC ] = D3DTEXF_GAUSSIANQUAD;

#define RENDER_OPTION( CFOP , D3DOP , VALUE )\
		sRenderOptionMap[ CFOP ].d3dRS        = D3DOP ;\
		sRenderOptionMap[ CFOP ].defaultValue = VALUE ;

		RENDER_OPTION( CFRO_Z_TEST         , D3DRS_ZENABLE          , D3DZB_TRUE )
		RENDER_OPTION( CFRO_Z_BUFFER_WRITE , D3DRS_ZWRITEENABLE     , TRUE )
		RENDER_OPTION( CFRO_ALPHA_TEST     , D3DRS_ALPHATESTENABLE  , FALSE )
		RENDER_OPTION( CFRO_ZBIAS          , D3DRS_DEPTHBIAS        , 0 )
		RENDER_OPTION( CFRO_ALPHA_BLENGING , D3DRS_ALPHABLENDENABLE , FALSE )
		RENDER_OPTION( CFRO_SRC_BLEND      , D3DRS_SRCBLEND         , D3DBLEND_SRCALPHA )
		RENDER_OPTION( CFRO_DEST_BLEND     , D3DRS_DESTBLEND        , D3DBLEND_INVSRCALPHA )
		RENDER_OPTION( CFRO_LIGHTING       , D3DRS_LIGHTING         , FALSE )
		RENDER_OPTION( CFRO_FOG            , D3DRS_FOGENABLE        , FALSE )
		RENDER_OPTION( CFRO_Z_BUFFER_WRITE , D3DRS_ZWRITEENABLE     , TRUE )
		RENDER_OPTION( CFRO_CULL_FACE      , D3DRS_CULLMODE         , D3DCULL_CW )

#undef  RENDER_OPTION

		return true;
	}

	RenderWindow::RenderWindow( RenderSystem* system , HWND hWnd , int w, int h , bool beAddional ) 
		:RenderTarget( nullptr )
		,mhWnd(hWnd)
		,mWidth(w)
		,mHeight(h)
	{
		D3DDevice* device = system->getD3DDevice();

		if ( !beAddional )
		{
			device->GetSwapChain( 0 , &mSwapChain );
			device->GetDepthStencilSurface( &mDepthSurface );
		}
		else
		{
			D3DPRESENT_PARAMETERS d3dpp;
			system->_setDefaultPresentParam( d3dpp );
			d3dpp.hDeviceWindow    = hWnd;
			d3dpp.BackBufferWidth  = w;
			d3dpp.BackBufferHeight = h;
			device->CreateAdditionalSwapChain( &d3dpp , &mSwapChain );
		}

		mSwapChain->GetBackBuffer( 0, D3DBACKBUFFER_TYPE_MONO, &mColorSurface );
	}

	void RenderWindow::resize( D3DDevice* device , int w , int h )
	{

		//IDirect3DSurface9* cSurface = nullptr;
		//IDirect3DSurface9* dSurface = nullptr;
		//D3DSURFACE_DESC desc;
		//mColorSurface->GetDesc( &desc );
		//HRESULT hr;
		//hr = device->CreateRenderTarget( w , h , desc.Format , D3DMULTISAMPLE_NONE , 0 , TRUE , &cSurface , NULL );

		//if ( FAILED( hr ) )
		//	return;

		//mDepthSurface->GetDesc( &desc );
		//device->CreateDepthStencilSurface( w , h , desc.Format , D3DMULTISAMPLE_NONE , 0 , TRUE , &dSurface , NULL );

		//if ( FAILED( hr ) )
		//{
		//	cSurface->Release();
		//	return;
		//}

		//mColorSurface->Release();
		//mColorSurface = cSurface;

		//mDepthSurface->Release();
		//mDepthSurface = dSurface;

		//mWidth = w;
		//mHeight = h;
	}

	void RenderWindow::swapBuffers()
	{
		RECT rect;
		::GetClientRect( mhWnd , &rect );
		mSwapChain->Present( &rect , NULL , NULL , NULL , 0 );
	}


}//namespace CFly