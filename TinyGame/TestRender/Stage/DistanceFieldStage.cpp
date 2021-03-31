#include "TestRenderStageBase.h"

#include "RHI/RHICommon.h"
#include "RHI/ShaderManager.h"
#include "RHI/RenderContext.h"
#include "RHI/DrawUtility.h"

#include "Renderer/MeshBuild.h"
#include "Renderer/MeshUtility.h"

#include "Core/ScopeExit.h"
#include "Math/PrimitiveTest.h"
#include "AsyncWork.h"

#include "FileSystem.h"
#include "Serialize/FileStream.h"


namespace Render
{

	class RayMarchingProgram : public GlobalShaderProgram
	{
		DECLARE_SHADER_PROGRAM(RayMarchingProgram, Global);
	public:
		static void SetupShaderCompileOption(ShaderCompileOption& option) 
		{

		}

		static char const* GetShaderFileName()
		{
			return "Shader/DistanceField";
		}

		static TArrayView< ShaderEntryInfo const > GetShaderEntries()
		{
			static ShaderEntryInfo const entries[] =
			{
				{ EShader::Vertex , SHADER_ENTRY(ScreenVS) },
				{ EShader::Pixel  , SHADER_ENTRY(RayMarchingPS) },
			};
			return entries;
		}
		
	};

	IMPLEMENT_SHADER_PROGRAM(RayMarchingProgram);

	class DistanceFieldTestStage : public TestRenderStageBase
	{
		typedef TestRenderStageBase BaseClass;
	public:
		DistanceFieldTestStage() {}

		RHITexture3DRef mTextureSDF;
		Mesh mMesh;
		
		DistanceFieldData mSDFData;
		class RayMarchingProgram* mProgRayMarching;


		template< class OP >
		static void serialize(DistanceFieldData& data , OP& op)
		{
			op & data.boundMin & data.boundMax & data.gridSize & data.maxDistance & data.volumeData;
		}

		bool loadDistanceFieldData(char const* path , DistanceFieldData& data)
		{
			InputFileSerializer serializer;
			if( !serializer.open(path) )
				return false;

			IStreamSerializer::ReadOp op(serializer);

			serialize(data , op);

			if( !serializer.isValid() )
			{
				return false;
			}
			return true;
		}

		bool saveDistanceFieldData(char const* path , DistanceFieldData& data)
		{
			OutputFileSerializer serializer;
			if( !serializer.open(path) )
				return false;

			IStreamSerializer::WriteOp op(serializer);
			serialize(data , op);
			if( !serializer.isValid() )
			{
				return false;
			}
			return true;
		}

		bool onInit() override
		{
			if( !BaseClass::onInit() )
				return false;

			//VERIFY_RETURN_FALSE(MeshBuild::Cube(mMesh, 1));
			VERIFY_RETURN_FALSE(MeshBuild::IcoSphere(mMesh, 1, 3));
			

			char const* testDataPath = "DistField/test.data";

			if( !loadDistanceFieldData(testDataPath, mSDFData) )
			{
				QueueThreadPool pool;
				pool.init(SystemPlatform::GetProcessorNumber() - 1);
				DistanceFieldBuildSetting setting;
				setting.workTaskPool = &pool;
#if _DEBUG
				setting.gridLength = 0.2;
#else
				setting.gridLength = 0.02;
#endif
				MeshUtility::BuildDistanceField(mMesh, setting , mSDFData);
				saveDistanceFieldData(testDataPath , mSDFData );
			}

			VERIFY_RETURN_FALSE( mTextureSDF = RHICreateTexture3D( ETexture::R32F , mSDFData.gridSize.x , mSDFData.gridSize.y , mSDFData.gridSize.z , 1 , 1, TCF_DefalutValue , &mSDFData.volumeData[0] ) );

			VERIFY_RETURN_FALSE(mProgRayMarching = ShaderManager::Get().getGlobalShaderT<RayMarchingProgram>());
			Vec2i screenSize = ::Global::GetScreenSize();

			::Global::GUI().cleanupWidget();

			auto devFrame = WidgetUtility::CreateDevFrame();
			restart();

			return true;
		}

		void onEnd() override
		{
			BaseClass::onEnd();
		}

		void restart() override
		{

		}
		void tick() override {}
		void updateFrame(int frame) override {}

		void onUpdate(long time) override
		{
			BaseClass::onUpdate(time);

			int frame = time / gDefaultTickTime;
			for( int i = 0; i < frame; ++i )
				tick();

			float dt = float(time) / 1000;
			updateFrame(frame);
		}


		void onRender(float dFrame) override
		{
			Graphics2D& g = Global::GetGraphics2D();
			RHICommandList& commandList = RHICommandList::GetImmediateList();

			Vec2i screenSize = ::Global::GetScreenSize();

			initializeRenderState();


			RHISetFixedShaderPipelineState(commandList, mView.worldToClip);
			mMesh.draw(commandList, LinearColor(0, 1, 0, 1));


			{
				RHISetBlendState(commandList, TStaticBlendState< CWM_RGBA, EBlend::One, EBlend::One >::GetRHI());
				RHISetShaderProgram(commandList, mProgRayMarching->getRHIResource());
				mProgRayMarching->setParam(commandList, SHADER_PARAM(WorldToLocal), Matrix4::Identity());
				mProgRayMarching->setParam(commandList, SHADER_PARAM(BoundMax), mSDFData.boundMax);
				mProgRayMarching->setParam(commandList, SHADER_PARAM(BoundMin), mSDFData.boundMin);
				mProgRayMarching->setParam(commandList, SHADER_PARAM(DistanceFactor), mSDFData.maxDistance);
				auto& sampler = TStaticSamplerState< ESampler::Bilinear, ESampler::Clamp, ESampler::Clamp, ESampler::Clamp >::GetRHI();
				mProgRayMarching->setTexture(commandList, SHADER_PARAM(DistanceFieldTexture), *mTextureSDF, SHADER_PARAM(DistanceFieldTextureSampler), sampler);
				mView.setupShader(commandList, *mProgRayMarching);
				DrawUtility::ScreenRect(commandList);
			}


			g.beginRender();

			g.endRender();
		}

		int TessFactor = 5;
		int TessFactor2 = 1;
		bool onMouse(MouseMsg const& msg) override
		{
			static Vec2i oldPos = msg.getPos();

			if( !BaseClass::onMouse(msg) )
				return false;
			return true;
		}

		bool onKey(KeyMsg const& msg) override
		{
			if(msg.isDown())
			{
				switch(msg.getCode())
				{
				case EKeyCode::Left: --TessFactor; break;
				case EKeyCode::Right: ++TessFactor; break;
				case EKeyCode::Down: --TessFactor2; break;
				case EKeyCode::Up: ++TessFactor2; break;
				case EKeyCode::F2:
					{
						ShaderManager::Get().reloadAll();
						//initParticleData();
					}
					break;
				}
			}
			return BaseClass::onKey(msg);
		}

		virtual bool onWidgetEvent(int event, int id, GWidget* ui) override
		{
			switch( id )
			{
			default:
				break;
			}

			return BaseClass::onWidgetEvent(event, id, ui);
		}
	protected:
	};

	REGISTER_STAGE2("Distance Field Test", DistanceFieldTestStage, EStageGroup::FeatureDev, 1);

}//namespace Render