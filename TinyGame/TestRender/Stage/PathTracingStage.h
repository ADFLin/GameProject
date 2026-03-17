#include "Stage/TestRenderStageBase.h"
#include "GameRenderSetup.h"

#include "RHI/RHICommand.h"
#include "PropertySet.h"

#include "PathTracing/PathTracingCommon.h"
#include "PathTracing/PathTracingRenderer.h"
#include "PathTracing/PathTracingEditor.h"

using namespace Render;
using namespace PathTracing;


class PathTracingStage : public TestRenderStageBase
#if TINY_WITH_EDITOR
						, public PathTracing::IEditorContext
						, public PathTracingEditor
#endif
{
	using BaseClass = TestRenderStageBase;
public:

	enum
	{
		UI_StatsMode = BaseClass::NEXT_UI_ID,


		NEXT_UI_ID,
	};


	PathTracingStage();

	bool onInit() override;
	void onEnd() override;

	GWidget* mFrame;
#if TINY_WITH_EDITOR

	// IEditorContext
	PathTracing::SceneData&    getSceneData() override { return mSceneData; }
	PathTracing::Renderer&     getRenderer() override { return mRenderer; }
	PathTracing::RenderConfig& getRenderConfig() override { return mRenderConfig; }
	Mesh&         getMesh(int meshId) override { return SharedAssetData::getMesh(meshId); }
	SimpleCamera& getMainCamera() { return mCamera; }


	void notifyDataChanged(EDataActionType type) override 
	{ 
		if (type == EDataActionType::ModifyRenderConfig)
		{
			mFrame->refresh();
		}
		else
		{
			bDataChanged = true;
			mView.frameCount = 0;
		}
	}

	RHITexture2D& getDisplayTexture() 
	{ 
		if (mLastOutputTexture)
			return *mLastOutputTexture;
		return mSceneRenderTargets.getFrameTexture(); 
	}

	void executeCommand(EEditorCommand commnad)
	{
		switch (commnad)
		{
		case PathTracing::EEditorCommand::SaveCamera:
			saveCameraTransform();
			break;
		case PathTracing::EEditorCommand::LoadCamera:
			loadCameaTransform();
			break;
		case PathTracing::EEditorCommand::ReloadScene:
			loadSceneData();
			break;
		}
	}

	void markViewDirty() override { restart(); }
#endif
	void loadScene(char const* path);
	void saveScene(char const* path);
	void addMeshFromFile(char const* filePath);

	void loadCameaTransform()
	{
		char const* text;
		if (::Global::GameConfig().tryGetStringValue("LastPos", "RayTracing", text))
		{
			sscanf(text, "%f %f %f", &mLastPos.x, &mLastPos.y, &mLastPos.z);
			mCamera.setPos(mLastPos);
		}
		if (::Global::GameConfig().tryGetStringValue("LastRoation", "RayTracing", text))
		{
			sscanf(text, "%f %f %f", &mLastRoation.x, &mLastRoation.y, &mLastRoation.z);
			mCamera.setRotation(mLastRoation.z, mLastRoation.x, mLastRoation.y);
		}
	}

	void saveCameraTransform()
	{
		InlineString<128> text;
		text.format("%g %g %g", mLastPos.x, mLastPos.y, mLastPos.z);
		::Global::GameConfig().setKeyValue("LastPos", "RayTracing", text.c_str());
		text.format("%g %g %g", mLastRoation.x, mLastRoation.y, mLastRoation.z);
		::Global::GameConfig().setKeyValue("LastRoation", "RayTracing", text.c_str());
	}

	void restart() { bDataChanged = true; mView.frameCount = 0; }
	void onUpdate(GameTimeSpan deltaTime) override;
	void updateMeshImportTransform(int index);
	void saveImage();


	Vector3 mLastPos;
	Vector3 mLastRoation;

	bool mbDrawDebug = false;
	int  mDebugShowDepth = 1;

	PathTracing::ISceneScript* mScript = nullptr;
	PathTracing::SceneData mSceneData;
	PathTracing::Renderer mRenderer;
	PathTracing::RenderConfig mRenderConfig;

	FrameRenderTargets mSceneRenderTargets;

	EDebugDsiplayMode mLastDebugDisplayModeRender = EDebugDsiplayMode::None;
	RHITexture2D* mLastOutputTexture = nullptr;

	void onRender(float dFrame) override;


	MsgReply onMouse(MouseMsg const& msg) override
	{
		static Vec2i oldPos = msg.getPos();

		if (msg.onLeftDown())
		{
			oldPos = msg.getPos();
		}
		if (msg.onMoving() && msg.isLeftDown())
		{
			float rotateSpeed = 0.01;
			Vector2 off = rotateSpeed * Vector2(msg.getPos() - oldPos);
			mCamera.rotateByMouse(off.x, off.y);
			oldPos = msg.getPos();
		}

		return BaseClass::onMouse(msg);
	}

	MsgReply onKey(KeyMsg const& msg) override;



	bool onWidgetEvent(int event, int id, GWidget* ui) override
	{
		switch (id)
		{
		case UI_StatsMode:
			{
				mRenderConfig.debugDisplayMode = EDebugDsiplayMode(ui->cast<GChoice>()->getSelection());
			}
			return true;
		default:
			break;
		}

		return BaseClass::onWidgetEvent(event, id, ui);
	}

	ERenderSystem getDefaultRenderSystem() override
	{
		return ERenderSystem::None;
	}

	void configRenderSystem(ERenderSystem systenName, RenderSystemConfigs& systemConfigs) override;

	bool setupRenderResource(ERenderSystem systemName) override;

	bool loadSceneData();

	void preShutdownRenderSystem(bool bReInit = false) override;



	struct Line
	{

		Vector2 start;
		Vector2 end;
		Color3f color;
	};
	TArray< Line > mPaths;
	

	void addLine(Vector3 p1, Vector3 p2, Color3f color)
	{
		Line line;
		line.start.x = p1.x;
		line.start.y = p1.y;
		line.end.x = p2.x;
		line.end.y = p2.y;
		line.color = color;
		mPaths.push_back(line);
	}



	Vector3 spherePos = Vector3::Zero();
	float radius = 100;
	RenderTransform2D mWorldToScreen;
	RenderTransform2D mScreenToWorld;


	void generatePath()
	{
		float refractiveIndex = 1.5;
		Vector3 viewPos = Vector3(400, 0, 0);


		int num = 250;
		auto TestRay = [&](Vector3 rayDir)
		{
			float dists[2];
			if (!LineSphereTest(viewPos, rayDir, spherePos, radius, dists))
				return;


			Color3f CRed(1, 0, 0);
			Color3f CYellow(1, 1, 0);

			Vector3 hitPos = viewPos + dists[1] * rayDir;
			addLine(viewPos, hitPos, CRed);
			
			{
				Vector3 N = GetNormal(hitPos - spherePos);
				Vector3 V = GetNormal(viewPos - hitPos);

				float cosI = N.dot(V);
				float sinI = Math::Sqrt(1 - Math::Square(cosI));
				float sinO = sinI / refractiveIndex;
				float cosO = Math::Sqrt(1 - Math::Square(sinO));
				rayDir = -GetNormal(N * cosO + GetNormal(V - cosI * N) * sinO);
			}


			LineSphereTest(hitPos, rayDir, spherePos, radius, dists);

			Vector3 hitPos2 = hitPos + dists[1] * rayDir;

	
			{
				Vector3 N = GetNormal(spherePos - hitPos2);
				Vector3 V = -rayDir;

				float cosI = N.dot(V);
				float sinI = Math::Sqrt(1 - Math::Square(cosI));
				float sinO = refractiveIndex * sinI;
				if (sinO <= 1)
				{
					float cosO = Math::Sqrt(1 - Math::Square(sinO));
					rayDir = -GetNormal(N * cosO + GetNormal(V - cosI * N) * sinO);
					addLine(hitPos, hitPos2, CRed);
					addLine(hitPos2, hitPos2 + 100 * rayDir, CRed);
				}
				else
				{
					addLine(hitPos, hitPos2, CYellow);
				}
			}
		};
		

		for (int i = 0; i < num; ++i)
		{
			float theta = 0.5 * Math::PI * i / float(num);

			float c , s;
			Math::SinCos(theta, s, c);

			Vector3 rayDir = Vector3(c, s, 0);
			TestRay(rayDir);
		}
	}
};