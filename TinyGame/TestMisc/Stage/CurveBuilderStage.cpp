#include "Stage/TestStageHeader.h"

#include "GameRenderSetup.h"

#include "CurveBuilder/ShapeCommon.h"
#include "CurveBuilder/ShapeFunction.h"
#include "CurveBuilder/ShapeMeshBuilder.h"
#include "CurveBuilder/Surface.h"

#include "CurveBuilder/CurveRenderer.h"
#include "CurveBuilder/ExpressionCompiler.h"

#include "Async/AsyncWork.h"
#include "RHI/RHIGraphics2D.h"
#include "ProfileSystem.h"

#include "RHI/DrawUtility.h"
#include "RHI/RenderContext.h"
#include "RHI/RHICommand.h"
#include "RHI/ShaderManager.h"
#include "Renderer/SimpleCamera.h"

#include "FunctionTraits.h"
#include <memory>


#define STR_INNER(A) #A
#define STR(A) STR_INNER(A)


namespace CB
{
	using namespace Render;

	#define TEST_EXPR sin(sqrt(x*x + y*y) - 1*t) + cos(sqrt(x*x + y*y) + 3*t)
	static RealType GTime;
	char const* TestExpr = STR(TEST_EXPR);

#define t GTime
	RealType MyFunc(RealType x, RealType y)
	{
		return TEST_EXPR;
	}
	FloatVector MyFuncV(FloatVector x, FloatVector y)
	{
		return TEST_EXPR;
	}
#undef t

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

		static RealType Test(RealType x, RealType  y)
		{
			return x + y;
			//LogMsg("x = %f, y = %f", x , y);
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

			GTime = 0.0f;

			mMeshBuilder = std::make_unique<ShapeMeshBuilder>();
			mMeshBuilder->bindTime(GTime);
			mMeshBuilder->initializeRHI();
			mMeshBuilder->getSymbolDefine().defineFunc("Test", Test);
	
			::Global::GUI().cleanupWidget();

			{
				Surface3D* surface;
				//surface = createSurfaceXY("x", Color4f(0.2, 0.6, 0.4, 1.0));
	
				//surface = createSurfaceXY(MyFunc, Color4f(1, 0.6, 0.4, 0.95));
				surface = createSurfaceXY(TestExpr, Color4f(0.2, 0.6, 1.0, 0.90));
				surface->setTransform(Matrix4::Translate(0,0,1));
				//surface = createSurfaceXY("Test(1,x + y)", Color4f(0.2, 0.6, 0.4, 0.3));
				//surface = createSurfaceXY("10 - x", Color4f(0.2, 0.6, 0.4, 0.3));
				//surface = createSurfaceXY("sqrt(x*x + y*y)", Color4f(1, 0.6, 0.4, 0.3));
				//surface = createSurfaceXY("(5 - x)*sin(t)", Color4f(1, 0.6, 0.4, 0.3));
				//surface = createSurfaceXY("sin(sqrt(x*x+y*y))", Color4f(1, 0.6, 0.4, 0.5));
				//surface = createSurfaceXY("sin(0.1*(x*x+y*y) + 0.01*t)", Color4f(0.2, 0.6, 0.1, 0.3) );
				mSurface = surface;

				//createSphareSurface(Color4f(0.2, 0.6, 0.1, 0.3), false);

				mTextCtrl = new GTextCtrl(UI_ANY, Vec2i(100, 100), 300, nullptr);
				updateUI();

				::Global::GUI().addWidget(mTextCtrl);
			}


			auto frame = WidgetUtility::CreateDevFrame();
			frame->addCheckBox("Pause", bPauseTime);

			ProfileSystem::Get().resetSample();
			restart();
			return true;
		}


		bool bPauseTime = false;

		Surface3D* mSurface;
		GTextCtrl* mTextCtrl;

		void updateUI()
		{

			if (mSurface->getFunction()->getEvalType() == EEvalType::Native)
			{
				mTextCtrl->setValue("Native Func");
				mTextCtrl->onEvent = nullptr;
			}
			else
			{
				mTextCtrl->setValue(static_cast<SurfaceXYFunc*>(mSurface->getFunction())->getExprString().c_str());
				mTextCtrl->onEvent = [this](int event, GWidget* widget)
				{
					if (event == EVT_TEXTCTRL_COMMITTED)
					{
						auto func = (SurfaceXYFunc*)mSurface->getFunction();
						func->setExpr(widget->cast<GTextCtrl>()->getValue());
						mSurface->addUpdateBits(RUF_FUNCTION);
					}
					return false;
				};
			}
		}

		template< typename TFunc >
		Surface3D* createSurfaceXY(TFunc funcPtr, Color4f const& color)
		{
			NativeSurfaceXYFunc* func = new NativeSurfaceXYFunc;
			func->setFunc(funcPtr);
			return createSurface(func, color);
		}

		Surface3D* createSurfaceXY(char const* expr, Color4f const& color, bool bUseGPU = false)
		{
			SurfaceXYFunc* func = new SurfaceXYFunc(bUseGPU);
			func->setExpr(expr);
			return createSurface(func, color);
		}

		Surface3D* createSurface(SurfaceFunc* func, Color4f const& color)
		{
			Surface3D* surface = new Surface3D;
			surface->setFunction(func);
			double Max = 10, Min = -10;
			surface->setRangeU(Range(Min, Max));
			surface->setRangeV(Range(Min, Max));

#if _DEBUG && 0
			int NumX = 20, NumY = 20;
#else
			int NumX = 300, NumY = 300;
			//int NumX = 1000, NumY = 1000;
#endif

			surface->setDataSampleNum(NumX, NumY);
			surface->setColor(color);
			surface->visible(true);
			surface->addUpdateBits(RUF_ALL_UPDATE_BIT);
			mSurfaceList.push_back(surface);

			return surface;
		}


		Surface3D* createSphareSurface(Color4f const& color, bool bUseGPU = false)
		{
			char const* exprList[] =
			{
				"10 * cos(u) * sin(v)",
				"10 * sin(u) * sin(v)",
				"10 * cos(v)",
			};

			Surface3D* surface = new Surface3D;

			double Max = 10, Min = -10;
			surface->setRangeU(Range(0, 2 * Math::PI));
			surface->setRangeV(Range(-Math::PI, Math::PI));

			SurfaceUVFunc* func = new SurfaceUVFunc(bUseGPU);
			for (int i = 0; i < 3; ++i)
				func->setExpr(i, exprList[i]);
			surface->setFunction(func);

#if _DEBUG && 0
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


		bool bGPUUpdateRequired = false;
		void onUpdate(GameTimeSpan deltaTime) override
		{
			BaseClass::onUpdate(deltaTime);

			static float t = 0;
			if (!bPauseTime)
			{
				GTime += deltaTime.value;
				bGPUUpdateRequired = true;
			}

			{
				PROFILE_ENTRY("Update Surface", "CB");
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
					if (current->getFunction()->getEvalType() == EEvalType::GPU)
						continue;
					current->update(*mMeshBuilder, bPauseTime);
				}
#endif
			}
		}

		void onRender(float dFrame) override
		{
			PROFILE_ENTRY("Stage.Render");
			
			if (bGPUUpdateRequired || true)
			{
				bGPUUpdateRequired = false;
				for (ShapeBase* current : mSurfaceList)
				{
					if (current->getFunction()->getEvalType() != EEvalType::GPU)
						continue;
					current->update(*mMeshBuilder, bPauseTime);
				}
			}

			RHIGraphics2D& g = Global::GetRHIGraphics2D();
			RHICommandList& commandList = RHICommandList::GetImmediateList();

			Vec2i screenSize = ::Global::GetScreenSize();

			RHISetFrameBuffer(commandList, nullptr);
			RHIClearRenderTargets(commandList, EClearBits::Color | EClearBits::Depth, 
				&LinearColor(0.7f, 0.7f, 0.7f, 1), 1);

			RHISetViewport(commandList, 0, 0, screenSize.x, screenSize.y);
			RHISetRasterizerState(commandList, TStaticRasterizerState<>::GetRHI());
			RHISetBlendState(commandList, TStaticBlendState<>::GetRHI());
			RHISetDepthStencilState(commandList, TStaticDepthStencilState<>::GetRHI());


			Matrix4 matProj = PerspectiveMatrix(Math::DegToRad(45.0f), float(screenSize.x) / screenSize.y, 0.1f, 1000.0f);
			mRenderer->getViewInfo().setupTransform(mCamera.getPos(), mCamera.getRotation(), matProj);
			mRenderer->getViewInfo().updateRHIResource();

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


#if 0
			g.beginRender();
			//::Global::GetDrawEngine().drawProfile(Vec2i(400, 20));
			g.endRender();
#endif
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
				case EKeyCode::Q:
					{
						switch (mSurface->getFunction()->getEvalType())
						{
						case EEvalType::Native:
							{
								SurfaceXYFunc* func = new SurfaceXYFunc(true);
								func->setExpr(TestExpr);
								mSurface->setFunction(func);
							}
							break;
						case EEvalType::GPU:
							{
								SurfaceXYFunc* func = new SurfaceXYFunc(false);
								func->setExpr(TestExpr);
								mSurface->setFunction(func);
							}
							break;
						case EEvalType::CPU:
							{
								NativeSurfaceXYFunc* func = new NativeSurfaceXYFunc;
								func->setFunc(MyFuncV);
								mSurface->setFunction(func);
							}
							break;
						default:
							break;
						}
						updateUI();
					} 
					break;
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

		ERenderSystem getDefaultRenderSystem() override
		{
			return ERenderSystem::OpenGL;
		}

	protected:
	};


}//namespace CB


REGISTER_STAGE_ENTRY("Curve Builder", CB::TestStage, EExecGroup::FeatureDev);