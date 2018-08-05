
#include "RenderGLStage.h"
#include "RenderGLStageAssetID.h"

#include "Thread.h"
#include "RHI/ShaderCompiler.h"
#include "RHI/RHICommand.h"
#include "SceneScript.h"

namespace RenderGL
{
	struct LoadDatalInfo
	{
		int  id;
		char const* name;
	};

	LoadDatalInfo gMaterialLists[] =
	{
#define DataOp( NAME ) { MaterialId::NAME , #NAME } ,
		MATERIAL_LIST(DataOp)
#undef DataOp
	};

	LoadDatalInfo gTextureLists[] =
	{
#define DataOp( NAME , PATH ) { TextureId::NAME , PATH } ,
		TEXTURE_LIST(DataOp)
#undef DataOp
	};

	LoadDatalInfo gMeshLists[] =
	{
#define DataOp( NAME , PATH ) { MeshId::NAME , PATH } ,
		MESH_LIST(DataOp)
#undef DataOp
	};

	bool SampleStage::loadAssetResouse()
	{

		mTextures.resize(ARRAY_SIZE(gTextureLists));
		mMeshs.resize(ARRAY_SIZE(gMeshLists));
		mMaterialAssets.resize(ARRAY_SIZE(gMaterialLists));

		typedef std::function< void(int id, StaticMesh* mesh, int section, OBJMaterialInfo const* mat) > MaterialBuildFun;
		class MaterialBuilder : public OBJMaterialBuildListener
		{
		public:
			virtual void build(int idxSection, OBJMaterialInfo const* mat) override
			{
				function(id, mesh, idxSection, mat);
			}
			MaterialBuildFun function;
			int id;
			StaticMesh* mesh;
		};

		
		MaterialBuildFun buildFun = [this](int id, StaticMesh* mesh, int section, OBJMaterialInfo const* matInfo)
		{
			if( matInfo == nullptr )
				return;

			auto textureLoadFun = [this]( OBJMaterialInfo const* matInfo , char const* textureDir ) -> Material*
			{
				char const* noramlTexture = matInfo->normalTextureName;
				if( noramlTexture == nullptr )
				{
					noramlTexture = matInfo->findParam("map_bump");
				}

				Material* mat = nullptr;
				if( noramlTexture )
				{
					mat = new MaterialInstance(getMaterial(MaterialId::Lightning));
					FixString< 256 > path = textureDir;
					path += noramlTexture;
					RHITexture2DRef tex = RHIUtility::LoadTexture2DFromFile(path);
					mat->setParameter(SHADER_PARAM(TextureB), *tex);
				}
				else
				{
					mat = new MaterialInstance( getMaterial(MaterialId::Havoc) );
				}
				
				if( matInfo->diffuseTextureName )
				{
					FixString< 256 > path = textureDir;
					path += matInfo->diffuseTextureName;
					RHITexture2DRef tex = RHIUtility::LoadTexture2DFromFile(path);
					mat->setParameter(SHADER_PARAM(TextureBase), *tex);
				}
				return mat;
			};

			if( id == MeshId::Lightning )
			{
				MaterialPtr mat{ textureLoadFun( matInfo, "Model/Lightning/") };
				mesh->setMaterial(section, mat);
			}
			else if( id == MeshId::Vanille )
			{
				MaterialPtr mat{ textureLoadFun( matInfo, "Model/Vanille/") };
				mesh->setMaterial(section, mat);
			}
			else if( id == MeshId::Havoc )
			{
				MaterialPtr mat{ textureLoadFun(matInfo, "Model/Havoc/") };
				mesh->setMaterial(section, mat);
			}
			else if( id == MeshId::Sponza )
			{
				MaterialPtr mat{ textureLoadFun(matInfo, "Model/") };
				mesh->setMaterial(section, mat);
			}
			else if( id == MeshId::Elephant )
			{
				MaterialPtr mat{ textureLoadFun(matInfo, "Model/Elephant/") };
				mesh->setMaterial(section, mat);
			}

		};

		class LoadAssetTask : public RunnableThreadT< LoadAssetTask >
		{
		public:
			unsigned run()
			{
				wglMakeCurrent(hDC, hLoadRC);
				loadingFun();
				return 0;
			}

			void exit() 
			{
				exitFun();
				wglMakeCurrent(nullptr, nullptr);
				wglDeleteContext(hLoadRC);

				delete this;
			}
			HDC   hDC;
			HGLRC hLoadRC;
			std::function< void() > loadingFun;
			std::function< void() > exitFun;
		};

		DrawEngine* de = ::Global::GetDrawEngine();
		HGLRC hRC = de->getGLContext()->getHandle();
		HGLRC hLoadRC = wglCreateContext(de->getWindow().getHDC());
		wglShareLists( hRC , hLoadRC );


		LoadAssetTask* loadingTask = new LoadAssetTask;
		mGpuSync.bUseFence = true;
		loadingTask->loadingFun = [this , buildFun]()
		{
			for( int i = 0; i < ARRAY_SIZE(gMaterialLists); ++i )
			{
				prevLoading();
				if( !mMaterialAssets[i].load(gMaterialLists[i].name) )
				{

				}
				::Global::GetAssetManager().registerAsset(&mMaterialAssets[i]);
				postLoading();
			}

			for( int i = 0; i < ARRAY_SIZE(gTextureLists); ++i )
			{
				prevLoading();
				if( i == TextureId::Terrain )
				{
					int j = 1;
				}
				if( mTextures[i].loadFile(gTextureLists[i].name) )
				{
				}
				postLoading();
			}

			Matrix4 MeshTransform[sizeof(gMeshLists) / sizeof(gMeshLists[0])];
			int* MeshSkip[sizeof(gMeshLists) / sizeof(gMeshLists[0])];
			for( int i = 0; i < ARRAY_SIZE(gMeshLists); ++i )
			{
				MeshTransform[i].setIdentity();
				MeshSkip[i] = nullptr;
			}
			Matrix4 FixRotation = Matrix4::Rotate(Vector3(1, 0, 0), Math::Deg2Rad(90));
			MeshTransform[MeshId::Dragon2] = Matrix4::Scale(0.05) * FixRotation  *  Matrix4::Translate(Vector3(0, 6, 0));
			MeshTransform[MeshId::Dragon] =  Matrix4::Scale(10) * FixRotation;
			MeshTransform[MeshId::Teapot] = Matrix4::Scale(0.05) * FixRotation;
			MeshTransform[MeshId::Elephant] = Matrix4::Scale(0.01) * FixRotation;
			MeshTransform[MeshId::Mario] = Matrix4::Scale(0.05) * FixRotation;
			MeshTransform[MeshId::Lightning] = Matrix4::Scale(0.03) *  FixRotation;
			MeshTransform[MeshId::Vanille] = Matrix4::Scale(0.03) *  FixRotation;
			MeshTransform[MeshId::Havoc] = Matrix4::Scale(0.015) *  FixRotation;
			MeshTransform[MeshId::Sponza] = Matrix4::Scale(0.05) *  FixRotation;
			MeshTransform[MeshId::Skeleton] = Matrix4::Scale(0.5) *  FixRotation;
			int skip[] = { 4 , 5 , -1 };
			int skeletonSkip[200];
			int n = 0;
			for( int i = 105; i <= 209; ++i )
				skeletonSkip[n++] = i;
			skeletonSkip[n++] = 212;
			skeletonSkip[n++] = 213;
			skeletonSkip[n++] = -1;
			MeshSkip[MeshId::Elephant] = skip;
			MeshSkip[MeshId::Skeleton] = skeletonSkip;

			MaterialBuilder materialBuilder;
			materialBuilder.function = buildFun;
			for( int i = 0; i < ARRAY_SIZE(gMeshLists); ++i )
			{

				if( i == MeshId::Sponza )
				{
					int a = 1;
				}
				prevLoading();
				StaticMesh* mesh{ new StaticMesh };
				materialBuilder.id = i;
				materialBuilder.mesh = mesh;
				if( MeshBuild::LoadObjectFile(*mesh, gMeshLists[i].name, &MeshTransform[i], &materialBuilder, MeshSkip[i]) )
				{
					mesh->postLoad();
					mesh->name = gMeshLists[i].name;
					mMeshs[i] = mesh;
					for( SceneAssetPtr& sceneAsset : mSceneAssets )
					{
						sceneAsset->scene.bNeedUpdatePrimitive = true;
					}
				}
				postLoading();
			}

		};
		loadingTask->exitFun = [this]()
		{
			ShaderManager::Get().registerShaderAssets( ::Global::GetAssetManager() );
			mGpuSync.bUseFence = false;
			mLoadingThread = nullptr;
		};
		loadingTask->hDC = de->getWindow().getHDC();
		loadingTask->hLoadRC = hLoadRC;
		mLoadingThread = loadingTask;
		loadingTask->start();


		return true;
	}

	void SampleStage::buildScene1(Scene& scene)
	{

		{
			auto meshObject = new StaticMeshObject;
			meshObject->meshPtr = getMesh(MeshId::Sponza);
			meshObject->worldTransform = Matrix4::Translate(Vector3(0, 0, 0));
			scene.addObject(meshObject);
		}

		return;

		{
			auto meshObject = new SimpleMeshObject;
			meshObject->mesh = &mSimpleMeshs[SimpleMeshId::SpherePlane];
			auto material = new MaterialInstance(getMaterial(MaterialId::Sphere));
			material->setParameter(SHADER_PARAM(Sphere.pos), Vector3(0, 20, 10));
			material->setParameter(SHADER_PARAM(Sphere.radius), 2.0f);
			meshObject->material.reset( material );
			meshObject->color = Vector4(1, 0, 0, 1);
			meshObject->worldTransform = Matrix4::Identity();
			scene.addObject(meshObject);
		}

		{
			auto meshObject = new SimpleMeshObject;
			meshObject->mesh = &mSimpleMeshs[SimpleMeshId::Box];
			auto material = new MaterialInstance(getMaterial(MaterialId::Simple1));
			meshObject->material.reset(material);
			meshObject->color = Vector4(0.3, 0.3, 1, 1);
			meshObject->worldTransform = Matrix4::Rotate(Vector3(1, 1, 1), Math::Deg2Rad(45)) * Matrix4::Translate(Vector3(-7, -6, 7));
			scene.addObject(meshObject);
		}

	}



	void SampleStage::renderScene(RenderContext& context)
	{

		ViewInfo const& viewInfo = context.getView();
		getScene(0).render(context);

		Matrix4 matWorld;

#if 1

#if 0
		matWorld = Matrix4::Identity();
		context.setWorld(matWorld);
		drawAxis(10);
#endif



		{
			Material* material = getMaterial(MaterialId::Mario);
			auto shader = context.setMaterial(material);
			if  (shader )
			{
				shader->setTexture(SHADER_PARAM(TextureD), getTexture(TextureId::MarioD).getRHI());
				shader->setTexture(SHADER_PARAM(TextureS), getTexture(TextureId::MarioS).getRHI());
			}
			Mesh& mesh = getMesh(MeshId::Mario);
			matWorld = Matrix4::Translate(Vector3(8, -8, 0));
			context.setWorld(matWorld);
			mesh.draw(LinearColor(0.7, 0.7, 0.7));
		}

		{
			//RHISetRasterizerState(TStaticRasterizerState< ECullMode::None >::GetRHI());
			Material* material = getMaterial(MaterialId::Simple3);
			auto shader = context.setMaterial(material);
			if( shader )
			{
				shader->setTexture(SHADER_PARAM(BaseTexture), getTexture(TextureId::Metel).getRHI());
			}
			matWorld = Matrix4::Translate(Vector3(-2.6, -2.6, 3));
			//matWorld = Matrix4::Translate(Vector3(0, 0, 7));
			context.setWorld(matWorld);
			mSimpleMeshs[SimpleMeshId::Sphere].draw( LinearColor(1,1,1) );
			//RHISetRasterizerState(TStaticRasterizerState<>::GetRHI());
		}
		{

			StaticMesh& mesh = getMesh(MeshId::Elephant);
			glColor3f(0.7, 0.7, 0.7);
			matWorld = Matrix4::Rotate(Vector3(0, 0, -1), Math::Deg2Rad(0)) * Matrix4::Translate(Vector3(6, 8, 4));
			mesh.render(matWorld, context);
		}



		if(  0 )
		{
			{
				StaticMesh& mesh = getMesh(MeshId::Sponza);
				matWorld = Matrix4::Translate(Vector3(0, 0, 0));
				mesh.render(matWorld, context);
			}


			{

				context.setMaterial(getMaterial(MaterialId::Simple1));
				matWorld = Matrix4::Rotate(Vector3(1, 1, 1), Math::Deg2Rad(45)) * Matrix4::Translate(Vector3(-7, -6, 7));
				context.setWorld(matWorld);
				mSimpleMeshs[SimpleMeshId::Box].draw(LinearColor(0.3, 0.3, 1));

			}

			{

				Material* material = getMaterial(MaterialId::Sphere);
				material->setParameter(SHADER_PARAM(Sphere.pos), Vector3(0, 10, 10));
				material->setParameter(SHADER_PARAM(Sphere.radius), 2.0f);
				context.setMaterial(material);
				context.setWorld(Matrix4::Identity());
				mSimpleMeshs[SimpleMeshId::SpherePlane].draw(LinearColor(1, 0, 0) );
			}
		}



		if( 1 )
		{
			Material* material = getMaterial(MaterialId::MetelA);
			auto shader = context.setMaterial(material);
			if( shader )
			{
				shader->setTexture(SHADER_PARAM(BaseTexture), getTexture(TextureId::Metel).getRHI());
			}
			Mesh& mesh = getMesh(MeshId::Dragon);
			matWorld = Matrix4::Rotate(Vector3(0, 0, 1), Math::Deg2Rad(45 + 180)) * Matrix4::Translate(Vector3(20, 0, 4));
			context.setWorld(matWorld);
			mesh.draw( Vector4(0.7, 0.7, 0.7, 1) );
		}

		if(1)
		{
			Material* material = getMaterial(MaterialId::POMTitle);
			auto shader = context.setMaterial(material);
			if (shader)
			{
				shader->setParam(SHADER_PARAM(DispFactor), Vector3(1, 0, 0));
#if 0
				shader->setTexture(SHADER_PARAM(BaseTexture), getTexture(TextureId::RocksD).getRHI());
				shader->setTexture(SHADER_PARAM(NoramlTexture), getTexture(TextureId::RocksNH).getRHI());
#else
				shader->setTexture(SHADER_PARAM(BaseTexture), getTexture(TextureId::Base).getRHI());
				shader->setTexture(SHADER_PARAM(NoramlTexture), getTexture(TextureId::Normal).getRHI());
#endif
			}
			matWorld = Matrix4::Scale(0.5) * Matrix4::Translate(Vector3(-12, 0, 2));
			context.setWorld(matWorld);
			mSimpleMeshs[SimpleMeshId::Plane].draw(LinearColor(0.7, 0.7, 0.7));
		}


		if( 1 )
		{
			Material* material = getMaterial(MaterialId::MetelA);
			auto shader = context.setMaterial(material);
			if( shader )
			{
				shader->setTexture(SHADER_PARAM(BaseTexture), getTexture(TextureId::Metel).getRHI());
			}
			Mesh& mesh = getMesh(MeshId::Dragon2);
			matWorld = Matrix4::Rotate(Vector3(0, 0, 1), Math::Deg2Rad(45 + 180)) * Matrix4::Translate(Vector3(6, -6, 4));
			context.setWorld(matWorld);
			mesh.draw(LinearColor(0.7, 0.7, 0.7,1.0) );
		}

		{
			StaticMesh& mesh = getMesh(MeshId::Havoc);
			glColor3f(0.7, 0.7, 0.7);
			matWorld = Matrix4::Rotate(Vector3(0, 0, 1), Math::Deg2Rad(85)) * Matrix4::Translate(Vector3(-35, -8, 12));
			mesh.render(matWorld, context);
		}

		{
			StaticMesh& mesh = getMesh(MeshId::Lightning);
			matWorld = Matrix4::Translate(Vector3(-5, -8, 3));
			mesh.render(matWorld , context);
		}

		if(1)
		{
			StaticMesh& mesh = getMesh(MeshId::Vanille);
			matWorld = Matrix4::Translate(Vector3(0, -8, 3));
			mesh.render(matWorld, context);
		}
	
		if( 1 )
		{
			Material* material = getMaterial(MaterialId::Simple1);
			auto shader = context.setMaterial(material);
			if ( shader )
			{
				shader->setTexture(SHADER_PARAM(BaseTexture), getTexture(TextureId::Metel).getRHI());
			}
			Mesh& mesh = getMesh(MeshId::Skeleton);
			matWorld = Matrix4::Translate(Vector3(0, -10, 5));
			context.setWorld(matWorld);
			mesh.draw(LinearColor(0.7, 0.7, 0.7));
		}


		context.setMaterial(getMaterial(MaterialId::Simple1));

		if( 0 )
		{
			matWorld = Matrix4::Rotate(Vector3(0, 0, -1), Math::Deg2Rad(45 + 180)) * Matrix4::Translate(Vector3(-6, 6, 4));
			Mesh& mesh = getMesh(MeshId::Teapot);
			context.setWorld(matWorld);
			mesh.draw(LinearColor(0.7, 0.7, 0.7, 1.0));
		}



		context.setMaterial(getMaterial(MaterialId::Simple1));

		matWorld = Matrix4::Translate(Vector3(-3, 3, 5));
		context.setWorld(matWorld);
		mSimpleMeshs[SimpleMeshId::Box].draw(LinearColor(0.3, 0.3, 1));


		matWorld = Matrix4::Scale(1) * Matrix4::Translate(Vector3(1, 3, 14));
		context.setWorld(matWorld);
		mSimpleMeshs[SimpleMeshId::Doughnut].draw(LinearColor(1, 1, 0));

		matWorld = Matrix4::Scale(1) * Matrix4::Translate(Vector3(6, 6, 5));
		context.setWorld(matWorld);
		mSimpleMeshs[SimpleMeshId::Box].draw(LinearColor(1, 1, 0));

		context.setMaterial(getMaterial((int)MaterialId::Simple2));

		matWorld = Matrix4::Rotate(Vector3(0.5, 1, 0), Math::Deg2Rad(-80)) * Matrix4::Scale(3) * Matrix4::Translate(Vector3(0.5 * 20, 0, 20));
		context.setWorld(matWorld);
		mSimpleMeshs[SimpleMeshId::Box].draw();
#endif
		context.setMaterial(getMaterial(MaterialId::Simple2));


		float scale = 2.5;
		float len = 10 * scale;
		
		if (0)
		{
			matWorld = Matrix4::Scale(10.0) * Matrix4::Translate(Vector3(0, 0, 0));
			context.setWorld(matWorld);
			mSimpleMeshs[SimpleMeshId::Plane].draw(LinearColor(1, 1, 1));

			matWorld = Matrix4::Rotate(Vector3(1, 0, 0), Math::Deg2Rad(90)) * Matrix4::Scale(scale) * Matrix4::Translate(Vector3(0, len, len));
			context.setWorld(matWorld);
			mSimpleMeshs[SimpleMeshId::Plane].draw(LinearColor(0.5 , 1 , 0 ));
	
			matWorld = Matrix4::Rotate(Vector3(1, 0, 0), Math::Deg2Rad(-90)) * Matrix4::Scale(scale) * Matrix4::Translate(Vector3(0, -len, len));
			context.setWorld(matWorld);
			mSimpleMeshs[SimpleMeshId::Plane].draw(LinearColor(1, 0.5, 1));
	
			matWorld = Matrix4::Rotate(Vector3(0, 1, 0), Math::Deg2Rad(-90)) * Matrix4::Scale(scale) * Matrix4::Translate(Vector3(len, 0, len));
			context.setWorld(matWorld);
			mSimpleMeshs[SimpleMeshId::Plane].draw(LinearColor(1, 1, 1));
	
	
			{
#if 0
				Material& material = getMaterial(MaterialId::POMTitle);
				auto shader = context.setMaterial(material);
				if ( shader )
				{
					shader->setParam(SHADER_PARAM(DispFactor), Vector3(-1, 1, 0));
#if 1
					shader->setTexture2D(SHADER_PARAM(BaseTexture), getTexture(TextureId::RocksD).getRHI());
					shader->setTexture2D(SHADER_PARAM(NoramlTexture), getTexture(TextureId::RocksNH).getRHI());
#else
					shader->setTexture2D(SHADER_PARAM(BaseTexture), getTexture(TextureId::Base).getRHI());
					shader->setTexture2D(SHADER_PARAM(NoramlTexture), getTexture(TextureId::Normal).getRHI());
#endif
				}
#endif
	
				matWorld = Matrix4::Rotate(Vector3(0, 1, 0), Math::Deg2Rad(90)) * Matrix4::Scale(scale) * Matrix4::Translate(Vector3(-len, 0, len));
				context.setWorld(matWorld);
				mSimpleMeshs[SimpleMeshId::Plane].draw(LinearColor(1, 1, 0.5) );
			}
		}

	}


	void AddRandomizeLight(std::vector< LightInfo >& lights , int num )
	{
		for( int i = 0; i < num; ++i )
		{
			LightInfo light;
			light.type = ( rand() % 2 ) ? LightType::Spot : LightType::Point;
			light.type = LightType::Point;
			light.pos = RandVector(Vector3(-68, -26, 0), Vector3(68, 26, 40));
			light.color = RandVector();
			light.intensity = RandFloat(50, 100);
			light.radius = RandFloat(10, 30);
			light.spotAngle.x = RandFloat(20, 70);
			light.spotAngle.y = Math::Min<float>( 88 , light.spotAngle.x + RandFloat(10, 30) );
			light.dir = RandDirection();
			//light.dir = Vector3(0,0,-1);
			light.bCastShadow = false;

			lights.push_back(light);

		}

	}
	void SampleStage::setupScene()
	{
		//addScene("SceneA");
		addScene();
		buildScene1( getScene(0) );

		int const RandLightNum = 0;
		mNumLightDraw = RandLightNum + 5;
		mTechShadow.mCascadeMaxDist = 400;

		static int const LightNum = 5;

		mLights.resize(LightNum);
		
		AddRandomizeLight(mLights, RandLightNum);
		
		Vector3 pos[LightNum] = { Vector3(0,0,0) , Vector3(0,0,0) , Vector3(0,0,0) , Vector3(0,0,0) , Vector3(-68 , 10 , 10 ) };
		Vector3 color[LightNum] = { Vector3(0.5,0.5,0.5) , Vector3(1,0.8,0) , Vector3(0.8,1,0) , Vector3(0.8,0.0,1) , Vector3(0.8,0.0,1) };
		float   intensity[LightNum] = { 10 , 300 , 300 , 500 , 300 };
		
		for( int i = 0; i < LightNum; ++i )
		{
			LightInfo& light = mLights[i];
			light.type = LightType::Point;
			light.pos = pos[i];
			light.color = color[i];
			light.intensity = intensity[i];
			light.radius = 20;
			//light.bCastShadow = true;
			light.dir = Vector3(-1, 1, -1);
		}

		if(0)
		{
			mLights[0].type = LightType::Directional;
			mLights[0].dir = Vector3(-1, 1, -1);
			mLights[0].intensity = 3;
			mLights[0].bCastShadow = false;
			mLights[0].dir = Vector3(1, 0, 0);
		}
		if(1)
		{
			LightInfo& light = mLights[0];
			light.type = LightType::Spot;
			light.dir = Vector3(0, 0.2, -1);
			light.spotAngle.x = 45;
			light.spotAngle.y = 45;
			light.radius = 20;
			light.pos = Vector3(0, 0, 10);
			light.color = Vector3(1, 1, 1);
			light.intensity = 400;
			light.bCastShadow = true;
		}

		{
			CycleTrack& track = mTracks[0];
			track.center = Vector3(0, 0, -20);
			track.radius = 1;
			track.period = 10;
			track.planeNormal = Vector3(0, 0, 1);
		}
		{
			CycleTrack& track = mTracks[1];
			track.center = Vector3(5, -5, 8);
			track.radius = 8;
			track.period = 10;
			track.planeNormal = Vector3(0.2, 0, 1);
		}
		{
			CycleTrack& track = mTracks[2];
			track.center = Vector3(5, 12, 8);
			track.radius = 8;
			track.period = 20;
			track.planeNormal = Vector3(0, -0.4, 1);
		}
		{
			CycleTrack& track = mTracks[3];
			track.center = Vector3(6, 5, 13);
			track.radius = 12;
			track.period = 25;
			track.planeNormal = Vector3(1, 1, 1);
		}
	}

}
