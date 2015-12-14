#include "CFlyPCH.h"
#include "CFScene.h"

#include "CFRenderSystem.h"

#include "CFWorld.h"
#include "CFObject.h"
#include "CFCamera.h"
#include "CFViewport.h"
#include "CFActor.h"
#include "CFLight.h"
#include "CFSprite.h"
#include "CFSceneNode.h"
#include "CFMaterial.h"

#include "CFRenderOption.h"
#include "CFShaderParamSet.h"
#include "CFMeshBuilder.h"
#include "CFPluginManager.h"

namespace CFly
{
	bool  g_useObjFrushumClip = true;

	Scene::Scene( World* world , int numGroup ) 
		:mWorld( world )
		,mNumGroup( numGroup )
		,mSuppressRenderOptionBits(0)
		,mReplaceMat( nullptr )
		,mPrevRenderTarget( nullptr )
	{
		mRenderListener = nullptr;

		for( int i = 0 ; i < 8 ; ++i )
			mUseLight[i] = nullptr;
		mNumLightUse = 0;


		mRoot = new SceneNode( this );
		mRoot->_setupRoot();

		mShaderParamSet = new ShaderParamSet( this );

		mRender2DCam = new Camera( this );
		mRender2DCam->setParent( mRoot );

		mRender2DCam->setProjectType( CFPT_ORTHOGONAL );
		mRender2DCam->scale( Vector3(1,1,-1) , CFTO_REPLACE );

		mRender2DCam->translate( Vector3(0,0,10000) , CFTO_LOCAL );
		mRender2DCam->rotate( CF_AXIS_Y , Math::Deg2Rad( 180 ) , CFTO_LOCAL );
		mRender2DCam->setNear( 10000 );
		mRender2DCam->setFar( -10000 );

		RenderWindow* window = getWorld()->getRenderWindow();
		mRender2DCam->setScreenRange( 0 , window->getWidth() - 1 , window->getHeight() -1 , 0 );
	}


	Scene::~Scene()
	{
		delete mRender2DCam;
		delete mShaderParamSet;
	}

	bool Scene::checkRenderGroup( SceneNode* node )
	{
		switch ( mRenderGroup )
		{
		case RG_OPTICY_GROUP:
			if ( node->isTranslucent() )
			{
				mTransNodeList.push_back( node );
				return false;
			}
			return true;
		case RG_TRANS_GROUP:
			return node->isTranslucent();
		}

		return true;
	}

	D3DDevice* Scene::getD3DDevice()
	{
		return mWorld->getD3DDevice();
	}

	template< class NODE >
	NODE* Scene::cloneNode( NODE* ptr , SceneNode* parent )
	{
		NODE* node = new NODE(*ptr);
		EntityManger::getInstance().registerEntity( node );
		_linkSceneNode( node , parent );
		return node;
	}

	Object* Scene::_cloneObject( Object* obj , SceneNode* parent )
	{
		return cloneNode( obj , parent );
	}

	Actor* Scene::_cloneActor( Actor* actor , SceneNode* parent )
	{
		return cloneNode( actor , parent );
	}


	Object* Scene::createObject( SceneNode* parent )
	{
		Object* node = new Object( this );

		_linkSceneNode( node , parent );
		EntityManger::getInstance().registerEntity( node );
		return node;
	}

	void Scene::_destroyObject( Object* obj )
	{
		destroyNodeImpl( obj );
	}

	Actor* Scene::createActor( SceneNode* parent )
	{
		Actor* node = new Actor( this );

		_linkSceneNode( node , parent );
		EntityManger::getInstance().registerEntity( node );
		return node;
	}

	void Scene::_destroyActor( Actor* actor )
	{
		destroyNodeImpl( actor );
	}

	Sprite* Scene::createSprite( SceneNode* parent )
	{
		Sprite* node = new Sprite( this );

		_linkSceneNode( node , parent );
		EntityManger::getInstance().registerEntity( node );
		return node;
	}

	void Scene::_destroySprite( Sprite* spr )
	{
		destroyNodeImpl( spr );
	}

	BillBoard* Scene::createBillBoard( SceneNode* parent /*= nullptr */ )
	{
		BillBoard* node = new BillBoard( this );

		_linkSceneNode( node , parent );
		EntityManger::getInstance().registerEntity( node );
		return node;
	}

	Camera* Scene::createCamera( SceneNode* parent )
	{
		Camera* node = new Camera( this );

		_linkSceneNode( node , parent );
		EntityManger::getInstance().registerEntity( node );
		return node;
	}

	void Scene::_destroyCamera( Camera* cam )
	{
		destroyNodeImpl( cam );
	}

	Light* Scene::createLight( SceneNode* parent )
	{
		Light* node = new Light( this );

		_linkSceneNode( node , parent );
		EntityManger::getInstance().registerEntity( node );

		mLightList.push_back( node );

		//update light list
		node->updateWorldTransform( false );
		return node;
	}

	void Scene::_destroyLight(Light* light)
	{
		mLightList.erase( std::find( mLightList.begin() , mLightList.end() , light ) );
		destroyNodeImpl( light );
	}

	Object* Scene::createSkyBox( char const* name , float size , bool isCubeMap )
	{
		struct SkyBoxInfo
		{
			Vector3 middle;
			Vector3 up;
			Vector3 right;
			char const* addName;
		};

		size = 0.5f * size;

		SkyBoxInfo skyInfo[] =
		{
			{ Vector3( 0,0,-1 ) , Vector3( 0,1,0 ) , Vector3( 1,0,0 ),"_FR"  } ,
			{ Vector3( 0,0, 1 )  , Vector3(0,1,0 ) , Vector3(-1,0,0 ),"_BK" } ,
			{ Vector3( -1,0,0 )  , Vector3( 0,1,0 ) , Vector3( 0,0,-1 ),"_LF" } ,
			{ Vector3(  1,0,0 ) , Vector3( 0,1,0 ), Vector3( 0,0,1),"_RT" } ,
			{ Vector3( 0,1,0 ) , Vector3( 0,0,1 ) , Vector3( 1,0,0 ),"_UP" } ,
			{ Vector3( 0,-1,0 )  , Vector3( 0,0,-1 ) , Vector3( 1,0,0 ),"_DN"  } ,
		};

		
		Object* obj = new SkyBox( this );
		//IObject* obj = new IObject( this );

		MeshBuilder builder( CFVT_XYZ | CFVF_TEX1( ( isCubeMap ) ? 3 : 2 ) );

		builder.reserveVexterBuffer( 24 );
		builder.reserveIndexBuffer( 3 * 12 );


		Material* mat;
		if ( isCubeMap )
		{
			mat = getWorld()->createMaterial( Color4f(1,1,1) );
			mat->addTexture( 0 , 0 , name , CFT_TEXTURE_CUBE_MAP );

			int idxVtx = 0;
			for( int i = 0 ; i  < 6 ; ++i )
			{
				SkyBoxInfo& info = skyInfo[i];

				//builder.setNormal( -info.middle );

				Vector3& middle = info.middle;
				Vector3& up = info.up;
				Vector3& right = info.right;

				Vector3 pos;
				pos = middle - up - right;
				builder.setPosition( size * pos );
				builder.setTexCoord( 0 , pos.mul( Vector3(1,1,1) ) );
				builder.addVertex();

				pos = middle - up + right;
				builder.setPosition( size * pos );
				builder.setTexCoord( 0 , pos.mul( Vector3(1,1,1) ) );
				builder.addVertex();

				pos = middle + up + right;
				builder.setPosition( size * pos );
				builder.setTexCoord( 0 , pos.mul( Vector3(1,1,1) ) );
				builder.addVertex();

				pos = middle + up - right;
				builder.setPosition( size * pos );
				builder.setTexCoord( 0 , pos.mul( Vector3(1,1,1) ) );
				builder.addVertex();

				builder.addQuad( idxVtx + 0 , idxVtx + 1 , idxVtx + 2 , idxVtx + 3 );

				idxVtx += 4;
			}

			builder.createIndexTrangle( obj , mat );

		}
		else
		{
			int idxVtx = 0;

			for( int i = 0 ; i  < 6 ; ++i )
			{
				SkyBoxInfo& info = skyInfo[i];

				//builder.setNormal( -info.middle );

				Vector3& middle = info.middle;
				Vector3& up = info.up;
				Vector3& right = info.right;

				Vector3 pos;
				pos = middle - up - right;
				builder.setPosition( size * pos );
				builder.setTexCoord( 0 , 0 , 1 );
				builder.addVertex();

				pos = middle - up + right;
				builder.setPosition( size * pos );
				builder.setTexCoord( 0 , 1 , 1 );
				builder.addVertex();

				pos = middle + up + right;
				builder.setPosition( size * pos );
				builder.setTexCoord( 0 , 1 , 0 );
				builder.addVertex();

				pos = middle + up - right;
				builder.setPosition( size * pos );
				builder.setTexCoord( 0 , 0 , 0 );
				builder.addVertex();

				builder.addQuad( idxVtx + 0 , idxVtx + 1 , idxVtx + 2 , idxVtx + 3 );

				char texName[128];
				strcpy( texName , name );
				strcat( texName , info.addName );

				mat = getWorld()->createMaterial( Color4f(1,1,1) );
				mat->addTexture( 0 , 0 , texName );

				builder.createIndexTrangle( obj , mat );
				builder.resetBuffer();
				idxVtx = 0;

			}
		}

		_linkSceneNode( obj , nullptr );
		EntityManger::getInstance().registerEntity( obj );

		return obj;
	}


	void Scene::destroyNodeImpl( SceneNode* node )
	{
		node->_prevDestroy();

		if ( !EntityManger::getInstance().removeEntity( node ) )
			return;

		if ( node->getParent() != mRoot )
			node->getParent()->onRemoveChild( node );

		node->unlinkChildren( true );
		node->setParent( nullptr );
		delete node;
	}

	void Scene::_unlinkSceneNode( SceneNode* node )
	{
		node->unlinkChildren( true );
		unregisterName( node );
		removeNode( node );
	}

	void Scene::_linkSceneNode( SceneNode* node , SceneNode* parent )
	{
		if ( !parent )
			parent = mRoot;
		assert( parent->getScene() == this );
		node->setParent( parent );
	}

	void Scene::render( Camera* camera , Viewport* viewport , unsigned flag )
	{
		D3DDevice* d3dDevice = getD3DDevice();

		{
			CF_PROFILE("prepareRender");
			if ( !prepareRender( camera , viewport , flag ) )
				return;
		}

		if ( mRenderListener )
			mRenderListener->onRenderNodeStart();
		{
			CF_PROFILE("renderGroup");
			mTransNodeList.clear();
			mRenderGroup = RG_OPTICY_GROUP;
			mRoot->renderChildren();
		//}
		//{
		//	CF_PROFILE2("renderTransGroup" , PROF_DISABLE_CHILDREN );
			mRenderGroup = RG_TRANS_GROUP;
			for( NodeList::iterator iter = mTransNodeList.begin();
				iter != mTransNodeList.end() ; ++iter )
			{
				SceneNode* node = static_cast<SceneNode*>( *iter );
				node->render( node->_getWorldTransformInternal( true ) );
			}
		}

		if ( mRenderListener )
			mRenderListener->onRenderNodeEnd();

		{
			CF_PROFILE("finishRender");
			finishRender();
		}
	}

	void Scene::render2D( Viewport* viewport , unsigned flag /*= CFRF_DEFULT */ )
	{
		mRenderSprite = true;
		render( mRender2DCam , viewport , flag );
		mRenderSprite = false;
	}

	void Scene::renderObject( Object* obj , Camera* camera , Viewport* viewport , unsigned flag /*= CFRF_DEFULT */ )
	{
		renderNode( obj , obj->_getWorldTransformInternal( false ) , camera , viewport , flag );
	}

	void Scene::renderNode( SceneNode* node , Matrix4 const& worldTrans , Camera* camera , Viewport* viewport , unsigned flag /*= CFRF_DEFULT */ )
	{
		D3DDevice* d3dDevice = getD3DDevice();

		if ( !prepareRender( camera , viewport , flag ) )
			return;

		if ( mRenderListener )
			mRenderListener->onRenderNodeStart();

		mRenderGroup = RG_ALL_GROUP;
		node->render( worldTrans );

		if ( mRenderListener )
			mRenderListener->onRenderNodeEnd();

		finishRender();
	}

	bool Scene::prepareRender(  Camera* camera , Viewport* viewport , unsigned flag )
	{
		mSuppressRenderOptionBits = 0;


		RenderSystem* renderSystem = _getRenderSystem();
		D3DDevice* d3dDevice = renderSystem->getD3DDevice();

		HRESULT hr = d3dDevice->BeginScene();
		if( FAILED( hr ) )
		{
			switch( hr )
			{
			case D3DERR_INVALIDCALL:
				d3dDevice->EndScene();
			}
			return false;
		}


		mRenderCamera   = camera;
		mShaderParamSet->setCurCamera( camera );
		mRenderViewport = viewport;

		if ( flag & CFRF_SAVE_RENDER_TARGET )
		{
			d3dDevice->GetRenderTarget( 
				mRenderViewport->getRenderTargetIndex() , 
				&mPrevRenderTarget );

			if ( mPrevRenderTarget )
				mPrevRenderTarget->AddRef();
		}


		DWORD clearFlag = 0;

		renderSystem->setupViewport( *mRenderViewport );
		renderSystem->setRenderTarget( 
			*mRenderViewport->getRenderTarget() , 
			mRenderViewport->getRenderTargetIndex() );

		if ( flag & CFRF_CLEAR_BG_COLOR )
			clearFlag |= D3DCLEAR_TARGET;
		if ( flag & CFRF_CLEAR_Z )
			clearFlag |= D3DCLEAR_ZBUFFER;
		if ( flag & CFRF_CLEAR_STENCIL )
			clearFlag |= D3DCLEAR_STENCIL;

		if ( clearFlag )
			d3dDevice->Clear( 0 , NULL , clearFlag , mRenderViewport->getBackgroundColor().toARGB() , 1.0f , 0 );

		if ( flag & CFRF_LIGHTING_DISABLE )
		{
			renderSystem->setLighting( false );
			for( int i = 0 ; i < mNumLightUse ; ++i )
				renderSystem->setLight( i , false );

			mSuppressRenderOptionBits |= CFRO_LIGHTING;
		}
		else
		{
			for( int i = 0 ; i < mNumLightUse; ++i )
			{
				if ( mUseLight[i] )
				{
					renderSystem->setLight( i , true );
					renderSystem->setupLight( i , *mUseLight[i] );
				}
			}
		}

		_setupDefultRenderOption( CFRO_ALL_OPTION_BIT & (~mSuppressRenderOptionBits ) );

		renderSystem->setAmbientColor( mAmbientColor );
		d3dDevice->SetRenderState( D3DRS_AMBIENT , mAmbientColor.toARGB() );

		renderSystem->setProjectMatrix( mRenderCamera->getProjectionMartix() );
		renderSystem->setViewMatrix( mRenderCamera->getViewMatrix() );

		gUseFrustumClip =  g_useObjFrushumClip && !( flag & CFRF_FRUSTUM_CLIP_DISABLE );

		return true;
	}

	void Scene::finishRender()
	{
		D3DDevice* d3dDevice = getD3DDevice();
		d3dDevice->EndScene();

		if ( mPrevRenderTarget )
		{
			d3dDevice->SetRenderTarget(
				mRenderViewport->getRenderTargetIndex() , 
				mPrevRenderTarget );
			mPrevRenderTarget->Release();
			mPrevRenderTarget = nullptr;
		}

		mRenderCamera   = NULL;
		mRenderViewport = NULL;
	}

	/*void Scene::_updateLight( Light* light )
	{

		Vector3 pos = light->getWorldPosition();
		pos -= lightProbeBS.center;

		if ( pos.length2() < lightProbeBS.radius * lightProbeBS.radius )
		{
			if ( light->mIdxUse != CF_FAIL_ID )
				return;

			if ( mNumLightUse >= 8 )
				return;

			mUseLight[ mNumLightUse ] = light;
			getD3DDevice()->LightEnable( light->mIdxUse , TRUE );

			++mNumLightUse;

		}
		else if ( light->mIdxUse != CF_FAIL_ID )
		{
			int idxLastLight = mNumLightUse - 1;
			if ( light->mIdxUse != idxLastLight )
			{
				Light* cLight = mUseLight[ idxLastLight ];
				mUseLight[ light->mIdxUse ] = cLight;

				mUseLight[ idxLastLight  ] = nullptr;
				getD3DDevice()->LightEnable( idxLastLight , FALSE );
			}
			else
			{
				mUseLight[ light->mIdxUse ] = nullptr;
				getD3DDevice()->LightEnable( light->mIdxUse , FALSE );
			}

			light->mIdxUse = CF_FAIL_ID;
			--mNumLightUse;

			return;
		}
	}*/


	void Scene::_setupDefultRenderOption( unsigned renderOptionBit )
	{
		renderOptionBit &= ~mSuppressRenderOptionBits;
		_getRenderSystem()->setupDefaultOption( renderOptionBit );
	}


	void Scene::_setupRenderOption( DWORD* renderOption , unsigned renderOptionBit )
	{
		renderOptionBit &= ~mSuppressRenderOptionBits;
		_getRenderSystem()->setupOption( renderOption , renderOptionBit );
	}


	Light* Scene::findLightByName( char const* name )
	{
		SceneNode* node = static_cast<SceneNode*>( findNodeByName( name ) );
		return entity_cast< Light >( node );
	}

	Camera* Scene::findCameraByName( char const* name )
	{
		SceneNode* node = static_cast<SceneNode*>( findNodeByName( name ) );
		return entity_cast< Camera >( node );
	}

	void Scene::setAmbientLight( float* rgb )
	{
		mAmbientColor.r = rgb[0];
		mAmbientColor.g = rgb[1];
		mAmbientColor.b = rgb[2];
		mAmbientColor.a = 1.0f;
	}

	void Scene::_renderMesh( MeshBase* mesh , Material* mat , RenderMode mode , float opacity , unsigned& restOptionBit )
	{
		RenderSystem* renderSystem = _getRenderSystem();
		D3DDevice* d3dDevice = getD3DDevice();

		Material* material = ( mReplaceMat ) ? mReplaceMat : mat;


		{
			CF_PROFILE("setupMaterial");
			_setupMaterial( material , mode , opacity , restOptionBit );
		}

		{
			CF_PROFILE("renderPrimitive");

			if ( material && material->useShader() )
			{
				mesh->_setupStream( renderSystem , true );

				ShaderEffect* shader = material->prepareShaderEffect();

				mShaderParamSet->setCurMaterial( material );
				shader->loadParam( mShaderParamSet );
				ShaderEffect::PassIterator iter( shader );

				while( iter.hasMoreElement() )
				{
					mesh->_renderPrimitive( renderSystem );
				}
			}
			else
			{
				mesh->_setupStream( renderSystem , false );
				mesh->_renderPrimitive( renderSystem );
			}
		}

	}

	SceneNode* Scene::findSceneNode( unsigned id )
	{
		return (SceneNode*) EntityManger::getInstance().extractEntityBits( id ,
			BIT(ET_ACTOR) | BIT(ET_OBJECT) | BIT(ET_CAMERA) | BIT(ET_LIGHT) );
	}

	void Scene::setSpriteWorldSize( int w , int h , int depth )
	{
		mRender2DCam->setFar( depth );
		mRender2DCam->setScreenRange( 0 , w , h , 0 );
	}

	void Scene::visitSceneNode( SceneNodeVisitor& visitor )
	{
		for ( NodeNameMap::iterator iter = mNameMap.begin();
			  iter != mNameMap.end() ; ++iter )
		{
			if ( !visitor.visit( static_cast< SceneNode*>( iter->second ) ) )
				return;
		}
	}

	RenderSystem* Scene::_getRenderSystem()
	{
		return mWorld->_getRenderSystem();
	}

	void Scene::release()
	{
		getWorld()->_destroyScene( this );
	}

	void Scene::destroyAllSceneNode()
	{

	}

	Actor* Scene::createActorFromFile( char const* fileName , char const* loaderName )
	{
		Actor* actor = createActor( nullptr );

		if ( !PluginManager::getInstance().load( getWorld() , actor , fileName , loaderName ) )
		{
			_destroyActor( actor );
			return nullptr;
		}
		return actor;
	}

	bool Scene::load( char const* fileName , char const* loaderName , ILoadListener* listener /*= NULL */ )
	{
		return PluginManager::getInstance().load( getWorld() , this , fileName , loaderName , listener );
	}

	void Scene::_setupMaterial( Material* material , RenderMode mode , float opacity , unsigned& restOptionBit )
	{
		RenderSystem* renderSystem = _getRenderSystem();
		D3DDevice* d3dDevice = renderSystem->getD3DDevice();

		unsigned useStage = 0;

		if ( material )
		{
			bool useShader = material->useShader();
			bool useTexLayer = ( mode != CFRM_NO_TEXTURE ) && ( !useShader );

			if ( !useShader )
			{
				renderSystem->setMaterialColor( 
					material->getAmbient() , material->getDiffuse() , material->getSpecular() ,
					material->getEmissive() , material->getShininess() );
			}


			if ( useTexLayer )
			{
				bool useColorKey = false;
				for( int i = 0 ; i < material->mMaxLayer ; ++i )
				{
					TextureLayer& layer = material->getTextureLayer( i );

					if ( Texture* tex = layer._setupDevice( useStage , renderSystem ) )
					{
						++useStage;
						useColorKey |= tex->haveColorKey();
					}
				}

				if ( useColorKey )
				{
#ifdef CF_RENDERSYSTEM_D3D9
					d3dDevice->SetRenderState( D3DRS_ALPHATESTENABLE , TRUE );
					d3dDevice->SetRenderState( D3DRS_ALPHAREF , 0xfe ) ;
					d3dDevice->SetRenderState( D3DRS_ALPHAFUNC , D3DCMP_GREATER );
#else
					glEnable( GL_ALPHA_TEST );
					glAlphaFunc( GL_GREATER , 0.98 );
#endif

					restOptionBit |= BIT(CFRO_ALPHA_TEST);
				}
			}


#ifdef CF_RENDERSYSTEM_D3D9
			static DWORD const colorTypeMap[3] = { D3DMCS_MATERIAL , D3DMCS_COLOR1 , D3DMCS_COLOR2 };

			if ( material->mLightingColorUsage )
			{
				d3dDevice->SetRenderState( D3DRS_COLORVERTEX , TRUE );
				d3dDevice->SetRenderState( D3DRS_AMBIENTMATERIALSOURCE  , colorTypeMap[ material->getLightingColor( CFLC_AMBIENT ) ] );
				d3dDevice->SetRenderState( D3DRS_DIFFUSEMATERIALSOURCE  , colorTypeMap[ material->getLightingColor( CFLC_DIFFUSE ) ] );
				d3dDevice->SetRenderState( D3DRS_SPECULARMATERIALSOURCE , colorTypeMap[ material->getLightingColor( CFLC_SPECLAR ) ] );
				d3dDevice->SetRenderState( D3DRS_EMISSIVEMATERIALSOURCE , colorTypeMap[ material->getLightingColor( CFLC_EMISSIVE ) ] );
			}
			else
			{
				d3dDevice->SetRenderState( D3DRS_COLORVERTEX , FALSE );
				d3dDevice->SetRenderState( D3DRS_AMBIENTMATERIALSOURCE  , D3DMCS_MATERIAL );
				d3dDevice->SetRenderState( D3DRS_DIFFUSEMATERIALSOURCE  , D3DMCS_MATERIAL );
				d3dDevice->SetRenderState( D3DRS_SPECULARMATERIALSOURCE , D3DMCS_MATERIAL );
				d3dDevice->SetRenderState( D3DRS_EMISSIVEMATERIALSOURCE , D3DMCS_MATERIAL );
			}
#else
			if ( material->mLightingColorUsage )
			{
				d3dDevice->SetRenderState( D3DRS_COLORVERTEX , TRUE );
				d3dDevice->SetRenderState( D3DRS_AMBIENTMATERIALSOURCE  , colorTypeMap[ material->getLightingColor( CFLC_AMBIENT ) ] );
				d3dDevice->SetRenderState( D3DRS_DIFFUSEMATERIALSOURCE  , colorTypeMap[ material->getLightingColor( CFLC_DIFFUSE ) ] );
				d3dDevice->SetRenderState( D3DRS_SPECULARMATERIALSOURCE , colorTypeMap[ material->getLightingColor( CFLC_SPECLAR ) ] );
				d3dDevice->SetRenderState( D3DRS_EMISSIVEMATERIALSOURCE , colorTypeMap[ material->getLightingColor( CFLC_EMISSIVE ) ] );
			}
			else
			{
				d3dDevice->SetRenderState( D3DRS_COLORVERTEX , FALSE );
				d3dDevice->SetRenderState( D3DRS_AMBIENTMATERIALSOURCE  , D3DMCS_MATERIAL );
				d3dDevice->SetRenderState( D3DRS_DIFFUSEMATERIALSOURCE  , D3DMCS_MATERIAL );
				d3dDevice->SetRenderState( D3DRS_SPECULARMATERIALSOURCE , D3DMCS_MATERIAL );
				d3dDevice->SetRenderState( D3DRS_EMISSIVEMATERIALSOURCE , D3DMCS_MATERIAL );
			}
#endif

			if ( opacity != 1.0f && useTexLayer )
			{
				BYTE  alpha = BYTE( opacity * 255 );
				d3dDevice->SetTextureStageState( useStage , D3DTSS_CONSTANT , D3DCOLOR_ARGB( alpha ,0,0,0 ) );
				d3dDevice->SetTextureStageState( useStage , D3DTSS_COLOROP, D3DTOP_SELECTARG1 );
				d3dDevice->SetTextureStageState( useStage , D3DTSS_COLORARG1 , D3DTA_CURRENT );
				d3dDevice->SetTextureStageState( useStage , D3DTSS_ALPHAOP , D3DTOP_MODULATE );
				d3dDevice->SetTextureStageState( useStage , D3DTSS_ALPHAARG1 , D3DTA_CURRENT );
				d3dDevice->SetTextureStageState( useStage , D3DTSS_ALPHAARG2 , D3DTA_CONSTANT );

				d3dDevice->SetRenderState( D3DRS_ALPHABLENDENABLE , TRUE );
				d3dDevice->SetRenderState( D3DRS_SRCBLEND , D3DBLEND_SRCALPHA );
				d3dDevice->SetRenderState( D3DRS_DESTBLEND , D3DBLEND_INVSRCALPHA );

				restOptionBit |= BIT(CFRO_ALPHA_BLENGING) | BIT(CFRO_DEST_BLEND ) | BIT(CFRO_SRC_BLEND );
				//d3dDevice->SetRenderState( D3DRS_ZWRITEENABLE , FALSE );
				//restOptionBit |= BIT(CFRO_Z_BUFFER_WRITE );
				++useStage;
			}
		}

		d3dDevice->SetTextureStageState( useStage , D3DTSS_COLOROP, D3DTOP_DISABLE );
		d3dDevice->SetTextureStageState( useStage , D3DTSS_ALPHAOP, D3DTOP_DISABLE );
	}

}//namespace CFly