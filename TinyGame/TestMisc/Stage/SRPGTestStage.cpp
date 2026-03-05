#include "Stage/TestStageHeader.h"
#include "Stage/TestRenderStageBase.h"

#include "GameRenderSetup.h"

#include "RHI/RHICommand.h"
#include "RHI/DrawUtility.h"
#include "RHI/RHIGraphics2D.h"

#include "Math/Vector2.h"
#include "Math/TVector2.h"
#include "DataStructure/Array.h"
#include "DataStructure/Grid2D.h"
#include "Async/Coroutines.h"
#include "TinyCore/TimerManager.h"
#include "Core/IntegerType.h"

namespace SRPG
{

	using namespace Render;
	using namespace Math;

	typedef Math::Vector2 Vector2;
	typedef TVector2<int> Vec2i;

	class GameTimeControl
	{
	public:
		float time;

		void update(float deltaTime)
		{
			time += deltaTime;
		}

		void reset()
		{
			time = 0;
		}
	};

	GameTimeControl GGameTime;

	class WaitForSeconds : public IAwaitInstruction
	{
	public:
		WaitForSeconds(float duration) : mDuration(duration) {}

		virtual void onSuspend(CoroutineContextHandle handle) override
		{
			::Global::GameTimerManager().addTimer(mDuration, [handle]()
			{
				Coroutines::Resume(handle);
			});
		}

	private:
		float mDuration;
	};

	enum EJob
	{
		Warrior,
		Archer,
		Mage,
	};

	enum ETeam
	{
		Player,
		Enemy,
	};

	struct AttributeSet
	{
		int health;
		int attack;
		int defense;
		int magicPower;
		int magicDefense;
		int mana;
		int moveStep;
		int attackRangeMin;
		int attackRangeMax;

		void setDefault(EJob job)
		{
			health = 100;
			attack = 20;
			defense = 10;
			magicPower = 10;
			magicDefense = 5;
			mana = 50;
			moveStep = 4;
			if (job == Archer)
			{
				attackRangeMin = 2;
				attackRangeMax = 3;
				attack = 18;
				moveStep = 3;
			}
			else if (job == Mage)
			{
				attackRangeMin = 1;
				attackRangeMax = 1;
				attack = 8;
				magicPower = 35;
				magicDefense = 15;
				mana = 100;
				moveStep = 3;
			}
			else // Warrior
			{
				attackRangeMin = 1;
				attackRangeMax = 1;
				attack = 25;
				defense = 15;
			}
		}
	};


	class Actor
	{
	public:
		Vec2i pos;
		Vector2 renderPos;
		TArray<Vec2i> movePath;
		ETeam team;
		EJob job;
		AttributeSet attributes;

		Coroutines::Event onActionFinish;
		bool bMoving = false;
		bool bDead = false;
		bool bFlying = false;

		bool isMoving() const { return movePath.size() > 0 || (renderPos - Vector2(pos)).length2() > 1e-4f; }
	};

	enum ECommand
	{
		CMD_ATTACK,
		CMD_MAGIC,
		CMD_ITEM,
		CMD_STAY,
		CMD_CANCEL,
		CMD_NONE
	};

	struct DamageEffect
	{
		Vector2 pos;
		int damage;
		float lifeTime;
	};

	struct MagicInfo
	{
		enum Type
		{
			Attack,
			Heal,
		};
		char const* name;
		int manaCost;
		int rangeMin;
		int rangeMax;
		int effectRadius;
		int power;
		Type type;
	};

	MagicInfo const GMagics[] =
	{
		{ "Fireball", 15, 1, 3, 1, 35, MagicInfo::Attack },
		{ "Heal",     10, 0, 3, 0, 40, MagicInfo::Heal },
		{ "Meteor",   40, 2, 5, 2, 60, MagicInfo::Attack },
	};

	namespace ETerrain 
	{
		enum Type : unsigned char
		{
			Plain,
			Water,
			Mountain,
			Forest,
			Block,
			Road,
		};
	}

#define P ETerrain::Plain
#define W ETerrain::Water
#define M ETerrain::Mountain
#define F ETerrain::Forest
#define R ETerrain::Road
#define B ETerrain::Block
	uint8 TestMap[]
	{
		P,P,P,F,F,P,P,P,P,P,P,W,W,P,
		P,M,P,P,F,P,P,R,R,R,P,W,W,P,
		P,M,M,P,P,P,P,R,P,R,P,P,P,P,
		P,P,P,P,P,M,P,R,P,R,R,R,P,P,
		F,F,P,P,M,M,P,R,P,P,P,R,P,P,
		W,W,P,P,P,P,P,R,R,P,P,R,P,P,
		W,W,W,P,F,F,P,P,R,R,R,R,P,P,
		P,W,W,P,F,F,P,P,P,P,P,P,P,P,
	};
#undef P
#undef W
#undef M
#undef F
#undef R
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
			initMap(TestMap, 14, 8);
			addActor(Vec2i(2, 4), ETeam::Player, Warrior);
			addActor(Vec2i(2, 5), ETeam::Player, Archer);
			addActor(Vec2i(2, 6), ETeam::Player, Mage);
			addActor(Vec2i(10, 3), ETeam::Enemy, Warrior);
			addActor(Vec2i(10, 4), ETeam::Enemy, Archer);
			addActor(Vec2i(10, 5), ETeam::Enemy, Mage);

		}
		void addActor(Vec2i const& pos, ETeam team, EJob job = Warrior)
		{
			auto actor = new Actor;
			actor->job = job;
			actor->attributes.setDefault(job);
			actor->team = team;
			actor->pos = pos;
			actor->renderPos = Vector2(pos);
			mActors.push_back(actor);
			auto& tile = mMap.getData(pos.x, pos.y);
			CHECK(tile.actor == nullptr);
			tile.actor = actor;
		}

		void moveActor(Actor& actor, int moveIndex)
		{
			if (moveIndex == INDEX_NONE) return; 
			auto const& targetMove = mMoveArea[moveIndex];
			
			Vec2i startPos = actor.pos;
			Vec2i endPos = targetMove.pos;

			if (startPos == endPos) return;

			// Reconstruct Path from current position to target, using mMoveArea as filter
			struct PathNode { Vec2i pos; int parent; };
			TArray<PathNode> openList;
			openList.push_back({ startPos, INDEX_NONE });

			int targetIndex = INDEX_NONE;
			for (int i = 0; i < openList.size(); ++i)
			{
				if (openList[i].pos == endPos) { targetIndex = i; break; }
				Vec2i offsets[] = { {1,0}, {0,1}, {-1,0}, {0,-1} };
				for (auto const& off : offsets)
				{
					Vec2i nextPos = openList[i].pos + off;
					bool inArea = mMoveArea.findIndexPred([&nextPos](auto const& info) { return info.pos == nextPos; }) != INDEX_NONE;
					if (!inArea) continue;

					if (openList.findIndexPred([&nextPos](auto const& node) { return node.pos == nextPos; }) == INDEX_NONE)
					{
						openList.push_back({ nextPos, i });
					}
				}
			}

			actor.movePath.clear();
			if (targetIndex != INDEX_NONE)
			{
				int cur = targetIndex;
				while (cur != INDEX_NONE)
				{
					actor.movePath.insertAt(0, openList[cur].pos);
					cur = openList[cur].parent;
				}
				if (actor.movePath.size() > 0 && actor.movePath[0] == actor.pos)
					actor.movePath.removeIndex(0);
			}

			auto& tile = mMap.getData(actor.pos.x, actor.pos.y);
			if (tile.actor == &actor) tile.actor = nullptr;

			actor.pos = endPos;
			auto& tileMove = mMap.getData(actor.pos.x, actor.pos.y);
			tileMove.actor = &actor;
		}


		struct BattleState
		{
			Actor* attacker = nullptr;
			Actor* defender = nullptr;
			int    damage = 0;
			float  timer = 0;
		} mBattleState;
		bool bInBattle = false;

		Coroutines::Event onBattleFinsih;

		void resolveCombat(Actor& attacker, Actor& defender)
		{
			int damage = Math::Max(1, attacker.attributes.attack - defender.attributes.defense);
			
			mBattleState.attacker = &attacker;
			mBattleState.defender = &defender;
			mBattleState.damage = damage;
			mBattleState.timer = 0;
			bInBattle = true;
		}

		void finishBattle()
		{
			int damage = mBattleState.damage;
			Actor& defender = *mBattleState.defender;
			defender.attributes.health -= damage;

			DamageEffect effect;
			effect.pos = Vector2(defender.pos) + Vector2(0.5f, 0.2f);
			effect.damage = damage;
			effect.lifeTime = 1.0f;
			mDamageEffects.push_back(effect);

			if (defender.attributes.health <= 0)
			{
				defender.attributes.health = 0;
				defender.bDead = true;
				auto& tile = mMap.getData(defender.pos.x, defender.pos.y);
				tile.actor = nullptr;
			}
			bInBattle = false;
			onBattleFinsih.trigger();
		}

		void resolveMagic(Actor& attacker, MagicInfo const& magic, Vec2i const& targetPos)
		{
			attacker.attributes.mana -= magic.manaCost;

			for (int dy = -magic.effectRadius; dy <= magic.effectRadius; ++dy)
			{
				for (int dx = -magic.effectRadius; dx <= magic.effectRadius; ++dx)
				{
					if (Math::Abs(dx) + Math::Abs(dy) > magic.effectRadius)
						continue;

					Vec2i pos = targetPos + Vec2i(dx, dy);
					if (!mMap.checkRange(pos.x, pos.y))
						continue;

					Actor* defender = mMap.getData(pos.x, pos.y).actor;
					if (defender)
					{
						int damage = 0;
						if (magic.type == MagicInfo::Attack)
						{
							damage = Math::Max(1, attacker.attributes.magicPower + magic.power - defender->attributes.magicDefense);
							defender->attributes.health -= damage;
						}
						else if (magic.type == MagicInfo::Heal)
						{
							damage = -(attacker.attributes.magicPower + magic.power);
							defender->attributes.health = Math::Min(100, defender->attributes.health - damage);
						}

						DamageEffect effect;
						effect.pos = Vector2(defender->pos) + Vector2(0.5f, 0.2f);
						effect.damage = Math::Abs(damage);
						effect.lifeTime = 1.0f;
						mDamageEffects.push_back(effect);

						if (defender->attributes.health <= 0)
						{
							defender->attributes.health = 0;
							defender->bDead = true;
							auto& tile = mMap.getData(defender->pos.x, defender->pos.y);
							tile.actor = nullptr;
						}
					}
				}
			}
		}

		void update(float dt)
		{
			GGameTime.update(dt);

			for (auto actor : mActors)
			{
				if (actor->movePath.size() > 0)
				{
					Vector2 target = Vector2(actor->movePath[0]);
					Vector2 dir = target - actor->renderPos;
					float dist = dir.length();
					float step = 6.0f * dt;
					if (dist < step)
					{
						actor->renderPos = target;
						actor->movePath.removeIndex(0);
						if (actor->movePath.empty())
						{
							actor->onActionFinish.trigger();
						}
					}
					else
					{
						actor->renderPos += dir * (step / dist);
					}
				}
				else
				{
					Vector2 target = Vector2(actor->pos);
					Vector2 dir = target - actor->renderPos;
					float dist = dir.length();
					if (dist > 0.001f)
					{
						float step = 6.0f * dt;
						if (dist < step) actor->renderPos = target;
						else actor->renderPos += dir * (step / dist);
					}
				}
			}

			for (int i = 0; i < mDamageEffects.size(); ++i)
			{
				mDamageEffects[i].lifeTime -= dt;
				mDamageEffects[i].pos.y -= dt * 0.5f;
				if (mDamageEffects[i].lifeTime < 0)
				{
					mDamageEffects.removeIndex(i);
					--i;
				}
			}
		}

		void cleanup()
		{
			Coroutines::Stop(mHandleLogic);

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
			virtual void onEnter(Game& game) {}
			virtual void onExit(Game& game) {}
			virtual void onTileClick(Game& game, Vec2i const& tilePos) {}
			virtual void onMouse(Game& game, MouseMsg const& msg) {}
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
			virtual void onTileClick(Game& game, Vec2i const& tilePos) override
			{
				int index = game.mMoveArea.findIndexPred([&tilePos](auto const& move)
				{
					return move.pos == tilePos;
				});

				if (index == INDEX_NONE)
					return;

				if (!game.canMoveTo(*game.mActiveActor, *game.mMoveArea[index].tile))
					return;

				Coroutines::Resume(game.mHandleLogic, index);
			}
		};

		class MenuAction : public InputActionT<MenuAction>
		{
		public:
			virtual void onTileClick(Game& game, Vec2i const& tilePos) override
			{
				Vec2i center = game.mActiveActor->pos;
				if (tilePos == center + Vec2i(1, 0)) Coroutines::Resume(game.mHandleLogic, (int)CMD_ATTACK);
				else if (tilePos == center + Vec2i(0, -1)) Coroutines::Resume(game.mHandleLogic, (int)CMD_ITEM);
				else if (tilePos == center + Vec2i(-1, 0)) Coroutines::Resume(game.mHandleLogic, (int)CMD_MAGIC);
				else if (tilePos == center + Vec2i(0, 1)) Coroutines::Resume(game.mHandleLogic, (int)CMD_STAY);
				else if (tilePos == center) Coroutines::Resume(game.mHandleLogic, (int)CMD_CANCEL);
				else
				{
					// Clicking outside menu icons could also cancel
				}
			}

			virtual void onMouse(Game& game, MouseMsg const& msg) override
			{
				if (msg.onRightDown())
				{
					Coroutines::Resume(game.mHandleLogic, (int)CMD_CANCEL);
				}
			}
		};

		class CastAction : public  InputActionT<CastAction>
		{
		public:
			virtual void onTileClick(Game& game, Vec2i const& tilePos) override
			{
				int index = game.mCastTargetList.findIndexPred([&game, &tilePos](auto const& targetIdx)
				{
					return game.mCastArea[targetIdx] == tilePos;
				});

				if (index == INDEX_NONE)
					return;

				Coroutines::Resume(game.mHandleLogic, game.mCastTargetList[index]);
			}

			virtual void onMouse(Game& game, MouseMsg const& msg) override
			{
				if (msg.onRightDown())
				{
					Coroutines::Resume(game.mHandleLogic, (int)INDEX_NONE);
				}
			}
		};

		class MagicSelectionAction : public InputActionT<MagicSelectionAction>
		{
		public:
			virtual void onTileClick(Game& game, Vec2i const& tilePos) override
			{
				Vec2i center = game.mActiveActor->pos;
				if (tilePos == center + Vec2i(1, 0)) Coroutines::Resume(game.mHandleLogic, 0); // Fireball
				else if (tilePos == center + Vec2i(0, -1)) Coroutines::Resume(game.mHandleLogic, 1); // Heal
				else if (tilePos == center + Vec2i(-1, 0)) Coroutines::Resume(game.mHandleLogic, 2); // Meteor
				else if (tilePos == center) Coroutines::Resume(game.mHandleLogic, (int)INDEX_NONE);
			}

			virtual void onMouse(Game& game, MouseMsg const& msg) override
			{
				if (msg.onRightDown())
				{
					Coroutines::Resume(game.mHandleLogic, (int)INDEX_NONE);
				}
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
			LogMsg("Action Changed -> %p", &action);
		}


		void logicEntry()
		{
			int currentActorIndex = 0;
			while (1)
			{
				if (mActors.empty()) break;
				currentActorIndex %= mActors.size();
				Actor* turnActor = mActors[currentActorIndex];
				
				if (turnActor->bDead)
				{
					mActors.removeIndex(currentActorIndex);
					continue;
				}

				mActiveActor = turnActor;
				Vec2i initialPos = turnActor->pos;
				bool bTurnEnd = false;
				bool bMoved = false;

				mMoveArea.clear();
				getMoveableArea(*turnActor, initialPos, mMoveArea);

				while (!bTurnEnd)
				{
					// --- PHASE 1: MOVEMENT ---
					if (!bMoved)
					{
						bShowMoveArea = true;
						bShowMenu = false;
						
						int indexMove = INDEX_NONE;
						if (turnActor->team == ETeam::Player)
						{
							changeAction(MoveAction::Get());
							indexMove = CO_AWAIT<int>(nullptr);
						}
						else // AI Move Decision
						{
							Actor* targetPlayer = nullptr;
							int minEnemyDist = 10000;
							for (auto a : mActors) 
							{
								if (a->team == ETeam::Player && !a->bDead) 
								{
									int d = (turnActor->pos - a->pos).length2();
									if (d < minEnemyDist) { minEnemyDist = d; targetPlayer = a; }
								}
							}

							if (targetPlayer)
							{
								float bestScore = -10000.0f;
								int minR = turnActor->attributes.attackRangeMin;
								int maxR = turnActor->attributes.attackRangeMax;

								// Mage considers magic range for positioning
								if (turnActor->job == Mage)
								{
									for (auto const& magic : GMagics)
									{
										if (turnActor->attributes.mana >= magic.manaCost)
										{
											maxR = Math::Max(maxR, magic.rangeMax);
										}
									}
								}

								for (int i = 0; i < mMoveArea.size(); ++i)
								{
									if (!canMoveTo(*turnActor, *mMoveArea[i].tile))
										continue;

									Vec2i tilePos = mMoveArea[i].pos;
									int dist = Math::Abs(tilePos.x - targetPlayer->pos.x) + Math::Abs(tilePos.y - targetPlayer->pos.y);
									
									float score = 0;
									if (dist >= minR && dist <= maxR) 
									{
										// We can attack/cast from here!
										score = 100.0f - (float)dist * 0.1f; 
									}
									else if (dist < minR)
									{
										score = -50.0f + (float)dist; 
									}
									else 
									{
										score = - (float)dist;
									}

									if (score > bestScore) { bestScore = score; indexMove = i; }
								}
							}
						}

						if (indexMove != INDEX_NONE)
						{
							if (turnActor->pos != mMoveArea[indexMove].pos)
							{
								moveActor(*turnActor, indexMove);
								if (turnActor->isMoving()) 
								{
									CO_AWAIT(turnActor->onActionFinish.wait()); 
								}
							}
							bMoved = true;
						}
						else if (turnActor->team == ETeam::Player) continue;
						else bMoved = true;
					}

					// --- PHASE 2: COMMAND MENU ---
					if (bMoved)
					{
						bShowMoveArea = (turnActor->team == ETeam::Player); 
						bShowMenu = (turnActor->team == ETeam::Player);
						
						int command = CMD_NONE;
						if (turnActor->team == ETeam::Player)
						{
							changeAction(MenuAction::Get());
							command = CO_AWAIT<int>(nullptr);
						}
						else // AI Menu Decision
						{
							// AI logic: Scan for targets
							command = CMD_STAY;

							// Check Magic first for Mage
							if (turnActor->job == Mage)
							{
								for (int mIdx = 0; mIdx < ARRAY_SIZE(GMagics); ++mIdx)
								{
									auto const& magic = GMagics[mIdx];
									if (turnActor->attributes.mana >= magic.manaCost)
									{
										for (int dy = -magic.rangeMax; dy <= magic.rangeMax; ++dy)
										{
											for (int dx = -magic.rangeMax; dx <= magic.rangeMax; ++dx)
											{
												int dist = Math::Abs(dx) + Math::Abs(dy);
												if (dist >= magic.rangeMin && dist <= magic.rangeMax)
												{
													// For AI simpler logic: check if there's an enemy in area
													Vec2i targetPos = turnActor->pos + Vec2i(dx, dy);
													if (mMap.checkRange(targetPos.x, targetPos.y))
													{
														auto const& tile = mMap(targetPos.x, targetPos.y);
														if (tile.actor && tile.actor->team != turnActor->team)
														{
															command = CMD_MAGIC;
															goto AI_DECISION_DONE;
														}
													}
												}
											}
										}
									}
								}
							}

							// Check Normal Attack
							{
								int minR = turnActor->attributes.attackRangeMin;
								int maxR = turnActor->attributes.attackRangeMax;
								for (int dy = -maxR; dy <= maxR; ++dy)
								{
									for (int dx = -maxR; dx <= maxR; ++dx)
									{
										int dist = Math::Abs(dx) + Math::Abs(dy);
										if (dist >= minR && dist <= maxR)
										{
											if (canAttack(*turnActor, turnActor->pos + Vec2i(dx, dy)))
											{
												command = CMD_ATTACK;
												goto AI_DECISION_DONE;
											}
										}
									}
								}
							}

						AI_DECISION_DONE:;
						}

						bShowMenu = false;

						if (command == CMD_STAY)
						{
							bTurnEnd = true;
						}
						else if (command == CMD_CANCEL)
						{
							bMoved = false;
						}
						else if (command == CMD_ATTACK)
						{
							mCastArea.clear();
							mCastTargetList.clear();
							
							int minR = turnActor->attributes.attackRangeMin;
							int maxR = turnActor->attributes.attackRangeMax;

							for (int dy = -maxR; dy <= maxR; ++dy)
							{
								for (int dx = -maxR; dx <= maxR; ++dx)
								{
									int dist = Math::Abs(dx) + Math::Abs(dy);
									if (dist >= minR && dist <= maxR)
									{
										Vec2i targetPos = turnActor->pos + Vec2i(dx, dy);
										if (mMap.checkRange(targetPos.x, targetPos.y))
										{
											int areaIdx = mCastArea.size();
											mCastArea.push_back(targetPos);
											if (canAttack(*turnActor, targetPos))
												mCastTargetList.push_back(areaIdx);
										}
									}
								}
							}

							int indexTarget = INDEX_NONE;
							if (turnActor->team == ETeam::Player)
							{
								bShowCastArea = true;
								changeAction(CastAction::Get());
								indexTarget = CO_AWAIT<int>(nullptr);
								bShowCastArea = false;
							}
							else if (mCastTargetList.size() > 0)
							{
								// AI Decision logic: show all targets, pick one, and wait a bit
								bShowCastArea = true;
								CO_AWAIT(WaitForSeconds(0.4));
								
								indexTarget = mCastTargetList[0]; // Just pick first victim
								
								// Filter list to highlight only the SPECIFIC target chosen
								int listVal = indexTarget;
								mCastTargetList.clear();
								mCastTargetList.push_back(listVal);
								
								CO_AWAIT(WaitForSeconds(0.4));
								bShowCastArea = false;
							}

							if (indexTarget != INDEX_NONE)
							{
								Vec2i targetPos = mCastArea[indexTarget];
								Actor* defender = mMap(targetPos.x, targetPos.y).actor;
								if (defender) 
								{
									resolveCombat(*turnActor, *defender);
									CO_AWAIT(onBattleFinsih.wait());
								}
								bTurnEnd = true;
								CO_AWAIT(WaitForSeconds(0.5));
							}
							else if (turnActor->team != ETeam::Player) bTurnEnd = true;
						}
						else if (command == CMD_MAGIC)
						{
							int magicIdx = INDEX_NONE;
							if (turnActor->team == ETeam::Player)
							{
								bShowMagicMenu = true;
								changeAction(MagicSelectionAction::Get());
								magicIdx = CO_AWAIT<int>(nullptr);
								bShowMagicMenu = false;
							}
							else if (turnActor->job == Mage)
							{
								// AI simple logic: if low health heal, else meteor or fireball
								if (turnActor->attributes.health < 40) magicIdx = 1;
								else magicIdx = 0;
							}

							if (magicIdx != INDEX_NONE)
							{
								mSelectedMagic = magicIdx;
								MagicInfo const& magic = GMagics[magicIdx];
								
								mCastArea.clear();
								mCastTargetList.clear();

								for (int dy = -magic.rangeMax; dy <= magic.rangeMax; ++dy)
								{
									for (int dx = -magic.rangeMax; dx <= magic.rangeMax; ++dx)
									{
										int dist = Math::Abs(dx) + Math::Abs(dy);
										if (dist >= magic.rangeMin && dist <= magic.rangeMax)
										{
											Vec2i targetPos = turnActor->pos + Vec2i(dx, dy);
											if (mMap.checkRange(targetPos.x, targetPos.y))
											{
												int areaIdx = mCastArea.size();
												mCastArea.push_back(targetPos);
												// For magic, we can cast on any tile in range
												mCastTargetList.push_back(areaIdx);
											}
										}
									}
								}

								int indexTarget = INDEX_NONE;
								if (turnActor->team == ETeam::Player)
								{
									bShowCastArea = true;
									changeAction(CastAction::Get());
									indexTarget = CO_AWAIT<int>(nullptr);
									bShowCastArea = false;
								}
								else if (mCastTargetList.size() > 0)
								{
									// AI picked a target pos
									indexTarget = 0; // Just pick first tile for now or random
									CO_AWAIT(WaitForSeconds(0.6));
								}

								if (indexTarget != INDEX_NONE)
								{
									resolveMagic(*turnActor, magic, mCastArea[indexTarget]);
									bTurnEnd = true;
									CO_AWAIT(WaitForSeconds(0.6));
								}
								else if (turnActor->team != ETeam::Player) bTurnEnd = true;
							}
							else
							{
								// Cancelled magic
								bTurnEnd = false; // Go back to menu
							}
						}
					}
				}
				bShowMoveArea = false; bShowMenu = false; bShowCastArea = false; bShowMagicMenu = false;
				// Global turn end delay
				CO_AWAIT(WaitForSeconds(0.4));
				currentActorIndex++;
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
			unsigned char value;
			Actor* actor;
		};

		struct MoveInfo
		{
			Vec2i    pos;
			MapTile* tile;
			int step;
			float remainStep;
			int parentIndex = INDEX_NONE;
		};
		bool bShowMoveArea = true;
		TArray<MoveInfo> mMoveArea;
		bool bShowCastArea = false;
		bool bShowMenu = false;
		TArray< Vec2i > mCastArea;
		TArray< int >   mCastTargetList;
		bool bShowMagicMenu = false;
		int mSelectedMagic = INDEX_NONE; // Index of the magic spell selected

		bool canPass(Actor& actor, MapTile const& tile)
		{
			if (tile.value == ETerrain::Block)
				return false;

			// Ground units cannot enter water
			if (tile.value == ETerrain::Water && !actor.bFlying)
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

					float moveCost = 1.0f;
					if (tileMove.value == ETerrain::Mountain)
					{
						moveCost = (actor.job == Archer) ? 4.0f : 2.5f; // Archer hates mountains
					}
					else if (tileMove.value == ETerrain::Forest)
					{
						moveCost = (actor.job == Archer) ? 1.0f : 2.0f; // Archer moves through forest easily
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
						if (move.remainStep >= remainStep)
							continue;

						move.remainStep = remainStep;
						move.parentIndex = index;
					}
					else
					{
						MoveInfo move;
						move.step = prevMove.step + 1;
						move.pos = movePos;
						move.tile = &tileMove;
						move.remainStep = remainStep;
						move.parentIndex = index;
						outMoveList.push_back(move);
					}
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
		Actor* mActiveActor = nullptr;
		TArray<DamageEffect> mDamageEffects;

		Coroutines::ExecutionHandle mHandleLogic;
	};



	class TestStage : public StageBase 
		            , public IGameRenderSetup
	{
		using BaseClass = StageBase;
	public:

		DECLARE_HOTRELOAD_CLASS(TestStage);

		TestStage(){}

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

		Render::RHITexture2DRef mTexBattleBg;
		Render::RHITexture2DRef mTexPlayer;
		Render::RHITexture2DRef mTexEnemy;
		Render::RHITexture2DRef mTexSkeleton;

		void restart() 
		{
			mGame.setup();

			mTexBattleBg = Render::RHIUtility::LoadTexture2DFromFile("Texture/SRPG/srpg_battle_bg_1772350016731.png");
			mTexPlayer  = Render::RHIUtility::LoadTexture2DFromFile("Texture/SRPG/srpg_player_warrior_1772350038047.png");
			mTexEnemy   = Render::RHIUtility::LoadTexture2DFromFile("Texture/SRPG/srpg_enemy_skeleton_1772351300740.png");

			Vec2i screenSize = ::Global::GetScreenSize();
			mWorldToScreen = RenderTransform2D::LookAt(screenSize, 0.5 * Vector2(mGame.mMap.getSize()), Vector2(0, -1), screenSize.y / float(10 + 2.5), false);

			mScreenToWorld = mWorldToScreen.inverse();

			mGame.run();
		}

		void onUpdate(GameTimeSpan deltaTime) override
		{
			BaseClass::onUpdate(deltaTime);
			float dt = deltaTime.value;
			mTotalTime += dt;
			
			if (mGame.bInBattle)
			{
				mGame.mBattleState.timer += dt;
				if (mGame.mBattleState.timer > 3.0f) // Battle scene duration
				{
					mGame.finishBattle();
				}
			}
			else
			{
				mGame.update(dt);
			}

			Coroutines::ThreadContext::Get().checkAllExecutions();
		}

		float mTotalTime = 0.0f;
		Vec2i mHoverPos = Vec2i(-1, -1);
		Game mGame;
		
		void drawBattleScene()
		{
			RHIGraphics2D& g = Global::GetRHIGraphics2D();
			Vec2i screenSize = ::Global::GetScreenSize();
			
			// Fill background with black
			g.setBrush(LinearColor(0,0,0));
			g.drawRect(Vector2(0,0), Vector2(screenSize));

			g.setBrush(LinearColor(1, 1, 1));
			// Battle Window Size (Original Aspect Ratio of SRPG)
			Vector2 windowSize = Vector2(screenSize.x * 0.85f, screenSize.y * 0.7f);
			if (windowSize.x > windowSize.y * 1.5f) windowSize.x = windowSize.y * 1.5f;
			Vector2 windowPos = Vector2((screenSize.x - windowSize.x) * 0.5f, (screenSize.y - windowSize.y) * 0.5f);

			// Draw Background
			if (mTexBattleBg)
			{
				g.drawTexture(*mTexBattleBg, windowPos, windowSize);
			}

			// Animation logic
			float animTime = mGame.mBattleState.timer;
			float playerOffX = 0;
			float enemyOffX = 0;
			float enemyFlash = 1.0f;
			
			if (animTime < 0.6f) 
			{
				// Entry animation
				float t = animTime / 0.6f;
				playerOffX = (1-t) * 100.0f;
				enemyOffX = (1-t) * -100.0f;
			}
			else if (animTime < 1.2f)
			{
				// Attacker moves forward
				float t = (animTime - 0.6f) / 0.6f;
				playerOffX = -Math::Sin(t * Math::PI) * 40.0f;
			}
			else if (animTime < 2.0f)
			{
				// Defender flashes / hit
				float t = (animTime - 1.2f);
				enemyFlash = (Math::Sin(t * 20.0f) > 0) ? 0.3f : 1.0f;
				enemyOffX = Math::Sin(t * 30.0f) * 4.0f;
			}

			Vector2 charSize = Vector2(windowSize.y * 0.5f, windowSize.y * 0.65f);
			g.beginBlend(enemyFlash, ESimpleBlendMode::Translucent);
			// Draw Enemy (Leftish, facing right)
			RHITexture2D* enemyTex = mTexEnemy;
			if (enemyTex)
			{
				g.drawTexture(*enemyTex, windowPos + Vector2(windowSize.x * 0.25f + enemyOffX, windowSize.y * 0.25f), charSize);
			}

			// Draw Player (Rightish, back to camera)
			if (mTexPlayer)
			{
				g.drawTexture(*mTexPlayer, windowPos + Vector2(windowSize.x * 0.55f + playerOffX, windowSize.y * 0.32f), charSize);
			}
			g.endBlend();
			// UI Boxes (Shining Force Style)
			auto drawStatusBox = [&](Vector2 pos, Actor& actor, bool bIsPlayer)
			{
				Vector2 boxSize(Math::Min(300.0f, windowSize.x * 0.4f), 120.0f);
				
				// Outer Glow Background
				g.beginBlend(0.4f, ESimpleBlendMode::Translucent);
				g.setBrush(LinearColor(0,0,0));
				g.drawRect(pos + Vector2(4, 4), boxSize);
				g.endBlend();

				// Main Box
				g.setBrush(LinearColor(0.04f, 0.12f, 0.45f)); // Classic Blue
				g.setPen(LinearColor(0.85f, 0.75f, 0.4f), 3.5f); // Gold / Tan
				g.drawRect(pos, boxSize);

				RenderUtility::SetFont(g, FONT_S16);
				RenderUtility::SetFontColor(g, EColor::White);
				char const* name = (actor.team == Player) ? "Hero" : "Goblin";
				if (actor.job == Mage) name = "Mage";
				else if (actor.job == Archer) name = "Archer";


				g.drawTextF(pos + Vector2(15, 12), "%s  L1", name);

				// HP Bar
				g.drawTextF(pos + Vector2(15, 48), "HP");
				g.setBrush(LinearColor(0.05f, 0.05f, 0.1f));
				g.drawRect(pos + Vector2(60, 52), Vector2(150, 18));
				
				float hpFactor = (float)Math::Max(0, actor.attributes.health) / 100.0f;
				g.setBrush(LinearColor(0.95f, 0.35f, 0.15f)); // Orange/Red
				g.drawRect(pos + Vector2(60, 52), Vector2(150 * hpFactor, 12));
				g.setBrush(LinearColor(1.0f, 0.6f, 0.15f)); // Lighter top Part
				g.drawRect(pos + Vector2(60, 52), Vector2(150 * hpFactor, 6));

				g.drawTextF(pos + Vector2(220, 48), "%d/%d", Math::Max(0, actor.attributes.health), 100);

				// MP Bar
				g.drawTextF(pos + Vector2(15, 82), "MP");
				g.setBrush(LinearColor(0.05f, 0.05f, 0.1f));
				g.drawRect(pos + Vector2(60, 86), Vector2(150, 18));
				
				float mpFactor = (float)actor.attributes.mana / 100.0f;
				g.setBrush(LinearColor(0.15f, 0.55f, 0.95f)); // Blue
				g.drawRect(pos + Vector2(60, 86), Vector2(150 * mpFactor, 12));
				g.setBrush(LinearColor(0.35f, 0.75f, 1.0f)); // Lighter top Part
				g.drawRect(pos + Vector2(60, 86), Vector2(150 * mpFactor, 6));

				g.drawTextF(pos + Vector2(220, 82), "%d/%d", actor.attributes.mana, 100);
			};

			if (mGame.mBattleState.attacker)
				drawStatusBox(windowPos + Vector2(windowSize.x - 310, 15), *mGame.mBattleState.attacker, true);
			
			if (mGame.mBattleState.defender)
				drawStatusBox(windowPos + Vector2(15, windowSize.y - 135), *mGame.mBattleState.defender, false);
		}

		void onRender(float dFrame) override
		{
			RHIGraphics2D& g = Global::GetRHIGraphics2D();

			RHICommandList& commandList = RHICommandList::GetImmediateList();
			RHISetFrameBuffer(commandList, nullptr);
			RHIClearRenderTargets(commandList, EClearBits::Color, &LinearColor(0.12f, 0.12f, 0.15f, 1.0f), 1);

			g.beginRender();
			
			if (mGame.bInBattle)
			{
				drawBattleScene();
				g.endRender();
				return;
			}

			g.pushXForm();
			g.transformXForm(mWorldToScreen, true);

			g.setTextRemoveScale(true);

			// --- Palette ---
			const LinearColor colorPlain    = LinearColor(0.35f, 0.55f, 0.25f);
			const LinearColor colorWater    = LinearColor(0.12f, 0.40f, 0.70f);
			const LinearColor colorWaterDeep= LinearColor(0.08f, 0.30f, 0.55f);
			const LinearColor colorMountain = LinearColor(0.48f, 0.42f, 0.38f);
			const LinearColor colorForest   = LinearColor(0.12f, 0.38f, 0.20f);
			const LinearColor colorRoad     = LinearColor(0.80f, 0.72f, 0.52f);
			const LinearColor colorSand     = LinearColor(0.65f, 0.60f, 0.42f);

			auto isWater = [&](int x, int y) {
				if (!mGame.mMap.checkRange(x, y)) return false;
				return mGame.mMap(x, y).value == ETerrain::Water;
			};

			// --- PASS 1: GROUND ---
			for (int j = 0; j < mGame.mMap.getSizeY(); ++j)
			{
				for (int i = 0; i < mGame.mMap.getSizeX(); ++i)
				{
					auto const& tile = mGame.mMap(i, j);
					Vector2 p(i, j);

					LinearColor base = (tile.value == ETerrain::Road) ? colorRoad : 
						               (tile.value == ETerrain::Water) ? colorWater : colorPlain;
					g.setBrush(base);
					RenderUtility::SetPen(g, EColor::Null);
					g.drawRect(p, Vector2(1, 1));

					if (tile.value != ETerrain::Water)
					{
						g.setBrush(colorSand);
						float bw = 0.08f;
						if (isWater(i, j - 1)) g.drawRect(p, Vector2(1, bw));
						if (isWater(i, j + 1)) g.drawRect(p + Vector2(0, 1 - bw), Vector2(1, bw));
						if (isWater(i - 1, j)) g.drawRect(p, Vector2(bw, 1));
						if (isWater(i + 1, j)) g.drawRect(p + Vector2(1 - bw, 0), Vector2(bw, 1));
					}
					else 
					{
						g.setBrush(colorWaterDeep);
						float xMin = isWater(i - 1, j) ? 0.0f : 0.25f;
						float xMax = isWater(i + 1, j) ? 1.0f : 0.75f;
						float yMin = isWater(i, j - 1) ? 0.0f : 0.25f;
						float yMax = isWater(i, j + 1) ? 1.0f : 0.75f;
						g.drawRect(p + Vector2(xMin, yMin), Vector2(xMax - xMin, yMax - yMin));
					}
				}
			}

			// --- PASS 2: OBJECTS & ACTORS ---
			for (int j = 0; j < mGame.mMap.getSizeY(); ++j)
			{
				for (int i = 0; i < mGame.mMap.getSizeX(); ++i) 
				{
					Vector2 p = Vector2(i, j);
					auto const& tile = mGame.mMap(i, j);
					
					// Draw elevated terrain (forest, mountain)
					if (tile.value == ETerrain::Forest)
					{
						g.setBrush(LinearColor(0.12f, 0.45f, 0.15f));
						g.drawCircle(p + Vector2(0.5f, 0.45f), 0.42f);
						g.drawCircle(p + Vector2(0.32f, 0.65f), 0.35f);
						g.drawCircle(p + Vector2(0.72f, 0.62f), 0.38f);
					}
					else if (tile.value == ETerrain::Mountain)
					{
						g.setBrush(LinearColor(0.55f, 0.52f, 0.48f));
						Vector2 pts[] = { p + Vector2(0.5f, 0.15f), p + Vector2(0.05f, 0.85f), p + Vector2(0.95f, 0.85f) };
						g.drawPolygon(pts, 3);
						g.setBrush(LinearColor(0.85f, 0.88f, 0.92f)); 
						Vector2 snow[] = { p + Vector2(0.5f, 0.15f), p + Vector2(0.35f, 0.4f), p + Vector2(0.65f, 0.4f) };
						g.drawPolygon(snow, 3);
					}
				}

				// Draw all actors that are visually on this row
				for (auto actorPtr : mGame.mActors)
				{
					auto& actor = *actorPtr;
					if (actor.bDead) continue;
					if (Math::FloorToInt(actor.renderPos.y + 0.5f) != j) continue;

					Vector2 p = actor.renderPos;
					float bob = actor.bFlying ? Math::Sin((float)mTotalTime * 5.0f) * 0.04f : 0.0f;
					Vector2 basePos = p + Vector2(0.5f, 0.82f);
					Vector2 unitPos = basePos + Vector2(0, -0.22f + bob); 
					LinearColor team = (actor.team == ETeam::Player) ? LinearColor(0.2f, 0.65f, 1.0f) : LinearColor(1.0f, 0.25f, 0.25f);

					// UNIT SHADOW - Small and transparent
					g.beginBlend(0.2f, ESimpleBlendMode::Translucent);
					g.setBrush(LinearColor(0, 0, 0.05f));
					g.drawCircle(basePos, 0.25f); 
					g.endBlend();

					// Unit
					g.setPen(LinearColor(1, 1, 1, 1), 1.6f);
					g.setBrush(team);
					Vector2 body[] = { unitPos + Vector2(-0.2f, 0.35f), unitPos + Vector2(0.2f, 0.35f), 
									   unitPos + Vector2(0.18f, -0.1f), unitPos + Vector2(-0.18f, -0.1f) };
					g.drawPolygon(body, 4);
					g.drawCircle(unitPos + Vector2(0, -0.25f), 0.20f); 

					// Health Bar
					g.beginBlend(0.8f, ESimpleBlendMode::Translucent);
					RenderUtility::SetPen(g, EColor::Null);
					float hp = (float)actor.attributes.health / 100.0f;
					Vector2 hpP = p + Vector2(0.15f, -0.18f + bob);
					g.setBrush(LinearColor(0,0,0));
					g.drawRect(hpP, Vector2(0.7f, 0.08f));
					g.setBrush(LinearColor(0.2f, 1.0f, 0.3f));
					g.drawRect(hpP, Vector2(0.7f * hp, 0.08f));
					g.endBlend();
				}
			}

			// --- OVERLAYS ---
			if (mGame.bShowMoveArea)
			{
				float pulse = Math::Sin((float)mTotalTime * 5.0f);
				g.beginBlend(0.3f + 0.1f * pulse, ESimpleBlendMode::Translucent);
				for (auto const& move : mGame.mMoveArea)
				{
					bool bCanStay = mGame.canMoveTo(*mGame.mActiveActor, *move.tile);
					g.setBrush(bCanStay ? LinearColor(0.2f, 0.7f, 1.0f) : LinearColor(0.1f, 0.3f, 0.6f));
					g.setPen(bCanStay ? LinearColor(0.5f, 0.9f, 1.0f) : LinearColor(0.3f, 0.5f, 0.8f), 1.5f);
					g.drawRect(Vector2(move.pos) + Vector2(0.05f, 0.05f), Vector2(0.9f, 0.9f));
				}
				g.endBlend();
			}

			if (mGame.bShowCastArea) 
			{
				float pulse = Math::Sin((float)mTotalTime * 5.0f);
				g.beginBlend(0.2f + 0.1f * pulse, ESimpleBlendMode::Translucent);
				g.setBrush(LinearColor(1.0f, 0.2f, 0.2f));
				g.setPen(LinearColor(1.0f, 0.5f, 0.5f), 1.0f);
				for (auto const& pos : mGame.mCastArea) g.drawRect(Vector2(pos) + Vector2(0.1f, 0.1f), Vector2(0.8f, 0.8f));
				g.endBlend();

				// Highlight actual targets more strongly
				g.beginBlend(0.4f + 0.2f * pulse, ESimpleBlendMode::Translucent);
				g.setPen(LinearColor(1.0f, 1.0f, 1.0f), 2.0f);
				for (int idx : mGame.mCastTargetList) g.drawRect(Vector2(mGame.mCastArea[idx]) + Vector2(0.05f, 0.05f), Vector2(0.9f, 0.9f));
				g.endBlend();

				// AOE Preview
				if (mGame.mSelectedMagic != INDEX_NONE && mGame.mMap.checkRange(mHoverPos.x, mHoverPos.y))
				{
					bool bInCastArea = false;
					for (auto const& pos : mGame.mCastArea) { if (pos == mHoverPos) { bInCastArea = true; break; } }

					if (bInCastArea)
					{
						auto const& magic = GMagics[mGame.mSelectedMagic];
						if (magic.effectRadius > 0)
						{
							g.beginBlend(0.5f + 0.1f * pulse, ESimpleBlendMode::Translucent);
							g.setBrush(LinearColor(0.2f, 1.0f, 1.0f)); // Cyan AOE
							g.setPen(LinearColor(1.0f, 1.0f, 1.0f), 2.5f);
							int radius = magic.effectRadius;
							for (int x = -radius; x <= radius; ++x)
							{
								for (int y = -radius; y <= radius; ++y)
								{
									if (std::abs(x) + std::abs(y) <= radius)
									{
										Vec2i aoePos = mHoverPos + Vec2i(x, y);
										if (mGame.mMap.checkRange(aoePos.x, aoePos.y))
										{
											g.drawRect(Vector2(aoePos), Vector2(1, 1));
										}
									}
								}
							}
							g.endBlend();
						}
					}
				}
			}

			if (mGame.bShowMenu && mGame.mActiveActor)
			{
				Vector2 center = Vector2(mGame.mActiveActor->pos);
				auto drawIcon = [&](Vector2 offset, char const* label, LinearColor color, bool bActive)
				{
					Vector2 p = center + offset;
					
					// Draw Background Circle
					g.beginBlend(0.85f, ESimpleBlendMode::Translucent);
					g.setBrush(color);
					g.setPen(LinearColor(1, 1, 1), bActive ? 3.0f : 1.2f);
					g.drawCircle(p + Vector2(0.5f, 0.5f), 0.44f);
					g.endBlend();

					// Draw Label
					RenderUtility::SetFont(g, FONT_S12);
					RenderUtility::SetFontColor(g, bActive ? EColor::Yellow : EColor::White);
					g.drawTextF(p + Vector2(0.24f, 0.38f), label);
				};

				bool hoverAtk = (mHoverPos == mGame.mActiveActor->pos + Vec2i(1, 0));
				bool hoverEnd = (mHoverPos == mGame.mActiveActor->pos + Vec2i(0, 1));
				bool hoverMag = (mHoverPos == mGame.mActiveActor->pos + Vec2i(-1, 0));
				bool hoverItm = (mHoverPos == mGame.mActiveActor->pos + Vec2i(0, -1));
				bool hoverCancel = (mHoverPos == mGame.mActiveActor->pos);

				drawIcon(Vector2(1, 0), "ATK", LinearColor(0.9f, 0.1f, 0.1f), hoverAtk);
				drawIcon(Vector2(0, 1), "END", LinearColor(0.25f, 0.25f, 0.25f), hoverEnd);
				drawIcon(Vector2(-1, 0), "MAG", LinearColor(0.1f, 0.2f, 0.9f), hoverMag);
				drawIcon(Vector2(0, -1), "ITM", LinearColor(0.1f, 0.8f, 0.1f), hoverItm);
				
				// Central Cancel Icon (Actor position)
				g.beginBlend(hoverCancel ? 0.8f : 0.4f, ESimpleBlendMode::Translucent);
				g.setBrush(LinearColor(1, 1, 0.2f));
				g.setPen(LinearColor(1, 1, 1), hoverCancel ? 2.5f : 1.0f);
				g.drawCircle(center + Vector2(0.5f, 0.5f), 0.3f);
				g.endBlend();
			}

			if (mGame.bShowMagicMenu && mGame.mActiveActor) 
			{
				Vector2 center = Vector2(mGame.mActiveActor->pos);
				auto drawIcon = [&](Vector2 offset, char const* label, LinearColor color, bool bActive) 
				{
					Vector2 p = center + offset;
					g.beginBlend(0.85f, ESimpleBlendMode::Translucent);
					g.setBrush(color);
					g.setPen(LinearColor(1, 1, 1), bActive ? 3.0f : 1.2f);
					g.drawCircle(p + Vector2(0.5f, 0.5f), 0.44f);
					g.endBlend();
					RenderUtility::SetFont(g, FONT_S12);
					RenderUtility::SetFontColor(g, bActive ? EColor::Yellow : EColor::White);
					g.drawTextF(p + Vector2(0.12f, 0.38f), label);
				};

				bool hoverF = (mHoverPos == mGame.mActiveActor->pos + Vec2i(1, 0));
				bool hoverH = (mHoverPos == mGame.mActiveActor->pos + Vec2i(0, -1));
				bool hoverM = (mHoverPos == mGame.mActiveActor->pos + Vec2i(-1, 0));
				bool hoverCancel = (mHoverPos == mGame.mActiveActor->pos);

				drawIcon(Vector2(1, 0), "FIRE", LinearColor(1.0f, 0.4f, 0.1f), hoverF);
				drawIcon(Vector2(0, -1), "HEAL", LinearColor(0.2f, 0.8f, 0.4f), hoverH);
				drawIcon(Vector2(-1, 0), "METEOR", LinearColor(0.5f, 0.1f, 0.8f), hoverM);

				g.beginBlend(hoverCancel ? 0.8f : 0.4f, ESimpleBlendMode::Translucent);
				g.setBrush(LinearColor(1.0f, 1.0f, 0.2f));
				g.drawCircle(center + Vector2(0.5f, 0.5f), 0.3f);
				g.endBlend();
			}

			if (mGame.mMap.checkRange(mHoverPos.x, mHoverPos.y)) 
			{
				g.beginBlend(0.35f, ESimpleBlendMode::Translucent);
				g.setBrush(LinearColor(1, 1, 1));
				g.setPen(LinearColor(1, 1, 1), 2.5f);
				g.drawRect(Vector2(mHoverPos), Vector2(1, 1));
				g.endBlend();
			}

			// --- PASS 4: FINAL INDICATORS (Drawn on top of everything) ---
			if (mGame.mActiveActor && !mGame.mActiveActor->bDead)
			{
				auto& actor = *mGame.mActiveActor;
				Vector2 p = actor.renderPos;
				float bob = actor.bFlying ? Math::Sin((float)mTotalTime * 5.0f) * 0.04f : 0.0f;
				float indicatorBob = Math::Sin((float)mTotalTime * 6.0f) * 0.05f;
				Vector2 unitPos = p + Vector2(0.5f, 0.82f - 0.22f + bob);
				Vector2 indicatorPos = unitPos + Vector2(0, -0.55f + indicatorBob);

				g.setBrush(LinearColor(1, 1, 0.2f));
				g.setPen(LinearColor(1, 1, 1), 1.2f);
				Vector2 tri[] = { indicatorPos + Vector2(-0.12f, -0.12f), indicatorPos + Vector2(0.12f, -0.12f), indicatorPos + Vector2(0, 0.08f) };
				g.drawPolygon(tri, 3);
			}

			g.popXForm();

			// --- UI STATUS CARDS ---
			Vec2i screenSize = ::Global::GetScreenSize();
			auto drawStatusBox = [&](Vector2 pos, Actor const& actor, bool bAlignRight) 
			{
				float width = 200.0f;
				float height = 100.0f;
				Vector2 boxPos = pos;
				if (bAlignRight) boxPos.x -= width;

				g.beginBlend(0.75f, ESimpleBlendMode::Translucent);
				g.setBrush(actor.team == ETeam::Player ? LinearColor(0.12f, 0.22f, 0.45f) : LinearColor(0.45f, 0.12f, 0.12f));
				g.setPen(LinearColor(0.9f, 0.9f, 0.9f), 2.0f);
				g.drawRoundRect(boxPos, Vector2(width, height), Vector2(10, 10));
				g.endBlend();

				RenderUtility::SetFont(g, FONT_S12);
				RenderUtility::SetFontColor(g, EColor::White);
				Vector2 tPos = boxPos + Vector2(15, 12);
				g.drawTextF(tPos, "%s - HP: %d / 100", (actor.team == ETeam::Player ? "PLAYER" : "ENEMY"), actor.attributes.health);
				
				tPos.y += 18;
				float barW = width - 30;
				g.setBrush(LinearColor(0,0,0,0.5f));
				g.drawRect(tPos, Vector2(barW, 8));
				g.setBrush(actor.attributes.health > 30 ? LinearColor(0.2f, 0.9f, 0.3f) : LinearColor(1, 0.2f, 0.2f));
				g.drawRect(tPos, Vector2(barW * (actor.attributes.health / 100.0f), 8));


				tPos.y += 40;
				g.drawTextF(tPos, "ATK: %d / MAG: %d", actor.attributes.attack, actor.attributes.magicPower);
				tPos.y += 18;
				g.drawTextF(tPos, "DEF: %d / MDEF: %d", actor.attributes.defense, actor.attributes.magicDefense);
				tPos.y += 18;
				g.drawTextF(tPos, "MP: %d / 100", actor.attributes.mana);
				tPos.y += 18;
				g.drawTextF(tPos, "MOVE: %d", actor.attributes.moveStep);
			};

			// Bottom Left: Active Actor
			if (mGame.mActiveActor)
			{
				drawStatusBox(Vector2(20, screenSize.y - 120), *mGame.mActiveActor, false);
			}

			// Top Right: Hovered or Target
			Actor* infoActor = nullptr;
			if (mGame.bShowCastArea && !mGame.mCastTargetList.empty())
			{
				// Show current candidate target
				Vec2i targetingPos = mGame.mCastArea[mGame.mCastTargetList[0]];
				infoActor = mGame.mMap(targetingPos.x, targetingPos.y).actor;
			}
			
			if (!infoActor && mGame.mMap.checkRange(mHoverPos.x, mHoverPos.y))
			{
				infoActor = mGame.mMap(mHoverPos.x, mHoverPos.y).actor;
			}

			if (infoActor)
			{
				drawStatusBox(Vector2(screenSize.x - 20, 20), *infoActor, true);
			}

			// Damage Effects
			for (auto const& effect : mGame.mDamageEffects)
			{
				Vector2 sPos = mWorldToScreen.transformPosition(effect.pos);
				g.beginBlend(effect.lifeTime, ESimpleBlendMode::Translucent);
				RenderUtility::SetFont(g, FONT_S24);
				RenderUtility::SetFontColor(g, EColor::Red);
				g.drawTextF(sPos, "%d", effect.damage);
				g.endBlend();
			}

			g.endRender();
		}

		MsgReply onMouse(MouseMsg const& msg) override
		{
			Vector2 wordPos = mScreenToWorld.transformPosition(msg.getPos());

			Vec2i tilePos;
			tilePos.x = Math::FloorToInt(wordPos.x);
			tilePos.y = Math::FloorToInt(wordPos.y);
			mHoverPos = tilePos;

			// Delegate all mouse events to current action
			if (mGame.mAction)
			{
				mGame.mAction->onMouse(mGame, msg);
			}

			if (msg.onLeftDown())
			{
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


	DEFINE_HOTRELOAD_CLASS(TestStage, StageBase);

	REGISTER_STAGE_ENTRY("SRPG", TestStage, EExecGroup::Dev);
}