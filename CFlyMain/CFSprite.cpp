#include "CFlyPCH.h"
#include "CFSprite.h"

#include "CFWorld.h"
#include "CFScene.h"
#include "CFRenderSystem.h"

namespace CFly
{
	LPDIRECT3DSTATEBLOCK9 gSpriteRenderStateBlock = NULL;

	Sprite::Sprite( Scene* scene ) 
		:Object( scene )
	{
		mUseBoundSphere = false;

		if ( gSpriteRenderStateBlock == NULL )
		{
			D3DDevice* d3dDevice = scene->getD3DDevice();

			d3dDevice->CreateStateBlock( D3DSBT_ALL , &gSpriteRenderStateBlock );

			d3dDevice->BeginStateBlock();

			d3dDevice->SetTextureStageState( 0 , D3DTSS_TEXCOORDINDEX , 0 );
			d3dDevice->SetTextureStageState( 0 , D3DTSS_COLOROP, D3DTOP_SELECTARG1 );
			d3dDevice->SetTextureStageState( 0 , D3DTSS_COLORARG1 , D3DTA_TEXTURE );
			d3dDevice->SetTextureStageState( 0 , D3DTSS_ALPHAOP , D3DTOP_SELECTARG1 );
			d3dDevice->SetTextureStageState( 0 , D3DTSS_ALPHAARG1 , D3DTA_TEXTURE );

			d3dDevice->SetTextureStageState( 1 , D3DTSS_COLOROP   , D3DTOP_MODULATE );
			d3dDevice->SetTextureStageState( 1 , D3DTSS_COLORARG1 , D3DTA_CURRENT );
			d3dDevice->SetTextureStageState( 1 , D3DTSS_COLORARG2 , D3DTA_CONSTANT );
			d3dDevice->SetTextureStageState( 1 , D3DTSS_ALPHAOP   , D3DTOP_SELECTARG1 );
			d3dDevice->SetTextureStageState( 1 , D3DTSS_ALPHAARG1 , D3DTA_CURRENT );
			d3dDevice->SetTextureStageState( 1 , D3DTSS_ALPHAARG2 , D3DTA_CONSTANT );

			d3dDevice->SetTextureStageState( 2 , D3DTSS_COLOROP  , D3DTOP_DISABLE );

			d3dDevice->SetRenderState( D3DRS_CULLMODE , D3DCULL_NONE );
			d3dDevice->SetRenderState( D3DRS_ZENABLE , TRUE );
			d3dDevice->SetRenderState( D3DRS_ZWRITEENABLE , TRUE );

			d3dDevice->SetFVF( SprVertex::FVF );

			d3dDevice->EndStateBlock( &gSpriteRenderStateBlock );	
		}
	}

	void Sprite::renderRectUnits( Matrix4 const& trans )
	{
		Scene* scene = getScene();
		RenderSystem* renderSysetem = getScene()->_getRenderSystem();

		renderSysetem->setWorldMatrix( trans );

		D3DDevice* d3dDevice = scene->getD3DDevice();
		scene->_setupRenderOption( mRenderOption , mUsageRenderOption );

		gSpriteRenderStateBlock->Apply();

		for( int i = 0 ; i < mRectUnits.size() ; ++i )
		{
			RectUnit& unit = mRectUnits[i];
			if ( !unit.beUsed )
				continue;

			D3DTEXTUREFILTERTYPE d3dFilter = D3DTypeMapping::convert( unit.filter );

			d3dDevice->SetSamplerState( 0 , D3DSAMP_MAGFILTER , d3dFilter );
			d3dDevice->SetSamplerState( 0 , D3DSAMP_MINFILTER , d3dFilter );

			Texture* tex = unit.texSlot[ unit.curSlot ];
			if ( tex )
				d3dDevice->SetTexture( 0 , tex->getD3DTexture());
			else
				d3dDevice->SetTexture( 0 , 0 );

			d3dDevice->SetTextureStageState( 1 , D3DTSS_CONSTANT  ,  *reinterpret_cast< DWORD* >( &unit.color) );


			//if ( tex && tex->checkFlag( Texture::TF_USAGE_COLOR_KEY ) ) 
			//{
			//	d3dDevice->SetRenderState( D3DRS_ALPHATESTENABLE , TRUE );
			//	d3dDevice->SetRenderState( D3DRS_ALPHAREF , 0x00000002 );
			//	d3dDevice->SetRenderState( D3DRS_ALPHAFUNC , D3DCMP_GREATER );
			//}
			//else
			//{
			//	d3dDevice->SetRenderState( D3DRS_ALPHATESTENABLE , FALSE );
			//}

			unit.buffer->setupDevice( d3dDevice , 0 );
			d3dDevice->DrawPrimitive( D3DPT_TRIANGLEFAN , 0 , 2 );
		}

		scene->_setupDefultRenderOption( mUsageRenderOption );
	}

	float evalTexcoord( SpriteTextureMapping method , int len , int tlen )
	{
		switch( method )
		{
		case STM_FILL: 
			return 1.0f;
		case STM_WARP:
		case STM_NONE: 
			return float( len ) / tlen;
		}
		return 0;
	}

	bool Sprite::setupRectUnit( RectUnit& unit , int ox , int oy , int w, int h , float depth , Texture* texture[] , int nTex , Color4ub const* color ,FilterMode filter , SpriteTextureMapping methodX , SpriteTextureMapping methodY)
	{
		assert( texture[0] && texture[0]->getType() == CFT_TEXTURE_2D );

		unsigned tw , th;
		texture[0]->getSize( tw , th );

		if ( methodX == STM_NONE && w > tw )
			w = tw;

		if ( methodY == STM_NONE && h > th )
			h = th;

		unit.width  = w;
		unit.height = h;

		if ( color )
		{
			unit.color = *color;
		}
		else
		{
			unit.color = Color4ub(255,255,255,255);
		}

		unit.texSlot.assign( texture , texture + nTex );
		unit.curSlot = 0;

		unit.filter = filter;

		MeshCreator* manager = getScene()->getWorld()->_getMeshCreator();

		if ( !unit.buffer )
			unit.buffer = manager->createVertexBufferFVF( SprVertex::FVF , sizeof( SprVertex ) , 4 );

		if ( !unit.buffer )
			return false;

		SprVertex* vtx = reinterpret_cast< SprVertex*>( unit.buffer->lock() );

		if ( !vtx )
			return false;

		int x2 = ox + w;
		int y2 = oy + h;

		vtx[0].pos = Vector3( ox , oy , depth );
		vtx[1].pos = Vector3( x2 , oy , depth );
		vtx[2].pos = Vector3( x2 , y2 , depth );
		vtx[3].pos = Vector3( ox , y2 , depth );

		//mVertex[0].color = D3DCOLOR(mRectColor);
		//mVertex[1].color = D3DCOLOR(mRectColor);
		//mVertex[2].color = D3DCOLOR(mRectColor);
		//mVertex[3].color = D3DCOLOR(mRectColor);

		float tx = evalTexcoord( methodX , w , tw );
		float ty = evalTexcoord( methodY , h , th );

		vtx[0].tex[0] = 0; 
		vtx[0].tex[1] = 0;

		vtx[1].tex[0] = tx; 
		vtx[1].tex[1] = 0;

		vtx[2].tex[0] = tx; 
		vtx[2].tex[1] = ty;

		vtx[3].tex[0] = 0; 
		vtx[3].tex[1] = ty;

		unit.buffer->unlock();

		unit.beUsed = true;

		return true;
	}


	void Sprite::renderImpl( Matrix4 const& trans )
	{
		Object::renderImpl( trans );

		if ( !mRectUnits.empty() )
			renderRectUnits( trans );
	}

	void Sprite::release()
	{
		getScene()->_destroySprite( this );
	}

	int Sprite::fetchEmptyRectUnit()
	{
		for( int i = 0 ; i < mRectUnits.size(); ++i )
		{
			if ( !mRectUnits[i].beUsed )
				return i;
		}

		int idx = mRectUnits.size();
		mRectUnits.resize( idx + 1 );
		return idx;
	}

	int Sprite::createRectArea( int ox , int oy , int w , int h , Texture* texture[] ,int nTex , float depth , Color4ub const* color /*= nullptr */, FilterMode filter , SpriteTextureMapping methodX , SpriteTextureMapping methodY )
	{
		if ( !texture[0] )
			return CF_FAIL_ID;

		int idx = fetchEmptyRectUnit();
		if ( !setupRectUnit( mRectUnits[idx] , ox , oy , w , h , depth , texture , nTex , color , filter , methodX , methodY ) )
			return CF_FAIL_ID;

		return idx;
	}

	int Sprite::createRectArea( int ox , int oy , int w , int h , char const* tex, int nTex, float depth , Color4ub const* color /*= nullptr */, Color3ub const* colorkey , FilterMode filter  , SpriteTextureMapping methodX , SpriteTextureMapping methodY)
	{
		MaterialManager* manager = getScene()->getWorld()->_getMaterialManager();

		Texture* texture[ 32 ];
		assert( nTex < 32 );

		if ( nTex )
		{
			char fullName[ 256 ];
			for ( int i = 0 ; i < nTex ; ++i )
			{
				sprintf_s( fullName , sizeof( fullName ) , tex , i );
				texture[i] = manager->createTexture( fullName , CFT_TEXTURE_2D , colorkey , 1  );
			}
		}
		else
		{
			texture[0] = manager->createTexture( tex , CFT_TEXTURE_2D , colorkey , 1  );
			nTex = 1;
		}

		if ( texture[0] )
			return createRectArea( ox , oy , w , h , texture , nTex , depth , color  , filter , methodX , methodY );

		return CF_FAIL_ID;
	}

	void Sprite::setRectColor( unsigned id , Color4ub const& color )
	{
		if ( id >= mRectUnits.size() )
			return;
		mRectUnits[ id ].color = color;
	}

	void Sprite::removeRectArea( unsigned id )
	{
		if ( id >= mRectUnits.size() )
			return;

		mRectUnits[id].beUsed = false;
		mRectUnits[id].texSlot.clear();
	}

	void Sprite::setRectTextureSlot( unsigned id , unsigned slot )
	{
		if ( id >= mRectUnits.size() )
			return;

		RectUnit& unit = mRectUnits[ id ];
		if ( slot >= unit.texSlot.size() )
			return;

		unit.curSlot = slot;
	}



};//namespace CFly