
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
		if( !mEmptyTexture.loadFile("Texture/Gird.png") )
			return false;
		if( !mEmptyMaterial.loadFile("EmptyMaterial") )
			return false;
		mTextures.resize(ARRAY_SIZE(gTextureLists));
		mMeshs.resize(ARRAY_SIZE(gMeshLists));
		mMaterials.resize(ARRAY_SIZE(gMaterialLists));

		class LoadAssetTask : public RunnableThreadT< LoadAssetTask >
		{
		public:
			void prevLoad()
			{
				while( !GpuSync->pervLoading() )
				{
					::Sleep(1);
				}
			}
			void postLoad()
			{
				GpuSync->postLoading();
			}
			unsigned run()
			{
				wglMakeCurrent(hDC, hLoadRC);
				
				for( int i = 0; i < ARRAY_SIZE(gMaterialLists); ++i )
				{
					prevLoad();
					MaterialPtr mat(new Material);
					if( mat->loadFile(gMaterialLists[i].name) )
						(*Materials)[i] = mat;
					postLoad();
				}

				for( int i = 0; i < ARRAY_SIZE(gTextureLists); ++i )
				{
					prevLoad();
					Texture2DPtr tex(new Texture2DRHI);
					if( tex->loadFile(gTextureLists[i].name) )
					{
						(*Textures)[i] = tex;
					}
					postLoad();
				}
				
				Matrix4 MeshTransform[sizeof(gMeshLists) / sizeof(gMeshLists[0])];
				int* MeshSkip[sizeof(gMeshLists) / sizeof(gMeshLists[0])];
				for( int i = 0; i < ARRAY_SIZE(gMeshLists); ++i )
				{
					MeshTransform[i].setIdentity();
					MeshSkip[i] = nullptr;
				}
				Matrix4 FixRotation = Matrix4::Rotate(Vector3(1, 0, 0), Math::Deg2Rad(90));
				MeshTransform[MeshId::Dragon] = Matrix4::Scale(0.05) * FixRotation  *  Matrix4::Translate(Vector3(0, 6, 0));
				MeshTransform[MeshId::Teapot] = FixRotation;
				MeshTransform[MeshId::Elephant] = Matrix4::Scale(0.01) * FixRotation;
				MeshTransform[MeshId::Mario] = Matrix4::Scale(0.05) * FixRotation;
				MeshTransform[MeshId::Lightning] = Matrix4::Scale(0.03) *  FixRotation;
				MeshTransform[MeshId::Vanille] = Matrix4::Scale(0.03) *  FixRotation;
				MeshTransform[MeshId::Havoc] = Matrix4::Scale(0.03) *  FixRotation;
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
				for( int i = 0; i < ARRAY_SIZE(gMeshLists); ++i )
				{
					if( i == MeshId::Havoc)
					{
						int a = 1;
					}
					prevLoad();
					MeshPtr mesh{ new Mesh };
					if( MeshUtility::createFromObjectFile(*mesh, gMeshLists[i].name, &MeshTransform[i], MeshSkip[i]) )
					{
						(*Meshs)[i] = mesh;
					}
					postLoad();
				}

				return 0;
			}

			void exit() 
			{
				GpuSync->bUseFence = false;
				wglMakeCurrent(nullptr, nullptr);
				wglDeleteContext(hLoadRC);
				delete this;
			}
			GpuSync* GpuSync;
			HDC   hDC;
			HGLRC hLoadRC;
			std::vector< Texture2DPtr >* Textures;
			std::vector< MeshPtr >* Meshs;
			std::vector< MaterialPtr >* Materials;
		};

		DrawEngine* de = ::Global::getDrawEngine();
		HGLRC hRC = de->getGLContext().getHandle();
		HGLRC hLoadRC = wglCreateContext(de->getWindow().getHDC());
		wglShareLists( hRC , hLoadRC );


		LoadAssetTask* loadingTask = new LoadAssetTask;
		mGpuSync.bUseFence = true;
		loadingTask->GpuSync = &mGpuSync;
		loadingTask->Textures = &mTextures;
		loadingTask->Meshs = &mMeshs;
		loadingTask->Materials = &mMaterials;
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

		if(1)
		{
			Material& material = getMaterial(MaterialId::POMTitle);
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
			matWorld = Matrix4::Translate(Vector3(0, 0, 4));
			param.setWorld(matWorld);
			glColor3f(0.7, 0.7, 0.7);
			mPlane.draw();
			glPopMatrix();
		}


		{
			Material& material = getMaterial(MaterialId::Mario);
			param.setMaterial(material);
			param.setMaterialParameter(SHADER_PARAM(TextureD), getTexture(TextureId::MarioD));
			param.setMaterialParameter(SHADER_PARAM(TextureS), getTexture(TextureId::MarioS));
			Mesh& mesh = getMesh(MeshId::Mario);
			glPushMatrix();
			matWorld = Matrix4::Translate(Vector3(8, -10, 0));
			param.setWorld(matWorld);
			glColor3f(0.7, 0.7, 0.7);
			mesh.draw();
			glPopMatrix();
		}
		{
			Material& material = getMaterial(MaterialId::MetelA);
			param.setMaterial(material);
			param.setMaterialParameter(SHADER_PARAM(BaseTexture), getTexture(TextureId::Metel));
			Mesh& mesh = getMesh(MeshId::Dragon);
			glPushMatrix();
			matWorld = Matrix4::Rotate(Vector3(0, 0, 1), Math::Deg2Rad(45 + 180)) * Matrix4::Translate(Vector3(6, -6, 4));
			param.setWorld(matWorld);
			glColor3f(0.7, 0.7, 0.7);
			mesh.draw();
			glPopMatrix();
		}

		{

			int TextureGroup[][3] =
			{
				{ TextureId::Lightning07  , TextureId::Lightning17 , TextureId::Lightning06 } , //hair
				{ TextureId::Lightning11  , TextureId::Lightning08 , TextureId::Lightning12 } , //skin
				{ TextureId::Lightning05  , TextureId::Lightning09 , TextureId::Lightning10 } ,
				{ TextureId::Lightning13  , TextureId::Lightning14 , TextureId::Lightning15 } ,
				{ TextureId::Lightning16  , TextureId::Lightning16 , TextureId::Lightning16 } , //eye
				{ TextureId::Lightning02  , TextureId::Lightning03 , TextureId::Lightning04 } ,
				{ TextureId::LightningW0  , TextureId::LightningW0 , TextureId::LightningW0 } , //Weapon
			};

			int SectionTexMap[] =
			{
				0 , 0 , 1 , 0 , 2 ,
				1 , 3 , 3 , 1 , 2 ,
				3 , 4 , 1 , 2 , 1 ,
				2 , 5 , 3 , 2 , 0 ,
				6 , 6 , -1 , -1 , -1 ,
			};

			Material& material = getMaterial(MaterialId::MetelA);
			param.setMaterial(material);
			param.setMaterialParameter(SHADER_PARAM(BaseTexture), getTexture(TextureId::Metel));
			Mesh& mesh = getMesh(MeshId::Havoc);
			glPushMatrix();
			matWorld = Matrix4::Translate(Vector3(12, -12, 10));
			param.setWorld(matWorld);
			glColor3f(0.7, 0.7, 0.7);
			for( int i = 0; i < mesh.mSections.size(); ++i )
			{
				int* group = TextureGroup[SectionTexMap[i]];

				param.setMaterialParameter(SHADER_PARAM(TextureA), getTexture(group[0]));
				param.setMaterialParameter(SHADER_PARAM(TextureB), getTexture(group[1]));
				param.setMaterialParameter(SHADER_PARAM(TextureC), getTexture(group[2]));

				mesh.drawSection(i);
			}
			glPopMatrix();
		}
		{
			int TextureGroup[][3] =
			{
				{ TextureId::Lightning07  , TextureId::Lightning17 , TextureId::Lightning06 } , //hair
				{ TextureId::Lightning11  , TextureId::Lightning08 , TextureId::Lightning12 } , //skin
				{ TextureId::Lightning05  , TextureId::Lightning09 , TextureId::Lightning10 } ,
				{ TextureId::Lightning13  , TextureId::Lightning14 , TextureId::Lightning15 } ,
				{ TextureId::Lightning16  , TextureId::Lightning16 , TextureId::Lightning16 } , //eye
				{ TextureId::Lightning02  , TextureId::Lightning03 , TextureId::Lightning04 } ,
				{ TextureId::LightningW0  , TextureId::LightningW0 , TextureId::LightningW0 } , //Weapon
			};

			int SectionTexMap[] =
			{
				0 , 0 , 1 , 0 , 2 ,
				1 , 3 , 3 , 1 , 2 ,
				3 , 4 , 1 , 2 , 1 ,
				2 , 5 , 3 , 2 , 0 ,
				6 , 6 , -1 , -1 , -1 ,
			};

			glPushMatrix();
			Mesh& mesh = getMesh(MeshId::Lightning);
			Material& material = getMaterial(MaterialId::Lightning);
			param.setMaterial(material);

			matWorld = Matrix4::Translate(Vector3(-5, -10, 3));
			param.setWorld(matWorld);

			glColor3f(0.7, 0.7, 0.7);
			for( int i = 0; i < mesh.mSections.size(); ++i )
			{
				int* group = TextureGroup[SectionTexMap[i]];

				param.setMaterialParameter(SHADER_PARAM(TextureA), getTexture(group[0]));
				param.setMaterialParameter(SHADER_PARAM(TextureB), getTexture(group[1]));
				param.setMaterialParameter(SHADER_PARAM(TextureC), getTexture(group[2]));

				mesh.drawSection(i);
			}
			glPopMatrix();
		}

		if(0)
		{

			int TextureGroup[][3] =
			{
				{ TextureId::Vanille03 , TextureId::Vanille03 ,TextureId::Vanille03 } , //hair
				{ TextureId::Vanille00 , TextureId::Vanille03 ,TextureId::Vanille03 } , //skin
				{ TextureId::Vanille05 , TextureId::Vanille03 ,TextureId::Vanille03 } , //eye

			};

			int SectionTexMap[] =
			{
				0 , 1 , 1 , 0 , 1 ,
				1 , 1 , 2 , 0 , 2 ,
			};


			Mesh& mesh = getMesh(MeshId::Vanille);
			glPushMatrix();
			matWorld = Matrix4::Translate(Vector3(0, -13, 3));

			Material& material = getMaterial(MaterialId::Lightning);
			param.setMaterial(material);
			param.setWorld(matWorld);
			glColor3f(0.7, 0.7, 0.7);
			for( int i = 0; i < mesh.mSections.size(); ++i )
			{
				int* group = TextureGroup[SectionTexMap[i]];

				param.setMaterialParameter(SHADER_PARAM(TextureA), getTexture(group[0]));
				param.setMaterialParameter(SHADER_PARAM(TextureB), getTexture(group[1]));
				param.setMaterialParameter(SHADER_PARAM(TextureC), getTexture(group[2]));

				mesh.drawSection(i);
			}
			glPopMatrix();
		}
	
		{
			Material& material = getMaterial(MaterialId::Simple1);
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


		{
			glPushMatrix();
			matWorld = Matrix4::Rotate(Vector3(0, 0, -1), Math::Deg2Rad(45 + 180)) * Matrix4::Translate(Vector3(-6, 6, 4));
			param.setWorld(matWorld);
			glColor3f(0.7, 0.7, 0.7);
			getMesh(MeshId::Teapot).draw();
			glPopMatrix();
		}



		{
			Material& material = getMaterial(MaterialId::Elephant);
			param.setMaterial(material);
			param.setMaterialParameter(SHADER_PARAM(BaseTexture), getTexture(TextureId::ElephantSkin));

			glPushMatrix();
			matWorld = Matrix4::Rotate(Vector3(0, 0, -1), Math::Deg2Rad(0)) * Matrix4::Translate(Vector3(6, 8, 4));
			param.setWorld(matWorld);
			glColor3f(0.7, 0.7, 0.7);
			getMesh(MeshId::Elephant).draw();
			glPopMatrix();
		}



		{
			glDisable(GL_CULL_FACE);
			Material& material = getMaterial(MaterialId::Simple3);
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
		param.setMaterial(getMaterial((int)MaterialId::Simple2));
		glPushMatrix();
		matWorld = Matrix4::Scale(10.0) * Matrix4::Translate(Vector3(0, 0, 0));
		param.setWorld(matWorld);
		glColor3f(1, 1, 1);
		mPlane.draw();
		glPopMatrix();

		float scale = 2.5;
		float len = 10 * scale + 1;
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




		//glPushMatrix();
		//mProgSphere.bind();
		//mProgSphere.setParam( "sphere.radius" , 2.0f );
		//mProgSphere.setParam( "sphere.localPos" , Vector3(5,5,5) );
		//mSpherePlane.draw();
		//mProgSphere.unbind();
		//glPopMatrix();

		//glFlush();
	}


	void SampleStage::buildLights()
	{
		mNumLightDraw = 3;
		mShadowTech.mCascadeMaxDist = 300;

		Vector3 pos[LightNum] = { Vector3(0,0,0) , Vector3(0,0,0) , Vector3(0,0,0) , Vector3(0,0,0)  };
		Vector3 color[LightNum] = { Vector3(0.5,0.5,0.5) , Vector3(1,0.8,0) , Vector3(0.8,1,0) , Vector3(0.8,0.0,1) };
		float   intensity[LightNum] = { 10 , 300 , 300 , 500 };
		
		for( int i = 0; i < LightNum; ++i )
		{
			LightInfo& light = mLights[i];
			light.type = LightType::Point;
			light.pos = pos[i];
			light.color = color[i];
			light.intensity = intensity[i];
			light.radius = 50;
		}

		mLights[0].type = LightType::Directional;
		mLights[0].dir = Vector3(-1, 1, -1);
		//mLights[0].dir = Vector3(1, 0, 0);
		if(0)
		{
			LightInfo& light = mLights[0];
			light.type = LightType::Point;
			light.dir = Vector3(0, 1, -1);
			light.spotAngle.x = 45;
			light.spotAngle.y = 45;
			light.pos = Vector3(0, 0, 10);
			light.color = Vector3(1, 1, 1);
			light.intensity = 400;
		}

		{
			CycleTrack& track = mTracks[0];
			track.center = Vector3(0, 0, -5);
			track.radius = 5;
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
