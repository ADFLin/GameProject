#pragma once
#ifndef MeshBuild_H_EF732B9E_63A7_4579_8AC5_AEB0072874D3
#define MeshBuild_H_EF732B9E_63A7_4579_8AC5_AEB0072874D3

#include "Renderer/Mesh.h"

namespace Render
{
	struct OBJMaterialInfo
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
		char const* ambientTextureName;
		char const* diffuseTextureName;
		char const* specularTextureName;
		char const* normalTextureName;
		TArray < std::pair< char const*, char const* > > unknownParameters;

		char const* findParam(char const* name) const
		{
			for (auto& pair : unknownParameters)
			{
				if (strcmp(name, pair.first) == 0)
					return pair.second;
			}
			return nullptr;
		}
	};

	class OBJMaterialBuildListener
	{
	public:
		virtual void build(int idxSection, OBJMaterialInfo const* mat) = 0;
	};

	struct MeshBuildData
	{
		TArray< uint8 > vertexData;
		TArray< uint32 > indexData;

		bool initializeRHI(Mesh& mesh)
		{
			uint32 vertexSize = mesh.mInputLayoutDesc.getVertexSize();
			if (!mesh.createRHIResource(vertexData.data(), int(vertexData.size() / vertexSize), indexData.data(), (int)indexData.size()))
				return false;

			return true;
		}

		VertexElementReader makeAttributeReader(InputLayoutDesc const& desc, EVertex::Attribute attribute) const
		{
			VertexElementReader result;
			result.vertexDataStride = desc.getVertexSize();
			result.pVertexData = vertexData.data() + desc.getAttributeOffset(attribute);
			return result;
		}
	};

	class FMeshBuild
	{
	public:
		static bool Tile(Mesh& mesh, int tileSize, float len, bool bHaveSkirt = true);
		static bool Tile(MeshBuildData& data, InputLayoutDesc& inputLayoutDesc, int tileSize, float len, bool bHaveSkirt = true);
		static bool UVSphere(Mesh& mesh, float radius, int rings, int sectors);
		static bool UVSphere(MeshBuildData& data, InputLayoutDesc& inputLayoutDesc, float radius, int rings, int sectors);
		static bool IcoSphere(Mesh& mesh, float radius, int numDiv);
		static bool IcoSphere(MeshBuildData& data, InputLayoutDesc& inputLayoutDesc, float radius, int numDiv);

		static bool OctSphere(Mesh& mesh, float radius, int level);
		static bool OctSphere(MeshBuildData& data, InputLayoutDesc& inputLayoutDesc, float radius, int level);

		static bool SkyBox(Mesh& mesh);
		static bool SkyBox(MeshBuildData& data, InputLayoutDesc& inputLayoutDesc);
		static bool CubeShare(Mesh& mesh, float halfLen = 1.0f);
		static bool CubeShare(MeshBuildData& data, InputLayoutDesc& inputLayoutDesc, float halfLen = 1.0f);
		static bool CubeOffset(Mesh& mesh, float halfLen, Vector3 const& offset);
		static bool CubeOffset(MeshBuildData& data, InputLayoutDesc& inputLayoutDesc, float halfLen, Vector3 const& offset);
		static bool CubeLineOffset(Mesh& mesh, float halfLen, Vector3 const& offset);
		static bool CubeLineOffset(MeshBuildData& data, InputLayoutDesc& inputLayoutDesc, float halfLen, Vector3 const& offset);
		static bool Cube(Mesh& mesh, float halfLen = 1.0f);
		static bool Cube(MeshBuildData& data, InputLayoutDesc& inputLayoutDesc, float halfLen = 1.0f);

		static bool Cone(Mesh& mesh, float height, float radius, int numSide);
		static bool Cone(MeshBuildData& data, InputLayoutDesc& inputLayoutDesc, float height, float radius, int numSide);
		static bool Doughnut(Mesh& mesh, float radius, float ringRadius, int rings, int sectors);
		static bool Doughnut(MeshBuildData& data, InputLayoutDesc& inputLayoutDesc, float radius, float ringRadius, int rings, int sectors);
		static bool PlaneZ(Mesh& mesh, float halfLen, float texFactor);
		static bool PlaneZ(MeshBuildData& data, InputLayoutDesc& inputLayoutDesc, float halfLen, float texFactor);
		static bool Plane(Mesh& mesh, Vector3 const& offset, Vector3 const& normal, Vector3 const& dirY, Vector2 const& size, float texFactor);
		static bool Plane(MeshBuildData& data, InputLayoutDesc& inputLayoutDesc, Vector3 const& offset, Vector3 const& normal, Vector3 const& dirY, Vector2 const& size, float texFactor);
		static bool Disc(Mesh& mesh, float radius, int sectors);
		static bool Disc(MeshBuildData& data, InputLayoutDesc& inputLayoutDesc, float radius, int sectors);

		static bool SimpleSkin(Mesh& mesh, float width, float height, int nx, int ny);
		static bool SimpleSkin(MeshBuildData& data, InputLayoutDesc& inputLayoutDesc, float width, float height, int nx, int ny);

		static bool LightSphere(Mesh& mesh);
		static bool LightSphere(MeshBuildData& data, InputLayoutDesc& inputLayoutDesc);
		static bool LightCone(Mesh& mesh);
		static bool LightCone(MeshBuildData& data, InputLayoutDesc& inputLayoutDesc);

		static bool SpritePlane(Mesh& mesh);
		static bool SpritePlane(MeshBuildData& data, InputLayoutDesc& inputLayoutDesc);

		struct MeshData
		{
			float* position;
			int    numPosition;
			float* normal;
			int    numNormal;
			int*   indices;
			int    numIndex;
		};
		static bool ObjFileMesh(Mesh& mesh, MeshData& data);

		static bool LoadObjectFile(
			Mesh& mesh, char const* path,
			Matrix4* pTransform = nullptr,
			OBJMaterialBuildListener* listener = nullptr,
			int * skip = nullptr);

	};

}
#endif // MeshBuild_H_EF732B9E_63A7_4579_8AC5_AEB0072874D3