#include "AppleSnakeStage.h"

#include "ApplsSnakeLevelData.h"
#define LV_LOAD LvPortalTest

namespace AppleSnake
{
	REGISTER_STAGE_ENTRY("Apple Snake", LevelStage, EExecGroup::Dev);

	struct StencilFillOp
	{
		StencilFillOp(RHIGraphics2D& g, uint32 refValue)
			:g(g), refValue(refValue)
		{

		}

		bool bShowMask = false;

		template< typename TFunc >
		void drawStencil(TFunc func)
		{
			RHIClearRenderTargets(g.getCommandList(), EClearBits::Stencil, nullptr, 0, 0, 0);
			g.resetRenderState();
			g.setCustomRenderState([this](RHICommandList& commandList, RenderBatchedElement& element)
			{
				RHISetBlendState(commandList, bShowMask ? TStaticBlendState< CWM_RGBA >::GetRHI() : TStaticBlendState< CWM_None >::GetRHI());
				RHISetDepthStencilState(commandList, TStaticDepthStencilState<
					false, ECompareFunc::Always, true, ECompareFunc::Always,
					EStencil::Replace, EStencil::Replace, EStencil::Replace
				>::GetRHI(), refValue);
			});

			func(g);
		}

		template< bool bIncluded >
		void setupState()
		{
			g.setCustomRenderState([this](RHICommandList& commandList, RenderBatchedElement& element, GraphicsDefinition::RenderState const& state)
			{
				RHISetBlendState(commandList, TStaticBlendState<>::GetRHI());
				RHISetDepthStencilState(commandList, TStaticDepthStencilState<
					true, ECompareFunc::Always, true, bIncluded ? ECompareFunc::Equal : ECompareFunc::NotEqual,
					EStencil::Keep, EStencil::Keep, EStencil::Keep
				>::GetRHI(), refValue);
				BatchedRender::SetupShaderState(commandList, g.getBaseTransform(), state);
			});
		}

		uint32 refValue;
		RHIGraphics2D& g;
	};

	static RenderTransform2D CalcPortalTransform(World::Portal const& portal)
	{
		RenderTransform2D result = RenderTransform2D::TranslateThenRotate(Vector2(0.5, 0), Math::DegToRad(90 * portal.dir));
		result.translateWorld(Vector2(portal.pos) + Vector2(0.5, 0.5));
		return result;
	}

	static bool ShouldDraw(uint8 tileId, int layerOrder)
	{
		constexpr uint32 ForegroundBits = MakeBitFlags(ETile::Cobweb, ETile::Trap, ETile::Portal);
		if (ForegroundBits & BIT(tileId))
			return layerOrder > 0;
		return layerOrder == 0;
	}

	bool LevelStage::loadMap(LevelData const& levelData)
	{
		int border = levelData.border;

		mMap.resize(levelData.size.x + 2 * border, levelData.size.y + 2 * border);
		mPortals.clear();
		mEntities.clear();
		mDeadSnakes.clear();

		Tile tile;
		tile.id = ETile::None;
		tile.meta = 0;
		mMap.fillValue(tile);

		int indexMeta = 0;
		auto ReadMeta = [&]()
		{
			if (indexMeta < levelData.meta.size())
			{
				auto meta = levelData.meta[indexMeta];
				++indexMeta;
				return meta;
			}
			return uint8(0);
		};

		auto ToDataIndex = [&](Vec2i const& pos)
		{
			return (pos.x - border) + levelData.size.x * (pos.y - border);
		};

		auto CheckDataRange = [&](Vec2i const& pos)
		{
			Vec2i dataPos = pos - Vec2i(border, border);
			return 0 <= dataPos.x && dataPos.x < levelData.size.x &&
				0 <= dataPos.y && dataPos.y < levelData.size.y &&
				ToDataIndex(pos) < levelData.data.size();
		};

		for (int i = 0; i < levelData.data.size(); ++i)
		{
			uint8 data = levelData.data[i];
			if (data == 0)
				continue;

			Vec2i pos = Vec2i(i % levelData.size.x, i / levelData.size.x) + Vec2i(border, border);

			if (ETile::IsEntity(data))
			{
				Entity e;
				e.id = data;
				e.pos = pos;
				mEntities.push_back(e);
				continue;
			}

			uint8 dataId   = data & (BIT(5) - 1);
			uint8 dataMeta = data >> 5;

			switch (dataId)
			{
			case ETile::Portal:
				{
					auto& tile = mMap(pos);
					tile.id = dataId;
					tile.meta = mPortals.size();

					auto meta = ReadMeta();
					Portal portal;
					portal.color = meta >> 4;
					portal.pos = pos;
					portal.dir = meta & 0xf;
					portal.link = INDEX_NONE;
					for (auto& other : mPortals)
					{
						if (other.color == portal.color)
						{
							portal.link = &other - mPortals.data();
							other.link = mPortals.size();
							break;
						}
					}
					mPortals.push_back(portal);
				}
				break;
			case ETile::SnakeHead:
				{
					Vec2i curPos = pos;
					int prevDir = INDEX_NONE;
					for (;;)
					{
						int dir = 0;
						for (; dir < 4; ++dir)
						{
							if (prevDir == dir)
								continue;

							Vec2i pos = curPos + GetDirOffset(dir);
							if (!CheckDataRange(pos))
								continue;

							if (levelData.data[ToDataIndex(pos)] == ETile::SnakeBody)
							{
								int dirInv = InverseDir(dir);
								if (prevDir == INDEX_NONE)
								{
									mSnake.reset(curPos, dirInv, 1);
								}
								mSnake.addTial(dirInv);
								prevDir = dirInv;
								curPos = pos;
								break;
							}
						}

						if (dir == 4)
							break;
					}

					for (int i = mSnake.getLength() - 2; i > 0; --i)
					{
						auto& body = mSnake.getBody(i);
						auto const& bodyNext = mSnake.getBody(i + 1);

						Vec2i offset = body.pos - bodyNext.pos;
						if (offset.x == 1)
						{
							body.moveDir = 0;
						}
						else if (offset.y == 1)
						{
							body.moveDir = 1;
						}
						else if (offset.x == -1)
						{
							body.moveDir = 2;
						}
						else //if (offset.y == -1)
						{
							body.moveDir = 3;
						}
					}
				}
				break;
			case ETile::SnakeBody:
				break;
			case ETile::Apple:
				{
					auto& tile = mMap(pos);
					tile.id = dataId;
					tile.meta = dataMeta;
				}
				break;
			default:
				mMap(pos) = dataId;
			}
		}

		return true;
	}

	void LevelStage::restart()
	{
		resetParams();
	
		mTweener.clear();
		Coroutines::Stop(mGameHandle);
		loadMap(LV_LOAD);

		Vec2i screenSize = ::Global::GetScreenSize();
		mWorldToScreen = RenderTransform2D::LookAt(screenSize, 0.5 * Vector2(mMap.getSize()), Vector2(0, -1), screenSize.y / float(10 + 2.5), false);
		mScreenToWorld = mWorldToScreen.inverse();
	}

	void LevelStage::drawTile(RHIGraphics2D& g, Tile const& tile, Vector2 const& rPos)
	{
		switch (tile.id)
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
				int type = tile.meta;

				int TypeColors[] = { EColor::Red, EColor::Magenta, EColor::Gold, EColor::Cyan, EColor::Purple };

				RenderUtility::SetPen(g, EColor::Black);
				RenderUtility::SetBrush(g, TypeColors[type]);
				g.drawCircle(rPos + Vector2(0.5, 0.5), 0.45);
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
			break;
		case ETile::Cobweb:
			{
				g.pushXForm();
				g.translateXForm(rPos.x + 0.5, rPos.y + 0.5);
				g.beginBlend(0.9);
				RenderUtility::SetPen(g, EColor::Null);
				RenderUtility::SetBrush(g, EColor::White);
				g.drawCircle(Vector2(0, 0), 0.4);
				g.endBlend();
				g.popXForm();
			}
			break;
		default:
			break;
		}
	}

	struct SnakeDraw
	{
		static constexpr float gap = 0.05;
		static constexpr float bodyLen = 1 - 2 * gap;
		static void DrawLinkBody(RHIGraphics2D& g, Vector2 const& basePos, DirType dir)
		{
			static constexpr Vector2 OffsetMap[] =
			{ 
				Vector2(bodyLen, 0), Vector2(0, bodyLen), Vector2(-2 * gap, 0),Vector2(0, -2 * gap)
			};
			static constexpr Vector2 SizeMap[] =
			{
				Vector2(2 * gap, bodyLen),Vector2(bodyLen, 2 * gap),Vector2(2 * gap, bodyLen),Vector2(bodyLen, 2 * gap)
			};
			g.drawRect(basePos + OffsetMap[dir], SizeMap[dir]);
		};

	};

	void LevelStage::drawDeadSnake(RHIGraphics2D& g, Snake const& snake, Vector2 const& rPos)
	{
		auto DrawBodyDir = [&](Snake::Body const& body)
		{
			RenderUtility::SetPen(g, EColor::Red);
			Vector2 p1 = Vector2(body.pos) + Vector2(0.5, 0.5) + mMoveOffset * Vector2(GetDirOffset(body.moveDir));
			g.drawLine(p1, p1 + 0.5f * Vector2(GetDirOffset(body.moveDir)));
			RenderUtility::SetPen(g, EColor::Null);
		};

		constexpr float gap = 0.05;
		constexpr float bodyLen = 1 - 2 * gap;
		g.pushXForm();
		g.translateXForm(rPos.x, rPos.y);
		RenderUtility::SetBrush(g, EColor::Brown, COLOR_DEEP);
		auto const& head = snake.getHead();
		Vector2 basePos = Vector2(head.pos) + Vector2(gap, gap);
		g.drawRect(basePos, Vector2(bodyLen, bodyLen));
		DrawBodyDir(head);

		for (int i = 1; i < snake.getLength() - 1; ++i)
		{
			RenderUtility::SetBrush(g, EColor::Brown, COLOR_DEEP);
			auto const& body = snake.getBody(i);
			basePos = Vector2(body.pos) + Vector2(gap, gap);
			SnakeDraw::DrawLinkBody(g, basePos, body.linkDir);
			g.drawRect(basePos, Vector2(bodyLen, bodyLen));
			DrawBodyDir(body);
		}

		RenderUtility::SetBrush(g, EColor::Brown, COLOR_DEEP);
		auto const& tail = snake.getTail();
		basePos = Vector2(tail.pos) + Vector2(gap, gap);
		SnakeDraw::DrawLinkBody(g, basePos, tail.linkDir);
		g.drawRect(basePos, Vector2(bodyLen, bodyLen));
		g.popXForm();
	}

	void LevelStage::drawSnake(RHIGraphics2D& g)
	{
		if (mSnake.getLength() == 0)
			return;

		constexpr float gap = 0.05;
		constexpr float bodyLen = 1 - 2 * gap;

		bool bDrawDir = true;
		auto CanDraw = [&](int index) -> bool
		{
			return mIndexDraw == INDEX_NONE || index > mIndexDraw;
		};
		auto GetPos = [&](Snake::Body const& body) -> Vector2
		{
			return Vector2(body.pos) + mMoveOffset * Vector2(GetDirOffset(body.moveDir));
		};

		g.pushXForm();
		g.translateXForm(0, mFallOffset);

		RenderUtility::SetPen(g, EColor::Null);

		int indexDrawDebug = mCPSnakeBodyIndex + 1;

		auto DrawBodyDir = [&](Snake::Body const& body)
		{
			if (!bDrawDir)
				return;

			Vector2 p1 = Vector2(body.pos) + Vector2(0.5, 0.5) + mMoveOffset * Vector2(GetDirOffset(body.moveDir));

			RenderUtility::SetPen(g, EColor::Red);
			g.drawLine(p1, p1 + 0.5f * Vector2(GetDirOffset(body.moveDir)));
			RenderUtility::SetPen(g, EColor::Blue);
			g.drawLine(p1, p1 + 0.5f * Vector2(GetDirOffset(body.linkDir)));
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
				{
					Vector2 basePos = Vector2(head.pos) + Vector2(gap, gap);
					g.drawRect(basePos, Vector2(bodyLen, bodyLen));
				}
				DrawBodyDir(head);

				for (int i = 1; i < mSnake.getLength() - 1; ++i)
				{
					RenderUtility::SetBrush(g, EColor::Yellow, GetLight());
					auto const& body = mSnake.getBody(i);
					if (CanDraw(i))
					{
						Vector2 basePos = Vector2(body.pos) + Vector2(gap, gap);
						SnakeDraw::DrawLinkBody(g, basePos, body.linkDir);
						g.drawRect(basePos, Vector2(bodyLen, bodyLen));
					}
					DrawBodyDir(body);

				}

				RenderUtility::SetBrush(g, EColor::Yellow, GetLight());
				auto const& tail = mSnake.getTail();
				if (CanDraw(mSnake.getLength() - 1))
				{
					Vector2 basePos = Vector2(tail.pos) + Vector2(gap, gap);
					SnakeDraw::DrawLinkBody(g, basePos, tail.linkDir);
					g.drawRect(basePos, Vector2(bodyLen, bodyLen));
				}
				DrawBodyDir(tail);
			}
			else
			{
				Vector2 vertices[5];
				int numVertices = 4;
				int rotateDir = 0;

				auto CheckPortalDraw = [&](int index)
				{
					if (index == mCPSnakeBodyIndex)
					{
						auto const& portalFrom = mPortals[mCPIndex];
						auto const& portalTo = mPortals[portalFrom.link];
						auto xFormTo = CalcPortalTransform(portalTo);
						auto xFormFrom = CalcPortalTransform(portalFrom);
						RenderTransform2D xFormTeleportInv = xFormTo.inverse() * RenderTransform2D(Vector2(-1, -1)) * xFormFrom;

						auto DebugDraw = [&](Color3ub const& color)
						{
							for (int i = 0; i < numVertices; ++i)
							{
								DrawDebugPoint(vertices[i], color, 0.1);
								DrawDebugText(vertices[i], InlineString<>::Make("%d", i), color);
							}
						};

						auto DrawPortalMask = [&](Portal const& portal)
						{
							Vector2 pos = Vector2(portal.pos) + GetDirOffset(portal.dir);
							switch (portal.dir)
							{
							case 0: g.drawRect(pos, Vector2(1 + gap, 1)); break;
							case 1: g.drawRect(pos, Vector2(1, 1 + gap)); break;
							case 2: g.drawRect(pos - Vector2(gap, 0), Vector2(1 + gap, 1)); break;
							case 3: g.drawRect(pos - Vector2(0, gap), Vector2(1, 1 + gap)); break;
							}
						};
						StencilFillOp op(g, 0x1);
						if (CanDraw(index))
						{
							op.drawStencil([&](RHIGraphics2D& g)
							{
								DrawPortalMask(portalTo);
							});
							op.setupState<false>();
							g.drawTriangleStrip(vertices, numVertices);
						}
						DebugDraw(Color3f(0, 1, 0));

						for (int i = 0; i < ARRAY_SIZE(vertices); ++i)
						{
							vertices[i] = xFormTeleportInv.transformPosition(vertices[i]);
						}
						if (CanDraw(index))
						{
							op.drawStencil([&](RHIGraphics2D& g)
							{
								DrawPortalMask(portalFrom);
							});
							op.setupState<false>();
							g.drawTriangleStrip(vertices, numVertices);
						}
						DebugDraw(Color3f(0, 0, 1));

						g.resetRenderState();
						g.flush();
					}
					else
					{
						if (CanDraw(index))
						{
							g.drawTriangleStrip(vertices, numVertices);
						}
					}
				};

				auto GetBodyVertex = [&](Vector2 const& v)
				{
					return bodyLen * (v - Vector2(0.5, 0.5)) + Vector2(0.5, 0.5);
				};

				auto DrawBody = [&](int index)
				{
					auto const& body = mSnake.getBody(index);
					auto const& bodyNext = mSnake.getBody(index + 1);

					Vector2 pos = GetPos(body);
					numVertices = 4;
					int nextDir = bodyNext.moveDir;

					if (index == mCPSnakeBodyIndex)
					{
						auto const& portalFrom = mPortals[mCPIndex];
						auto const& portalTo = mPortals[portalFrom.link];

						int deltaDir = portalFrom.dir - InverseDir(portalTo.dir);
						nextDir -= deltaDir;
						if (nextDir >= 4)
							nextDir -= 4;
						else if (nextDir < 0)
							nextDir += 4;
					}

					if (body.moveDir == nextDir || mMoveOffset == -1.0)
					{
						rotateDir = 0;
						Vec2i const* edgeBack = GetBlockEdge(InverseDir(body.moveDir));
						vertices[2] = pos + GetBodyVertex(edgeBack[1]);
						vertices[3] = pos + GetBodyVertex(edgeBack[0]);

						if (index == indexDrawDebug)
						{
							DrawDebugPoint(vertices[0], Color3f(1, 0, 1), 0.1);
							DrawDebugPoint(vertices[1], Color3f(0, 1, 1), 0.1);
							DrawDebugPoint(vertices[2], Color3f(1, 0, 0), 0.1);
							DrawDebugPoint(vertices[3], Color3f(0, 1, 0), 0.1);
						}
					}
					else
					{

						Vector2 cornerIn = Vector2(body.pos) + GetBodyVertex(GetBlockCorner(InverseDir(body.moveDir), InverseDir(nextDir)));
						Vector2 cornerOut = Vector2(body.pos) + Math::Max(-1.0f, 2 * mMoveOffset) * Vector2(GetDirOffset(body.moveDir)) + GetBodyVertex(GetBlockCorner(InverseDir(body.moveDir), nextDir));

						if (index == indexDrawDebug)
						{
							DrawDebugPoint(vertices[0], Color3f(1, 0, 1), 0.1);
							DrawDebugPoint(vertices[1], Color3f(0, 1, 1), 0.1);
							DrawDebugPoint(cornerIn, Color3f(1, 0, 0), 0.1);
							DrawDebugPoint(cornerOut, Color3f(0, 1, 0), 0.1);
						}

						if ((body.moveDir + 1) % 4 == nextDir)
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
							vertices[4] = cornerOut + (2 * mMoveOffset + 1) * Vector2(GetDirOffset(nextDir));
							numVertices = 5;
						}
						else
						{
							vertices[4] = Vector2(body.pos) - 1.0f * Vector2(GetDirOffset(body.moveDir)) + GetBodyVertex(GetBlockCorner(InverseDir(body.moveDir), nextDir));
						}

						if (index == indexDrawDebug)
							DrawDebugPoint(vertices[4], Color3f(0, 0, 1), 0.1);
					}

					CheckPortalDraw(index);
					DrawBodyDir(body);
				};

				auto const& head = mSnake.getHead();
				//g.drawRect(GetPos(head), Vector2(1, 1));
				Vec2i const* edge = GetBlockEdge(head.moveDir);
				Vector2 pos = GetPos(head);
				vertices[0] = pos + GetBodyVertex(edge[0]);
				vertices[1] = pos + GetBodyVertex(edge[1]);

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

				int indexTail = mSnake.getLength() - 1;
				if (CanDraw(indexTail))
				{
					RenderUtility::SetBrush(g, EColor::Yellow, GetLight());
					if (bGrowing)
					{
						DirType linkDir = mSnake.getBody(indexTail - 1).moveDir;
						Vector2 basePos = Vector2(tail.pos) + Vector2(gap, gap);
						SnakeDraw::DrawLinkBody(g, basePos, linkDir);
						g.drawRect(basePos, Vector2(bodyLen, bodyLen));
					}
					else
					{
						SwapVertices(indexTail);
						Vector2 pos = GetPos(tail);
						Vec2i const* edgeBack = GetBlockEdge(InverseDir(tail.moveDir));
						vertices[2] = pos + GetBodyVertex(edgeBack[1]);
						vertices[3] = pos + GetBodyVertex(edgeBack[0]);
						CheckPortalDraw(indexTail);
					}
				}
				DrawBodyDir(tail);
			}
		}
		g.popXForm();
	}

	void LevelStage::onRender(float dFrame)
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

		auto DrawTiles = [&](int layerOrder)
		{
			for (int j = 0; j < mMap.getSizeY(); ++j)
			{
				for (int i = 0; i < mMap.getSizeX(); ++i)
				{
					auto const& tile = mMap.getData(i, j);
					if (!ShouldDraw(tile.id, layerOrder))
						continue;;

					drawTile(g, tile, Vector2(i, j));
				}
			}
		};

		DrawTiles(0);
		drawSnake(g);


		auto DrawEntity = [&](Entity const& entity, Vector2 const& rPos)
		{
			switch (entity.id)
			{
			case ETile::Rock:
				{
					RenderUtility::SetPen(g, EColor::Black);
					RenderUtility::SetBrush(g, EColor::Gray);
					g.drawRoundRect(rPos, Vector2(1, 1), Vector2(0.7, 0.7));
				}
				break;
			}
		};

		for (int index = 0; index < mEntities.size(); ++index)
		{
			auto const& entity = mEntities[index];

			uint8 tileId = entity.id;
			Vector2 rPos = Vector2(entity.pos);

			if (mBlockFallOffset > 0 && (entity.flags & EEntityFlag::Fall))
			{
				rPos.y += mBlockFallOffset;
			}

			bool bEntityMoving = mMoveOffset < 0 && mMoveEntityIndex == index;
			if (bEntityMoving)
			{
				rPos += mMoveOffset * Vector2(GetDirOffset(mActionDir));
			}

			if (tileId == ETile::SnakeDead)
			{
				auto& snake = mDeadSnakes[entity.meta];
				drawDeadSnake(g, snake, rPos);
			}
			else
			{
				if (bEntityMoving)
				{
					if (entity.flags & EEntityFlag::CrossPortal)
					{
						DrawEntity(entity, rPos);

						Portal& portalTo = getPortalChecked(entity.pos);
						Portal& portalFrom = mPortals[portalTo.link];

						rPos = Vector2(portalFrom.pos) + (1 + mMoveOffset) * Vector2(GetDirOffset(portalFrom.dir));
						DrawEntity(entity, rPos);
					}
					else
					{
						DrawEntity(entity, rPos);
					}
				}
				else
				{
					DrawEntity(entity, rPos);
				}
			}
		}

		DrawTiles(1);
#if 0
		for (auto const& portal : mPortals)
		{
			RenderTransform2D xForm = CalcPortalTransform(portal);
			DrawDebugPoint(xForm.transformPosition(Vector2(0.25, 0)), Color3f(1, 0, 0), 0.1);
			DrawDebugPoint(xForm.transformPosition(Vector2(-0.25, 0)), Color3f(0, 1, 0), 0.1);
		}
#endif

		g.setTextRemoveScale(true);
		DrawDebugCommit(Global::GetIGraphics2D());

		g.popXForm();


		g.drawTextF(Vector2(50, 50), "index = %d", mCPSnakeBodyIndex);

		g.endRender();
	}

	bool LevelStage::canMove(DirType dir, MoveInfo& moveInfo)
	{
		uint8 headTileId = getTile(mSnake.getHead().pos);

		moveInfo.crossPortalIndex = checkPortalMove(mSnake.getHead().pos, dir, moveInfo.pos, moveInfo.dir);
		if (moveInfo.crossPortalIndex == INDEX_NONE)
		{
			moveInfo.pos = mSnake.getHead().pos + GetDirOffset(dir);
			moveInfo.dir = dir;
		}

		int entityIndex = getEntityIndex(moveInfo.pos);

		if (entityIndex != INDEX_NONE)
		{
			Entity& entity = mEntities[entityIndex];
			moveInfo.tileId = entity.id;

			if (ETile::CanPush(moveInfo.tileId))
			{
				Vec2i testPos;
				if (moveInfo.tileId == ETile::SnakeDead)
				{
					auto& snake = mDeadSnakes[entity.meta];
					if (!checkMoveTo(entity, moveInfo.dir, false))
						return false;
				}
				else
				{
					DirType testDir;
					moveInfo.pushProtalIndex = checkPortalMove(moveInfo.pos, moveInfo.dir, testPos, testDir);
					if (moveInfo.pushProtalIndex == INDEX_NONE)
					{
						testPos = moveInfo.pos + GetDirOffset(moveInfo.dir);
					}

					uint8 testTileId = getThing(testPos);
					if (!ETile::CanEnityMoveTo(testTileId))
						return false;

				}
				moveInfo.pushIndex = entityIndex;
			}
			else if (!ETile::CanMoveTo(moveInfo.tileId))
				return false;
		}
		else
		{
			moveInfo.tileId = getThing(moveInfo.pos, false);
			if (!ETile::CanMoveTo(moveInfo.tileId))
				return false;
		}

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

	void LevelStage::doMove(DirType dir, MoveInfo& moveInfo)
	{
		if (moveInfo.tileId == ETile::Apple)
		{
			auto type = mMap(moveInfo.pos).meta;
			mMap(moveInfo.pos) = ETile::None;
			mSnake.growBody();
			switch (type)
			{
			case EApple::Poison:
				++mPoisonCount;
				break;
			}
		}
		else if (ETile::CanPush(moveInfo.tileId))
		{
			Entity& entity = mEntities[moveInfo.pushIndex];
			Vec2i moveToPos;
			DirType testDir;
			if (checkPortalMove(entity.pos, moveInfo.dir, moveToPos, testDir) == INDEX_NONE)
			{
				moveToPos = entity.pos + GetDirOffset(moveInfo.dir);
			}
			entity.pos = moveToPos;
		}

		mSnake.moveStep(moveInfo.pos, dir, moveInfo.dir);
	}

	void LevelStage::moveActionEntry(DirType dir)
	{
		MoveInfo moveInfo;
		if (canMove(dir, moveInfo))
		{
			if (mPoisonCount > 0)
			{
				int indexSplit = mSnake.getLength() / 2;
				if (mSnake.getLength() % 2)
					indexSplit += 1;

				DeadSnake deadSnake;
				mSnake.splitBack(indexSplit, deadSnake);
				addDeadSnake(std::move(deadSnake));
				--mPoisonCount;
			}

			doMove(dir, moveInfo);

			if (moveInfo.crossPortalIndex != INDEX_NONE)
			{
				mCPIndex = moveInfo.crossPortalIndex;
				mCPSnakeBodyIndex = 0;
			}
			else if (mCPIndex != INDEX_NONE)
			{
				++mCPSnakeBodyIndex;
			}

			mMoveEntityIndex = moveInfo.pushIndex;
			if (mMoveEntityIndex != INDEX_NONE)
			{
				auto& entity = mEntities[mMoveEntityIndex];
				if (moveInfo.pushProtalIndex != INDEX_NONE)
				{
					entity.flags |= EEntityFlag::CrossPortal;
				}
			}
			bGrowing = moveInfo.tileId == ETile::Apple;
			mTweener.tweenValue<Easing::OQuad>(mMoveOffset, -1.0f, 0.0f, 0.3f).finishCallback(
				[this]()
				{
					mMoveOffset = 0.0f;
					Coroutines::Resume(mGameHandle);
				}
			);
			CO_YEILD(nullptr);

			if (mCPIndex != INDEX_NONE && mCPSnakeBodyIndex == mSnake.getLength() - 1)
			{
				mCPIndex = INDEX_NONE;
				mCPSnakeBodyIndex = INDEX_NONE;
			}

			if (mMoveEntityIndex != INDEX_NONE)
			{
				auto& entity = mEntities[mMoveEntityIndex];
				if (moveInfo.pushProtalIndex != INDEX_NONE)
				{
					entity.flags &= ~EEntityFlag::CrossPortal;
				}
			}
			bGrowing = false;
		}
		else
		{
			if (dir == InverseDir(GravityDir) && ETile::CanMoveTo(moveInfo.tileId))
			{
				float jumpOffset = -0.1;
				mTweener.tweenValue<Easing::Linear>(mFallOffset, 0.0f, jumpOffset, 0.1f).finishCallback(
					[this]()
					{
						Coroutines::Resume(mGameHandle);
					}
				);
				CO_YEILD(nullptr);
				mTweener.tweenValue<Easing::Linear>(mFallOffset, jumpOffset, 0.0f, 0.1f).finishCallback(
					[this]()
					{
						mFallOffset = 0.0f;
						Coroutines::Resume(mGameHandle);
					}
				);
				CO_YEILD(nullptr);
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
					if (mCPIndex != INDEX_NONE)
					{
						++mCPSnakeBodyIndex;
					}

					mTweener.tweenValue<Easing::Linear>(mMoveOffset, -1.0f, 0.0f, 0.2f).finishCallback(
						[this]()
						{
							mMoveOffset = 0.0f;
							Coroutines::Resume(mGameHandle);
						}
					);

					mIndexDraw = i;
					CO_YEILD(nullptr);

					if (mCPIndex != INDEX_NONE && mCPSnakeBodyIndex == mSnake.getLength() - 1)
					{
						mCPIndex = INDEX_NONE;
						mCPSnakeBodyIndex = INDEX_NONE;
					}
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
			bool bSnakeFall = (mCPIndex == INDEX_NONE) && checkSnakeFall();

			TArray<int> fallEntityIndices;
			for (auto& entity : mEntities)
			{
				if (!ETile::CanFall(entity.id))
					continue;

				bool bEntityFall = checkMoveTo(entity, GravityDir, bSnakeFall);
				if (bEntityFall)
				{
					entity.flags |= EEntityFlag::Fall;
					fallEntityIndices.push_back(&entity - mEntities.data());
				}
			}

			if (bSnakeFall == false && fallEntityIndices.empty())
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
			if (!fallEntityIndices.empty())
			{
				mTweener.tweenValue<Easing::Linear>(mBlockFallOffset, 0.0f, 1.0f, 0.3f).finishCallback(
					[this]()
					{
						mBlockFallOffset = 0.0f;
						Coroutines::Resume(mGameHandle);
					}
				);
			}

			if (!fallEntityIndices.empty())
			{
				CO_YEILD(nullptr);
				for (auto index : fallEntityIndices)
				{
					Entity& entity = mEntities[index];
					entity.flags &= ~EEntityFlag::Fall;
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
				CO_YEILD(nullptr);
				mSnake.applyOffset(GetDirOffset(GravityDir));

				int yMinBody = MaxInt32;
				for (auto bodyIt = mSnake.createIterator(); bodyIt.haveMore(); bodyIt.goNext())
				{
					auto const& body = bodyIt.getElement();
					if (yMinBody > body.pos.y)
					{
						yMinBody = body.pos.y;
					}
					uint8 tileId = getTile(body.pos);
					if (tileId == ETile::Trap)
					{
						bRestartRequest = true;
						return;
					}
				}

				int yKill = mMap.getSizeY();
				if (yMinBody > yKill)
				{
					bRestartRequest = true;
					break;
				}
			}
		}
	}

}//namespace AppleSnake