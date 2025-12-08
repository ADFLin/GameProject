#pragma once
#ifndef TFWRGameAPI_H
#define TFWRGameAPI_H

#include "TFWRScript.h"
#include "TFWRGameState.h"

namespace TFWR
{

	struct FGameAPI
	{
		static GameExecutionContext& GetContext()
		{
			return *static_cast<GameExecutionContext*>(IExecuteManager::GetCurrentContext());
		}

		static GameState& GetGame()
		{
			return *GetContext().game;
		}
		static Drone& GetDrone()
		{
			return *GetContext().drone;
		}

		static void Wait(int waitTicks);
		static int SetTickEnabled(lua_State* L);
		static void Error();
		static void till();
		static bool harvest();
		static bool can_harvest();
		static bool plant(Entity* entity);
		static int get_pos_x();
		static int get_pos_y();
		static int  get_world_size();
		static Entity* get_entity_type();
		static void clear();
		static void set_execution_speed(float speed);
		static bool move(EDirection direction);
		static bool can_move(EDirection direction);
		static EGround get_ground_type();
		static float num_items(EItem item);
		static float get_water();
		static bool use_item(EItem item, int nTimes);
		static bool swap(EDirection direction);
		static int num_unlocked(BoxingValue const& thing);
		static Entity* __Internal_get_companion(int& x, int& y);
		static int __Internal_measure(BoxingValue const& arg1, BoxingValue& v1, BoxingValue& v2);

		template< typename TEnum >
		static void RegisterEnum(ScriptHandle L, char const* space)
		{
			lua_newtable(L);
			for (ReflectEnumValueInfo const& value : REF_GET_ENUM_VALUES(TEnum))
			{
				lua_pushinteger(L, value.value);
				lua_setfield(L, -2, value.text);
			}
			lua_setglobal(L, space);
		}

		template< typename TEnum >
		static void RegisterEnumWithOffset(ScriptHandle L, char const* space, int offset)
		{
			lua_newtable(L);
			for (ReflectEnumValueInfo const& value : REF_GET_ENUM_VALUES(TEnum))
			{
				lua_pushinteger(L, value.value + offset);
				lua_setfield(L, -2, value.text);
			}
			lua_setglobal(L, space);
		}

		template< typename TEnum >
		static void RegisterEnum(ScriptHandle L)
		{
			lua_getglobal(L, "_G");
			for (ReflectEnumValueInfo const& value : REF_GET_ENUM_VALUES(TEnum))
			{
				lua_pushinteger(L, value.value);
				lua_setfield(L, -2, value.text);
			}
			lua_pop(L, 1);
		}

		template< typename TClass >
		class PropertyCollector
		{
		public:
			template< typename T >
			void beginClass(char const* name) {}

			template< typename T, typename P >
			void addProperty(P(T::* memberPtr), char const* name)
			{
				lua_pushlightuserdata(L, &(mPtr->*memberPtr));
				lua_setfield(L, -2, name);
			}

			template< typename T, typename P, typename ...TMeta >
			void addProperty(P(T::* memberPtr), char const* name, TMeta&& ...meta)
			{
				addProperty(memberPtr, name);
			}

			void endClass()
			{
			}

			ScriptHandle L;
			TClass* mPtr;
		};

		template< typename TClass >
		static void RegisterGlobal(ScriptHandle L, char const* name, TClass& value)
		{
			lua_newtable(L);
			PropertyCollector<TClass> collector;
			collector.mPtr = &value;
			collector.L = L;
			TClass::CollectReflection(collector);
			lua_setglobal(L, name);
		}

		static void RegisterFunc(lua_State* L);
		static void RegisterRawFunc(lua_State* L);
		static lua_State* CreateGameState();
	};

} // namespace TFWR

#endif // TFWRGameAPI_H
