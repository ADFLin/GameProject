#include "RichScene.h"

#include "RichPlayer.h"
#include "RichArea.h"

#include "RenderUtility.h"
#include "GameGlobal.h"
#include "InlineString.h"
#include "RHI/RHIGraphics2D.h"
#include "RenderDebug.h"
#include "BitUtility.h"


namespace Rich
{
	Vec2i const MapPos( 20 , 20 );
	

	int gRoleColor[] = { EColor::Red , EColor::Yellow , EColor::Green, EColor::Blue, EColor::Pink };

	int    const TileVisualLength = 50;
	Vec2i  const TileVisualSize(TileVisualLength, TileVisualLength);

	struct TileParams
	{
		TileParams()
		{

		}

		Vec2i size    = Vec2i(134, 68);
		Vec2i border  = Vec2i(2  , 0);
	};


	TileParams GTileParams;
	
	/// 
	///   --------> sx  
	///  |   offset/ cy	
	///	 |        /	
	/// \|/       \   
	/// sy         \
	//               cx
	float const transToScanPosFactor[4][4] =
	{
		//  sx         sy
		//cx  cy     cx  cy
		{ 0.5, 0.5, 0.5,-0.5 },
		{ 0.5,-0.5,-0.5,-0.5 },
		{-0.5,-0.5,-0.5, 0.5 },
		{-0.5, 0.5, 0.5, 0.5 },
	};

	static const int transToMapPosFactor[4][4] =
	{
		//  cx      cy
		//sx  sy  sx  sy
		{ 1 ,1 , 1 , -1 },
		{ 1 ,-1 ,-1 ,-1 },
		{-1 ,-1 ,-1 , 1 },
		{-1 , 1 , 1 , 1 },
	};


	Scene::Scene()
	{
		mLevel = nullptr;

		mbAlwaySkipMoveAnim = false;
		mbSkipMoveAnim = false;
	}

	struct TileFileDesc
	{
		char const* fileName;
		Vec2i center;
		Vec2i border;
		char const* maskfileName = nullptr;
	};

	TileFileDesc const GImageDesc[] =
	{
		{"Grass.png"  , Vec2i(69, 35) , Vec2i(5,16) },
		{"Road-01.png", Vec2i(69, 35) , Vec2i(5,15) },
		{"Road-02.png", Vec2i(69, 35) , Vec2i(5,15) },
		{"Road-03.png", Vec2i(69, 35) , Vec2i(5,16) },
		{"Road-04.png", Vec2i(69, 35) , Vec2i(5,15) },
		{"Road-05.png", Vec2i(69, 35) , Vec2i(5,16) },
		{"Road-06.png", Vec2i(69, 35) , Vec2i(5,16) },
		{"Road-07.png", Vec2i(69, 35) , Vec2i(5,16) },
		{"Road-08.png", Vec2i(69, 35) , Vec2i(5,15) },
		{"Road-09.png", Vec2i(69, 35) , Vec2i(5,16) },
		{"Road-10.png", Vec2i(69, 35) , Vec2i(5,16) },
		{"Road-11.png", Vec2i(69, 35) , Vec2i(5,16) },
		{"Road-12.png", Vec2i(69, 35) , Vec2i(5,16) },
		{"Road-13.png", Vec2i(69, 35) , Vec2i(5,16) },
		{"Road-14.png", Vec2i(69, 35) , Vec2i(5,16) },
		{"Road-15.png", Vec2i(69, 35) , Vec2i(5,16) },

		{"House-01.png", Vec2i(68, 73) , Vec2i(5,40) , "House-01_M.png"},
		{"Station.png" , Vec2i(70, 92) , Vec2i(5,65) },
	};
	
	struct ActorFileDesc
	{
		char const* fileName;
		Vec2i center;
		float scale;
	};

	ActorFileDesc const GActorImageDesc [] =
	{
		{"Player-01.png", Vec2i(91, 275), 0.3 },
		{"Player-02.png", Vec2i(91, 275), 0.3 },
		{"Player-03.png", Vec2i(91, 275), 0.3 },
		{"Player-04.png", Vec2i(91, 275), 0.3 },
		{"Player-05.png", Vec2i(91, 275), 0.3 },

		{"Chance.png" , Vec2i(473/2, 515) , 0.5 },
	};



	Vec2i GetAreaLocation(Vec2i const& tilePos)
	{
		if (tilePos.x == 0)
		{
			return Vec2i(-1, tilePos.y);
		}
		else if (tilePos.y == 0)
		{
			return Vec2i(tilePos.x, -1);
		}
		else if (tilePos.x == 10)
		{
			return Vec2i(11, tilePos.y);
		}
		else if (tilePos.y == 10)
		{
			return Vec2i(tilePos.x, 11);
		}
		return Vec2i(-1, -1);
	}

	bool Scene::initializeRHI()
	{
		using namespace Render;


		VERIFY_RETURN_FALSE(mTextureAtlas.initialize(ETexture::RGBA8, 2048, 2048, 2));

		auto LoadImage = [&](char const* name) -> int
		{
			InlineString<> path;
			path.format("Rich/%s", name);
			int id = mTextureAtlas.addImageFile(path);
			if (id != INDEX_NONE)
			{
				Vector2 uvMin, uvMax;
				mTextureAtlas.getRectUV(id, uvMin, uvMax);
				ImageInfo info;
				info.uvPos = uvMin;
				info.uvSize = uvMax - uvMin;
				IntVector2 size;
				mTextureAtlas.getRectSize(id, size);
				info.size = Vector2(size);
				CHECK(mImageInfos.size() == id);
				mImageInfos.push_back(info);
			}
			return id;
		};

		mTileImages.resize(ARRAY_SIZE(GImageDesc));
		for (int i = 0; i < ARRAY_SIZE(GImageDesc); ++i)
		{
			auto& tile = mTileImages[i];
			auto const& desc = GImageDesc[i];
			tile.id = LoadImage(desc.fileName);
			VERIFY_RETURN_FALSE(tile.id != INDEX_NONE);

			Vec2i imageSize;
			mTextureAtlas.getRectSize(tile.id, imageSize);
			Vector2 scale = Vector2(imageSize - desc.border).div(Vector2(GTileParams.size));
			tile.size = Vector2(imageSize).mul(scale);
			tile.center = Vector2(desc.center).div(Vector2(imageSize));

			if (desc.maskfileName)
			{
				tile.maskId = LoadImage(desc.maskfileName);
				VERIFY_RETURN_FALSE(tile.maskId != INDEX_NONE);
				
			}
		}

		mActorImages.resize(ARRAY_SIZE(GActorImageDesc));
		for (int i = 0; i < ARRAY_SIZE(GActorImageDesc); ++i)
		{
			InlineString<> path;
			auto& actor = mActorImages[i];
			auto const& desc = GActorImageDesc[i];

			actor.id = LoadImage(desc.fileName);
			VERIFY_RETURN_FALSE(actor.id != INDEX_NONE);

			Vec2i imageSize;
			mTextureAtlas.getRectSize(actor.id, imageSize);
			actor.size = ( desc.scale * float( GTileParams.size.x ) / imageSize.x ) * Vector2(imageSize);
			actor.center = Vector2(desc.center).div(Vector2(imageSize));
		}

		{
			mNumberId = LoadImage("BigNum.png");
		}

		GTextureShowManager.registerTexture("Atlas", &mTextureAtlas.getTexture());
		return true;
	}

	void Scene::releaseRHI()
	{
		using namespace Render;
		mTextureAtlas.finalize();
		GTextureShowManager.releaseRHI();
	}


	namespace ETile
	{
		enum Type
		{
			Road ,
			Null ,
			House,
			Station,
			ActionCard,
		};
	}
	uint32 MakeTileId(ETile::Type type, uint16 value)
	{
		return (value << 4 ) | type;
	}
	void Scene::initiliazeTiles()
	{
		World& world = mLevel->getWorld();
		for (auto iter = world.createTileIterator(); iter.haveMore(); iter.advance())
		{
			MapCoord mapPos = iter.getPos();

			uint linkBits = 0;
			LinkHandle links[MAX_DIR_NUM];
			int numLinks = world.getLinks(mapPos, links);
			for (int i = 0; i < numLinks; ++i)
			{
				Vec2i linkPos = world.getLinkCoord(mapPos, links[i]);
				Vec2i linkDir = linkPos - mapPos;
				int dir;
				if (linkDir.y == 0)
				{
					if (linkDir.x > 0)
					{
						dir = 0;
					}
					else
					{
						dir = 2;
					}
				}
				else
				{
					if (linkDir.y > 0)
					{
						dir = 1;
					}
					else
					{
						dir = 3;
					}
				}
				linkBits |= BIT(dir);
			}

			mTileIdMap[mapPos] = TileId::Make(ETile::Road , linkBits);
		}

		{


			for (auto iter = getLevel().getWorld().createAreaIterator(); iter.haveMore(); iter.advance())
			{
				updateAreaTile(iter.getArea());
			}
		}


	}

	void Scene::render(IGraphics2D& gInterface, RenderParams const& renderParams)
	{
		RHIGraphics2D& g = gInterface.getImpl<RHIGraphics2D>();
		g.beginRender();


		RenderUtility::SetPen( g , EColor::Gray );
		RenderUtility::SetBrush( g , EColor::Gray );
		g.drawRect( Vec2i(0,0) , ::Global::GetScreenSize() );
		
		render(gInterface, renderParams.view ? *renderParams.view : mView);

		if ( renderParams.bDrawDebug )
		{
			renderDebug(gInterface);
		}

		if (0)
		{

			int index = 0;
			for (auto player : mLevel->getPlayerList())
			{
				InlineString<> str;
				g.setTextColor(RenderUtility::GetColor(gRoleColor[player->mRoleId]));
				str.format(TEXT("%d %d"), index, player->getTotalMoney());
				g.drawText(Vec2i(10, 10 + 20 * index), str.c_str());
				++index;
			}
		}
		
		{
			g.setTexture(mTextureAtlas.getTexture());
			g.beginBlend(1.0f);
			int index = 0;
			for (auto player : mLevel->getPlayerList())
			{
				g.setBrush(RenderUtility::GetColor(gRoleColor[player->mRoleId]));
				drawNumber(gInterface, Vec2i(10, 10 + 30 * index), 16, player->getTotalMoney());
				++index;
			}
			g.endBlend();
		}

		g.endRender();
	}


	static Vector2 PiovtOffset(Vector2 const& pivot, Vector2 size)
	{
		return pivot.mul(size);
	}


	int GRoadIdMap[] = { -1, -1, -1, };


	void Scene::render(IGraphics2D& gInterface, RenderView& view)
	{
		RHIGraphics2D& g = gInterface.getImpl<RHIGraphics2D>();


		auto DrawTitleImage = [&](int id, Vector2 const& titlePos)
		{
			if (id == INDEX_NONE)
				return;

			auto const& tileInfo = mTileImages[id];
			auto const& imageInfo = mImageInfos[tileInfo.id];

			Vector2 size = view.zoom * tileInfo.size;
			Vector2 pos = titlePos - size.mul(tileInfo.center);
			g.drawTexture(mTextureAtlas.getTexture(), pos, size, imageInfo.uvPos, imageInfo.uvSize);
		};

		auto DrawTitleImageWithMask = [&](int id, Vector2 const& titlePos, Color3ub const& color)
		{
			if (id == INDEX_NONE)
				return;

			auto const& tileInfo = mTileImages[id];
			auto const& imageInfo = mImageInfos[tileInfo.id];

			Vector2 size = view.zoom * tileInfo.size;
			Vector2 pos = titlePos - size.mul(tileInfo.center);
			g.drawTexture(pos, size, imageInfo.uvPos, imageInfo.uvSize);

			auto const& maskImageInfo = mImageInfos[tileInfo.maskId];
			g.setBrush(color);
			g.drawTexture(pos, size, maskImageInfo.uvPos, maskImageInfo.uvSize);
			RenderUtility::SetBrush(g, EColor::White);
		};


		auto DrawActorImage = [&](int id, Vector2 const& titlePos)
		{
			if (id == INDEX_NONE)
				return;

			auto const& actorInfo = mActorImages[id];
			auto const& imageInfo = mImageInfos[actorInfo.id];
			Vector2 size = view.zoom * actorInfo.size;
			Vector2 pos = titlePos - size.mul(actorInfo.center);
			g.drawTexture(pos, size, imageInfo.uvPos, imageInfo.uvSize);
		};


		int xScanStart = Math::FloorToInt(Math::ToTileValue(view.screenOffset.x, view.scanTileSize.x)) - 1;
		int yScanStart = Math::FloorToInt(Math::ToTileValue(view.screenOffset.y, view.scanTileSize.y)) - 1;
		int xScanEnd = Math::CeilToInt(Math::ToTileValue(view.screenOffset.x + view.screenSize.x, view.scanTileSize.x));
		int yScanEnd = Math::CeilToInt(Math::ToTileValue(view.screenOffset.y + view.screenSize.y, view.scanTileSize.y));

		Vector2 const offset = 0.5 * view.scanTileSize;

		RenderCoord coord;

		RenderUtility::SetBrush(g, EColor::Null);


		g.beginBlend(1.0, ESimpleBlendMode::Translucent);
		RenderUtility::SetBrush(g, EColor::White);
		g.setTexture(mTextureAtlas.getTexture());

		auto DrawTileRect = [&](Vector2 const& screenPos, Vector2 const& halfSize)
		{
			Vector2 pos[4] =
			{
				screenPos + Vector2(-halfSize.x , 0),
				screenPos + Vector2(0, halfSize.y),
				screenPos + Vector2(halfSize.x , 0),
				screenPos + Vector2(0, -halfSize.y),
			};
			g.drawPolygon(pos, ARRAY_SIZE(pos));
		};

		auto DrawScanArea = [&](Vector2 const& scanPos)
		{
			view.calcCoordFromScan(coord, scanPos);
			Vec2i const& mapPos = coord.map;

			int id = INDEX_NONE;
			auto iter = mTileIdMap.find(mapPos);
			if (iter != mTileIdMap.end())
			{
				TileId const& tileId = iter->second;
				switch (tileId.type)
				{
				case ETile::Road:
					id = FBitUtility::RotateLeft(tileId.params[0], view.dir, 4);
					break;
				case ETile::House:
					{
						int areaId = tileId.params[0];
						int color = tileId.params[1];
						int level = tileId.params[2];
						if (level > 0)
						{
							DrawTitleImageWithMask(16, coord.screen, RenderUtility::GetColor(color));
						}
						else
						{
							DrawTitleImage(0, coord.screen);
							RenderUtility::SetBrush(g, color);
							DrawTileRect(coord.screen, 0.5 * view.scanTileSize);
							RenderUtility::SetBrush(g, EColor::White);
						}
					}
					break;
				case ETile::Station:
					{
						DrawTitleImage(17, coord.screen);
					}
					break;
				case ETile::ActionCard:
					{
						int group = tileId.params[0];
						DrawTitleImage(0, coord.screen);
						DrawActorImage(5, coord.screen);
					}
					break;
				default:
					id = 0;
				}
			}
			else
			{
				if (-2 <= mapPos.x && mapPos.x <= 12 &&
					-2 <= mapPos.y && mapPos.y <= 12)
				{
					id = 0;
				}
			}

			DrawTitleImage(id, coord.screen);
		};


		auto DrawActor = [&](ActorComp& actor, Vector2 const& screenPos)
		{
			int id = static_cast<Player&>(actor).getRoleId();
			DrawActorImage(id, screenPos);
		};

		struct ActorRenderData
		{
			ActorComp* actor;
			Vector2   screenPos;
		};
		ActorRenderData lastRender;
		lastRender.actor = nullptr;

		auto DrawScanAreaActors = [&](Vector2 const& scanPos, TArray<ActorRenderData>& pendingRender)
		{
			view.calcCoordFromScan(coord, scanPos);
			Tile* tile = getLevel().getWorld().getTile(coord.map);
			if (tile)
			{
				for (ActorComp& actor : tile->actors)
				{
					Vector2 pos = actor.getOwner()->getComponentT<ActorRenderComp>(COMP_RENDER)->pos;
					Vector2 actorScanPos = view.convMapToScanPos(pos);
					Vector2 actorScreenPos = view.convScanToScreenPos(actorScanPos);

					if (actorScanPos.y > scanPos.y)
					{
						pendingRender.push_back({ &actor, actorScreenPos });
					}
					else
					{
						DrawActor(actor, actorScreenPos);
					}
				}
			}
		};

		TArray<ActorRenderData> pendingRender[2];
		auto DrawScaneLine = [&](int yScan, Vector2 offset, int index)
		{
			for (int xScan = xScanStart; xScan <= xScanEnd; ++xScan)
			{
				DrawScanArea(Vector2(xScan, yScan) + offset);
			}
			for (auto const& data : pendingRender[index])
			{
				DrawActor(*data.actor, data.screenPos);
			}
			pendingRender[1 - index].clear();
			for (int xScan = xScanStart; xScan <= xScanEnd; ++xScan)
			{
				DrawScanAreaActors(Vector2(xScan, yScan) + offset, pendingRender[1 - index]);
			}
			if (lastRender.actor)
			{
				DrawActor(*lastRender.actor, lastRender.screenPos);
			}
		};

		for( int yScan = yScanStart ; yScan <= yScanEnd ; ++yScan )
		{
			DrawScaneLine(yScan, Vector2::Zero(), 0);
			DrawScaneLine(yScan, Vector2(0.5, 0.5), 1);
		}

		g.endBlend();

#if 0
		for (int yScan = yScanStart; yScan <= yScanEnd; ++yScan)
		{
			RenderUtility::SetPen(g, EColor::Green);

			for (int xScan = xScanStart; xScan <= xScanEnd; ++xScan)
			{
				view.calcCoordFromScan(coord, Vector2(xScan, yScan));
				//g.drawRect(coord.screen - PiovtOffset(view.scanTileSize, Vector2(0.5, 0.5)), view.scanTileSize);

				InlineString<> text;
				text.format("(%d,%d)", coord.map.x, coord.map.y);
				g.drawText(coord.screen, text);

			}

			RenderUtility::SetPen(g, EColor::Orange);

			for (int xScan = xScanStart - 1; xScan <= xScanEnd; ++xScan)
			{
				view.calcCoordFromScan(coord, Vector2(xScan + 0.5, yScan + 0.5f));
				//g.drawRect(coord.screen - PiovtOffset(view.scanTileSize, Vector2(0.5, 0.5)), view.scanTileSize);

				InlineString<> text;
				text.format("(%d,%d)", coord.map.x, coord.map.y);
				g.drawText(coord.screen, text);
			}
		}
#endif
	}

	void Scene::renderDebug(IGraphics2D& g)
	{
		World& world = getLevel().getWorld();
		World::MapDataType& map = world.mMapData;


		class AreaRenderer : public AreaVisitor
		{
		public:
			AreaRenderer(IGraphics2D& g) :g(g) {}

			virtual void visit(LandArea& area)
			{
				Player* owner = area.getOwner();
				if (owner)
				{
					RenderUtility::SetBrush(g, gRoleColor[owner->getRoleId()], COLOR_LIGHT);
					g.drawRect(rPos, TileVisualSize);
				}
				else
				{
					RenderUtility::SetBrush(g, EColor::Gray);
					g.drawRect(rPos, TileVisualSize);
				}
				LandArea::Info const& info = area.getInfo();
				InlineString<> str;
				str.format("%s %d", info.name.c_str(), area.mLevel);
				g.drawText(rPos, str.c_str());
			}

			virtual void visit(StationArea& area)
			{

			}

			virtual void visit(CardArea& area)
			{

			}

			virtual void visit(EmptyArea& area)
			{

			}

			virtual void visit(StartArea& area)
			{
				RenderUtility::SetBrush(g, EColor::Blue, COLOR_LIGHT);
				g.drawRect(rPos, TileVisualSize);
				g.drawText(rPos, TileVisualSize, "Start");
			}

			IGraphics2D& g;
			Vec2i       rPos;

		};

		AreaRenderer drawer(g);
		for (int i = 0; i < map.getSizeX(); ++i)
		{
			for (int j = 0; j < map.getSizeY(); ++j)
			{
				Area* area = world.getArea(MapCoord(i, j));
				if (area)
				{
					RenderUtility::SetPen(g, EColor::Black);
					RenderUtility::SetBrush(g, EColor::White);
					Vec2i rPos = MapPos + TileVisualLength * Vec2i(i, j);
					g.drawRect(rPos, TileVisualSize);
					drawer.rPos = rPos;
					area->accept(drawer);
				}
			}
		}

		for (RenderComp* comp : mRenderList)
		{
			comp->renderDebug(g);
		}

		for (auto const& group : mPlayingAnims)
		{
			group.cur->renderDebug(g);
		}
	}

	void Scene::update(long time)
	{
		for (int index = 0; index < mPlayingAnims.size(); ++index)
		{
			AnimGroup& group = mPlayingAnims[index];
			if (!upateAnim(group, time))
			{
				mPlayingAnims.removeIndex(index);
				--index;
			}
		}

		for(RenderComp* comp : mRenderList)
		{
			comp->update( time );
		}
	}

	class DiceAnimation : public Animation
	{
	public:
		DiceAnimation()
		{
			curTime = 0;
		}
		virtual bool update( long time )
		{
			curTime += time;
			if ( curTime > duration )
				return false;
			return true;
		}

		virtual void renderDebug(IGraphics2D& g )
		{
			for( int i = 0 ; i < numDice ; ++i )
			{
				drawDice( g , Vec2i( 150 + i * 100 , 150 ) , value[i]);
			}
		}

		virtual void render(IGraphics2D& g)
		{
			for (int i = 0; i < numDice; ++i)
			{
				drawDice(g, Vec2i(150 + i * 100, 150), value[i]);
			}
		}

		void drawDice(IGraphics2D& g , Vec2i const& pos , int value )
		{
			RenderUtility::SetPen( g , EColor::Black );
			RenderUtility::SetBrush( g , EColor::White );
			RenderUtility::SetFont( g , FONT_S24 );

			g.drawRoundRect( pos , Vec2i( 80 , 80 ) , Vec2i( 10 , 10 ) );
			g.setTextColor(Color3ub(0 , 0 , 0) );
			InlineString< 32 > str;
			if ( curTime < duration / 2 )
				str.format( "%d" , ::Global::Random() % 6 + 1 );
			else
				str.format( "%d" , value );
			g.drawText( pos , Vec2i( 80 , 80 ) , str );
		}
		long curTime;
		long duration;
		int  numDice;
		int  value[ MAX_MOVE_POWER ];

	};

	class ActorMoveAnimation : public Animation
	{
	public:
		ActorMoveAnimation( ActorComp& actor , long duration )
		{
			comp = actor.getOwner()->getComponentT< ActorRenderComp >( COMP_RENDER );
			comp->bUpdatePos = false;

			from = actor.getPrevPos();
			dif  = actor.getPos() - actor.getPrevPos();
			timeCur = 0;
			timeTotal = duration;
		}
		virtual bool update( long time )
		{
			timeCur += time;
			if ( timeCur > timeTotal )
			{
				comp->pos = from + dif;
				comp->bUpdatePos = true;
				return false;
			}
			
			comp->pos = from + dif * float( timeCur ) / timeTotal;
			return true;
		}
		virtual void skip()
		{

		}
		virtual void render(IGraphics2D& g )
		{


		}
		ActorRenderComp* comp;
		long      timeTotal;
		long      timeCur;
		Vector2     from;
		Vector2     dif;
	};

	class ActorMessageAnimation : public Animation
	{
	public:


		void setup(AnimSetupHelper& helper) override
		{
			mHelper = &helper;
			renderPos = mHelper->calcScreenPos(actor->getPos());
		}

		bool update(long time) override
		{
			if (duration < 0)
				return false;

			duration -= time;
			return true;
		}


		void renderDebug(IGraphics2D& g) override
		{
			RenderUtility::SetFont(g, FONT_S24);
			g.setTextColor(Color3ub(255, 0, 0));
			g.drawText(renderPos, msg.c_str());
		}

		Vec2i renderPos;
		AnimSetupHelper* mHelper;
		long  duration;
		String msg;
		ActorComp* actor;


	};


	void Scene::onWorldMsg( WorldMsg const& msg )
	{
		switch( msg.id )
		{
		case MSG_MOVE_START:
			mbSkipMoveAnim = mbAlwaySkipMoveAnim;
			break;
		case MSG_MOVE_STEP:
			{
				if (!mbSkipMoveAnim)
				{
					auto anim = new ActorMoveAnimation(*msg.getParam<Player*>(), 500);
					addAnim(anim);
					CO_YEILD(WaitAnimation(anim));
				}
			}
			break;
		case MSG_THROW_DICE:
			{
				auto anim = new DiceAnimation;
				anim->duration = 1000;
				anim->numDice = msg.getParam<int>(0);
				for( int i = 0 ; i < anim->numDice ; ++i )
					anim->value[i] = msg.getParam<int*>(1)[i];

				addAnim( anim );
				CO_YEILD(WaitAnimation(anim));
			}
			break;
		case MSG_PLAYER_MONEY_MODIFIED:
			{
				auto anim = new ActorMessageAnimation;
				anim->duration = 1000;
				anim->msg = InlineString<>::Make( "%s%d" , msg.getParam<int>(1) > 0 ? "+" : "-" , Math::Abs(msg.getParam<int>(1)) );
				anim->actor = msg.getParam<Player*>();
				addAnim( anim );
			}
			break;
		case MSG_BUY_LAND:
		case MSG_UPGRADE_LAND:
			{
				LandArea* area = msg.getParam<LandArea*>(0);
				updateAreaTile(area);
			}
			break;
		case MSG_DISPOSE_ASSET:
			{
				PlayerAsset** assets = msg.getParam<PlayerAsset**>(0);
				int numAssets = msg.getParam<int>(1);
				for( int i = 0; i < numAssets; ++i )
				{
					Area* area = assets[i]->getAssetArea();
					if (area)
					{
						updateAreaTile(area);
					}				
				}
			}
			break;
		}
		
	}

	void Scene::addAnim( Animation* anim , bool bNewGroup )
	{
		if (bNewGroup || mPlayingAnims.empty())
		{
			AnimGroup group;
			group.cur = anim;
			group.last = anim;
			anim->setup(*this);
			mPlayingAnims.push_back(group);
		}
		else
		{
			AnimGroup& group = mPlayingAnims.back();
			group.last->mNext = anim;
			group.last = anim;
		}
	}

	void Scene::drawNumber(IGraphics2D& gInterface, Vector2 const& pos, float width,  int number, Vector2 const& pivot)
	{
		RHIGraphics2D& g = gInterface.getImpl<RHIGraphics2D>();

		int digials[32];
		int numDigial = 0;
		do
		{
			digials[numDigial++] = number % 10;
			number /= 10;
		} while (number);

	
		auto const& imageInfo = mImageInfos[mNumberId];

		Vector2 digialSize = Vector2(width, 10 * width * imageInfo.size.y / imageInfo.size.x );
		Vector2 frameNum = Vector2(10, 1);
		Vector2 uvSize = imageInfo.uvSize.div(frameNum);
		Vector2 numberSize = Vector2(numDigial, 1).mul(digialSize);
		Vector2 LTPos = pos - pivot.mul(numberSize);

		for (int i = 0; i < numDigial; ++i)
		{
			Vector2 frame = Vector2(digials[numDigial - i - 1], 0);	
			Vector2 uvPos = imageInfo.uvPos + uvSize * frame;
			g.drawTexture(LTPos + Vector2(i * width, 0), digialSize, uvPos, uvSize);
		}
	}

	Vec2i Scene::calcScreenPos(MapCoord const& pos)
	{
		return pos * TileVisualLength + MapPos;
	}

	bool Scene::calcCoord(Vec2i const& sPos, MapCoord& coord)
	{
		coord = ( sPos - MapPos ) / TileVisualLength;
		return true;
	}

	bool Scene::upateAnim(AnimGroup& group, long time)
	{
		Animation* anim = group.cur;
		if (!anim->update(time))
		{
			if (anim->mHandler)
			{
				anim->mHandler->handleAnimFinish();
			}

			Animation* next = anim->mNext;
			delete anim;
			group.cur = next;

			if (group.cur)
			{
				group.cur->setup(*this);
			}
			else
			{
				group.last = nullptr;
				return false;
			}
		}
		return true;
	}

	void Scene::updateAreaTile(Area* area)
	{
		class TileAreaVisitor : public AreaVisitor
		{
		public:

			TileAreaVisitor(Scene& scene) :scene(scene) {}
			virtual void visit(LandArea& area)
			{
				int color = EColor::Gray;
				if (area.getOwner())
				{
					color = gRoleColor[area.getOwner()->getRoleId()];
				}
				int level = area.mLevel;
				TileId tileId = TileId::Make(ETile::House, area.getId(), color, level);
				Vec2i areaLocation = GetAreaLocation(area.getPos());
				scene.mTileIdMap[areaLocation] = tileId;
			}
			virtual void visit(StationArea& area)
			{
				TileId tileId = TileId::Make(ETile::Station, area.getId());
				Vec2i areaLocation = GetAreaLocation(area.getPos());
				scene.mTileIdMap[areaLocation] = tileId;
			}
			virtual void visit(CardArea& area)
			{
				TileId tileId = TileId::Make(ETile::ActionCard, area.mGroup);
				Vec2i areaLocation = GetAreaLocation(area.getPos());
				scene.mTileIdMap[areaLocation] = tileId;
			}
			virtual void visit(EmptyArea& area) {}
			virtual void visit(StartArea& area) {}
			virtual void visit(ComponyArea& area) {}
			virtual void visit(StoreArea& area) {}
			virtual void visit(JailArea& area) {}
			virtual void visit(ActionEventArea& area) {}

			Scene& scene;
		};
		static TileAreaVisitor StaticVisitor(*this);
		area->accept(StaticVisitor);
	}

	void ActorRenderComp::renderDebug(IGraphics2D& g)
	{
		RenderUtility::SetPen( g , EColor::Black );
		RenderUtility::SetBrush( g , EColor::Red );
		Vec2i rPos = MapPos + Vec2i( TileVisualLength * pos ) + TileVisualSize / 2;
		g.drawCircle( rPos  , 10 );
	}

	void ActorRenderComp::update( long time )
	{
		if ( !bUpdatePos )
			return;
		pos = getOwner()->getComponentT< ActorComp >(COMP_ACTOR)->getPos();
	}

	void PlayerRenderComp::renderDebug(IGraphics2D& g )
	{
		Player* player = getOwner()->getComponentT< Player >(COMP_ACTOR);
		RenderUtility::SetPen( g , EColor::Black );
		RenderUtility::SetBrush( g , gRoleColor[ player->getRoleId() ] );
		Vec2i rPos = MapPos + Vec2i( TileVisualLength * pos ) + TileVisualSize / 2;
		g.drawCircle( rPos  , 10 );
	}

	Vector2 RenderView::convScanToMapPos(Vector2 const& scanPos) const
	{
		int const (&factor)[4] = transToMapPosFactor[ dir ];
		return Vector2(scanPos.x * factor[0] + scanPos.y * factor[1] ,
			           scanPos.x * factor[2] + scanPos.y * factor[3] );
	}

	Vector2 RenderView::convScreenToMapPos( Vec2i const& screenPos ) const
	{
		Vector2 scanPos = convScreenToScanPos(screenPos);
		return convScanToMapPos(scanPos);
	}

	Vector2 RenderView::convMapToScreenPos( Vec2i const& mapPos ) const
	{
		Vector2 scanPos = convMapToScanPos(mapPos);
		return convScanToScreenPos(scanPos);
	}

	Vector2 RenderView::convMapToScreenPos( Vector2 const& mapPos) const
	{
		float const (&factor)[4] = transToScanPosFactor[ dir ];

		Vector2 scanPos;
		scanPos.x = float(mapPos.x * factor[0] + mapPos.y * factor[1]);
		scanPos.y = float(mapPos.x * factor[2] + mapPos.y * factor[3]);

		return convScanToScreenPos(scanPos);
	}

	Vector2 RenderView::convMapToScanPos(Vector2 const& mapPos) const
	{
		float const (&factor)[4] = transToScanPosFactor[dir];
		Vector2 result;
		result.x = float(mapPos.x * factor[0] + mapPos.y * factor[1]);
		result.y = float(mapPos.x * factor[2] + mapPos.y * factor[3]);
		return result;
	}

	Vector2 RenderView::convScanToScreenPos(Vector2 const& scanPos) const
	{
		return scanTileSize.mul(scanPos + Vector2(0.5, 0.5)) - screenOffset;
	}

	Vector2 RenderView::convScreenToScanPos(Vector2 const& screenPos) const
	{
		return  scanTileSize.div(screenPos + screenOffset) - Vector2(0.5, 0.5);
	}

	Vector2 RenderView::calcMapToScreenOffset( float dx , float dy )
	{
		float const (&factor)[4] = transToScanPosFactor[ dir ];
		Vector2 scanOffset;
		scanOffset.x = dx * factor[0] + dy * factor[1];
		scanOffset.y = dx * factor[2] + dy * factor[3];
		return calcScanToScreenOffset(scanOffset);
	}

	Vector2 RenderView::calcScanToScreenOffset(Vector2 const& scanOffset)
	{
		return scanOffset.mul(scanTileSize);
	}

	void RenderView::update()
	{
		scanTileSize = zoom * Vector2(GTileParams.size + GTileParams.border);
		titleSize = zoom * GTileParams.size;
	}

	void RenderView::calcCoordFromMap(RenderCoord& coord, Vec2i const& mapPos)
	{
		coord.map    = mapPos;
		coord.scan = convMapToScanPos( mapPos );
		coord.screen = convScanToScreenPos(coord.scan);
	}

	void RenderView::calcCoordFromScan( RenderCoord& coord , Vector2 const& scanPos )
	{
		coord.scan   = scanPos;
		coord.map    = convScanToMapPos(scanPos);
		coord.screen = convScanToScreenPos(scanPos);
	}

}//namespace Rich