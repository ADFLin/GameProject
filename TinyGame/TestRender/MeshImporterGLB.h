#pragma once
#ifndef GLBImporter_H_
#define GLBImporter_H_

#include "RHI/RHICommon.h"
#include "Renderer/MeshImportor.h"
#include "DataStructure/Array.h"

namespace tinygltf
{
	class Model;
}

namespace Render
{
	class MeshImporterGLB : public IMeshImporter
	{
	public:
		MeshImporterGLB();
		~MeshImporterGLB();

		bool importFromFile(char const* filePath, Mesh& outMesh, MeshImportSettings* settings) override;
		bool importFromFile(char const* filePath, MeshImportData& outMeshData) override;
		bool importMultiFromFile(char const* filePath, TArray<Mesh>& outMeshes, MeshImportSettings* settings) override;

	private:
		bool parseModel(tinygltf::Model const& model, MeshImportData& outMeshData);
		bool createMesh(MeshImportData& meshData, Mesh& outMesh);
	};
}

#endif // GLBImporter_H_
