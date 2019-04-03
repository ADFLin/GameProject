#include "TestRenderStageBase.h"

#include "RHI/RHICommon.h"
#include "RHI/ShaderCompiler.h"
#include "RHI/RenderContext.h"
#include "RHI/DrawUtility.h"
#include "RHI/MeshUtility.h"

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
				{ Shader::eVertex , SHADER_ENTRY(ScreenVS) },
				{ Shader::ePixel  , SHADER_ENTRY(RayMarchingPS) },
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

		virtual bool onInit()
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
				setting.WorkTaskPool = &pool;
#if _DEBUG
				setting.gridLength = 0.2;
#else
				setting.gridLength = 0.02;
#endif
				MeshUtility::BuildDistanceField(mMesh, setting , mSDFData);
				saveDistanceFieldData(testDataPath , mSDFData );
			}

			VERIFY_RETURN_FALSE( mTextureSDF = RHICreateTexture3D( Texture::eR32F , mSDFData.gridSize.x , mSDFData.gridSize.y , mSDFData.gridSize.z , 1 , 1, TCF_DefalutValue , &mSDFData.volumeData[0] ) );

			VERIFY_RETURN_FALSE(mProgRayMarching = ShaderManager::Get().getGlobalShaderT<RayMarchingProgram>());
			Vec2i screenSize = ::Global::GetDrawEngine().getScreenSize();

			::Global::GUI().cleanupWidget();

			auto devFrame = WidgetUtility::CreateDevFrame();
			restart();

			return true;
		}

		virtual void onEnd()
		{
			BaseClass::onEnd();
		}

		void restart()
		{

		}
		void tick() {}
		void updateFrame(int frame) {}

		virtual void onUpdate(long time)
		{
			BaseClass::onUpdate(time);

			int frame = time / gDefaultTickTime;
			for( int i = 0; i < frame; ++i )
				tick();

			float dt = float(time) / 1000;
			updateFrame(frame);
		}


		void onRender(float dFrame)
		{
			Graphics2D& g = Global::GetGraphics2D();

			GameWindow& window = Global::GetDrawEngine().getWindow();

			initializeRenderState();


			RHISetupFixedPipelineState(mView.worldToView, mView.viewToClip);
			mMesh.draw(LinearColor(0, 1, 0, 1));


			{
				RHISetBlendState(TStaticBlendState< CWM_RGBA, Blend::eOne, Blend::eOne >::GetRHI());
				GL_BIND_LOCK_OBJECT(mProgRayMarching);
				mProgRayMarching->setParam(SHADER_PARAM(WorldToLocal), Matrix4::Identity());
				mProgRayMarching->setParam(SHADER_PARAM(BoundMax), mSDFData.boundMax);
				mProgRayMarching->setParam(SHADER_PARAM(BoundMin), mSDFData.boundMin);
				mProgRayMarching->setParam(SHADER_PARAM(DistanceFactor), mSDFData.maxDistance);
				auto& sampler = TStaticSamplerState< Sampler::eBilinear, Sampler::eClamp, Sampler::eClamp, Sampler::eClamp >::GetRHI();
				mProgRayMarching->setTexture(SHADER_PARAM(DistanceFieldTexture), *mTextureSDF, sampler);
				mView.setupShader(*mProgRayMarching);
				DrawUtility::ScreenRectShader();
			}


			g.beginRender();

			g.endRender();
		}

		int TessFactor = 5;
		int TessFactor2 = 1;
		bool onMouse(MouseMsg const& msg)
		{
			static Vec2i oldPos = msg.getPos();

			if( !BaseClass::onMouse(msg) )
				return false;
			return true;
		}

		bool onKey(unsigned key, bool isDown)
		{
			if( isDown )
			{
				switch( key )
				{
				case Keyboard::eLEFT: --TessFactor; break;
				case Keyboard::eRIGHT: ++TessFactor; break;
				case Keyboard::eDOWN: --TessFactor2; break;
				case Keyboard::eUP: ++TessFactor2; break;
				case Keyboard::eF2:
					{
						ShaderManager::Get().reloadAll();
						//initParticleData();
					}
					break;
				}
			}
			return BaseClass::onKey(key , isDown);
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