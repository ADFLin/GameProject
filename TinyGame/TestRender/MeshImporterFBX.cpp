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
		importSetting.positionScale = 0.1;
		vertexFormat.getMeshInputLayout(outMesh.mInputLayoutDesc, importSetting.bAddTangleAndNormal);
		int vertexSize = outMesh.mInputLayoutDesc.getVertexSize(0);
		std::vector< uint8 > vertexBufferData;
		int numVertices = pFBXMesh->GetPolygonVertexCount();
		vertexBufferData.resize(numVertices * vertexSize);

		auto GetBufferData = [&outMesh, &vertexBufferData](Vertex::Attribute attribute) -> uint8*
		{
			int offset = outMesh.mInputLayoutDesc.getAttributeOffset(attribute);
			if (offset < 0)
				return nullptr;
			return &vertexBufferData[offset];
		};


		uint8* pPosition = GetBufferData(Vertex::ATTRIBUTE_POSITION);
		uint8* pColor = GetBufferData(Vertex::ATTRIBUTE_COLOR);
		uint8* pNormal = vertexFormat.normals.empty() ? nullptr : GetBufferData(Vertex::ATTRIBUTE_NORMAL);
		uint8* pTangent = vertexFormat.tangents.empty() ? nullptr : GetBufferData(Vertex::ATTRIBUTE_TANGENT);
		uint8* pTexcoord = GetBufferData(Vertex::ATTRIBUTE_TEXCOORD);

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
						*((Vector2*)(pTexcoord)+i) = GetTexcoordData(vertexFormat.texcoords[i], vertexId, controlId);
					}
					pTexcoord += vertexSize;
				}
				++vertexId;
			}

		} // for polygonCount

		std::vector< int > indexBufferData;
		indexBufferData.resize(numTriangles * 3);
		int* pIndex = &indexBufferData[0];
		vertexId = 0;

		for (int idxPolygon = 0; idxPolygon < polygonCount; idxPolygon++)
		{
			int lPolygonSize = pFBXMesh->GetPolygonSize(idxPolygon);
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
					MeshUtility::FillTriangleListTangent(outMesh.mInputLayoutDesc, vertexBufferData.data(), numVertices, indexBufferData.data(), indexBufferData.size());
				}
				else
				{
					MeshUtility::FillTriangleListNormalAndTangent(outMesh.mInputLayoutDesc, vertexBufferData.data(), numVertices, indexBufferData.data(), indexBufferData.size());
				}
			}
		}

		outMesh.mType = EPrimitive::TriangleList;
		bool result = outMesh.createRHIResource(vertexBufferData.data(), numVertices, indexBufferData.data(), indexBufferData.size(), true);

		return result;
	}

}