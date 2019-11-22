#include "ZumaPCH.h"
#include "D3D9RenderSystem.h"

#include "Image/GIFLoader.h"
#include "ZFont.h"

namespace Zuma
{


bool DXTexture2D::createFromRawData( LPDIRECT3DDEVICE9 d3dDevice , unsigned char* data , int w , int h )
{
	release();

	if ( FAILED( D3DXCreateTexture( 
			d3dDevice , w , h , D3DX_DEFAULT , D3DPOOL_DEFAULT, 
			D3DFMT_A8R8G8B8 , D3DPOOL_MANAGED , &m_d3dTexture
		) ) )
		return false;

	D3DLOCKED_RECT rect;
	if ( FAILED( m_d3dTexture->LockRect( 0 , &rect , NULL , D3DLOCK_DISCARD )) )
		return false;

	memcpy( rect.pBits , data , w * h * 4 );

	if ( FAILED( m_d3dTexture->UnlockRect( 0 )) )
		return false;

	mWidth  = w;
	mHeight = h;

	return true;
}

bool DXTexture2D::createFromFile(  LPDIRECT3DDEVICE9 d3dDevice , char const* path , D3DCOLOR colorKey )
{
	release();

	D3DXIMAGE_INFO     imgInfo;

	if( FAILED(D3DXGetImageInfoFromFile( path , &imgInfo) ) )
		return false;

	if ( FAILED(D3DXCreateTextureFromFileEx( 
			d3dDevice , path , imgInfo.Width , imgInfo.Height ,  D3DX_DEFAULT ,
			D3DPOOL_DEFAULT, D3DFMT_A8R8G8B8 , D3DPOOL_MANAGED , 
			D3DX_DEFAULT,D3DX_DEFAULT, colorKey , &imgInfo , NULL , &m_d3dTexture 
		) ) )
		return false;

	mWidth  = imgInfo.Width;
	mHeight = imgInfo.Height;

	return true;
}

void DXTexture2D::release()
{
	if ( m_d3dTexture )
	{
		m_d3dTexture->Release();
		m_d3dTexture = NULL;
	}
}

D3D9RenderSystem::D3D9RenderSystem() 
	:mD3DDevice(NULL)
{

}

bool D3D9RenderSystem::init( LPDIRECT3DDEVICE9 d3dDevice )
{
	mD3DDevice = d3dDevice;

	if ( FAILED( D3DXCreateSprite( d3dDevice , &mD3DSprite ) ) )
		return false;

	if ( FAILED( D3DXCreateMatrixStack( 0 , &mWorldMatStack ) ))
		return false;

	mWorldMatStack->LoadIdentity();

	if ( FAILED( d3dDevice->CreateVertexBuffer( 
		4 * sizeof( CVertex ) , 0 , CVertex::FVF , 
		D3DPOOL_DEFAULT , &mBmpVtxBuffer , NULL )) )
		return false;

	if ( FAILED( d3dDevice->CreateVertexBuffer( 
		MaxPolygonSize * sizeof( PVertex ) , 0 , PVertex::FVF , 
		D3DPOOL_DEFAULT , &mPolyVtxBuffer , NULL )) )
		return false;


	d3dDevice->SetRenderState( D3DRS_LIGHTING , FALSE );
	d3dDevice->SetRenderState( D3DRS_CULLMODE , D3DCULL_NONE );
	d3dDevice->SetRenderState( D3DRS_ZENABLE , FALSE );

	D3DXMATRIX mtx;
	D3DXMatrixOrthoOffCenterLH( &mtx , 0 , g_ScreenWidth  , 0 , g_ScreenHeight  , -100 , 100 );
	d3dDevice->SetTransform( D3DTS_PROJECTION , &mtx );

	mCurColor.r = 1.0;
	mCurColor.g = 1.0;
	mCurColor.b = 1.0;
	mCurColor.a = 1.0;

	return true;
}

bool D3D9RenderSystem::prevRender()
{
	if( FAILED( getD3DDevice()->BeginScene() ) )
		return false;

	getD3DDevice()->Clear( 0, NULL, D3DCLEAR_TARGET, D3DCOLOR_XRGB( 125, 125, 125 ), 1.0f, 0 );

	D3DXMATRIX mtx;
	D3DXMatrixOrthoOffCenterLH( &mtx , 0 , g_ScreenWidth  , 0 , g_ScreenHeight  , -100 , 100 );
	getD3DDevice()->SetTransform( D3DTS_PROJECTION , &mtx );


	D3DXVECTOR3 vEyePt( 0.0f, 0.0f, 100.0f );
	D3DXVECTOR3 vLookatPt( 0.0f, 0.0f, 0.0f );
	D3DXVECTOR3 vUpVec( 0.0f, 1.0f, 0.0f );
	D3DXMATRIXA16 matView;
	D3DXMatrixLookAtLH( &matView, &vEyePt, &vLookatPt, &vUpVec );
	getD3DDevice()->SetTransform( D3DTS_VIEW, &matView );

	return true;
}

void D3D9RenderSystem::postRender()
{
	HRESULT hr;
	hr = getD3DDevice()->EndScene();
	hr = getD3DDevice()->Present( NULL, NULL, NULL, NULL );
	if ( FAILED( hr ) )
	{
		switch( hr )
		{
			//case D3DERR_DEVICEREMOVED:
			//break;
		case D3DERR_INVALIDCALL:
			break;
		}
	}
}

void D3D9RenderSystem::cleanup()
{
	if ( mD3DSprite )
		mD3DSprite->Release();
	if ( mWorldMatStack )
		mWorldMatStack->Release();
}

bool D3D9RenderSystem::loadGIFAlpha( char const* path , TempRawData& data )
{
	return ::LoadGIFImage( path , &TempRawData::GIFCreateAlpha , &data );
}

bool D3D9RenderSystem::loadGIF( char const* path , TempRawData& data )
{
	return LoadGIFImage( path , &TempRawData::GIFCreateRGB , &data );
}

bool D3D9RenderSystem::loadGIFAlpha( DXTexture2D& texture , char const* path )
{

	D3DLOCKED_RECT rect;
	if ( FAILED( texture.getD3DTexture()->LockRect( 0 , &rect , NULL , D3DLOCK_DISCARD )) )
		return false;


	TempRawData data;
	assert( texture.getWidth() == rect.Pitch / 4 );

	data.w = texture.getWidth();
	data.h = texture.getHeight();
	data.buffer = ( unsigned char* )rect.pBits;

	bool result = ::LoadGIFImage( path , &TempRawData::GIFFillAlpha , &data );

	data.buffer = NULL;
	return result;
}



bool D3D9RenderSystem::loadTexture( ITexture2D& texture , char const* path , char const* alphaPath )
{
	texture.release();

	bool haveImage = ( path != NULL && *path != '\0');
	bool haveAlpha = ( alphaPath != NULL && *alphaPath != '\0') ;
	if ( !haveImage && !haveAlpha )
		return false;

	DXTexture2D& texDX = DXTexture2D::CastToImpl( texture );

	if ( haveImage )
	{
		if ( strstr( path , ".gif") )
		{
			TempRawData data;

			if ( !loadGIF( path , data ) ||
				!texDX.createFromRawData( getD3DDevice() , data.buffer , data.w , data.h ) )
				return false;
		}
		else if ( !texDX.createFromFile( getD3DDevice() , path , 0  ) )
			return false;

		if ( haveAlpha && !loadGIFAlpha( texDX , alphaPath ) )
			return false;
	}
	else
	{
		TempRawData data;
		if ( !loadGIFAlpha( alphaPath , data ) ||
			 !texDX.createFromRawData( getD3DDevice() , data.buffer , data.w , data.h ) )
			return false;
	}

	return true;
}

DXTexture2D* D3D9RenderSystem::createTexture( char const* path , char const* alphaPath )
{

	bool haveImage = ( path != NULL && *path != '\0');
	bool haveAlpha = ( alphaPath != NULL && *alphaPath != '\0') ;
	if ( !haveImage && !haveAlpha )
		return NULL;

	DXTexture2D* tex = new DXTexture2D;

	if ( haveImage )
	{
		if ( strstr( path , ".gif") )
		{
			TempRawData data;

			if ( !loadGIF( path , data ) ||
				!tex->createFromRawData( getD3DDevice() , data.buffer , data.w , data.h ) )
				goto fail;
		}
		else if ( !tex->createFromFile( getD3DDevice() , path , 0  ) )
			goto fail;

		if ( haveAlpha && !loadGIFAlpha( *tex , alphaPath ))
			goto fail;
	}
	else
	{
		TempRawData data;
		if ( !loadGIFAlpha( alphaPath , data ) ||
		     !tex->createFromRawData( getD3DDevice() , data.buffer , data.w , data.h ) )
			 goto fail;
	}

	return tex;

fail:
	delete tex;
	return NULL;
}

void D3D9RenderSystem::translateWorld( float x , float y  )
{
	mWorldMatStack->TranslateLocal( x , y , 0.0f );
	updateWorldMatrix();
}

void D3D9RenderSystem::rotateWorld( float angle )
{
	mWorldMatStack->RotateAxisLocal( &D3DXVECTOR3( 0 , 0 , 1 ) , PI * angle / 180.0f );
	updateWorldMatrix();
}

void D3D9RenderSystem::scaleWorld( float sx , float sy )
{
	mWorldMatStack->ScaleLocal( sx , sy , 1.0f );
	updateWorldMatrix();

}

void D3D9RenderSystem::setWorldIdentity()
{
	mWorldMatStack->LoadIdentity();
	updateWorldMatrix();
}

void D3D9RenderSystem::popWorldTransform()
{
	mWorldMatStack->Pop();
	updateWorldMatrix();
}

void D3D9RenderSystem::pushWorldTransform()
{
	mWorldMatStack->Push();
}

void D3D9RenderSystem::loadWorldMatrix( Vector2 const& pos , Vector2 const& dir )
{
	static float m[16] =
	{
		1, 0 ,0, 0,
		0, 1, 0, 0,
		0, 0, 1, 0,
		0, 0, 0, 1,
	};

	m[0] =  dir.x;  m[1] = dir.y;
	m[4] = -dir.y;  m[5] = dir.x;
	m[12] = pos.x ; m[13] = pos.y;

	mWorldMatStack->LoadMatrix( reinterpret_cast< D3DXMATRIX const*>( m ) );
	getD3DDevice()->SetTransform( D3DTS_WORLD , mWorldMatStack->GetTop() );
}

void D3D9RenderSystem::drawBitmap( ITexture2D const& tex , unsigned flag  )
{
	DXTexture2D const& texture = DXTexture2D::CastToImpl( tex  );

	pushWorldTransform();

	if ( flag & TBF_OFFSET_CENTER )
		translateWorld( -0.5f * tex.getWidth() , -0.5f * tex.getHeight() );

	mD3DSprite->Begin( D3DXSPRITE_ALPHABLEND | D3DXSPRITE_DONOTSAVESTATE );

	setupBlend( tex.ownAlpha() ,  flag );

	mD3DSprite->SetTransform( mWorldMatStack->GetTop()  );

	mD3DSprite->Draw( getD3DTexture(tex) , 
		NULL , NULL , NULL, mCurColor 
	);
	mD3DSprite->End();

	enableBlendImpl( false );

	popWorldTransform();


}

void D3D9RenderSystem::drawBitmap( ITexture2D const& tex , Vector2 const& texPos , Vector2 const& texSize, unsigned flag /*= 0 */ )
{
	DXTexture2D const& texture = DXTexture2D::CastToImpl( tex  );

	RECT rect;
	rect.left = texPos.x;
	rect.top  = texPos.y;
	rect.right= texPos.x + texSize.x;
	rect.bottom= texPos.y + texSize.y;

	pushWorldTransform();

	if ( flag & TBF_OFFSET_CENTER )
		translateWorld( -0.5f * texSize.x , -0.5f * texSize.y );

	mD3DSprite->Begin( D3DXSPRITE_ALPHABLEND | D3DXSPRITE_DONOTSAVESTATE );

	setupBlend( tex.ownAlpha() ,  flag );

	mD3DSprite->SetTransform( mWorldMatStack->GetTop() );

	mD3DSprite->Draw( getD3DTexture(tex) , &rect , NULL , NULL, mCurColor );
	
	mD3DSprite->End();

	enableBlendImpl( false );

	popWorldTransform();

}

void D3D9RenderSystem::drawBitmapWithinMask( ITexture2D const& tex , ITexture2D const& mask , Vector2 const& pos , unsigned flag /*= 0 */ )
{

	DXTexture2D const& texDX = DXTexture2D::CastToImpl( tex );

	setupBlend( tex.ownAlpha() ,  flag );

	float tx = pos.x / tex.getWidth();
	float ty = pos.y / tex.getHeight();

	float tw = tx + float(mask.getWidth()) / tex.getWidth();
	float th = ty + float(mask.getHeight()) / tex.getHeight();

	CVertex* v;
	if( FAILED( mBmpVtxBuffer->Lock( 0, 4 * sizeof( CVertex ), ( void** )&v, 0 ) ) )
		return;

	v[0].t1u = tx; v[0].t1v = ty;
	v[0].t2u = 0;  v[0].t2v =  0;
	v[0].x = 0;
	v[0].y = 0;
	v[0].z = 0;

	v[1].t1u = tw; v[1].t1v = ty;
	v[1].t2u = 1;  v[1].t2v =  0;
	v[1].x = mask.getWidth();
	v[1].y = 0;
	v[1].z = 0;

	v[3].t1u = tw; v[2].t1v = th;
	v[3].t2u = 1;  v[2].t2v =  1;
	v[3].x = mask.getWidth();
	v[3].y = mask.getHeight();
	v[3].z = 0;
	
	v[2].t1u = tx; v[3].t1v = th;
	v[2].t2u = 0;  v[3].t2v =  1;
	v[2].x = 0;
	v[2].y = mask.getHeight();
	v[2].z = 0;

	v[0].color = mCurColor;
	v[1].color = mCurColor;
	v[2].color = mCurColor;
	v[3].color = mCurColor;

	mBmpVtxBuffer->Unlock();

	LPDIRECT3DDEVICE9 d3dDevice = getD3DDevice();

	pushWorldTransform();

	if ( flag & TBF_OFFSET_CENTER )
		translateWorld( -0.5f * mask.getWidth() , -0.5f * mask.getHeight() );

	bindTexture( 0 , tex );
	d3dDevice->SetTextureStageState( 0, D3DTSS_TEXCOORDINDEX, 0 );
	d3dDevice->SetTextureStageState( 0, D3DTSS_COLOROP, D3DTOP_MODULATE );
	d3dDevice->SetTextureStageState( 0, D3DTSS_COLORARG1, D3DTA_TEXTURE );
	d3dDevice->SetTextureStageState( 0, D3DTSS_COLORARG2, D3DTA_DIFFUSE );
	d3dDevice->SetTextureStageState( 0, D3DTSS_ALPHAOP , D3DTOP_DISABLE );
	//d3dDevice->SetTextureStageState( 0, D3DTSS_COLORARG1, D3DTA_TEXTURE );

	bindTexture( 1 , mask );
	d3dDevice->SetTextureStageState( 1, D3DTSS_TEXCOORDINDEX , 1 );
	d3dDevice->SetTextureStageState( 1, D3DTSS_COLOROP, D3DTOP_SELECTARG1 );
	d3dDevice->SetTextureStageState( 1, D3DTSS_COLORARG1, D3DTA_CURRENT );
	d3dDevice->SetTextureStageState( 1, D3DTSS_ALPHAOP, D3DTOP_SELECTARG1 );
	d3dDevice->SetTextureStageState( 1, D3DTSS_ALPHAARG1 , D3DTA_TEXTURE );

	d3dDevice->SetTextureStageState( 2 , D3DTSS_COLOROP , D3DTOP_DISABLE );

	d3dDevice->SetStreamSource( 0, mBmpVtxBuffer , 0, sizeof( CVertex ) );
	d3dDevice->SetFVF( CVertex::FVF );
	d3dDevice->DrawPrimitive( D3DPT_TRIANGLESTRIP , 0, 2 );

	enableBlendImpl( false );

	popWorldTransform();

}


void D3D9RenderSystem::setColor( uint8 r , uint8 g , uint8 b , uint8 a /*= 255 */ )
{
	mCurColor.r = FLOAT(r) / 255.0f;
	mCurColor.g = FLOAT(g) / 255.0f;
	mCurColor.b = FLOAT(b) / 255.0f;
	mCurColor.a = FLOAT(a) / 255.0f;

	//m_curColor = D3DCOLOR_ARGB( 255 , 255 , 255 , 255 );
}

D3DBLEND D3D9RenderSystem::convBlendValue( BlendEnum value )
{
	switch ( value )
	{
	case BLEND_ONE :         return D3DBLEND_ONE;
	case BLEND_ZERO :        return D3DBLEND_ZERO;
	case BLEND_SRCALPHA:     return D3DBLEND_SRCALPHA;
	case BLEND_DSTALPHA:     return D3DBLEND_DESTALPHA;
	case BLEND_INV_SRCALPHA: return D3DBLEND_INVSRCALPHA;
	case BLEND_INV_DSTALPHA: return D3DBLEND_INVDESTALPHA;
	case BLEND_SRCCOLOR:     return D3DBLEND_SRCCOLOR;
	case BLEND_DSTCOLOR:     return D3DBLEND_DESTCOLOR;
	case BLEND_INV_SRCCOLOR: return D3DBLEND_INVSRCCOLOR;
	case BLEND_INV_DSTCOLOR: return D3DBLEND_INVDESTCOLOR;
	default:
		assert(0);
	}
	return D3DBLEND_ONE;
}

void D3D9RenderSystem::setupBlendFun( BlendEnum src , BlendEnum dst )
{
	getD3DDevice()->SetRenderState( D3DRS_SRCBLEND,  convBlendValue( src ) );
	getD3DDevice()->SetRenderState( D3DRS_DESTBLEND , convBlendValue( dst)  );
}

void D3D9RenderSystem::enableBlendImpl( bool beE )
{
	getD3DDevice()->SetRenderState( D3DRS_ALPHABLENDENABLE , ( beE ) ? TRUE : FALSE );
}


ITexture2D* D3D9RenderSystem::createTextureRes( ImageResInfo& info )
{
	DXTexture2D* tex = createTexture( info.path.c_str() , info.pathAlpha.c_str() );

	if ( tex )
	{
		tex->col = info.numCol;
		tex->row = info.numRow;
	}
	return tex;
}

void D3D9RenderSystem::updateWorldMatrix()
{
	//m_worldMatStack->LoadMatrix( &mat );
	getD3DDevice()->SetTransform( D3DTS_WORLD ,  mWorldMatStack->GetTop() );
}

ITexture2D* D3D9RenderSystem::createFontTexture( ZFontLayer& layer )
{
	return createTexture( layer.imagePath.c_str() ,layer.alhpaPath.c_str() );
}

void D3D9RenderSystem::drawPolygon( Vector2 const pos[] , int num )
{
	assert( num <= MaxPolygonSize );

	//mD3DDevice->SetTexture( 0 , NULL );

	PVertex* v;

	if ( FAILED( mPolyVtxBuffer->Lock( 0 , MaxPolygonSize * sizeof(PVertex) , (void**)&v, 0 ) ))
		return;

	for( int i = 0; i < num; ++i )
	{
		v[i].x = pos[i].x;
		v[i].y = pos[i].y;
		v[i].z = 0.0f;
		v[i].color = mCurColor;
	}

	mPolyVtxBuffer->Unlock();

	mD3DDevice->SetStreamSource( 0 , mPolyVtxBuffer , 0 , sizeof( PVertex ) );
	mD3DDevice->SetFVF( PVertex::FVF );
	mD3DDevice->DrawPrimitive( D3DPT_TRIANGLEFAN , 0 , num  - 2 );
}



}//namespace Zuma