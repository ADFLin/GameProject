#include "TShadow.h"

#include "ConsoleSystem.h"
#include "THero.h"
#include "TActor.h"
#include "TLevel.h"
#include "UtilityMath.h"
#include "profile.h"
#include "TheFlyDX.h"

DEFINE_CON_VAR( bool  , gShadowShow , true );
DEFINE_CON_VAR( int   , gShadowMaxNum , 10 );
DEFINE_CON_VAR( float , gShadowMaxRadius , 500 );

std::vector< int > FuCShadowModify::m_triIDVec;

TShadow::TShadow( FnScene& scene ) 
	:FuCShadowModify( scene.Object() )
{
	//scene.SetCurrentRenderGroup(RG_SHADOW);
	CreateShadowMaterial( Vec3D(0.4,0.4,0.4) );
	SetRenderTarget( 512 , Vec3D(1,1,1) );
	SetShadowOffsetFromGround( 0.2f );
}

void TShadow::setActor( TActor* actor )
{
	if ( actor )
	{
		m_fyActor.Object( actor->getFlyActor().Object() );
		
		SetShadowSize( 200 );
		Show( TRUE );

		Vec3D pos( 100 , 100 , 500 );
		SetCharacter( m_fyActor.Object() , pos );
	}
	else
	{
		Show( false );
	}

}

void TShadow::render( FnTerrain& fyTerrain )
{
	setTerrain( fyTerrain.Object() );
	Render();
}

void TShadow::refreshObj()
{
	m_objVec.clear();
	fillActorObject( m_fyActor );
}

bool TShadowManager::UpdateCallBack::processResult( TEntityBase* entity )
{
	if ( entity == manger->m_player )
		return true;

	if ( TActor* actor = TActor::upCast(entity) )
	{
		if ( actor->isDead() )
			return true;

		if ( num == gShadowMaxNum )
			return false;
		manger->setShadowActor( num , actor );
		++num;
	}
	//else if( TObject* obj = TObject::upCast( entity ) )
	//{
	//	manger->m_playerShadow->fillObject( obj->getFlyObj() );
	//}
	return true;
}

TShadowManager::TShadowManager()
{
	callback.manger = this;
	m_playerShadow = NULL;
}

TShadowManager::~TShadowManager()
{
	//delete m_playerShadow;
	//for( int i = 0 ; i < shadowVec.size() ; ++i )
	//{
	//	delete shadowVec[i];
	//}
}

void TShadowManager::init( THero* player )
{
	setPlayer( player );
}

void TShadowManager::setPlayer( THero* player )
{
	m_player =  player;
	if ( m_playerShadow )
		m_playerShadow->setActor( player );
}

void TShadowManager::changeLevel( TLevel* level )
{
	for( int i = 0 ; i < shadowVec.size() ; ++i )
	{
		delete shadowVec[i];
	}

	delete m_playerShadow;

	shadowVec.clear();
	m_playerShadow =  new TShadow( level->getFlyScene() );
	
	curLevel = level;
}

void TShadowManager::setShadowActor( int i , TActor* actor )
{
	if ( i < shadowVec.size() )
	{
		shadowVec[i]->setActor( actor );
	}
	else
	{
		TShadow* shadow = new TShadow( curLevel->getFlyScene() );
		shadow->setActor( actor );
		shadowVec.push_back( shadow );
	}
}

void TShadowManager::render()
{
	if ( ! gShadowShow )
		return ;

	if ( !m_player )
		return;

	TPROFILE("Shadow Render");

	static int frame = 0;

	++frame;
	frame = frame % 15;
	if ( frame == 0 )
		refreshTerrainTri();

	callback.num = 0;
	//m_playerShadow->m_objVec.clear();
	//m_playerShadow->fillActorObject( m_player->getFlyActor() );
	TEntityManager::instance().findEntityInSphere( 
		m_player->getPosition() , gShadowMaxRadius , &callback );

	for ( int i = callback.num ; i < shadowVec.size() ; ++i )
	{
		shadowVec[i]->setActor( NULL );
	}

	FnTerrain terrain;
	terrain.Object( curLevel->getFlyTerrain().Object() );

	m_playerShadow->render( terrain );

	for ( int i = 0 ; i < callback.num ; ++i )
	{
		shadowVec[i]->render( terrain );
	}
}

void TShadowManager::refreshTerrainTri()
{
	std::vector< int >& triVec = FuCShadowModify::getTriIDVec();

	triVec.clear();

	FnTriangle tT;
	tT.Object( curLevel->getFlyTerrain().Object() , 0);

	Vec3D actorPos = m_player->getPosition();
	int nList = tT.GetTriangleNumber();

	int tri[3];
	float pos[16];

	float r2 = gShadowMaxRadius * gShadowMaxRadius;

	for (int i = 0; i < nList; i++)
	{
		tT.GetTopology( i , tri);
		tT.GetVertex(tri[0], pos);
		float dx = actorPos.x() - pos[0];
		float dy = actorPos.y() - pos[1];
		if ( dx * dx + dy * dy < r2 )
			triVec.push_back( i );
	}
}

void TShadowManager::refreshPlayerObj()
{
	m_playerShadow->refreshObj();
} 

//////////////////////////////////////////////////////
static void FuShadowDisplayCallback(OBJECTid oID)
{
   FnObject seed;
   FuCShadowModify **s;

   seed.Object(oID);
   s = (FuCShadowModify **) seed.GetData();

   (*s)->Display();
}


/*---------------------------------
  Transform vertex with M16 matrix
  C.Wang 1013, 2004
 ----------------------------------*/
void FUTransformVertexWithM16_Simple(float *v0, float *ov0, float *M)
{
   *(v0) = (*ov0)*M[0] + (*(ov0+1))*M[4] + (*(ov0+2))*M[8] + M[12];
   *(v0+1) = (*ov0)*M[1] + (*(ov0+1))*M[5] + (*(ov0+2))*M[9] + M[13];
   *(v0+2) = (*ov0)*M[2] + (*(ov0+1))*M[6] + (*(ov0+2))*M[10] + M[14];
}      


/*-------------------------------------
  constructor of classic shadow object
  C.Wang 0608, 2007
 --------------------------------------*/
FuCShadowModify::FuCShadowModify(SCENEid sID, int rgID)
{
   // initialize the shadow
   mHost = sID;
   mShadowMat = FAILED_MATERIAL_ID;
   mViewport = FAILED_ID;
   mRenderTarget = FAILED_MATERIAL_ID;

   // create the camera
   FnScene scene;
   FnObject seed;

   scene.Object(sID);
   m_lightCam.Object( scene.CreateCamera(ROOT) );

   // backup the rendering group
   int oldRG = scene.GetCurrentRenderGroup();

   if (rgID >= 0) {
      // set current rendering group
      scene.SetCurrentRenderGroup(rgID);
   }

   // create the seed object for displaying the shadow
   mSeed = scene.CreateObject(ROOT);
   seed.Object(mSeed);
   seed.BindPostRenderFunction(FuShadowDisplayCallback);
   seed.SetRenderOption(ALPHA, TRUE);
   FuCShadowModify **s = (FuCShadowModify **)seed.InitData(sizeof(FuCShadowModify *));
   *s = this;

   mShadowHeightOffset = 0.4f;
   mShadowSize = 50.0f;

   // recover current rendering group
   scene.SetCurrentRenderGroup(oldRG);

}


/*------------------------------------
  destructor of classic shadow object
  C.Wang 0522, 2006
 -------------------------------------*/
FuCShadowModify::~FuCShadowModify()
{
   // delete the associated camera
   FnScene scene;
   FnWorld gw;
   scene.Object(mHost);
   gw.Object(scene.GetWorld());

   gw.DeleteViewport(mViewport);
   mViewport = FAILED_ID;

   gw.DeleteMaterial(mShadowMat);
   mShadowMat = FAILED_MATERIAL_ID;

   gw.DeleteMaterial(mRenderTarget);
   mRenderTarget = FAILED_MATERIAL_ID;

   scene.DeleteCamera( m_lightCam.Object() );
   m_lightCam.Object( FAILED_ID );

   scene.DeleteObject(mSeed);
   mSeed = FAILED_ID;

}


/*-----------------------
  show / hide the shadow
  C.Wang 0910, 2006
 ------------------------*/
void FuCShadowModify::Show(BOOL beShow)
{
   FnObject seed;
   seed.Object(mSeed);
   seed.Show(beShow);
}


/*-------------------------------
  create the material for shadow
  C.Wang 0522, 2006
 --------------------------------*/
void FuCShadowModify::CreateShadowMaterial(float *color)
{
   FnWorld gw;
   FnScene scene;
   scene.Object(mHost);
   gw.Object(scene.GetWorld());

   if (mShadowMat != FAILED_MATERIAL_ID) {
      gw.DeleteMaterial(mShadowMat);
   }

   // create the material
   float blackColor[3];

   blackColor[0] = blackColor[1] = blackColor[2] = 0.0f;
   mShadowMat = gw.CreateMaterial(blackColor, blackColor, blackColor, 1.0f, color);
}


/*--------------------------
  set rendering target data
  C.Wang 0522, 2006
 ---------------------------*/
void FuCShadowModify::SetRenderTarget(int w, float *bgColor)
{
   FnScene scene;
   FnWorld gw;
   FnViewport vp;
   char shadowName[256];

   scene.Object(mHost);
   gw.Object(scene.GetWorld());
   gw.DeleteMaterial(mRenderTarget);
   gw.DeleteViewport(mViewport);

   // create the viewport with the associated size
   mViewport = gw.CreateViewport(0, 0, w, w);

   // create the render target material
   mRenderTarget = gw.CreateMaterial(NULL, NULL, NULL, 1.0f, NULL);

   // generate a unique shadow texture name
   sprintf(shadowName, "shadow%d", mRenderTarget);

   FnMaterial mat;
   mat.Object(mRenderTarget);
   mat.AddRenderTargetTexture(0, 0, shadowName, TEXTURE_32, mViewport);
   mat.SetTextureAddressMode(0, CLAMP_TEXTURE);

   // set render target
   vp.Object(mViewport);
   vp.SetRenderTarget(mRenderTarget, 0, 0, 0);
   vp.SetBackgroundColor(bgColor[0], bgColor[1], bgColor[2]);
}


/*----------------------------------------
  set the character to be rendered shadow
  C.Wang 0522, 2006
 -----------------------------------------*/
void FuCShadowModify::SetCharacter(CHARACTERid acID, float *pos)
{
   m_fyActor.Object(acID);

   if (acID == FAILED_ID) 
	   return;

   m_objVec.clear();
   fillActorObject( m_fyActor );

   // set camera's position and orientation
   float dir[3];

   dir[0] = -pos[0];
   dir[1] = -pos[1];
   dir[2] = -pos[2];
   FyNormalizeVector3(dir);

   m_lightPos.setValue( pos[0] ,pos[1],pos[2] );
   m_lightCam.SetWorldDirection(dir, NULL);

}


/*----------------------------------
  display the shadow on the terrain
  C.Wang 0522, 2006
 -----------------------------------*/
void FuCShadowModify::Display()
{
	TPROFILE("Shadow Display");

	if ( m_fyActor.Object() == FAILED_ID ) 
		return;

	FnObject model;
	model.Object(mSeed);
	if (!model.GetVisibility()) 
		return;

	WORLDid wID;
	FnTriangle tT;
	FnScene scene;
	FnTerrain terrain;
	int vLen , texLen = 2, tri[3];
	float M[16], *G, vLC[3];
	
	// get all neighboring triangles
	//nList = terrain.GetAllVertexNeighborTriangles(iOne, list, 64);
	//if (nList <= 0) return;

	// get the matrix to character's base coordinate
	m_lightCam.SetWorldPosition( m_actorPos );
	G = m_lightCam.GetMatrix(TRUE);
	FyInverseM16(G, M);

	tT.Object( m_terrain.Object() , 0);
	scene.Object(mHost);
	wID = scene.GetWorld();

	// reset all rendering states
	FyResetRenderStates(wID);
	FySetD3DRenderState(wID, D3DRS_LIGHTING, FALSE);
	FySetD3DRenderState(wID, D3DRS_FOGENABLE, FALSE);
	FySetD3DRenderState(wID, D3DRS_ZWRITEENABLE, FALSE);

	// set current material
	FySetCurrentMaterial(wID, mRenderTarget, FALSE, 1.0f);

	FySetD3DRenderState(wID, D3DRS_ALPHABLENDENABLE, TRUE);
	FySetD3DRenderState(wID, D3DRS_ALPHATESTENABLE, FALSE);
	FySetD3DRenderState(wID, D3DRS_SRCBLEND, D3DBLEND_ZERO);
	FySetD3DRenderState(wID, D3DRS_DESTBLEND, D3DBLEND_SRCCOLOR);

	LPDIRECT3DDEVICE9 dev = FyGetD3DDevice(wID);
	float pos[16];


	FuLitedVertex v[3];

	// display these triangles
	vLen = 6;

	v[0].diffuse = D3DCOLOR_RGBA(255, 255, 255, 255);
	v[1].diffuse = D3DCOLOR_RGBA(255, 255, 255, 255);
	v[2].diffuse = D3DCOLOR_RGBA(255, 255, 255, 255);

	int nTri = m_triIDVec.size();
	for ( int i = 0; i < nTri ; i++) 
	{
		// get the triangle vertices
		tT.GetTopology( m_triIDVec[i] , tri);

		for ( int j = 0; j < 3; ++j ) 
		{
			tT.GetVertex(tri[2-j], pos);
			v[j].pos[0] = pos[0];
			v[j].pos[1] = pos[1];
			v[j].pos[2] = pos[2] + mShadowHeightOffset;

			// calculate the texture coordinate
			FUTransformVertexWithM16_Simple(vLC, v[j].pos, M);
			CalculateShadowUV(vLC, v[j].uv);
		}

		FyDrawTriangles(wID, XYZ_DIFFUSE, 3 , 1 , &texLen, (float *) &v[0] );

	}

}


/*---------------------------------------------
  calculate the shadow texture UV for vertices
  C.Wang 0524, 2006
 ----------------------------------------------*/
void FuCShadowModify::CalculateShadowUV(float *pos, float *uv)
{
   float w2 = mShadowSize*0.5f;
   uv[0] = (pos[0]/w2 + 1.0f)*0.5f;
   uv[1] = 1.0f - (pos[1]/w2 + 1.0f)*0.5f;
}


/*------------------
  render the shadow
  C.Wang 0522, 2006
 -------------------*/
void FuCShadowModify::Render()
{
	if ( !m_objVec.empty() )
	{
		FnViewport vp;
		vp.Object(mViewport);
		m_fyActor.GetWorldPosition( m_actorPos );
		m_lightCam.SetWorldPosition( m_actorPos + m_lightPos );
		vp.RenderShadow( m_lightCam.Object() , &m_objVec[0] , m_objVec.size() , mShadowMat, TRUE);
	}
}

void FuCShadowModify::fillActorObject( FnActor& actor )
{

	for ( int i = 0; i < actor.SkinNumber(); i++) 
	{
		m_objVec.push_back(  actor.GetSkin(i) );
	}

	for ( int i = 0; i < actor.AttachmentNumber(); i++) 
	{
		m_objVec.push_back(  actor.GetAttachment(i) );
	}

	int numPart = actor.SkeletonObjectNumber();
	for ( int i = 0; i < numPart; i++) 
	{
		if (actor.IsGeometry(i)) 
		{
			m_objVec.push_back(  actor.GetSkeletonObject(i) );
		}
	}
}