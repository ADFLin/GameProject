#include "TFWRGameState.h"

#include "TFWRGameAPI.h"
#include "TFWREntities.h"
#include "LuaInspector.h"

namespace TFWR
{
	GameState::GameState()
	{
	}

	GameState::~GameState()
	{
		release();
	}

	void GameState::init(int size)
	{
		mMainL = FGameAPI::CreateGameState();
		mGameLogic.init(size);
	}

	void GameState::resizeMap(int size)
	{
		mGameLogic.resizeMap(size);
	}

	void GameState::reset()
	{
		mGameLogic.reset();
	}

	void GameState::clear()
	{
		mGameLogic.clear();
		for (auto drone : mDrones)
		{
			drone->reset();
			if (mExecutingDrone != drone)
			{
				stopExecution(*drone);
			}
		}

		if (mDrones.size() > 0 && mDrones[0] != mExecutingDrone)
		{
			mExecutingDrone = nullptr;
		}
	}

	void GameState::release()
	{
		if (mMainL)
		{
			lua_close(mMainL);
			mMainL = nullptr;
		}

		for (auto drone : mDrones)
		{
			delete drone;
		}
		mDrones.clear();
	}

	void GameState::runExecution(Drone& drone, ScriptFile& codeFile)
	{
		if (drone.executeL)
		{
			stopExecution(drone);
		}

		drone.executeL = IExecuteManager::Get().createThread(mMainL, codeFile.name.c_str());
		drone.executionCode = &codeFile;
	}

	void GameState::stopExecution(Drone& drone)
	{
		if (drone.executeL == nullptr)
			return;

		ExecutionContext context;
		context.mainL = mMainL;
		context.object = &drone;
		IExecuteManager::Get().stop(context);
	}

	bool GameState::isExecutingCode()
	{
		bool bHadExecution = false;
		for (Drone* drone : mDrones)
		{
			if (drone->executeL)
			{
				if (drone->stateFlags & ExecutableObject::ePause)
					return false;

				bHadExecution = true;
			}
		}
		return bHadExecution;
	}

	void GameState::update(float deltaTime)
	{
		if (!isExecutingCode())
			return;

		UpdateArgs updateArgs;
		updateArgs.speed = 1.0f;
		
		// Apply power speed boost from game logic
		if (mGameLogic.getItemNum(EItem::Power) > 0)
		{
			updateArgs.speed *= 2.0f;
		}
		
		updateArgs.fTicks = updateArgs.speed * deltaTime * 400; // TicksPerSecond
		updateArgs.deltaTime = updateArgs.speed * deltaTime;

		// Update game logic (handles resource generation, tile updates)
		mGameLogic.update(deltaTime);

		// Update drones
		for (Drone* drone : mDrones)
		{
			update(*drone, updateArgs);
		}
	}

	void GameState::update(Drone& drone, UpdateArgs const& updateArgs)
	{
		if (drone.executeL == nullptr)
		{
			return;
		}

		drone.fTicksAcc += updateArgs.fTicks;
		if (drone.fTicksAcc <= 1.0)
			return;

		GameExecutionContext context;
		context.drone = &drone;
		context.game = this;
		context.mainL = mMainL;
		context.object = &drone;

		mExecutingDrone = &drone;
		IExecuteManager::Get().execute(context);
		if (mExecutingDrone == nullptr)
		{
			stopExecution(drone);
		}
		else
		{
			mExecutingDrone = nullptr;
		}
	}

	Drone* GameState::createDrone(ScriptFile& file)
	{
		Drone* drone = new Drone;
		drone->reset();
		mDrones.push_back(drone);
		return drone;
	}

	void GameState::applyTickCost(Drone& drone, int tickCost)
	{
		drone.fTicksAcc -= tickCost;
	}

	void GameState::till(Drone& drone)
	{
		auto result = mGameLogic.doTill(drone);
		applyTickCost(drone, result.tickCost);
	}

	bool GameState::harvest(Drone& drone)
	{
		auto result = mGameLogic.doHarvest(drone);
		applyTickCost(drone, result.tickCost);
		return result.success;
	}

	bool GameState::canHarvest(Drone& drone)
	{
		return mGameLogic.canHarvest(drone);
	}

	bool GameState::plant(Drone& drone, Entity& entity)
	{
		auto result = mGameLogic.doPlant(drone, entity);
		applyTickCost(drone, result.tickCost);
		return result.success;
	}

	bool GameState::move(Drone& drone, EDirection direction)
	{
		auto result = mGameLogic.doMove(drone, direction);
		applyTickCost(drone, result.tickCost);
		return result.success;
	}

	bool GameState::useItem(Vec2i const& pos, EItem item, int nTimes)
	{
		auto result = mGameLogic.doUseItem(pos, item, nTimes);
		// Note: tick cost is applied in the API layer for this operation
		return result.success;
	}


	bool GameState::swapPlant(Vec2i const& pos, EDirection direction)
	{
		auto result = mGameLogic.doSwapPlant(pos, direction);
		// Note: tick cost is applied in the API layer for this operation
		return result.success;
	}

	std::optional<std::string> GameState::getVariableValue(ScriptFile* codeFile, StringView varName)
	{
		if (!mMainL)
			return std::nullopt;

		lua_State* targetL = mMainL;
		bool bFoundInStack = false;

		if (codeFile)
		{
			for (Drone* drone : mDrones)
			{
				if (drone->executionCode == codeFile && drone->executeL)
				{
					targetL = drone->executeL;
					
					// 1. Search Locals
					std::string result;
					bool found = false;
					LuaInspector::EnumerateLocals(targetL, 0, [&](const char* name, const std::string& value)
					{
						if (varName == name)
						{
							result = value;
							found = true;
							return false;
						}
						return true;
					});

					if (found)
						return result;

					// 2. Search Upvalues
					// We need to find the function on stack to inspect upvalues?
					// LuaInspector::EnumerateUpvalues expects a function index on stack.
					// For the running script, the function might be at a specific stack level?
					// Usually the current function is involved in the current stack frame.
					// But EnumerateUpvalues takes a stack index.
					// We can try to get the function from the debug info of level 0.
					lua_Debug ar;
					if (lua_getstack(targetL, 0, &ar))
					{
						if (lua_getinfo(targetL, "f", &ar)) // pushes function onto stack
						{
							LuaInspector::EnumerateUpvalues(targetL, -1, [&](const char* name, const std::string& value)
							{
								if (varName == name)
								{
									result = value;
									found = true;
									return false;
								}
								return true;
							});
							lua_pop(targetL, 1); // pop function
						}
					}

					if (found)
						return result;

					break; // Only check the first matching drone? Or should we check all? Assuming one instance per code file for now or just take the first one.
				}
			}
		}

		// 3. Search Globals
		// Use LuaInspector::EnumerateGlobals or just direct lookup since we have the name
		lua_getglobal(targetL, varName.data());
		
		std::string valueStr;
		int type = lua_type(targetL, -1);
		if (type != LUA_TNIL)
		{
			valueStr = LuaInspector::ValueToString(targetL, -1);
		}
		else
		{
			lua_pop(targetL, 1);
			return std::nullopt;
		}

		lua_pop(targetL, 1); // Pop the value from the stack
		return valueStr;
	}

} // namespace TFWR
