#include "MeshImporterOBJ.h"
#include "Renderer/MeshBuild.h"
#include "Renderer/MeshUtility.h"
#include "FileSystem.h"
#include "tinyobjloader/tiny_obj_loader.h"

#include <algorithm>
#include <map>
#include <vector>
#include <string>

namespace Render
{
	REGISTER_MESH_IMPORTER(MeshImporterOBJ, "OBJ");

	MeshImporterOBJ::MeshImporterOBJ() {}
	MeshImporterOBJ::~MeshImporterOBJ() {}

	bool MeshImporterOBJ::importFromFile(char const* filePath, Mesh& outMesh, MeshImportSettings* settings)
	{
		struct SettingsMaterialListener : public OBJMaterialBuildListener
		{
			MeshImportSettings* settings;
			void build(int idxSection, OBJMaterialInfo const* mat) override
			{
				if (settings)
					settings->setupMaterial(idxSection, (void*)mat);
			}
		};

		SettingsMaterialListener listener;
		listener.settings = settings;
		return FMeshBuild::LoadObjectFile(outMesh, filePath, nullptr, &listener);
	}

	bool MeshImporterOBJ::importFromFile(char const* filePath, MeshImportData& outMeshData)
	{
		std::vector<tinyobj::shape_t> shapes;
		std::vector<tinyobj::material_t> materials;

		char const* dirPos = FFileUtility::GetFileName(filePath);
		std::string dir = std::string(filePath, dirPos - filePath);

		std::string err = tinyobj::LoadObj(shapes, materials, filePath, dir.c_str());

		if (shapes.empty())
			return false;

		int numVerticesTotal = 0;
		int numIndicesTotal = 0;

		for (int i = 0; i < shapes.size(); ++i)
		{
			tinyobj::mesh_t& objMesh = shapes[i].mesh;
			numVerticesTotal += objMesh.positions.size() / 3;
			numIndicesTotal += objMesh.indices.size();
		}

		if (numVerticesTotal == 0)
			return false;

		outMeshData.desc.clear();
		outMeshData.desc.addElement(0, EVertex::ATTRIBUTE_POSITION, EVertex::Float3);
		outMeshData.desc.addElement(0, EVertex::ATTRIBUTE_NORMAL, EVertex::Float3);

		bool bHasTexCoord = !shapes[0].mesh.texcoords.empty();
		if (bHasTexCoord)
		{
			outMeshData.desc.addElement(0, EVertex::ATTRIBUTE_TEXCOORD, EVertex::Float2);
			outMeshData.desc.addElement(0, EVertex::ATTRIBUTE_TANGENT, EVertex::Float4);
		}

		int vertexStride = outMeshData.desc.getVertexSize();
		int vertexStrideFloats = vertexStride / sizeof(float);
		TArray< float > vertices(numVerticesTotal * vertexStrideFloats);
		TArray< uint32 > indices;
		indices.reserve(numIndicesTotal);

		struct ObjMeshSection
		{
			int matId;
			TArray< uint32 > indices;
		};
		std::map< int, ObjMeshSection > meshSectionMap;

		int vOffsetFloats = 0;
		for (int idx = 0; idx < shapes.size(); ++idx)
		{
			tinyobj::mesh_t& objMesh = shapes[idx].mesh;
			int nvCur = vOffsetFloats / vertexStrideFloats;

			int matId = objMesh.material_ids.empty() ? -1 : objMesh.material_ids[0];
			ObjMeshSection& meshSection = meshSectionMap[matId];
			meshSection.matId = matId;
			int startIndex = (int)meshSection.indices.size();
			meshSection.indices.reserve(meshSection.indices.size() + objMesh.indices.size());
			for (auto idx : objMesh.indices)
			{
				meshSection.indices.push_back(idx + nvCur);
			}

			int objNV = (int)objMesh.positions.size() / 3;
			float* pPos = &objMesh.positions[0];
			float* v = &vertices[vOffsetFloats];

			if (objMesh.normals.empty())
			{
				for (int i = 0; i < objNV; ++i)
				{
					v[0] = pPos[0]; v[1] = pPos[1]; v[2] = pPos[2];
					pPos += 3;
					v += vertexStrideFloats;
				}
			}
			else
			{
				float* pNormal = &objMesh.normals[0];
				for (int i = 0; i < objNV; ++i)
				{
					v[0] = pPos[0]; v[1] = pPos[1]; v[2] = pPos[2];
					v[3] = pNormal[0]; v[4] = pNormal[1]; v[5] = pNormal[2];
					pPos += 3;
					pNormal += 3;
					v += vertexStrideFloats;
				}
			}

			if (bHasTexCoord && !objMesh.texcoords.empty())
			{
				float* pTex = &objMesh.texcoords[0];
				float* vUV = &vertices[vOffsetFloats] + 6;
				for (int i = 0; i < objNV; ++i)
				{
					vUV[0] = pTex[0]; vUV[1] = pTex[1];
					pTex += 2;
					vUV += vertexStrideFloats;
				}
			}

			vOffsetFloats += objNV * vertexStrideFloats;
		}

		TArray< ObjMeshSection* > sortedMeshSections;
		for (auto& pair : meshSectionMap)
		{
			sortedMeshSections.push_back(&pair.second);
		}
		std::sort(sortedMeshSections.begin(), sortedMeshSections.end(),
			[](ObjMeshSection const* m1, ObjMeshSection const* m2) -> bool
		{
			return m1->matId < m2->matId;
		});

		outMeshData.sections.clear();
		for (auto meshSection : sortedMeshSections)
		{
			MeshSection section;
			section.count = (int)meshSection->indices.size();
			section.indexStart = (int)indices.size();
			outMeshData.sections.push_back(section);
			indices.append(meshSection->indices);
		}

		if (shapes[0].mesh.normals.empty())
		{
			if (bHasTexCoord)
				MeshUtility::FillNormalTangent_TriangleList(outMeshData.desc, &vertices[0], numVerticesTotal, indices.data(), indices.size());
			else
				MeshUtility::FillNormal_TriangleList(outMeshData.desc, &vertices[0], numVerticesTotal, indices.data(), indices.size());
		}
		else if (bHasTexCoord)
		{
			MeshUtility::FillTangent_TriangleList(outMeshData.desc, &vertices[0], numVerticesTotal, indices.data(), indices.size());
		}

		outMeshData.vertices.assign((uint8*)vertices.data(), (uint8*)vertices.data() + vertices.size() * sizeof(float));
		outMeshData.indices = std::move(indices);
		outMeshData.numVertices = numVerticesTotal;

		return true;
	}

	bool MeshImporterOBJ::importMultiFromFile(char const* filePath, TArray<Mesh>& outMeshes, MeshImportSettings* settings)
	{
		// OBJ usually doesn't have multiple meshes in a way that maps easily to importMultiFromFile without more info.
		// We'll just import it as one mesh for now.
		Mesh mesh;
		if (importFromFile(filePath, mesh, settings))
		{
			outMeshes.push_back(std::move(mesh));
			return true;
		}
		return false;
	}

	bool MeshImporterOBJ::createMesh(MeshImportData& meshData, Mesh& outMesh)
	{
		outMesh.mType = EPrimitive::TriangleList;
		outMesh.mInputLayoutDesc = std::move(meshData.desc);
		outMesh.mSections = std::move(meshData.sections);
		return outMesh.createRHIResource(meshData.vertices.data(), meshData.numVertices, meshData.indices.data(), (int)meshData.indices.size());
	}

}
