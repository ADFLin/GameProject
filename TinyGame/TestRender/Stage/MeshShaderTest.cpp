#include "TestRenderStageBase.h"

#include "RHI/GLExtensions.h"
#include "RHI/GlobalShader.h"

#include "Renderer/MeshUtility.h"
#include "RHI/RHIGraphics2D.h"

#include "Simplygon/SimplygonSDK.h"
#include "Simplygon/SimplygonSDKLoader.h"
#include "RHI/OpenGLCommand.h"
#include "RHI/RHICommandListImpl.h"

#define USE_SHADER_STATS 1
namespace Render
{
	struct MeshletRenderResource
	{

		TStructuredBuffer< MeshletData>  mMeshletBuffer;
		TStructuredBuffer< uint32 >  mPrimitiveIndexBuffer;
		TStructuredBuffer< uint32 >  mVertexIndexBuffer;
		TStructuredBuffer< MeshletCullData > mCullDataBuffer;


		int getMeshletCount()
		{
			return mMeshletBuffer.getElementNum();
		}

		bool initialize(Mesh& mesh, DataCacheInterface& dataCache, DataCacheKey const& key)
		{
			int const maxVertices = 64;
			int const maxPrimitives = 126;
			std::vector<MeshletData> meshlets;
			std::vector<PackagedTriangleIndices> primitiveIndices;
			std::vector<uint8> uniqueVertexIndices;
			std::vector<MeshletCullData> cullDataList;

			auto DataLoad = [&](IStreamSerializer& serializer) -> bool
			{
				serializer >> meshlets;
				serializer >> primitiveIndices;
				serializer >> uniqueVertexIndices;
				serializer >> cullDataList;
				return true;
			};

			auto DataSave = [&](IStreamSerializer& serializer) -> bool
			{
				serializer << meshlets;
				serializer << primitiveIndices;
				serializer << uniqueVertexIndices;
				serializer << cullDataList;
				return true;
			};
			if ( true || !dataCache.loadDelegate(key, DataLoad))
			{
				VERIFY_RETURN_FALSE(mesh.buildMeshlet(maxVertices, maxPrimitives, meshlets, uniqueVertexIndices, primitiveIndices, &cullDataList));

				if (!dataCache.saveDelegate(key, DataSave))
				{

				}
			}

			VERIFY_RETURN_FALSE(mVertexIndexBuffer.initializeResource(uniqueVertexIndices.size() / sizeof(uint32), EStructuredBufferType::Buffer));
			mVertexIndexBuffer.updateBuffer(TArrayView<uint32>((uint32*)uniqueVertexIndices.data(), uniqueVertexIndices.size() / sizeof(uint32)));

			VERIFY_RETURN_FALSE(mMeshletBuffer.initializeResource(meshlets.size(), EStructuredBufferType::Buffer));
			mMeshletBuffer.updateBuffer(MakeView(meshlets));

			VERIFY_RETURN_FALSE(mPrimitiveIndexBuffer.initializeResource(primitiveIndices.size(), EStructuredBufferType::Buffer));
			mPrimitiveIndexBuffer.updateBuffer(TArrayView<uint32>((uint32*)primitiveIndices.data(), primitiveIndices.size()));

			VERIFY_RETURN_FALSE(mCullDataBuffer.initializeResource(cullDataList.size(), EStructuredBufferType::Buffer));
			mCullDataBuffer.updateBuffer(MakeView(cullDataList));

			return true;
		}

		bool initialize(Mesh& mesh)
		{
			int const maxVertices = 64;
			int const maxPrimitives = 126;
			std::vector<MeshletData> meshlets;
			std::vector<PackagedTriangleIndices> primitiveIndices;
			std::vector<uint8> uniqueVertexIndices;
			std::vector< MeshletCullData > cullDataList;
			VERIFY_RETURN_FALSE( mesh.buildMeshlet(maxVertices, maxPrimitives, meshlets, uniqueVertexIndices, primitiveIndices, &cullDataList) );

			VERIFY_RETURN_FALSE( mVertexIndexBuffer.initializeResource(uniqueVertexIndices.size() / sizeof(uint32), EStructuredBufferType::Buffer) );
			mVertexIndexBuffer.updateBuffer(TArrayView<uint32>((uint32*)uniqueVertexIndices.data(), uniqueVertexIndices.size() / sizeof(uint32)));

			VERIFY_RETURN_FALSE(mMeshletBuffer.initializeResource(meshlets.size(), EStructuredBufferType::Buffer));
			mMeshletBuffer.updateBuffer(MakeView(meshlets));

			VERIFY_RETURN_FALSE(mPrimitiveIndexBuffer.initializeResource(primitiveIndices.size(), EStructuredBufferType::Buffer));
			mPrimitiveIndexBuffer.updateBuffer(TArrayView<uint32>((uint32*)primitiveIndices.data(), primitiveIndices.size()));

			VERIFY_RETURN_FALSE(mCullDataBuffer.initializeResource(cullDataList.size(), EStructuredBufferType::Buffer));
			mCullDataBuffer.updateBuffer(MakeView(cullDataList));

			return true;
		}

		void releaseRHI()
		{
			mMeshletBuffer.releaseResources();
			mPrimitiveIndexBuffer.releaseResources();
			mVertexIndexBuffer.releaseResources();
			mCullDataBuffer.releaseResources();
		}
	};

	struct MeshletShaderParameters
	{
		void bindParameters(ShaderParameterMap const& parameterMap)
		{
			BIND_SHADER_PARAM(parameterMap, MeshletDataBlock);
			BIND_SHADER_PARAM(parameterMap, PrimitiveIndicesBlock);
			BIND_SHADER_PARAM(parameterMap, VertexIndicesBlock);
		}

		template< class TShader >
		void setParameters(RHICommandList& commandList, TShader& shader, MeshletRenderResource& resource)
		{
			shader.setStorageBuffer(commandList, SHADER_MEMBER_PARAM(MeshletDataBlock), *resource.mMeshletBuffer.getRHI());
			shader.setStorageBuffer(commandList, SHADER_MEMBER_PARAM(PrimitiveIndicesBlock), *resource.mPrimitiveIndexBuffer.getRHI());
			shader.setStorageBuffer(commandList, SHADER_MEMBER_PARAM(VertexIndicesBlock), *resource.mVertexIndexBuffer.getRHI());
		}

		DEFINE_SHADER_PARAM(MeshletDataBlock);
		DEFINE_SHADER_PARAM(PrimitiveIndicesBlock);
		DEFINE_SHADER_PARAM(VertexIndicesBlock);
	};


	class SimpleMeshProgram : public GlobalShaderProgram
	{
		DECLARE_SHADER_PROGRAM(SimpleMeshProgram, Global)
	public:
		virtual void bindParameters(ShaderParameterMap const& parameterMap) override
		{
			mMeshletParam.bindParameters(parameterMap);
		}

		static void SetupShaderCompileOption(ShaderCompileOption& option) {}
		static char const* GetShaderFileName()
		{
			return "Shader/Test/MeshShaderTest";
		}
		MeshletShaderParameters mMeshletParam;
	};

	template< bool bUseTask >
	class TSimpleMeshProgram : public SimpleMeshProgram
	{
		DECLARE_SHADER_PROGRAM(TSimpleMeshProgram, Global)

		using BaseClass = SimpleMeshProgram;
	public:

		static void SetupShaderCompileOption(ShaderCompileOption& option) 
		{
			BaseClass::SetupShaderCompileOption(option);
			option.addDefine(SHADER_PARAM(MESH_USE_PACKED_CONE), MESH_USE_PACKED_CONE);
			option.addDefine(SHADER_PARAM(USE_STATS), USE_SHADER_STATS);
			option.addDefine(SHADER_PARAM(USE_TASK), bUseTask);
		}

		static TArrayView< ShaderEntryInfo const > GetShaderEntries()
		{
			if (bUseTask)
			{
				static ShaderEntryInfo const entriesWithTask[] =
				{
					{ EShader::Task   , SHADER_ENTRY(MainTS) },
					{ EShader::Mesh   , SHADER_ENTRY(MainMS) },
					{ EShader::Pixel  , SHADER_ENTRY(MainPS) },
				};
				return entriesWithTask;
			}
			static ShaderEntryInfo const entries[] =
			{
				{ EShader::Mesh   , SHADER_ENTRY(MainMS) },
				{ EShader::Pixel  , SHADER_ENTRY(MainPS) },
			};
			return entries;
		}

		MeshletShaderParameters mMeshletParam;
	};

	IMPLEMENT_SHADER_PROGRAM_T(template<>, TSimpleMeshProgram<false>);
	IMPLEMENT_SHADER_PROGRAM_T(template<>, TSimpleMeshProgram<true>);

	class SimpleProgram : public GlobalShaderProgram
	{
		DECLARE_SHADER_PROGRAM(SimpleProgram, Global)
	public:

		static void SetupShaderCompileOption(ShaderCompileOption& option) {}
		static char const* GetShaderFileName()
		{
			return "Shader/Test/MeshShaderTest";
		}
		static TArrayView< ShaderEntryInfo const > GetShaderEntries()
		{
			static ShaderEntryInfo const entries[] =
			{
				{ EShader::Vertex , SHADER_ENTRY(MainVS) },
				{ EShader::Pixel  , SHADER_ENTRY(MainPS) },
			};
			return entries;
		}

	};


	IMPLEMENT_SHADER_PROGRAM(SimpleProgram);

	struct GPU_ALIGN RenderStats
	{
		int renderMeshletCount;
		int renderPrimitiveCount;
		int outputVertexCount;
		int dummy;
	};


	class MeshShaderTestStage : public TestRenderStageBase
	{
		using BaseClass = TestRenderStageBase;
	public:
		MeshShaderTestStage() {}

		bool onInit() override
		{
			if (!BaseClass::onInit())
				return false;

			char const* SimplygonSDKPath = "C:/GameProject/OtherLib/Simplygon/SimplygonSDKRuntimeReleasex64.dll";

			int initval = SimplygonSDK::Initialize(&mSimplygonSDK, SimplygonSDKPath, nullptr);
			if ( initval != SimplygonSDK::SG_ERROR_NOERROR )
			{
				LogWarning( 0, "Can't Initialize Simplygon : %s", SimplygonSDK::GetError(initval));
				//return false;
			}

			IntVector2 screenSize = ::Global::GetScreenSize();
			::Global::GUI().cleanupWidget();

			auto frameWidget = WidgetUtility::CreateDevFrame();
			FWidgetProperty::Bind(frameWidget->addCheckBox(UI_ANY, "Use Mesh Shader"), bUseMeshShader);
			FWidgetProperty::Bind(frameWidget->addCheckBox(UI_ANY, "With Task Shader"), bUseTask);
			FWidgetProperty::Bind(frameWidget->addCheckBox(UI_ANY, "Hold Cull View"), bHoldCullView, 
				[this](bool bHold)
				{
					if (bHold)
					{
						mCullView.setupTransform(mView.worldToView, mView.viewToClip);
					}
				}
			);
			return true;
		}

		SimplygonSDK::ISimplygonSDK* mSimplygonSDK = nullptr;

		void GenerateLOD( Mesh& mesh )
		{
			if (mSimplygonSDK == nullptr)
				return;

			using namespace SimplygonSDK;
			spGeometryData geom = mSimplygonSDK->CreateGeometryData();


			{
				uint8* pVertex = (uint8*)RHILockBuffer(mesh.mVertexBuffer, ELockAccess::ReadOnly);
				uint8* pIndices = (uint8*)RHILockBuffer(mesh.mIndexBuffer, ELockAccess::ReadOnly);

				ON_SCOPE_EXIT
				{
					RHIUnlockBuffer(mesh.mVertexBuffer);
					RHIUnlockBuffer(mesh.mIndexBuffer);
				};

				int numVertices = mesh.mVertexBuffer->getNumElements();

				std::vector< uint32 > tempBuffer;
				int numTriangles = 0;
				uint32* pTriangleIndex = MeshUtility::ConvertToTriangleList(mesh.mType, pIndices, mesh.mIndexBuffer->getNumElements(), mesh.mIndexBuffer->isIntType(), tempBuffer, numTriangles);
				auto posReader = mesh.makePositionReader(pVertex);
				auto normalReader = mesh.makeAttributeReader(pVertex, EVertex::ATTRIBUTE_NORMAL);
				auto tangentReader = mesh.makeAttributeReader(pVertex, EVertex::ATTRIBUTE_TANGENT);
				geom->SetVertexCount(numVertices);
				geom->SetTriangleCount(numTriangles);

				spRealArray coords = geom->GetCoords();
				//geom->AddNormals();
				//spRealArray normals = geom->GetNormals();
				//geom->AddTangents(0);
				//spRealArray tangents = geom->GetTangents(0);
				spRidArray vertex_ids = geom->GetVertexIds();
				for (int i = 0; i < numVertices; ++i)
				{
					coords->SetTuple(i, &posReader.get<float>(i));
					//normals->SetTuple(i, &normalReader.get<float>(i));
					//tangents->SetTuple(i, &tangentReader.get<float>(i));
				}

				for (int i = 0; i < 3 * numTriangles; ++i)
				{
					vertex_ids->SetItem(i, pTriangleIndex[i]);
				}
			}

			spScene scene = mSimplygonSDK->CreateScene();
			spSceneMesh sceneMesh = mSimplygonSDK->CreateSceneMesh();
			sceneMesh->SetGeometry(geom);
			scene->GetRootNode()->AddChild(sceneMesh);

			spScene sceneLod = scene->NewCopy();
			spReductionProcessor reductionProcessor = mSimplygonSDK->CreateReductionProcessor();
			reductionProcessor->SetScene(sceneLod);

			///////////////////////////////////////////////////////////////////////////////////////////////
			// SETTINGS - Most of these are set to the same value by default, but are set anyway for clarity

			// The reduction settings object contains settings pertaining to the actual decimation
			spReductionSettings reductionSettings = reductionProcessor->GetReductionSettings();
			reductionSettings->SetKeepSymmetry(true); //Try, when possible to reduce symmetrically
			reductionSettings->SetUseAutomaticSymmetryDetection(true); //Auto-detect the symmetry plane, if one exists. Can, if required, be set manually instead.
			reductionSettings->SetUseHighQualityNormalCalculation(true); //Drastically increases the quality of the LODs normals, at the cost of extra processing time.
			reductionSettings->SetReductionHeuristics(SG_REDUCTIONHEURISTICS_CONSISTENT); //Choose between "fast" and "consistent" processing. Fast will look as good, but may cause inconsistent 
			//triangle counts when comparing MaxDeviation targets to the corresponding percentage targets.

			// The reducer uses importance weights for all features to decide where and how to reduce.
			// These are advanced settings and should only be changed if you have some specific reduction requirement
			/*reductionSettings->SetShadingImportance(2.f); //This would make the shading twice as important to the reducer as the other features.*/

			// The actual reduction triangle target are controlled by these settings
			reductionSettings->SetStopCondition(SG_STOPCONDITION_ANY);//The reduction stops when any of the targets below is reached
			reductionSettings->SetReductionTargets(SG_REDUCTIONTARGET_ALL);//Selects which targets should be considered when reducing
			reductionSettings->SetTriangleRatio(0.5); //Targets at 50% of the original triangle count
			reductionSettings->SetTriangleCount(10); //Targets when only 10 triangle remains
			reductionSettings->SetMaxDeviation(REAL_MAX); //Targets when an error of the specified size has been reached. As set here it never happens.
			reductionSettings->SetOnScreenSize(50); //Targets when the LOD is optimized for the selected on screen pixel size

			// The repair settings object contains settings to fix the geometries
			spRepairSettings repairSettings = reductionProcessor->GetRepairSettings();
			repairSettings->SetTjuncDist(0.0f); //Removes t-junctions with distance 0.0f
			repairSettings->SetWeldDist(0.0f); //Welds overlapping vertices

			// The normal calculation settings deal with the normal-specific reduction settings
			spNormalCalculationSettings normalSettings = reductionProcessor->GetNormalCalculationSettings();
			normalSettings->SetReplaceNormals(false); //If true, this will turn off normal handling in the reducer and recalculate them all afterwards instead.
			//If false, the reducer will try to preserve the original normals as well as possible
			/*normalSettings->SetHardEdgeAngleInRadians( 3.14159f*60.0f/180.0f ); //If the normals are recalculated, this sets the hard-edge angle.*/

			//END SETTINGS 
			///////////////////////////////////////////////////////////////////////////////////////////////

			// Add progress observer
			//reductionProcessor->AddObserver(&progressObserver, SG_EVENT_PROGRESS);

			// Run the actual processing. After this, the set geometry will have been reduced according to the settings
			reductionProcessor->RunProcessing();

			spSceneMesh sceneMeshLod = ISceneMesh::SafeCast( sceneLod->GetRootNode()->GetChild(0) );

			spGeometryData geomLod = sceneMeshLod->GetGeometry();
			LogMsg("Mesh : v = %u t = %u", geom->GetVertexCount(), geom->GetTriangleCount());
			LogMsg("MeshLod : v = %u t = %u", geomLod->GetVertexCount(), geomLod->GetTriangleCount());


			mMeshLOD.mInputLayoutDesc.addElement(0, EVertex::ATTRIBUTE_POSITION, EVertex::Float3);

			{
				spRealData posData = mSimplygonSDK->CreateRealData();
				geomLod->GetCoords()->GetData(posData);

				spRidData indexData = mSimplygonSDK->CreateRidData();
				geomLod->GetVertexIds()->GetData(indexData);

				mMeshLOD.createRHIResource((void*)posData->_GetData(), posData->_GetItemCount() / 3, (void*)indexData->_GetData(), indexData->_GetItemCount(), true);
				mMeshLOD.mType = EPrimitive::TriangleList;
			}
		}

		Mesh mMeshLOD;


		bool bHoldCullView = false;
		bool bUseTask = false;
		bool bUseMeshShader = true;

		SimpleMeshProgram* mProgSimpleMesh;
		SimpleMeshProgram* mProgSimpleMeshWithTask;
		SimpleProgram* mProgSimple;

		TStructuredBuffer< RenderStats > mRenderStatsBuffer;

		virtual ERenderSystem getDefaultRenderSystem() override
		{
			return ERenderSystem::OpenGL;
		}

		virtual bool isRenderSystemSupported(ERenderSystem systemName) override
		{
			if (systemName == ERenderSystem::D3D11)
				return false;

			return true;
		}

		virtual void configRenderSystem(ERenderSystem systenName, RenderSystemConfigs& systemConfigs) override
		{
			systemConfigs.numSamples = 1;
			systemConfigs.screenWidth = 1080;
			systemConfigs.screenHeight = 720;
		}


		virtual bool setupRenderSystem(ERenderSystem systemName) override
		{
			VERIFY_RETURN_FALSE(BaseClass::setupRenderSystem(systemName));

			VERIFY_RETURN_FALSE(SharedAssetData::createSimpleMesh());

			VERIFY_RETURN_FALSE(mRenderStatsBuffer.initializeResource(1 , EStructuredBufferType::RWBuffer, true));


			if (GRHISupportMeshShader)
			{
				auto meshId = SimpleMeshId::Doughnut;
				Mesh& mesh = mSimpleMeshs[meshId];


				mProgSimpleMesh = ShaderManager::Get().getGlobalShaderT<TSimpleMeshProgram<false>>();
				mProgSimpleMeshWithTask = ShaderManager::Get().getGlobalShaderT<TSimpleMeshProgram<true>>();
				DataCacheKey key;
				key.typeName = "MESHLET";
				key.version = "0BC5470C-D311-493F-825D-9C5AB77F3E14";
				key.keySuffix.add(meshId);
				mMeshletResource.initialize(mesh, ::Global::DataCache() , key);
				GenerateLOD(mesh);
			}
			

			mProgSimple = ShaderManager::Get().getGlobalShaderT<SimpleProgram>();

			if (systemName == ERenderSystem::OpenGL)
			{
				glEnable(GL_MULTISAMPLE);
			}

			
			mTexture = RHIUtility::LoadTexture2DFromFile("Texture/rocks.jpg");
			return true;
		}

		virtual void preShutdownRenderSystem(bool bReInit = false) override
		{
			mMeshletResource.releaseRHI();

			mTexture.release();

			BaseClass::preShutdownRenderSystem(bReInit);
		}

		ViewInfo mCullView;

		RHITexture2DRef mTexture;
		MeshletRenderResource mMeshletResource;

		void onEnd() override
		{


		}

		void restart() override
		{

		}

		void tick() override
		{

		}

		void onUpdate(long time) override
		{
			BaseClass::onUpdate(time);
		}

		void onRender(float dFrame) override
		{
			initializeRenderState();



			RHICommandList& commandList = RHICommandList::GetImmediateList();
			Vec2i screenSize = ::Global::GetScreenSize();

			int RenderMeshCount = 100;
			{
				GPU_PROFILE("Mesh");

				RHISetRasterizerState(commandList, TStaticRasterizerState<>::GetRHI());

				if ( bUseMeshShader && GRHISupportMeshShader )
				{
#if USE_SHADER_STATS
					RenderStats zeroStats;
					memset(&zeroStats, 0, sizeof(RenderStats));
					mRenderStatsBuffer.updateBuffer( TArrayView<RenderStats>( &zeroStats, 1 ) );
#endif
					SimpleMeshProgram* progSimpleMesh = (bUseTask) ? mProgSimpleMeshWithTask : mProgSimpleMesh;
					RHISetShaderProgram(commandList, progSimpleMesh->getRHIResource());
					progSimpleMesh->setStorageBuffer(commandList, SHADER_PARAM(VertexDataBlock), *mSimpleMeshs[SimpleMeshId::Doughnut].mVertexBuffer);
					progSimpleMesh->setTexture(commandList, SHADER_PARAM(Texture), *mTexture);
					progSimpleMesh->mMeshletParam.setParameters(commandList, *progSimpleMesh, mMeshletResource);
					mView.setupShader(commandList, *progSimpleMesh);
					if (bHoldCullView)
					{
						mCullView.setupShader(commandList, *progSimpleMesh, &StructuredBufferInfo(SHADER_PARAM(CullViewBlock)));

					}
					else
					{
						mView.setupShader(commandList, *progSimpleMesh, &StructuredBufferInfo(SHADER_PARAM(CullViewBlock)));
					}
					progSimpleMesh->setStorageBuffer(commandList, SHADER_PARAM(CullDataBlock), *mMeshletResource.mCullDataBuffer.getRHI());
					if (bUseTask)
					{
#if USE_SHADER_STATS
						progSimpleMesh->setAtomicCounterBuffer(commandList, SHADER_PARAM(RenderMeshletCount), *mRenderStatsBuffer.getRHI());
#endif
						progSimpleMesh->setParam(commandList, SHADER_PARAM(TaskParams), IntVector4(mMeshletResource.getMeshletCount() - 1, 0, 0, 0));
					}

					for (int i = 0; i < RenderMeshCount; ++i)
					{
						Vector3 offset;
						offset.x = i % 10;
						offset.y = i / 10;
						offset.z = 0;
						progSimpleMesh->setParam(commandList, SHADER_PARAM(LocalToWorld), Matrix4::Scale(0.15) * Matrix4::Translate(offset));
						
						if (bUseTask)
						{
							
							int const NumThread = 32;
							RHIDrawMeshTasks(commandList, ( mMeshletResource.getMeshletCount() + NumThread - 1 ) / NumThread, 1 , 1);

						}
						else
						{
							RHIDrawMeshTasks(commandList, mMeshletResource.getMeshletCount(), 1 , 1);
						}	
					}


					if (bUseTask)
					{
#if USE_SHADER_STATS
						if (GRHISystem->getName() == RHISystemName::OpenGL)
						{
							ShaderParameter param;
							progSimpleMesh->getRHIResource()->getResourceParameter(EShaderResourceType::AtomicCounter, SHADER_PARAM(RenderMeshletCount), param);
							glBindBufferBase(GL_ATOMIC_COUNTER_BUFFER, param.mLoc, 0);
						}
#endif
					}

					if (bHoldCullView)
					{
						Vector3 vertices[8];
						for (uint i = 0; i < 8; ++i)
						{
							Vector4 ndc = Vector4(2 * float((i) & 0x1 ) - 1 , 2 * float((i>>1) & 0x1) - 1 , ( i / 4 ) ? GRHIClipZMin : 0.9999 , 1 );
							Vector4 worldPosH = ndc * mCullView.clipToWorld;
							vertices[i] = worldPosH.dividedVector();
						}

						RHISetBlendState(commandList, TStaticBlendState<CWM_RGBA , EBlend::SrcAlpha, EBlend::OneMinusSrcAlpha >::GetRHI());
						RHISetRasterizerState(commandList, TStaticRasterizerState<ECullMode::None>::GetRHI());
						RHISetDepthStencilState(commandList, StaticDepthDisableState::GetRHI());
						RHISetFixedShaderPipelineState(commandList, mView.worldToClip, LinearColor(1, 0, 0, 0.15));
						static const uint32 FaceIndices[] =
						{
							0,1,5, 0,5,4,
							1,3,7, 1,7,5,
							3,2,6, 3,6,7,
							2,0,4, 2,4,6,
						};
						TRenderRT< RTVF_XYZ >::DrawIndexed(commandList, EPrimitive::TriangleList, vertices, ARRAY_SIZE(vertices), FaceIndices, ARRAY_SIZE(FaceIndices));
						static const uint32 LineIndices[] =
						{
							0, 1, 1, 3, 3, 2, 2, 0,
							4, 5, 5, 7, 7, 6, 6, 4,
							0, 4, 1, 5, 2, 6, 3, 7,
						};
						RHISetBlendState(commandList, TStaticBlendState<>::GetRHI());
						RHISetDepthStencilState(commandList, TStaticDepthStencilState<>::GetRHI());
						RHISetFixedShaderPipelineState(commandList, mView.worldToClip , LinearColor(1, 1, 0, 1));
						TRenderRT< RTVF_XYZ >::DrawIndexed(commandList, EPrimitive::LineList, vertices, ARRAY_SIZE(vertices) , LineIndices, ARRAY_SIZE(LineIndices));
					}
				}
				else
				{

					RHISetRasterizerState(commandList, TStaticRasterizerState<ECullMode::Back , EFillMode::Wireframe >::GetRHI());
					RHISetShaderProgram(commandList, mProgSimple->getRHIResource());
					mProgSimple->setTexture(commandList, SHADER_PARAM(Texture), *mTexture);
					mView.setupShader(commandList, *mProgSimple);
					for (int i = 0; i < 100; ++i)
					{
						Vector3 offset;
						offset.x = i % 10;
						offset.y = i / 10;
						offset.z = 0;
						mProgSimple->setParam(commandList, SHADER_PARAM(LocalToWorld), Matrix4::Scale(0.15) * Matrix4::Translate(offset));
						//mSimpleMeshs[SimpleMeshId::Doughnut].draw(commandList);

						if (i % 2)
						{
							mSimpleMeshs[SimpleMeshId::Doughnut].draw(commandList);
						}
						else
						{
							mMeshLOD.draw(commandList);
						}
						
					}
				}
			}




			RHIFlushCommand(commandList);

#if USE_SHADER_STATS
			if (GRHISupportMeshShader && bUseMeshShader )
			{
				RHIGraphics2D& g = Global::GetRHIGraphics2D();
				RenderStats renderStats = { 0 };

				if (GRHISystem->getName() == RHISystemName::OpenGL)
				{
					GLuint handle = OpenGLCast::GetHandle(*mRenderStatsBuffer.getRHI());
					glGetNamedBufferSubData(handle, 0, sizeof(RenderStats), &renderStats);
				}
				else
				{
					void* ptr = (RenderStats*)RHILockBuffer(mRenderStatsBuffer.getRHI(), ELockAccess::ReadOnly);
					
					
					RHIUnlockBuffer(mRenderStatsBuffer.getRHI());
				}



				
				g.beginRender();

				InlineString<512> str;
				SimpleTextLayout textLayout;
				textLayout.posX = 10;
				textLayout.posY = 10;
				textLayout.offset = 15;
				
				g.setTextColor(Color3f(1, 0, 0));
				textLayout.show(g, "MeshletCount = %u( %.2f %%)", renderStats.renderMeshletCount, 100 * float(renderStats.renderMeshletCount) / (RenderMeshCount * mMeshletResource.getMeshletCount()));
				textLayout.show(g, "PrimitiveCount = %u( %.2f %%)", renderStats.renderPrimitiveCount, 100 * float(renderStats.renderPrimitiveCount) / (RenderMeshCount * mSimpleMeshs[SimpleMeshId::Doughnut].mIndexBuffer->getNumElements() / 3));
				textLayout.show(g, "VertexCount = %u( %.2f %%)", renderStats.outputVertexCount, 100 * float(renderStats.outputVertexCount) / (RenderMeshCount * mSimpleMeshs[SimpleMeshId::Doughnut].mVertexBuffer->getNumElements()));
				g.endRender();

				
			}
#endif
		}

		bool onMouse(MouseMsg const& msg) override
		{
			if (!BaseClass::onMouse(msg))
				return false;
			return true;
		}

		bool onKey(KeyMsg const& msg) override
		{
			if (msg.isDown())
			{
				switch (msg.getCode())
				{
				default:
					break;
				}
			}
			return BaseClass::onKey(msg);
		}

		bool onWidgetEvent(int event, int id, GWidget* ui) override
		{
			switch (id)
			{
			default:
				break;
			}

			return BaseClass::onWidgetEvent(event, id, ui);
		}
	protected:
	};

	REGISTER_STAGE_ENTRY("Mesh Shader", MeshShaderTestStage, EExecGroup::FeatureDev, "Render|Test");

}