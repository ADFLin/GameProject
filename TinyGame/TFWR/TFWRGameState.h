#pragma once
#ifndef TFWRScriptedGameState_H_9D4E5F6A_7B8C_9D0E_1F2A_3B4C5D6E7F8A
#define TFWRScriptedGameState_H_9D4E5F6A_7B8C_9D0E_1F2A_3B4C5D6E7F8A

#include "TFWRGameLogic.h"
#include "TFWRScript.h"
#include <optional>

namespace TFWR
{
	// Drone with script execution capability
	class Drone : public ExecutableObject
		        , public DroneState
	{
	public:
		Drone()
		{
			reset();
			clearExecution();
		}

		void reset()
		{
			DroneState::reset();
		}
	};

	// Forward declaration
	class GameState;

	// Execution context for scripted game
	struct GameExecutionContext : ExecutionContext
	{
		GameExecutionContext()
		{
		}

		~GameExecutionContext()
		{
		}

		GameState* game = nullptr;
		Drone* drone = nullptr;
	};

	class GameState
	{
	public:
		GameState();
		~GameState();

		void init(int size);
		void resizeMap(int size);
		void reset();
		void clear();
		void release();

		// Script execution management
		void runExecution(Drone& drone, ScriptFile& codeFile);
		void stopExecution(Drone& drone);
		bool isExecutingCode();

		// Update
		void update(float deltaTime);
		void update(Drone& drone, UpdateArgs const& updateArgs);

		// Game operations - apply tick costs after calling GameLogic
		void till(Drone& drone);
		bool harvest(Drone& drone);
		bool canHarvest(Drone& drone);
		bool plant(Drone& drone, Entity& entity);
		bool move(Drone& drone, EDirection direction);
		bool useItem(Vec2i const& pos, EItem item, int nTimes);
		bool swapPlant(Vec2i const& pos, EDirection direction);

		// Direct access to game logic
		GameLogic& getGameLogic() { return mGameLogic; }
		GameLogic const& getGameLogic() const { return mGameLogic; }

		// Drone management
		Drone* createDrone(ScriptFile& file);
		TArray<Drone*> const& getDrones() const { return mDrones; }

		// Lua state access
		lua_State* getMainLuaState() { return mMainL; }

		// Currently executing drone (for script context)
		Drone* getExecutingDrone() { return mExecutingDrone; }

		std::optional<std::string> getVariableValue(ScriptFile* codeFile, StringView varName);

	private:
		void applyTickCost(Drone& drone, int tickCost);

		GameLogic mGameLogic;
		lua_State* mMainL = nullptr;
		TArray<Drone*> mDrones;
		Drone* mExecutingDrone = nullptr;
	};

} // namespace TFWR

#endif // TFWRScriptedGameState_H_9D4E5F6A_7B8C_9D0E_1F2A_3B4C5D6E7F8A
