#include "PathTracingRenderer.h"


#include "RHI/ShaderManager.h"
#include "RHI/GpuProfiler.h"
#include "RHI/DrawUtility.h"

#include "Renderer/BasePassRendering.h"
#include "Renderer/SceneView.h"
#include "Renderer/MeshBuild.h"
#include "ProfileSystem.h"
#include "RHI/RHIUtility.h"



namespace PathTracing
{
	using namespace Render;

	class PathTracingSoftwarePS : public GlobalShader
	{
		using BaseClass = GlobalShader;
		DECLARE_SHADER(PathTracingSoftwarePS, Global);
	public:
		SHADER_PERMUTATION_TYPE_BOOL(UseDebugDisplay, SHADER_PARAM(USE_DEBUG_DISPLAY));
		SHADER_PERMUTATION_TYPE_BOOL(UseSplitAccumulate, SHADER_PARAM(USE_SPLIT_ACCUMULATE));
		SHADER_PERMUTATION_TYPE_BOOL(UseMIS, SHADER_PARAM(USE_MIS));
		SHADER_PERMUTATION_TYPE_BOOL(UseBVH4, SHADER_PARAM(USE_BVH4));

		using PermutationDomain = TShaderPermutationDomain<UseDebugDisplay, UseSplitAccumulate, UseMIS, UseBVH4>;

		static char const* GetShaderFileName()
		{
			return "Shader/Game/PathTracingSoftware";
		}

		static void SetupShaderCompileOption(PermutationDomain const& domain, ShaderCompileOption& option)
		{

		}

		void bindParameters(ShaderParameterMap const& parameterMap)
		{
			BIND_SHADER_PARAM(parameterMap, DisplayMode);
			BIND_SHADER_PARAM(parameterMap, BoundBoxWarningCount);
			BIND_SHADER_PARAM(parameterMap, TriangleWarningCount);
			BIND_SHADER_PARAM(parameterMap, NumObjects);
			BIND_SHADER_PARAM(parameterMap, NumEmittingObjects);
			BIND_SHADER_PARAM(parameterMap, BlendFactor);
			BIND_TEXTURE_PARAM(parameterMap, HistoryTexture);
			BIND_TEXTURE_PARAM(parameterMap, SkyTexture);
			mIBLParams.bindParameters(parameterMap);
		}

		DEFINE_SHADER_PARAM(DisplayMode);
		DEFINE_SHADER_PARAM(BoundBoxWarningCount);
		DEFINE_SHADER_PARAM(TriangleWarningCount);
		DEFINE_SHADER_PARAM(NumObjects);
		DEFINE_SHADER_PARAM(NumEmittingObjects);
		DEFINE_SHADER_PARAM(BlendFactor);
		DEFINE_TEXTURE_PARAM(HistoryTexture);
		DEFINE_TEXTURE_PARAM(SkyTexture);
		IBLShaderParameters mIBLParams;
	};


	class DenoisePS : public GlobalShader
	{
		using BaseClass = GlobalShader;
		DECLARE_SHADER(DenoisePS, Global);
	public:
		static char const* GetShaderFileName()
		{
			return "Shader/Game/RayTracing";
		}

		static void SetupShaderCompileOption(ShaderCompileOption& option)
		{

		}

		void bindParameters(ShaderParameterMap const& parameterMap)
		{
			BIND_TEXTURE_PARAM(parameterMap, FrameTexture);
			BIND_TEXTURE_PARAM(parameterMap, GBufferTextureA);
			BIND_SHADER_PARAM(parameterMap, ScreenSizeInv);
		}

		DEFINE_TEXTURE_PARAM(FrameTexture);
		DEFINE_TEXTURE_PARAM(GBufferTextureA);
		DEFINE_SHADER_PARAM(ScreenSizeInv);
	};


	class AccumulatePS : public GlobalShader
	{
		using BaseClass = GlobalShader;
		DECLARE_SHADER(AccumulatePS, Global);
	public:
		SHADER_PERMUTATION_TYPE_BOOL(UseDebugDisplay, SHADER_PARAM(USE_DEBUG_DISPLAY));
		using PermutationDomain = TShaderPermutationDomain<UseDebugDisplay>;

		static char const* GetShaderFileName()
		{
			return "Shader/Game/RayTracing";
		}

		static void SetupShaderCompileOption(PermutationDomain const& domain, ShaderCompileOption& option)
		{

		}

		void bindParameters(ShaderParameterMap const& parameterMap)
		{
			BIND_TEXTURE_PARAM(parameterMap, HistoryTexture);
			BIND_TEXTURE_PARAM(parameterMap, FrameTexture);
		}

		DEFINE_TEXTURE_PARAM(HistoryTexture);
		DEFINE_TEXTURE_PARAM(FrameTexture);
	};


	class PathTracingHardwareRayGen : public GlobalShader
	{
		using BaseClass = GlobalShader;
		DECLARE_SHADER(PathTracingHardwareRayGen, Global);
	public:

		SHADER_PERMUTATION_TYPE_BOOL(UseDebugDisplay, SHADER_PARAM(USE_DEBUG_DISPLAY));
		SHADER_PERMUTATION_TYPE_BOOL(UseMIS, SHADER_PARAM(USE_MIS));


		using PermutationDomain = TShaderPermutationDomain<UseDebugDisplay, UseMIS>;

		static void SetupShaderCompileOption(PermutationDomain const& domain, ShaderCompileOption& option)
		{

		}

		static char const* GetShaderFileName()
		{
			return "Shader/Game/PathTracingHardware";
		}

		void bindParameters(ShaderParameterMap const& parameterMap)
		{
			BIND_SHADER_PARAM(parameterMap, Outputradiance);
			BIND_SHADER_PARAM(parameterMap, OutputNormal);
			BIND_SHADER_PARAM(parameterMap, NumObjects);
			BIND_SHADER_PARAM(parameterMap, NumEmittingObjects);
			BIND_SHADER_PARAM(parameterMap, BlendFactor);
			BIND_TEXTURE_PARAM(parameterMap, SkyTexture);
			BIND_SHADER_PARAM(parameterMap, Objects);
			BIND_SHADER_PARAM(parameterMap, Materials);
			BIND_SHADER_PARAM(parameterMap, MeshVertices);
			BIND_SHADER_PARAM(parameterMap, EmittingObjectIds);
			BIND_SHADER_PARAM(parameterMap, gScene);
			BIND_TEXTURE_PARAM(parameterMap, HistoryTexture);
			BIND_SHADER_PARAM(parameterMap, DisplayMode);
			mIBLParams.bindParameters(parameterMap);
		}

		DEFINE_SHADER_PARAM(Outputradiance);
		DEFINE_SHADER_PARAM(OutputNormal);
		DEFINE_SHADER_PARAM(NumObjects);
		DEFINE_SHADER_PARAM(NumEmittingObjects);
		DEFINE_SHADER_PARAM(BlendFactor);
		DEFINE_TEXTURE_PARAM(SkyTexture);
		DEFINE_SHADER_PARAM(Objects);
		DEFINE_SHADER_PARAM(Materials);
		DEFINE_SHADER_PARAM(MeshVertices);
		DEFINE_SHADER_PARAM(EmittingObjectIds);
		DEFINE_SHADER_PARAM(gScene);
		DEFINE_SHADER_PARAM(DisplayMode);
		DEFINE_SHADER_PARAM(TriangleWarningCount);
		DEFINE_TEXTURE_PARAM(HistoryTexture);
		IBLShaderParameters mIBLParams;
	};

	class PathTracingHardwareClosestHit : public GlobalShader
	{
		using BaseClass = GlobalShader;
		DECLARE_SHADER(PathTracingHardwareClosestHit, Global);
	public:
		static char const* GetShaderFileName()
		{
			return "Shader/Game/PathTracingHardware";
		}

		void bindParameters(ShaderParameterMap const& parameterMap)
		{
			BIND_SHADER_PARAM(parameterMap, Objects);
			BIND_SHADER_PARAM(parameterMap, MeshVertices);
			BIND_SHADER_PARAM(parameterMap, Meshes);
		}

		DEFINE_SHADER_PARAM(Objects);
		DEFINE_SHADER_PARAM(MeshVertices);
		DEFINE_SHADER_PARAM(Meshes);
	};

	class PathTracingHardwareMiss : public GlobalShader
	{
		using BaseClass = GlobalShader;
		DECLARE_SHADER(PathTracingHardwareMiss, Global);
	public:
		static char const* GetShaderFileName()
		{
			return "Shader/Game/PathTracingHardware";
		}
	};

	class PathTracingHardwareSphereIntersection : public GlobalShader
	{
		using BaseClass = GlobalShader;
		DECLARE_SHADER(PathTracingHardwareSphereIntersection, Global);
	public:
		static char const* GetShaderFileName()
		{
			return "Shader/Game/PathTracingHardware";
		}

		void bindParameters(ShaderParameterMap const& parameterMap)
		{
			BIND_SHADER_PARAM(parameterMap, Objects);
		}

		DEFINE_SHADER_PARAM(Objects);
	};

	class PathTracingHardwareCubeIntersection : public GlobalShader
	{
		using BaseClass = GlobalShader;
		DECLARE_SHADER(PathTracingHardwareCubeIntersection, Global);
	public:
		static char const* GetShaderFileName()
		{
			return "Shader/Game/PathTracingHardware";
		}

		void bindParameters(ShaderParameterMap const& parameterMap)
		{
			BIND_SHADER_PARAM(parameterMap, Objects);
		}

		DEFINE_SHADER_PARAM(Objects);
	};

	IMPLEMENT_SHADER(PathTracingSoftwarePS, EShader::Pixel, SHADER_ENTRY(MainPS));
	IMPLEMENT_SHADER(AccumulatePS, EShader::Pixel, SHADER_ENTRY(AccumulatePS));
	IMPLEMENT_SHADER(DenoisePS, EShader::Pixel, SHADER_ENTRY(DenoisePS));

	IMPLEMENT_SHADER(PathTracingHardwareRayGen, EShader::RayGen, SHADER_ENTRY(PathTraceRayGen));
	IMPLEMENT_SHADER(PathTracingHardwareClosestHit, EShader::RayClosestHit, SHADER_ENTRY(PathTraceClosestHit));
	IMPLEMENT_SHADER(PathTracingHardwareMiss, EShader::RayMiss, SHADER_ENTRY(PathTraceMiss));
	IMPLEMENT_SHADER(PathTracingHardwareSphereIntersection, EShader::RayIntersection, SHADER_ENTRY(SphereIntersection));
	IMPLEMENT_SHADER(PathTracingHardwareCubeIntersection, EShader::RayIntersection, SHADER_ENTRY(CubeIntersection));


	void RenderResourceData::buildSceneResource(RHICommandList& commnadList, SceneData& sceneData)
	{
		TIME_SCOPE("RenderResourceData.buildSceneResource");

		mDebugPrimitives.clear();

		BVHTree objectBVH;
		TArray< BVHTree::Primitive > primitives;
		primitives.resize(sceneData.objects.size());
		for (int i = 0; i < sceneData.objects.size(); ++i)
		{
			auto const& object = sceneData.objects[i];
			BVHTree::Primitive& primitive = primitives[i];
			primitive.id = i;

			primitive.bound.invalidate();
			switch (object.type)
			{
			case OBJ_SPHERE:
				{
					float r = object.meta.x;
					primitive.center = object.pos;
					primitive.bound.min = -Vector3(r, r, r);
					primitive.bound.max = Vector3(r, r, r);
					primitive.bound.translate(primitive.center);
				}
				break;
			case OBJ_CUBE:
				{
					Vector3 halfSize = object.meta;
					primitive.center = object.pos;
					primitive.bound = GetAABB(object.rotation, halfSize);
					primitive.bound.translate(primitive.center);
				}
				break;
			case OBJ_TRIANGLE_MESH:
				if (sceneData.meshes.isValidIndex(AsValue<int32>(object.meta.x)))
				{
					MeshData& mesh = sceneData.meshes[AsValue<int32>(object.meta.x)];
					float scale = object.meta.y;

					Vector3 boundMin = mMeshBVHNodes[mesh.nodeIndex].boundMin;
					Vector3 boundMax = mMeshBVHNodes[mesh.nodeIndex].boundMax;

					Vector3 offset = (scale * 0.5f) * (boundMax + boundMin);
					Vector3 halfSize = (scale * 0.5f) * (boundMax - boundMin);

					primitive.center = object.pos + object.rotation.rotate(offset);
					primitive.bound = GetAABB2(offset, object.rotation, halfSize);
					primitive.bound.translate(object.pos);
				}
				break;
			case OBJ_QUAD:
				{
					Vector3 halfSize = object.meta;
					primitive.center = object.pos;
					primitive.bound = GetAABB(object.rotation, halfSize);
					primitive.bound.translate(primitive.center);
				}
				break;
			}

			addDebugAABB(primitive.bound);
		}

		BVHTreeBuilder builder(objectBVH);
		builder.minSplitPrimitiveCount = 2;
		builder.build(MakeConstView(primitives));

		TArray< BVHNodeData > nodes;
		TArray< int32 > objectIds;
		FBVH::Generate(objectBVH, nodes, objectIds);
		mSceneBVHNodeBuffer.initializeResource(nodes, EStructuredBufferType::Buffer);
		mObjectIdBuffer.initializeResource(objectIds, EStructuredBufferType::Buffer);

		TArray< BVH4NodeData > nodesV4;
		TArray< int32 > objectIdsV4;
		FBVH::Generate(objectBVH, nodesV4, objectIdsV4);
		mSceneBVH4NodeBuffer.initializeResource(nodesV4, EStructuredBufferType::Buffer);
		mObjectIdBufferV4.initializeResource(objectIdsV4, EStructuredBufferType::Buffer);

		auto UpdateBuffer = [&](auto& buffer, auto const& data, int& currentCount, EStructuredBufferType type)
		{
			uint32 bufferCapacity = buffer.getElementNum();
			if (bufferCapacity < (uint32)data.size())
			{
				buffer.initializeResource((uint32)data.size() * 2, type, BCF_None);
				RHIUpdateBuffer(*buffer.getRHI(), 0, (uint32)data.size(), (void*)data.data());
			}
			else
			{
				RHIUpdateBuffer(*buffer.getRHI(), 0, (uint32)data.size(), (void*)data.data());
			}
			currentCount = (uint32)data.size();
		};

		UpdateBuffer(mObjectBuffer, sceneData.objects, mNumObjects, EStructuredBufferType::Buffer);
		UpdateBuffer(mMaterialBuffer, sceneData.materials, mNumMaterials, EStructuredBufferType::Buffer);

		TArray<int32> emittingObjectIds;
		for (int i = 0; i < sceneData.objects.size(); ++i)
		{
			int matId = sceneData.objects[i].materialId;
			if (matId >= 0 && matId < sceneData.materials.size())
			{
				auto const& emissive = sceneData.materials[matId].emissiveColor;
				if (emissive.r > 0 || emissive.g > 0 || emissive.b > 0)
				{
					emittingObjectIds.push_back(i);
				}
			}
		}

		mNumEmittingObjects = emittingObjectIds.size();
		if (emittingObjectIds.empty())
		{
			emittingObjectIds.push_back(0); // Dummy fallback
		}
		mEmittingObjectIdBuffer.initializeResource(emittingObjectIds, EStructuredBufferType::Buffer);

		mSceneBVHNodes = nodes;
		mSceneObjectIds = objectIds;

		if (GRHISupportRayTracing)
		{
			buildSceneResourceHW(commnadList, sceneData);
		}
	}


	void RenderResourceData::buildSceneResourceHW(RHICommandList& commandList, SceneData& sceneData)
	{
		TArray<RayTracingInstanceDesc> instances;
		for (int i = 0; i < (int)sceneData.objects.size(); ++i)
		{
			auto const& obj = sceneData.objects[i];
			RHIBottomLevelAccelerationStructure* blas = nullptr;
			Matrix4 transform = Matrix4::Identity();

			if (obj.type == OBJ_TRIANGLE_MESH)
			{
				int meshId = AsValue<int32>(obj.meta.x);
				if (mSceneMeshesBLAS.isValidIndex(meshId) && mSceneMeshesBLAS[meshId].isValid())
				{
					blas = mSceneMeshesBLAS[meshId];
					transform = Matrix4::Scale(obj.meta.y) * Matrix4::Rotate(obj.rotation) * Matrix4::Translate(obj.pos);
				}
			}
			else if (obj.type == OBJ_SPHERE)
			{
				blas = mSphereBLAS;
				transform = Matrix4::Scale(obj.meta.x) * Matrix4::Translate(obj.pos);
			}
			else if (obj.type == OBJ_CUBE)
			{
				blas = mCubeBLAS;
				transform = Matrix4::Scale(obj.meta) * Matrix4::Rotate(obj.rotation) * Matrix4::Translate(obj.pos);
			}
			else if (obj.type == OBJ_QUAD)
			{
				blas = mQuadBLAS;
				transform = Matrix4::Scale(obj.meta.x, obj.meta.y, 1.0f) * Matrix4::Rotate(obj.rotation) * Matrix4::Translate(obj.pos);
			}

			if (blas)
			{
				RayTracingInstanceDesc desc;
				desc.instanceID = i;
				desc.recordIndex = (obj.type == OBJ_SPHERE) ? 1 : ((obj.type == OBJ_CUBE) ? 2 : 0);
				desc.flags = (obj.type == OBJ_SPHERE || obj.type == OBJ_CUBE) ? 0 : (uint32)ERayTracingInstanceFlags::ForceOpaque;
				desc.transform = transform;
				desc.blas = blas;
				instances.push_back(desc);
			}
		}

		if (!instances.empty())
		{
			if (!mTLAS.isValid() || mNumTLASInstances < (uint32)instances.size())
			{
				if (mNumTLASInstances < (uint32)instances.size())
				{
					mNumTLASInstances = (uint32)instances.size() * 2;
				}
				mTLAS = RHICreateTopLevelAccelerationStructure(mNumTLASInstances, EAccelerationStructureBuildFlags::PreferFastTrace | EAccelerationStructureBuildFlags::AllowUpdate);
				mTLAS->setDebugName("SceneTLAS");
			}
			mCurTLASInstanceCount = (uint32)instances.size();
			RHIUpdateTopLevelAccelerationStructureInstances(commandList, mTLAS, instances.data(), (uint32)instances.size());
			RHIBuildAccelerationStructure(commandList, mTLAS, nullptr, nullptr, mCurTLASInstanceCount);
			RHIResourceTransition(commandList, { (RHIResource*)mTLAS.get() }, EResourceTransition::UAVBarrier);
		}
	}

	bool RenderResourceData::updateMeshResource(TArrayView< MeshData const > meshes, int indexUpdateStart)
	{
		bool bVertexBufferRebuild = false;

		if (indexUpdateStart < (int)mSceneMeshesBLAS.size())
		{
			mSceneMeshesBLAS.resize(indexUpdateStart);
		}

		if (indexUpdateStart < (int)meshes.size())
		{
			mNumMeshes = Math::Min(mNumMeshes, indexUpdateStart);
			mNumVertices = Math::Min(mNumVertices, meshes[indexUpdateStart].startIndex);
			mNumBVHNodes = Math::Min(mNumBVHNodes, meshes[indexUpdateStart].nodeIndex);
			mNumBVH4Nodes = Math::Min(mNumBVH4Nodes, meshes[indexUpdateStart].nodeIndexV4);
		}
		else if (meshes.empty())
		{
			mNumMeshes = 0;
			mNumVertices = 0;
			mNumBVHNodes = 0;
			mNumBVH4Nodes = 0;
			mSceneMeshesBLAS.clear();
		}


		UpdateBuffer(mMeshBuffer, meshes, mNumMeshes, EStructuredBufferType::StaticBuffer, indexUpdateStart);
		mMeshBuffer.mResource->setDebugName("MeshBuffer");
		bVertexBufferRebuild = UpdateBuffer(mVertexBuffer, mMeshVertices, mNumVertices, EStructuredBufferType::StaticBuffer, indexUpdateStart < meshes.size() ? meshes[indexUpdateStart].startIndex : -1);
		mVertexBuffer.mResource->setDebugName("VertexBuffer");

		UpdateBuffer(mBVHNodeBuffer, mMeshBVHNodes, mNumBVHNodes, EStructuredBufferType::StaticBuffer, indexUpdateStart < meshes.size() ? meshes[indexUpdateStart].nodeIndex : -1);
		mBVHNodeBuffer.mResource->setDebugName("BVHNodeBuffer");
		UpdateBuffer(mBVH4NodeBuffer, mMeshBVH4Nodes, mNumBVH4Nodes, EStructuredBufferType::StaticBuffer, indexUpdateStart < meshes.size() ? meshes[indexUpdateStart].nodeIndexV4 : -1);
		mBVH4NodeBuffer.mResource->setDebugName("BVH4NodeBuffer");

		if (GRHISupportRayTracing)
		{
			if (bVertexBufferRebuild)
			{
				mSceneMeshesBLAS.clear();
			}

			for (int i = (int)mSceneMeshesBLAS.size(); i < (int)meshes.size(); ++i)
			{
				auto const& mesh = meshes[i];
				RayTracingGeometryDesc geoDesc;
				geoDesc.vertexBuffer = mVertexBuffer.getRHI();
				geoDesc.vertexCount = mesh.numTriangles * 3;
				geoDesc.vertexStride = sizeof(MeshVertexData);
				geoDesc.vertexOffset = mesh.startIndex * sizeof(MeshVertexData);
				geoDesc.bOpaque = true;

				RHIBottomLevelAccelerationStructureRef blas = RHICreateBottomLevelAccelerationStructure(&geoDesc, 1, EAccelerationStructureBuildFlags::PreferFastTrace);
				blas->setDebugName("MeshBlas");
				RHIBuildAccelerationStructure(RHICommandList::GetImmediateList(), blas, nullptr, nullptr);
				mSceneMeshesBLAS.push_back(blas);
			}
		}

		return true;
	}

	bool RenderResourceData::updateSingleMeshResource(TArrayView< MeshData const > meshes, int meshId, SceneData& sceneData)
	{
		auto const& mesh = meshes[meshId];

		UpdateBuffer(mVertexBuffer, mMeshVertices, mNumVertices, EStructuredBufferType::StaticBuffer, mesh.startIndex);
		UpdateBuffer(mBVHNodeBuffer, mMeshBVHNodes, mNumBVHNodes, EStructuredBufferType::StaticBuffer, mesh.nodeIndex);
		UpdateBuffer(mBVH4NodeBuffer, mMeshBVH4Nodes, mNumBVH4Nodes, EStructuredBufferType::StaticBuffer, mesh.nodeIndexV4);

		if (GRHISupportRayTracing)
		{
			auto& commandList = RHICommandList::GetImmediateList();
			RayTracingGeometryDesc geoDesc;
			geoDesc.vertexBuffer = mVertexBuffer.getRHI();
			geoDesc.vertexCount = mesh.numTriangles * 3;
			geoDesc.vertexStride = sizeof(MeshVertexData);
			geoDesc.vertexOffset = mesh.startIndex * sizeof(MeshVertexData);
			geoDesc.bOpaque = true;

			RHIBottomLevelAccelerationStructureRef blas = RHICreateBottomLevelAccelerationStructure(&geoDesc, 1, EAccelerationStructureBuildFlags::PreferFastTrace);
			blas->setDebugName("MeshBlas");
			RHIBuildAccelerationStructure(commandList, blas, nullptr, nullptr);
			mSceneMeshesBLAS[meshId] = blas;

			if (mTLAS.isValid())
			{
				bool bAnyUpdated = false;
				for (int i = 0; i < (int)sceneData.objects.size(); ++i)
				{
					auto const& obj = sceneData.objects[i];
					if (obj.type == OBJ_TRIANGLE_MESH && AsValue<int32>(obj.meta.x) == meshId)
					{
						RHIBottomLevelAccelerationStructure* objBlas = nullptr;
						int meshIdInObj = AsValue<int32>(obj.meta.x);
						if (mSceneMeshesBLAS.isValidIndex(meshIdInObj) && mSceneMeshesBLAS[meshIdInObj].isValid())
						{
							objBlas = mSceneMeshesBLAS[meshIdInObj];
						}
						if (objBlas)
						{
							RayTracingInstanceDesc desc;
							desc.instanceID = i;
							desc.recordIndex = 0;
							desc.flags = (uint32)ERayTracingInstanceFlags::ForceOpaque;
							desc.transform = Matrix4::Scale(obj.meta.y) * Matrix4::Rotate(obj.rotation) * Matrix4::Translate(obj.pos);
							desc.blas = blas.get();
							RHIUpdateTopLevelAccelerationStructureInstances(commandList, mTLAS, &desc, 1, i);
							bAnyUpdated = true;
						}
					}
				}

				if (bAnyUpdated)
				{
					RHIBuildAccelerationStructure(commandList, mTLAS, nullptr, nullptr, mCurTLASInstanceCount);
					RHIResourceTransition(commandList, { (RHIResource*)mTLAS.get() }, EResourceTransition::UAVBarrier);
				}
			}

			RHIFlushCommand(commandList);
		}

		return true;
	}


	RHITexture2D* Renderer::render(RHICommandList& commandList, RenderConfig const& config, ViewInfo& view, FrameRenderTargets& sceneRenderTargets)
	{
		Vec2i const& screenSize = view.rectSize;

		RHIResourceTransition(commandList, { (RHIResource*)&sceneRenderTargets.getFrameTexture(),
											 (RHIResource*)&sceneRenderTargets.getGBuffer().getRenderTexture(EGBuffer::A) }, EResourceTransition::RenderTarget);
		RHIResourceTransition(commandList, { (RHIResource*)&sceneRenderTargets.getPrevFrameTexture() }, EResourceTransition::SRV);

		sceneRenderTargets.getFrameBuffer()->setTexture(1, sceneRenderTargets.getGBuffer().getRenderTexture(EGBuffer::A));
		sceneRenderTargets.getFrameBuffer()->removeDepth();
		RHISetFrameBuffer(commandList, sceneRenderTargets.getFrameBuffer());

		RHISetBlendState(commandList, TStaticBlendState<>::GetRHI());
		RHISetDepthStencilState(commandList, StaticDepthDisableState::GetRHI());
		RHISetRasterizerState(commandList, TStaticRasterizerState<ECullMode::None>::GetRHI());

		if (config.bUseHardwareRayTracing && GRHISupportRayTracing)
		{
			GPU_PROFILE("PathTracingHardware");
			// Check if resources are ready
			PathTracingHardwareRayGen::PermutationDomain domain;
			domain.set<PathTracingHardwareRayGen::UseDebugDisplay>(config.debugDisplayMode != EDebugDsiplayMode::None);
			domain.set<PathTracingHardwareRayGen::UseMIS>(config.bUseMIS);
			PathTracingHardwareRayGen* rayGenShader = ShaderManager::Get().getGlobalShaderT<PathTracingHardwareRayGen>(domain);

			if (!mRayTracingPSO.isValid() || rayGenShader != mLastRayGenShader)
			{
				RHIFlushCommand(commandList);

				mLastRayGenShader = rayGenShader;
				RayTracingPipelineStateInitializer initializer;
				initializer.rayGenShader = ShaderManager::Get().getGlobalShaderT<PathTracingHardwareRayGen>(domain)->getRHI();
				initializer.missShader = ShaderManager::Get().getGlobalShaderT<PathTracingHardwareMiss>()->getRHI();
				initializer.hitGroups.resize(3);
				initializer.hitGroups[0].closestHitShader = ShaderManager::Get().getGlobalShaderT<PathTracingHardwareClosestHit>()->getRHI();

				initializer.hitGroups[1].closestHitShader = initializer.hitGroups[0].closestHitShader;
				initializer.hitGroups[1].intersectionShader = ShaderManager::Get().getGlobalShaderT<PathTracingHardwareSphereIntersection>()->getRHI();

				initializer.hitGroups[2].closestHitShader = initializer.hitGroups[0].closestHitShader;
				initializer.hitGroups[2].intersectionShader = ShaderManager::Get().getGlobalShaderT<PathTracingHardwareCubeIntersection>()->getRHI();

				// PathPayload: float3 radiance (12), float3 throughput (12), float3 nextOrigin (12), float3 nextDir (12), uint seed (4), bool done (4), float3 normal (12), float dist (4), int matId (4), int objId (4), bool hit (4)
				// Actually maxPayloadSize should be large enough.
				initializer.maxPayloadSize = 88;
				initializer.maxAttributeSize = 2 * sizeof(float);
				initializer.maxRecursionDepth = 2;
				mRayTracingPSO = RHICreateRayTracingPipelineState(initializer);
				mSBT = RHICreateRayTracingShaderTable(mRayTracingPSO);
			}

			if (mRayTracingPSO.isValid())
			{
				RHIResourceTransition(commandList, { (RHIResource*)&sceneRenderTargets.getFrameTexture(), (RHIResource*)&sceneRenderTargets.getGBuffer().getRenderTexture(EGBuffer::A) }, EResourceTransition::UAV);
				RHISetRayTracingPipelineState(commandList, mRayTracingPSO, mSBT);


				PathTracingHardwareClosestHit* closestHitShader = ShaderManager::Get().getGlobalShaderT<PathTracingHardwareClosestHit>();
				SET_SHADER_TEXTURE_AND_SAMPLER(commandList, *rayGenShader, HistoryTexture, sceneRenderTargets.getPrevFrameTexture(), TStaticSamplerState<>::GetRHI());
				rayGenShader->setRWTexture(commandList, SHADER_PARAM(Outputradiance), sceneRenderTargets.getFrameTexture(), 0, EAccessOp::WriteOnly);
				rayGenShader->setRWTexture(commandList, SHADER_PARAM(OutputNormal), sceneRenderTargets.getGBuffer().getRenderTexture(EGBuffer::A), 0, EAccessOp::WriteOnly);
				rayGenShader->setParam(commandList, SHADER_PARAM(NumObjects), mNumObjects);
				rayGenShader->setParam(commandList, SHADER_PARAM(NumEmittingObjects), mNumEmittingObjects);
				rayGenShader->setParam(commandList, SHADER_PARAM(DisplayMode), (int32)config.debugDisplayMode);
				float blendFactor = 1.0f / float(Math::Min(view.frameCount, 4096) + 1);
				rayGenShader->setParam(commandList, SHADER_PARAM(BlendFactor), blendFactor);
				if (mIBLResource.texture.isValid())
				{
					SET_SHADER_TEXTURE_AND_SAMPLER(commandList, *rayGenShader, SkyTexture, *mIBLResource.texture, TStaticSamplerState<>::GetRHI());
					rayGenShader->mIBLParams.setParameters(commandList, *rayGenShader, mIBLResource);
				}

				rayGenShader->setStorageBuffer(commandList, SHADER_PARAM(Materials), *mMaterialBuffer.getRHI(), EAccessOp::ReadOnly);
				rayGenShader->setStorageBuffer(commandList, SHADER_PARAM(Objects), *mObjectBuffer.getRHI(), EAccessOp::ReadOnly);
				rayGenShader->setStorageBuffer(commandList, SHADER_PARAM(EmittingObjectIds), *mEmittingObjectIdBuffer.getRHI(), EAccessOp::ReadOnly);
				closestHitShader->setStorageBuffer(commandList, SHADER_PARAM(Materials), *mMaterialBuffer.getRHI(), EAccessOp::ReadOnly);
				closestHitShader->setStorageBuffer(commandList, SHADER_PARAM(Objects), *mObjectBuffer.getRHI(), EAccessOp::ReadOnly);

				PathTracingHardwareSphereIntersection* sphereIntersection = ShaderManager::Get().getGlobalShaderT<PathTracingHardwareSphereIntersection>();
				PathTracingHardwareCubeIntersection* cubeIntersection = ShaderManager::Get().getGlobalShaderT<PathTracingHardwareCubeIntersection>();
				sphereIntersection->setStorageBuffer(commandList, SHADER_PARAM(Objects), *mObjectBuffer.getRHI(), EAccessOp::ReadOnly);
				cubeIntersection->setStorageBuffer(commandList, SHADER_PARAM(Objects), *mObjectBuffer.getRHI(), EAccessOp::ReadOnly);

				SetStructuredStorageBuffer(commandList, *closestHitShader, mVertexBuffer);
				SetStructuredStorageBuffer(commandList, *closestHitShader, mMeshBuffer);


				view.setupShader(commandList, *rayGenShader);
				RHISetShaderAccelerationStructure(commandList, rayGenShader->getRHI(), "gScene", mTLAS);


				RHIDispatchRays(commandList, screenSize.x, screenSize.y, 1);
				RHIResourceTransition(commandList, { (RHIResource*)&sceneRenderTargets.getFrameTexture() }, EResourceTransition::SRV);
			}
		}
		else
		{
			PathTracingSoftwarePS::PermutationDomain permutationVector;


			if (config.debugDisplayMode != EDebugDsiplayMode::None)
			{
				permutationVector.set<PathTracingSoftwarePS::UseDebugDisplay>(true);
				permutationVector.set<PathTracingSoftwarePS::UseSplitAccumulate>(false);
				permutationVector.set<PathTracingSoftwarePS::UseMIS>(false);
			}
			else
			{
				permutationVector.set<PathTracingSoftwarePS::UseDebugDisplay>(false);
				permutationVector.set<PathTracingSoftwarePS::UseSplitAccumulate>(config.bSplitAccumulate);
				permutationVector.set<PathTracingSoftwarePS::UseMIS>(config.bUseMIS);
			}

			permutationVector.set<PathTracingSoftwarePS::UseBVH4>(config.bUseBVH4);
			PathTracingSoftwarePS* pathTracingPS = ShaderManager::Get().getGlobalShaderT<PathTracingSoftwarePS>(permutationVector);

			GPU_PROFILE("PathTracing");
			GraphicsShaderStateDesc state;
			state.vertex = mScreenVS->getRHI();
			state.pixel = pathTracingPS->getRHI();
			RHISetGraphicsShaderBoundState(commandList, state);

			if (config.debugDisplayMode != EDebugDsiplayMode::None)
			{
				pathTracingPS->setParam(commandList, SHADER_PARAM(DisplayMode), int(config.debugDisplayMode));
				pathTracingPS->setParam(commandList, SHADER_PARAM(BoundBoxWarningCount), int(config.boundBoxWarningCount));
				pathTracingPS->setParam(commandList, SHADER_PARAM(TriangleWarningCount), int(config.triangleWarningCount));
			}

			view.setupShader(commandList, *pathTracingPS);
			SetStructuredStorageBuffer(commandList, *pathTracingPS, mMaterialBuffer);
			SetStructuredStorageBuffer(commandList, *pathTracingPS, mObjectBuffer);
			SetStructuredStorageBuffer(commandList, *pathTracingPS, mVertexBuffer);
			SetStructuredStorageBuffer(commandList, *pathTracingPS, mMeshBuffer);
			if (config.bUseBVH4)
			{
				SetStructuredStorageBuffer(commandList, *pathTracingPS, mBVH4NodeBuffer);
			}
			else
			{
				SetStructuredStorageBuffer(commandList, *pathTracingPS, mBVHNodeBuffer);
			}

			SetStructuredStorageBuffer< SceneBVHNodeData >(commandList, *pathTracingPS, mSceneBVHNodeBuffer);
			if (config.bUseBVH4 && 0)
			{
				SetStructuredStorageBuffer< SceneBVH4NodeData >(commandList, *pathTracingPS, mSceneBVH4NodeBuffer);
				SetStructuredStorageBuffer< ObjectIdData >(commandList, *pathTracingPS, mObjectIdBufferV4);
			}
			else
			{
				SetStructuredStorageBuffer< ObjectIdData >(commandList, *pathTracingPS, mObjectIdBuffer);
			}

			if (mEmittingObjectIdBuffer.isValid())
			{
				SetStructuredStorageBuffer< EmittingObjectIdData >(commandList, *pathTracingPS, mEmittingObjectIdBuffer);
			}

			pathTracingPS->setParam(commandList, SHADER_PARAM(NumObjects), mNumObjects);
			pathTracingPS->setParam(commandList, SHADER_PARAM(NumEmittingObjects), mNumEmittingObjects);

			float blendFactor = 1.0f / float(Math::Min(view.frameCount, 4096) + 1);
			pathTracingPS->setParam(commandList, SHADER_PARAM(BlendFactor), blendFactor);
			SET_SHADER_TEXTURE_AND_SAMPLER(commandList, *pathTracingPS, HistoryTexture, sceneRenderTargets.getPrevFrameTexture(), TStaticSamplerState<>::GetRHI());
			if (mIBLResource.texture.isValid())
			{
				SET_SHADER_TEXTURE_AND_SAMPLER(commandList, *pathTracingPS, SkyTexture, *mIBLResource.texture, TStaticSamplerState<>::GetRHI());
				pathTracingPS->mIBLParams.setParameters(commandList, *pathTracingPS, mIBLResource);
			}
			DrawUtility::ScreenRect(commandList);
		}

		RHITexture2D* pOutputTexture = &sceneRenderTargets.getFrameTexture();
		if (config.debugDisplayMode == EDebugDsiplayMode::None && config.bSplitAccumulate)
		{
			GPU_PROFILE("Accumulate");

			GraphicsShaderStateDesc state;
			state.vertex = mScreenVS->getRHI();
			state.pixel = mAccumulatePS->getRHI();
			RHISetGraphicsShaderBoundState(commandList, state);

			SET_SHADER_TEXTURE_AND_SAMPLER(commandList, *mAccumulatePS, HistoryTexture, sceneRenderTargets.getPrevFrameTexture(), TStaticSamplerState<>::GetRHI());
			SET_SHADER_TEXTURE_AND_SAMPLER(commandList, *mAccumulatePS, FrameTexture, sceneRenderTargets.getFrameTexture(), TStaticSamplerState<>::GetRHI());
			DrawUtility::ScreenRect(commandList);

		}

		if (config.bUseDenoise && config.debugDisplayMode == EDebugDsiplayMode::None)
		{
			GPU_PROFILE("Denoise");
			RHIResourceTransition(commandList, { (RHIResource*)&sceneRenderTargets.getFrameTexture() ,
												 (RHIResource*)&sceneRenderTargets.getGBuffer().getRenderTexture(EGBuffer::A) }, EResourceTransition::SRV);
			RHIResourceTransition(commandList, { (RHIResource*)mDenoiseTexture.get() }, EResourceTransition::RenderTarget);
			RHISetFrameBuffer(commandList, mDenoiseFrameBuffer);

			GraphicsShaderStateDesc state;
			state.vertex = mScreenVS->getRHI();
			state.pixel = mDenoisePS->getRHI();
			RHISetGraphicsShaderBoundState(commandList, state);

			SET_SHADER_TEXTURE_AND_SAMPLER(commandList, *mDenoisePS, FrameTexture, sceneRenderTargets.getFrameTexture(), TStaticSamplerState<>::GetRHI());
			SET_SHADER_TEXTURE_AND_SAMPLER(commandList, *mDenoisePS, GBufferTextureA, sceneRenderTargets.getGBuffer().getRenderTexture(EGBuffer::A), TStaticSamplerState<>::GetRHI());
			mDenoisePS->setParam(commandList, SHADER_PARAM(ScreenSizeInv), Vector2(1.0f / screenSize.x, 1.0f / screenSize.y));
			view.setupShader(commandList, *mDenoisePS);

			DrawUtility::ScreenRect(commandList);
			pOutputTexture = mDenoiseTexture;
		}

		RHIResourceTransition(commandList, { (RHIResource*)pOutputTexture }, EResourceTransition::SRV);
		return pOutputTexture;
	}

	bool Renderer::initializeRHI(ETexture::Format RTFormat)
	{
		{
			char const* HDRImagePath = "Texture/HDR/A.hdr";
			mHDRImage = RHIUtility::LoadTexture2DFromFile(HDRImagePath, TextureLoadOption().HDR());
			if (mHDRImage)
			{
				mIBLResourceBuilder.initializeShader();
				VERIFY_RETURN_FALSE(mIBLResourceBuilder.loadOrBuildResource(::Global::DataCache(), HDRImagePath, *mHDRImage, mIBLResource));
			}
		}

		{
			ScreenVS::PermutationDomain permutationVector;
			permutationVector.set<ScreenVS::UseTexCoord>(true);
			VERIFY_RETURN_FALSE(mScreenVS = ShaderManager::Get().getGlobalShaderT<ScreenVS>(permutationVector));
		}

		{
			Vec2i screenSize = ::Global::GetScreenSize();
			mDenoiseTexture = RHICreateTexture2D(RTFormat, screenSize.x, screenSize.y, 1, 1, TCF_RenderTarget | TCF_CreateSRV);
			mDenoiseFrameBuffer = RHICreateFrameBuffer();
			mDenoiseFrameBuffer->setTexture(0, *mDenoiseTexture);
		}
		mDebugPrimitives.initializeRHI();

		VERIFY_RETURN_FALSE(mAccumulatePS = ShaderManager::Get().getGlobalShaderT<AccumulatePS>());
		VERIFY_RETURN_FALSE(mDenoisePS = ShaderManager::Get().getGlobalShaderT<DenoisePS>());

		if (GRHISupportRayTracing)
		{
			float aabbData[] = { -1.0f, -1.0f, -1.0f, 1.0f, 1.0f, 1.0f };
			mPrimitiveAABBBuffer = RHICreateBuffer(sizeof(float) * 6, 1, BCF_None, aabbData);

			RayTracingGeometryDesc procGeoDesc;
			procGeoDesc.type = ERayTracingGeometryType::Procedural;
			procGeoDesc.vertexBuffer = mPrimitiveAABBBuffer;
			procGeoDesc.vertexCount = 1;
			procGeoDesc.vertexStride = sizeof(float) * 6;
			procGeoDesc.bOpaque = true;
			mSphereBLAS = RHICreateBottomLevelAccelerationStructure(&procGeoDesc, 1, EAccelerationStructureBuildFlags::PreferFastTrace);
			RHIBuildAccelerationStructure(RHICommandList::GetImmediateList(), mSphereBLAS.get(), nullptr, nullptr);

			mCubeBLAS = mSphereBLAS; // Can use the same unit AABB BLAS, selected by hit group in TLAS

			auto BuildTriangleBLAS = [&](Mesh& mesh, RHIBottomLevelAccelerationStructureRef& outBLAS)
			{
				RayTracingGeometryDesc geoDesc;
				geoDesc.vertexBuffer = mesh.mVertexBuffer;
				geoDesc.vertexCount = mesh.getVertexCount();
				geoDesc.vertexStride = mesh.mInputLayoutDesc.getVertexSize();
				if (mesh.mIndexBuffer.isValid())
				{
					geoDesc.indexBuffer = mesh.mIndexBuffer;
					geoDesc.indexCount = mesh.mIndexBuffer->getNumElements();
					geoDesc.indexType = IsIntType(mesh.mIndexBuffer) ? EIndexBufferType::U32 : EIndexBufferType::U16;
				}
				outBLAS = RHICreateBottomLevelAccelerationStructure(&geoDesc, 1, EAccelerationStructureBuildFlags::PreferFastTrace);
				RHIBuildAccelerationStructure(RHICommandList::GetImmediateList(), outBLAS.get(), nullptr, nullptr);
			};

			{
				FMeshBuild::Plane(mQuadMesh, Vector3::Zero(), Vector3(0, 0, 1), Vector3(0, 1, 0), Vector2(1, 1), 1.0f);
				BuildTriangleBLAS(mQuadMesh, mQuadBLAS);
			}
		}

		// Ensure all base BLAS are built before scene load
		RHIFlushCommand(RHICommandList::GetImmediateList());
	}

	void Renderer::releaseRHI()
	{
		RenderResourceData::releaseRHI();

		mScreenVS = nullptr;

		mQuadMesh.releaseRHIResource();

		mHDRImage.release();
		mDenoiseTexture.release();
		mDenoiseFrameBuffer.release();

		mIBLResource.releaseRHI();
		mIBLResourceBuilder.releaseRHI();
	}

}//namespace PathTracing