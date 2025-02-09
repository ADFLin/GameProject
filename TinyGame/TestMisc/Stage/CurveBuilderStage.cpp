#include "Stage/TestStageHeader.h"

#include "GameRenderSetup.h"

#include "CurveBuilder/ShapeCommon.h"
#include "CurveBuilder/ShapeFunction.h"
#include "CurveBuilder/ShapeMeshBuilder.h"
#include "CurveBuilder/Surface.h"

#include "CurveBuilder/CurveRenderer.h"
#include "CurveBuilder/FPUCompiler.h"

#include "Async/AsyncWork.h"
#include "RHI/RHIGraphics2D.h"
#include "ProfileSystem.h"

#include "RHI/DrawUtility.h"
#include "RHI/RenderContext.h"
#include "RHI/RHICommand.h"
#include "RHI/ShaderManager.h"
#include "Renderer/SimpleCamera.h"

#include "GL/wglew.h"
#include <memory>




namespace CB
{
	using namespace Render;

	class TestStage : public StageBase
		            , public IGameRenderSetup
	{
		using BaseClass = StageBase;
	public:

		std::unique_ptr<ShapeMeshBuilder>   mMeshBuilder;
		std::unique_ptr<CurveRenderer>   mRenderer;
		SimpleCamera  mCamera;
		std::vector<ShapeBase*> mSurfaceList;
#if USE_PARALLEL_UPDATE
		std::unique_ptr< QueueThreadPool > mUpdateThreadPool;
#endif

		TestStage()
		{

		}


		virtual void configRenderSystem(ERenderSystem systenName, RenderSystemConfigs& systemConfigs) override
		{
			systemConfigs.screenWidth = 1200;
			systemConfigs.screenHeight = 800;
			systemConfigs.numSamples = 4;
		}

		bool onInit() override
		{
			if( !BaseClass::onInit() )
				return false;


			if( !ShaderHelper::Get().init() )
				return false;

#if USE_PARALLEL_UPDATE
			int numThread = SystemPlatform::GetProcessorNumber();
			mUpdateThreadPool = std::make_unique<QueueThreadPool>();
			mUpdateThreadPool->init(numThread);
#endif

			Vec2i screenSize = ::Global::GetScreenSize();
			mRenderer = std::make_unique<CurveRenderer>();
			if( !mRenderer->initialize(screenSize) )
				return false;

			mCamera.setPos(Vector3(20, 20, 20));
			mCamera.setViewDir(Vector3(-1, -1, -1), Vector3(0, 0, 1));

			mMeshBuilder = std::make_unique<ShapeMeshBuilder>();

			wglSwapIntervalEXT(0);
			glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_NICEST);


			::Global::GUI().cleanupWidget();

			{
				Surface3D* surface = createSurfaceXY("x", Color4f(0.2, 0.6, 0.4, 0.3));
				//Surface3D* surface = createSurfaceXY("cos(0.1*(x*x+y*y) + 0.01*t)", Color4f(0.2, 0.6, 0.4, 0.3));
				surface = createSurfaceXY("sin(sqrt(x*x+y*y) + 0.1*t)", Color4f(1, 0.6, 0.4, 0.5));
				//surface = createSurfaceXY("sin(0.1*(x*x+y*y) + 0.01*t)", Color4f(0.2, 0.6, 0.1, 0.3) );

				GTextCtrl* textCtrl = new GTextCtrl(UI_ANY, Vec2i(100, 100), 200, nullptr);
				textCtrl->setValue( static_cast<SurfaceXYFunc*>( surface->getFunction() )->getExprString().c_str());
				textCtrl->onEvent = [surface,this](int event, GWidget* widget)
				{
					if ( event == EVT_TEXTCTRL_COMMITTED )
					{
						SurfaceXYFunc* func = (SurfaceXYFunc*)surface->getFunction();
						func->setExpr(widget->cast<GTextCtrl>()->getValue());
						surface->addUpdateBits(RUF_FUNCTION);
					}
					return false;
				};

				::Global::GUI().addWidget(textCtrl);
			}

			ProfileSystem::Get().resetSample();
			restart();
			return true;
		}

		Surface3D* createSurfaceXY(char const* expr , Color4f const& color )
		{
			Surface3D* surface = new Surface3D;
			SurfaceXYFunc* func = new SurfaceXYFunc;
			surface->setFunction(func);
			func->setExpr(expr);

			double Max = 10, Min = -10;
			surface->setRangeU(Range(Min, Max));
			surface->setRangeV(Range(Min, Max));

#if _DEBUG
			int NumX = 20, NumY = 20;
#else
			int NumX = 300, NumY = 300;
#endif

			surface->setDataSampleNum(NumX, NumY);
			surface->setColor(color);
			surface->visible(true);
			surface->addUpdateBits(RUF_ALL_UPDATE_BIT);
			mSurfaceList.push_back(surface);

			return surface;
		}

		void onEnd() override
		{
			BaseClass::onEnd();
		}

		void restart() {}

		void onUpdate(GameTimeSpan deltaTime) override
		{
			BaseClass::onUpdate(deltaTime);

			static float t = 0;
			t += 1;
			mMeshBuilder->setTime(t);

			{
				PROFILE_ENTRY("Update Surface");

#if USE_PARALLEL_UPDATE
				for (ShapeBase* current : mSurfaceList)
				{
					mUpdateThreadPool->addFunctionWork([this, current]()
					{
						current->update(*mMeshBuilder);
					});
				}
				mUpdateThreadPool->waitAllWorkComplete();
#else
				for (ShapeBase* current : mSurfaceList)
				{
					current->update(*mMeshBuilder);
				}
#endif
			}
		}

		void onRender(float dFrame) override
		{
			PROFILE_ENTRY("Stage.Render");
			RHIGraphics2D& g = Global::GetRHIGraphics2D();
			RHICommandList& commandList = RHICommandList::GetImmediateList();

			Vec2i screenSize = ::Global::GetScreenSize();

			RHIClearRenderTargets(commandList, EClearBits::Color | EClearBits::Depth, 
				&LinearColor(0.7f, 0.7f, 0.7f, 1), 1, 1);

			RHISetViewport(commandList, 0, 0, screenSize.x, screenSize.y);
			RHISetRasterizerState(commandList, TStaticRasterizerState<>::GetRHI());
			RHISetBlendState(commandList, TStaticBlendState<>::GetRHI());
			RHISetDepthStencilState(commandList, TStaticDepthStencilState<>::GetRHI());

			Matrix4 matProj = AdjProjectionMatrixForRHI( PerspectiveMatrix( Math::DegToRad(45.0f) , float(screenSize.x) / screenSize.y, 0.1f, 1000.0f) );
			mRenderer->getViewInfo().setupTransform(mCamera.getPos(), mCamera.getRotation(), matProj);
			mRenderer->beginRender(commandList);
			{
				mRenderer->drawAxis();
				for( ShapeBase* current : mSurfaceList )
				{
					if( current->isVisible() )
					{
						mRenderer->drawShape(*current);
					}
				}	
			}
			mRenderer->endRender();

			g.beginRender();
			//::Global::GetDrawEngine().drawProfile(Vec2i(400, 20));
			g.endRender();
		}

		MsgReply onMouse(MouseMsg const& msg) override
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

			return BaseClass::onMouse(msg);
		}

		MsgReply onKey(KeyMsg const& msg) override
		{
			if( msg.isDown())
			{
				float moveDist = 0.1;
				switch (msg.getCode())
				{
				case EKeyCode::R: restart(); break;
				case EKeyCode::W: mCamera.moveFront(moveDist); break;
				case EKeyCode::S: mCamera.moveFront(-moveDist); break;
				case EKeyCode::D: mCamera.moveRight(moveDist); break;
				case EKeyCode::A: mCamera.moveRight(-moveDist); break;
				case EKeyCode::Z: mCamera.moveUp(0.5); break;
				case EKeyCode::X: mCamera.moveUp(-0.5); break;
				case EKeyCode::F5: mRenderer->reloadShader(); break;
				case EKeyCode::Add: modifyParamIncrement(0.5); break;
				case EKeyCode::Subtract: modifyParamIncrement(2); break;
				}
			}
			return BaseClass::onKey(msg);
		}

		void modifyParamIncrement( float modifyFactor )
		{
			for( auto surface : mSurfaceList )
			{
				auto surface3D = static_cast<Surface3D*>(surface);
				surface3D->setIncrement(surface3D->getParamU().getIncrement() * modifyFactor, surface3D->getParamU().getIncrement() * modifyFactor);
			}
		}

		bool onWidgetEvent(int event, int id, GWidget* ui) override
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


}//namespace CB


REGISTER_STAGE_ENTRY("Curve Builder", CB::TestStage, EExecGroup::FeatureDev);