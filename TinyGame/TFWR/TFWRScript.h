#pragma once
#ifndef TFWRScript_H_C37FE583_CB9D_4263_952B_CBEB55566031
#define TFWRScript_H_C37FE583_CB9D_4263_952B_CBEB55566031

#include "Template/StringView.h"

#include "LogSystem.h"
#include "HashString.h"
#include "InlineString.h"
#include "FileSystem.h"

#include <unordered_set>

#include "Lua/lua.h"

struct lua_State;

namespace TFWR
{
	using ScriptHandle = lua_State*;

#define TFWR_SCRIPT_DIR "Script/TFWR"

	int Print(lua_State* L);

	template<typename T>
	constexpr bool IsOutputValue = ((std::is_pointer_v<T> || std::is_reference_v<T>) && !std::is_const_v<T>) && (::Meta::IsPrimary< std::remove_reference_t< std::remove_cv_t< std::remove_pointer_t<T>>> >::Value);

	struct FLuaBinding
	{
		template< typename RT, typename ...TArgs >
		static void Register(lua_State* L, char const* name, RT(*funcPtr)(TArgs...))
		{
			lua_getglobal(L, "_G");
			lua_pushlightuserdata(L, funcPtr);
			lua_pushcclosure(L, &StaticFunc< RT, TArgs...>, 1);
			lua_setfield(L, -2, name);
			lua_pop(L, 1);
		}


		template< typename RT, typename ...TArgs, typename ...TUpValues >
		static void Register(lua_State* L, char const* name, RT(*funcPtr)(TArgs...), TUpValues ...upValuses)
		{
			lua_getglobal(L, "_G");
			lua_pushlightuserdata(L, funcPtr);
			lua_pushcclosure(L, &StaticFunc< RT, TArgs...>, 1);
			PushValues(L, std::forward<TUpValues>(upValues)...);
			lua_setfield(L, -(2 + sizeof...(TUpValues)), name);
			lua_pop(L, 1);
		}

		template< typename RT, typename ...TArgs >
		static int StaticFunc(lua_State* L)
		{
			using MyFunc = RT(*)(TArgs...);
			MyFunc funcPtr = (MyFunc)lua_touserdata(L, lua_upvalueindex(1));
			return Invoke<0, RT, TArgs...>(L, funcPtr, 1);
		}

		template< typename RT, typename T, typename ...TArgs >
		struct TMemberFuncCallable
		{
			using MyFunc = RT(T::*)(TArgs...);
			MyFunc funcPtr;
			T* obj;

			template< typename ...Ts >
			auto operator()(Ts&& ...ts)
			{
				if (obj == nullptr)
					return RT();

				return (obj->*funcPtr)(std::forward<Ts>(ts)...);
			}
		};

		template< typename RT, typename T, typename ...TArgs >
		static int MemberFunc(lua_State* L)
		{
			using MyFunc = RT(T::*)(TArgs...);
			TMemberFuncCallable callable;
			callable.obj = LoadObjectPtr(L, 1);
			callable.funcPtr = (MyFunc)lua_touserdata(L, lua_upvalueindex(1));
			return Invoke<0, RT, TArgs...>(L, callable, 2);
		}

		template< int NumUpValues, typename RT, typename ...TArgs, typename TCallable >
		static int Invoke(lua_State* L, TCallable callable, int index)
		{
			int numOutput = 0;
			if constexpr (sizeof...(TArgs) > 0)
			{
				std::tuple< std::remove_reference_t< std::remove_cv_t<TArgs>>... > args;
				LoadArgs< TArgs... >(L, args, index);
				if constexpr (std::is_same_v< RT, void >)
				{
					std::apply(funcPtr, args);
				}
				else
				{
					RT result = std::apply(callable, args);
					PushValue(L, result);
					numOutput += 1;
				}
				numOutput += PushArgOutputs< TArgs... >(L, args);
			}
			else
			{
				if constexpr (std::is_same_v< RT, void >)
				{
					callable();
				}
				else
				{
					RT result = callable();
					PushValue(L, result);
					numOutput += 1;
				}
			}

			return numOutput;
		}

		template< typename ...TArgs, typename T >
		static void LoadArgs(lua_State* L, T& args, int index = 1)
		{
			LoadArgsImpl< 0, TArgs... >(L, args, index);
		}

		template< int ArgIndex, typename TArg, typename ...TArgs, typename T>
		static void LoadArgsImpl(lua_State* L, T& args, int index)
		{
			index += LoadArg< TArg >(L, std::get<ArgIndex>(args), index);
			LoadArgsImpl< ArgIndex + 1, TArgs...>(L, args, index);
		}

		template< int ArgIndex, typename T>
		static void LoadArgsImpl(lua_State* L, T& args, int index)
		{
		}

		template< typename TArg, typename T >
		static int LoadArg(lua_State* L, T& arg, int index)
		{
			if constexpr (IsOutputValue<TArg>)
			{
				return 0;
			}

			if constexpr (std::is_integral_v<T>)
			{
				arg = lua_tointeger(L, index);
			}
			else if constexpr (std::is_enum_v<T>)
			{
				arg = (T)lua_tointeger(L, index);
			}
			else if constexpr (std::is_floating_point_v<T>)
			{
				arg = lua_tonumber(L, index);
			}
			else if constexpr (std::is_pointer_v<T> && !Meta::IsPrimary< std::remove_pointer_t<T> >::Value)
			{
				arg = (T)lua_touserdata(L, index);
			}
			else
			{
				arg = T();
			}

			return 1;
		}

		template< typename T >
		static T* LoadObjectPtr(lua_State* L, int index)
		{
			if (!lua_isuserdata(L, index))
			{
				return nullptr;
			}

			return (T*)lua_touserdata(L, index);
		}

		template< typename ...TArgs, typename T>
		static int PushArgOutputs(lua_State* L, T& args)
		{
			return PushArgOutputsImpl<0, TArgs...>(L, args);
		}

		template< int ArgIndex, typename TArg, typename ...TArgs, typename T>
		static int PushArgOutputsImpl(lua_State* L, T& args)
		{
			int numOutput = PushArgOutput< TArg >(L, std::get<ArgIndex>(args));
			numOutput += PushArgOutputsImpl<ArgIndex + 1, TArgs...>(L, args);
			return numOutput;
		}

		template< int ArgIndex, typename T>
		static int PushArgOutputsImpl(lua_State* L, T& args)
		{
			return 0;
		}


		template< typename TArg, typename T >
		static int PushArgOutput(lua_State* L, T const& arg)
		{
			if constexpr (!IsOutputValue<TArg>)
			{
				return 0;
			}

			PushValue(L, arg);
			return 1;
		}


		template< typename ...TValues>
		static void PushValues(lua_State* L, TValues&& ...values)
		{
			PushValuesImpl(L, std::forward<T>(values)...);
		}

		template< typename T, typename ...TValues >
		static void PushValuesImpl(lua_State* L, T&& value, TValues&& ...values)
		{
			PushValue(L, std::forward<T>(value));
			PushValuesImpl(L, std::forward<T>(values)...);
		}

		template< typename T>
		static void PushValuesImpl(lua_State* L, T& value)
		{
			PushValue(L, std::forward<T>(value));
		}

		template< typename Q >
		static void PushValue(lua_State* L, Q&& value)
		{
			using T = std::remove_reference_t<Q>;

			if constexpr (std::is_integral_v<T> || std::is_enum_v<T>)
			{
				lua_pushinteger(L, (lua_Integer)value);
			}
			else if constexpr (std::is_floating_point_v<T>)
			{
				lua_pushnumber(L, value);
			}
			else if constexpr (std::is_pointer_v<T> && !Meta::IsPrimary< std::remove_pointer_t<T> >::Value)
			{
				lua_pushlightuserdata(L, value);
			}
			else
			{
				lua_pushnil(L);
			}
		}

	};

	class LuaBindingCollector
	{
		template< typename T >
		void beginClass(char const* name)
		{


		}

		template< typename T, typename TBase >
		void addBaseClass()
		{


		}

		template< typename T, typename P >
		void addProperty(P(T::*memberPtr), char const* name)
		{



		}

		template< typename T, typename P, typename ...TMeta >
		void addProperty(P(T::*memberPtr), char const* name, TMeta&& ...meta)
		{

		}

		void endClass()
		{



		}

	};

	class IExecuteHookListener
	{
	public:
		virtual void onHookLine(StringView const& fileName, int line) {}
		virtual void onHookCall() {}
		virtual void onHookReturn() {}
	};

	class ScriptFile;

	struct ExecutableObject
	{
		enum EState
		{
			ePause = BIT(0),
			eExecuting = BIT(2),
			eStep = BIT(3),
		};

		void clearExecution()
		{
			fTicksAcc = 0;
			line = 0;
			executeL = nullptr;
			stateFlags = 0;
			executionCode = nullptr;
		}

		ScriptFile* executionCode;
		ScriptHandle executeL;
		float fTicksAcc;
		int line;
		uint32 stateFlags;
	};


	struct ExecutionContext
	{
		ScriptHandle mainL;
		ExecutableObject* object;
	};

	class ScriptFile
	{
	public:
		std::string name;
		std::string code;

		InlineString<> getPath()
		{
			InlineString<> path;
			path.format(TFWR_SCRIPT_DIR"/%s.lua", name.c_str());
			return path;
		}

		bool loadCode()
		{
			return FFileUtility::LoadToString(getPath().c_str(), code);
		}
	};
	class IExecuteManager
	{
	public:
		static IExecuteManager& Get();
		static ExecutionContext* GetCurrentContext();

		virtual ~IExecuteManager() = default;

		virtual ScriptHandle createThread(ScriptHandle masterL, char const* fileName) = 0;

		bool bTickEnabled = true;
		IExecuteHookListener* mHookListener = nullptr;

		enum EExecuteStatus
		{
			Ok,
			Error,
			Completed,
		};

		virtual EExecuteStatus execute(ExecutionContext& context) = 0;
		virtual void stop(ExecutionContext& context) = 0;
	};

}//namespace TFWR


#endif // TFWRScript_H_C37FE583_CB9D_4263_952B_CBEB55566031