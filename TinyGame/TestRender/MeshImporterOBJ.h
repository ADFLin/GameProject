#pragma once
#ifndef MeshImporterOBJ_H_3C65EB33_5587_48D3_A93E_875796E2AF4B
#define MeshImporterOBJ_H_3C65EB33_5587_48D3_A93E_875796E2AF4B

#include "RHI/RHICommon.h"
#include "Renderer/MeshImportor.h"
#include "DataStructure/Array.h"

namespace Render
{
	class MeshImporterOBJ : public IMeshImporter
	{
	public:
		MeshImporterOBJ();
		~MeshImporterOBJ();

		bool importFromFile(char const* filePath, Mesh& outMesh, MeshImportSettings* settings) override;
		bool importFromFile(char const* filePath, MeshImportData& outMeshData) override;
		bool importMultiFromFile(char const* filePath, TArray<Mesh>& outMeshes, MeshImportSettings* settings) override;

	private:
		bool createMesh(MeshImportData& meshData, Mesh& outMesh);
	};

}

#endif // MeshImporterOBJ_H_3C65EB33_5587_48D3_A93E_875796E2AF4B
