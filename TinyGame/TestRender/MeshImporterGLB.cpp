#include "MeshImporterGLB.h"
#include "Renderer/MeshUtility.h"
#include "Core/ScopeGuard.h"

#include "tinygltf/tiny_gltf.h"

namespace Render
{
	REGISTER_MESH_IMPORTER(MeshImporterGLB, "GLB");

	MeshImporterGLB::MeshImporterGLB()
	{
	}

	MeshImporterGLB::~MeshImporterGLB()
	{
	}

	bool MeshImporterGLB::importFromFile(char const* filePath, Mesh& outMesh, MeshImportSettings* settings)
	{
		MeshImportData meshData;
		if (!importFromFile(filePath, meshData))
			return false;

		return createMesh(meshData, outMesh);
	}

	static bool LoadImageData(tinygltf::Image* image, const int image_idx, std::string* err,
		std::string* warn, int req_width, int req_height,
		const unsigned char* bytes, int size, void* user_data)
	{
		// Check for WebP signature: RIFF....WEBP
		if (size > 12 &&
			bytes[0] == 'R' && bytes[1] == 'I' && bytes[2] == 'F' && bytes[3] == 'F' &&
			bytes[8] == 'W' && bytes[9] == 'E' && bytes[10] == 'B' && bytes[11] == 'P')
		{
			if (warn) *warn += "WebP image detected and skipped (not supported by stb_image).\n";
			// Create a dummy 1x1 magenta image to avoid downstream crashes
			image->width = 1;
			image->height = 1;
			image->component = 4;
			image->bits = 8;
			image->pixel_type = TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE;
			image->image.assign({ 255, 0, 255, 255 });
			return true;
		}

		return tinygltf::LoadImageData(image, image_idx, err, warn, req_width, req_height, bytes, size, user_data);
	}

	bool MeshImporterGLB::importFromFile(char const* filePath, MeshImportData& outMeshData)
	{
		tinygltf::Model model;
		tinygltf::TinyGLTF loader;
		loader.SetImageLoader(LoadImageData, nullptr);
		std::string err;
		std::string warn;

		bool ret = loader.LoadBinaryFromFile(&model, &err, &warn, filePath);
		if (!warn.empty())
		{
			// Log warning
		}
		if (!err.empty())
		{
			// Log error
		}
		if (!ret) return false;

		return parseModel(model, outMeshData);
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

	bool MeshImporterGLB::parseModel(tinygltf::Model const& model, MeshImportData& outData)
	{
		if (model.meshes.empty())
			return false;

		const auto& firstMesh = model.meshes[0];
		if (firstMesh.primitives.empty())
			return false;

		const auto& firstPrim = firstMesh.primitives[0];

		// Define Layout
		outData.desc.addElement(0, EVertex::ATTRIBUTE_POSITION, EVertex::Float3);
		bool bHasNormal = firstPrim.attributes.count("NORMAL") > 0;
		bool bHasTangent = firstPrim.attributes.count("TANGENT") > 0;
		bool bHasTexcoord = firstPrim.attributes.count("TEXCOORD_0") > 0;
		bool bHasColor = firstPrim.attributes.count("COLOR_0") > 0;

		if (bHasNormal) outData.desc.addElement(0, EVertex::ATTRIBUTE_NORMAL, EVertex::Float3);
		if (bHasTangent) outData.desc.addElement(0, EVertex::ATTRIBUTE_TANGENT, EVertex::Float4);
		if (bHasTexcoord) outData.desc.addElement(0, EVertex::ATTRIBUTE_TEXCOORD, EVertex::Float2);
		if (bHasColor) outData.desc.addElement(0, EVertex::ATTRIBUTE_COLOR, EVertex::Float4);

		int vertexSize = outData.desc.getVertexSize(0);
		int totalTriangles = 0;

		for (const auto& mesh : model.meshes)
		{
			for (const auto& primitive : mesh.primitives)
			{
				if (primitive.indices >= 0)
					totalTriangles += model.accessors[primitive.indices].count / 3;
				else
					totalTriangles += model.accessors[primitive.attributes.at("POSITION")].count / 3;
			}
		}

		outData.numVertices = totalTriangles * 3;
		outData.vertices.resize(outData.numVertices * vertexSize);
		outData.indices.resize(outData.numVertices);

		int posOffset = outData.desc.getAttributeOffset(EVertex::ATTRIBUTE_POSITION);
		int normalOffset = bHasNormal ? outData.desc.getAttributeOffset(EVertex::ATTRIBUTE_NORMAL) : -1;
		int tangentOffset = bHasTangent ? outData.desc.getAttributeOffset(EVertex::ATTRIBUTE_TANGENT) : -1;
		int texcoordOffset = bHasTexcoord ? outData.desc.getAttributeOffset(EVertex::ATTRIBUTE_TEXCOORD) : -1;
		int colorOffset = bHasColor ? outData.desc.getAttributeOffset(EVertex::ATTRIBUTE_COLOR) : -1;

		uint8* pBuffer = outData.vertices.data();
		uint32* pIndex = outData.indices.data();
		int currentVertexIdx = 0;

		for (const auto& mesh : model.meshes)
		{
			for (const auto& primitive : mesh.primitives)
			{
				const auto& posAcc = model.accessors[primitive.attributes.at("POSITION")];
				const auto& posView = model.bufferViews[posAcc.bufferView];
				const float* posData = reinterpret_cast<const float*>(&(model.buffers[posView.buffer].data[posView.byteOffset + posAcc.byteOffset]));
				int posStride = posAcc.ByteStride(posView) / sizeof(float);

				const float* normData = nullptr; int normStride = 0;
				if (bHasNormal && primitive.attributes.count("NORMAL"))
				{
					const auto& acc = model.accessors[primitive.attributes.at("NORMAL")];
					const auto& view = model.bufferViews[acc.bufferView];
					normData = reinterpret_cast<const float*>(&(model.buffers[view.buffer].data[view.byteOffset + acc.byteOffset]));
					normStride = acc.ByteStride(view) / sizeof(float);
				}

				const float* tanData = nullptr; int tanStride = 0;
				if (bHasTangent && primitive.attributes.count("TANGENT"))
				{
					const auto& acc = model.accessors[primitive.attributes.at("TANGENT")];
					const auto& view = model.bufferViews[acc.bufferView];
					tanData = reinterpret_cast<const float*>(&(model.buffers[view.buffer].data[view.byteOffset + acc.byteOffset]));
					tanStride = acc.ByteStride(view) / sizeof(float);
				}

				const float* uvData = nullptr; int uvStride = 0;
				if (bHasTexcoord && primitive.attributes.count("TEXCOORD_0"))
				{
					const auto& acc = model.accessors[primitive.attributes.at("TEXCOORD_0")];
					const auto& view = model.bufferViews[acc.bufferView];
					uvData = reinterpret_cast<const float*>(&(model.buffers[view.buffer].data[view.byteOffset + acc.byteOffset]));
					uvStride = acc.ByteStride(view) / sizeof(float);
				}

				const float* colData = nullptr; int colStride = 0;
				if (bHasColor && primitive.attributes.count("COLOR_0"))
				{
					const auto& acc = model.accessors[primitive.attributes.at("COLOR_0")];
					const auto& view = model.bufferViews[acc.bufferView];
					colData = reinterpret_cast<const float*>(&(model.buffers[view.buffer].data[view.byteOffset + acc.byteOffset]));
					colStride = acc.ByteStride(view) / sizeof(float);
				}

				auto CopyVertex = [&](int srcIdx, int destIdx)
				{
					uint8* pDest = pBuffer + destIdx * vertexSize;
					// Position: glTF is RH (+X, +Y, +Z). Engine is LH. Flip Z.
					float* pPos = reinterpret_cast<float*>(pDest + posOffset);
					pPos[0] = posData[srcIdx * posStride + 0];
					pPos[1] = posData[srcIdx * posStride + 1];
					pPos[2] = posData[srcIdx * posStride + 2];

					if (normalOffset >= 0 && normData)
					{
						float* pNorm = reinterpret_cast<float*>(pDest + normalOffset);
						pNorm[0] = normData[srcIdx * normStride + 0];
						pNorm[1] = normData[srcIdx * normStride + 1];
						pNorm[2] = normData[srcIdx * normStride + 2];
					}
					if (tangentOffset >= 0 && tanData)
					{
						float* pTan = reinterpret_cast<float*>(pDest + tangentOffset);
						pTan[0] = tanData[srcIdx * tanStride + 0];
						pTan[1] = tanData[srcIdx * tanStride + 1];
						pTan[2] = tanData[srcIdx * tanStride + 2]; 
						pTan[3] = (tanStride >= 4) ? tanData[srcIdx * tanStride + 3] : 1.0f;
					}
					if (texcoordOffset >= 0 && uvData)
					{
						float* pUV = reinterpret_cast<float*>(pDest + texcoordOffset);
						pUV[0] = uvData[srcIdx * uvStride + 0];
						pUV[1] = uvData[srcIdx * uvStride + 1];
					}
					if (colorOffset >= 0 && colData)
					{
						float* pCol = reinterpret_cast<float*>(pDest + colorOffset);
						pCol[0] = colData[srcIdx * colStride + 0];
						pCol[1] = colData[srcIdx * colStride + 1];
						pCol[2] = colData[srcIdx * colStride + 2];
						pCol[3] = (colStride >= 4) ? colData[srcIdx * colStride + 3] : 1.0f;
					}
					pIndex[destIdx] = destIdx;
				};

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
						
						// Keep winding same as source (glTF CCW -> Flip Z makes it LH CW which is correct for D3D)
						CopyVertex(idx[0], currentVertexIdx + 0);
						CopyVertex(idx[1], currentVertexIdx + 1);
						CopyVertex(idx[2], currentVertexIdx + 2);
						currentVertexIdx += 3;
					}
				}
				else
				{
					for (size_t i = 0; i < posAcc.count; i += 3)
					{
						CopyVertex(i + 0, currentVertexIdx + 0);
						CopyVertex(i + 1, currentVertexIdx + 1);
						CopyVertex(i + 2, currentVertexIdx + 2);
						currentVertexIdx += 3;
					}
				}
			}
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
