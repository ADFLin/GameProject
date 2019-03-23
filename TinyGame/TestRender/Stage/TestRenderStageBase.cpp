#include "TestRenderStageBase.h"


namespace Render
{

	struct OBJMaterialSaveInfo
	{
		char const* name;

		Vector3 ambient;
		Vector3 diffuse;
		Vector3 specular;
		Vector3 transmittance;
		Vector3 emission;
		float shininess;
		float ior;
		float dissolve;
		int   illum;
		std::string ambientTextureName;
		std::string diffuseTextureName;
		std::string specularTextureName;
		std::string normalTextureName;
		std::map< std::string, std::string > unknownParameters;

		template< class OP >
		void serialize(OP op)
		{
			op & ambient;
			op & diffuse;
			op & specular;
			op & transmittance;
			op & emission;
			op & shininess;
			op & ior;
			op & dissolve;
			op & illum;
			op & ambientTextureName;
			op & diffuseTextureName;
			op & specularTextureName;
			op & normalTextureName;
			op & unknownParameters;
		}
	};

	TYPE_SUPPORT_SERIALIZE_FUNC(OBJMaterialSaveInfo)

	bool LoadObjectMesh(StaticMesh& mesh, char const* path)
	{
		return false;
	}

	bool SharedAssetData::createCommonShader()
	{
		VERIFY_RETURN_FALSE(ShaderManager::Get().loadFileSimple(mProgSphere, "Shader/Game/Sphere"));

		return true;
	}

	bool SharedAssetData::createSimpleMesh()
	{
		VERIFY_RETURN_FALSE(BuildMesh(mSimpleMeshs[SimpleMeshId::Plane], "PlaneZ", MeshBuild::PlaneZ, 10, 1));
		//VERIFY_RETURN_FALSE(MeshBuild::PlaneZ(mSimpleMeshs[SimpleMeshId::Plane], 10, 1));
		VERIFY_RETURN_FALSE(BuildMesh(mSimpleMeshs[SimpleMeshId::Tile], "Tile", MeshBuild::Tile, 64, 1.0f, true));
		VERIFY_RETURN_FALSE(BuildMesh(mSimpleMeshs[SimpleMeshId::Sphere], "UVSphere", MeshBuild::UVSphere, 2.5, 60, 60));
		//VERIFY_RETURN_FALSE(MeshBuild::UVSphere(mSimpleMeshs[ SimpleMeshId::Sphere ], 2.5, 4, 4) );
		VERIFY_RETURN_FALSE(BuildMesh(mSimpleMeshs[SimpleMeshId::Sphere2], "IcoSphere", MeshBuild::IcoSphere, 2.5, 4));
		VERIFY_RETURN_FALSE(BuildMesh(mSimpleMeshs[SimpleMeshId::Box], "Cube", MeshBuild::Cube, 1));
		VERIFY_RETURN_FALSE(BuildMesh(mSimpleMeshs[SimpleMeshId::SkyBox], "SkyBox", MeshBuild::SkyBox));
		VERIFY_RETURN_FALSE(BuildMesh(mSimpleMeshs[SimpleMeshId::Doughnut], "Doughnut", MeshBuild::Doughnut, 2, 1, 60, 60));
		VERIFY_RETURN_FALSE(BuildMesh(mSimpleMeshs[SimpleMeshId::SimpleSkin], "SimpleSkin", MeshBuild::SimpleSkin, 5, 2.5, 20, 20));
		VERIFY_RETURN_FALSE(BuildMesh(mSimpleMeshs[SimpleMeshId::Terrain], "Terrain", MeshBuild::Tile, 1024, 1.0, false));

		Vector3 v[] =
		{
			Vector3(1,1,0),
			Vector3(-1,1,0) ,
			Vector3(-1,-1,0),
			Vector3(1,-1,0),
		};
		int   idx[6] = { 0 , 1 , 2 , 0 , 2 , 3 };
		mSimpleMeshs[SimpleMeshId::SpherePlane].mInputLayoutDesc.addElement(Vertex::ePosition, Vertex::eFloat3);
		if( !mSimpleMeshs[SimpleMeshId::SpherePlane].createRHIResource(&v[0], 4, &idx[0], 6, true) )
			return false;

		return true;
	}

	void TestRenderStageBase::drawLightPoints(ViewInfo& view, TArrayView< LightInfo > lights)
	{
		GL_BIND_LOCK_OBJECT(mProgSphere);

		RHISetBlendState(TStaticBlendState<>::GetRHI());
		RHISetDepthStencilState(TStaticDepthStencilState<>::GetRHI());

		float radius = 0.15f;
		view.setupShader(mProgSphere);
		mProgSphere.setParam(SHADER_PARAM(Primitive.worldToLocal), Matrix4::Identity());
		mProgSphere.setParam(SHADER_PARAM(Sphere.radius), radius);


		for( auto const& light : lights )
		{
			if( !light.testVisible(view) )
				return;

			mProgSphere.setParam(SHADER_PARAM(Sphere.localPos), light.pos);
			mProgSphere.setParam(SHADER_PARAM(Sphere.baseColor), light.color);
			mSimpleMeshs[SimpleMeshId::SpherePlane].draw();
		}
	}

}//namespace Render