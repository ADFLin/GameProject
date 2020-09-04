#include "TestRenderStageBase.h"

#include "RHI/GLExtensions.h"
#include "RHI/GlobalShader.h"

namespace Render
{
	struct MeshletRenderResource
	{

		TStructuredBuffer<MeshletData>  mMeshletBuffer;
		TStructuredBuffer< uint32 >  mPrimitiveIndexBuffer;
		TStructuredBuffer< uint32 >  mVertexIndexBuffer;

		int getMeshletCount()
		{
			return mMeshletBuffer.getElementNum();
		}

		bool initialize(Mesh& mesh)
		{
			int const maxVertices = 64;
			int const maxPrimitives = 126;
			std::vector<MeshletData> meshlets;
			std::vector<PackagedTriangleIndices> primitiveIndices;
			std::vector<uint8> uniqueVertexIndices;
			mesh.buildMeshlet(64, 126, meshlets, uniqueVertexIndices, primitiveIndices);
			mVertexIndexBuffer.initializeResource(uniqueVertexIndices.size() / sizeof(uint32), EStructuredBufferType::Buffer);
			mVertexIndexBuffer.updateBuffer(TArrayView<uint32>((uint32*)uniqueVertexIndices.data(), uniqueVertexIndices.size() / sizeof(uint32)));

			mMeshletBuffer.initializeResource(meshlets.size(), EStructuredBufferType::Buffer);
			mMeshletBuffer.updateBuffer(MakeView(meshlets));
			mPrimitiveIndexBuffer.initializeResource(primitiveIndices.size(), EStructuredBufferType::Buffer);
			mPrimitiveIndexBuffer.updateBuffer(TArrayView<uint32>((uint32*)primitiveIndices.data(), primitiveIndices.size()));

			return true;
		}

		void releaseRHI()
		{
			mMeshletBuffer.releaseResources();
			mPrimitiveIndexBuffer.releaseResources();
			mVertexIndexBuffer.releaseResources();
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
		static TArrayView< ShaderEntryInfo const > GetShaderEntries()
		{
			static ShaderEntryInfo const entries[] =
			{
				{ EShader::Mesh   , SHADER_ENTRY(MainMS) },
				{ EShader::Pixel  , SHADER_ENTRY(MainPS) },
			};
			return entries;
		}

		MeshletShaderParameters mMeshletParam;

	};


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


	IMPLEMENT_SHADER_PROGRAM(SimpleMeshProgram);
	IMPLEMENT_SHADER_PROGRAM(SimpleProgram);


	class MeshShaderTestStage : public TestRenderStageBase
	{
		using BaseClass = TestRenderStageBase;
	public:
		MeshShaderTestStage() {}

		bool onInit() override
		{
			if (!BaseClass::onInit())
				return false;

			if (GRHISupportMeshShader && GRHISystem->getName() == RHISytemName::OpenGL)
			{
				GLint MaxDrawMeshTasksCount = 0;
				glGetIntegerv(GL_MAX_DRAW_MESH_TASKS_COUNT_NV, &MaxDrawMeshTasksCount);
				LogMsg("MaxDrawMeshTasksCount = %d", MaxDrawMeshTasksCount);

				GLint MaxMeshOutVertexCount = 0;
				glGetIntegerv(GL_MAX_MESH_OUTPUT_VERTICES_NV, &MaxMeshOutVertexCount);
				LogMsg("MaxMeshOutVertexCount = %d", MaxMeshOutVertexCount);

				GLint MaxMeshOutPrimitiveCount = 0;
				glGetIntegerv(GL_MAX_MESH_OUTPUT_PRIMITIVES_NV, &MaxMeshOutPrimitiveCount);
				LogMsg("MaxMeshOutPrimitiveCount = %d", MaxMeshOutPrimitiveCount);
			}

			IntVector2 screenSize = ::Global::GetScreenSize();
			::Global::GUI().cleanupWidget();

			auto frameWidget = WidgetUtility::CreateDevFrame();
			FWidgetProperty::Bind(frameWidget->addCheckBox(UI_ANY, "Use Mesh Shader"), bUseMeshShader);

			return true;
		}
		bool bUseMeshShader = true;
		SimpleMeshProgram* mProgSimpleMesh;
		SimpleProgram* mProgSimple;
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

			if (GRHISupportMeshShader)
			{
				mProgSimpleMesh = ShaderManager::Get().getGlobalShaderT<SimpleMeshProgram>();
				mMeshletResource.initialize(mSimpleMeshs[SimpleMeshId::Doughnut]);
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

		void updateFrame(int frame) override
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
			{
				GPU_PROFILE("Mesh");

				if ( bUseMeshShader && GRHISupportMeshShader )
				{
					RHISetShaderProgram(commandList, mProgSimpleMesh->getRHIResource());
					mProgSimpleMesh->setStorageBuffer(commandList, SHADER_PARAM(VertexDataBlock), *mSimpleMeshs[SimpleMeshId::Doughnut].mVertexBuffer);
					mProgSimpleMesh->setTexture(commandList, SHADER_PARAM(Texture), *mTexture);
					mProgSimpleMesh->mMeshletParam.setParameters(commandList, *mProgSimpleMesh, mMeshletResource);
					mView.setupShader(commandList, *mProgSimpleMesh);

					for (int i = 0; i < 100; ++i)
					{
						Vector3 offset;
						offset.x = i % 10;
						offset.y = i / 10;
						offset.z = 0;
						mProgSimpleMesh->setParam(commandList, SHADER_PARAM(LocalToWorld), Matrix4::Scale(0.15) * Matrix4::Translate(offset));
						RHIDrawMeshTasks(commandList, 0, mMeshletResource.getMeshletCount());
					}
					
				}
				else
				{
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
						mSimpleMeshs[SimpleMeshId::Doughnut].draw(commandList);
					}
				}
			}
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

	REGISTER_STAGE("Mesh Shader", MeshShaderTestStage, EStageGroup::FeatureDev);

}