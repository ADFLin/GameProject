#include "CFlyPCH.h"
#include "CFMaterial.h"

#include "CFEntity.h"
#include "CFWorld.h"
#include "CFRenderOption.h"
#include "CFRenderSystem.h"

namespace CFly
{
	Material* MaterialManager::createMaterial( float const* amb , float const* dif , float const* spe , float shine , float const* emi )
	{
		Material* material = new Material( amb , dif , spe , shine , emi );
		material->mMamager = this;
		EntityManager::Get().registerEntity( material );
		return material;
	}

	void MaterialManager::destoryMaterial( Material* material )
	{
		if ( !EntityManager::Get().removeEntity( material ) )
			return;

		delete material;
	}

	Texture* MaterialManager::createTexture( char const* texName, TextureType type , Color3ub const* colorKey , int mipMapLevel )
	{
		char path[256];
		if ( !mWorld->getTexturePath( path , texName ) )
			return nullptr;

		Texture* texture = nullptr;
		HRESULT hr;

		switch( type )
		{
		case CFT_TEXTURE_2D:
			{
				D3DTexture* d3dTexture;
				hr = D3DXCreateTextureFromFileEx( 
					mD3dDevice   , 
					path         ,       //file name
					D3DX_DEFAULT ,      //width
					D3DX_DEFAULT ,      //height
					(mipMapLevel)? mipMapLevel : D3DX_DEFAULT ,      //mip levels
					0 ,                 //usage
					D3DFMT_UNKNOWN,    //texture color format
					D3DPOOL_MANAGED,   //memory class
					D3DX_DEFAULT ,      //filter
					D3DX_DEFAULT ,      //mip filter
					( colorKey ) ? D3DCOLOR_ARGB( 0xff , colorKey->r , colorKey->g , colorKey->b ) : 0xff000000 ,  //color key
					NULL,              //source info
					NULL,              //pallette
					&d3dTexture
				);

				if ( hr == D3D_OK )
					texture = new Texture( d3dTexture , this );
			}
			break;
		case CFT_TEXTURE_CUBE_MAP:
			{
				D3DCubeTexture* d3dTexture;
				hr = D3DXCreateCubeTextureFromFileEx( 
					mD3dDevice , 
					path , //file name
					D3DX_DEFAULT , //length
					(mipMapLevel)?mipMapLevel : D3DX_DEFAULT ,      //mip levels
					0,            //usage
					D3DFMT_UNKNOWN,    //texture color format
					D3DPOOL_DEFAULT,   //memory class
					D3DX_DEFAULT ,      //filter
					D3DX_DEFAULT ,    //mip filter
					( colorKey ) ? D3DCOLOR_ARGB( 0xff , colorKey->r , colorKey->g , colorKey->b ) : 0xff000000 ,   //color key
					NULL,              //source info
					NULL,              //pallette
					&d3dTexture
				);

				if ( hr == D3D_OK )
					texture = new Texture( d3dTexture , this );
			}
			break;
		case CFT_TEXTURE_3D:
			{
				D3DVolumeTexture* d3dTexture;
				hr = D3DXCreateVolumeTextureFromFile( 
					mD3dDevice , path , &d3dTexture 
				);

				if ( hr == D3D_OK )
					texture = new Texture( d3dTexture , this );
			}
			break;
		default:
			return nullptr;
		}

		if ( !texture )
		{
			LogError( "Can't Load Texture File ! \n"
				      "Path = %s Error Code = %d",  
				      path , hr );
			return nullptr;
		}

		unsigned flag = ( colorKey ) ? Texture::TF_USAGE_COLOR_KEY : 0;

		texture->_setupAttribute( type , flag );
		EntityManager::Get().registerEntity( texture );
		return texture;
	}

	Texture* MaterialManager::createTexture2D( char const* texName , TextureFormat format , uint8* rawData , int w , int h , Color3ub const* colorKey , int mipMapLevel /*= 0*/ )
	{
		D3DTexture* d3dTexture;

		int pixelSize = 0;
		switch( format )
		{
		case CF_TEX_FMT_ARGB32 :
		case CF_TEX_FMT_RGB32  :
		case CF_TEX_FMT_R32F   :
			pixelSize = 4; break;
		case CF_TEX_FMT_RGB24  :
			pixelSize = 3; break;
		case CF_TEX_FMT_R5G6B5 :
			pixelSize = 2; break;
		default:
			assert(0);
		}

		//HRESULT hr = D3DXCreateTextureFromFileInMemoryEx(
		//	m_d3dDevice , rawData , w * h * pixelSize ,
		//	w , h , 
		//	(mipMapLevel)? mipMapLevel : D3DX_DEFAULT ,      //mip levels
		//	D3DUSAGE_AUTOGENMIPMAP ,                 //usage
		//	convertD3DFormat( format ) ,    //texture color format
		//	D3DPOOL_MANAGED,   //memory class
		//	D3DX_DEFAULT ,      //filter
		//	D3DX_DEFAULT ,      //mip filter
		//	( beKey ) ? D3DCOLOR_ARGB( 0xff , keyR , keyG , keyB ) : 0xff000000 ,  //color key
		//	NULL,              //source info
		//	NULL,              //pallette
		//	&d3dTexture );

		HRESULT hr = D3DXCreateTexture( 
			mD3dDevice , w , h , 
			(mipMapLevel)? mipMapLevel : D3DX_DEFAULT ,
			D3DUSAGE_AUTOGENMIPMAP ,
			D3DTypeMapping::convert( format ) ,
			D3DPOOL_MANAGED ,
			&d3dTexture );

		if ( hr != D3D_OK )
		{
			return nullptr;
		}
		

		//d3dTexture->AddDirtyRect(NULL);
		//IDirect3DSurface9	*pSurface;
		//hr = d3dTexture->GetSurfaceLevel(0, &pSurface);
		//if (hr == D3D_OK)
		//{
		//	hr = D3DXLoadSurfaceFromMemory(
		//		pSurface, 
		//		NULL,  NULL , 
		//		rawData , convertD3DFormat( format )  , w * pixelSize , 
		//		NULL,  NULL , D3DX_DEFAULT , 
		//		( beKey ) ? D3DCOLOR_ARGB( 0 , keyR , keyG , keyB ) : 0xff000000   //color key
		//	);
		//}

		D3DLOCKED_RECT lockRect;
		hr = d3dTexture->LockRect( 0 , &lockRect , NULL  , D3DLOCK_DISCARD );
		int dataPitch = pixelSize * w;

		if ( lockRect.Pitch == dataPitch )
		{
			memcpy( lockRect.pBits , rawData ,  h * lockRect.Pitch );
		}
		else
		{
			uint8* dest = static_cast< uint8* >( lockRect.pBits );
			uint8* src  = rawData;
			int delta = lockRect.Pitch - dataPitch;
			for( int i = 0 ; i < h ; ++i )
			{
				memcpy( dest , src ,  dataPitch );
				memset( dest + dataPitch , 0 , delta );
				dest += lockRect.Pitch;
				src  += dataPitch;
			}
		}

		d3dTexture->UnlockRect(0);

		if( mipMapLevel )
		{
			d3dTexture->GenerateMipSubLevels();
		}

		unsigned flag = ( colorKey ) ? Texture::TF_USAGE_COLOR_KEY : 0;

		Texture* texture = new Texture( d3dTexture , this );
		texture->_setupAttribute( CFT_TEXTURE_2D , flag );
		EntityManager::Get().registerEntity( texture );
		return texture;
	}

	Texture* MaterialManager::createRenderCubeMap( char const* texName , TextureFormat format , int len , bool haveZBuffer )
	{
		D3DFORMAT d3dFormat = D3DTypeMapping::convert( format );

		D3DCubeTexture* d3dTexture;
		HRESULT hr = mD3dDevice->CreateCubeTexture( 
				len , 1 , D3DUSAGE_RENDERTARGET , 
				d3dFormat , D3DPOOL_DEFAULT , 
				&d3dTexture , NULL );

		if ( hr != D3D_OK )
			return nullptr;

		unsigned flag = Texture::TF_USAGE_RENDER_TARGET;
		
		Texture* texture = new Texture( d3dTexture , this );
		texture->_setupAttribute( CFT_TEXTURE_CUBE_MAP , flag );

		if ( haveZBuffer )
			texture->_addDepthRenderBuffer( mWorld->_getRenderSystem() , len , len );

		return texture;
	}

	Texture* MaterialManager::createRenderTarget( char const* texName , TextureFormat format , int w , int h , bool haveZBuffer )
	{
		D3DFORMAT d3dFormat = D3DTypeMapping::convert( format );

		D3DTexture* d3dTexture = NULL;
		HRESULT hr = mD3dDevice->CreateTexture( 
			w , h , 1 , D3DUSAGE_RENDERTARGET , 
			d3dFormat , D3DPOOL_DEFAULT , 
			&d3dTexture , NULL );

		if ( hr != D3D_OK )
			return nullptr;

		unsigned flag = Texture::TF_USAGE_RENDER_TARGET;

		Texture* texture = new Texture( d3dTexture , this );
		texture->_setupAttribute( CFT_TEXTURE_2D , flag );

		if ( haveZBuffer )
			texture->_addDepthRenderBuffer( mWorld->_getRenderSystem() , w , h );

		return texture;
	}

	MaterialManager::MaterialManager( World* world ) 
		:mWorld(world)
	{
		mD3dDevice = world->getD3DDevice();
	}

	ShaderEffect* MaterialManager::createShaderEffect( char const* name )
	{
		char path[256];
		mWorld->_setUseDirBit( BIT(DIR_SHADER) );
		if ( !mWorld->getPath( path , name , ".fx") )
			return nullptr;

		LPD3DXEFFECT effect;
		LPD3DXBUFFER buffer;

		HRESULT hr = D3DXCreateEffectFromFile( 
			mD3dDevice , path , NULL , NULL , 
			D3DXSHADER_DEBUG , NULL, &effect , &buffer );

		if ( hr != D3D_OK )
		{
			char const* error = (char const*)buffer->GetBufferPointer();
			LogError( "Compile Shader file Error : \n"
				       "path = %s \n"
					   "ErrorMsg = %s ",
					   path , error  );
			return nullptr;
		}

		ShaderEffect* shader = new ShaderEffect( this );
		shader->mD3DEffect = effect;

		return shader;
	}

	void MaterialManager::destoryTexture( Texture* texture )
	{
		delete texture;
	}

	void MaterialManager::destoryShader( ShaderEffect* shader )
	{
		delete shader;
	}

	void MaterialManager::destoryRenderTarget( RenderTexture* renderTarget )
	{
		delete renderTarget;
	}

	Material::Material( float const* amb , float const* dif , float const* spe , float shine , float const* emi ) 
		:mMaxLayer(0)
		,mNumLayer( 0 )
		,mOpacity( 1.0f )
	{
		mTexLayer.reserve( 2 );
		setAmbient( amb ? Color4f(amb):Color4f(0,0,0,1) );
		setDiffuse( dif ? Color4f(dif):Color4f(0,0,0,1) );
		setSpeclar( spe ? Color4f(spe):Color4f(0,0,0,1) );
		setEmissive( emi ? Color4f(emi):Color4f(0,0,0,1) );
		setShininess( shine );
		mLightingColorUsage = 0;
	}

	void Material::resetTextureLayer( int numLayer )
	{
		mTexLayer.resize( numLayer );
		mNumLayer = numLayer;
	}

	Texture* Material::addTexture( int slot , int idxLayer , Texture* texture )
	{
		adjustTextureLayer( idxLayer );
		mTexLayer[idxLayer].setTexture( texture , slot );
		return texture;
	}

	Texture* Material::addTexture( int slot, int idxLayer, char const* texName, TextureType type /*= CFT_TEXTURE_2D */, Color3ub const* colorKey /*= nullptr */, int mipMapLevel /*= 0 */ )
	{

		Texture* texture = mMamager->createTexture( texName , type , colorKey , mipMapLevel);

		if ( !texture )
			return nullptr;

		addTexture( slot , idxLayer , texture );
		return texture;
	}

	void Material::adjustTextureLayer( int idxLayer )
	{
		if ( idxLayer >= mNumLayer )
		{
			resetTextureLayer( std::max( idxLayer + 1 , mNumLayer ) );
		}

		TextureLayer& layer = mTexLayer[ idxLayer ];
		if ( !layer.isEnable() )
		{
			layer.setTexOperator( CFT_COLOR_MAP );
			layer.setUsageTexcoord( idxLayer );
		}

		if ( idxLayer >=  mMaxLayer )
			mMaxLayer = idxLayer + 1;
	}


	int Material::addSenquenceTexture( int idxLayer, char const* texFormat , int numTex , TextureType type /*= CFT_TEXTURE_2D */, Color3ub const* colorKey , int mipMapLevel /*= 0 */ )
	{
		adjustTextureLayer( idxLayer );

		TextureLayer& texLayer = mTexLayer[ idxLayer ];

		texLayer.setupSlotNumber( numTex );

		char texName[512];
		int num = 0;
		for( int i = 0 ; i < numTex ; ++i )
		{
			sprintf( texName , texFormat , i );
			Texture* tex = mMamager->createTexture( texName , type , colorKey , mipMapLevel );

			if ( !tex )
				break;
			texLayer.setTexture( tex , i );
		}

		return num;

	}


	Texture* Material::addRenderCubeMap( int slot , int idxLayer , char const* texName , TextureFormat format , int length , bool haveZBuffer )
	{
		Texture* texture = mMamager->createRenderCubeMap( texName , format , length , haveZBuffer );

		return addTexture( slot , idxLayer , texture );
	}

	Texture* Material::addRenderTarget( int slot , int idxLayer , char const* texName , TextureFormat format , Viewport* viewport , bool haveZBuffer )
	{
		int w,h;
		viewport->getSize( &w , &h );
		Texture* texture = mMamager->createRenderTarget( texName , format , w , h , haveZBuffer );

		viewport->setRenderTarget( texture->getRenderTarget() , 0 );

		return addTexture( slot , idxLayer , texture );
	}

	void Material::destroyThis()
	{
		mMamager->destoryMaterial( this );
	}

	void Material::getColor( D3DCOLORVALUE const& color , float* rgba )
	{
		assert( rgba );
		rgba[0] = color.r ;
		rgba[1] = color.g ;
		rgba[2] = color.b ;
		rgba[3] = color.a ;
	}



	void Material::setColor( D3DCOLORVALUE& color , float* rgba )
	{
		if ( rgba )
		{
			color.r = rgba[0];
			color.g = rgba[1];
			color.b = rgba[2];
			color.a = rgba[3];
		}
		else
		{
			color.r = 0;
			color.g = 0;
			color.b = 0;
			color.a = 1.0f;
		}
	}


	ShaderEffect* Material::addShaderEffect( char const* name , char const* tech )
	{
		mShader =  mMamager->getShaderEffect( name );
		mTechniqueName = tech;
		return mShader;
	}

	ShaderEffect* Material::prepareShaderEffect()
	{
		assert( mShader );
		mShader->setTechnique( mTechniqueName.c_str() );
		return mShader;
	}

	void Material::setTexOperator( int idxLayer , TexOperator texOp )
	{
		getTextureLayer( idxLayer ).setTexOperator( texOp );
	}

	SlotTexAnimation* Material::createSlotAnimation( int idxLayer )
	{
		SlotTexAnimation* anim = new SlotTexAnimation( &getTextureLayer( idxLayer) );
		mAnimList.push_back( anim );
		return anim;
	}

	IUVTexAnimation* Material::createUVAnimation( int idxLayer , float du , float dv )
	{
		IUVTexAnimation* anim = new IUVTexAnimation( &getTextureLayer( idxLayer) , du , dv );
		mAnimList.push_back( anim );
		return anim;
	}

	void Material::updateAnimation( float dt )
	{
		for( MatAnimationList::iterator iter = mAnimList.begin();
			iter != mAnimList.end() ;++iter )
		{
			(*iter)->updateAnimation( dt );
		}
	}

	TextureLayer& Material::getTextureLayer(int idxLayer)
	{
		assert( idxLayer < mNumLayer );
		return mTexLayer[ idxLayer ];
	}

	TextureLayer const& Material::getTextureLayer(int idxLayer) const
	{
		return const_cast< Material* >( this )->getTextureLayer( idxLayer );
	}

	TextureLayer::TextureLayer() 
		:mSlot(1)
	{
		mUsageMask = TLF_USE_ALAPHA;
		mTexOp = CFT_UNKNOW_MAP;
		mFilter = CF_FILTER_LINEAR;
		mCurSlot  = 0;
		std::fill_n( mMode , 3 , CFAM_WRAP );
		mTrans.setIdentity();
	}


	Texture* TextureLayer::_setupDevice( int idxLayer , RenderSystem* renderSystem )
	{
		D3DDevice* d3dDevice = renderSystem->getD3DDevice();

		if ( !( mUsageMask & TLF_ENABLE ) )
			return nullptr;

		Texture* curTex = getCurrentTexture();

		renderSystem->setupTexture( idxLayer , curTex , mIdxCoord );

		if ( mUsageMask & TLF_USE_TRANSFORM )
		{
			d3dDevice->SetTransform( 
				(D3DTRANSFORMSTATETYPE)(D3DTS_TEXTURE0 + idxLayer) , 
				reinterpret_cast< D3DMATRIX const*>( &mTrans ) );

			d3dDevice->SetTextureStageState( idxLayer , D3DTSS_TEXTURETRANSFORMFLAGS , D3DTTFF_COUNT2 );
		}
		else
		{
			d3dDevice->SetTextureStageState( idxLayer , D3DTSS_TEXTURETRANSFORMFLAGS , D3DTTFF_DISABLE );
		}

		DWORD d3dfilter = D3DTypeMapping::convert( mFilter );
		d3dDevice->SetSamplerState( idxLayer , D3DSAMP_MAGFILTER , d3dfilter );
		d3dDevice->SetSamplerState( idxLayer , D3DSAMP_MINFILTER , d3dfilter );

		if ( curTex->haveColorKey() )
			d3dDevice->SetSamplerState( idxLayer , D3DSAMP_MIPFILTER , D3DTEXF_NONE );
		else
			d3dDevice->SetSamplerState( idxLayer , D3DSAMP_MIPFILTER , D3DTEXF_LINEAR );

		renderSystem->setupTexAdressMode( idxLayer , curTex->getType() , mMode );

#if 0
		if ( modeD3D == D3DTADDRESS_WRAP )
		{
			//texture warping
			static const D3DRENDERSTATETYPE wrapState[] = { 
				D3DRS_WRAP0 , D3DRS_WRAP1 , D3DRS_WRAP2 , D3DRS_WRAP3 ,
				D3DRS_WRAP4 , D3DRS_WRAP5 , D3DRS_WRAP6 , D3DRS_WRAP7 };

			switch( curTex->getType() )
			{
			case CFT_TEXTURE_1D:
				d3dDevice->SetRenderState( wrapState[ idxLayer ] , D3DWRAP_U );
				break;
			case CFT_TEXTURE_2D:
				d3dDevice->SetRenderState( wrapState[ idxLayer ] , D3DWRAP_U | D3DWRAP_V );
				break;
			case CFT_TEXTURE_CUBE_MAP:
			case CFT_TEXTURE_3D:
				d3dDevice->SetRenderState( wrapState[ idxLayer ] , D3DWRAP_U | D3DWRAP_V | D3DWRAP_W );
				break;
			}
		}
#endif

		d3dDevice->SetTextureStageState( idxLayer, D3DTSS_COLOROP, colorOp );
		d3dDevice->SetTextureStageState( idxLayer, D3DTSS_COLORARG1, colorArg1 );
		d3dDevice->SetTextureStageState( idxLayer, D3DTSS_COLORARG2, colorArg2 );

		if ( mUsageMask & TLF_USE_ALAPHA )
		{
			d3dDevice->SetTextureStageState( idxLayer, D3DTSS_ALPHAOP, alphaOp );
			d3dDevice->SetTextureStageState( idxLayer, D3DTSS_ALPHAARG1 , alphaArg1 );
			d3dDevice->SetTextureStageState( idxLayer, D3DTSS_ALPHAARG2 , alphaArg2 );
		}
		else
		{
			d3dDevice->SetTextureStageState( idxLayer, D3DTSS_ALPHAOP, D3DTOP_SELECTARG1 );
			d3dDevice->SetTextureStageState( idxLayer, D3DTSS_ALPHAARG1, D3DTA_CURRENT );
		}

		return curTex;
	}

	void TextureLayer::setTexOperator( TexOperator Op )
	{
		mTexOp =  Op;

		switch( Op )
		{
		case CFT_COLOR_MAP:
			colorOp   = D3DTOP_MODULATE;
			colorArg1 = D3DTA_TEXTURE;
			colorArg2 = D3DTA_DIFFUSE;
			alphaOp   = D3DTOP_MODULATE;
			alphaArg1 = D3DTA_TEXTURE;
			alphaArg2 = D3DTA_DIFFUSE;
			break;
		case CFT_LIGHT_MAP:
			colorOp   = D3DTOP_MODULATE;
			colorArg1 = D3DTA_TEXTURE;
			colorArg2 = D3DTA_CURRENT;
			alphaOp   = D3DTOP_SELECTARG1;
			alphaArg1 = D3DTA_CURRENT;
			alphaArg2 = D3DTA_TEXTURE;
			break;
		}
	}

	void TextureLayer::setUseAlapha( bool beAlapha )
	{
		if ( beAlapha )
			mUsageMask |= TLF_USE_ALAPHA;
		else
			mUsageMask &= ~TLF_USE_ALAPHA;
	}

	void TextureLayer::setTexture( Texture* tex , int slot )
	{
		if ( mSlot.size() <= slot )
			setupSlotNumber( slot + 1 );

		mSlot[ slot ] = tex;

		if ( mSlot[ mCurSlot ] )
			addUsageFlag( TLF_ENABLE );
		else
			removeUsageFlag( TLF_ENABLE );
	}

	void TextureLayer::setAddressMode( int idx , TexAddressMode mode )
	{
		mMode[idx] = mode;
	}

	void TextureLayer::rotate( float angle , TransOp op )
	{
		float c = Math::Cos( angle );
		float s = Math::Sin( angle );

		//           c -s  0
		//  mtx = [  s  c  0 ]
		//           0  0  1
		switch( op )
		{
		case CFTO_REPLACE:
			mTrans.setIdentity();
			mTrans[0] = c; mTrans[1] = -s;
			mTrans[4] = s; mTrans[5] = c;
			break;

		case CFTO_GLOBAL:
			mTrans[0] = c * mTrans[0] + s * mTrans[1];
			mTrans[1] = -s * mTrans[0] + c * mTrans[1];

			mTrans[4] = c * mTrans[4] + s * mTrans[5];
			mTrans[5] = -s * mTrans[4] + c * mTrans[5];

			mTrans[8] = c * mTrans[8] + s * mTrans[9];
			mTrans[9] = -s * mTrans[8] + c * mTrans[9];
			break;
		case CFTO_LOCAL:
			mTrans[0] = c * mTrans[0] - s * mTrans[4];
			mTrans[1] = c * mTrans[1] - s * mTrans[5];

			mTrans[4] = c * mTrans[0] + s * mTrans[4];
			mTrans[5] = c * mTrans[1] + s * mTrans[5];
			break;
		}
	}

	void TextureLayer::translate( float u , float v , TransOp op )
	{
		//           1  0  0
		//  mtx = [  0  1  0 ]
		//           u  v  1
		switch( op )
		{
		case CFTO_REPLACE:
			mTrans.setIdentity();
			mTrans[8] = u;
			mTrans[9] = v;
			break;
		case CFTO_GLOBAL:
			mTrans[8] += u;
			mTrans[9] += v;
			break;
		case CFTO_LOCAL:
			mTrans[8] += u * mTrans[0] + v * mTrans[4];
			mTrans[9] += u * mTrans[1] + v * mTrans[5];
			break;
		}
	}


	void TextureLayer::_setD3DTexOperation( DWORD cOp , DWORD cArg1 , DWORD cArg2 , DWORD aOp , DWORD aArg1 , DWORD aArg2 )
	{
		mTexOp = CFT_USER_DEFINE_MAP;

		colorOp = cOp;
		colorArg1 = cArg1;
		colorArg2 = cArg2;

		alphaOp = aOp;
		alphaArg1 = aArg1;
		alphaArg2 = aArg2;
	}

	Texture* TextureLayer::getCurrentTexture()
	{
		if ( mSlot.empty() )
			return nullptr;
		return mSlot[ mCurSlot ];
	}

	bool ShaderEffect::PassIterator::hasMoreElement()
	{
		if ( curPass )
			shader->mD3DEffect->EndPass();

		if (curPass >= nPass )
		{
			shader->mD3DEffect->End();
			return false;
		}

		shader->mD3DEffect->BeginPass( curPass );
		shader->mD3DEffect->CommitChanges();
		++curPass;
		return true;
	}

	ShaderEffect::PassIterator::PassIterator( ShaderEffect* shader ) :shader(shader)
	{
		shader->mD3DEffect->Begin( &nPass , 0 );
		curPass = 0;
	}


	void ShaderEffect::destroyThis()
	{
		manager->destoryShader( this );
	}

	ShaderParamGroup& ShaderEffect::getGroup( ParamContent content , char const* name )
	{
		ShaderParamGroup* pg = nullptr;

		switch ( content )
		{
		case SP_WORLD :
		case SP_VIEW  :
		case SP_PROJ  :
		case SP_WORLD_INV :
		case SP_VIEW_PROJ :
		case SP_WORLD_VIEW:
		case SP_WVP :
		case SP_AMBIENT_LIGHT:
			pg = &gpuParamContent["Global"];
			if ( pg->group == PG_DEFAULT_DATA )
				pg->group = PG_GLOBAL;
			break;

		case SP_CAMERA_POS :
		case SP_CAMERA_VIEW_PROJ :
		case SP_CAMERA_VIEW:
		case SP_CAMERA_WORLD  :
		case SP_CAMERA_Z_FAR :
		case SP_CAMERA_Z_NEAR:
			if ( name == NULL )
				name = "RenderCamera";
			pg = &gpuParamContent[ name ];
			if ( pg->group == PG_DEFAULT_DATA )
				pg->group = PG_CAMERA;
			break;

		case SP_LIGHT_POS :
		case SP_LIGHT_DIR :
		case SP_LIGHT_DIFFUSE :
		case SP_LIGHT_RANGE :
		case SP_LIGHT_SPOT :
		case SP_LIGHT_ATTENUATION:
			pg = &gpuParamContent[ name ];
			if ( pg->group == PG_DEFAULT_DATA )
				pg->group = PG_LIGHT;
			break;

		case SP_MATERIAL_AMBIENT :
		case SP_MATERIAL_DIFFUSE :
		case SP_MATERIAL_SPECULAR:
		case SP_MATERIAL_EMISSIVE :
		case SP_MATERIAL_SHINENESS:
		case SP_MATERIAL_TEXLAYER0 :
		case SP_MATERIAL_TEXLAYER1 :
		case SP_MATERIAL_TEXLAYER2 :
		case SP_MATERIAL_TEXLAYER3 :
		case SP_MATERIAL_TEXLAYER4 :
		case SP_MATERIAL_TEXLAYER5 :
		case SP_MATERIAL_TEXLAYER6 :
		case SP_MATERIAL_TEXLAYER7 :
			pg = &gpuParamContent[ "material" ];
			if ( pg->group == PG_DEFAULT_DATA )
				pg->group = PG_MATERIAL;
			break;

		case SP_MATRIX :
		case SP_FLOAT :
		case SP_VECTOR :
			pg = &gpuParamContent[ "OtherParam" ];
			break;
		default:
			assert( 0 );
		}

		return *pg;
	}

	void ShaderEffect::addParam( ParamContent content , char const* varName )
	{
		ShaderParamGroup& group = getGroup( content , NULL );

		ShaderParam param;
		param.content = content;
		param.varName = varName;
		group.paramList.push_back( param );
	}

	void ShaderEffect::addParam( ParamContent content , char const* groupName , char const* varName )
	{
		ShaderParamGroup& group = getGroup( content , groupName );

		ShaderParam param;
		param.content = content;
		param.varName = varName;
		group.paramList.push_back( param );
	}

	void ShaderEffect::loadParam( ShaderParamSet* autoParamContent )
	{
		for( GpuParamContent::iterator iter( gpuParamContent.begin() ) , 
			                           itEnd( gpuParamContent.end() ); 
			 iter != itEnd ; ++iter )
		{
			autoParamContent->setupShaderParam( this , iter->second , iter->first.c_str() );
		}
	}

	Texture::Texture( D3DTexture* d3dTexture , MaterialManager* manager ) 
		:manager(manager)
		,mTex2D(d3dTexture)
		,mType( CFT_TEXTURE_2D )
	{

	}

	Texture::Texture( D3DCubeTexture* d3dTexture , MaterialManager* manager ) 
		:manager(manager)
		,mTexCubeMap(d3dTexture)
		,mType( CFT_TEXTURE_CUBE_MAP )
	{

	}

	Texture::Texture( D3DVolumeTexture* d3dTexture , MaterialManager* manager )
		:manager(manager)
		,mTexVolume(d3dTexture)
		,mType( CFT_TEXTURE_3D )
	{

	}


	Texture::~Texture()
	{
		mTex2D->Release();
	}

	void Texture::destroyThis()
	{
		manager->destoryTexture( this );
	}

	D3DSurface* Texture::_getCubeMapSurface( UINT level , UINT face )
	{
		assert( mType == CFT_TEXTURE_CUBE_MAP );
		IDirect3DSurface9* surface;
		mTexCubeMap->GetCubeMapSurface( (D3DCUBEMAP_FACES)face , level , &surface );
		surface->Release();
		return surface;
	}

	D3DSurface* Texture::_getSurface( UINT level )
	{
		assert( mType == CFT_TEXTURE_2D );
		IDirect3DSurface9* surface;
		mTex2D->GetSurfaceLevel( level , &surface );
		surface->Release();
		return surface;
	}

	void Texture::_setupAttribute( TextureType type , unsigned flag )
	{
		mType = type;
		mFlag = flag;

		if ( mFlag & TF_USAGE_RENDER_TARGET )
		{
			switch( getType() )
			{
			case CFT_TEXTURE_CUBE_MAP:
				for( UINT face = 0 ; face < 6 ; ++face )
				{
					mRenderTargetVec.push_back( 
						new RenderTexture( this , face ) );
				}
				break;
			case CFT_TEXTURE_2D:
				mRenderTargetVec.push_back( 
					new RenderTexture( this ) );
				break;
			default:
				assert(0);
			}
		}
	}

	D3DDevice* Texture::_getD3DDevice()
	{
		D3DDevice* d3dDevice = nullptr;
		switch( mType )
		{
		case CFT_TEXTURE_1D:
		case CFT_TEXTURE_2D:
			mTex2D->GetDevice( &d3dDevice );
			break;
		case CFT_TEXTURE_3D:
			mTexVolume->GetDevice( &d3dDevice );
			break;
		case CFT_TEXTURE_CUBE_MAP:
			mTexCubeMap->GetDevice( &d3dDevice );
			break;
		}
		return d3dDevice;
	}

	void Texture::_addDepthRenderBuffer( RenderSystem* renderSystem  , int w , int h )
	{
		for( int i = 0 ; i < mRenderTargetVec.size() ; ++i )
			mRenderTargetVec[i]->_addDepthBuffer( renderSystem , w , h );
	}

	void Texture::getSize( unsigned& w , unsigned& h )
	{
		D3DSURFACE_DESC desc;
		HRESULT hr = getD3DTexture()->GetLevelDesc( 0 , &desc );

		if ( hr == S_OK )
		{
			w = desc.Width;
			h = desc.Height;
		}
		else
		{
			w = h = 0;
		}
	}

	RenderTexture::RenderTexture( Texture* texture ) 
		:RenderTarget( texture->_getSurface(0) )
	{
		assert( texture->getType() == CFT_TEXTURE_2D );
	}

	RenderTexture::RenderTexture( Texture* texture , UINT face ) 
		:RenderTarget( texture->_getCubeMapSurface( 0 , face  ) )
	{
		assert( texture->getType() == CFT_TEXTURE_CUBE_MAP );
	}

	void RenderTexture::_addDepthBuffer( RenderSystem* renderSystem , int w , int h )
	{
		D3DDevice* d3dDevice = renderSystem->getD3DDevice();

		d3dDevice->CreateDepthStencilSurface( 
			w , h ,
			renderSystem->getDepthStencilFormat(),
			D3DMULTISAMPLE_NONE ,
			0,
			TRUE,
			&mDepthSurface,
			NULL );
	}


	SlotTexAnimation::SlotTexAnimation( TextureLayer* texlayer )
		:TexLayerAnimation( texlayer )
		,mFPS( 1.0f )
		,mFramePos( 0.0f )
		,mNumTex(0)
	{
		mNumTex = texlayer->getSlotNum();
	}

	void SlotTexAnimation::updateAnimation( float dt )
	{
		mFramePos += dt * mFPS;

		float length = mNumTex - 1;
		if ( mIsLoop )
		{
			mFramePos = Math::Fmod( mFramePos , length );
			if ( mFramePos < 0.0f )
				mFramePos += length;
		}
		else
		{
			if ( mFramePos > length )
				mFramePos = length;
			if ( mFramePos < 0.0f )
				mFramePos = 0.0f;
		}
		mTexLayer->setCurrentSlot( (int)mFramePos );
	}


	void IUVTexAnimation::updateAnimation( float dt )
	{
		mTexLayer->translate( mDeltaU * dt , mDeltaV * dt , CFTO_GLOBAL );
	}

}//namespace CFly