#pragma once
#ifndef AppleSnakeStage_H_098D9222_7656_40B4_A0AA_F570D11BEF88
#define AppleSnakeStage_H_098D9222_7656_40B4_A0AA_F570D11BEF88

#include "Stage/TestStageHeader.h"

#include "AppleSnakeLevel.h"

#include "GameRenderSetup.h"

#include "RHI/RHICommand.h"
#include "RHI/DrawUtility.h"
#include "RHI/GpuProfiler.h"
#include "RHI/RHIGraphics2D.h"

#include "Core/IntegerType.h"
#include "MarcoCommon.h"

#include "Async/Coroutines.h"
#include "Tween.h"
#include "Misc/DebugDraw.h"
#include "DebugDraw.h"



namespace AppleSnake
{
	struct LevelData;
#define LV_LOAD LvPortalTest

	using namespace Render;

	class GameStage : public StageBase
		            , public IGameRenderSetup
		            , public WorldData
	{
		using BaseClass = StageBase;
	public:

		bool loadMap(LevelData const& levelData);


		bool onInit() override
		{
			if (!BaseClass::onInit())
				return false;

			::Global::GUI().cleanupWidget();

			auto frame = WidgetUtility::CreateDevFrame();

			frame->addButton("Restart", [this](int event, GWidget*) -> bool
			{
				restart();
				return false;
			});
			frame->addCheckBox("Pause", bPaused);

			restart();

			return true;
		}

		void restart();


		bool bPaused = false;
		void onUpdate(long time) override
		{
			if (bRestartRequest)
			{
				bRestartRequest = false;
				restart();
			}

			if (bPaused)
				return;

			float dt = time / 1000.0f;
			mTweener.update(dt);
		}

		struct Sprite
		{
			Vector2 offset = Vector2::Zero();
			int color = EColor::Null;
		};

		RenderTransform2D mWorldToScreen;
		RenderTransform2D mScreenToWorld;
		Tween::GroupTweener<float> mTweener;

		float mMoveOffset = 0;
		float mFallOffset = 0;
		float mBlockFallOffset = 0;
		int   mMoveEntityIndex = INDEX_NONE;
		int   mIndexDraw = INDEX_NONE;
		int   mCPIndex = INDEX_NONE;
		int   mCPSnakeBodyIndex = INDEX_NONE;

		void onRender(float dFrame) override
		{
			Vec2i screenSize = ::Global::GetScreenSize();

			DrawDebugClear();

			RHICommandList& commandList = RHICommandList::GetImmediateList();
			RHISetFrameBuffer(commandList, nullptr);
			RHIClearRenderTargets(commandList, EClearBits::Color, &LinearColor(0.2, 0.2, 0.2, 1), 1);

			RHIGraphics2D& g = ::Global::GetRHIGraphics2D();
			g.beginRender();
			g.pushXForm();
			g.transformXForm(mWorldToScreen, false);


			bool bDrawDir = true;

			if (mSnake.getLength())
			{
				auto CanDraw = [&](int index) -> bool
				{
					return mIndexDraw == INDEX_NONE || index > mIndexDraw;
				};
				auto GetPos = [&](Snake::Body const& body) -> Vector2
				{
					return Vector2(body.pos) + mMoveOffset * Vector2(GetDirOffset(body.dir));
				};


				g.pushXForm();
				g.translateXForm(0, mFallOffset);

				RenderUtility::SetPen(g, EColor::Null);

				auto DrawBodyDir = [&](Snake::Body const& body)
				{
					if (!bDrawDir)
						return;

					RenderUtility::SetPen(g, EColor::Red);
					Vector2 p1 = Vector2(body.pos) + Vector2(0.5, 0.5) + mMoveOffset * Vector2(GetDirOffset(body.dir));
					g.drawLine(p1, p1 + 0.5f * Vector2(GetDirOffset(body.dir)));
					RenderUtility::SetPen(g, EColor::Null);
				};

				if (mSnake.getLength() <= 1)
				{
					RenderUtility::SetBrush(g, EColor::Yellow);
					auto const& head = mSnake.getHead();
					g.drawRect(head.pos, Vector2(1, 1));
					DrawBodyDir(head);
				}
				else
				{

					bool bLight = true;
					auto GetLight = [&]()
					{
						bLight = !bLight;
						return bLight ? COLOR_NORMAL : COLOR_DEEP;
					};
					if (mMoveOffset == 0)
					{
						RenderUtility::SetBrush(g, EColor::Yellow);
						auto const& head = mSnake.getHead();
						if (CanDraw(0))
							g.drawRect(head.pos, Vector2(1, 1));
						DrawBodyDir(head);

						for (int i = 1; i < mSnake.getLength() - 1; ++i)
						{
							RenderUtility::SetBrush(g, EColor::Yellow, GetLight());
							auto const& body = mSnake.getBody(i);
							if (CanDraw(i))
								g.drawRect(body.pos, Vector2(1, 1));
							DrawBodyDir(body);
						}


						RenderUtility::SetBrush(g, EColor::Yellow, GetLight());
						auto const& tail = mSnake.getTail();
						if (CanDraw(mSnake.getLength() - 1))
							g.drawRect(tail.pos, Vector2(1, 1));
						DrawBodyDir(tail);
					}
					else
					{
						Vector2 vertices[5];
						int numVertices = 4;
						int rotateDir = 0;

						auto DrawBody = [&](int index)
						{
							auto const& body = mSnake.getBody(index);
							auto const& bodyNext = mSnake.getBody(index + 1);

							Vector2 pos = GetPos(body);
							numVertices = 4;

							if (body.dir == bodyNext.dir || mMoveOffset == -1.0)
							{
								rotateDir = 0;
								Vec2i const* edgeBack = GetBlockEdge(InverseDir(body.dir));
								vertices[2] = pos + edgeBack[1];
								vertices[3] = pos + edgeBack[0];
								if (index == 1)
								{
									DrawDebugPoint(vertices[0], Color3f(1, 0, 1), 0.1);
									DrawDebugPoint(vertices[1], Color3f(0, 1, 1), 0.1);
									DrawDebugPoint(vertices[2], Color3f(1, 0, 0), 0.1);
									DrawDebugPoint(vertices[3], Color3f(0, 1, 0), 0.1);
								}
								if (CanDraw(index))
									g.drawTriangleStrip(vertices, numVertices);
							}
							else
							{
								Vector2 cornerIn = Vector2(body.pos) + Vector2(GetBlockCorner(InverseDir(body.dir), InverseDir(bodyNext.dir)));
								Vector2 cornerOut = Vector2(body.pos) + Math::Max(-1.0f, 2 * mMoveOffset) * Vector2(GetDirOffset(body.dir)) + Vector2(GetBlockCorner(InverseDir(body.dir), bodyNext.dir));

								if (index == 1)
								{
									DrawDebugPoint(vertices[0], Color3f(1, 0, 1), 0.1);
									DrawDebugPoint(vertices[1], Color3f(0, 1, 1), 0.1);
									DrawDebugPoint(cornerIn, Color3f(1, 0, 0), 0.1);
									DrawDebugPoint(cornerOut, Color3f(0, 1, 0), 0.1);
								}

								if ((body.dir + 1) % 4 == bodyNext.dir)
								{
									rotateDir = 1;
									vertices[2] = cornerIn;
									vertices[3] = cornerOut;
								}
								else
								{
									rotateDir = -1;
									vertices[2] = cornerOut;
									vertices[3] = cornerIn;
								}

								if (mMoveOffset < -0.5f)
								{
									vertices[4] = cornerOut + (2 * mMoveOffset + 1) * Vector2(GetDirOffset(bodyNext.dir));
									numVertices = 5;
								}
								else
								{
									vertices[4] = Vector2(body.pos) - 1.0f * Vector2(GetDirOffset(body.dir)) + Vector2(GetBlockCorner(InverseDir(body.dir), bodyNext.dir));
								}

								if (index == 1)
									DrawDebugPoint(vertices[4], Color3f(0, 0, 1), 0.1);

								if (CanDraw(index))
									g.drawTriangleStrip(vertices, numVertices);
							}

							DrawBodyDir(body);
						};

						auto const& head = mSnake.getHead();
						//g.drawRect(GetPos(head), Vector2(1, 1));
						Vec2i const* edge = GetBlockEdge(head.dir);
						Vector2 pos = GetPos(head);
						vertices[0] = pos + edge[0];
						vertices[1] = pos + edge[1];

						RenderUtility::SetBrush(g, EColor::Yellow);
						DrawBody(0);

						auto SwapVertices = [&](int index)
						{
							if (rotateDir != 0)
							{
								if (mMoveOffset > -0.5)
								{
									if (CanDraw(index))
										g.drawTriangleList(vertices + 2, 3);
								}
								if (rotateDir < 0)
								{
									vertices[0] = vertices[4];
									vertices[1] = vertices[3];
								}
								else
								{
									vertices[0] = vertices[2];
									vertices[1] = vertices[4];
								}
							}
							else
							{
								vertices[0] = vertices[2];
								vertices[1] = vertices[3];
							}
						};
						for (int i = 1; i < mSnake.getLength() - 1; ++i)
						{
							RenderUtility::SetBrush(g, EColor::Yellow, GetLight());
							SwapVertices(i);
							DrawBody(i);
						}

						auto const& tail = mSnake.getTail();

						if (CanDraw(mSnake.getLength() - 1))
						{
							RenderUtility::SetBrush(g, EColor::Yellow, GetLight());
							if (bGrowing)
							{
								g.drawRect(tail.pos, Vector2(1, 1));
							}
							else
							{
								SwapVertices(mSnake.getLength() - 1);
								Vector2 pos = GetPos(tail);
								Vec2i const* edgeBack = GetBlockEdge(InverseDir(tail.dir));
								vertices[2] = pos + edgeBack[1];
								vertices[3] = pos + edgeBack[0];
								g.drawTriangleStrip(vertices, numVertices);
							}
						}
						DrawBodyDir(tail);
					}
				}
				g.popXForm();
			}

			int indexFall = 0;
			for (int index = 0; index < mEntities.size(); ++index)
			{
				auto const& entity = mEntities[index];

				uint8 tileId = entity.id;
				Vector2 rPos = Vector2(entity.pos);

				if (mBlockFallOffset > 0 && mFallEntityIndices[indexFall] == index)
				{
					rPos.y += mBlockFallOffset;
					++indexFall;
				}
				if (mMoveOffset < 0 && mMoveEntityIndex == index)
				{
					rPos += mMoveOffset * Vector2(GetDirOffset(mActionDir));
				}

				switch (tileId)
				{
				case ETile::Rock:
					{
						RenderUtility::SetPen(g, EColor::Black);
						RenderUtility::SetBrush(g, EColor::Gray);
						g.drawRoundRect(rPos, Vector2(1, 1), Vector2(0.7, 0.7));
					}
					break;
				}
			}


			for (int j = 0; j < mMap.getSizeY(); ++j)
			{
				for (int i = 0; i < mMap.getSizeX(); ++i)
				{
					auto const& tile = mMap.getData(i, j);
					uint8 tileId = tile.id;
					Vector2 rPos = Vector2(i, j);

					switch (tileId)
					{
					case ETile::Block:
						{
							RenderUtility::SetPen(g, EColor::Black);
							RenderUtility::SetBrush(g, EColor::Brown);
							g.drawRoundRect(rPos, Vector2(1, 1), Vector2(0.5, 0.5));
						}
						break;
					case ETile::Apple:
						{
							RenderUtility::SetPen(g, EColor::Black);
							RenderUtility::SetBrush(g, EColor::Red);
							g.drawCircle(rPos + Vector2(0.5, 0.5), 0.5);
						}
						break;
					case ETile::Goal:
						{
							RenderUtility::SetPen(g, EColor::Black);
							RenderUtility::SetBrush(g, EColor::Black);
							g.drawCircle(rPos + Vector2(0.5, 0.5), 0.5);
						}
						break;
					case ETile::Trap:
						{
							RenderUtility::SetPen(g, EColor::Black);
							RenderUtility::SetBrush(g, EColor::Magenta);
							g.drawRoundRect(rPos, Vector2(0.7, 0.7), Vector2(0.5, 0.5));
						}
						break;
					case ETile::Portal:
						{
							auto& portal = mPortals[tile.meta];

							g.pushXForm();
							g.translateXForm(rPos.x + 0.5, rPos.y + 0.5);
							g.rotateXForm(Math::DegToRad(90 * portal.dir));

							g.beginBlend(0.5);
							RenderUtility::SetPen(g, EColor::Null);

							int PortalColors[] = { EColor::Blue, EColor::Red, EColor::Green };
							RenderUtility::SetBrush(g, PortalColors[portal.color], COLOR_LIGHT);
							float width = 0.2;
							g.drawRect(Vector2(0.5 - width, -0.5), Vector2(width, 1));
							g.endBlend();

							g.popXForm();
						}
					default:
						break;
					}
				}
			}

			DrawDebugCommit(Global::GetIGraphics2D());

			g.popXForm();
			g.endRender();

		}

		Coroutines::ExecutionHandle mGameHandle;
		bool bRestartRequest = false;



		struct MoveInfo
		{
			Vec2i pos;
			uint8 tileId;
			DirType dir;
			int crossPortalIndex = INDEX_NONE;
			int pushIndex = INDEX_NONE;
		};

		bool canMove(DirType dir, MoveInfo& moveInfo)
		{
			uint8 headTileId = getTile(mSnake.getHead().pos);

			moveInfo.crossPortalIndex = checkPortalMove(mSnake.getHead().pos, dir, moveInfo.pos, moveInfo.dir);
			if (moveInfo.crossPortalIndex == INDEX_NONE)
			{
				moveInfo.pos = mSnake.getHead().pos + GetDirOffset(dir);
				moveInfo.dir = dir;
			}
			moveInfo.tileId = getThing(moveInfo.pos);

			if (ETile::CanPush(moveInfo.tileId))
			{
				Vec2i testPos;
				DirType testDir;
				if (checkPortalMove(moveInfo.pos, moveInfo.dir, testPos, testDir) == INDEX_NONE)
				{
					testPos = moveInfo.pos + GetDirOffset(moveInfo.dir);
				}
				uint8 testTileId = getThing(testPos);
				if (testTileId != ETile::None && testTileId != ETile::Portal && testTileId != ETile::Trap)
					return false;

				moveInfo.pushIndex = getEntityIndex(moveInfo.pos);
			}
			else if (!ETile::CanMoveTo(moveInfo.tileId))
				return false;

			if (dir == InverseDir(GravityDir))
			{
				auto bodyIter = mSnake.createIterator();
				Vec2i curPos = mSnake.getHead().pos;
				bodyIter.goNext();
				bool canMoveUp = false;
				for (; bodyIter.haveMore(); bodyIter.goNext())
				{
					Vec2i const& pos = bodyIter.getElement().pos;
					if ((pos - curPos) != GetDirOffset(GravityDir))
					{
						canMoveUp = true;
						break;
					}
					curPos = pos;
				}

				if (!canMoveUp)
					return false;
			}
			return true;
		}


		void doMove(DirType dir, MoveInfo& moveInfo)
		{
			if (moveInfo.tileId == ETile::Apple)
			{
				mMap(moveInfo.pos) = ETile::None;
				mSnake.growBody();
			}
			else if (ETile::CanPush(moveInfo.tileId))
			{
				Vec2i moveToPos;
				DirType testDir;
				if (checkPortalMove(moveInfo.pos, moveInfo.dir, moveToPos, testDir) == INDEX_NONE)
				{
					moveToPos = moveInfo.pos + GetDirOffset(moveInfo.dir);
				}

				int index = getEntityIndex(moveInfo.pos);
				Entity& entity = mEntities[index];
				entity.pos = moveToPos;
			}

			mSnake.moveStep(moveInfo.pos, moveInfo.dir);

			if (moveInfo.crossPortalIndex != INDEX_NONE)
			{
				mCPIndex = moveInfo.crossPortalIndex;
				mCPSnakeBodyIndex = 0;
			}
			else if (mCPIndex != INDEX_NONE)
			{
				++mCPSnakeBodyIndex;
				if (mCPSnakeBodyIndex == mSnake.getLength() - 1)
					mCPIndex = INDEX_NONE;
			}
		}

		bool bMoving = false;
		bool bGrowing = false;
		TArray<int> mFallEntityIndices;
		DirType mActionDir = INDEX_NONE;

		void moveAction(DirType dir)
		{
			if (mActionDir != INDEX_NONE)
				return;

			auto Func = [this, dir]()
			{
				TGuardValue<int> scopedValue(mActionDir, dir);
				MoveInfo moveInfo;
				if (canMove(dir, moveInfo))
				{
					doMove(dir, moveInfo);

					mMoveEntityIndex = moveInfo.pushIndex;
					bGrowing = moveInfo.tileId == ETile::Apple;
					mTweener.tweenValue<Easing::Linear>(mMoveOffset, -1.0f, 0.0f, 0.3f).finishCallback(
						[this]()
					{
						mMoveOffset = 0.0f;
						Coroutines::Resume(mGameHandle);
					}
					);
					Coroutines::YeildReturn(nullptr);
					bGrowing = false;
				}
				else
				{
					if (dir == InverseDir(GravityDir) && ETile::CanMoveTo(moveInfo.tileId))
					{
						float jumpOffset = -0.1;
						mTweener.sequence().tweenValue<Easing::Linear>(mFallOffset, 0.0f, jumpOffset, 0.1f).finishCallback(
							[this]()
							{
								Coroutines::Resume(mGameHandle);
							}
						);
						Coroutines::YeildReturn(nullptr);
						mTweener.sequence().tweenValue<Easing::Linear>(mFallOffset, jumpOffset, 0.0f, 0.1f).finishCallback(
							[this]()
							{
								mFallOffset = 0.0f;
								Coroutines::Resume(mGameHandle);
							}
						);
						Coroutines::YeildReturn(nullptr);
					}
					return;
				}

				switch (moveInfo.tileId)
				{
				case ETile::Goal:
					{
						int numStep = mSnake.getLength();
						mIndexDraw = INDEX_NONE;
						for (int i = 0; i < numStep; ++i)
						{
							mSnake.moveStep(dir);
							mTweener.tweenValue<Easing::Linear>(mMoveOffset, -1.0f, 0.0f, 0.2f).finishCallback(
								[this]()
								{
									mMoveOffset = 0.0f;
									Coroutines::Resume(mGameHandle);
								}
							);
							mIndexDraw = i;
							Coroutines::YeildReturn(nullptr);
						}
						return;
					}
					break;
				case ETile::Trap:
					{
						bRestartRequest = true;
						return;
					}
					break;
				}


				for (;;)
				{
					bool bSnakeFall = checkSnakeFall();
					mFallEntityIndices.clear();
					for (auto& entity : mEntities)
					{
						uint8 tileId = entity.id;
						if (!ETile::CanFall(tileId))
							continue;

						bool bBlockFall = false;
						Vec2i blockLandPos = entity.pos + GetDirOffset(GravityDir);
						if (mMap.checkRange(blockLandPos))
						{
							uint8 landId = getThing(blockLandPos);
							if (landId == ETile::SnakeBody)
							{
								bBlockFall = bSnakeFall;
							}
							else if (landId == ETile::None)
							{
								bBlockFall = true;
							}
						}
						else
						{
							bBlockFall = true;
						}
						if (bBlockFall)
						{
							mFallEntityIndices.push_back(&entity - mEntities.data());
						}
					}

					if (bSnakeFall == false && mFallEntityIndices.empty())
						break;

					if (bSnakeFall)
					{
						mTweener.tweenValue<Easing::Linear>(mFallOffset, 0.0f, 1.0f, 0.3f).finishCallback(
							[this]()
							{
								mFallOffset = 0.0f;
								Coroutines::Resume(mGameHandle);
							}
						);
					}
					if (!mFallEntityIndices.empty())
					{
						mTweener.tweenValue<Easing::Linear>(mBlockFallOffset, 0.0f, 1.0f, 0.3f).finishCallback(
							[this]()
							{
								mBlockFallOffset = 0.0f;
								Coroutines::Resume(mGameHandle);
							}
						);
					}

					if (!mFallEntityIndices.empty())
					{
						Coroutines::YeildReturn(nullptr);
						for (auto index : mFallEntityIndices)
						{
							Entity& entity = mEntities[index];
							entity.pos += GetDirOffset(GravityDir);

							int yKill = mMap.getSizeY() + 5;
							if (entity.pos.y > yKill)
							{
								entity.id = ETile::None;
							}
						}
					}

					if (bSnakeFall)
					{
						Coroutines::YeildReturn(nullptr);
						mSnake.applyOffset(GetDirOffset(GravityDir));

						auto bodyIter = mSnake.createIterator();
						for (; bodyIter.haveMore(); bodyIter.goNext())
						{
							auto const& body = bodyIter.getElement();
							uint8 tileId = getTile(body.pos);
							if (tileId == ETile::Trap)
							{
								bRestartRequest = true;
								return;
							}
						}
					}

					int yHead = mSnake.getHead().pos.y;
					int yKill = mMap.getSizeY() + mSnake.getLength();
					if (yHead > yKill)
					{
						bRestartRequest = true;
						break;
					}
				}
			};

			mGameHandle = Coroutines::Start(Func);
		}

		MsgReply onKey(KeyMsg const& msg) override
		{
			if (msg.isDown())
			{
				switch (msg.getCode())
				{
				case EKeyCode::D: moveAction(0); break;
				case EKeyCode::A: moveAction(2); break;
				case EKeyCode::S: moveAction(GravityDir); break;
				case EKeyCode::W: moveAction(InverseDir(GravityDir)); break;
				default:
					break;
				}
			}

			return BaseClass::onKey(msg);

		}

		MsgReply onMouse(MouseMsg const& msg) override
		{
			if (msg.onLeftDown())
			{
				Vector2 worldPos = mScreenToWorld.transformPosition(msg.getPos());
				Vec2i blockPos;
				blockPos.x = Math::FloorToInt(Math::ToTileValue(worldPos.x, 1.0f));
				blockPos.y = Math::FloorToInt(Math::ToTileValue(worldPos.y, 1.0f));

			}
			else if (msg.onRightDown())
			{
			}
			return BaseClass::onMouse(msg);
		}
	};

}

#endif // AppleSnakeStage_H_098D9222_7656_40B4_A0AA_F570D11BEF88