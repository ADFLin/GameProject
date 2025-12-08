#include "TFWRGameAPI.h"

#include "TFWREntities.h"
#include "TFWRGameState.h"

#include "LogSystem.h"
#include "Core/StringConv.h"

#include "Lua/lauxlib.h"
#include "Lua/lualib.h"

namespace TFWR
{
	void FGameAPI::Wait(int waitTicks)
	{
		GetContext().object->fTicksAcc -= waitTicks;
	}

	int FGameAPI::SetTickEnabled(lua_State* L)
	{
		IExecuteManager::Get().bTickEnabled = lua_toboolean(L, 1);
		return 0;
	}

	void FGameAPI::Error()
	{
		luaL_error(GetContext().object->executeL, "Exec Error");
	}

	void FGameAPI::till()
	{
		GetGame().till(GetDrone());
	}
	
	bool FGameAPI::harvest()
	{
		return GetGame().harvest(GetDrone());
	}

	bool FGameAPI::can_harvest()
	{
		Wait(1);
		return GetGame().canHarvest(GetDrone());
	}

	bool FGameAPI::plant(Entity* entity)
	{
		bool bSuccess = false;
		if (entity)
		{
			bSuccess = GetGame().plant(GetDrone(), *entity);
		}
		return bSuccess;
	}

	int FGameAPI::get_pos_x()
	{
		Wait(1);
		return GetDrone().pos.x;
	}

	int FGameAPI::get_pos_y()
	{
		Wait(1);
		return GetDrone().pos.y;
	}

	int  FGameAPI::get_world_size()
	{
		Wait(1);
		return GetGame().getGameLogic().getTiles().getSizeX();
	}

	Entity* FGameAPI::get_entity_type()
	{
		Wait(1);
		return GetGame().getGameLogic().getPlantEntity(GetDrone().pos);
	}

	void FGameAPI::clear()
	{
		GetGame().clear();
		Wait(200);
	}

	void FGameAPI::set_execution_speed(float speed)
	{
		GetGame().getGameLogic().setMaxSpeed(speed);
		Wait(1);
	}

	bool FGameAPI::move(EDirection direction)
	{
		return GetGame().move(GetDrone(), direction);
	}

	bool FGameAPI::can_move(EDirection direction)
	{
		Wait(1);
		return GetGame().getGameLogic().canMove(GetDrone().pos, direction);
	}

	EGround FGameAPI::get_ground_type()
	{
		Wait(1);
		return GetGame().getGameLogic().getGroundType(GetDrone().pos);
	}

	float FGameAPI::num_items(EItem item)
	{
		Wait(1);
		return GetGame().getGameLogic().getItemNum(item);
	}

	float FGameAPI::get_water()
	{
		Wait(1);
		return GetGame().getGameLogic().getTile(GetDrone().pos).waterValue;
	}

	bool FGameAPI::use_item(EItem item, int nTimes)
	{
		bool result = GetGame().useItem(GetDrone().pos, item, nTimes);
		Wait(result ? 200 : 1);
		return result;
	}

	bool FGameAPI::swap(EDirection direction)
	{
		bool bSuccess = GetGame().swapPlant(GetDrone().pos, direction);
		Wait(bSuccess ? 200 : 1);
		return bSuccess;
	}

	// Type masks for num_unlocked to distinguish different enum types
	constexpr int TypeMask_Unlock = 0x00000000;
	constexpr int TypeMask_Ground = 0x10000000;
	constexpr int TypeMask_Item = 0x20000000;
	constexpr int TypeMask_Mask = 0xF0000000;
	constexpr int TypeMask_Value = ~TypeMask_Mask;

	int FGameAPI::num_unlocked(BoxingValue const& thing)
	{
		Wait(1);
		auto& gameLogic = GetGame().getGameLogic();
		
		int result = 0;
		
		// Check if argument is userdata (Entity*)
		if (thing.type == BoxingValue::UserData)
		{
			Entity* entity = (Entity*)thing.pValue;
			if (entity)
			{
				result = gameLogic.getUnlockLevel(*entity);
			}
		}
		else if (thing.type == BoxingValue::Int)
		{
			int rawValue = thing.iValue;
			int type = rawValue & TypeMask_Mask;
			int value = rawValue & TypeMask_Value;

			switch (type)
			{
			case TypeMask_Unlock:
				if (value >= 0 && value < EUnlock::COUNT)
				{
					result = gameLogic.getUnlockLevel((EUnlock::Type)value);
				}
				break;
			case TypeMask_Item:
				if (value >= 0 && value < (int)EItem::COUNT)
				{
					result = gameLogic.getUnlockLevel((EItem)value);
				}
				break;
			case TypeMask_Ground:
				if (value >= 0 && value < (int)EGround::COUNT)
				{
					result = gameLogic.getUnlockLevel((EGround)value);
				}
				break;
			}
		}

		return result;
	}

	Entity* FGameAPI::__Internal_get_companion(int& x, int& y)
	{
		Wait(1);
		Vec2i pos;
		auto entity = GetGame().getGameLogic().getCompanion(GetDrone().pos, pos);
		if (entity)
		{
			x = pos.x;
			y = pos.y;
		}
		return entity;
	}

	int FGameAPI::__Internal_measure(BoxingValue const& arg1, BoxingValue& v1, BoxingValue& v2)
	{
		Wait(1);
		Vec2i pos = GetDrone().pos;
		if (arg1.type == BoxingValue::Int)
		{
			if (0 <= arg1.iValue && arg1.iValue < 4)
			{
				pos += GDirectionOffset[arg1.iValue];
			}
			else
			{
				Error();
				return 0;
			}
		}
		if (!GetGame().getGameLogic().getTiles().checkRange(pos))
			return 0;

		BoxingValue* outputs[] = { &v1, &v2 };
		int numReturn = GetGame().getGameLogic().measure(pos, outputs);
		return numReturn;
	}

	void FGameAPI::RegisterFunc(lua_State* L)
	{
#define REGISTER(FUNC_NAME)\
	FLuaBinding::Register(L, #FUNC_NAME, FUNC_NAME)
		REGISTER(__Internal_get_companion);
		REGISTER(__Internal_measure);
		REGISTER(move);
		REGISTER(can_move);
		REGISTER(till);
		REGISTER(harvest);
		REGISTER(can_harvest);
		REGISTER(plant);
		REGISTER(get_pos_x);
		REGISTER(get_pos_y);
		REGISTER(get_world_size);
		REGISTER(get_entity_type);
		REGISTER(get_ground_type);
		REGISTER(get_water);
		REGISTER(use_item);
		REGISTER(swap);
		REGISTER(clear);
		REGISTER(num_unlocked);
#undef REGISTER
	}

	void FGameAPI::RegisterRawFunc(lua_State* L)
	{
		luaL_Reg CustomLib[] =
		{
			{"print", Print},
			{"_SetTickEnabled", SetTickEnabled},
			{NULL, NULL} /* end of array */
		};

		lua_getglobal(L, "_G");
		luaL_setfuncs(L, CustomLib, 0);
		lua_pop(L, 1);
	}


	lua_State* FGameAPI::CreateGameState()
	{
		lua_State* L = luaL_newstate();
		luaL_openlibs(L);
		RegisterEnum< EDirection >(L);
		RegisterEnum< EUnlock::Type >(L, "Unlocks");
		RegisterEnumWithOffset< EGround >(L, "Grounds", TypeMask_Ground);
		RegisterEnumWithOffset< EItem >(L, "Items", TypeMask_Item);
		RegisterFunc(L);
		RegisterGlobal(L, "Entities", GEntities);
		RegisterRawFunc(L);


		int state = luaL_dofile(L, TFWR_SCRIPT_DIR"/Main.lua");
		if (state != 0)
		{
			LogWarning(0, "Error(Main.lua): %s", lua_tostring(L, -1));
		}
		return L;
	}

} // namespace TFWR
