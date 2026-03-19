#include "MeshImporterGLB.h"
#include "Image/ImageData.h"
#include "Renderer/MeshUtility.h"
#include "Core/ScopeGuard.h"

#define TINYGLTF_IMPLEMENTATION
#define TINYGLTF_NO_STB_IMAGE
#define TINYGLTF_NO_STB_IMAGE_WRITE
#include "tinygltf/tiny_gltf.h"
#include "LogSystem.h"

namespace Render
{
	REGISTER_MESH_IMPORTER(MeshImporterGLB, "GLB");

	MeshImporterGLB::MeshImporterGLB()
	{
	}

	MeshImporterGLB::~MeshImporterGLB()
	{
	}

	static bool LoadImageData(tinygltf::Image* image, const int image_idx, std::string* err,
		std::string* warn, int req_width, int req_height,
		const unsigned char* bytes, int size, void* user_data)
	{
		ImageData imageData;
		LogMsg("LoadImageData: idx=%d, size=%d", image_idx, size);
		ImageLoadOption option;
		option.bUpThreeComponentToFour = true;
		if (!imageData.loadFromMemory((void*)bytes, size, option))
		{
			if (err)
			{
				*err += "Failed to load image via ImageData in GLB importer.\n";
			}
			return false;
		}

		image->width = imageData.width;
		image->height = imageData.height;
		image->component = imageData.numComponent;
		image->bits = 8;
		image->pixel_type = TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE;
		image->image.assign((unsigned char*)imageData.data, (unsigned char*)imageData.data + imageData.dataSize);

		return true;
	}

	bool MeshImporterGLB::importFromFile(char const* filePath, Mesh& outMesh, MeshImportSettings* settings)
	{
		tinygltf::Model model;
		tinygltf::TinyGLTF loader;
		loader.SetImageLoader(LoadImageData, nullptr);
		std::string err;
		std::string warn;

		bool ret = loader.LoadBinaryFromFile(&model, &err, &warn, filePath);
		if (!ret) return false;

		MeshImportData meshData;
		if (!parseModel(model, meshData, settings))
			return false;

		return createMesh(meshData, outMesh);
	}

	bool MeshImporterGLB::importFromFile(char const* filePath, MeshImportData& outMeshData)
	{
		return importFromFile(filePath, outMeshData, nullptr);
	}

	bool MeshImporterGLB::importFromFile(char const* filePath, MeshImportData& outMeshData, MeshImportSettings* settings)
	{
		tinygltf::Model model;
		tinygltf::TinyGLTF loader;
		loader.SetImageLoader(LoadImageData, nullptr);
		std::string err;
		std::string warn;

		bool ret = loader.LoadBinaryFromFile(&model, &err, &warn, filePath);
		if (!ret) return false;

		return parseModel(model, outMeshData, settings);
	}

	bool MeshImporterGLB::importMultiFromFile(char const* filePath, TArray<Mesh>& outMeshes, MeshImportSettings* settings)
	{
		// Currently only supporting single mesh import or multiple meshes combined
		Mesh mesh;
		if (importFromFile(filePath, mesh, settings))
		{
			outMeshes.push_back(std::move(mesh));
			return true;
		}
		return false;
	}

	template< class T, int NumComponent >
	struct AccessorReader
	{
		AccessorReader(tinygltf::Model const& model, int index)
		{
			if (index < 0) return;
			const auto& acc = model.accessors[index];
			if (acc.bufferView < 0) return;
			const auto& view = model.bufferViews[acc.bufferView];
			pData = &(model.buffers[view.buffer].data[view.byteOffset + acc.byteOffset]);
			byteStride = acc.ByteStride(view);
			componentType = acc.componentType;
			count = acc.count;
		}

		void read(int index, T* out) const
		{
			if (!pData) return;
			const uint8* pElement = pData + index * byteStride;
			for (int i = 0; i < NumComponent; ++i)
			{
				switch (componentType)
				{
				case TINYGLTF_COMPONENT_TYPE_FLOAT:
					out[i] = reinterpret_cast<const float*>(pElement)[i];
					break;
				case TINYGLTF_COMPONENT_TYPE_DOUBLE:
					out[i] = (float)reinterpret_cast<const double*>(pElement)[i];
					break;
				case TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT:
					out[i] = reinterpret_cast<const uint16*>(pElement)[i] / 65535.0f;
					break;
				case TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE:
					out[i] = reinterpret_cast<const uint8*>(pElement)[i] / 255.0f;
					break;
				default:
					out[i] = 0;
					break;
				}
			}
		}

		const uint8* pData = nullptr;
		int byteStride = 0;
		int componentType = 0;
		size_t count = 0;
	};

	bool MeshImporterGLB::parseModel(tinygltf::Model const& model, MeshImportData& outData, MeshImportSettings* settings)
	{
		if (model.meshes.empty()) return false;

		const auto& firstMesh = model.meshes[0];
		if (firstMesh.primitives.empty()) return false;
		const auto& firstPrim = firstMesh.primitives[0];

		// Define Layout
		outData.desc.addElement(0, EVertex::ATTRIBUTE_POSITION, EVertex::Float3);
		bool bHasNormal = firstPrim.attributes.count("NORMAL") > 0;
		bool bHasTangent = firstPrim.attributes.count("TANGENT") > 0;
		bool bHasTexcoord = firstPrim.attributes.count("TEXCOORD_0") > 0;
		bool bHasColor = firstPrim.attributes.count("COLOR_0") > 0;

		if (bHasNormal) { outData.desc.addElement(0, EVertex::ATTRIBUTE_NORMAL, EVertex::Float3); }
		bool bNeedGenerateTangent = bHasNormal && bHasTexcoord;
		if (bHasTangent || bNeedGenerateTangent) { outData.desc.addElement(0, EVertex::ATTRIBUTE_TANGENT, EVertex::Float4); }
		if (bHasTexcoord) { outData.desc.addElement(0, EVertex::ATTRIBUTE_TEXCOORD, EVertex::Float2); }
		if (bHasColor) { outData.desc.addElement(0, EVertex::ATTRIBUTE_COLOR, EVertex::Float4); }

		int vertexSize = outData.desc.getVertexSize(0);
		int totalTriangles = 0;
		for (const auto& mesh : model.meshes) {
			for (const auto& primitive : mesh.primitives) {
				if (primitive.indices >= 0) totalTriangles += model.accessors[primitive.indices].count / 3;
				else totalTriangles += model.accessors[primitive.attributes.at("POSITION")].count / 3;
			}
		}

		outData.numVertices = totalTriangles * 3;
		outData.vertices.resize(outData.numVertices * vertexSize);
		outData.indices.resize(outData.numVertices);

		int posOffset = outData.desc.getAttributeOffset(EVertex::ATTRIBUTE_POSITION);
		int normalOffset = bHasNormal ? outData.desc.getAttributeOffset(EVertex::ATTRIBUTE_NORMAL) : -1;
		int tangentOffset = outData.desc.getAttributeOffset(EVertex::ATTRIBUTE_TANGENT);
		int texcoordOffset = bHasTexcoord ? outData.desc.getAttributeOffset(EVertex::ATTRIBUTE_TEXCOORD) : -1;
		int colorOffset = bHasColor ? outData.desc.getAttributeOffset(EVertex::ATTRIBUTE_COLOR) : -1;

		uint8* pBuffer = outData.vertices.data();
		uint32* pIndex = outData.indices.data();
		int currentVertexIdx = 0;

		for (const auto& mesh : model.meshes)
		{
			for (const auto& primitive : mesh.primitives)
			{
				AccessorReader<float, 3> posReader(model, primitive.attributes.at("POSITION"));
				AccessorReader<float, 3> normReader(model, bHasNormal && primitive.attributes.count("NORMAL") ? primitive.attributes.at("NORMAL") : -1);
				AccessorReader<float, 4> tanReader(model, bHasTangent && primitive.attributes.count("TANGENT") ? primitive.attributes.at("TANGENT") : -1);
				AccessorReader<float, 2> uvReader(model, bHasTexcoord && primitive.attributes.count("TEXCOORD_0") ? primitive.attributes.at("TEXCOORD_0") : -1);
				AccessorReader<float, 4> colReader(model, bHasColor && primitive.attributes.count("COLOR_0") ? primitive.attributes.at("COLOR_0") : -1);

				auto CopyVertex = [&](int srcIdx, int destIdx)
				{
					uint8* pDest = pBuffer + destIdx * vertexSize;
					// glTF RH -> Engine LH: Flip Z
					float pos[3]; posReader.read(srcIdx, pos);
					float* pPos = reinterpret_cast<float*>(pDest + posOffset);
					pPos[0] = pos[0]; pPos[1] = pos[1]; pPos[2] = -pos[2];

					if (normalOffset >= 0 && normReader.pData) 
					{
						float norm[3]; normReader.read(srcIdx, norm);
						float* pNorm = reinterpret_cast<float*>(pDest + normalOffset);
						pNorm[0] = norm[0]; pNorm[1] = norm[1]; pNorm[2] = -norm[2];
					}
					if (tangentOffset >= 0 && tanReader.pData)
					{
						float tan[4]; tanReader.read(srcIdx, tan);
						float* pTan = reinterpret_cast<float*>(pDest + tangentOffset);
						pTan[0] = tan[0]; pTan[1] = tan[1]; pTan[2] = -tan[2]; pTan[3] = tan[3];
					}
					if (texcoordOffset >= 0 && uvReader.pData)
					{
						float uv[2]; uvReader.read(srcIdx, uv);
						float* pUV = reinterpret_cast<float*>(pDest + texcoordOffset);
						pUV[0] = uv[0]; pUV[1] = uv[1];
					}
					if (colorOffset >= 0 && colReader.pData) 
					{
						float col[4]; colReader.read(srcIdx, col);
						float* pCol = reinterpret_cast<float*>(pDest + colorOffset);
						pCol[0] = col[0]; pCol[1] = col[1]; pCol[2] = col[2]; pCol[3] = col[3];
					}
					pIndex[destIdx] = destIdx;
				};

				int indexStart = currentVertexIdx;
				if (primitive.indices >= 0)
				{
					const auto& acc = model.accessors[primitive.indices];
					const auto& view = model.bufferViews[acc.bufferView];
					const uint8* data = &(model.buffers[view.buffer].data[view.byteOffset + acc.byteOffset]);
					for (size_t i = 0; i < acc.count; i += 3)
					{
						uint32 idx[3];
						for (int n = 0; n < 3; ++n)
						{
							if (acc.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT) idx[n] = reinterpret_cast<const uint32*>(data)[i + n];
							else if (acc.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT) idx[n] = reinterpret_cast<const uint16*>(data)[i + n];
							else if (acc.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE) idx[n] = reinterpret_cast<const uint8*>(data)[i + n];
						}
						CopyVertex(idx[0], currentVertexIdx + 0);
						CopyVertex(idx[1], currentVertexIdx + 1);
						CopyVertex(idx[2], currentVertexIdx + 2);
						currentVertexIdx += 3;
					}
				}
				else
				{
					for (size_t i = 0; i < posReader.count; i += 3)
					{
						CopyVertex(i + 0, currentVertexIdx + 0);
						CopyVertex(i + 1, currentVertexIdx + 1);
						CopyVertex(i + 2, currentVertexIdx + 2);
						currentVertexIdx += 3;
					}
				}

				MeshSection section;
				section.indexStart = indexStart;
				section.count = currentVertexIdx - indexStart;
				outData.sections.push_back(section);

				if (settings)
				{
					if (primitive.material >= 0 && primitive.material < model.materials.size())
					{
						const auto& mat = model.materials[primitive.material];
						GLBMaterialInfo info;
						auto GetImageIndex = [&](int textureIndex) -> int
						{
							if (textureIndex < 0 || textureIndex >= model.textures.size()) 
								return -1;

							const auto& tex = model.textures[textureIndex];
							int source = tex.source;
							if (source == -1 && !tex.extensions.empty())
							{
								static const char* sSourceExtensions[] = { "EXT_texture_webp", "MSFT_texture_dds", "KHR_texture_basisu" };
								for (const char* extName : sSourceExtensions)
								{
									auto it = tex.extensions.find(extName);
									if (it != tex.extensions.end() && it->second.IsObject() && it->second.Has("source"))
									{
										source = it->second.Get("source").Get<int>();
										LogMsg("Found source in extension %s: %d", extName, source);
										break;
									}
								}
							}

							if (source == -1)
							{
								for (auto const& pair : tex.extensions) 
								{
									LogMsg("Unknown Texture Extension: %s", pair.first.c_str());
								}
							}

							LogMsg("GetImageIndex: tex=%d -> img=%d", textureIndex, source);
							return source;
						};

						LogMsg("Mat: %s, BaseColorTexIdx: %d", mat.name.c_str(), mat.pbrMetallicRoughness.baseColorTexture.index);
						info.name = mat.name.empty() ? nullptr : mat.name.c_str();
						info.baseColorFactor = LinearColor((float)mat.pbrMetallicRoughness.baseColorFactor[0], (float)mat.pbrMetallicRoughness.baseColorFactor[1], (float)mat.pbrMetallicRoughness.baseColorFactor[2], (float)mat.pbrMetallicRoughness.baseColorFactor[3]);
						info.baseColorTexture.imageIndex = GetImageIndex(mat.pbrMetallicRoughness.baseColorTexture.index);
						info.baseColorTexture.texCoordIndex = mat.pbrMetallicRoughness.baseColorTexture.texCoord;

						info.metallicFactor = (float)mat.pbrMetallicRoughness.metallicFactor;
						info.roughnessFactor = (float)mat.pbrMetallicRoughness.roughnessFactor;
						info.metallicRoughnessTexture.imageIndex = GetImageIndex(mat.pbrMetallicRoughness.metallicRoughnessTexture.index);
						info.metallicRoughnessTexture.texCoordIndex = mat.pbrMetallicRoughness.metallicRoughnessTexture.texCoord;

						info.normalTexture.imageIndex = GetImageIndex(mat.normalTexture.index);
						info.normalTexture.texCoordIndex = mat.normalTexture.texCoord;
						info.normalScale = (float)mat.normalTexture.scale;

						info.emissiveTexture.imageIndex = GetImageIndex(mat.emissiveTexture.index);
						info.emissiveTexture.texCoordIndex = mat.emissiveTexture.texCoord;
						info.emissiveFactor = Color3f((float)mat.emissiveFactor[0], (float)mat.emissiveFactor[1], (float)mat.emissiveFactor[2]);

						info.occlusionTexture.imageIndex = GetImageIndex(mat.occlusionTexture.index);
						info.occlusionTexture.texCoordIndex = mat.occlusionTexture.texCoord;
						info.occlusionStrength = (float)mat.occlusionTexture.strength;

						info.bDoubleSided = mat.doubleSided;

						TArray< GLBImageInfo > imageInfos;
						imageInfos.resize(model.images.size());
						for( int i = 0 ; i < model.images.size() ; ++i )
						{
							auto const& image = model.images[i];
							imageInfos[i].data = image.image.data();
							imageInfos[i].dataSize = image.image.size();
							imageInfos[i].width = image.width;
							imageInfos[i].height = image.height;
							imageInfos[i].component = image.component;
						}
						info.images = imageInfos;

						settings->setupMaterial(0, outData.sections.size() - 1, (void*)&info);
					}
				}
			}
		}

		if (outData.desc.getAttributeOffset(EVertex::ATTRIBUTE_TANGENT) != INDEX_NONE)
		{
			MeshUtility::FillTangent_TriangleList(outData.desc, outData.vertices.data(), outData.numVertices, outData.indices.data(), (int)outData.indices.size());
		}

		return true;
	}

	bool MeshImporterGLB::createMesh(MeshImportData& meshData, Mesh& outMesh)
	{
		outMesh.mType = EPrimitive::TriangleList;
		outMesh.mInputLayoutDesc = std::move(meshData.desc);
		outMesh.mSections = std::move(meshData.sections);
		if (outMesh.mSections.empty())
		{
			MeshSection section;
			section.indexStart = 0;
			section.count = meshData.indices.size();
			outMesh.mSections.push_back(section);
		}

		bool bResult = outMesh.createRHIResource(meshData.vertices.data(), meshData.numVertices, meshData.indices.data(), meshData.indices.size());

		if (bResult)
		{
			// Log success and stats for debugging
			// LogMsg("GLB Mesh Created: %d vertices, %d indices", meshData.numVertices, (int)meshData.indices.size());
		}

		return bResult;
	}
}
