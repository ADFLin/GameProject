
#include "RenderGLStage.h"
#include "RenderGLStageAssetID.h"

#include "Thread.h"

namespace RenderGL
{



	struct LoadDatalInfo
	{
		int  id;
		char const* name;
	};

	LoadDatalInfo gMaterialLists[] =
	{
#define DataOp( NAME ) { MaterialId::##NAME , #NAME } ,
		MATERIAL_LIST(DataOp)
#undef DataOp
	};

	LoadDatalInfo gTextureLists[] =
	{
#define DataOp( NAME , PATH ) { TextureId::##NAME , PATH } ,
		TEXTURE_LIST(DataOp)
#undef DataOp
	};

	LoadDatalInfo gMeshLists[] =
	{
#define DataOp( NAME , PATH ) { MeshId::##NAME , PATH } ,
		MESH_LIST(DataOp)
#undef DataOp
	};

	bool SampleStage::loadAssetResouse()
	{
		if( !InitGlobalRHIResource() )
			return false;

		mTextures.resize(ARRAY_SIZE(gTextureLists));
		mMeshs.resize(ARRAY_SIZE(gMeshLists));
		mMaterials.resize(ARRAY_SIZE(gMaterialLists));

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
					RHITexture2D* tex = RHICreateTexture2D();
					tex->loadFile(path);
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
					RHITexture2D* tex = RHICreateTexture2D();
					tex->loadFile(path);
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

		DrawEngine* de = ::Global::getDrawEngine();
		HGLRC hRC = de->getGLContext().getHandle();
		HGLRC hLoadRC = wglCreateContext(de->getWindow().getHDC());
		wglShareLists( hRC , hLoadRC );


		LoadAssetTask* loadingTask = new LoadAssetTask;
		mGpuSync.bUseFence = true;
		loadingTask->loadingFun = [this , buildFun]()
		{
			for( int i = 0; i < ARRAY_SIZE(gMaterialLists); ++i )
			{
				prevLoading();
				if( !mMaterials[i].loadFile(gMaterialLists[i].name) )
				{

				}
				postLoading();
			}

			for( int i = 0; i < ARRAY_SIZE(gTextureLists); ++i )
			{
				prevLoading();
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
				StaticMeshPtr mesh{ new StaticMesh };
				materialBuilder.id = i;
				materialBuilder.mesh = mesh.get();
				if( MeshUtility::createFromObjectFile(*mesh, gMeshLists[i].name, &MeshTransform[i], &materialBuilder, MeshSkip[i]) )
				{
					mesh->postLoad();
					mesh->name = gMeshLists[i].name;
					mMeshs[i] = mesh;
				}
				postLoading();
			}

		};
		loadingTask->exitFun = [this]()
		{
			mGpuSync.bUseFence = false;
		};
		loadingTask->hDC = de->getWindow().getHDC();
		loadingTask->hLoadRC = hLoadRC;
		loadingTask->start();


		return true;
	}

	void SampleStage::renderScene(RenderParam& param)
	{
		Matrix4 matWorld;

#if 1
		glPushMatrix();
		matWorld = Matrix4::Identity();
		param.setWorld(matWorld);
		//drawAxis(10);
		glPopMatrix();


		{
			//glDisable(GL_CULL_FACE);
			Material* material = getMaterial(MaterialId::Simple3);
			param.setMaterial(material);
			param.setMaterialParameter(SHADER_PARAM(BaseTexture), getTexture(TextureId::Metel));
			glPushMatrix();
			matWorld = Matrix4::Translate(Vector3(-2.6, -2.6, 3));
			//matWorld = Matrix4::Translate(Vector3(0, 0, 7));
			param.setWorld(matWorld);
			glColor3f(1, 1, 1);
			mSphereMesh.draw();
			glPopMatrix();
			glEnable(GL_CULL_FACE);
		}


		if (1)
		{

			Material* material = getMaterial(MaterialId::Sphere);
			material->setParameter(SHADER_PARAM(Sphere.pos), Vector3(0, 10 , 10));
			material->setParameter(SHADER_PARAM(Sphere.radius), 2.0f );
			
			param.setMaterial(material);
			glPushMatrix();
			glColor3f(1, 0, 0);
			mSpherePlane.draw();
			glPopMatrix();
		}

		

		{
			StaticMesh& mesh = getMesh(MeshId::Sponza);
			glColor3f(1, 1, 1);
			matWorld = Matrix4::Translate(Vector3(0, 0, 0));
			mesh.render(matWorld, param );
		}

		return;
		if( 1 )
		{
			Material* material = getMaterial(MaterialId::MetelA);
			param.setMaterial(material);
			param.setMaterialParameter(SHADER_PARAM(BaseTexture), getTexture(TextureId::Metel));
			Mesh& mesh = getMesh(MeshId::Dragon);
			glPushMatrix();
			matWorld = Matrix4::Rotate(Vector3(0, 0, 1), Math::Deg2Rad(45 + 180)) * Matrix4::Translate(Vector3(20, 0, 4));
			param.setWorld(matWorld);
			glColor3f(0.7, 0.7, 0.7);
			mesh.draw();
			glPopMatrix();
		}

		if(1)
		{
			Material* material = getMaterial(MaterialId::POMTitle);
			param.setMaterial(material);
			param.setMaterialParameter(SHADER_PARAM(DispFactor), Vector3(1, 0, 0));
#if 0
			param.setMaterialParameter(SHADER_PARAM(BaseTexture), getTexture(TextureId::RocksD));
			param.setMaterialParameter(SHADER_PARAM(NoramlTexture), getTexture(TextureId::RocksNH));
#else
			param.setMaterialParameter(SHADER_PARAM(BaseTexture), getTexture(TextureId::Base));
			param.setMaterialParameter(SHADER_PARAM(NoramlTexture), getTexture(TextureId::Normal));
#endif
			glPushMatrix();
			matWorld = Matrix4::Scale(0.5) * Matrix4::Translate(Vector3(-12, 0, 2));
			param.setWorld(matWorld);
			glColor3f(0.7, 0.7, 0.7);
			mPlane.draw();
			glPopMatrix();
		}


		{
			Material* material = getMaterial(MaterialId::Mario);
			param.setMaterial(material);
			param.setMaterialParameter(SHADER_PARAM(TextureD), getTexture(TextureId::MarioD));
			param.setMaterialParameter(SHADER_PARAM(TextureS), getTexture(TextureId::MarioS));
			Mesh& mesh = getMesh(MeshId::Mario);
			glPushMatrix();
			matWorld = Matrix4::Translate(Vector3(8, -8, 0));
			param.setWorld(matWorld);
			glColor3f(0.7, 0.7, 0.7);
			mesh.draw();
			glPopMatrix();
		}

		if( 1 )
		{
			Material* material = getMaterial(MaterialId::MetelA);
			param.setMaterial(material);
			param.setMaterialParameter(SHADER_PARAM(BaseTexture), getTexture(TextureId::Metel));
			Mesh& mesh = getMesh(MeshId::Dragon2);
			glPushMatrix();
			matWorld = Matrix4::Rotate(Vector3(0, 0, 1), Math::Deg2Rad(45 + 180)) * Matrix4::Translate(Vector3(6, -6, 4));
			param.setWorld(matWorld);
			glColor3f(0.7, 0.7, 0.7);
			mesh.draw();
			glPopMatrix();
		}

		{
			StaticMesh& mesh = getMesh(MeshId::Havoc);
			glColor3f(0.7, 0.7, 0.7);
			matWorld = Matrix4::Rotate(Vector3(0, 0, 1), Math::Deg2Rad(85)) * Matrix4::Translate(Vector3(-35, -8, 12));
			mesh.render(matWorld, param);
		}

		{
			StaticMesh& mesh = getMesh(MeshId::Lightning);
			glColor3f(0.7, 0.7, 0.7);
			matWorld = Matrix4::Translate(Vector3(-5, -8, 3));
			mesh.render(matWorld , param);
		}

		if(1)
		{
			StaticMesh& mesh = getMesh(MeshId::Vanille);
			glColor3f(0.7, 0.7, 0.7);
			matWorld = Matrix4::Translate(Vector3(0, -8, 3));
			mesh.render(matWorld, param);
		}
	
		if( 1 )
		{
			Material* material = getMaterial(MaterialId::Simple1);
			param.setMaterial(material);
			param.setMaterialParameter(SHADER_PARAM(BaseTexture), getTexture(TextureId::Metel));
			Mesh& mesh = getMesh(MeshId::Skeleton);
			glPushMatrix();
			matWorld = Matrix4::Translate(Vector3(0, -10, 5));
			param.setWorld(matWorld);
			glColor3f(0.7, 0.7, 0.7);
			mesh.draw();
			glPopMatrix();
		}


		param.setMaterial(getMaterial(MaterialId::Simple1));

		if( 1 )
		{
			glPushMatrix();
			matWorld = Matrix4::Rotate(Vector3(0, 0, -1), Math::Deg2Rad(45 + 180)) * Matrix4::Translate(Vector3(-6, 6, 4));
			Mesh& mesh = getMesh(MeshId::Teapot);
			param.setWorld(matWorld);
			glColor3f(0.7, 0.7, 0.7);
			mesh.draw();
			glPopMatrix();
		}

		{

			StaticMesh& mesh = getMesh(MeshId::Elephant);
			glColor3f(0.7, 0.7, 0.7);
			matWorld = Matrix4::Rotate(Vector3(0, 0, -1), Math::Deg2Rad(0)) * Matrix4::Translate(Vector3(6, 8, 4));
			mesh.render(matWorld, param);
		}

		param.setMaterial(getMaterial(MaterialId::Simple1));
		glPushMatrix();
		matWorld = Matrix4::Rotate(Vector3(1, 1, 1), Math::Deg2Rad(45)) * Matrix4::Translate(Vector3(-7, -6, 7));
		param.setWorld(matWorld);
		glColor3f(0.3, 0.3, 1);
		mBoxMesh.draw();
		glPopMatrix();

		glPushMatrix();
		matWorld = Matrix4::Translate(Vector3(-3, 3, 5));
		param.setWorld(matWorld);
		glColor3f(0.3, 0.3, 1);
		mBoxMesh.draw();
		glPopMatrix();

		glPushMatrix();
		matWorld = Matrix4::Scale(1) * Matrix4::Translate(Vector3(1, 3, 14));
		param.setWorld(matWorld);
		glColor3f(1, 1, 0);
		mDoughnutMesh.draw();
		glPopMatrix();

		glPushMatrix();
		matWorld = Matrix4::Scale(1) * Matrix4::Translate(Vector3(6, 6, 5));
		param.setWorld(matWorld);
		glColor3f(1, 1, 0);
		mBoxMesh.draw();
		glPopMatrix();

		param.setMaterial(getMaterial((int)MaterialId::Simple2));

		glPushMatrix();
		matWorld = Matrix4::Rotate(Vector3(0.5, 1, 0), Math::Deg2Rad(-80)) * Matrix4::Scale(3) * Matrix4::Translate(Vector3(0.5 * 20, 0, 20));
		param.setWorld(matWorld);
		glColor3f(1, 1, 1);
		mBoxMesh.draw();
		glPopMatrix();
#endif
		param.setMaterial(getMaterial(MaterialId::Simple2));


		float scale = 2.5;
		float len = 10 * scale;
		
		if (0)
		{
			glPushMatrix();
			matWorld = Matrix4::Scale(10.0) * Matrix4::Translate(Vector3(0, 0, 0));
			param.setWorld(matWorld);
			glColor3f(1, 1, 1);
			mPlane.draw();
			glPopMatrix();

			glPushMatrix();
			matWorld = Matrix4::Rotate(Vector3(1, 0, 0), Math::Deg2Rad(90)) * Matrix4::Scale(scale) * Matrix4::Translate(Vector3(0, len, len));
			param.setWorld(matWorld);
			glColor3f(0.5, 1, 0);
			mPlane.draw();
			glPopMatrix();
	
			glPushMatrix();
			matWorld = Matrix4::Rotate(Vector3(1, 0, 0), Math::Deg2Rad(-90)) * Matrix4::Scale(scale) * Matrix4::Translate(Vector3(0, -len, len));
			param.setWorld(matWorld);
			glColor3f(1, 0.5, 1);
			mPlane.draw();
			glPopMatrix();
	
			glPushMatrix();
			matWorld = Matrix4::Rotate(Vector3(0, 1, 0), Math::Deg2Rad(-90)) * Matrix4::Scale(scale) * Matrix4::Translate(Vector3(len, 0, len));
			param.setWorld(matWorld);
			glColor3f(1, 1, 1);
			mPlane.draw();
			glPopMatrix();
	
	
			{
	#if 0
				Material& material = getMaterial(MaterialId::POMTitle);
				param.setMaterial(material);
				param.setMaterialParameter(SHADER_PARAM(DispFactor), Vector3(-1, 1, 0));
	#if 1
				param.setMaterialParameter(SHADER_PARAM(BaseTexture), getTexture(TextureId::RocksD));
				param.setMaterialParameter(SHADER_PARAM(NoramlTexture), getTexture(TextureId::RocksNH));
	#else
				param.setMaterialParameter(SHADER_PARAM(BaseTexture), getTexture(TextureId::Base));
				param.setMaterialParameter(SHADER_PARAM(NoramlTexture), getTexture(TextureId::Normal));
	#endif
	#endif
	
				glPushMatrix();
				matWorld = Matrix4::Rotate(Vector3(0, 1, 0), Math::Deg2Rad(90)) * Matrix4::Scale(scale) * Matrix4::Translate(Vector3(-len, 0, len));
				param.setWorld(matWorld);
				glColor3f(1, 1, 0.5);
				mPlane.draw();
				glPopMatrix();
			}
		}




		//glPushMatrix();
		//mProgSphere.bind();
		//mProgSphere.setParam( "sphere.radius" , 2.0f );
		//mProgSphere.setParam( "sphere.localPos" , Vector3(5,5,5) );
		//mSpherePlane.draw();
		//mProgSphere.unbind();
		//glPopMatrix();

		//glFlush();
	}

	float RandFloat()
	{
		return float(::rand()) / RAND_MAX;
	}
	float RandFloat(float min , float max)
	{
		return min + ( max - min ) * float(::rand()) / RAND_MAX;
	}
	Vector3 RandVector(Vector3 const& min , Vector3 const& max )
	{
		return Vector3(RandFloat(min.x , max.x), RandFloat(min.y, max.y), RandFloat(min.z, max.z));
	}
	Vector3 RandVector()
	{
		return Vector3(RandFloat(), RandFloat(), RandFloat());
	}

	Vector3 RandDirection()
	{
		float s1, c1;
		Math::SinCos(Math::PI * RandFloat(), s1, c1);
		float s2, c2;
		Math::SinCos(2 * Math::PI * RandFloat(), s2, c2);
		return Vector3( s1 * c2 , s1 * s2 , c1 );
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
			light.spotAngle.y = Math::Min( 88 , light.spotAngle.x + RandFloat(10, 30) );
			light.dir = RandDirection();
			//light.dir = Vector3(0,0,-1);
			light.bCastShadow = false;

			lights.push_back(light);

		}

	}
	void SampleStage::setupScene()
	{
		int const RandLightNum = 0;
		mNumLightDraw = RandLightNum + 1;
		mShadowTech.mCascadeMaxDist = 400;

		static int const LightNum = 5;

		mLights.resize(LightNum);
		
		AddRandomizeLight(mLights, RandLightNum);
		
		Vector3 pos[LightNum] = { Vector3(0,0,0) , Vector3(0,0,0) , Vector3(0,0,0) , Vector3(0,0,0) , Vector3(-58 , 10 , 10 ) };
		Vector3 color[LightNum] = { Vector3(0.5,0.5,0.5) , Vector3(1,0.8,0) , Vector3(0.8,1,0) , Vector3(0.8,0.0,1) , Vector3(0.8,0.0,1) };
		float   intensity[LightNum] = { 10 , 300 , 300 , 500 , 300 };
		
		for( int i = 0; i < LightNum; ++i )
		{
			LightInfo& light = mLights[i];
			light.type = LightType::Point;
			light.pos = pos[i];
			light.color = color[i];
			light.intensity = intensity[i];
			light.radius = 10;
			light.bCastShadow = false;
			light.dir = Vector3(-1, 1, -1);
		}

		mLights[0].type = LightType::Directional;
		mLights[0].dir = Vector3(-1, 1, -1);
		mLights[0].intensity = 3;
		mLights[0].bCastShadow = false;
		mLights[0].dir = Vector3(1, 0, 0);
		if(0)
		{
			LightInfo& light = mLights[1];
			light.type = LightType::Spot;
			light.dir = Vector3(0, 0.2, -1);
			light.spotAngle.x = 45;
			light.spotAngle.y = 45;
			light.pos = Vector3(0, 0, 10);
			light.color = Vector3(1, 1, 1);
			light.intensity = 400;
			//light.bCastShadow = true;
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
