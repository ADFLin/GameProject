#pragma once
#ifndef TFWRScript_H_C37FE583_CB9D_4263_952B_CBEB55566031
#define TFWRScript_H_C37FE583_CB9D_4263_952B_CBEB55566031

#include "Template/StringView.h"

#include "ReflectionCollect.h"
#include "InlineString.h"
#include "FileSystem.h"
#include "Meta/Concept.h"

#include "Lua/lua.h"
#include "Lua/lauxlib.h"
#include <cstring>
#include <memory>



namespace TFWR
{
	extern int GObjectCount;
	using ScriptHandle = lua_State*;

#define TFWR_SCRIPT_DIR "Script/TFWR"

	int Print(lua_State* L);

	template<typename T>
	using RemoveCVR = std::remove_reference_t< std::remove_cv_t< std::remove_pointer_t<T>>>;

	template<typename T>
	constexpr bool IsOutputValue = ((std::is_pointer_v<T> || std::is_reference_v<T>) && !std::is_const_v<T>) && (::Meta::IsPrimary< RemoveCVR<T> >::Value || std::is_same_v< RemoveCVR<T>, BoxingValue >);

	struct CReflectCollectable
	{
		template< typename T, typename Collector >
		static auto Requires(T& t, Collector& c) -> decltype
		(
			T::CollectReflection(c)
		);
	};

	template<typename T>
	constexpr bool IsRefectableObject = TCheckConcept< CReflectCollectable, T, ReflectionCollector>::Value;
	
	struct BoxingValue
	{
		BoxingValue()
		{
			type = None;
		}
		explicit BoxingValue(double value)
		{
			type = Number;
			fValue = value;
		}
		explicit BoxingValue(int64 value)
		{
			type = Int;
			iValue = value;
		}
		explicit BoxingValue(int value)
		{
			type = Int;
			iValue = value;
		}

		explicit BoxingValue(bool value)
		{
			type = Bool;
			iValue = value;
		}

		template< typename T >
		BoxingValue& operator = (T value)
		{
			this->operator = (BoxingValue(value));
			return *this;
		}

		void load(lua_State* L, int index)
		{
			if (lua_isinteger(L, index))
			{
				*this = lua_tointeger(L, index);
			}
			else if (lua_isnumber(L, index))
			{
				*this = lua_tonumber(L, index);
			}
			else if (lua_isboolean(L, index))
			{
				*this = (bool)lua_toboolean(L, index);
			}
			else if (lua_isuserdata(L, index))
			{
				type = UserData;
				pValue = lua_touserdata(L, index);
			}
			else if (lua_isnil(L, index))
			{
				type = None;
			}
		}

		void push(lua_State* L) const
		{
			switch (type)
			{
			case BoxingValue::Number:
				lua_pushnumber(L, fValue);
				break;
			case BoxingValue::Int:
				lua_pushinteger(L, iValue);
				break;
			case BoxingValue::Bool:
				lua_pushboolean(L, iValue);
				break;
			default:
				lua_pushnil(L);
				break;
			}
		}

		enum Type
		{
			None,
			Bool,
			Number,
			UserData,
			Int,
		};
		Type type;
		union 
		{
			lua_Number  fValue;
			lua_Integer iValue;
			void*       pValue;
		};
	};

	template< typename T >
	const char*& GetLuaMetaName()
	{
		static const char* s_name = nullptr;
		return s_name;
	}

	// userdata wrapper stored in Lua.
	// Supports both owned instances (created by Lua) and non-owned raw pointers (passed from C++).
	template< typename T >
	struct LuaUserData
	{
		std::unique_ptr<T> ownedInstance;
		T*                 rawPtr = nullptr;
	};

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

		template< typename T, typename TFunc >
		struct TMemberFuncCallable
		{
			TFunc funcPtr;
			T* obj;

			template< typename ...Ts >
			auto operator()(Ts&& ...ts)
			{
				CHECK(obj);
				return (obj->*funcPtr)(std::forward<Ts>(ts)...);
			}
		};

		template< typename RT, typename T, typename ...TArgs >
		static int MemberFunc(lua_State* L)
		{
			using MyFunc = RT(T::*)(TArgs...);
			TMemberFuncCallable<T, MyFunc> callable;
			callable.obj = LoadObjectPtr<T>(L, 1);
			callable.funcPtr = *reinterpret_cast<MyFunc*>(lua_touserdata(L, lua_upvalueindex(1)));
			return Invoke<0, RT, TArgs...>(L, callable, 2);
		}

		template< typename RT, typename T, typename ...TArgs >
		static int ConstMemberFunc(lua_State* L)
		{
			using MyFunc = RT(T::*)(TArgs...) const;
			TMemberFuncCallable<T, MyFunc> callable;
			callable.obj = LoadObjectPtr<T>(L, 1);
			callable.funcPtr = *reinterpret_cast<MyFunc*>(lua_touserdata(L, lua_upvalueindex(1)));
			return Invoke<0, RT, TArgs...>(L, callable, 2);
		}

		template< int NumUpValues, typename RT, typename ...TArgs, typename TCallable >
		static int Invoke(lua_State* L, TCallable callable, int index)
		{
			int numOutput = 0;
			if constexpr (sizeof...(TArgs) > 0)
			{
				std::tuple< std::remove_cv_t< std::remove_reference_t<TArgs>>... > args;
				LoadArgs< TArgs... >(L, args, index);
				if constexpr (std::is_same_v< RT, void >)
				{
					std::apply(callable, args);
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
			else if constexpr (std::is_pointer_v<T> && IsRefectableObject<std::remove_pointer_t<T>>)
			{
				if (lua_isnil(L, index))
				{
					arg = nullptr;
				}
				else
				{
					using UserDataT = LuaUserData<std::remove_pointer_t<T>>;
			
					// ≈Á√“ metatable
					if (lua_getmetatable(L, index) == 0)
					{
						arg = nullptr;
						return 1;
					}
					
					luaL_getmetatable(L, GetLuaMetaName<std::remove_pointer_t<T>>());
					bool ok = lua_rawequal(L, -1, -2);
					lua_pop(L, 2);
					
					if (!ok)
					{
						arg = nullptr;
						return 1;
					}
					
					UserDataT* ud = (UserDataT*)lua_touserdata(L, index);
					arg = ud ? ud->rawPtr : nullptr;
				}
			}
			else if constexpr (std::is_pointer_v<T> && !Meta::IsPrimary< std::remove_pointer_t<T> >::Value)
			{
				arg = (T)lua_touserdata(L, index);
			}
			else if constexpr (std::is_same_v<T, BoxingValue>)
			{
				arg.load(L, index);
			}
			else
			{
				arg = T();
			}

			return 1;
		}


		template< typename T >
		static LuaUserData<T>* ValidateAndGetUserData(lua_State* L, int index)
		{
			if (!lua_isuserdata(L, index))
				return nullptr;

			if (lua_getmetatable(L, index) == 0)
				return nullptr;

			// Stack: [..., userdata, ..., mt]

			luaL_getmetatable(L, GetLuaMetaName<T>()); // Stack: [..., mt, expected_mt]

			bool match = false;

			// Check loop
			int mtIndex = lua_gettop(L) - 1;
			int expectedIndex = lua_gettop(L);

			// Current mt is at -2 (mtIndex)
			// Expected is at -1 (expectedIndex)

			// We need to iterate on the metatable at -2
			// We will push successive base metatables on top of expected_mt, compare, and pop.

			lua_pushvalue(L, mtIndex); // Push start mt. Stack: [..., mt, expected, cur_mt]

			while (true)
			{
				if (lua_rawequal(L, -1, expectedIndex))
				{
					match = true;
					break;
				}

				// Get __base
				lua_getfield(L, -1, "__base"); // Stack: [..., mt, expected, cur_mt, base_mt_or_nil]

				if (lua_isnil(L, -1))
				{
					lua_pop(L, 2); // Pop nil and cur_mt
					break;
				}

				// Replace cur_mt with base_mt
				lua_remove(L, -2); // Stack: [..., mt, expected, base_mt]
			}

			lua_pop(L, 2); // Pop expected_mt and mt

			if (!match)
				return nullptr;

			LuaUserData<T>* ud = (LuaUserData<T>*)lua_touserdata(L, index);
			return (ud && ud->rawPtr) ? ud : nullptr;
		}

		template< typename T >
		static T* LoadObjectPtr(lua_State* L, int index)
		{
			if constexpr (IsRefectableObject<T>)
			{
				LuaUserData<T>* ud = ValidateAndGetUserData<T>(L, 1);
				if (ud == nullptr || ud->rawPtr == nullptr)
				{
					luaL_error(L, "Invalid object pointer");
				}
				return ud->rawPtr;
			}
			else
			{
				if (!lua_isuserdata(L, index))
				{
					return nullptr;
				}

				return (T*)lua_touserdata(L, index);
			}
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
			PushValuesImpl(L, std::forward<TValues>(values)...);
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
			using T = std::remove_const_t< std::remove_reference_t<Q>>;

			if constexpr (std::is_integral_v<T> || std::is_enum_v<T>)
			{
				lua_pushinteger(L, (lua_Integer)value);
			}
			else if constexpr (std::is_floating_point_v<T>)
			{
				lua_pushnumber(L, value);
			}
			else if constexpr (std::is_pointer_v<T> && IsRefectableObject<std::remove_pointer_t<T>>)
			{
				PushObject(L, value);
			}
			else if constexpr (std::is_pointer_v<T> && !Meta::IsPrimary< std::remove_pointer_t<T> >::Value)
			{
				lua_pushlightuserdata(L, value);
			}
			else if constexpr (std::is_same_v<T, BoxingValue>)
			{
				value.push(L);
			}
			else
			{
				lua_pushnil(L);
			}
		}

		template< typename T >
		static LuaUserData<T>* CreateLuaUserData(lua_State* L)
		{
			LuaUserData<T>* ud = (LuaUserData<T>*)lua_newuserdata(L, sizeof(LuaUserData<T>));
			new (ud) LuaUserData<T>();
			luaL_getmetatable(L, GetLuaMetaName<T>());
			lua_setmetatable(L, -2);
			return ud;
		}

		template< typename T >
		static void PushObject(lua_State* L, T* obj)
		{
			if (obj == nullptr)
			{
				lua_pushnil(L);
				return;
			}

			LuaUserData<T>* ud = CreateLuaUserData<T>(L);
			ud->rawPtr = obj;
		}
	};

	class LuaBindingCollector
	{
	public:
		LuaBindingCollector(lua_State* inL)
			: L(inL)
		{
		}
		template< typename T >
		static void Register(lua_State* L)
		{
			LuaBindingCollector collector(L);
			REF_COLLECT_TYPE(T, collector);
		}

		template< typename T >
		static const char*& LuaMetaName()
		{
			return GetLuaMetaName<T>();
		}

		template< typename T >
		void beginClass(char const* name)
		{
			// Create metatable for the class
			luaL_newmetatable(L, name); // metatable at top
			// store name for this type so other helpers can look it up
			LuaMetaName< T >() = name;

			// Create property getter/setter tables stored on metatable
			lua_newtable(L);
			lua_setfield(L, -2, "__propget");
			lua_newtable(L);
			lua_setfield(L, -2, "__propset");

			// Set __index metamethod to dispatcher
			// Upvalue 1: The metatable itself
			lua_pushvalue(L, -1);
			lua_pushcclosure(L, &LuaBindingCollector::IndexDispatcher, 1);
			lua_setfield(L, -2, "__index");

			// Set __newindex metamethod to dispatcher
			// Upvalue 1: The metatable itself
			lua_pushvalue(L, -1);
			lua_pushcclosure(L, &LuaBindingCollector::NewIndexDispatcher, 1);
			lua_setfield(L, -2, "__newindex");

			// Set __tostring to include class name
			lua_pushstring(L, name);
			lua_pushcclosure(L, &LuaBindingCollector::ToString, 1);
			lua_setfield(L, -2, "__tostring");

			// Set __gc to delete owned C++ instance when userdata is collected
			lua_pushcfunction(L, &LuaBindingCollector::GC<T>);
			lua_setfield(L, -2, "__gc");

			// Create a class table that will hold static-like members (e.g. 'new')
			lua_newtable(L); // class table

			// Create a table within the class table to store constructor overloads
			lua_newtable(L); // constructors table

			// Create the dispatcher closure for 'new'
			lua_pushvalue(L, -1); // copy constructors table to use as upvalue
			lua_pushcclosure(L, &LuaBindingCollector::ConstructorDispatcher<T>, 1);
			lua_setfield(L, -3, "new"); // class_table.new = dispatcher

			// Store the constructors table in the class table for addConstructor to find
			lua_setfield(L, -2, "__constructors");

			// Make the class table global
			lua_setglobal(L, name);

			if (std::is_default_constructible_v<T>)
			{
				addConstructor<T>();
			}
		}

		template< typename T, typename TBase >
		void addBaseClass()
		{
			// Assumes derived metatable is at the top of the stack
			const char* baseName = LuaMetaName<TBase>();
			if (!baseName)
			{
				return;
			}

			// Get base class metatable
			luaL_getmetatable(L, baseName); // base_mt at -1, derived_mt at -2

			// Store base metatable in __base field of derived metatable
			lua_pushvalue(L, -1);
			lua_setfield(L, -3, "__base");

			// Create new __index closure with derived_mt (uv1) and base_mt (uv2)
			lua_pushvalue(L, -2);
			lua_pushvalue(L, -2);
			lua_pushcclosure(L, &LuaBindingCollector::IndexDispatcherWithBase, 2);
			lua_setfield(L, -3, "__index"); // derived_mt.__index = closure

			// Create new __newindex closure with derived_mt (uv1) and base_mt (uv2)
			lua_pushvalue(L, -2);
			lua_pushvalue(L, -2);
			lua_pushcclosure(L, &LuaBindingCollector::NewIndexDispatcherWithBase, 2);
			lua_setfield(L, -3, "__newindex"); // derived_mt.__newindex = closure

			lua_pop(L, 1); // pop base_mt
		}

		template< typename T, typename ...TArgs >
		void addConstructor()
		{
			// Get the class table from global
			lua_getglobal(L, LuaMetaName<T>());
			// Get the constructors table from the class table
			lua_getfield(L, -1, "__constructors");

			if (lua_istable(L, -1))
			{
				// Create the CFunction for this specific constructor overload
				if constexpr (sizeof...(TArgs) == 0)
				{
					lua_pushcfunction(L, &LuaBindingCollector::ConstructFunc<T>);
				}
				else
				{
					lua_pushcclosure(L, &LuaBindingCollector::ConstructFuncWithArgs<T, TArgs...>, 0);
				}
				// Store the CFunction in the table, keyed by argument count
				lua_rawseti(L, -2, sizeof...(TArgs));
			}

			lua_pop(L, 2); // pop constructors table and class table
		}

		template< typename T, typename P >
		void addProperty(P(T::*memberPtr), char const* name)
		{
			// metatable must be at top
			// create userdata to hold member pointer

			using MemberPtr = P(T::*);
			// Use placement new for safety
			new (lua_newuserdata(L, sizeof(MemberPtr))) MemberPtr(memberPtr);

			// push getter closure with memberPtr as upvalue
			lua_pushcclosure(L, &LuaBindingCollector::GetProperty<T, P>, 1);
			// store in metatable.__propget[name]
			lua_getfield(L, -2, "__propget");
			lua_pushvalue(L, -2); // copy getter
			lua_setfield(L, -2, name);
			lua_pop(L, 1); // pop __propget
			lua_pop(L, 1); // pop getter

			// create setter closure (writable)
			new (lua_newuserdata(L, sizeof(MemberPtr))) MemberPtr(memberPtr);
			lua_pushcclosure(L, &LuaBindingCollector::SetProperty<T, P>, 1);
			// store in metatable.__propset[name]
			lua_getfield(L, -2, "__propset");
			lua_pushvalue(L, -2); // copy setter
			lua_setfield(L, -2, name);
			lua_pop(L, 1); // pop __propset
			lua_pop(L, 1); // pop setter
		}

		template< typename T, typename P, typename ...TMeta >
		void addProperty(P(T::*memberPtr), char const* name, TMeta&& ...meta)
		{
			addProperty<T, P>(memberPtr, name);
		}

		template< typename T, typename RT, typename ...TArgs >
		void addFunction(RT(T::*funcPtr)(TArgs...), char const* name)
		{
			using MyFunc = RT(T::*)(TArgs...);
			new (lua_newuserdata(L, sizeof(MyFunc))) MyFunc(funcPtr);
			lua_pushcclosure(L, &FLuaBinding::MemberFunc<RT, T, TArgs...>, 1);
			lua_setfield(L, -2, name);
		}

		template< typename T, typename RT, typename ...TArgs >
		void addFunction(RT(T::*funcPtr)(TArgs...) const, char const* name)
		{
			using MyFunc = RT(T::*)(TArgs...) const;
			new (lua_newuserdata(L, sizeof(MyFunc))) MyFunc(funcPtr);
			lua_pushcclosure(L, &FLuaBinding::ConstMemberFunc<RT, T, TArgs...>, 1);
			lua_setfield(L, -2, name);
		}

		template< typename T, typename RT, typename ...TArgs >
		void addFunction(RT(*funcPtr)(TArgs...), char const* name)
		{
			lua_pushlightuserdata(L, funcPtr);
			lua_pushcclosure(L, &FLuaBinding::StaticFunc<RT, T, TArgs...>, 1);
			lua_setfield(L, -2, name);
		}

		template< typename T, typename RT, typename ...TArgs, typename ...TMeta >
		void addFunction(RT(T::*funcPtr)(TArgs...), char const* name, TMeta&& ...meta)
		{
			addFunction<T, RT, TArgs...>(funcPtr, name);
		}

		template< typename T, typename RT, typename ...TArgs, typename ...TMeta >
		void addFunction(RT(T::*funcPtr)(TArgs...) const, char const* name, TMeta&& ...meta)
		{
			addFunction<T, RT, TArgs...>(funcPtr, name);
		}

		template< typename T, typename RT, typename ...TArgs, typename ...TMeta >
		void addFunction(RT(*funcPtr)(TArgs...), char const* name, TMeta&& ...meta)
		{
			addFunction<T, RT, TArgs...>(funcPtr, name);
		}

		void endClass()
		{
			// pop metatable from stack
			lua_pop(L, 1);
		}


	private:
		lua_State* L = nullptr;

		// __index dispatcher: looks for methods, then properties.
		static int IndexDispatcher(lua_State* L)
		{
			// stack: userdata, key
			// upvalue 1: metatable

			// Check for method in metatable
			lua_pushvalue(L, 2); // stack: userdata, key, key
			lua_rawget(L, lua_upvalueindex(1));   // stack: userdata, key, value
			if (!lua_isnil(L, -1))
			{
				return 1; // Found a method
			}
			lua_pop(L, 1); // pop nil

			// Check for property getter
			if (lua_getfield(L, lua_upvalueindex(1), "__propget") == LUA_TTABLE)
			{
				lua_pushvalue(L, 2); // key
				lua_rawget(L, -2);   // getter
				if (lua_isfunction(L, -1))
				{
					lua_pushvalue(L, 1); // self
					if (lua_pcall(L, 1, 1, 0) != LUA_OK)
					{
						return luaL_error(L, "%s", lua_tostring(L, -1));
					}
					return 1; // Return result of getter
				}
				lua_pop(L, 1); // pop nil/non-function
			}
			lua_pop(L, 1); // pop __propget table or nil

			lua_pushnil(L);
			return 1;
		}

		// __newindex dispatcher: looks for property setter.
		static int NewIndexDispatcher(lua_State* L)
		{
			// stack: userdata, key, value
			// upvalue 1: metatable

			if (lua_getfield(L, lua_upvalueindex(1), "__propset") == LUA_TTABLE)
			{
				lua_pushvalue(L, 2); // key
				lua_rawget(L, -2);   // setter
				if (lua_isfunction(L, -1))
				{
					lua_pushvalue(L, 1); // self
					lua_pushvalue(L, 3); // value
					if (lua_pcall(L, 2, 0, 0) != LUA_OK)
					{
						return luaL_error(L, "%s", lua_tostring(L, -1));
					}
					return 0; // Success
				}
				lua_pop(L, 1); // pop nil
			}
			lua_pop(L, 1); // pop __propset
			return luaL_error(L, "Property '%s' is read-only or does not exist", lua_tostring(L, 2));
		}

		static int IndexDispatcherWithBase(lua_State* L)
		{
			// stack: userdata, key
			// upvalue 1: derived metatable
			// upvalue 2: base metatable

			// Check for method in derived metatable
			lua_pushvalue(L, 2); // key
			lua_rawget(L, lua_upvalueindex(1));   // value
			if (!lua_isnil(L, -1))
			{
				return 1; // Found a method
			}
			lua_pop(L, 1); // pop nil

			// Check for property getter in derived
			if (lua_getfield(L, lua_upvalueindex(1), "__propget") == LUA_TTABLE)
			{
				lua_pushvalue(L, 2); // key
				lua_rawget(L, -2);   // getter
				if (lua_isfunction(L, -1))
				{
					lua_pushvalue(L, 1); // self
					if (lua_pcall(L, 1, 1, 0) != LUA_OK)
					{
						return luaL_error(L, "%s", lua_tostring(L, -1));
					}
					return 1; // Return result of getter
				}
				lua_pop(L, 1); // pop nil/non-function
			}
			lua_pop(L, 1); // pop __propget table or nil

			// If not found, delegate to base class
			lua_getfield(L, lua_upvalueindex(2), "__index");
			if (lua_isfunction(L, -1))
			{
				lua_pushvalue(L, 1); // self
				lua_pushvalue(L, 2); // key
				lua_call(L, 2, 1);
				return 1;
			}
			else if (lua_istable(L, -1))
			{
				lua_pushvalue(L, 2); // key
				lua_gettable(L, -2);
				return 1;
			}

			lua_pushnil(L);
			return 1;
		}

		static int NewIndexDispatcherWithBase(lua_State* L)
		{
			// stack: userdata, key, value
			// upvalue 1: derived metatable
			// upvalue 2: base metatable

			// Check for property setter in derived
			if (lua_getfield(L, lua_upvalueindex(1), "__propset") == LUA_TTABLE)
			{
				lua_pushvalue(L, 2); // key
				lua_rawget(L, -2);   // setter
				if (lua_isfunction(L, -1))
				{
					lua_pushvalue(L, 1); // self
					lua_pushvalue(L, 3); // value
					if (lua_pcall(L, 2, 0, 0) != LUA_OK)
					{
						return luaL_error(L, "%s", lua_tostring(L, -1));
					}
					return 0; // Success
				}
				lua_pop(L, 1); // pop nil
			}
			lua_pop(L, 1); // pop __propset table or nil

			// If not found, delegate to base class
			lua_getfield(L, lua_upvalueindex(2), "__newindex");
			if (lua_isfunction(L, -1))
			{
				lua_pushvalue(L, 1); // self
				lua_pushvalue(L, 2); // key
				lua_pushvalue(L, 3); // value
				lua_call(L, 3, 0);
				return 0;
			}
			else if (lua_istable(L, -1))
			{
				lua_pushvalue(L, 2); // key
				lua_pushvalue(L, 3); // value
				lua_settable(L, -3);
				return 0;
			}

			return luaL_error(L, "Property '%s' is read-only or does not exist", lua_tostring(L, 2));
		}

		static int ToString(lua_State* L)
		{
			const char* name = lua_tostring(L, lua_upvalueindex(1));
			lua_pushfstring(L, "<%s object>", name);
			return 1;
		}

		template< typename T >
		static int ConstructorDispatcher(lua_State* L)
		{
			int n_args = lua_gettop(L);
			// The constructor table is the first upvalue.
			lua_pushvalue(L, lua_upvalueindex(1));
			lua_rawgeti(L, -1, n_args); // get constructor function for n_args

			if (lua_isfunction(L, -1))
			{
				lua_remove(L, -2); // remove __constructors table
				lua_insert(L, 1); // move function to bottom
				if (lua_pcall(L, n_args, 1, 0) != LUA_OK)
				{
					return lua_error(L);
				}
				return 1;
			}

			return luaL_error(L, "No constructor for %s with %d arguments", LuaMetaName<T>(), n_args);
		}


		template< typename T, typename P >
		static int GetProperty(lua_State* L)
		{
			T* obj = FLuaBinding::LoadObjectPtr<T>(L, 1);
			using MemberPtr = P(T::*);
			MemberPtr memberPtr = *reinterpret_cast<MemberPtr*>(lua_touserdata(L, lua_upvalueindex(1)));
			auto& value = obj->*memberPtr;
			FLuaBinding::PushValue(L, value);
			return 1;
		}

		template< typename T, typename P >
		static int SetProperty(lua_State* L)
		{
			T* obj = FLuaBinding::LoadObjectPtr<T>(L, 1);
			using MemberPtr = P(T::*);
			MemberPtr memberPtr = *reinterpret_cast<MemberPtr*>(lua_touserdata(L, lua_upvalueindex(1)));
			P value{};
			if (!FLuaBinding::LoadArg<P>(L, value, 2))
			{
				return luaL_error(L, "Invalid value for property");
			}
			obj->*memberPtr = value;
			return 0;
		}

		template< typename T >
		static int ConstructFunc(lua_State* L)
		{
			LuaUserData<T>* ud = FLuaBinding::CreateLuaUserData<T>(L);
			ud->ownedInstance = std::make_unique<T>();
			ud->rawPtr = ud->ownedInstance.get();
			++GObjectCount;
			return 1;
		}

		template< typename T, typename ...TArgs >
		static int ConstructFuncWithArgs(lua_State* L)
		{
			std::tuple<std::remove_cv_t<std::remove_reference_t<TArgs>>...> args;
			FLuaBinding::LoadArgs<TArgs...>(L, args, 1);

			LuaUserData<T>* ud = FLuaBinding::CreateLuaUserData<T>(L);
			ud->ownedInstance = std::apply([](auto&&... args) {
				return std::make_unique<T>(std::forward<decltype(args)>(args)...);
			}, args);
			ud->rawPtr = ud->ownedInstance.get();
			++GObjectCount;
			return 1;
		}

		template< typename T >
		static int GC(lua_State* L)
		{
			LuaUserData<T>* ud = (LuaUserData<T>*)lua_touserdata(L, 1);
			// The unique_ptr's destructor is called automatically when 'ud' is collected.
			// We just need to explicitly call the destructor of LuaUserData itself
			// because it was created with placement new.
			if (ud)
			{
				ud->~LuaUserData();
				--GObjectCount;
			}
			return 0;
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