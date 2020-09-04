#pragma once
#ifndef FBXImporter_H_DC2F076B_F3B4_478B_BB14_836A29F8F52D
#define FBXImporter_H_DC2F076B_F3B4_478B_BB14_836A29F8F52D

#include "RHI/RHICommon.h"
#include "Renderer/MeshImportor.h"

#include "fbxsdk.h"

#if _DEBUG
#pragma comment(lib, "debug/libfbxsdk.lib")
#else
#pragma comment(lib, "release/libfbxsdk.lib")
#endif
namespace Render
{
	class MeshImporterFBX : public IMeshImporter
	{
	public:
		MeshImporterFBX();

		~MeshImporterFBX();

		void cleanup();
		bool importFromFile(char const* filePath, Mesh& outMesh, MeshImportSettings* settings) override;


		struct FBXConv
		{
			static Vector4 To(FbxColor const& value)
			{
				return Vector4(value.mRed, value.mGreen, value.mBlue, value.mAlpha);
			}
			static Vector4 To(FbxVector4 const& value)
			{
				return Vector4(value.mData[0], value.mData[1], value.mData[2], value.mData[3]);
			}
			static Vector2 To(FbxVector2 const& value)
			{
				return Vector2(value.mData[0], value.mData[1]);
			}
		};

		struct FBXElementDataInfo
		{
			int   index;
			FbxLayerElement* buffer;
			FbxLayerElement::EMappingMode   mappingMode;
			FbxLayerElement::EReferenceMode referenceMode;

			template< class OutputType, class ElementType >
			OutputType getData(int controlId, int vertexId) const
			{
				ElementType* pDataBuffer = (ElementType*)(buffer);
				int index = (mappingMode == FbxLayerElement::eByControlPoint) ? controlId : vertexId;
				index = (referenceMode == FbxLayerElement::eDirect) ? index : pDataBuffer->GetIndexArray().GetAt(index);
				return FBXConv::To(pDataBuffer->GetDirectArray()[index]);
			}
		};
		struct FBXVertexFormat
		{
			std::vector< FBXElementDataInfo > colors;
			std::vector< FBXElementDataInfo > normals;
			std::vector< FBXElementDataInfo > tangents;
			std::vector< FBXElementDataInfo > binormals;
			std::vector< FBXElementDataInfo > texcoords;

			void getMeshInputLayout(InputLayoutDesc& inputLayout, bool bAddNormalTangent)
			{
				inputLayout.addElement(0, Vertex::ATTRIBUTE_POSITION, Vertex::eFloat3);
				if (!colors.empty())
				{
					inputLayout.addElement(0, Vertex::ATTRIBUTE_COLOR, Vertex::eFloat4);
				}
				if (!normals.empty() || bAddNormalTangent)
				{
					inputLayout.addElement(0, Vertex::ATTRIBUTE_NORMAL, Vertex::eFloat3);
				}
				if (!tangents.empty() || bAddNormalTangent)
				{
					inputLayout.addElement(0, Vertex::ATTRIBUTE_TANGENT, Vertex::eFloat4);
				}
				if (!texcoords.empty())
				{
					int idxTex = 0;
					for (auto const& texInfo : texcoords)
					{
						inputLayout.addElement(0, Vertex::ATTRIBUTE_TEXCOORD + idxTex, Vertex::eFloat2);
					}
				}
			}

			int numVertexElementData;
			int numPolygonVertexElementData;
		};

		struct FBXPolygon
		{
			int polygonId;
			std::vector< int > groups;
		};

		static void GetMeshVertexFormat(FbxMesh* pMesh, FBXVertexFormat& outFormat);

		bool parseMesh(FbxMesh* pFBXMesh, Mesh& outMesh);

		FbxManager*    mManager = nullptr;
		FbxIOSettings* mIOSetting = nullptr;
		FbxScene*      mScene = nullptr;
		std::unique_ptr< FbxGeometryConverter > mGeometryConverter;
	};
}
#endif // FBXImporter_H_DC2F076B_F3B4_478B_BB14_836A29F8F52D
