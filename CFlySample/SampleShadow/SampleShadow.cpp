#include "SampleBase.h"

#include "CFMeshBuilder.h"
#include "CFVertexUtility.h"

class IShadowCaster
{

};

class IShadowCasterData
{
public:
	virtual void release();
};



typedef IDirect3DVertexShader9 D3DVertexShader;
typedef IDirect3DPixelShader9  D3DPixelShader;

class IResource
{
	String name;
};


enum ShaderType
{
	CF_VERTEX_SHADER ,
	CF_PIXEL_SHADER ,
	CF_GOEM_SHADER ,
};

class IShader : public IResource
{
public:
	virtual void setupDevice( D3DDevice* device ) = 0;
	ShaderType getShaderType(){ return type; }
	ShaderType type;
};

class IVertexShader : public IShader
{
public:

};

class IShaderFactory
{

};



class ITechniquePass
{


};


class IShadowVolumeBuilder
{
public:
	IShadowVolumeBuilder()
		:meshBuilder( CFVT_XYZ_N )
	{

	}

	struct edge_t
	{
		int  oldIdx[2];
		int  newIdx1[2];
		int  newIdx2[2];
		bool beSwap;
	};

	void generateMesh( float* inVtx , int VtxOffset , int* inTriIndex , int numTri )
	{
		meshBuilder.reserveVexterBuffer( 8 * numTri );
		meshBuilder.reserveIndexBuffer( 2 * numTri );
		int numVtx = 0;

		for( int i = 0 ; i < numTri ; ++i )
		{
			int i0 = inTriIndex[ 3 * i + 0 ];
			int i1 = inTriIndex[ 3 * i + 1 ];
			int i2 = inTriIndex[ 3 * i + 2 ];

			float* v0 = inVtx + VtxOffset * i0;
			float* v1 = inVtx + VtxOffset * i1;
			float* v2 = inVtx + VtxOffset * i2;

			Vector3 normal = calcNormal( v0 , v1 , v2 );
			meshBuilder.setNormal( normal );

			meshBuilder.setPosition(Vector3(v0));
			meshBuilder.addVertex();
			meshBuilder.setPosition(Vector3(v1));
			meshBuilder.addVertex();
			meshBuilder.setPosition(Vector3(v2));
			meshBuilder.addVertex();

			int n0 = numVtx;
			int n1 = numVtx + 1;
			int n2 = numVtx + 2;

			meshBuilder.addTriangle( n0 , n1 , n2 );
			addEdgeMap( i0 , i1 , n0 , n1 );
			addEdgeMap( i1 , i2 , n1 , n2 );
			addEdgeMap( i2 , i0 , n2 , n0 );

			numVtx += 3;
		}

		for ( EdgeList::iterator iter = sharedEdgeList.begin();
			iter != sharedEdgeList.end() ; ++iter )
		{
			edge_t& edge = *iter;

			meshBuilder.addTriangle( 
				edge.newIdx1[0] , edge.newIdx2[0] , edge.newIdx1[1] ) ;

			meshBuilder.addTriangle( 
				edge.newIdx2[0] , edge.newIdx1[0] , edge.newIdx2[1]  ) ;
		}

		for ( EdgeList::iterator iter = unSharedEdgeList.begin();
			iter != unSharedEdgeList.end() ; ++iter )
		{
			edge_t& edge = *iter;

			EdgeList::iterator iter2 = iter;
			++iter2;
			for ( ; iter2 != unSharedEdgeList.end() ; ++iter2 )
			{
				edge_t& tEdge = *iter2;
				int    other;
				int    iNormal;
				int    iShare;

				if ( edge.oldIdx[0] == tEdge.oldIdx[0] )
				{
					iShare = 0;
					other = tEdge.oldIdx[1];
					iNormal = ( tEdge.beSwap )? tEdge.newIdx1[1] : tEdge.newIdx1[0];
				}
				else if ( edge.oldIdx[1] == tEdge.oldIdx[0] )
				{
					iShare = 1;
					other = tEdge.oldIdx[1];
					iNormal = ( tEdge.beSwap )? tEdge.newIdx1[1] : tEdge.newIdx1[0];
				}
				else if ( edge.oldIdx[0] == tEdge.oldIdx[1] )
				{
					iShare = 0;
					other = tEdge.oldIdx[0];
					iNormal = ( tEdge.beSwap )? tEdge.newIdx1[0] : tEdge.newIdx1[1];
				}
				else if ( edge.oldIdx[1] == tEdge.oldIdx[1] )
				{
					iShare = 1;
					other = tEdge.oldIdx[0];
					iNormal = ( tEdge.beSwap )? tEdge.newIdx1[0] : tEdge.newIdx1[1];
				}
				else
				{
					continue;
				}

				int i0 = edge.oldIdx[0];
				int i1 = edge.oldIdx[1];
				if ( iShare == 0)
					std::swap( i0 , i1 );

				int i2 = other;

				float* v0 = inVtx + VtxOffset * i0;
				float* v1 = inVtx + VtxOffset * i1;
				float* v2 = inVtx + VtxOffset * i2;

				float* n = meshBuilder.getVertexComponent( iNormal , CFCM_NORMAL );

				Vector3 normal = calcNormal( v0 , v1 , v2 );


				if ( normal.dot( Vector3(n) ) > 0 )
					normal = -normal;

				meshBuilder.setNormal( normal );

				meshBuilder.setPosition(Vector3(v0));
				meshBuilder.addVertex();
				meshBuilder.setPosition(Vector3(v1));
				meshBuilder.addVertex();
				meshBuilder.setPosition(Vector3(v2));
				meshBuilder.addVertex();

				int n0 = numVtx;
				int n1 = numVtx + 1;
				int n2 = numVtx + 2;

				meshBuilder.addTriangle( n0 , n1 , n2 );

				//meshBuilder.addTriangle( 
				//	edge.newIdx1[0] , edge.newIdx2[0] , edge.newIdx1[1] ) ;

				//meshBuilder.addTriangle( 
				//	edge.newIdx2[0] , edge.newIdx1[0] , edge.newIdx2[1]  ) ;


				//meshBuilder.addTriangle( 
				//	edge.newIdx1[0] , edge.newIdx2[0] , edge.newIdx1[1] ) ;

				//meshBuilder.addTriangle( 
				//	edge.newIdx2[0] , edge.newIdx1[0] , edge.newIdx2[1]  ) ;
			}
		}
	}


	void addEdgeMap( int idx0 , int idx1 , int newIdx0 , int newIdx1 )
	{
		bool beSwap = false;
		if ( idx0 > idx1 )
		{
			beSwap =true;
			std::swap( idx0 , idx1 );
		}


		for ( EdgeList::iterator iter = unSharedEdgeList.begin();
			  iter != unSharedEdgeList.end() ; ++iter )
		{
			edge_t& edge = *iter;

			if ( edge.oldIdx[0] != idx0 ||
				 edge.oldIdx[1] != idx1 )
				continue;

			assert( edge.newIdx2[0] == -1 );

			edge.newIdx2[0] = newIdx0;
			edge.newIdx2[1] = newIdx1;

			sharedEdgeList.splice( sharedEdgeList.end() , unSharedEdgeList , iter );
			return ;
		}

		edge_t edge;
		edge.beSwap = beSwap;
		edge.oldIdx[0] = idx0;
		edge.oldIdx[1] = idx1;
		edge.newIdx1[0] = newIdx0;
		edge.newIdx1[1] = newIdx1;
		edge.newIdx2[0] = -1;
		edge.newIdx2[1] = -1;

		unSharedEdgeList.push_back( edge );
		return;
	}

	int createShadowVolumeMesh( Object* obj )
	{
		World* world = obj->getScene()->getWorld();
		Material* mat = world->createMaterial( NULL );
		ShaderEffect* shader = mat->addShaderEffect( "shadowVolume" , "RenderSVolume" );

		return meshBuilder.createIndexTrangle( obj , mat );
	}

	typedef std::list< edge_t > EdgeList; 
	EdgeList unSharedEdgeList;
	EdgeList sharedEdgeList;
	MeshBuilder meshBuilder;
};

class ShadowSample : public SampleBase
{
public:
	Light*    mainLight;
	Actor*    actor;
	Object*   plane;

	Object*   vShadowObj;
	Object*   ball;

	static int const CubeMapSize = 512;
	bool onSetupSample()
	{
		mMainScene->setAmbientLight( Color4f(0.3,0.3,0.3) );
		mainLight = mMainScene->createLight( nullptr );
		mainLight->registerName( "mainLight" );
		mainLight->translate( Vector3(0,100,0) , CFTO_LOCAL );
		mainLight->setLightColor( Color4f(1,1,1) );
		mainLight->setAttenuation( 1.0 , 0.001f , 0.0001f );

		{
			Object* obj = mMainScene->createObject( mainLight );

			obj->load( "ball.cw3" );
			obj->scale( Vector3(0.1,0.1,0.1) , CFTO_LOCAL ) ;

		}

		ball = mMainScene->createObject( nullptr );

		//ball->createPlane( NULL , 100 , 100 , 0 , 1 , 1 , Vector3(0,1,0) );
		ball->load( "box.cw3" );
		ball->setOpacity( 0.7f );
		ball->addVertexNormal();

		//ball->show( false );

		MeshBase* geom = ball->getElement( 0 )->getMesh();
		VertexBuffer* vtxBuffer = geom->_getVertexBuffer();
		float* v = reinterpret_cast< float*>( vtxBuffer->lock() );

		IndexBuffer* idxBuffer = geom->getIndexBuffer();
		assert( idxBuffer->isUseInt32() );
		int* idxTri = (int*)idxBuffer->lock( CFBL_READ_ONLY );


		IShadowVolumeBuilder builder;
		builder.generateMesh( v , vtxBuffer->getVertexSize() / 4 , idxTri , geom->getElementNum() );

	
		idxBuffer->unlock();

		vShadowObj = mMainScene->createObject( nullptr );
		builder.createShadowVolumeMesh( vShadowObj );
		vShadowObj->translate( Vector3(0,0,100) , CFTO_LOCAL );
		Material* mat = vShadowObj->getElement( 0 )->getMaterial();
		ShaderEffect* shader = mat->getShaderEffect();

		shader->addParam( SP_WVP , "mWVP" );
		shader->addParam( SP_WORLD_VIEW , "mWorldView" );
		shader->addParam( SP_VIEW , "mView" );
		shader->addParam( SP_WORLD , "mWorld" );
		shader->addParam( SP_VIEW_PROJ , "mViewProj" );
		shader->addParam( SP_PROJ , "mProj" );
		shader->addParam( SP_CAMERA_Z_FAR , "zFarClip" );
		shader->addParam( SP_LIGHT_POS , "mainLight" , "lightPosition" );


		{
			plane = mMainScene->createObject( nullptr );

			Material* mat = mWorld->createMaterial( Vector3(0.3,0.3,0.3) , Vector3(0.6,0.6,0.6) );
			mat->addTexture( 0 , 0 , "0016" );

			ShaderEffect* shader = mat->addShaderEffect( "phong" , "Phong" );

			shader->addParam( SP_WORLD , "mWorld" );
			shader->addParam( SP_WVP   , "mWVP" );
			shader->addParam( SP_WORLD_INV , "mWorldInv" );
			shader->addParam( SP_AMBIENT_LIGHT , "ambLgt" );
			shader->addParam( SP_LIGHT_POS , "mainLight" , "mainLightPosition" );
			shader->addParam( SP_LIGHT_DIFFUSE , "mainLight" , "mainLightColor" );
			shader->addParam( SP_LIGHT_ATTENUATION , "mainLight" , "atten" );
			shader->addParam( SP_CAMERA_POS , "camPosition" );
			shader->addParam( SP_MATERIAL_AMBIENT , "amb" );
			shader->addParam( SP_MATERIAL_DIFFUSE , "dif" );
			shader->addParam( SP_MATERIAL_SPECULAR , "spe" );
			shader->addParam( SP_MATERIAL_SHINENESS , "power" );
			shader->addParam( SP_MATERIAL_TEXLAYER0 , "texMap" );

			plane->createPlane( mat ,  1000 , 1000 , 0 , 5 , 5, Vector3(0,1,0) , Vector3(1,0,0) );
			plane->translate( Vector3(0,-100 ,0 ) , CFTO_LOCAL );

		}


		lightHeight = 100.0f;
		return true; 
	}

	 void onExitSample()
	 {

	 }

	float lightHeight;
	long onUpdate( long time )
	{
		static float angle = 0.0f;
		static float theta = 0.0f;

		float radius = 50;

		SampleBase::onUpdate( time );
		mainLight->setLocalPosition( Vector3( 
			radius * Math::Sin( theta ) * Math::Cos( angle ) ,  
			lightHeight + radius * Math::Cos( theta ) ,  
			radius * Math::Sin( theta ) * Math::Sin( angle ) ) );


		angle += 0.001 * time;
		theta += 0.0005 * time;

		return time;
	}

	MsgReply handleKeyEvent(KeyMsg const& msg)
	{
		if (msg.isDown())
		{
			switch (msg.getCode())
			{
			case EKeyCode::O: lightHeight += 1; break;
			case EKeyCode::P: lightHeight -= 1; break;
			}
		}
		return SampleBase::handleKeyEvent(msg);
	}

	void stencielTest()
	{
		D3DDevice* device = mMainScene->getD3DDevice();

		device->SetRenderState( D3DRS_STENCILENABLE , TRUE );
		device->SetRenderState( D3DRS_STENCILREF  , 1 );
		device->SetRenderState( D3DRS_STENCILMASK , 0xffffffff );
		device->SetRenderState( D3DRS_STENCILFUNC , D3DCMP_ALWAYS );
		device->SetRenderState( D3DRS_STENCILFAIL , D3DSTENCILOP_REPLACE );
		device->SetRenderState( D3DRS_STENCILZFAIL , D3DSTENCILOP_REPLACE );
		device->SetRenderState( D3DRS_STENCILPASS  , D3DSTENCILOP_REPLACE );
		device->SetRenderState( D3DRS_ZWRITEENABLE , FALSE );

		mMainScene->renderObject( plane , mMainCamera , mMainViewport , CFRF_LIGHTING_DISABLE | CFRF_DEFULT );


		device->SetRenderState( D3DRS_STENCILENABLE , TRUE );
		device->SetRenderState( D3DRS_STENCILREF   , 1 );
		device->SetRenderState( D3DRS_STENCILMASK  , 0xffffffff );
		device->SetRenderState( D3DRS_STENCILFUNC  , D3DCMP_EQUAL );
		device->SetRenderState( D3DRS_STENCILFAIL  , D3DSTENCILOP_KEEP );
		device->SetRenderState( D3DRS_STENCILZFAIL , D3DSTENCILOP_KEEP );
		device->SetRenderState( D3DRS_STENCILPASS  , D3DSTENCILOP_KEEP );
		device->SetRenderState( D3DRS_ZWRITEENABLE , TRUE );

		mMainScene->renderObject( ball , mMainCamera , mMainViewport , 0 );

	}
	void onRenderScene()
	{
		D3DDevice* device = mMainScene->getD3DDevice();

		//stencielTest();

		vShadowObj->show( false );
		device->SetRenderState( D3DRS_STENCILENABLE , FALSE );
		device->SetRenderState( D3DRS_ZFUNC         , D3DCMP_LESSEQUAL );
		mMainScene->render(  mMainCamera , mMainViewport ,  CFRF_LIGHTING_DISABLE | CFRF_DEFULT );

		vShadowObj->show( true );
		mMainScene->renderObject( vShadowObj , mMainCamera , mMainViewport , CFRF_FRUSTUM_CLIP_DISABLE );
		
		//device->SetRenderState( D3DRS_CULLMODE , D3DCULL_CCW );
        device->SetRenderState( D3DRS_ZFUNC   ,  D3DCMP_LESSEQUAL );
        device->SetRenderState( D3DRS_STENCILENABLE , TRUE );
		device->SetRenderState( D3DRS_ZWRITEENABLE , TRUE );
        device->SetRenderState( D3DRS_ALPHABLENDENABLE , TRUE );
        device->SetRenderState( D3DRS_BLENDOP , D3DBLENDOP_ADD );
        device->SetRenderState( D3DRS_SRCBLEND , D3DBLEND_ONE );
        device->SetRenderState( D3DRS_DESTBLEND , D3DBLEND_ONE );
        device->SetRenderState( D3DRS_STENCILREF   , 1 );
		device->SetRenderState( D3DRS_STENCILMASK  , 0xffffffff );
        device->SetRenderState( D3DRS_STENCILFUNC  , D3DCMP_GREATER );
		device->SetRenderState( D3DRS_STENCILFAIL  , D3DSTENCILOP_KEEP );
		device->SetRenderState( D3DRS_STENCILZFAIL , D3DSTENCILOP_KEEP );
		device->SetRenderState( D3DRS_STENCILPASS  , D3DSTENCILOP_KEEP );
		

		vShadowObj->show( false );
		mMainScene->render(  mMainCamera , mMainViewport , 0  );
	}

	void onShowMessage()
	{
		SampleBase::onShowMessage();
		Vector3 pos = mMainCamera->getWorldPosition();
		pushMessage( "cam pos = %f %f %f" , pos.x , pos.y , pos.z );
	}


};

INVOKE_SAMPLE( ShadowSample )
