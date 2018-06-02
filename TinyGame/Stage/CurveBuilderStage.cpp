#include "TestStageHeader.h"

#include "CurveBuilder/ShapeCommon.h"
#include "CurveBuilder/ShapeFun.h"
#include "CurveBuilder/ShapeMaker.h"
#include "CurveBuilder/Surface.h"

#include "CurveBuilder/CurveRenderer.h"
#include "CurveBuilder/FPUCompiler.h"

#include "GLGraphics2D.h"
#include "ProfileSystem.h"

#include "RHI/DrawUtility.h"
#include "RHI/RenderContext.h"
#include "RHI/RHICommand.h"

#include "GL/wglew.h"

namespace CB
{
	using namespace RenderGL;

	class TemplateTestStage : public StageBase
	{
		typedef StageBase BaseClass;
	public:

		std::unique_ptr<ShapeMaker>   mSurfaceMaker;
		std::unique_ptr<CurveRenderer>   mRenderer;
		SimpleCamera  mCamera;
		std::vector<ShapeBase*> mSurfaceList;

		TemplateTestStage() 
		{

		}

		virtual bool onInit()
		{
			if( !BaseClass::onInit() )
				return false;

			if( !::Global::getDrawEngine()->startOpenGL(true,4) )
				return false;

			if( !InitGlobalRHIResource() )
				return false;

			if( !ShaderHelper::Get().init() )
				return false;


			Vec2i screenSize = ::Global::getDrawEngine()->getScreenSize();
			mRenderer.reset( new CurveRenderer );
			if( !mRenderer->initialize(screenSize) )
				return false;

			mCamera.setPos(Vector3(20, 20, 20));
			mCamera.setViewDir(Vector3(-1, -1, -1), Vector3(0, 0, 1));

			mSurfaceMaker.reset( new ShapeMaker );

			wglSwapIntervalEXT(0);
			glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_NICEST);


			::Global::GUI().cleanupWidget();

			{
				Surface3D* surface = createSurfaceXY("cos(0.1*(x*x+y*y) + 0.01*t)", Color4f(0.2, 0.6, 0.4, 0.3));
				surface = createSurfaceXY("sin(x)*cos(y+0.01*t)", Color4f(0.2, 0.6, 0.4, 0.5));
				surface = createSurfaceXY("sin(0.1*(x*x+y*y) + 0.01*t)", Color4f(0.2, 0.6, 0.1, 0.3) );

				GTextCtrl* textCtrl = new GTextCtrl(UI_ANY, Vec2i(100, 100), 200, nullptr);
				textCtrl->setValue( static_cast<SurfaceXYFun*>( surface->getFunction() )->getExprString().c_str());
				textCtrl->onEvent = [surface,this](int event, GWidget* widget)
				{
					if ( event == EVT_TEXTCTRL_ENTER )
					{
						SurfaceXYFun* fun = (SurfaceXYFun*)surface->getFunction();
						fun->setExpr(widget->cast<GTextCtrl>()->getValue());
						surface->addUpdateBit(RUF_FUNCTION);
					}
					return false;
				};

				::Global::GUI().addWidget(textCtrl);
			}

			//ProfileSystem::Get().reset();
			restart();
			return true;
		}

		Surface3D* createSurfaceXY(char const* expr , Color4f const& color )
		{
			Surface3D* surface = new Surface3D;
			SurfaceXYFun* fun = new SurfaceXYFun;
			surface->setFunction(fun);
			fun->setExpr(expr);

			double Max = 10, Min = -10;
			surface->setRangeU(Range(Min, Max));
			surface->setRangeV(Range(Min, Max));

#if _DEBUG
			int NumX = 100, NumY = 100;
#else
			int NumX = 300, NumY = 300;
#endif

			surface->setDataSampleNum(NumX, NumY);
			surface->setColor(color);
			surface->visible(true);
			surface->addUpdateBit(RUB_ALL_UPDATE_BIT);
			mSurfaceList.push_back(surface);

			return surface;
		}

		virtual void onEnd()
		{
			BaseClass::onEnd();
		}

		void restart() {}
		void tick() 		
		{
			static float t = 0;
			t += 1;
			mSurfaceMaker->setTime(t);

			{
				PROFILE_ENTRY("Update Surface");

#if USE_PARALLEL_UPDATE
				for( ShapeBase* current : mSurfaceList )
				{
					mSurfaceMaker->addUpdateWork([&,current]
					{
						current->update(*mSurfaceMaker);
					});
				}
				mSurfaceMaker->waitUpdateDone();
#else
				for( ShapeBase* current : mSurfaceList )
				{
					current->update(*mSurfaceMaker);
				}
#endif
			}

		}
		void updateFrame(int frame) {}

		virtual void onUpdate(long time)
		{
			BaseClass::onUpdate(time);

			int frame = time / gDefaultTickTime;
			for( int i = 0; i < frame; ++i )
				tick();

			updateFrame(frame);
		}

		void onRender(float dFrame)
		{
			PROFILE_ENTRY("Stage.Render");
			GLGraphics2D& g = Global::getGLGraphics2D();

			int width = ::Global::getDrawEngine()->getScreenWidth();
			int height = ::Global::getDrawEngine()->getScreenHeight();


			glClearDepth(1.0f);							// Depth Buffer Setup
			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
			glClearColor(0.7f, 0.7f, 0.7f, 1.0f);

			RHISetViewport(0, 0, width, height);
			RHISetDepthStencilState(TStaticDepthStencilState<>::GetRHI());

			glMatrixMode(GL_PROJECTION);
			Matrix4 matProj = PerspectiveMatrix( Math::Deg2Rad(45.0f) , (GLdouble)width / (GLdouble)height, 0.1f, 1000.0f);
			glLoadMatrixf(matProj);

			glMatrixMode(GL_MODELVIEW);
			Matrix4 matView = mCamera.getViewMatrix();
			glLoadMatrixf(matView);

			mRenderer->getViewInfo().setupTransform(matView, matProj);
			mRenderer->beginRender();
			{
				mRenderer->drawAxis();

				for( ShapeBase* current : mSurfaceList )
				{
					if( current->isVisible() )
						mRenderer->drawShape( *current);
				}		
			}
			mRenderer->endRender();

			g.beginRender();

			::Global::getDrawEngine()->drawProfile(Vec2i(400, 20));
			g.endRender();
		}

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

			float moveDist = 0.1;
			switch( key )
			{
			case Keyboard::eR: restart(); break;
			case 'W': mCamera.moveFront(moveDist); break;
			case 'S': mCamera.moveFront(-moveDist); break;
			case 'D': mCamera.moveRight(moveDist); break;
			case 'A': mCamera.moveRight(-moveDist); break;
			case 'Z': mCamera.moveUp(0.5); break;
			case 'X': mCamera.moveUp(-0.5); break;
			case Keyboard::eF5: mRenderer->reloadShaer();
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


}//namespace CB


REGISTER_STAGE("Curve Builder", CB::TemplateTestStage, EStageGroup::FeatureDev);