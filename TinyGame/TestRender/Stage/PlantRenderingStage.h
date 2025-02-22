#ifndef PlantRenderStage_h__
#define PlantRenderStage_h__

#include "Stage/TestRenderStageBase.h"
#include "Renderer/MeshBuild.h"



namespace Render
{

	struct PlantSettings
	{
		int meshLevel = 400;

		int numCraters = 200;
		float radiusDistribution = 0.606f;
		Vector2 radiusMinMax = { 0.01f , 0.17f };

		float rimWidth = 0.7f;
		float rimSteepness = 0.42f;
		float smoothness = 0.34f;
		float floorHeight = -0.29f;
	};

	struct CraterInfo
	{
		Vector3 pos;
		float radius;
	};


	class GenerateHeightProgram : public GlobalShaderProgram
	{
	public:
		DECLARE_SHADER_PROGRAM(GenerateHeightProgram, Global);

		static char const* GetShaderFileName()
		{
			return "Shader/Game/Plant";
		}
		static TArrayView< ShaderEntryInfo const > GetShaderEntries()
		{
			static ShaderEntryInfo const entries[] =
			{
				{ EShader::Compute , SHADER_ENTRY(GenerateHeightCS) },
			};
			return entries;
		}
	};

	class RenderProgram : public GlobalShaderProgram
	{
	public:
		DECLARE_SHADER_PROGRAM(RenderProgram, Global);

		static char const* GetShaderFileName()
		{
			return "Shader/Game/Plant";
		}
		static TArrayView< ShaderEntryInfo const > GetShaderEntries()
		{
			static ShaderEntryInfo const entries[] =
			{
				{ EShader::Vertex , SHADER_ENTRY(RenderVS) },
				{ EShader::Pixel, SHADER_ENTRY(RenderPS) },
			};
			return entries;
		}
	};

	class PlantRenderingStage : public TestRenderStageBase
	{
		using BaseClass = TestRenderStageBase;
	public:

		Mesh mMesh;
		MeshBuildData mMeshData;

		TStructuredBuffer<float> mMeshOffsetData;
		TStructuredBuffer< CraterInfo > mCraterData;

		GenerateHeightProgram* mProgGenerateHeight;
		RenderProgram* mProgRender;

		PlantSettings mSettings;

		bool bWireframe = false;

		enum EBuildMask
		{
			MeshBase     = BIT(0),
			Crater       = BIT(1),
			PlantHeight  = BIT(2),

			All = 0xff,
		};

		uint32 mPendingBuildMask = 0;


		bool buildPlantMesh();
		bool createMeshBaseData();
		bool applyHeightToMesh();
		bool generateCraterData();
		void generatePlantHeight();


		bool setupRenderResource(ERenderSystem systemName) override;


		void preShutdownRenderSystem(bool bReInit) override
		{

			mMesh.releaseRHIResource();
			mMeshOffsetData.releaseResource();
		}


		ERenderSystem getDefaultRenderSystem() override
		{
			return ERenderSystem::OpenGL;
		}

		bool onInit() override;

		void onEnd() override
		{
			BaseClass::onEnd();
		}

		void restart() {}

		void onUpdate(GameTimeSpan deltaTime) override;

		void onRender(float dFrame) override;

		MsgReply onMouse(MouseMsg const& msg) override
		{
			return BaseClass::onMouse(msg);
		}

		MsgReply onKey(KeyMsg const& msg) override
		{
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


	};


}
#endif // PlantRenderStage_h__
