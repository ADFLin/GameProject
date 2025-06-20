#include "CubePCH.h"
#include "CubeStage.h"

#include "GameGlobal.h"
#include "GameGUISystem.h"

namespace Cube
{

	bool TestStage::onInit()
	{
		::Global::GUI().cleanupWidget();

		int posX = 0 * ChunkSize;
		int posY = 0 * ChunkSize;
		mCamera.setPos( Vec3f(posX, posY, 140 ) );
		mDebugCamera.setPos(Vec3f(0, 0, 140));

		Level::InitializeData();

		mLevel = new Level;
		mLevel->setupWorld();
		Vec2i screenSize = ::Global::GetScreenSize();
		mScene = new Scene(screenSize.x, screenSize.y);
		mScene->changeWorld( mLevel->getWorld() );

		restart();
		return true;
	}

	void TestStage::onEnd()
	{
		delete mScene;
		delete mLevel;
	}

	MsgReply TestStage::onMouse(MouseMsg const& msg)
	{
		if (mScene == nullptr)
		{
			return MsgReply::Unhandled();
		}

		Camera& cameraCtrl = mScene->mRenderEngine->mDebugCamera ? mDebugCamera : mCamera;

		static Vec2i oldPos = msg.getPos();
		if (msg.onMoving())
		{
			float rotateSpeed = 0.01;
			Vector2 off = rotateSpeed * Vector2(msg.getPos() - oldPos);
			cameraCtrl.rotateByMouse(off.x, off.y);
			oldPos = msg.getPos();
		}

		if (msg.onLeftDown())
		{
			BlockPosInfo info;
			BlockId id = mLevel->getWorld().rayBlockTest(mCamera.getPos(), mCamera.getViewDir(), 100, &info);
			if (id)
			{
				mLevel->getWorld().setBlockNotify(info.x, info.y, info.z, BLOCK_NULL);
			}
		}

		return BaseClass::onMouse(msg);
	}

	MsgReply TestStage::onKey(KeyMsg const& msg)
	{
		Camera& cameraCtrl = mScene->mRenderEngine->mDebugCamera ? mDebugCamera : mCamera;
		if (msg.isDown())
		{
			switch (msg.getCode())
			{
			case EKeyCode::B: bDrawBlockTexture = !bDrawBlockTexture; break;
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
			case EKeyCode::I:
				mScene->mRenderEngine->bWireframeMode = !mScene->mRenderEngine->bWireframeMode;
				break;
			}
		}
		return BaseClass::onKey(msg);
	}

	void TestStage::configRenderSystem(ERenderSystem systenName, RenderSystemConfigs& systemConfigs)
	{
		systemConfigs.screenWidth = 1280;
		systemConfigs.screenHeight = 800;
	}



}//namespace Cube