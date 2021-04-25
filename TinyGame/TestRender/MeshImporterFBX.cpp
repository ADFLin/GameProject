#include "MeshImporterFBX.h"

#include "Core/ScopeExit.h"
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

		bool lImportStatus = mImporter->Initialize(filePath, -1, mManager->GetIOSettings());

		mScene->Clear();
		bool lStatus = mImporter->Import(mScene);
		FbxNode* pRootNode = mScene->GetRootNode();

		if (pRootNode)
		{
			for (int i = 0; i < pRootNode->GetChildCount(); i++)
			{
				FbxNode* pNode = pRootNode->GetChild(i);

				if (pNode->GetNodeAttribute())
				{
					FbxNodeAttribute::EType attributeType = pNode->GetNodeAttribute()->GetAttributeType();
					switch (attributeType)
					{
					case FbxNodeAttribute::eMesh:
						return parseMesh((FbxMesh*)pNode->GetNodeAttribute(), outMesh);
						break;
					default:
						break;
					}
				}
			}
		}
		return false;
	}

	void MeshImporterFBX::GetMeshVertexFormat(FbxMesh* pMesh, FBXVertexFormat& outFormat)
	{
		auto AddElementData = [&outFormat](int indexBuffer, FbxLayerElement* pElement, std::vector< FBXElementDataInfo >& elements)
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
		std::vector< FBXPolygon > polygons(polygonCount);

		FBXVertexFormat vertexFormat;
		GetMeshVertexFormat(pFBXMesh, vertexFormat);

		auto const GetColorData = [](FBXElementDataInfo const& elementInfo, int vertexId, int controlId) -> Vector4
		{
			return elementInfo.getData< Vector4, FbxGeometryElementVertexColor >(controlId, vertexId);
		};
		auto const GetTexcoordData = [](FBXElementDataInfo const& elementInfo, int vertexId, int controlId) -> Vector2
		{
			return elementInfo.getData< Vector2, FbxGeometryElementUV >(controlId, vertexId);
		};
		auto const GetNormalData = [](FBXElementDataInfo const& elementInfo, int vertexId, int controlId) -> Vector4
		{
			return elementInfo.getData< Vector4, FbxGeometryElementNormal >(controlId, vertexId);
		};
		auto const GetTangentData = [](FBXElementDataInfo const& elementInfo, int vertexId, int controlId) -> Vector4
		{
			return elementInfo.getData< Vector4, FbxGeometryElementTangent >(controlId, vertexId);
		};
		auto const GetBinormalData = [pFBXMesh](FBXElementDataInfo const& elementInfo, int vertexId, int controlId) -> Vector4
		{
			return elementInfo.getData< Vector4, FbxGeometryElementBinormal >(controlId, vertexId);
		};


		struct FBXImportSetting
		{
			float positionScale = 1.0;
			bool  bAddTangleAndNormal = true;

		};

		FBXImportSetting importSetting;
		importSetting.positionScale = 1;
		vertexFormat.getMeshInputLayout(outMesh.mInputLayoutDesc, importSetting.bAddTangleAndNormal);
		int vertexSize = outMesh.mInputLayoutDesc.getVertexSize(0);
		std::vector< uint8 > vertexData;
		int numVertices = pFBXMesh->GetPolygonVertexCount();
		vertexData.resize(numVertices * vertexSize);

		auto GetBufferData = [&outMesh, &vertexData](EVertex::Attribute attribute) -> uint8*
		{
			int offset = outMesh.mInputLayoutDesc.getAttributeOffset(attribute);
			if (offset < 0)
				return nullptr;
			return &vertexData[offset];
		};


		uint8* pPosition = GetBufferData(EVertex::ATTRIBUTE_POSITION);
		uint8* pColor = GetBufferData(EVertex::ATTRIBUTE_COLOR);
		uint8* pNormal = vertexFormat.normals.empty() ? nullptr : GetBufferData(EVertex::ATTRIBUTE_NORMAL);
		uint8* pTangent = vertexFormat.tangents.empty() ? nullptr : GetBufferData(EVertex::ATTRIBUTE_TANGENT);
		uint8* pTexcoord = GetBufferData(EVertex::ATTRIBUTE_TEXCOORD);

		auto baseLayer = pFBXMesh->GetLayer(0);
		FbxLayerElementMaterial* layerElementMaterial = baseLayer->GetMaterials();
		FbxLayerElement::EMappingMode materialMappingMode = layerElementMaterial ?
			layerElementMaterial->GetMappingMode() : FbxLayerElement::eByPolygon;


		struct IndexSectionInfo
		{
			uint32 index;
			int    section;
		};

		int numTriangles = 0;
		int vertexId = 0;
		for (int idxPolygon = 0; idxPolygon < polygonCount; idxPolygon++)
		{
			FBXPolygon& polygon = polygons[idxPolygon];
			int lPolygonSize = pFBXMesh->GetPolygonSize(idxPolygon);

			//DisplayInt("        Polygon ", idxPolygon);
			for (int i = 0; i < pFBXMesh->GetElementPolygonGroupCount(); i++)
			{
				FbxGeometryElementPolygonGroup* lePolgrp = pFBXMesh->GetElementPolygonGroup(i);
				switch (lePolgrp->GetMappingMode())
				{
				case FbxGeometryElement::eByPolygon:
					if (lePolgrp->GetReferenceMode() == FbxGeometryElement::eIndex)
					{
						int polyGroupId = lePolgrp->GetIndexArray().GetAt(idxPolygon);
						polygon.groups.push_back(polyGroupId);
						break;
					}
				default:
					// any other mapping modes don't make sense
					break;
				}
			}


			if (lPolygonSize >= 3)
			{
				numTriangles += (lPolygonSize - 2);
			}

			for (int idxPolyVertex = 0; idxPolyVertex < lPolygonSize; ++idxPolyVertex)
			{
				int controlId = pFBXMesh->GetPolygonVertex(idxPolygon, idxPolyVertex);
				if (pPosition)
				{
					FbxVector4* pVertexPos = pFBXMesh->GetControlPoints();

					auto const& v = pVertexPos[controlId];
					*((Vector3*)pPosition) = importSetting.positionScale * Vector3(v.mData[0], v.mData[1], v.mData[2]);
					pPosition += vertexSize;
				}

				if (pColor)
				{
					*((Vector4*)pColor) = GetColorData(vertexFormat.colors[0], vertexId, controlId);
					pColor += vertexSize;
				}

				if (pNormal)
				{
					*((Vector3*)pNormal) = GetNormalData(vertexFormat.normals[0], vertexId, controlId).xyz();
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

		std::vector< int > indexData;
		indexData.resize(numTriangles * 3);
		int* pIndex = &indexData[0];
		vertexId = 0;

		for (int idxPolygon = 0; idxPolygon < polygonCount; idxPolygon++)
		{
			int lPolygonSize = pFBXMesh->GetPolygonSize(idxPolygon);

			int32 materialIndex = 0;

			if (layerElementMaterial)
			{
				switch (materialMappingMode)
				{
					// material index is stored in the IndexArray, not the DirectArray (which is irrelevant with 2009.1)
				case FbxLayerElement::eAllSame:
					{
						materialIndex = layerElementMaterial->GetIndexArray().GetAt(0);
					}
					break;
				case FbxLayerElement::eByPolygon:
					{
						materialIndex = layerElementMaterial->GetIndexArray().GetAt(idxPolygon);
					}
					break;
				}
			}

			if (lPolygonSize < 3)
			{
				vertexId += lPolygonSize;
				continue;
			}
			if (lPolygonSize == 3)
			{
				for (int i = 0; i < 3; ++i)
				{
					*pIndex = vertexId;
					++vertexId;
					++pIndex;
				}
			}
			else
			{

			}
		}

		if (importSetting.bAddTangleAndNormal)
		{
			if (pTangent == nullptr)
			{
				if (pNormal)
				{
					MeshUtility::FillTriangleListTangent(outMesh.mInputLayoutDesc, vertexData.data(), numVertices, indexData.data(), indexData.size());
				}
				else
				{
					MeshUtility::FillTriangleListNormalAndTangent(outMesh.mInputLayoutDesc, vertexData.data(), numVertices, indexData.data(), indexData.size());
				}
			}
		}

		outMesh.mType = EPrimitive::TriangleList;
		bool result = outMesh.createRHIResource(vertexData.data(), numVertices, indexData.data(), indexData.size(), true);

		return result;
	}

}