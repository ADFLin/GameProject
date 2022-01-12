#include "ProjectPCH.h"
#include "CMiniMapUI.h"

#include "CFWorld.h"
#include "CFObject.h"
#include "CFMaterial.h"
#include "CFCamera.h"
#include "CFScene.h"

#include "CSceneLevel.h"
#include "CUISystem.h"
#include "SpatialComponent.h"
#include "CActor.h"
#include "UtilityFlyFun.h"
#include "UtilityMath.h"

#include "EventType.h"
#include "UtilityGlobal.h"

int const CMiniMapUI::MapRadius = 140;
int const CMiniMapUI::Length   =  200;

int const gMapTextureSize = 1024;

static float gViewDist[] = 
{
    600 , 1200 , 1500 , 3000 , 5000 , 8000 , 10000, 12000 ,
};


CMiniMapUI::CMiniMapUI( Vec2i const& pos ) 
	:CWidget(  pos , Vec2i( Length , Length ) , nullptr )
{
	Vec2i size( Length , Length );

	CUISystem::Get().setTextureDir("Data/UI/");

	mSprite->createRectArea( 0 , 0, size.x , size.y , "minimap" , 0 , 0.0f , nullptr , &Color3ub(255,255,255)  );
	mSprite->setRenderOption( CFly::CFRO_ALPHA_BLENGING , true );
	//m_spr.SetRenderOption( SOURCE_BLEND_MODE , BLEND_SRC_ALPHA  );
	//m_spr.SetRenderOption( DESTINATION_BLEND_MODE , BLEND_ONE );


	int btnSize = 20;
	int shift_y = 5;
	{
		char const* dir = "Data/UI/miniMap";
		char const* texName[] = { "up" , "up_press" , "up" };
		CButtonUI* button  = new CSimpleButton( dir , &texName[0] , 
			Vec2i( size.x - btnSize - 5 , Length / 2 - shift_y - btnSize ) ,
			Vec2i( btnSize , btnSize ) , this  );
		UG_ConnectEvent( EVT_UI_BUTTON_CLICK , button->getID() , EventCallBack( this , &CMiniMapUI::increaseMapViewDist ) );
	}

	{
		char const* dir = "Data/UI/miniMap";
		char const* texName[] = { "down" , "down_press" , "down" };
		CButtonUI* button  = new CSimpleButton( dir , &texName[0] , 
			Vec2i( size.x - btnSize - 5 , Length / 2 + shift_y ) , 
			Vec2i( btnSize , btnSize ) , this  );
		UG_ConnectEvent( EVT_UI_BUTTON_CLICK , button->getID() , EventCallBack( this , &CMiniMapUI::decreaseMapViewDist ) );
	}

	rotateMap = true;

	mapScaleFactor = 7000;
	viewDistIndex = 0;
	mapViewDist = gViewDist[ viewDistIndex ]; 
	
	createMapObject();
}


CMiniMapUI::~CMiniMapUI()
{
	if ( mMapObject )
		mMapObject->release();
}

void CMiniMapUI::increaseMapViewDist( TEvent const& event )
{
	++viewDistIndex;
	if ( viewDistIndex >= ARRAY_SIZE( gViewDist) )
		viewDistIndex = ARRAY_SIZE( gViewDist) - 1;

	mapViewDist = gViewDist[ viewDistIndex ];
}

void CMiniMapUI::decreaseMapViewDist( TEvent const& event )
{
	--viewDistIndex;
	if ( viewDistIndex < 0 )
		viewDistIndex = 0;

	mapViewDist = gViewDist[ viewDistIndex ];
}


void CMiniMapUI::onUpdateUI()
{
	CActor* actor = handle;

	if ( !actor )
		return;

	Vec3D playerPos = mSpatialComp->getPosition();
	//static float const vf[ 4 ][ 2] = 
	//{ 
	//	- 0.5f , - 0.5f ,  0.5f , -0.5f ,  
	//	  0.5f ,   0.5f , -0.5f ,  0.5f ,
	//};

	static float const vf[ 4 ][ 2] = 
	{ 
		- 0.5f , - 0.5f ,  0.5f , -0.5f ,  
		0.5f ,   0.5f , -0.5f ,  0.5f ,
	};



	float tx = playerPos.x / mapScaleFactor + 0.5f;
	float ty = playerPos.y / mapScaleFactor + 0.5f;

	float c = 1;
	float s = 0;

	
	if ( rotateMap )
	{
		Vec3D dir = actor->getFaceDir();
		float angle = UM_Angle( dir , Vec3D( 0 , 1 , 0 ) );
		if ( dir.cross( Vec3D(0,1,0) ).z > 0  )
			angle = -angle;

		c = cos( angle );
		s = sin( angle );
	}

	float tl = mapViewDist /  mapScaleFactor;

	CFly::Locker< CFly::VertexBuffer > locker( mTexCoordBuffer );

	char* data = static_cast< char* >( locker.getData() );

	if ( !data )
		return;

	data += mTexCoordOffset;
	float* texcoord = reinterpret_cast< float* >( data + mTexCoordOffset );

	for( int i = 0 ; i < 4 ; ++ i )
	{
		float* texcoord = reinterpret_cast< float* >( data + i * mTexCoordBuffer->getVertexSize()  );
		//DevMsg( 5 , "( %.3f , %.3f )" , tx ,ty );

		float lx = vf[i][0] * tl;
		float ly = vf[i][1] * tl;

		if ( rotateMap )
		{
			texcoord[0] = tx + c * lx + s * ly ;
			texcoord[1] = ty + s * lx - c * ly ;
		}
		else
		{
			texcoord[0] = tx + lx ;
			texcoord[1] = ty - ly ;
		}
	}
}

void CMiniMapUI::makeMapTexture( CFScene* scene )
{
	CFCamera* cam = scene->createCamera();

	Vec3D pos( 0 , 0 , 5000 );
	
	cam->setProjectType( CFly::CFPT_ORTHOGONAL );
	float s = mapScaleFactor / 2;
	cam->setScreenRange( -s , s , -s  , s );
	
	cam->setAspect( 1.0f );

	cam->setLookAt( pos , pos + Vec3D(0,0,-1) , Vec3D(0,-1,0) );
	cam->setNear( -10000 );
	cam->setFar( 10000 );

	scene->render( cam , mMapViewport );
	cam->release();
}

void CMiniMapUI::setLevel( CSceneLevel* level )
{
	//FnScene mapScene;
	//mapScene.Object(  UF_GetWorld().CreateScene(1) );

	//std::vector< OBJECTid >& objVec = level->getLevelObjVec() ; 
	//for( int i = 0;i < objVec.size();++i )
	//{
	//	FnObject obj , cpyObj;
	//	obj.Object( objVec[i] );
	//	cpyObj.Object( obj.Instance(FALSE) );
	//	cpyObj.ChangeScene( mapScene.Object() );
	//	cpyObj.SetRenderOption( LIGHTING , FALSE );
	//}

	makeMapTexture( level->getRenderScene() );
}

void CMiniMapUI::drawText()
{

}

MsgReply CMiniMapUI::onMouseMsg( MouseMsg const& msg )
{
	CWidget::onMouseMsg( msg );
	if ( msg.onLeftDown() )
	{
		//TVec2D dPos = msg.getPos() - getWorldPos() - ( 0.5 *getSize() )  ;
		//if ( dPos.x * dPos.x + dPos.y * dPos.y < MapRadius * MapRadius )
		//{
		//	Vec3D pos = transWorldPos( dPos );
		//	pos[2] += 1000;
		//	TraceCallback callback;
		//	RayLineTest( pos , pos + Vec3D( 0 , 0 , -10000 ) , COF_TERRAIN , NULL , &callback );

		//	if ( callback.hasHit() )
		//	{
		//		m_control->setRoutePos( callback.getHitPos() ) ;
		//	}
		//}
	}
	return MsgReply::Handled();
}

Vec3D CMiniMapUI::transWorldPos( Vec2i const& dPos )
{
	CActor* actor = handle;
	assert( actor );

	float dL = mapViewDist / MapRadius;

	Vec3D faceDir = actor->getFaceDir();
	Vec3D rightSideDir = faceDir.cross( Vec3D(0,0,1) );

	return mSpatialComp->getPosition() + faceDir * ( dPos.y * dL ) 
		   + rightSideDir * ( dPos.x * dL );
}


void CMiniMapUI::createMapObject()
{

	mMapViewport = CUISystem::Get().getCFWorld()->createViewport( 0, 0 , gMapTextureSize , gMapTextureSize );
	mMapViewport->setBackgroundColor( 0.01 , 0.01 , 0.01 );

	Material* mapMat = CUISystem::Get().getCFWorld()->createMaterial();
	CUISystem::Get().setTextureDir( "Data/UI/" );

	mapMat->addRenderTarget( 0 , 0 , "mini_map" , CFly::CF_TEX_FMT_RGB32 , mMapViewport , true );
	mapMat->addTexture( 0 , 1 , "mask" );

	{
		CFly::TextureLayer& texLayer = mapMat->getTextureLayer(0);
		texLayer.setAddressMode( 0 , CFly::CFAM_CLAMP );
		texLayer.setAddressMode( 1 , CFly::CFAM_CLAMP );
		texLayer.setUsageTexcoord(0);
		texLayer._setD3DTexOperation( 
			D3DTOP_MODULATE , D3DTA_DIFFUSE , D3DTA_TEXTURE ,
			D3DTOP_SELECTARG2 , D3DTA_DIFFUSE , D3DTA_TEXTURE );
	}
	{
		CFly::TextureLayer& texLayer = mapMat->getTextureLayer(1);
		texLayer.setAddressMode( 0 , CFly::CFAM_CLAMP );
		texLayer.setAddressMode( 1 , CFly::CFAM_CLAMP );
		texLayer.setUsageTexcoord(1);
		texLayer._setD3DTexOperation( 
			D3DTOP_SELECTARG1 , D3DTA_CURRENT , D3DTA_TEXTURE ,
			D3DTOP_SELECTARG2 , D3DTA_CURRENT , D3DTA_TEXTURE );
	}

	mMapObject = mSprite->getScene()->createObject( mSprite );

	mMapObject->enableVisibleTest( false );
	mMapObject->setRenderOption( CFly::CFRO_ALPHA_BLENGING , true );
	mMapObject->setRenderOption( CFly::CFRO_SRC_BLEND  , CFly::CF_BLEND_SRC_ALPHA  );
	mMapObject->setRenderOption( CFly::CFRO_DEST_BLEND , CFly::CF_BLEND_INV_SRC_ALPHA  );
	mMapObject->setRenderOption( CFly::CFRO_CULL_FACE , CFly::CF_CULL_NONE );

	float v[ 10 * 4 ];
	float* vtx = v;
	Vec3D color( 1,1,1 );

	float w = MapRadius;

	int offset = ( Length - w ) / 2;
	Vec3D off( offset , offset , 0 );

	vtx = UF_SetXYZ_RGB( vtx , Vec3D( 0 , 0 , 0 ) , color , 0.0f , 0.0f , 0.0f , 0.0f );
	vtx = UF_SetXYZ_RGB( vtx , Vec3D( w , 0 , 0 ) , color , 1.0f , 0.0f , 1.0f , 0.0f );
	vtx = UF_SetXYZ_RGB( vtx , Vec3D( w , w , 0 ) , color , 1.0f , 1.0f , 1.0f , 1.0f );
	vtx = UF_SetXYZ_RGB( vtx , Vec3D( 0 , w , 0 ) , color , 0.0f , 1.0f , 0.0f , 1.0f );

	int tri[6] = { 0 , 1 , 2 , 0 , 2 , 3 };

	using namespace CFly;
	VertexType vertexType = CFVT_XYZ_CF1 | CFVF_TEX2( 2 , 2 );
	int goemID = mMapObject->createIndexedTriangle( mapMat , vertexType , v , 4 , tri , 2 );
	MeshBase* shape = mMapObject->getElement( goemID )->getMesh();
	mTexCoordBuffer = shape->getVertexElement( CFly::CFV_TEXCOORD , mTexCoordOffset );

	mMapObject->setLocalPosition( off );

}

void CMiniMapUI::setFollowActor( CActor* actor )
{
	handle = actor;
	mSpatialComp = actor->getSpatialComponent();
}
