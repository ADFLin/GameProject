#include "TestStageHeader.h"

#include "RHI/RHICommon.h"
#include "RHI/ShaderCompiler.h"
#include "RHI/RenderContext.h"
#include "RHI/DrawUtility.h"
#include "RHI/MeshUtility.h"

#include "Core/ScopeExit.h"
#include "Math/PrimitiveTest.h"
#include "AsyncWork.h"

#include "FileSystem.h"
#include "DataStream.h"


namespace RenderGL
{


	class ViewFrustum
	{
	public:
		Matrix4 getPerspectiveMatrix()
		{
			return PerspectiveMatrix(mYFov, mAspect, mNear, mFar);
		}

		float mNear;
		float mFar;
		float mYFov;
		float mAspect;
	};

	class RayMarchingProgram : public GlobalShaderProgram
	{
		DECLARE_GLOBAL_SHADER(RayMarchingProgram);
	public:
		static void SetupShaderCompileOption(ShaderCompileOption& option) 
		{

		}

		static char const* GetShaderFileName()
		{
			return "Shader/DistanceField";
		}

		static ShaderEntryInfo const* GetShaderEntries()
		{
			static ShaderEntryInfo const entries[] =
			{
				{ Shader::eVertex , SHADER_ENTRY(ScreenVS) },
				{ Shader::ePixel  , SHADER_ENTRY(RayMarchingPS) },
				{ Shader::eEmpty  , nullptr },
			};
			return entries;
		}
		
	};

	IMPLEMENT_GLOBAL_SHADER(RayMarchingProgram);

	class DistanceFieldTestStage : public StageBase
	{
		typedef StageBase BaseClass;
	public:
		DistanceFieldTestStage() {}

		RHITexture3DRef mTextureSDF;
		Mesh mMesh;
		ViewInfo mView;
		
		ViewFrustum mViewFrustum;
		SimpleCamera  mCamera;
		DistanceFieldData mSDFData;
		class RayMarchingProgram* mProgRayMarching;


		template< class OP >
		static void serialize(DistanceFieldData& data , OP& op)
		{
			op & data.boundMin & data.boundMax & data.gridSize & data.maxDistance & data.volumeData;
		}

		bool loadDistanceFieldData(char const* path , DistanceFieldData& data)
		{
			InputFileStream stream;
			if( !stream.open(path) )
				return false;

			DataSerializer serializer(stream);
			DataSerializer::ReadOp op(serializer);
			serialize(data , op);
			return true;
		}

		bool saveDistanceFieldData(char const* path , DistanceFieldData& data)
		{
			OutputFileStream stream;
			if( !stream.open(path) )
				return false;

			DataSerializer serializer(stream);
			DataSerializer::WriteOp op(serializer);
			serialize(data , op);
			return true;
		}

		


		virtual bool onInit()
		{
			if( !BaseClass::onInit() )
				return false;

			if( !::Global::getDrawEngine()->startOpenGL() )
				return false;

			//VERIFY_INITRESULT(MeshBuild::Cube(mMesh, 1));
			VERIFY_INITRESULT(MeshBuild::IcoSphere(mMesh, 1, 3));
			

			char const* testDataPath = "DistField/test.data";

			if( !loadDistanceFieldData(testDataPath, mSDFData) )
			{
				QueueThreadPool pool;
				pool.init(SystemPlatform::GetProcessorNumber() - 1);
				DistanceFieldBuildSetting setting;
				setting.WorkTaskPool = &pool;
				setting.gridLength = 0.02;
				MeshUtility::BuildDistanceField(mMesh, setting , mSDFData);
				saveDistanceFieldData(testDataPath , mSDFData );
			}

			VERIFY_INITRESULT( mTextureSDF = RHICreateTexture3D( Texture::eR32F , mSDFData.gridSize.x , mSDFData.gridSize.y , mSDFData.gridSize.z , TCF_DefalutValue , &mSDFData.volumeData[0] ) );

			VERIFY_INITRESULT(mProgRayMarching = ShaderManager::Get().getGlobalShaderT<RayMarchingProgram>());
			Vec2i screenSize = ::Global::getDrawEngine()->getScreenSize();
			mViewFrustum.mNear = 0.01;
			mViewFrustum.mFar = 800.0;
			mViewFrustum.mAspect = float(screenSize.x) / screenSize.y;
			mViewFrustum.mYFov = Math::Deg2Rad(60 / mViewFrustum.mAspect);

			mCamera.setPos(Vector3(20, 0, 20));
			mCamera.setViewDir(Vector3(-1, 0, 0), Vector3(0, 0, 1));
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
			Graphics2D& g = Global::getGraphics2D();

			GameWindow& window = Global::getDrawEngine()->getWindow();

			mView.gameTime = 0;
			mView.realTime = 0;
			mView.rectOffset = IntVector2(0, 0);
			mView.rectSize = IntVector2(window.getWidth(), window.getHeight());

			Matrix4 matView = mCamera.getViewMatrix();
			mView.setupTransform(matView, mViewFrustum.getPerspectiveMatrix());

			glMatrixMode(GL_PROJECTION);
			glLoadMatrixf(mView.viewToClip);
			glMatrixMode(GL_MODELVIEW);
			glLoadMatrixf(mView.worldToView);

			glClearColor(0.2, 0.2, 0.2, 1);
			glClearDepth(1.0);
			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

			RHISetViewport(mView.rectOffset.x, mView.rectOffset.y, mView.rectSize.x, mView.rectSize.y);
			RHISetDepthStencilState(TStaticDepthStencilState<>::GetRHI());
			RHISetBlendState(TStaticBlendState<>::GetRHI());


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

			if( msg.onLeftDown() )
			{
				oldPos = msg.getPos();
			}
			if( msg.onMoving() && msg.isLeftDown() )
			{
				float rotateSpeed = 0.01;
				Vector2 off = rotateSpeed * Vector2(msg.getPos() - oldPos);
				mCamera.rotateByMouse(off.x, off.y);
				oldPos = msg.getPos();
			}

			if( !BaseClass::onMouse(msg) )
				return false;
			return true;
		}

		bool onKey(unsigned key, bool isDown)
		{
			if( !isDown )
				return false;
			switch( key )
			{
			case 'W': mCamera.moveFront(1); break;
			case 'S': mCamera.moveFront(-1); break;
			case 'D': mCamera.moveRight(1); break;
			case 'A': mCamera.moveRight(-1); break;
			case 'Z': mCamera.moveUp(0.5); break;
			case 'X': mCamera.moveUp(-0.5); break;
			case Keyboard::eLEFT: --TessFactor; break;
			case Keyboard::eRIGHT: ++TessFactor; break;
			case Keyboard::eDOWN: --TessFactor2; break;
			case Keyboard::eUP: ++TessFactor2; break;
			case Keyboard::eR: restart(); break;
			case Keyboard::eF2:
				{
					ShaderManager::Get().reloadAll();
					//initParticleData();
				}
				break;
			}
			return false;
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

}//namespace RenderGL