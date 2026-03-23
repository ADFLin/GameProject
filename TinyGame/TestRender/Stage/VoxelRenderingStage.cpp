#include "Stage/TestRenderStageBase.h"
#include "Renderer/MeshBuild.h"
#include "Math/PrimitiveTest.h"
#include "Math/GeometryPrimitive.h"
#include "RHI/SceneRenderer.h"

namespace Render
{

	struct VoxelRawData
	{
		TArray< uint8 > data;
		IntVector3 dims;
	};

	using AABBox = Math::TAABBox< Vector3 >;

	struct OctTreeNode
	{
		int children[8];
	};

	class FVoxelBuild
	{
	public:
		static bool MeshSurface(VoxelRawData& voxelRawData, AABBox const& bbox, MeshBuildData const& meshData, InputLayoutDesc const& inputLayoutDesc, Matrix4 const& transform)
		{
			IntVector3 const& dims = voxelRawData.dims;
			Vector3 boundSize = bbox.getSize();
			Vector3 voxelSize = { boundSize.x / dims.x, boundSize.y / dims.y, boundSize.z / dims.z };

			auto posReader = meshData.makeAttributeReader(inputLayoutDesc, EVertex::ATTRIBUTE_POSITION);
			auto GetVertexPos = [&](uint32 index) -> Vector3
			{
				return TransformPosition(posReader[index], transform);
			};

			float thresholdLength = voxelSize.length() * 0.5f;

			for (int i = 0; i < (int)meshData.indexData.size(); i += 3)
			{
				Vector3 v0 = GetVertexPos(meshData.indexData[i]);
				Vector3 v1 = GetVertexPos(meshData.indexData[i + 1]);
				Vector3 v2 = GetVertexPos(meshData.indexData[i + 2]);

				Vector3 tMin = v0.min(v1).min(v2);
				Vector3 tMax = v0.max(v1).max(v2);

				int iMin[3], iMax[3];
				iMin[0] = Math::Max(0, Math::FloorToInt((tMin.x - bbox.min.x) / voxelSize.x));
				iMin[1] = Math::Max(0, Math::FloorToInt((tMin.y - bbox.min.y) / voxelSize.y));
				iMin[2] = Math::Max(0, Math::FloorToInt((tMin.z - bbox.min.z) / voxelSize.z));

				iMax[0] = Math::Min(dims.x - 1, Math::FloorToInt((tMax.x - bbox.min.x) / voxelSize.x));
				iMax[1] = Math::Min(dims.y - 1, Math::FloorToInt((tMax.y - bbox.min.y) / voxelSize.y));
				iMax[2] = Math::Min(dims.z - 1, Math::FloorToInt((tMax.z - bbox.min.z) / voxelSize.z));

				for (int z = iMin[2]; z <= iMax[2]; ++z)
				{
					for (int y = iMin[1]; y <= iMax[1]; ++y)
					{
						for (int x = iMin[0]; x <= iMax[0]; ++x)
						{
							Vector3 p = bbox.min + Vector3((x + 0.5f) * voxelSize.x, (y + 0.5f) * voxelSize.y, (z + 0.5f) * voxelSize.z);
							float outSide;
							Vector3 closest = Math::PointToTriangleClosestPoint(p, v0, v1, v2, outSide);
							if ((p - closest).length2() < thresholdLength * thresholdLength)
							{
								voxelRawData.data[x + dims.x * (y + dims.y * z)] = 1;
							}
						}
					}
				}
			}
			return true;
		}

		static bool MeshSolid(VoxelRawData& voxelRawData, AABBox const& bbox, MeshBuildData const& meshData, InputLayoutDesc const& inputLayoutDesc, Matrix4 const& transform)
		{
			IntVector3 const& dims = voxelRawData.dims;
			Vector3 boundSize = bbox.getSize();
			Vector3 voxelSize = { boundSize.x / dims.x, boundSize.y / dims.y, boundSize.z / dims.z };

			auto posReader = meshData.makeAttributeReader(inputLayoutDesc, EVertex::ATTRIBUTE_POSITION);
			auto GetVertexPos = [&](uint32 index) -> Vector3
			{
				return TransformPosition(posReader[index], transform);
			};

			TArray< TArray<float> > zIntercepts;
			zIntercepts.resize(dims.x * dims.y);

			for (int i = 0; i < (int)meshData.indexData.size(); i += 3)
			{
				Vector3 v0 = GetVertexPos(meshData.indexData[i]);
				Vector3 v1 = GetVertexPos(meshData.indexData[i + 1]);
				Vector3 v2 = GetVertexPos(meshData.indexData[i + 2]);

				Vector3 tMin = v0.min(v1).min(v2);
				Vector3 tMax = v0.max(v1).max(v2);

				int iMinX = Math::Max(0, Math::FloorToInt((tMin.x - bbox.min.x) / voxelSize.x));
				int iMinY = Math::Max(0, Math::FloorToInt((tMin.y - bbox.min.y) / voxelSize.y));
				int iMaxX = Math::Min(dims.x - 1, Math::FloorToInt((tMax.x - bbox.min.x) / voxelSize.x));
				int iMaxY = Math::Min(dims.y - 1, Math::FloorToInt((tMax.y - bbox.min.y) / voxelSize.y));

				Vector3 e1 = v1 - v0;
				Vector3 e2 = v2 - v0;
				Vector3 n = e1.cross(e2);
				if (Math::Abs(n.z) < 1e-6f)
					continue;

				for (int y = iMinY; y <= iMaxY; ++y)
				{
					for (int x = iMinX; x <= iMaxX; ++x)
					{
						Vector3 p = bbox.min + Vector3((x + 0.5f) * voxelSize.x, (y + 0.5f) * voxelSize.y, 0.0f);

						// Check if (x,y) is inside 2D projection of triangle
						Vector2 p2(p.x, p.y);
						Vector2 a2(v0.x, v0.y);
						Vector2 b2(v1.x, v1.y);
						Vector2 c2(v2.x, v2.y);

						float w0 = ((b2.y - c2.y) * (p2.x - c2.x) + (c2.x - b2.x) * (p2.y - c2.y)) / ((b2.y - c2.y) * (a2.x - c2.x) + (c2.x - b2.x) * (a2.y - c2.y));
						float w1 = ((c2.y - a2.y) * (p2.x - c2.x) + (a2.x - c2.x) * (p2.y - c2.y)) / ((b2.y - c2.y) * (a2.x - c2.x) + (c2.x - b2.x) * (a2.y - c2.y));
						float w2 = 1.0f - w0 - w1;

						if (w0 >= 0 && w1 >= 0 && w2 >= 0)
						{
							// Calculate Z using plane equation: n.x*(x-v0.x) + n.y*(y-v0.y) + n.z*(z-v0.z) = 0
							float z = v0.z - (n.x * (p.x - v0.x) + n.y * (p.y - v0.y)) / n.z;
							zIntercepts[x + dims.x * y].push_back((z - bbox.min.z) / voxelSize.z);
						}
					}
				}
			}

			for (int y = 0; y < dims.y; ++y)
			{
				for (int x = 0; x < dims.x; ++x)
				{
					auto& list = zIntercepts[x + dims.x * y];
					if (list.empty()) continue;
					std::sort(list.begin(), list.end());

					for (int i = 0; i + 1 < (int)list.size(); i += 2)
					{
						int zStart = Math::Max(0, (int)Math::Round(list[i]));
						int zEnd = Math::Min(dims.z - 1, (int)Math::Round(list[i + 1]));
						for (int z = zStart; z <= zEnd; ++z)
						{
							voxelRawData.data[x + dims.x * (y + dims.y * z)] = 1;
						}
					}
				}
			}

			return true;
		}
	};

	class VoxelProgram : public GlobalShaderProgram
	{
		DECLARE_SHADER_PROGRAM(VoxelProgram, Global);
	public:
		static void SetupShaderCompileOption(ShaderCompileOption& option)
		{
			option.addDefine(SHADER_PARAM(USE_INSTANCED), 1);
		}

		static char const* GetShaderFileName() { return "Shader/Game/Voxel"; }
		static TArrayView< ShaderEntryInfo const > GetShaderEntries()
		{
			static ShaderEntryInfo const entries[] =
			{
				{ EShader::Vertex , SHADER_ENTRY(MainVS) },
				{ EShader::Pixel  , SHADER_ENTRY(MainPS) },
			};
			return entries;
		}

		void bindParameters(ShaderParameterMap const& parameterMap)
		{
			BIND_SHADER_PARAM(parameterMap, LocalToWorld);
			BIND_SHADER_PARAM(parameterMap, VoxelColor);
		}

		DEFINE_SHADER_PARAM(LocalToWorld);
		DEFINE_SHADER_PARAM(VoxelColor);
	};

	IMPLEMENT_SHADER_PROGRAM(VoxelProgram);

	class VoxelRayMarchProgram : public GlobalShaderProgram
	{
		DECLARE_SHADER_PROGRAM(VoxelRayMarchProgram, Global);
	public:
		static void SetupShaderCompileOption(ShaderCompileOption& option) {}
		static char const* GetShaderFileName() { return "Shader/Game/VoxelRayMarch"; }
		static TArrayView< ShaderEntryInfo const > GetShaderEntries()
		{
			static ShaderEntryInfo const entries[] = {
				{ EShader::Vertex , SHADER_ENTRY(MainVS) },
				{ EShader::Pixel  , SHADER_ENTRY(MainPS) },
			};
			return entries;
		}

		void bindParameters(ShaderParameterMap const& parameterMap)
		{
			BIND_SHADER_PARAM(parameterMap, LocalToWorld);
			BIND_SHADER_PARAM(parameterMap, WorldToLocal);
			BIND_SHADER_PARAM(parameterMap, BBoxMin);
			BIND_SHADER_PARAM(parameterMap, BBoxMax);
			BIND_SHADER_PARAM(parameterMap, VoxelDims);
			BIND_SHADER_PARAM(parameterMap, VoxelColor);
			BIND_SHADER_PARAM(parameterMap, VoxelData);
		}

		DEFINE_SHADER_PARAM(LocalToWorld);
		DEFINE_SHADER_PARAM(WorldToLocal);
		DEFINE_SHADER_PARAM(BBoxMin);
		DEFINE_SHADER_PARAM(BBoxMax);
		DEFINE_SHADER_PARAM(VoxelDims);
		DEFINE_SHADER_PARAM(VoxelColor);
		DEFINE_SHADER_PARAM(VoxelData);
	};

	IMPLEMENT_SHADER_PROGRAM(VoxelRayMarchProgram);

	class VoxelRenderingStage : public TestRenderStageBase
	{
		using BaseClass = TestRenderStageBase;
	public:
		VoxelRenderingStage() {}

		bool onInit() override
		{
			if (!BaseClass::onInit())
				return false;

			MeshBuildData doughnutData;
			InputLayoutDesc inputLayout;
			FMeshBuild::Doughnut(doughnutData, inputLayout, 2.0f, 1.0f, 60, 60);

			mBBox.min = Vector3(-3.5, -3.5, -1.5);
			mBBox.max = Vector3(3.5, 3.5, 1.5);
			mRawData.dims = IntVector3(64, 64, 32);
			mRawData.data.resize(mRawData.dims.x * mRawData.dims.y * mRawData.dims.z, 0);

			FVoxelBuild::MeshSurface(mRawData, mBBox, doughnutData, inputLayout, Matrix4::Identity());

			::Global::GUI().cleanupWidget();
			return true;
		}


		virtual ERenderSystem getDefaultRenderSystem() override
		{
			return ERenderSystem::None;
		}

		virtual void configRenderSystem(ERenderSystem systenName, RenderSystemConfigs& systemConfigs) override
		{

		}


		virtual bool setupRenderResource(ERenderSystem systemName) override
		{
			VERIFY_RETURN_FALSE(BaseClass::setupRenderResource(systemName));
			VERIFY_RETURN_FALSE(createSimpleMesh());
			VERIFY_RETURN_FALSE(loadCommonShader());
			VERIFY_RETURN_FALSE(mProgVoxel = ShaderManager::Get().getGlobalShaderT<VoxelProgram>(true));
			VERIFY_RETURN_FALSE(mProgRayMarch = ShaderManager::Get().getGlobalShaderT<VoxelRayMarchProgram>(true));

			mVoxelMesh.setupMesh(getMesh(SimpleMeshId::Box));
			
			TArray<uint32> bufferData;
			bufferData.resize(mRawData.data.size());
			for (int i = 0; i < (int)mRawData.data.size(); ++i)
				bufferData[i] = mRawData.data[i];
			mVoxelBuffer.initializeResource((uint32)bufferData.size(), EStructuredBufferType::Buffer, BCF_None, bufferData.data());

			updateVoxelMesh();
			return true;
		}

		void updateVoxelMesh()
		{
			mVoxelMesh.clearInstance();

			IntVector3 const& dims = mRawData.dims;
			Vector3 boundSize = mBBox.getSize();
			Vector3 voxelSize = { boundSize.x / dims.x, boundSize.y / dims.y, boundSize.z / dims.z };

			for (int z = 0; z < dims.z; ++z)
			{
				for (int y = 0; y < dims.y; ++y)
				{
					for (int x = 0; x < dims.x; ++x)
					{
						if (mRawData.data[x + dims.x * (y + dims.y * z)] == 1)
						{
							Vector3 p = mBBox.min + Vector3((x + 0.5f) * voxelSize.x, (y + 0.5f) * voxelSize.y, (z + 0.5f) * voxelSize.z);
							mVoxelMesh.addInstance(p, voxelSize * 0.5f, Quaternion::Identity(), Vector4(0, 1, 0, 1));
						}
					}
				}
			}
		}

		virtual void preShutdownRenderSystem(bool bReInit = false) override
		{
			mVoxelBuffer.releaseResource();
			mVoxelMesh.releaseRHI();
			BaseClass::preShutdownRenderSystem(bReInit);
		}


		void onEnd() override
		{


		}

		void restart() override
		{

		}

		void onUpdate(GameTimeSpan deltaTime) override
		{
			BaseClass::onUpdate(deltaTime);
		}

		void onRender(float dFrame) override
		{
			initializeRenderState();

			RHICommandList& commandList = RHICommandList::GetImmediateList();
			
			if (bRayMarchMode)
			{
				RHISetRasterizerState(commandList, TStaticRasterizerState<ECullMode::Front>::GetRHI());

				RHISetShaderProgram(commandList, mProgRayMarch->getRHI());
				mView.setupShader(commandList, *mProgRayMarch);
				Matrix4 boxLocalToWorld = Matrix4::Scale(mBBox.getSize()) * Matrix4::Translate(mBBox.getCenter());
				SET_SHADER_PARAM(commandList, *mProgRayMarch, LocalToWorld, boxLocalToWorld);

				float det;
				Matrix4 boxWorldToLocal;
				boxLocalToWorld.inverse(boxWorldToLocal, det);

				SET_SHADER_PARAM(commandList, *mProgRayMarch, WorldToLocal, boxWorldToLocal);
				SET_SHADER_PARAM(commandList, *mProgRayMarch, BBoxMin, mBBox.min);
				SET_SHADER_PARAM(commandList, *mProgRayMarch, BBoxMax, mBBox.max);
				SET_SHADER_PARAM(commandList, *mProgRayMarch, VoxelDims, mRawData.dims);
				SET_SHADER_PARAM(commandList, *mProgRayMarch, VoxelColor, LinearColor(0, 1, 0, 1));
				mProgRayMarch->setStorageBuffer(commandList, mProgRayMarch->mParamVoxelData, *mVoxelBuffer.getRHI(), EAccessOp::ReadOnly);
				getMesh(SimpleMeshId::Box).draw(commandList);
			}
			else if (mVoxelMesh.mInstanceTransforms.size() > 0)
			{
				RHISetShaderProgram(commandList, mProgVoxel->getRHI());
				mView.setupShader(commandList, *mProgVoxel);
				SET_SHADER_PARAM(commandList, *mProgVoxel, LocalToWorld, Matrix4::Identity());
				SET_SHADER_PARAM(commandList, *mProgVoxel, VoxelColor, LinearColor(0, 1, 0, 1));
				mVoxelMesh.draw(commandList);
			}
		}

		MsgReply onMouse(MouseMsg const& msg) override
		{
			return BaseClass::onMouse(msg);
		}

		MsgReply onKey(KeyMsg const& msg) override
		{
			if (msg.isDown())
			{
				switch (msg.getCode())
				{
				case EKeyCode::M:
					bRayMarchMode = !bRayMarchMode;
					break;
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
		VoxelRawData mRawData;
		AABBox mBBox;
		InstancedMesh mVoxelMesh;
		VoxelProgram* mProgVoxel;

		TStructuredBuffer<uint32> mVoxelBuffer;
		VoxelRayMarchProgram* mProgRayMarch;
		bool bRayMarchMode = false;
	};

	REGISTER_STAGE_ENTRY("Voxel Rendering", VoxelRenderingStage, EExecGroup::FeatureDev);

}