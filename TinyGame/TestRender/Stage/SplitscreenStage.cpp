#include "Stage/TestStageHeader.h"

#include "GameRenderSetup.h"
#include "RHI/ShaderProgram.h"
#include "FileSystem.h"
#include "RHI/ShaderManager.h"
#include "RHI/RHICommand.h"
#include "RHI/DrawUtility.h"
#include "RHI/GpuProfiler.h"
#include "RHI/RHIGraphics2D.h"
#include "RHI/RHIUtility.h"
#include "RenderDebug.h"
#include "Rect.h"
#include "TinyCore/DebugDraw.h"

using namespace Render;

class SplitScreenTestStage : public StageBase
				           , public IGameRenderSetup
{
	using BaseClass = StageBase;
public:
	SplitScreenTestStage() {}



	struct Player 
	{
		Vector2 pos;
		Vector2 vel = Vector2::Zero();
		Vector2 moveDir = Vector2::Zero();
		int viewIndex;

		void update(float dt)
		{
			float const moveAcc = 5;

			Vector2 acc = Math::GetNormalSafe(moveDir) * moveAcc;

			float const maxSpeed = 10;
			float const damping = 0.5;
			if (acc.length2() > 0)
			{
				vel += moveDir;
				float speed = vel.length();
				if (speed > maxSpeed)
				{
					vel *= maxSpeed / speed;
				}
			}
			else
			{
				vel *= damping;
			}
			pos += vel * dt;
		}
	};

	TArray< Player > mPlayers;

	bool onInit() override
	{
		if (!BaseClass::onInit())
			return false;
		::Global::GUI().cleanupWidget();

		auto frame = WidgetUtility::CreateDevFrame();
		restart();
		return true;
	}

	void onEnd() override
	{
		BaseClass::onEnd();
	}

	void restart() 
	{
		mPlayers.resize(2);
		{
			auto& player = mPlayers[0];
			player.pos = Vector2(0, 0);
		}

		{
			auto& player = mPlayers[1];
			player.pos = Vector2(5, 0);
		}

	}
	void tick() 
	{
		DrawDebugClear();

		float deltaTime = gDefaultTickTime / 1000.0f;
		for (int playerIndex = 0; playerIndex < mPlayers.size(); ++playerIndex)
		{
			auto& player = mPlayers[playerIndex];
			player.update(deltaTime);
		}

		updateViews(deltaTime);
	}


	static Vector2 CalcPolygonCentroid(TArrayView<Vector2 const> posList)
	{
		int indexPrev = posList.size() - 1;
		float totalWeight = 0;
		Vector2 pos = Vector2::Zero();
		for (int index = 0; index < posList.size(); ++index)
		{
			Vector2 const& p1 = posList[indexPrev];
			Vector2 const& p2 = posList[index];
			float area = p1.cross(p2);
			pos += (p1 + p2) * area;
			totalWeight += area;
			indexPrev = index;
		}

		return pos / (3 * totalWeight);
	}
	static Vector2 GetProjection(Vector2 const& v, Vector2 const& dir)
	{
		return v.dot(dir) * dir;
	}


	void updateViews(float deltaTime)
	{
		Vec2i screenSize = ::Global::GetScreenSize();

		float defaultZoom = float(screenSize.x) / 100;

		auto SeupSingleView = [&](Vector2 const& lookPos)
		{
			mNumView = 1;
			auto& view = mViews[0];
			view.index = 0;
			view.screenOffset = Vector2(0, 0);
			view.pos = lookPos;
			view.zoom = defaultZoom;
			view.area = { Vector2(0,0) , Vector2(screenSize.x, 0) , screenSize , Vector2(0, screenSize.y) };

			for (auto& player : mPlayers)
			{
				player.viewIndex = view.index;
			}
		};

		RenderTransform2D worldToScreen = RenderTransform2D::LookAt(screenSize, Vector2(0, 0), Vector2(0, 1), defaultZoom, true);


		if (mPlayers.size() == 1)
		{
			SeupSingleView(mPlayers[0].pos);
			return;
		}
		else if (mPlayers.size() == 2)
		{
			Player& playerA = mPlayers[0];
			Player& playerB = mPlayers[1];
			Vector2 centerWorld = 0.5 * (playerA.pos + playerB.pos);

			Vector2 posA = worldToScreen.transformPosition(playerA.pos);
			Vector2 posB = worldToScreen.transformPosition(playerB.pos);
			Vector2 offset = posB - posA;

			Vector2 dir = offset;
			float distScreen = dir.normalize();
			if (distScreen == 0)
			{
				SeupSingleView(centerWorld);
				return;
			}

			float distances[2];
			Vector2 center = 0.5 * Vector2(screenSize);
			Vector2 normal = Vector2(dir.y, -dir.x);
			Math::LineAABBTest(center, normal, Vector2(0, 0), screenSize, distances);

			Vector2 p0 = center + normal * distances[0];
			Vector2 p1 = center + normal * distances[1];

			float error = 1e-4;
			if (Math::Abs(p0.y) < error || Math::Abs(p1.y) < error)
			{
				if (Math::Abs(p1.y) < error)
				{
					using namespace std;
					swap(p0, p1);
				}

				p0.y = 0;
				p1.y = screenSize.y;

				int indexLeft = dir.x > 0 ? 0 : 1;
				mViews[indexLeft].area = 
				{
					Vector2(0,0) , p0 , p1 , Vector2(0, screenSize.y)
				};
				mViews[1 - indexLeft].area =
				{
					p0, Vector2(screenSize.x, 0) , screenSize,  p1 ,
				};
			}
			else
			{
				if (Math::Abs(p1.x) < error)
				{
					using namespace std;
					swap(p0, p1);
				}

				p0.x = 0;
				p1.x = screenSize.x;
				int indexTop = dir.y > 0 ? 0 : 1;
				mViews[indexTop].area =
				{
					Vector2(0,0) , Vector2(screenSize.x, 0) , p1 , p0
				};
				mViews[1 - indexTop].area =
				{
					p0, p1 , screenSize,  Vector2(0, screenSize.y)
				};
			}

			for (int i = 0; i < 2; ++i)
			{
				View& view = mViews[i];
				Vector2 pos = CalcPolygonCentroid(view.area);
				view.screenOffset = pos - center;

				DrawDebugPoint(pos, Color3f(1, 0, 0), 5);
				Vector2 pos2 = center + GetProjection(view.screenOffset, dir);
				DrawDebugPoint(pos2, Color3f(0, 1, 0), 5);
				DrawDebugLine(pos, pos2, Color3f(1, 1, 0), 2);
			}

			Vector2 p0S = center + GetProjection(mViews[0].screenOffset, dir);
			Vector2 p1S = center + GetProjection(mViews[1].screenOffset, dir);
			float dist2 = Math::Distance(p0S, p1S);
	
			float alpha = (distScreen - dist2) / 250;
			if (alpha <= 0)
			{
				SeupSingleView(centerWorld);
				return;
			}

			if (alpha > 1)
				alpha = 1;

			alpha = alpha * alpha * (3.0f - 2.0f*alpha);

			mNumView = 2;
			for (int i = 0; i < 2; ++i)
			{
				View& view = mViews[i];
				view.screenOffset = Math::LinearLerp(GetProjection(view.screenOffset, dir), view.screenOffset, alpha);
				view.index = i;
				view.pos = mPlayers[i].pos;
				view.zoom = defaultZoom;
				mPlayers[i].viewIndex = i;
			}
			return;
		}
		else if (mPlayers.size() == 3)
		{
			Player& playerA = mPlayers[0];
			Player& playerB = mPlayers[1];
			Player& playerC = mPlayers[2];
			Vector2 posA = worldToScreen.transformPosition(playerA.pos);
			Vector2 posB = worldToScreen.transformPosition(playerB.pos);
			Vector2 posC = worldToScreen.transformPosition(playerC.pos);

			Vector2 dBA = posB - posA;
			Vector2 dCA = posC - posA;

			if (dBA.cross(dCA) < 1e-4)
			{





			}


			Vector2 baseScreen = Math::GetCircumcirclePoint(posA, posB, posC);


			float coords[3];
			if (!Math::Barycentric(baseScreen, posA, posB, posC, coords))
			{




			}
			else
			{




			}
		}
	}
	void updateFrame(int frame) {}

	void onUpdate(long time) override
	{
		BaseClass::onUpdate(time);

		int frame = time / gDefaultTickTime;
		for (int i = 0; i < frame; ++i)
			tick();

		updateFrame(frame);
	}

	struct View
	{
		int     index;
		Vector2 pos;
		Vector2 screenOffset;
		float   zoom;
		TRect<float>    areaRect;
		TArray<Vector2> area;
	};

	View mViews[4];
	int  mNumView;
	static constexpr int ColorMap[] = { EColor::Red, EColor::Green, EColor::Blue };


	void render(RHICommandList& commandList, View const& view)
	{

		RHIGraphics2D& g = ::Global::GetRHIGraphics2D();
		g.enablePen(false);
		g.setTextureState(nullptr);
		g.resetRenderState();
#if 1
		g.setCustomRenderState([&view](RHICommandList& commandList, RenderBatchedElement& element)
		{
			RHISetBlendState(commandList, TStaticBlendState< CWM_None >::GetRHI());
			RHISetDepthStencilState(commandList, TStaticDepthStencilState<
				false, ECompareFunc::Always, true, ECompareFunc::Always, 
				EStencil::Replace, EStencil::Replace, EStencil::Replace 
			>::GetRHI(), BIT(view.index));

		});
		g.drawPolygon(view.area.data(), view.area.size());
#endif
#if 1

		auto SetupDrawState = [&](RHICommandList& commandList)
		{
			RHISetBlendState(commandList, TStaticBlendState<>::GetRHI());
			RHISetDepthStencilState(commandList, TStaticDepthStencilState<
				true, ECompareFunc::Always, true, ECompareFunc::Equal,
				EStencil::Keep, EStencil::Keep, EStencil::Keep
			>::GetRHI(), BIT(view.index));
		};

		Vec2i screenSize = ::Global::GetScreenSize();
		RenderTransform2D worldToScreen = RenderTransform2D::LookAt(screenSize, view.pos, Vector2(0, 1), view.zoom, true) * RenderTransform2D::Translate(view.screenOffset);

		g.pushXForm();
		g.transformXForm(worldToScreen, false);

		//RenderUtility::SetBrush(g, view.index == 0 ? EColor::Red : EColor::Green);
		//g.drawRect(Vector2::Zero(), screenSize);

#if 1
		g.setTextureState(mTex);
		//g.setSampler(TStaticSamplerState<ESampler::Trilinear>::GetRHI());
		g.setCustomRenderState([&](RHICommandList& commandList, RenderBatchedElement& element, GraphicsDefinition::RenderState const& state)
		{
			SetupDrawState(commandList);
			BatchedRender::SetupShaderState(commandList, g.getBaseTransform(), state);
		});
		RenderUtility::SetBrush(g, EColor::White);
		float len = 80;
		g.drawTexture(Vector2(-len, -len), Vector2(len, len));
		g.drawTexture(Vector2(   0, -len), Vector2(len, len));
		g.drawTexture(Vector2(   0,    0), Vector2(len, len));
		g.drawTexture(Vector2(-len,    0), Vector2(len, len));

#endif


		g.setTextureState(nullptr);
		g.resetRenderState();
		g.setCustomRenderState([&](RHICommandList& commandList, RenderBatchedElement& element, GraphicsDefinition::RenderState const& state)
		{
			SetupDrawState(commandList);
		});

		RenderUtility::SetPen(g, EColor::Red);
		g.drawLine(Vector2(0, 0), Vector2(1000, 0));
		RenderUtility::SetPen(g, EColor::Green);
		g.drawLine(Vector2(0, 0), Vector2(0, 1000));


		RenderUtility::SetPen(g, EColor::Black);
		for (int playerIndex = 0; playerIndex < mPlayers.size(); ++playerIndex)
		{
			auto& player = mPlayers[playerIndex];
			RenderUtility::SetBrush(g, ColorMap[playerIndex]);
			g.drawCircle(player.pos, 1);
		}

		g.popXForm();
#endif
		g.flush();
		g.resetRenderState();
	}

	void onRender(float dFrame) override
	{
		Vec2i screenSize = ::Global::GetScreenSize();

		RHICommandList& commandList = RHICommandList::GetImmediateList();
		RHISetFrameBuffer(commandList, nullptr);
		RHIClearRenderTargets(commandList, EClearBits::Color | EClearBits::Stencil , &LinearColor(0, 0, 0, 1), 1);

		RHIGraphics2D& g = ::Global::GetRHIGraphics2D();
		g.beginRender();

		for (int i = 0; i < mNumView; ++i)
		{
			render(commandList, mViews[i]);
		}

		g.setPenWidth(4);
		RenderUtility::SetBrush(g, EColor::Null);

		int controlViewIndex = INDEX_NONE;
		if (mPlayers.size() > 1)
		{
			controlViewIndex = mPlayers[mPlayerControlIndex].viewIndex;
		}

		for (int i = 0; i < mNumView; ++i)
		{
			auto& view = mViews[i];
			if (view.index == controlViewIndex )
				continue;

			RenderUtility::SetPen(g, ColorMap[view.index]);
			g.drawPolygon(view.area.data(), view.area.size());
		}

		if (controlViewIndex != INDEX_NONE)
		{
			auto& view = mViews[controlViewIndex];
			RenderUtility::SetPen(g, EColor::Yellow);
			g.drawPolygon(view.area.data(), view.area.size());
		}

		DrawDebugCommit(::Global::GetIGraphics2D());
		g.endRender();
	}

	MsgReply onMouse(MouseMsg const& msg) override
	{
		return BaseClass::onMouse(msg);
	}


	int mPlayerControlIndex = 0;

	MsgReply onKey(KeyMsg const& msg) override
	{
		auto& playerControlled = mPlayers[mPlayerControlIndex];
		if (msg.isDown())
		{
			switch (msg.getCode())
			{
			case EKeyCode::R: restart(); break;
			case EKeyCode::A: playerControlled.moveDir.x = -1; break;
			case EKeyCode::D: playerControlled.moveDir.x =  1; break;
			case EKeyCode::S: playerControlled.moveDir.y = -1; break;
			case EKeyCode::W: playerControlled.moveDir.y =  1; break;
			case EKeyCode::Z: 
				--mPlayerControlIndex; 
				if (mPlayerControlIndex < 0) 
					mPlayerControlIndex += mPlayers.size(); 
				break;
			case EKeyCode::X:
				++mPlayerControlIndex;
				if (mPlayerControlIndex >= mPlayers.size())
					mPlayerControlIndex = 0;
				break;
			case EKeyCode::N:
				{
					mPlayers.resize(mPlayers.size() + 1);
					mPlayers.back().pos = playerControlled.pos;
				}
				break;
			case EKeyCode::M:
				if ( mPlayers.size() > 1)
				{
					mPlayers.pop_back();
				}
				break;
			}
		}
		else
		{
			switch (msg.getCode())
			{
			case EKeyCode::A: if (playerControlled.moveDir.x == -1) playerControlled.moveDir.x = 0; break;
			case EKeyCode::D: if (playerControlled.moveDir.x == 1)  playerControlled.moveDir.x = 0; break;
			case EKeyCode::S: if (playerControlled.moveDir.y == -1) playerControlled.moveDir.y = 0; break;
			case EKeyCode::W: if (playerControlled.moveDir.y == 1)  playerControlled.moveDir.y = 0; break;
			}
		}
		return BaseClass::onKey(msg);
	}

	bool onWidgetEvent(int event, int id, GWidget* ui) override
	{
		switch (id)
		{
		default:
			break;
		}

		return BaseClass::onWidgetEvent(event, id, ui);
	}

	RHITexture2DRef mTex;
	bool setupRenderResource(ERenderSystem systemName) override
	{
		mTex = RHIUtility::LoadTexture2DFromFile("Texture/UVChecker.png", TextureLoadOption().FlipV());

		GTextureShowManager.registerTexture("Tex", mTex);
		return true;
	}


	void preShutdownRenderSystem(bool bReInit = false) override
	{

	}


	void configRenderSystem(ERenderSystem systenName, RenderSystemConfigs& systemConfigs) override
	{
		systemConfigs.screenWidth = 1280;
		systemConfigs.screenHeight = 720;
	}

protected:
};

REGISTER_STAGE_ENTRY("Split Screen", SplitScreenTestStage, EExecGroup::GraphicsTest);