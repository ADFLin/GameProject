#ifndef CubeStage_h__
#define CubeStage_h__

#include "StageBase.h"

#include "Cube/CubeScene.h"
#include "Cube/CubeLevel.h"
#include "cube/CubeWorld.h"

#include "WinGLPlatform.h"
#include "GameRenderSetup.h"
#include "RHI/RHIGraphics2D.h"
#include "Renderer/SimpleCamera.h"

namespace Cube
{
	class Camera : public Render::SimpleCamera, public ICamera
	{
	public:
		virtual Vec3f getPos() { return Render::SimpleCamera::getPos(); }
		virtual Vec3f getViewDir() { return Render::SimpleCamera::getViewDir(); }
		virtual Vec3f getUpDir() { return Render::SimpleCamera::getUpDir(); }
	};

	class TestStage : public StageBase
		            , public IGameRenderSetup
	{
		typedef StageBase BaseClass;
	public:
		TestStage(){}

		virtual bool onInit();

		virtual void onUpdate(GameTimeSpan deltaTime)
		{
			BaseClass::onUpdate(deltaTime);

			mCamera.updatePosition(deltaTime);
			mDebugCamera.updatePosition(deltaTime);


			mLevel->tick(deltaTime);
			mScene->tick(deltaTime);
		}

		void onRender( float dFrame )
		{

			Vector2 screenSize = ::Global::GetScreenSize();
			mScene->render( mCamera );

			RHIGraphics2D& g = Global::GetRHIGraphics2D();
			g.beginRender();

			Vector2 crosschairSize = Vector2(5,5);
			RenderUtility::SetBrush(g, EColor::Red);
			g.drawRect( 0.5f * (screenSize - crosschairSize), crosschairSize);
			g.endRender();
		}

		void restart()
		{

		}

		MsgReply onMouse( MouseMsg const& msg )
		{
			Camera& cameraCtrl = mScene->mRenderEngine->mDebugCamera ? mDebugCamera : mCamera;

			static Vec2i oldPos = msg.getPos();
			if (msg.onMoving())
			{
				float rotateSpeed = 0.01;
				Vector2 off = rotateSpeed * Vector2(msg.getPos() - oldPos);
				cameraCtrl.rotateByMouse(off.x, off.y);
				oldPos = msg.getPos();
			}

			if ( msg.onLeftDown() )
			{
				BlockPosInfo info;
				BlockId id = mLevel->getWorld().rayBlockTest( mCamera.getPos() , mCamera.getViewDir() , 100 , &info );
				if ( id )
				{
					mLevel->getWorld().setBlockNotify( info.x , info.y , info.z , BLOCK_NULL );
				}
			}

			return BaseClass::onMouse(msg);
		}

		MsgReply onKey(KeyMsg const& msg)
		{
			Camera& cameraCtrl = mScene->mRenderEngine->mDebugCamera ? mDebugCamera : mCamera;
			if ( msg.isDown() )
			{
				switch (msg.getCode())
				{
				case EKeyCode::R: restart(); break;
				case EKeyCode::W: cameraCtrl.moveFront(0.5); break;
				case EKeyCode::S: cameraCtrl.moveFront(-0.5); break;
				case EKeyCode::Up: cameraCtrl.setPos(mCamera.getPos() + Vec3f(0, 0, 2)); break;
				case EKeyCode::Q:  
					if (mScene->mRenderEngine->mDebugCamera)
					{
						mScene->mRenderEngine->mDebugCamera = nullptr;
					}
					else
					{
						mScene->mRenderEngine->mDebugCamera = &mDebugCamera;
						mDebugCamera.setPos(mCamera.getPos());
					}
					break;
				}
			}
			return BaseClass::onKey(msg);
		}

		ERenderSystem getDefaultRenderSystem() override
		{
			return ERenderSystem::D3D11;
		}


		void configRenderSystem(ERenderSystem systenName, RenderSystemConfigs& systemConfigs) override;

	protected:
		Camera mDebugCamera;
		Camera mCamera;
		Level*       mLevel;
		Scene*       mScene;

	};
}
#endif // CubeStage_h__
