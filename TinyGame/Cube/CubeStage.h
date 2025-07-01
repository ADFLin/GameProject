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

		bool onInit();
		void onEnd();

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


			if (bDrawBlockTexture)
			{
				RenderUtility::SetBrush(g, EColor::White);

				Vector2 girdSize = Vector2(21, 21);
				Vector2 pos = Vector2(21, 90);
				Vector2 size = girdSize * Vector2(64, 32);
				g.drawTexture(*mScene->mRenderEngine->mTexBlockAtlas, pos, size);
				for (int i = 0; i < 32; ++i)
				{
					g.drawTextF(pos + i * Vector2(girdSize.x, 0) - Vector2(0, girdSize.y), girdSize, "%d", i);
					g.drawTextF(pos + (i + 32) * Vector2(girdSize.x, 0) - Vector2(0, girdSize.y), girdSize, "%d", (i +32) );
					g.drawTextF(pos + i * Vector2(0, girdSize.y) - Vector2(girdSize.x, 0), girdSize, "%d", i);
				}
			}


			g.endRender();
		}

		void restart()
		{

		}


		bool bDrawBlockTexture = false;


		MsgReply onMouse( MouseMsg const& msg );

		MsgReply onKey(KeyMsg const& msg);

		ERenderSystem getDefaultRenderSystem() override
		{
			return ERenderSystem::D3D11;
		}


		void configRenderSystem(ERenderSystem systenName, RenderSystemConfigs& systemConfigs) override;



	protected:
		Camera mDebugCamera;
		Camera mCamera;
		Level* mLevel = nullptr;
		Scene* mScene = nullptr;

	};
}
#endif // CubeStage_h__
