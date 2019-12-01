#include "SampleBase.h"

#include "CFPluginManager.h"
#include "MdlFileLoader.h"
#include "LoLFileLoader.h"

class AnimationUpdataVisitor : public SceneNodeVisitor
{
public:
	bool visit( SceneNode* node )
	{
		Object* obj = entity_cast< Object >( node );
		if ( obj )
			obj->updateAnimation( time );
		return true;
	}
	float time;
};

class ActorSample : public SampleBase
{
public:
	
	Light*       mainLight;
	Actor*       mFlyActor;
	Actor*       mTestActor;

	Object*   object;
	Camera*   cam;
	TexLayerAnimation* texAnim;

	std::vector< Actor* > mActors;

	static int const CubeMapSize = 512;

	ActorSample()
	{
		mFlyActor  = nullptr;
		mTestActor = nullptr;
		texAnim = nullptr;
	}

	struct Vertex
	{
		static VertexType const Type = CFVT_XYZ | CFVF_BLENDWEIGHTSIZE( 2 ) | CFV_BLENDINDICES | CFV_COLOR | CFVF_TEX1(2);
		Vector3 pos;
		float   weight[2];
		uint32  boneIndices;
		Vector3 color;
		float   tex[2];
	};
	bool onSetupSample()
	{
		PluginManager::Get().registerLinker( "LoL" , new LoLFileLinker );
		AnimationState* state;

		mMainViewport->setBackgroundColor( 0.5 , 0.5 , 0.5 );

		mainLight = mMainScene->createLight( nullptr );
		mainLight->registerName( "mainLight" );
		mainLight->translate( Vector3(0,0,20) , CFTO_LOCAL );
		mainLight->setLightColor( Color4f(1,1,1) );

		mMainCamera->setLookAt( Vector3(100,100,100) , Vector3(0,0,0) , Vector3(0,0,1) );


		{
			Quaternion q;
			q.setRotation( Vector3(1,2,3) , Math::Deg2Rad(10) );

			Quaternion qq;
			qq.setRotation( Vector3(2,1,1) , Math::Deg2Rad(20) );

			Quaternion qqq;
			q.setRotation( Vector3(3,1,3) , Math::Deg2Rad(50) );

			Quaternion qq1 = q * qq;
			Quaternion qq2 = qq * q;
			
			Matrix4 mq  = Matrix4::Rotate( q ) * Matrix4::Rotate( qq );
			Matrix4 mq1 = Matrix4::Rotate( qq1 );
			Matrix4 mq2 = Matrix4::Rotate( qq2 );

			Matrix4 m;
			m.setRotation( Vector3(1,2,3) , Math::Deg2Rad(90) );
			Matrix4 m2;
			m2.setQuaternion( q );

			Quaternion q1;
			q1.setMatrix( m );

			Vector3 v( 1 , 0 , 0 );
			Vector3 v1 = q.rotate( v );
			Vector3 v2 = v * m;


			int i = 1;
		}
		{
			mTestActor = mMainScene->createActor();
			mTestActor->addFlag( Actor::NODE_DISABLE_SKELETON );
			Skeleton* skeleton = mTestActor->getSkeleton();

			Object* skin = mTestActor->createSkin();
			Material* mat = mWorld->createMaterial();
			mat->addTexture( 0 , 0 , "bg1" );

			Vertex v[4] =
			{
				{ Vector3(0,0,0) , 1 , 0 , 0x00000201 , Vector3(1,0,0) , 0 , 0 } ,
				{ Vector3(0,0,0) , 0 , 1 , 0x00000201 , Vector3(0,1,0) , 1 , 0 } ,
				{ Vector3(0,0,0) , 1 , 0 , 0x00000403 , Vector3(1,1,1) , 1 , 1 } ,
				{ Vector3(0,0,0) , 0 , 1 , 0x00000403 , Vector3(0,0,1) , 0 , 1 } ,
			};
			int index[6] = { 0 , 1 , 2 , 0 , 2 , 3 };

			MeshInfo info;
			info.vertexType    = Vertex::Type;
			info.primitiveType = CFPT_TRIANGLELIST;
			info.pVertex       = &v[0];
			info.numVertices   = 4;
			info.pIndex        = &index[0];
			info.numIndices    = 6;
			info.isIntIndexType= true;
			info.flag          = CFVD_SOFTWARE_PROCESSING;
			skin->createMesh( mat , info );
			
			{
				mTestActor->setBoneTransSize( 5 );
				Matrix4* mat = mTestActor->getBoneTransMatrix();
				mat[0].setIdentity();
				mat[1].setTranslation( Vector3( 0 , 0 , 0 ) );
				mat[2].setTranslation( Vector3( 100 , 0 , 0 ) );
				mat[3].setTranslation( Vector3( 100 , 100 , 0 ) );
				mat[4].setTranslation( Vector3( 0 , 100 , 0 ) );
			}

		}

		//{
		//	cam = mMainScene->createCamera( NULL );
		//	cam->setAspect( float(  g_ScreenWidth ) / g_ScreenHeight );
		//	cam->setNear( 50.0f );
		//	cam->setFar( 3000.0f );
		//	cam->translate( CFTO_GLOBAL , Vector3(0,0,100) );
		//	cam->rotate( CFTO_LOCAL , CF_AXIS_Y , Math::Deg2Rad( 180 ) );

		//	IObject* obj = mMainScene->createObject( cam );
		//	obj->setRenderMode( CFRM_WIREFRAME );
		//	cam->createFrustumGeom( obj );

		//	mMainCamera->setClipFrustum( cam );
		//}

		{
			mWorld->setDir( DIR_ACTOR , "../Data/LoL" );
			PluginManager::Get().setLoaderMeta( DATA_LOL_INDEX_MODEL_SKIN , 0 );
			Actor* actor = mMainScene->createActorFromFile( "Sona" );

			if ( actor )
			{
				//actor->addFlag( Actor::CFNF_DISABLE_SKELETON );
				//actor->setBoneTransSize( 255 );
				//Matrix4* mat = actor->getBoneTransMatrix();
				//for ( int i = 0 ; i < 255 ; ++i )
				//{
				//	mat[i].setTranslate( Vector3( 0 , 50 , 0 ) );

				//}
				actor->setWorldPosition( Vector3(0,100,0) );
				float s = 0.45;
				actor->scale( Vector3(s,s,s) , CFTO_LOCAL );
				actor->rotate( CF_AXIS_X , Math::Deg2Rad( 90 ) , CFTO_LOCAL );
				actor->rotate( CF_AXIS_Y , Math::Deg2Rad( 180 ) , CFTO_LOCAL );

				state = actor->getAnimationState( "idle1" );
				if ( state )
				{
					state->setWeight(1.0);
					state->enableLoop( true );
					state->enable( true );
				}

				mActors.push_back( actor );
			}
			
		}

		//mWorld->setDir( DIR_ACTOR , "../Data/NPC" );
		//mFlyActor = mMainScene->createActorFromFile( "angel.cwc" );
		mWorld->setDir( DIR_ACTOR , "../Data/NPC/hero" );
		mFlyActor = mMainScene->createActorFromFile( "hero.cwc" );
		if ( mFlyActor )
		{
			mFlyActor->setWorldPosition( Vector3(100,100,0) );

			state = mFlyActor->getAnimationState( "2" );
			state->setWeight(1.0);
			state->enableLoop( true );
			state->enable( true );

			mActors.push_back( mFlyActor );
		}

		//mWorld->setDir( DIR_ACTOR , "../Data/NPC" );

		//IObject* obj = mMainScene->createObject( nullptr );
		//obj->load( "box" );

		{
			MdlFileImport loader( mMainScene );
			Actor* actor = loader.loadActor( "../Data/HL/gsg9.mdl" );
			if ( actor )
			{
				//Object* weapon = loader.loadObject( "../Data/HL/w_mp5.mdl" );

				//weapon->translate( Vector3(10.855000 ,-0.41671500 , 1.8706800 ) , CFTO_LOCAL );
				//weapon->rotate( CFTO_LOCAL , CF_AXIS_Z , Math::Deg2Rad(180) );
				//state = mHLActor->getAnimationState("ref_shoot_ak47");
				//actor->setWorldPosition( Vector3(100,200,0) );

				//BoneNode* bone = actor->getSkeleton()->getBone( "Bip01 R Hand" );
				//BoneNode* bone = actor->getSkeleton()->getBone( 27 );
				actor->setWorldPosition( Vector3(-100,100,50) );
				//actor->applyAttachment( weapon , 27 , CFTO_GLOBAL );
				//state = actor->getAnimationState("ref_shoot_mp5");
				state = actor->getAnimationState("idle1");
				if ( state )
				{
					state->setWeight(1.0);
					state->enableLoop( true );
					state->enable( true );
				}
				mActors.push_back( actor );
			}
		}

		
		object = mMainScene->createObject( nullptr );
		object->load( "box.cw3" );
		object->translate( Vector3(100,0,0) , CFTO_LOCAL );


		//IObject* cObj = object->instance( true );
		//cObj->translate( CFTO_LOCAL , Vector3(100,0,0));

		mWorld->setDir( DIR_SCENE , "../Data/BT_LOWER" );
		mMainScene->load( "BT_LOWER" );


		mMainViewport->setBackgroundColor( 0.3 , 0.3 ,0.3 );

		{
#if 0
			Object* plane = mMainScene->createObject();
			Material* mat = mWorld->createMaterial();
			mat->addTexture( 0 , 0 , "bg1" );
			plane->createPlane( mat , 200 , 200 );
			plane->translate( Vector3(100,200,0) , CFTO_LOCAL );
			plane->setRenderOption( CFRO_CULL_FACE , CF_CULL_NONE );
#endif

			//plane->setRenderMode( CFRM_WIREFRAME );

		}

		{
			BillBoard* plane = mMainScene->createBillBoard();
			plane->setBoardType( BillBoard::BT_CYLINDICAL_Z );
			Material* mat = mWorld->createMaterial();

			mat->addSenquenceTexture( 0 , "Texture/C_thounder%04d" , 27 );
			texAnim = mat->createSlotAnimation( 0 );
			//texAnim = mat->createUVAnimation( 0 , 0.001 , 0.001 );
			//mat->addTexture( 0 , 0 , "Texture/C_thounder0001" );
			//texAnim = new IUVTexAnimation( &mat->getTextureLayer(0) , 0.1 , 0.1 );
			//texAnim = new ISlotTextureAnimation( mat , 0 , "Texture/C_thounder%04d" , 27  );

			plane->createPlane( mat , 200 , 200 );
			plane->setRenderOption( CFRO_ALPHA_BLENGING , true );
			plane->setRenderOption( CFRO_CULL_FACE , CF_CULL_CCW );
			plane->setRenderOption( CFRO_Z_BUFFER_WRITE , false );
			plane->translate( Vector3(100,200,200) , CFTO_LOCAL );
			plane->setRenderOption( CFRO_SRC_BLEND , CF_BLEND_ONE );
			plane->setRenderOption( CFRO_DEST_BLEND , CF_BLEND_ONE );
		}

		createCoorditeAxis( 10 );

		return true; 
	}

	 void onExitSample()
	 {

	 }

	long onUpdate( long time )
	{
		static float angle = 0.0f;

		SampleBase::onUpdate( time );
		//mainLight->setLocalPosition( 
		//	Vector3( 50 * Math::Cos( angle ) , 50 * Math::Sin( angle ) , 50 ) );

		AnimationUpdataVisitor visitor;
		visitor.time = 30 * time / 1000.0f ;
		//mMainScene->visitSceneNode( visitor );

		if ( mTestActor )
		{
			Matrix4* mat = mTestActor->getBoneTransMatrix();
			mat[1].setTranslation( Vector3( 0 , 0 , 10 * sin( angle  ) ) );
			mat[2].setTranslation( Vector3( 100 , 0 , 10 * sin( 2 *angle  ) ) );
			mat[3].setTranslation( Vector3( 100 , 100 , 10 * sin( 3 *angle ) ) );
			mat[4].setTranslation( Vector3( 0 , 100 , 10 * sin( 4 *angle ) ) );
		}

		for( int i = 0 ; i < mActors.size() ; ++i )
		{
			mActors[i]->updateAnimation( 30 * time / 1000.0f );
		}


		if ( texAnim )
			texAnim->updateAnimation( 30 * time / 1000.0f );
		angle += 0.001 * time;

		return time;
	}

	bool handleKeyEvent( unsigned key , bool isDown )
	{
		if ( !isDown )
			return false;

		switch( key )
		{
		case Keyboard::eUP:    cam->translate( Vector3( 0,0,10 ) , CFTO_LOCAL ); break;
		case Keyboard::eDOWN:  cam->translate( Vector3( 0,0,-10 ) , CFTO_LOCAL); break;
		case Keyboard::eLEFT:  cam->translate( Vector3( -10 ,0 , 0 ) , CFTO_LOCAL ); break;
		case Keyboard::eRIGHT: cam->translate( Vector3( 10 ,0 , 0 ) , CFTO_LOCAL ); break;
		case Keyboard::eP: std::swap( mMainCamera , cam );break;
		case Keyboard::eO: g_useObjFrushumClip = !g_useObjFrushumClip; break;
		default:
			SampleBase::handleKeyEvent( key , isDown );
		}
		return true;
	}


	void onRenderScene()
	{
		//mMainScene->render2D( mMainViewport );
		SampleBase::onRenderScene();
	}

	void onShowMessage()
	{
		SampleBase::onShowMessage();
		//Vector3 pos = mMainCamera->getWorldPosition();
		if ( mFlyActor )
		{
			Vector3 pos = mFlyActor->getWorldPosition();
			//pushMessage( "tri num = %d" , g_totalRenderTriangleNumber );
			pushMessage( "cam pos = %f %f %f" , pos.x , pos.y , pos.z );
		}
		pushMessage( "enable cam clip = %s" , g_useObjFrushumClip ? "true" : "false" );

	}


};

INVOKE_SAMPLE( ActorSample )
