#include "MeshImporterFBX.h"

#include "Core/ScopeGuard.h"
#include "Renderer/MeshUtility.h"


namespace Render
{
	REGISTER_MESH_IMPORTER(MeshImporterFBX, "FBX");

	MeshImporterFBX::MeshImporterFBX()
	{
		mManager = FbxManager::Create();
		mIOSetting = FbxIOSettings::Create(mManager, IOSROOT);
		mManager->SetIOSettings(mIOSetting);
		mScene = FbxScene::Create(mManager, "My Scene");

		mGeometryConverter = std::make_unique< FbxGeometryConverter >(mManager);
	}

	MeshImporterFBX::~MeshImporterFBX()
	{
		cleanup();
	}

	void MeshImporterFBX::cleanup()
	{
		mGeometryConverter.release();
#define SAFE_DESTROY( P ) if ( P ){ P->Destroy(); P = nullptr; }
		SAFE_DESTROY(mIOSetting);
		SAFE_DESTROY(mScene);
		SAFE_DESTROY(mManager);
	}

	bool MeshImporterFBX::importFromFile(char const* filePath, Mesh& outMesh, MeshImportSettings* settings)
	{
		FbxImporter* mImporter = FbxImporter::Create(mManager, "");
		ON_SCOPE_EXIT
		{
			mImporter->Destroy();
		};

		if (!mImporter->Initialize(filePath, -1, mManager->GetIOSettings())) return false;

		mScene->Clear();
		if (!mImporter->Import(mScene)) return false;

		FbxNode* pRootNode = mScene->GetRootNode();
		if (pRootNode)
		{
			MeshImportData meshData;
			int meshCount = 0;
			FbxAMatrix I; I.SetIdentity();
			processNodeRecursive(pRootNode, meshData, meshCount, I);

			if (meshCount > 0 && meshData.numVertices > 0)
			{
				return createMesh(meshData, outMesh);
			}
		}
		return false;
	}

	bool MeshImporterFBX::importFromFile(char const* filePath, MeshImportData& outMeshData)
	{
		FbxImporter* mImporter = FbxImporter::Create(mManager, "");
		ON_SCOPE_EXIT 
		{ 
			mImporter->Destroy(); 
		};

		if (!mImporter->Initialize(filePath, -1, mManager->GetIOSettings())) 
			return false;

		mScene->Clear();
		if (!mImporter->Import(mScene)) 
			return false;

		FbxNode* pRootNode = mScene->GetRootNode();
		if (pRootNode)
		{
			int meshCount = 0;
			FbxAMatrix I; I.SetIdentity();
			processNodeRecursive(pRootNode, outMeshData, meshCount, I);
			return (meshCount > 0 && outMeshData.numVertices > 0);
		}
		return false;
	}

	bool MeshImporterFBX::importMultiFromFile(char const* filePath, TArray<Mesh>& outMeshes, MeshImportSettings* settings)
	{
		FbxImporter* mImporter = FbxImporter::Create(mManager, "");
		ON_SCOPE_EXIT 
		{ 
			mImporter->Destroy(); 
		};

		if (!mImporter->Initialize(filePath, -1, mManager->GetIOSettings())) 
			return false;

		mScene->Clear();
		if (!mImporter->Import(mScene)) 
			return false;

		FbxNode* pRootNode = mScene->GetRootNode();
		if (pRootNode)
		{
			TArray< MeshImportData > meshDataList;
			FbxAMatrix I; I.SetIdentity();
			processNodeRecursiveMulti(pRootNode, meshDataList, I);

			for (auto& meshData : meshDataList)
			{
				if (meshData.numVertices > 0)
				{
					Mesh mesh;
					if (createMesh(meshData, mesh))
					{
						outMeshes.push_back(std::move(mesh));
					}
				}
			}
			return !outMeshes.empty();
		}
		return false;
	}

	void MeshImporterFBX::processNodeRecursive(FbxNode* pNode, MeshImportData& outData, int& meshCount, FbxAMatrix const& parentTransform)
	{
		FbxAMatrix localTransform = pNode->EvaluateLocalTransform();
		FbxAMatrix globalTransform = parentTransform * localTransform;

		for (int i = 0; i < pNode->GetNodeAttributeCount(); i++)
		{
			FbxNodeAttribute* pAttr = pNode->GetNodeAttributeByIndex(i);
			if (pAttr->GetAttributeType() == FbxNodeAttribute::eMesh)
			{
				MeshImportData subMeshData;
				if (parseMesh((FbxMesh*)pAttr, subMeshData, globalTransform))
				{
					if (meshCount == 0) outData = std::move(subMeshData);
					else if (outData.desc == subMeshData.desc) outData.append(subMeshData);
					meshCount++;
				}
			}
		}

		for (int i = 0; i < pNode->GetChildCount(); i++)
		{
			processNodeRecursive(pNode->GetChild(i), outData, meshCount, globalTransform);
		}
	}

	void MeshImporterFBX::processNodeRecursiveMulti(FbxNode* pNode, TArray<MeshImportData>& meshDataList, FbxAMatrix const& parentTransform)
	{
		FbxAMatrix localTransform = pNode->EvaluateLocalTransform();
		FbxAMatrix globalTransform = parentTransform * localTransform;

		for (int i = 0; i < pNode->GetNodeAttributeCount(); i++)
		{
			FbxNodeAttribute* pAttr = pNode->GetNodeAttributeByIndex(i);
			if (pAttr->GetAttributeType() == FbxNodeAttribute::eMesh)
			{
				MeshImportData subMeshData;
				if (parseMesh((FbxMesh*)pAttr, subMeshData, globalTransform))
				{
					bool bAppended = false;
					for (auto& existingData : meshDataList)
					{
						if (existingData.desc == subMeshData.desc)
						{
							existingData.append(subMeshData);
							bAppended = true;
							break;
						}
					}
					if (!bAppended) meshDataList.push_back(std::move(subMeshData));
				}
			}
		}

		for (int i = 0; i < pNode->GetChildCount(); i++)
		{
			processNodeRecursiveMulti(pNode->GetChild(i), meshDataList, globalTransform);
		}
	}

	void MeshImporterFBX::GetMeshVertexFormat(FbxMesh* pMesh, FBXVertexFormat& outFormat)
	{
		auto AddElementData = [&outFormat](int indexBuffer, FbxLayerElement* pElement, TArray< FBXElementDataInfo >& elements)
		{
			elements.push_back({ indexBuffer , pElement ,pElement->GetMappingMode(), pElement->GetReferenceMode() });
		};

		for (int i = 0; i < pMesh->GetElementVertexColorCount(); i++)
		{
			FbxGeometryElementVertexColor* pElement = pMesh->GetElementVertexColor(i);
			AddElementData(i, pElement, outFormat.colors);
		}
		for (int i = 0; i < pMesh->GetElementUVCount(); ++i)
		{
			FbxGeometryElementUV* pElement = pMesh->GetElementUV(i);
			AddElementData(i, pElement, outFormat.texcoords);
		}
		for (int i = 0; i < pMesh->GetElementNormalCount(); ++i)
		{
			FbxGeometryElementNormal* pElement = pMesh->GetElementNormal(i);
			AddElementData(i, pElement, outFormat.normals);

		}
		for (int i = 0; i < pMesh->GetElementTangentCount(); ++i)
		{
			FbxGeometryElementTangent* pElement = pMesh->GetElementTangent(i);
			AddElementData(i, pElement, outFormat.tangents);
		}
		for (int i = 0; i < pMesh->GetElementBinormalCount(); ++i)
		{
			FbxGeometryElementBinormal* pElement = pMesh->GetElementBinormal(i);
			AddElementData(i, pElement, outFormat.binormals);
		}
	}

	bool MeshImporterFBX::parseMesh(FbxMesh* pFBXMesh, Mesh& outMesh)
	{
		MeshImportData meshData;
		FbxAMatrix I; I.SetIdentity();
		if (!parseMesh(pFBXMesh, meshData, I))
			return false;

		return createMesh(meshData, outMesh);
	}

	bool MeshImporterFBX::parseMesh(FbxMesh* pFBXMesh, MeshImportData& outData, FbxAMatrix const& transform)
	{
		int32 LayerSmoothingCount = pFBXMesh->GetLayerCount(FbxLayerElement::eSmoothing);
		for (int32 i = 0; i < LayerSmoothingCount; i++)
		{
			FbxLayerElementSmoothing const* SmoothingInfo = pFBXMesh->GetLayer(0)->GetSmoothing();
			if (SmoothingInfo && SmoothingInfo->GetMappingMode() != FbxLayerElement::eByPolygon)
			{
				mGeometryConverter->ComputePolygonSmoothingFromEdgeSmoothing(pFBXMesh, i);
			}
		}

		if (!pFBXMesh->IsTriangleMesh())
		{
			const bool bReplace = true;
			FbxNodeAttribute* ConvertedNode = mGeometryConverter->Triangulate(pFBXMesh, bReplace);

			if (ConvertedNode != nullptr && ConvertedNode->GetAttributeType() == FbxNodeAttribute::eMesh)
			{
				pFBXMesh = (FbxMesh*)ConvertedNode;
			}
			else
			{
				return false; // not clean, missing some dealloc
			}
		}

		int polygonCount = pFBXMesh->GetPolygonCount();
		FBXVertexFormat vertexFormat;
		GetMeshVertexFormat(pFBXMesh, vertexFormat);

		auto const GetColorData = [](FBXElementDataInfo const& elementInfo, int vertexId, int controlId) -> Vector4 { return elementInfo.getData< Vector4, FbxGeometryElementVertexColor >(controlId, vertexId); };
		auto const GetTexcoordData = [](FBXElementDataInfo const& elementInfo, int vertexId, int controlId) -> Vector2 { return elementInfo.getData< Vector2, FbxGeometryElementUV >(controlId, vertexId); };
		auto const GetNormalData = [](FBXElementDataInfo const& elementInfo, int vertexId, int controlId) -> Vector4 { return elementInfo.getData< Vector4, FbxGeometryElementNormal >(controlId, vertexId); };
		auto const GetTangentData = [](FBXElementDataInfo const& elementInfo, int vertexId, int controlId) -> Vector4 { return elementInfo.getData< Vector4, FbxGeometryElementTangent >(controlId, vertexId); };

		struct FBXImportSetting
		{
			float positionScale = 1.0;
			bool  bAddTangleAndNormal = true;
		};

		FBXImportSetting importSetting;
		importSetting.positionScale = 1;
		vertexFormat.getMeshInputLayout(outData.desc, importSetting.bAddTangleAndNormal);
		int vertexSize = outData.desc.getVertexSize(0);
		int numVertices = pFBXMesh->GetPolygonVertexCount();
		outData.vertices.resize(numVertices * vertexSize);
		auto GetBufferData = [&outData](EVertex::Attribute attribute) -> uint8*
		{
			int offset = outData.desc.getAttributeOffset(attribute);
			return (offset < 0) ? nullptr : &outData.vertices[offset];
		};

		uint8* pPosition = GetBufferData(EVertex::ATTRIBUTE_POSITION);
		uint8* pColor = GetBufferData(EVertex::ATTRIBUTE_COLOR);
		uint8* pNormal = vertexFormat.normals.empty() ? nullptr : GetBufferData(EVertex::ATTRIBUTE_NORMAL);
		uint8* pTangent = vertexFormat.tangents.empty() ? nullptr : GetBufferData(EVertex::ATTRIBUTE_TANGENT);
		uint8* pTexcoord = GetBufferData(EVertex::ATTRIBUTE_TEXCOORD);

		auto baseLayer = pFBXMesh->GetLayer(0);
		FbxLayerElementMaterial* layerElementMaterial = baseLayer->GetMaterials();
		FbxLayerElement::EMappingMode materialMappingMode = layerElementMaterial ? layerElementMaterial->GetMappingMode() : FbxLayerElement::eByPolygon;

		int numTriangles = 0;
		int vertexId = 0;
		for (int idxPolygon = 0; idxPolygon < polygonCount; idxPolygon++)
		{
			int lPolygonSize = pFBXMesh->GetPolygonSize(idxPolygon);
			if (lPolygonSize >= 3) numTriangles += (lPolygonSize - 2);

			for (int idxPolyVertex = 0; idxPolyVertex < lPolygonSize; ++idxPolyVertex)
			{
				int controlId = pFBXMesh->GetPolygonVertex(idxPolygon, idxPolyVertex);
				if (pPosition)
				{
					FbxVector4 vLocal = pFBXMesh->GetControlPoints()[controlId];
					// Revert to Old Behavior: USE RAW DATA, DON'T TRANSFORM IN IMPORTER
					*((Vector3*)pPosition) = importSetting.positionScale * Vector3(vLocal.mData[0], vLocal.mData[1], vLocal.mData[2]);
					pPosition += vertexSize;
				}

				if (pColor)
				{
					*((Vector4*)pColor) = GetColorData(vertexFormat.colors[0], vertexId, controlId);
					pColor += vertexSize;
				}

				if (pNormal)
				{
					// Revert to Old Behavior: USE RAW DATA, DON'T TRANSFORM IN IMPORTER
					Vector4 n = GetNormalData(vertexFormat.normals[0], vertexId, controlId);
					*((Vector3*)pNormal) = Vector3(n.x, n.y, n.z);
					pNormal += vertexSize;
				}

				if (pTangent)
				{
					*((Vector4*)pTangent) = GetTangentData(vertexFormat.tangents[0], vertexId, controlId);
					pTangent += vertexSize;
				}

				if (pTexcoord)
				{
					for (int i = 0; i < vertexFormat.texcoords.size(); ++i)
					{
						Vector2* pV = (Vector2*)(pTexcoord);
						pV[i] = GetTexcoordData(vertexFormat.texcoords[i], vertexId, controlId);
					}
					pTexcoord += vertexSize;
				}
				++vertexId;
			}
		} // for polygonCount

		outData.indices.resize(numTriangles * 3);
		uint32* pIndex = &outData.indices[0];
		vertexId = 0;

		for (int idxPolygon = 0; idxPolygon < polygonCount; idxPolygon++)
		{
			int lPolygonSize = pFBXMesh->GetPolygonSize(idxPolygon);
			int32 materialIndex = 0;

			if (layerElementMaterial)
			{
				switch (materialMappingMode)
				{
				case FbxLayerElement::eAllSame:
					materialIndex = layerElementMaterial->GetIndexArray().GetAt(0);
					break;
				case FbxLayerElement::eByPolygon:
					materialIndex = layerElementMaterial->GetIndexArray().GetAt(idxPolygon);
					break;
				default:
					break;
				}
			}

			if (lPolygonSize < 3) { vertexId += lPolygonSize; continue; }
			if (lPolygonSize == 3)
			{
				for (int i = 0; i < 3; ++i)
				{
					*pIndex = vertexId;
					++vertexId;
					++pIndex;
				}
			}
		}

		if (importSetting.bAddTangleAndNormal)
		{
			// CORRECT LOGIC: Fix the missing normal generation if tangents exist
			if (pNormal == nullptr)
			{
				if (pTangent == nullptr)
					MeshUtility::FillTriangleListNormalAndTangent(outData.desc, outData.vertices.data(), numVertices, outData.indices.data(), outData.indices.size());
				else
					MeshUtility::FillTriangleListNormal(outData.desc, outData.vertices.data(), numVertices, outData.indices.data(), outData.indices.size());
			}
			else if (pTangent == nullptr)
			{
				MeshUtility::FillTriangleListTangent(outData.desc, outData.vertices.data(), numVertices, outData.indices.data(), outData.indices.size());
			}
		}

		outData.numVertices = numVertices;
		return true;
	}

	bool MeshImporterFBX::createMesh(MeshImportData& meshData, Mesh &outMesh)
	{
		outMesh.mType = EPrimitive::TriangleList;
		outMesh.mInputLayoutDesc = std::move(meshData.desc);
		outMesh.mSections = std::move(meshData.sections);
		bool result = outMesh.createRHIResource(meshData.vertices.data(), meshData.numVertices, meshData.indices.data(), meshData.indices.size());
		return result;
	}

}