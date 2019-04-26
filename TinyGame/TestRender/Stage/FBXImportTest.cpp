#include "BRDFTestStage.h"
#include "TestRenderStageBase.h"

#include "Core/ScopeExit.h"

#include "fbxsdk.h"
#if _DEBUG
#pragma comment(lib, "debug/libfbxsdk.lib")
#else
#pragma comment(lib, "release/libfbxsdk.lib")
#endif
namespace Render
{

	class FBXImporter
	{
	public:
		FBXImporter()
		{

			mManager = FbxManager::Create();
			mIOSetting = FbxIOSettings::Create(mManager, IOSROOT);	
			mManager->SetIOSettings(mIOSetting);
			mScene = FbxScene::Create(mManager, "My Scene");

			mGeometryConverter = std::make_unique< FbxGeometryConverter >(mManager);
		}

		~FBXImporter()
		{
			cleanup();
		}

		void cleanup()
		{
			mGeometryConverter.release();
#define SAFE_DESTROY( P ) if ( P ){ P->Destroy(); P = nullptr; }
			SAFE_DESTROY(mIOSetting);
			SAFE_DESTROY(mScene);
			SAFE_DESTROY(mManager);
		}
		bool import( char const* filePath , Mesh& outMesh )
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

			if( pRootNode )
			{
				for( int i = 0; i < pRootNode->GetChildCount(); i++ )
				{
					FbxNode* pNode = pRootNode->GetChild(i);

					if( pNode->GetNodeAttribute() )
					{
						FbxNodeAttribute::EType attributeType = pNode->GetNodeAttribute()->GetAttributeType();
						switch( attributeType )
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

			template< class OutputType , class ElementType >
			OutputType getData( int controlId , int vertexId ) const
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
				inputLayout.addElement(Vertex::ATTRIBUTE_POSITION, Vertex::eFloat3);
				if( !colors.empty() )
				{
					inputLayout.addElement(Vertex::ATTRIBUTE_COLOR, Vertex::eFloat4);
				}
				if( !normals.empty() || bAddNormalTangent )
				{
					inputLayout.addElement(Vertex::ATTRIBUTE_NORMAL, Vertex::eFloat3);
				}
				if( !tangents.empty() || bAddNormalTangent )
				{
					inputLayout.addElement(Vertex::ATTRIBUTE_TANGENT, Vertex::eFloat4);
				}
				if( !texcoords.empty() )
				{
					int idxTex = 0;
					for( auto const& texInfo : texcoords )
					{
						inputLayout.addElement(Vertex::ATTRIBUTE_TEXCOORD + idxTex, Vertex::eFloat2);
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

		static void GetMeshVertexFormat(FbxMesh* pMesh, FBXVertexFormat& outFormat)
		{

			auto AddElementData = [&outFormat](int indexBuffer, FbxLayerElement* pElement, std::vector< FBXElementDataInfo >& elements)
			{
				elements.push_back({ indexBuffer , pElement ,pElement->GetMappingMode(), pElement->GetReferenceMode() });
			};

			for( int i = 0; i < pMesh->GetElementVertexColorCount(); i++ )
			{
				FbxGeometryElementVertexColor* pElement = pMesh->GetElementVertexColor(i);
				AddElementData(i, pElement, outFormat.colors);
			}
			for( int i = 0; i < pMesh->GetElementUVCount(); ++i )
			{
				FbxGeometryElementUV* pElement = pMesh->GetElementUV(i);
				AddElementData(i, pElement, outFormat.texcoords);
			}
			for( int i = 0; i < pMesh->GetElementNormalCount(); ++i )
			{
				FbxGeometryElementNormal* pElement = pMesh->GetElementNormal(i);
				AddElementData(i, pElement, outFormat.normals);

			}
			for( int i = 0; i < pMesh->GetElementTangentCount(); ++i )
			{
				FbxGeometryElementTangent* pElement = pMesh->GetElementTangent(i);
				AddElementData(i, pElement, outFormat.tangents);
			}
			for( int i = 0; i < pMesh->GetElementBinormalCount(); ++i )
			{
				FbxGeometryElementBinormal* pElement = pMesh->GetElementBinormal(i);
				AddElementData(i, pElement, outFormat.binormals);
			}
		}

		bool parseMesh(FbxMesh* pFBXMesh, Mesh& outMesh )
		{
			int32 LayerSmoothingCount = pFBXMesh->GetLayerCount(FbxLayerElement::eSmoothing);
			for( int32 i = 0; i < LayerSmoothingCount; i++ )
			{
				FbxLayerElementSmoothing const* SmoothingInfo = pFBXMesh->GetLayer(0)->GetSmoothing();
				if( SmoothingInfo && SmoothingInfo->GetMappingMode() != FbxLayerElement::eByPolygon )
				{
					mGeometryConverter->ComputePolygonSmoothingFromEdgeSmoothing(pFBXMesh, i);
				}
			}

			if( !pFBXMesh->IsTriangleMesh() )
			{
				const bool bReplace = true;
				FbxNodeAttribute* ConvertedNode = mGeometryConverter->Triangulate(pFBXMesh, bReplace);

				if( ConvertedNode != NULL && ConvertedNode->GetAttributeType() == FbxNodeAttribute::eMesh )
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
				return elementInfo.getData< Vector4 , FbxGeometryElementVertexColor > (controlId, vertexId);
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

			auto GetBufferData = [&outMesh, &vertexBufferData](Vertex::Semantic semantic) -> uint8*
			{
				int offset = outMesh.mInputLayoutDesc.getSematicOffset(semantic);
				if( offset < 0 )
					return nullptr;
				return &vertexBufferData[offset];
			};


			uint8* pPosition = GetBufferData(Vertex::ePosition);
			uint8* pColor = GetBufferData(Vertex::eColor);
			uint8* pNormal = vertexFormat.normals.empty() ? nullptr : GetBufferData(Vertex::eNormal);
			uint8* pTangent = vertexFormat.tangents.empty() ? nullptr : GetBufferData(Vertex::eTangent);
			uint8* pTexcoord = GetBufferData(Vertex::eTexcoord);

			int numTriangles = 0;
			int vertexId = 0;
			for( int idxPolygon = 0; idxPolygon < polygonCount; idxPolygon++ )
			{
				FBXPolygon& polygon = polygons[idxPolygon];
				int lPolygonSize = pFBXMesh->GetPolygonSize(idxPolygon);

				//DisplayInt("        Polygon ", idxPolygon);
				for( int i = 0; i < pFBXMesh->GetElementPolygonGroupCount(); i++ )
				{
					FbxGeometryElementPolygonGroup* lePolgrp = pFBXMesh->GetElementPolygonGroup(i);
					switch( lePolgrp->GetMappingMode() )
					{
					case FbxGeometryElement::eByPolygon:
						if( lePolgrp->GetReferenceMode() == FbxGeometryElement::eIndex )
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

				if( lPolygonSize >= 3 )
				{
					numTriangles += (lPolygonSize - 2);
				}

				for( int idxPolyVertex = 0; idxPolyVertex < lPolygonSize; ++idxPolyVertex )
				{
					int controlId = pFBXMesh->GetPolygonVertex(idxPolygon, idxPolyVertex);
					if( pPosition )
					{
						FbxVector4* pVertexPos = pFBXMesh->GetControlPoints();

						auto const& v = pVertexPos[controlId];
						*((Vector3*)pPosition) = importSetting.positionScale * Vector3(v.mData[0], v.mData[1], v.mData[2]);
						pPosition += vertexSize;
					}

					if( pColor )
					{
						*((Vector4*)pColor) = GetColorData(vertexFormat.colors[0], vertexId, controlId);
						pColor += vertexSize;
					}

					if( pNormal )
					{
						*((Vector3*)pNormal) = GetNormalData(vertexFormat.normals[0], vertexId, controlId).xyz();
						pNormal += vertexSize;
					}

					if( pTangent )
					{
						*((Vector4*)pTangent) = GetTangentData(vertexFormat.tangents[0], vertexId, controlId);
						pTangent += vertexSize;
					}

					if( pTexcoord )
					{
						for( int i = 0; i < vertexFormat.texcoords.size(); ++i )
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

			for( int idxPolygon = 0; idxPolygon < polygonCount; idxPolygon++ )
			{
				int lPolygonSize = pFBXMesh->GetPolygonSize(idxPolygon);
				if( lPolygonSize < 3 )
				{
					vertexId += lPolygonSize;
					continue;
				}
				if( lPolygonSize == 3 )
				{
					for( int i = 0; i < 3; ++i )
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

			if( importSetting.bAddTangleAndNormal )
			{
				if( pTangent == nullptr )
				{
					if( pNormal )
					{
						MeshUtility::FillTriangleListTangent(outMesh.mInputLayoutDesc, vertexBufferData.data(), numVertices, indexBufferData.data(), indexBufferData.size());
					}
					else
					{
						MeshUtility::FillTriangleListNormalAndTangent(outMesh.mInputLayoutDesc, vertexBufferData.data(), numVertices, indexBufferData.data(), indexBufferData.size());
					}
				}
			}

			outMesh.mType = PrimitiveType::TriangleList;
			bool result = outMesh.createRHIResource(vertexBufferData.data(), numVertices, indexBufferData.data(), indexBufferData.size(), true);

			return result;
		}
	
		FbxManager*    mManager = nullptr;
		FbxIOSettings* mIOSetting = nullptr;
		FbxScene*      mScene = nullptr;
		std::unique_ptr< FbxGeometryConverter > mGeometryConverter;


	};
	class EnvTestProgram : public ShaderProgram
	{
		typedef ShaderProgram BaseClass;
		virtual void bindParameters(ShaderParameterMap& parameterMap)
		{
			BaseClass::bindParameters(parameterMap);
			mParamIBL.bindParameters(parameterMap);
		}
	public:
		IBLShaderParameters mParamIBL;
	};

	class FBXImportTestStage : public BRDFTestStage
	{
		typedef BRDFTestStage BaseClass;
	public:
		FBXImportTestStage()
		{


		}

		EnvTestProgram mTestShader;

		Mesh mMesh;
		RHITexture2DRef mDiffuseTexture;
		RHITexture2DRef mNormalTexture;
		RHITexture2DRef mMetalTexture;
		RHITexture2DRef mRoughnessTexture;
		bool mbUseMipMap = true;
		bool mbUseShaderBlit = false;
		float mSkyLightInstensity = 1.0;
		virtual bool onInit()
		{
			if( !BaseClass::onInit() )
				return false;

			VERIFY_RETURN_FALSE(ShaderManager::Get().loadFile(mTestShader , "Shader/Game/EnvLightingTest", SHADER_ENTRY(MainVS), SHADER_ENTRY(MainPS) , nullptr ) );
			VERIFY_RETURN_FALSE(mDiffuseTexture = RHIUtility::LoadTexture2DFromFile("Mesh/Cerberus/Cerberus_A.tga" , TextureLoadOption().SRGB().MipLevel(10).ReverseH()));
			VERIFY_RETURN_FALSE(mNormalTexture = RHIUtility::LoadTexture2DFromFile("Mesh/Cerberus/Cerberus_N.tga", TextureLoadOption().MipLevel(10).ReverseH()));
			VERIFY_RETURN_FALSE(mMetalTexture = RHIUtility::LoadTexture2DFromFile("Mesh/Cerberus/Cerberus_M.tga", TextureLoadOption().MipLevel(10).ReverseH()));
			VERIFY_RETURN_FALSE(mRoughnessTexture = RHIUtility::LoadTexture2DFromFile("Mesh/Cerberus/Cerberus_R.tga", TextureLoadOption().MipLevel(10).ReverseH()));

			FBXImporter importer;
			
			char const* filePath = "Mesh/Cerberus/Cerberus_LP.FBX";
			VERIFY_RETURN_FALSE( importer.import(filePath, mMesh) );

			auto frame = ::Global::GUI().findTopWidget< DevFrame >();
			WidgetPropery::Bind(frame->addCheckBox(UI_ANY, "Use MinpMap"), mbUseMipMap);
			WidgetPropery::Bind(frame->addSlider(UI_ANY), mSkyLightInstensity , 0 , 10 );
			WidgetPropery::Bind(frame->addCheckBox(UI_ANY, "Use Shader Blit"), mbUseShaderBlit);
 			return true;
		}

		virtual void onEnd()
		{
			mTestShader.releaseRHI();
			BaseClass::onEnd();
		}

		void onRender(float dFrame)
		{

			Vec2i screenSize = ::Global::GetDrawEngine().getScreenSize();
			RHICommandList& commandList = RHICommandList::GetImmediateList();
			initializeRenderState();

			{
				GPU_PROFILE("Scene");
				mSceneRenderTargets.attachDepthTexture();
				GL_BIND_LOCK_OBJECT(mSceneRenderTargets.getFrameBuffer());


				glClearColor(0.2, 0.2, 0.2, 1);
				glClearDepth(mViewFrustum.bUseReverse ? 0 : 1);
				glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

				{
					GPU_PROFILE("SkyBox");
					RHISetDepthStencilState(commandList, StaticDepthDisableState::GetRHI());
					RHISetRasterizerState(commandList, TStaticRasterizerState< ECullMode::None >::GetRHI());

					RHISetShaderProgram(commandList, mProgSkyBox->getRHIResource());
					mProgSkyBox->setTexture(commandList, SHADER_PARAM(Texture), *mHDRImage);
					switch( SkyboxShowIndex )
					{
					case ESkyboxShow::Normal:
						mProgSkyBox->setTexture(commandList, SHADER_PARAM(CubeTexture), mIBLResource.texture);
						mProgSkyBox->setParam(commandList, SHADER_PARAM(CubeLevel), float(0));
						break;
					case ESkyboxShow::Irradiance:
						mProgSkyBox->setTexture(commandList, SHADER_PARAM(CubeTexture), mIBLResource.irradianceTexture);
						mProgSkyBox->setParam(commandList, SHADER_PARAM(CubeLevel), float(0));
						break;
					default:
						mProgSkyBox->setTexture(commandList, SHADER_PARAM(CubeTexture), mIBLResource.perfilteredTexture,
												TStaticSamplerState< Sampler::eTrilinear, Sampler::eClamp, Sampler::eClamp, Sampler::eClamp > ::GetRHI());
						mProgSkyBox->setParam(commandList, SHADER_PARAM(CubeLevel), float(SkyboxShowIndex - ESkyboxShow::Prefiltered_0));
					}

					mView.setupShader(commandList, *mProgSkyBox);
					mSkyBox.drawShader(commandList);
				}

				RHISetDepthStencilState(commandList, TStaticDepthStencilState<>::GetRHI());
				{
					RHISetupFixedPipelineState(commandList, mView.worldToView, mView.viewToClip);
					DrawUtility::AixsLine(commandList);
				}

				{
					RHISetShaderProgram(commandList, mTestShader.getRHIResource());
					mView.setupShader(commandList, mTestShader);

					auto& samplerState = (mbUseMipMap) ? TStaticSamplerState<Sampler::eTrilinear>::GetRHI() : TStaticSamplerState<Sampler::eBilinear>::GetRHI();
					mTestShader.setTexture(commandList, SHADER_PARAM(DiffuseTexture), mDiffuseTexture , samplerState);
					mTestShader.setTexture(commandList, SHADER_PARAM(NormalTexture), mNormalTexture, samplerState);
					mTestShader.setTexture(commandList, SHADER_PARAM(MetalTexture), mMetalTexture, samplerState);
					mTestShader.setTexture(commandList, SHADER_PARAM(RoughnessTexture), mRoughnessTexture, samplerState);
					mTestShader.setParam(commandList, SHADER_PARAM(SkyLightInstensity), mSkyLightInstensity);
					mTestShader.mParamIBL.setParameters(commandList, mTestShader, mIBLResource);
					mMesh.drawShader(commandList);
				}
				if ( 0 )
				{
					GPU_PROFILE("LightProbe Visualize");

					RHISetShaderProgram(commandList, mProgVisualize->getRHIResource());
					mProgVisualize->setStructuredUniformBufferT< LightProbeVisualizeParams >(commandList, *mParamBuffer.getRHI());
					mProgVisualize->setTexture(commandList, SHADER_PARAM(NormalTexture), mNormalTexture);
					mView.setupShader(commandList, *mProgVisualize);
					mProgVisualize->setParameters(commandList, mIBLResource);
					RHIDrawPrimitiveInstanced(commandList, PrimitiveType::Quad, 0, 4, mParams.gridNum.x * mParams.gridNum.y);
				}

			}


			RHISetDepthStencilState(commandList, StaticDepthDisableState::GetRHI());
			{
				RHISetViewport(commandList, 0, 0, screenSize.x, screenSize.y);
				OrthoMatrix matProj(0, screenSize.x, 0, screenSize.y, -1, 1);
				MatrixSaveScope matScope(matProj);
				DrawUtility::DrawTexture(commandList, *mHDRImage, IntVector2(10, 10), IntVector2(512, 512));
			}

			if( bEnableTonemap )
			{
				GPU_PROFILE("Tonemap");

				mSceneRenderTargets.swapFrameTexture();
				PostProcessContext context;
				context.mInputTexture[0] = &mSceneRenderTargets.getPrevFrameTexture();

				GL_BIND_LOCK_OBJECT(mSceneRenderTargets.getFrameBuffer());

				RHISetShaderProgram(commandList, mProgTonemap->getRHIResource());
				RHISetRasterizerState(commandList, TStaticRasterizerState< ECullMode::None >::GetRHI());
				RHISetDepthStencilState(commandList, StaticDepthDisableState::GetRHI());
				RHISetBlendState(commandList, TStaticBlendState< CWM_RGB >::GetRHI());
				mProgTonemap->setParameters(commandList, context);
				DrawUtility::ScreenRectShader(commandList);
			}

			{
				GPU_PROFILE("Blit To Screen");
				if( mbUseShaderBlit )
				{
					ShaderHelper::Get().copyTextureToBuffer(commandList, mSceneRenderTargets.getFrameTexture());
				}
				else
				{
					mSceneRenderTargets.getFrameBuffer().blitToBackBuffer();
				}
			}
		}

	};

	REGISTER_STAGE2("FBX Import Test", FBXImportTestStage, EStageGroup::FeatureDev, 1);

}