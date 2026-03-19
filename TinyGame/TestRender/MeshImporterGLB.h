#pragma once
#ifndef GLBImporter_H_
#define GLBImporter_H_

#include "RHI/RHICommon.h"
#include "Renderer/MeshImportor.h"
#include "DataStructure/Array.h"
#include "Template/ArrayView.h"
#include "Math/Vector4.h"
#include "Math/Vector3.h"
#include "Core/Color.h"

namespace tinygltf
{
	class Model;
}

namespace Render
{
	struct GLBImageInfo
	{
		unsigned char const* data;
		size_t dataSize;
		int width;
		int height;
		int component;
	};

	struct GLBMaterialTextureInfo
	{
		int imageIndex = -1;
		int texCoordIndex = 0;
	};

	struct GLBMaterialInfo
	{
		char const* name;
		LinearColor     baseColorFactor;
		GLBMaterialTextureInfo baseColorTexture;

		float       metallicFactor;
		float       roughnessFactor;
		GLBMaterialTextureInfo metallicRoughnessTexture;

		GLBMaterialTextureInfo normalTexture;
		float       normalScale;

		GLBMaterialTextureInfo emissiveTexture;
		Color3f     emissiveFactor;

		GLBMaterialTextureInfo occlusionTexture;
		float       occlusionStrength;

		bool        bDoubleSided;

		// Link back to image data in model
		TArrayView< GLBImageInfo const > images;
	};

	class MeshImporterGLB : public IMeshImporter
	{
	public:
		MeshImporterGLB();
		~MeshImporterGLB();

		bool importFromFile(char const* filePath, Mesh& outMesh, MeshImportSettings* settings) override;
		bool importFromFile(char const* filePath, MeshImportData& outMeshData) override;
		bool importMultiFromFile(char const* filePath, TArray<Mesh>& outMeshes, MeshImportSettings* settings) override;

	private:
		bool importFromFile(char const* filePath, MeshImportData& outMeshData, MeshImportSettings* settings);
		bool parseModel(tinygltf::Model const& model, MeshImportData& outMeshData, MeshImportSettings* settings);
		bool createMesh(MeshImportData& meshData, Mesh& outMesh);
	};
}

#endif // GLBImporter_H_
