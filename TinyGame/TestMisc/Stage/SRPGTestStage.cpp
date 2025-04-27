#include "Stage/TestStageHeader.h"
#include "Stage/TestRenderStageBase.h"

#include "GameRenderSetup.h"

#include "RHI/RHICommand.h"
#include "RHI/DrawUtility.h"
#include "RHI/RHIGraphics2D.h"

#include "Async/Coroutines.h"
#include "DataStructure/Grid2D.h"

namespace SRPG
{

	using namespace Render;

	struct AttributeSet
	{
		int health;
		int attack;
		int defense;
		int moveStep;

		void setDefault()
		{
			health = 100;
			attack = 20;
			defense = 0;
			moveStep = 4;
		}
	};


	enum ETeam
	{
		Player,
		Enemy,
	};


	class Actor
	{
	public:
		Vec2i pos;
		ETeam team;
		AttributeSet attributes;
	};

	namespace ETerrain 
	{
		enum Type : uint8
		{
			Plain,
			Block,
			Mountain,
			Forest,
		};
	}

#define P ETerrain::Plain
#define B ETerrain::Block
#define M ETerrain::Mountain

	uint8 TestMap[]
	{
		P,P,P,P,P,P,
		P,M,P,B,B,P,
		P,M,P,P,P,P,
		P,B,P,P,P,P,
		P,P,B,P,P,P,
		P,P,P,P,P,P,
	};

#undef P
#undef B


	class Game
	{
	public:
		~Game()
		{
			cleanup();
		}

		void setup()
		{
			cleanup();
			initMap(TestMap, 6, 6);
			addActor(Vec2i(1, 1), ETeam::Player);
			addActor(Vec2i(3, 3), ETeam::Enemy);
		}

		void addActor(Vec2i const& pos, ETeam team)
		{
			Actor* actor = new Actor;
			actor->attributes.setDefault();
			actor->pos = pos;
			actor->team = team;
			mActors.push_back(actor);

			auto& tile = mMap.getData(pos.x, pos.y);
			CHECK(tile.actor == nullptr);
			tile.actor = actor;
		}

		void moveActor(Actor& actor, Vec2i const& pos)
		{
			auto& tile = mMap.getData(actor.pos.x, actor.pos.y);
			CHECK(tile.actor == nullptr || tile.actor == &actor);
			tile.actor = nullptr;

			auto& tileMove = mMap.getData(pos.x, pos.y);
			CHECK(tileMove.actor == nullptr);
			actor.pos = pos;
			tileMove.actor = &actor;
		}

		void cleanup()
		{
			for (auto actor : mActors)
			{
				delete actor;
			}
			mActors.clear();
		}
		void run()
		{
			mHandleLogic = Coroutines::Start(
				[this]()
				{
					logicEntry();
				}
			);
		}

		class IInputAction
		{
		public:
			virtual void onEnter(Game& game){}
			virtual void onExit(Game& game){}
			virtual void onTileClick(Game& game, Vec2i const& tilePos){}
		};

		template< typename T> 
		class InputActionT : public IInputAction
		{
		public:
			static T& Get()
			{
				static T StaticInstance;
				return StaticInstance;
			}
		};


		class MoveAction : public  InputActionT<MoveAction>
		{
		public:
			virtual void onTileClick(Game& game, Vec2i const& tilePos) 
			{
				int index = game.mMoveArea.findIndexPred([&tilePos](auto const& move)
				{
					return move.pos == tilePos;
				});

				if (index == INDEX_NONE)
					return;

				Coroutines::Resume(game.mHandleLogic, index);
			}
		};

		class CastAction : public  InputActionT<CastAction>
		{

		public:
			virtual void onTileClick(Game& game, Vec2i const& tilePos)
			{
				int index = game.mCastTargetList.findIndexPred([&game, &tilePos](auto const& index)
				{
					return game.mCastArea[index] == tilePos;
				});

				if (index == INDEX_NONE)
					return;

				Coroutines::Resume(game.mHandleLogic, index);
			}

		};


		IInputAction* mAction = nullptr;

		class IActorAction
		{
		public:
			virtual void getInfo(Game& game, Actor& actor, TArray< Vec2i > outCastArea, TArray< int >&  outCastTargetList) = 0;
		};


		template< typename T>
		class ActorActionT : public IActorAction
		{
		public:
			static T& Get()
			{
				static T StaticInstance;
				return StaticInstance;
			}
		};

		struct ActionContext
		{
			TArray< Vec2i >& castArea; 
			TArray< int >&   castTargetList;
		};


		class MeleeAction : public ActorActionT<MeleeAction>
		{
		public:
			virtual void getInfo(Game& game, Actor& actor, TArray< Vec2i > outCastArea, TArray< int >&  outCastTargetList)
			{
				int const NumMeleeDir = 4;
				Vec2i const MeleeDirOffset[] =
				{
					Vec2i(1,0),Vec2i(0,1),Vec2i(-1,0),Vec2i(0,-1),
				};

				for (int dir = 0; dir < 4; ++dir)
				{
					Vec2i targetPos = actor.pos + MeleeDirOffset[dir];
					outCastArea.push_back(targetPos);
					if (game.canAttack(actor, targetPos))
					{
						outCastTargetList.push_back(dir);
					}
				}
			}
		};

		void changeAction(IInputAction& action)
		{
			if (mAction == &action)
				return;

			if (mAction)
			{
				mAction->onExit(*this);
			}
			mAction = &action;
			mAction->onEnter(*this);
		}


		void logicEntry()
		{
			while (1)
			{
				Actor* turnActor = mActors[0];
				mMoveArea.clear();
				getMoveableArea(*turnActor, turnActor->pos, mMoveArea);


				do
				{
					changeAction(MoveAction::Get());

					int indexMove = CO_YEILD<int>(nullptr);

					auto const& move = mMoveArea[indexMove];
					if (!canMoveTo(*turnActor, *move.tile))
						continue;

					if (turnActor->pos != move.pos)
					{
						moveActor(*turnActor, move.pos);
					}

					//int command = CO_YEILD<int>(nullptr);
					int command = 0;

					mCastArea.clear();
					mCastTargetList.clear();
					switch (command)
					{
					case 0:
						{
							int const NumMeleeDir = 4;
							Vec2i const MeleeDirOffset[] =
							{
								Vec2i(1,0),Vec2i(0,1),Vec2i(-1,0),Vec2i(0,-1),
							};

							for (int dir = 0; dir < 4; ++dir)
							{
								Vec2i targetPos = turnActor->pos + MeleeDirOffset[dir];
								mCastArea.push_back(targetPos);
								if (canAttack(*turnActor, targetPos))
								{
									mCastTargetList.push_back(dir);
								}
							}
						}
						break;
					default:
						break;
					}
					if (mCastTargetList.size())
					{
						int indexTarget = CO_YEILD<int>(nullptr);
						if (indexTarget != INDEX_NONE)
						{


						}
					}


				} 
				while (0);

			}
		}


		void handleTileClick(Vec2i const& tilePos)
		{
			if (mAction)
			{
				mAction->onTileClick(*this, tilePos);
			}
		}

		struct MapTile
		{
			uint8 value;
			Actor* actor;
		};
		struct MoveInfo
		{
			Vec2i    pos;
			MapTile* tile;
			int step;
			float remainStep;
		};
		bool bShowMoveArea = true;
		TArray<MoveInfo> mMoveArea;
		bool bShowCastArea = true;
		TArray< Vec2i > mCastArea;
		TArray< int >   mCastTargetList;

		bool canPass(Actor& actor, MapTile const& tile)
		{
			if (tile.value == ETerrain::Block)
				return false;

			if (tile.actor && tile.actor->team != actor.team)
				return false;

			return true;
		}

		bool canAttack(Actor& actor, Vec2i const& pos)
		{
			if (!mMap.checkRange(pos.x, pos.y))
				return false;

			auto const& tile = mMap(pos.x, pos.y);
			if (tile.actor == nullptr)
				return false;

			if (tile.actor->team == actor.team)
				return false;

			return true;
		}

		bool canMoveTo(Actor& actor, MapTile const& tile)
		{
			if (tile.actor && tile.actor != &actor)
				return false;

			return true;
		}

		void getMoveableArea(Actor& actor, Vec2i const& pos, TArray< MoveInfo >& outMoveList)
		{
			{
				MoveInfo move;
				move.step = 0;
				move.pos = pos;
				move.tile = &mMap(pos.x, pos.y);
				move.remainStep = actor.attributes.moveStep;
				outMoveList.push_back(move);
			}
			int const NumMoveDir = 4;
			Vec2i const MoveOffset[] =
			{
				Vec2i(1,0),Vec2i(0,1),Vec2i(-1,0),Vec2i(0,-1),
			};

			for (int index = 0; index < outMoveList.size(); ++index)
			{
				auto const& prevMove = outMoveList[index];
				auto const& tilePrev = mMap(prevMove.pos.x, prevMove.pos.y);

				for (int dir = 0; dir < NumMoveDir; ++dir)
				{
					Vec2i movePos = prevMove.pos + MoveOffset[dir];
					if ( !mMap.checkRange(movePos.x, movePos.y) )
						continue;

					auto& tileMove = mMap(movePos.x, movePos.y);
					if (!canPass(actor, tileMove))
						continue;

					float moveCost = 1.0;
					if (tileMove.value == ETerrain::Mountain)
					{
						moveCost = 2.5;
					}

					float remainStep = prevMove.remainStep - moveCost;
					if (remainStep < 0)
						continue;

					int indexFound = outMoveList.findIndexPred([&movePos](auto const& info)
					{
						return info.pos == movePos;
					});

					if (indexFound != INDEX_NONE)
					{
						auto& move = outMoveList[indexFound];
	
						if (move.remainStep > remainStep)
							continue;

						outMoveList.removeIndex(indexFound);
						if (indexFound < index)
						{
							--index;
						}
					}
					MoveInfo move;
					move.step = prevMove.step + 1;
					move.pos = movePos;
					move.tile = &tileMove;
					move.remainStep = remainStep;
					outMoveList.push_back(move);
				}
			}
		}


		void initMap(uint8 const* data ,int sizeX, int sizeY)
		{
			mMap.resize(sizeX, sizeY);
			for(int j = 0 ; j < sizeY; ++j)
			{
				for (int i = 0; i < sizeX; ++i)
				{
					auto& tile = mMap(i, j);
					tile.value = data[j * sizeX + i];
					tile.actor = nullptr;
				}
			}
		}


		TArray< Actor* > mActors;
		TGrid2D< MapTile > mMap;


		Coroutines::ExecutionHandle mHandleLogic;
	};



	class TestStage : public StageBase 
		            , public IGameRenderSetup
	{
		using BaseClass = StageBase;
	public:
		TestStage() {}

		bool onInit() override
		{
			if (!BaseClass::onInit())
				return false;
			::Global::GUI().cleanupWidget();
			restart();
			return true;
		}

		void onEnd() override
		{
			BaseClass::onEnd();
		}

		RenderTransform2D mWorldToScreen;
		RenderTransform2D mScreenToWorld;

		void restart() 
		{
			mGame.setup();

			Vec2i screenSize = ::Global::GetScreenSize();
			mWorldToScreen = RenderTransform2D::LookAt(screenSize, 0.5 * Vector2(mGame.mMap.getSize()), Vector2(0, -1), screenSize.y / float(10 + 2.5), false);

			mScreenToWorld = mWorldToScreen.inverse();

			mGame.run();
		}

		void onUpdate(GameTimeSpan deltaTime) override
		{
			BaseClass::onUpdate(deltaTime);
		}

		Game mGame;
		
		void onRender(float dFrame) override
		{
			RHIGraphics2D& g = Global::GetRHIGraphics2D();

			RHICommandList& commandList = RHICommandList::GetImmediateList();
			RHISetFrameBuffer(commandList, nullptr);
			RHIClearRenderTargets(commandList, EClearBits::Color, &LinearColor(0.2, 0.2, 0.2, 1), 1);


			g.beginRender();
			g.pushXForm();
			g.transformXForm(mWorldToScreen, false);

			for (int j = 0; j < mGame.mMap.getSizeY(); ++j)
			{
				for (int i = 0; i < mGame.mMap.getSizeY(); ++i)
				{
					auto const& tile = mGame.mMap(i, j);

					RenderUtility::SetPen(g, EColor::Black);
					switch (tile.value)
					{
					case ETerrain::Plain:
						{
							RenderUtility::SetBrush(g, EColor::Green, COLOR_LIGHT);
							g.drawRect(Vector2(i, j), Vector2(1, 1));
						}
						break;
					case ETerrain::Block:
						{
							RenderUtility::SetBrush(g, EColor::Gray);
							g.drawRect(Vector2(i, j), Vector2(1, 1));
						}
						break;
					case ETerrain::Mountain:
						{
							RenderUtility::SetBrush(g, EColor::Brown, COLOR_LIGHT);
							g.drawRect(Vector2(i, j), Vector2(1, 1));
						}
						break;
					}
				}
			}

			if (mGame.bShowMoveArea)
			{
				g.beginBlend(0.5f, ESimpleBlendMode::Translucent);
				RenderUtility::SetPen(g, EColor::Null);
				RenderUtility::SetBrush(g, EColor::White);
				for (auto const& move : mGame.mMoveArea)
				{
					g.drawRect(move.pos, Vector2(1, 1));
				}
				g.endBlend();
			}

			if (mGame.bShowCastArea)
			{
				g.beginBlend(0.5f, ESimpleBlendMode::Translucent);
				RenderUtility::SetPen(g, EColor::Null);
				RenderUtility::SetBrush(g, EColor::Red);
				for (auto const& pos : mGame.mCastArea)
				{
					g.drawRect(pos, Vector2(1, 1));
				}
				g.endBlend();
			}

			RenderUtility::SetBrush(g, EColor::White);
			RenderUtility::SetPen(g, EColor::Black);
			for (auto actor : mGame.mActors)
			{
				g.drawCircle(Vector2(actor->pos) + Vector2(0.5,0.5), 0.3);
			}

			g.popXForm();
			g.endRender();
		}

		MsgReply onMouse(MouseMsg const& msg) override
		{
			Vector2 wordPos = mScreenToWorld.transformPosition(msg.getPos());

			Vec2i tilePos;
			tilePos.x = Math::FloorToInt(Math::ToTileValue(wordPos.x, 1.0f));
			tilePos.y = Math::FloorToInt(Math::ToTileValue(wordPos.y, 1.0f));
			if (msg.onLeftDown())
			{
				LogMsg("%d %d", tilePos.x, tilePos.y);
				if (mGame.mMap.checkRange(tilePos.x, tilePos.y))
				{
					mGame.handleTileClick(tilePos);
				}
			}
			return BaseClass::onMouse(msg);
		}

		MsgReply onKey(KeyMsg const& msg) override
		{
			if (msg.isDown())
			{
				switch (msg.getCode())
				{
				case EKeyCode::R: restart(); break;
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
	protected:
	};
	

	REGISTER_STAGE_ENTRY("SRPG", TestStage, EExecGroup::Dev);
}