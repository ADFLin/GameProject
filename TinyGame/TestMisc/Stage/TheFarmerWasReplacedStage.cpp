
#include "Stage/TestStageHeader.h"
#include "GameRenderSetup.h"

#include "TFPRCore.h"

#include "RHI/RHICommand.h"
#include "RHI/DrawUtility.h"
#include "RHI/GpuProfiler.h"
#include "RHI/RHIGraphics2D.h"

#include "Core/StringConv.h"
#include "LogSystem.h"

#include "ReflectionCollect.h"

#include "Lua/lua.h"
#include "Lua/lauxlib.h"
#include "Lua/lualib.h"
#include "Lua/lstate.h"
#include "Lua/lopcodes.h"

#include "DataStructure/Grid2D.h"
#include "Math/TVector2.h"
#include "Math/GeometryPrimitive.h"

#include "Meta/MetaBase.h"
#include "RandomUtility.h"
#include "AssetViewer.h"
#include "FileSystem.h"
#include "StringParse.h"
#include "Lua/ldebug.h"
#include <memory>


namespace TFWR
{
	typedef TVector2<int> Vec2i;

	using namespace Render;
	int Print(lua_State* L)
	{
		int nargs = lua_gettop(L);

		std::string output;
		for (int i = 1; i <= nargs; ++i) 
		{
			if (i != 1)
			{
				output += " ";
			}
			if (lua_isstring(L, i)) 
			{
				char const* str = lua_tostring(L, i);
				output += str;
			}
			else if(lua_isinteger(L, i))
			{
				output += FStringConv::From(lua_tointeger(L, i));
			}
		}

		LogMsg(output.c_str());
		return 0;
	}

	int TestFunc(float a, int b, float& c)
	{
		c = 2 * a + b;
		return a + b;
	}

	template<typename T>
	constexpr bool IsOutputValue = (std::is_pointer_v<T> || std::is_reference_v<T>) && !std::is_const_v<T> && (::Meta::IsPrimary< std::remove_reference_t< std::remove_cv_t<T>> >::Value);

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

		template< typename ...TArgs , typename T >
		static void LoadArgs(lua_State* L, T& args, int index = 1)
		{
			LoadArgsImpl< 0, TArgs... >(L, args, index);
		}

		template< int ArgIndex , typename TArg, typename ...TArgs, typename T>
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

			if constexpr (std::is_integral_v<T> || std::is_enum_v<T> )
			{
				lua_pushinteger(L, value);
			}
			else if constexpr (std::is_floating_point_v<T>)
			{
				lua_pushnumber(L, value);
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

	struct TestData
	{

		int a;
		float b;


		REFLECT_STRUCT_BEGIN(TestData)
			REF_PROPERTY(a)
			REF_PROPERTY(b)
		REFLECT_STRUCT_END()
	};

	template <typename T> 
	class Lunar
	{
		typedef struct { T *pT; } userdataType;
	public:
		typedef int (T::*mfp)(lua_State* L);
		typedef struct { const char *name; mfp mfunc; } RegType;

		static void Register(lua_State* L)
		{
			lua_newtable(L);
			int methods = lua_gettop(L);

			luaL_newmetatable(L, T::className);
			int metatable = lua_gettop(L);

			// store method table in globals so that
			// scripts can add functions written in Lua.
			lua_pushvalue(L, methods);
			set(L, LUA_GLOBALSINDEX, T::className);

			// hide metatable from Lua getmetatable()
			lua_pushvalue(L, methods);
			set(L, metatable, "__metatable");

			lua_pushvalue(L, methods);
			set(L, metatable, "__index");

			lua_pushcfunction(L, tostring_T);
			set(L, metatable, "__tostring");

			lua_pushcfunction(L, gc_T);
			set(L, metatable, "__gc");

			lua_newtable(L);                // mt for method table
			lua_pushcfunction(L, new_T);
			lua_pushvalue(L, -1);           // dup new_T function
			set(L, methods, "new");         // add new_T to method table
			set(L, -3, "__call");           // mt.__call = new_T
			lua_setmetatable(L, methods);

			// fill method table with methods from class T
			for (RegType *l = T::methods; l->name; l++) 
			{
				lua_pushstring(L, l->name);
				lua_pushlightuserdata(L, (void*)l);
				lua_pushcclosure(L, thunk, 1);
				lua_settable(L, methods);
			}

			lua_pop(L, 2);  // drop metatable and method table
		}

		// call named lua method from userdata method table
		static int call(lua_State* L, const char *method,
			int nargs = 0, int nresults = LUA_MULTRET, int errfunc = 0)
		{
			int base = lua_gettop(L) - nargs;  // userdata index
			if (!luaL_checkudata(L, base, T::className))
			{
				lua_settop(L, base - 1);           // drop userdata and args
				lua_pushfstring(L, "not a valid %s userdata", T::className);
				return -1;
			}

			lua_pushstring(L, method);         // method name
			lua_gettable(L, base);             // get method from userdata
			if (lua_isnil(L, -1)) 
			{            // no method?
				lua_settop(L, base - 1);           // drop userdata and args
				lua_pushfstring(L, "%s missing method '%s'", T::className, method);
				return -1;
			}
			lua_insert(L, base);               // put method under userdata, args

			int status = lua_pcall(L, 1 + nargs, nresults, errfunc);  // call method
			if (status) 
			{
				const char *msg = lua_tostring(L, -1);
				if (msg == NULL) msg = "(error with no message)";
				lua_pushfstring(L, "%s:%s status = %d\n%s",
					T::className, method, status, msg);
				lua_remove(L, base);             // remove old message
				return -1;
			}
			return lua_gettop(L) - base + 1;   // number of results
		}

		// push onto the Lua stack a userdata containing a pointer to T object
		static int push(lua_State* L, T *obj, bool gc = false)
		{
			if (!obj) { lua_pushnil(L); return 0; }
			luaL_getmetatable(L, T::className);  // lookup metatable in Lua registry
			if (lua_isnil(L, -1)) luaL_error(L, "%s missing metatable", T::className);
			int mt = lua_gettop(L);
			subtable(L, mt, "userdata", "v");
			userdataType *ud =
				static_cast<userdataType*>(pushuserdata(L, obj, sizeof(userdataType)));
			if (ud) 
			{
				ud->pT = obj;  // store pointer to object in userdata
				lua_pushvalue(L, mt);
				lua_setmetatable(L, -2);
				if (gc == false) 
				{
					lua_checkstack(L, 3);
					subtable(L, mt, "do not trash", "k");
					lua_pushvalue(L, -2);
					lua_pushboolean(L, 1);
					lua_settable(L, -3);
					lua_pop(L, 1);
				}
			}
			lua_replace(L, mt);
			lua_settop(L, mt);
			return mt;  // index of userdata containing pointer to T object
		}

		// get userdata from Lua stack and return pointer to T object
		static T *check(lua_State* L, int narg)
		{
			userdataType *ud =
				static_cast<userdataType*>(luaL_checkudata(L, narg, T::className));
			if (!ud)
			{
				luaL_typerror(L, narg, T::className);
				return NULL;
			}
			return ud->pT;  // pointer to T object
		}

	private:
		Lunar();  // hide default constructor

		// member function dispatcher
		static int thunk(lua_State* L)
		{
			// stack has userdata, followed by method args
			T *obj = check(L, 1);  // get 'self', or if you prefer, 'this'
			lua_remove(L, 1);  // remove self so member function args start at index 1
			// get member function from upvalue
			RegType *l = static_cast<RegType*>(lua_touserdata(L, lua_upvalueindex(1)));
			return (obj->*(l->mfunc))(L);  // call member function
		}

		// create a new T object and
		// push onto the Lua stack a userdata containing a pointer to T object
		static int new_T(lua_State* L)
		{
			lua_remove(L, 1);   // use classname:new(), instead of classname.new()
			T *obj = new T(L);  // call constructor for T objects
			push(L, obj, true); // gc_T will delete this object
			return 1;           // userdata containing pointer to T object
		}

		// garbage collection metamethod
		static int gc_T(lua_State* L)
		{
			if (luaL_getmetafield(L, 1, "do not trash")) 
			{
				lua_pushvalue(L, 1);  // dup userdata
				lua_gettable(L, -2);
				if (!lua_isnil(L, -1)) return 0;  // do not delete object
			}
			userdataType *ud = static_cast<userdataType*>(lua_touserdata(L, 1));
			T *obj = ud->pT;
			if (obj) delete obj;  // call destructor for T objects
			return 0;
		}

		static int tostring_T(lua_State* L)
		{
			char buff[32];
			userdataType *ud = static_cast<userdataType*>(lua_touserdata(L, 1));
			T *obj = ud->pT;
			sprintf(buff, "%p", (void*)obj);
			lua_pushfstring(L, "%s (%s)", T::className, buff);

			return 1;
		}

		static void set(lua_State* L, int table_index, const char *key)
		{
#if 0
			lua_setfield(L, table_index key);
#else
			lua_pushstring(L, key);
			lua_insert(L, -2);  // swap value and key
			lua_settable(L, table_index);
#endif
		}

		static void weaktable(lua_State* L, const char *mode)
		{
			lua_newtable(L);
			lua_pushvalue(L, -1);  // table is its own metatable
			lua_setmetatable(L, -2);
			lua_pushliteral(L, "__mode");
			lua_pushstring(L, mode);
			lua_settable(L, -3);   // metatable.__mode = mode
		}

		static void subtable(lua_State* L, int tindex, const char *name, const char *mode)
		{
			lua_pushstring(L, name);
			lua_gettable(L, tindex);
			if (lua_isnil(L, -1)) 
			{
				lua_pop(L, 1);
				lua_checkstack(L, 3);
				weaktable(L, mode);
				lua_pushstring(L, name);
				lua_pushvalue(L, -2);
				lua_settable(L, tindex);
			}
		}

		static void *pushuserdata(lua_State* L, void *key, size_t sz)
		{
			void *ud = 0;
			lua_pushlightuserdata(L, key);
			lua_gettable(L, -2);     // lookup[key]
			if (lua_isnil(L, -1)) {
				lua_pop(L, 1);         // drop nil
				lua_checkstack(L, 3);
				ud = lua_newuserdata(L, sz);  // create new userdata
				lua_pushlightuserdata(L, key);
				lua_pushvalue(L, -2);  // dup userdata
				lua_settable(L, -4);   // lookup[key] = userdata
			}
			return ud;
		}
	};

	class CodeFile
	{
	public:
		std::string name;
		std::string code;

		InlineString<> getPath()
		{
			InlineString<> path;
			path.format("TFWR/%s.lua", name.c_str());
			return path;
		}

		bool loadCode()
		{
			return FFileUtility::LoadToString(getPath().c_str(), code);
		}
	};

	using ScriptHandle = lua_State*;

#define OP_CODE_LIST(op)\
	op(OP_MOVE)\
	op(OP_LOADI)\
	op(OP_LOADF)\
	op(OP_LOADK)\
	op(OP_LOADKX)\
	op(OP_LOADFALSE)\
	op(OP_LFALSESKIP)\
	op(OP_LOADTRUE)\
	op(OP_LOADNIL)\
	op(OP_GETUPVAL)\
	op(OP_SETUPVAL)\
	op(OP_GETTABUP)\
	op(OP_GETTABLE)\
	op(OP_GETI)\
	op(OP_GETFIELD)\
	op(OP_SETTABUP)\
	op(OP_SETTABLE)\
	op(OP_SETI)\
	op(OP_SETFIELD)\
	op(OP_NEWTABLE)\
	op(OP_SELF)\
	op(OP_ADDI)\
	op(OP_ADDK)\
	op(OP_SUBK)\
	op(OP_MULK)\
	op(OP_MODK)\
	op(OP_POWK)\
	op(OP_DIVK)\
	op(OP_IDIVK)\
	op(OP_BANDK)\
	op(OP_BORK)\
	op(OP_BXORK)\
	op(OP_SHRI)\
	op(OP_SHLI)\
	op(OP_ADD)\
	op(OP_SUB)\
	op(OP_MUL)\
	op(OP_MOD)\
	op(OP_POW)\
	op(OP_DIV)\
	op(OP_IDIV)\
	op(OP_BAND)\
	op(OP_BOR)\
	op(OP_BXOR)\
	op(OP_SHL)\
	op(OP_SHR)\
	op(OP_MMBIN)\
	op(OP_MMBINI)\
	op(OP_MMBINK)\
	op(OP_UNM)\
	op(OP_BNOT)\
	op(OP_NOT)\
	op(OP_LEN)\
	op(OP_CONCAT)\
	op(OP_CLOSE)\
	op(OP_TBC)\
	op(OP_JMP)\
	op(OP_EQ)\
	op(OP_LT)\
	op(OP_LE)\
	op(OP_EQK)\
	op(OP_EQI)\
	op(OP_LTI)\
	op(OP_LEI)\
	op(OP_GTI)\
	op(OP_GEI)\
	op(OP_TEST)\
	op(OP_TESTSET)\
	op(OP_CALL)\
	op(OP_TAILCALL)\
	op(OP_RETURN)\
	op(OP_RETURN0)\
	op(OP_RETURN1)\
	op(OP_FORLOOP)\
	op(OP_FORPREP)\
	op(OP_TFORPREP)\
	op(OP_TFORCALL)\
	op(OP_TFORLOOP)\
	op(OP_SETLIST)\
	op(OP_CLOSURE)\
	op(OP_VARARG)\
	op(OP_VARARGPREP)\
	op(OP_EXTRAARG)\



	class IExecuteHookListener
	{
	public:
		virtual void onHookLine(StringView const& fileName, int line){}
		virtual void onHookCall() {}
		virtual void onHookReturn(){}
	};


	struct ExecutionContext
	{
		lua_State* mainL;
		ExecutableObject* object;
	};

	class ExecuteManager
	{
	public:
		static ExecuteManager& Get()
		{
			static ExecuteManager StaticInstance;
			return StaticInstance;
		}

		bool bTickEnabled = true;
		int  mOpTicks[1 << SIZE_OP];
		IExecuteHookListener* mHookListener = nullptr;

		ExecuteManager()
		{
			std::fill_n(mOpTicks, ARRAY_SIZE(mOpTicks), 0);
			std::fill_n(mOpTicks, OP_EXTRAARG + 1, 1);
			mOpTicks[OP_CALL] = 0;
			mOpTicks[OP_TAILCALL] = 0;
			mOpTicks[OP_TFORCALL] = 0;
			mOpTicks[OP_EXTRAARG] = 0;
		}

		enum EExecuteStatus
		{
			Ok,
			Error,
			Completed,
		};

		EExecuteStatus execute(ExecutionContext& context)
		{
			ExecutableObject& execObject = *context.object;

			int nres;
			execObject.stateFlags |= ExecutableObject::eExecuting;
			int status = lua_resume(execObject.executeL, context.mainL, 0, &nres);
			execObject.stateFlags &= ~ExecutableObject::eExecuting;

			if (status == LUA_YIELD)
			{
				lua_pop(execObject.executeL, nres);
				return EExecuteStatus::Ok;
			}
			else if (status == LUA_OK)
			{
				onExecutionCompleted(execObject);
				lua_pop(execObject.executeL, nres);

				execObject.reset();
				return EExecuteStatus::Completed;
			}
			else
			{
				onExecutionError(execObject, lua_tostring(execObject.executeL, -1));
				execObject.reset();
				return EExecuteStatus::Error;
			}
		}

		static void StopHook(lua_State* L, lua_Debug* ar)
		{
			luaL_error(L, "Stop Execution");
		}


		void onExecutionCompleted(ExecutableObject& execObject)
		{

		}
		void onExecutionError(ExecutableObject& execObject, char const* error)
		{
			LogWarning(0, "Handle Lua error : %s", error);
		}


		void onHookLine(StringView fileName, int line)
		{
			mHookListener->onHookLine(fileName, line);
		}

		void stop(ExecutionContext& context)
		{
			ExecutableObject& execObject = *context.object;
			if (execObject.executeL == nullptr)
				return;

			lua_sethook(execObject.executeL, StopHook, LUA_MASKCOUNT, 1);

			int nres;
			execObject.stateFlags |= ExecutableObject::eExecuting;
			int status = lua_resume(execObject.executeL, context.mainL, 0, &nres);
			execObject.stateFlags &= ~ExecutableObject::eExecuting;
			execObject.reset();
		}


		struct BreakPoint
		{
			HashString fileName;
			int line;

			bool operator == (BreakPoint const& rhs)
			{
				return fileName == rhs.fileName && line == rhs.line;
			}

			uint32 getTypeHash() const
			{
				uint32 result = HashValues(fileName, line);
				return result;
			}
		};

		std::unordered_set<BreakPoint, MemberFuncHasher > mBreakpoints;

		void hookExecution(ExecutionContext& context, lua_State* L, lua_Debug *ar)
		{
			auto& execObject = *context.object;

			if (ar->event == LUA_HOOKLINE)
			{
#if 1
				printf("-Line- ");
				if (ar->name)
				{
					printf("Name: %s, ", ar->name);
				}
				if (ar->source)
				{
					printf("Source: %s, Line: %d\n", ar->source, ar->currentline);
				}
				else
				{
					printf("Unknown source.\n");
				}
#endif

				if (ar->source[0] != '@' || FCString::CompareN(ar->source, "@TFWR/Main.lua", 14) == 0)
				{
					return;
				}



				if (execObject.stateFlags & ExecutableObject::eStep)
				{
					execObject.stateFlags &= ~ExecutableObject::eStep;
					execObject.stateFlags |= ExecutableObject::ePause;

					execObject.line = ar->currentline;
					auto fileName = StringView(ar->source + 1, ar->srclen - 1);
					onHookLine(fileName, ar->currentline);

					lua_yield(L, 0);
					return;
				}
				return;
			}
			else if (ar->event == LUA_HOOKCALL || ar->event == LUA_HOOKRET)
			{
				lua_getinfo(L, "nSl", ar); // Get name and source information
#if 1
				printf("(Func) ");
				if (ar->name)
				{
					printf("Name: %s, ", ar->name);
				}
				if (ar->source)
				{
					printf("Source: %s, Line: %d\n", ar->source, ar->currentline);
				}
				else
				{
					printf("Unknown source.\n");
				}
#endif

				if (FCString::CompareN(ar->source, "@TFWR/Main.lua", 14) == 0)
				{
					bTickEnabled = ar->event != LUA_HOOKCALL;
				}

				return;
			}

			lua_getinfo(L, "nSl", ar);

#if 1
			printf("[Count] ");
			if (ar->name)
			{
				printf("Name: %s, ", ar->name);
			}
			if (ar->source)
			{
				printf("Source: %s, Line: %d\n", ar->source, ar->currentline);
			}
			else
			{
				printf("Unknown source.\n");
			}
#endif


			const Instruction* pc = L->ci->u.l.savedpc;
			int op = GET_OPCODE(*(pc - 1));

			if (bTickEnabled)
			{
				if (execObject.fTicksAcc < mOpTicks[op])
				{
					lua_yield(L, 0);
					return;
				}

				execObject.fTicksAcc -= mOpTicks[op];
				if (ar->source[0] == '@')
				{
					execObject.line = ar->currentline;
					auto fileName = StringView(ar->source + 1, ar->srclen - 1);
					onHookLine(fileName, ar->currentline);
				}
			}


#if 0
#define CASE_OP(op) case op: LogMsg(#op); break;
			switch (op)
			{
				OP_CODE_LIST(CASE_OP)
			}
#endif
		}


	};


	class GameState;
	struct MapTile;

	static Vec2i constexpr GDirectionOffset[] =
	{
		Vec2i(1,0),
		Vec2i(-1,0),
		Vec2i(0,1),
		Vec2i(0,-1),
	};

	constexpr int TicksPerSecond = 400;

	struct TimeRange
	{
		float min;
		float max;
	};
	TimeRange PlantGrowthTimeMap[EPlant::COUNT] =
	{
		{ 0.5 , 0.5 },
		{ 3.2 , 4.8 },
		{ 4.8 , 7.2 },
		{ 5.6 , 8.4 },
		{ 0.2 , 3.8 },
		{ 0.9 , 1.1 },
		{ 4.0 , 6.0 },
		{ 0.18 , 0.22 },
	};

	BasePlantEntity::BasePlantEntity(EPlant::Type plant, EItem production) 
		:plant(plant), production(production)
	{

	}
	void BasePlantEntity::grow(MapTile& tile, GameState& game, UpdateArgs const& updateArgs)
	{
		if (tile.growTime == 1.0)
			return;

		auto growTimeRange = PlantGrowthTimeMap[plant];
		float growTime = RandFloat(growTimeRange.min, growTimeRange.max);
		tile.growValue += updateArgs.speed * updateArgs.deltaTime / growTime;
		if (tile.growValue >= 1.0)
		{
			tile.growValue = 1.0;
		}
	}

	void BasePlantEntity::harvest(MapTile& tile, GameState& game)
	{
		if (tile.growValue < 1.0)
			return;

		game.addItem(production, 5);
	}
	std::string BasePlantEntity::getDebugInfo(MapTile const& tile)
	{
		return InlineString<>::Make("%d %f", plant, tile.growValue);
	}

	void TreePlantEntity::grow(MapTile& tile, GameState& game, UpdateArgs const& updateArgs)
	{
		if (tile.growTime == 1.0)
			return;

		int numNeighborTree = 0;
		for (int dir = 0; dir < 4; ++dir)
		{
			Vec2i nPos = game.getPos(tile) + GDirectionOffset[dir];
			if (game.mTiles.checkRange(nPos))
			{
				auto const& nTile = game.getTile(nPos);
				if (nTile.plant == tile.plant)
				{
					++numNeighborTree;
				}
			}
		}

		auto growTimeRange = PlantGrowthTimeMap[plant];
		float growTime = RandFloat(growTimeRange.min, growTimeRange.max);
		tile.growValue += updateArgs.speed * updateArgs.deltaTime / (growTime * (numNeighborTree + 1));
		if (tile.growValue >= 1.0)
		{
			tile.growValue = 1.0;
		}
	}

	void AreaPlantEntity::grow(MapTile& tile, GameState& game, UpdateArgs const& updateArgs)
	{
		if (tile.growTime == 1.0)
			return;

		auto growTimeRange = PlantGrowthTimeMap[plant];
		float growTime = RandFloat(growTimeRange.min, growTimeRange.max);
		tile.growValue += updateArgs.speed * updateArgs.deltaTime / growTime;
		if (tile.growValue >= 1.0)
		{
			tile.growValue = 1.0;
			tile.meta = 0;
			game.tryMergeArea(game.getPos(tile), mMaxSize, this);
		}
	}

	struct EntityLibrary
	{
		EntityLibrary()
			:Grass(EPlant::Grass, EItem::Hay)
			,Bush(EPlant::Bush, EItem::Wood)
			,Tree(EPlant::Tree, EItem::Wood)
			,Pumpkin(EPlant::Pumpkin, EItem::Pumpkin)

		{



		}

		void registerScript(ScriptHandle L)
		{
			lua_newtable(L);
#define REGISTER(NAME)\
	lua_pushlightuserdata(L, &NAME);\
	lua_setfield(L, -2, #NAME)

			REGISTER(Grass);
			REGISTER(Bush);
			REGISTER(Tree);
			REGISTER(Pumpkin);
			lua_setglobal(L, "Entities");
		}
		SimplePlantEntity Grass;
		SimplePlantEntity Bush;
		TreePlantEntity Tree;
		AreaPlantEntity Pumpkin;
	};

	EntityLibrary GEntities;


	struct WorldExecuteContext;

	WorldExecuteContext* GExecContext = nullptr;
	struct WorldExecuteContext : ExecutionContext
	{
		WorldExecuteContext()
		{
			prevContext = GExecContext;
			GExecContext = this;
		}

		~WorldExecuteContext()
		{
			GExecContext = prevContext;
		}


		GameState* game;
		Drone* drone;
		WorldExecuteContext* prevContext;
	};


	struct FGameAPI
	{
		static void Wait(int waitTicks)
		{
			GExecContext->object->fTicksAcc -= waitTicks;
		}

		static int SetTickEnabled(lua_State* L)
		{
			ExecuteManager::Get().bTickEnabled = lua_toboolean(L, 1);
			return 0;
		}

		static void Error()
		{
			luaL_error(GExecContext->object->executeL, "Exec Error");
		}

		static void HookEntry(lua_State* L, lua_Debug *ar)
		{
			ExecuteManager::Get().hookExecution(*GExecContext, L, ar);
		}

		static void till()
		{
			GExecContext->game->till(*GExecContext->drone);
			Wait(200);
		}
		static bool harvest()
		{
			bool bSuccess = GExecContext->game->harvest(*GExecContext->drone);
			Wait(bSuccess ? 200 : 1);
			return bSuccess;
		}

		static bool can_harvest()
		{
			Wait(1);
			return GExecContext->game->canHarvest(*GExecContext->drone);
		}

		static bool plant(Entity* entity)
		{
			bool bSuccess = false;
			if (entity)
			{
				bSuccess = GExecContext->game->plant(*GExecContext->drone, *entity);
			}

			Wait(bSuccess ? 200 : 1);
			return bSuccess;
		}

		static int get_pos_x()
		{
			Wait(1);
			return GExecContext->drone->pos.x;
		}

		static int get_pos_y()
		{
			Wait(1);
			return GExecContext->drone->pos.y;
		}

		static int  get_world_size()
		{
			Wait(1);
			return GExecContext->game->mTiles.getSizeX();
		}

		static Entity* get_entity_type()
		{
			Wait(1);
			return GExecContext->game->getPlantEntity(*GExecContext->drone);
		}

		static void set_execution_speed(float speed)
		{
			GExecContext->game->mMaxSpeed = speed;
			Wait(1);
		}

		//static bool move(EDirection direction)
		static bool move(int direction)
		{
			bool bSuccess = GExecContext->game->move(*GExecContext->drone, (EDirection)direction);
			Wait(bSuccess ? 200 : 1);
			return bSuccess;
		}

		static void Register(lua_State* L)
		{
#define REGISTER(FUNC_NAME)\
	FLuaBinding::Register(L, #FUNC_NAME, FUNC_NAME)


			REGISTER(move);
			REGISTER(till);
			REGISTER(harvest);
			REGISTER(can_harvest);
			REGISTER(plant);
			REGISTER(get_pos_x);
			REGISTER(get_pos_y);
			REGISTER(get_world_size);
			REGISTER(get_entity_type);

#undef REGISTER
		}

		static void LoadLib(lua_State* L)
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
		static lua_State* CreateGameState()
		{
			lua_State* L = luaL_newstate();
			luaL_openlibs(L);
			luaL_dofile(L, "TFWR/Main.lua");
			LoadLib(L);
			Register(L);
			GEntities.registerScript(L);
			FLuaBinding::Register(L, "TestFunc", TestFunc);

			return L;
		}

		static lua_State* CreateExecuteState(lua_State* gameL, char const* fileName)
		{
			InlineString<> path;
			path.format("TFWR/%s.lua", fileName);
			lua_State* L = lua_newthread(gameL);
			lua_sethook(L, HookEntry, LUA_MASKCOUNT | LUA_MASKLINE | LUA_MASKCALL | LUA_MASKRET, 1);
			luaL_loadfile(L, path.c_str());
			return L;
		}
	};


	GameState::GameState()
	{
		mItems.resize(EItem::COUNT, 0);
		mUnlockLevels.resize(EUnlock::COUNT, 0);
	}

	void GameState::init()
	{
		mMainL = FGameAPI::CreateGameState();
		mTiles.resize(3, 3);
		reset();
	}

	void GameState::runExecution(Drone& drone, CodeFile& codeFile)
	{
		if (drone.executeL)
		{
			stopExecution(drone);
		}

		drone.executeL = FGameAPI::CreateExecuteState(mMainL, codeFile.name.c_str());
		drone.executionCode = &codeFile;
	}

	void GameState::stopExecution(Drone& drone)
	{
		if (drone.executeL == nullptr)
			return;

		ExecutionContext context;
		context.mainL = mMainL;
		context.object = &drone;
		ExecuteManager::Get().stop(context);
	}

	Drone* GameState::createDrone(CodeFile& file)
	{
		Drone* drone = new Drone;
		drone->reset();
		mDrones.push_back(drone);
		return drone;
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

	void GameState::release()
	{
		if (mMainL)
		{
			lua_close(mMainL);
		}

		for (auto drone : mDrones)
		{
			delete drone;
		}
		mDrones.clear();
	}

	Vec2i GameState::getPos(MapTile const& tile) const
	{
		int index = &tile - mTiles.getRawData();
		Vec2i pos;
		mTiles.toCoord(index, pos.x, pos.y);
		return pos;
	}

	void GameState::reset()
	{
		for (auto& tile : mTiles)
		{
			tile.reset();
		}

		for (auto drone : mDrones)
		{
			drone->reset();
		}

		mAreas.clear();
		mFreeAreaIndex = INDEX_NONE;
	}

	void GameState::update(float deltaTime)
	{
		if (!isExecutingCode())
			return;

		UpdateArgs updateArgs;
		updateArgs.speed = 1.0f;
		updateArgs.fTicks = updateArgs.speed * deltaTime * TicksPerSecond;
		updateArgs.deltaTime = deltaTime;

		for (Drone* drone : mDrones)
		{
			update(*drone, updateArgs);
		}
		for (auto& tile : mTiles)
		{
			update(tile, updateArgs);
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

		WorldExecuteContext context;
		context.drone = &drone;
		context.game = this;
		context.mainL = mMainL;
		context.object = &drone;

		ExecuteManager::Get().execute(context);
	}

	void GameState::update(MapTile& tile, UpdateArgs const& updateArgs)
	{
		if (tile.plant == nullptr && tile.ground == EGround::Grassland)
		{
			tile.plant = &GEntities.Grass;
		}
		if (tile.plant)
		{
			tile.plant->grow(tile, *this, updateArgs);
		}
	}

	bool GameState::isUnlocked(EUnlock::Type unlock)
	{
		return mUnlockLevels[unlock] > 0;
	}

	void GameState::till(Drone& drone)
	{
		auto& tile = getTile(drone.pos);
	}

	bool GameState::harvest(Drone& drone)
	{
		auto& tile = getTile(drone.pos);
		if (tile.plant == nullptr)
			return false;

		tile.plant->harvest(tile, *this);
		tile.growValue = 0.0;
		tile.plant = nullptr;
		return true;
	}

	bool GameState::canHarvest(Drone& drone)
	{
		auto& tile = getTile(drone.pos);
		if (tile.plant == nullptr)
			return false;

		if (tile.growValue < 1.0)
			return false;

		return true;
	}

	bool GameState::plant(Drone& drone, Entity& entity)
	{
		auto& tile = getTile(drone.pos);
		if (tile.plant)
		{
			if (tile.plant != &GEntities.Grass)
				return false;
		}

		tile.plant = &entity;
		tile.growValue = 0.0;
		tile.plant->plant(tile, *this);
		return true;
	}

	bool GameState::move(Drone& drone, EDirection direction)
	{
		static Vec2i const moveOffset[] =
		{
			Vec2i(1,0),
			Vec2i(-1,0),
			Vec2i(0,1),
			Vec2i(0,-1),
		};

		Vec2i pos = drone.pos + moveOffset[direction];
		switch (direction)
		{
		case East:
			if (pos.x >= mTiles.getSizeX())
				pos.x = 0;
			break;
		case West:
			if (pos.x < 0)
				pos.x = mTiles.getSizeX() - 1;
			break;
		case North:
			if (pos.y >= mTiles.getSizeY())
				pos.y = 0;
			break;
		case South:
			if (pos.y < 0)
				pos.y = mTiles.getSizeY() - 1;
			break;
		}
		drone.pos = pos;
		return true;
	}

	int GameState::tryMergeArea(Vec2i const& pos, int maxSize, Entity* entity)
	{
		int result = 0;
		for (int size = 2; size <= maxSize; ++size)
		{
			auto const& tile = getTile(pos);

			Area area;
			if (tile.meta > 0)
			{
				area = mAreas[tile.meta - 1];
			}
			else
			{
				area.min = pos;
				area.max = pos + Vec2i(1, 1);
			}

			if (area.getSize().x >= maxSize)
				continue;

			for (int dir = 0; dir < 4; ++dir)
			{
				int id = tryMergeArea(area, size, dir, entity);
				if (id > 0)
				{
					result = id;
					break;
				}
			}
		}
		return result;
	}

	int GameState::tryMergeArea(Area const& areaToMerge, int size, int dir, Entity* entity)
	{
		static constexpr Vec2i dirOffset[] =
		{
			Vec2i(1,1), Vec2i(-1,1), Vec2i(1,-1), Vec2i(-1,-1),
		};


		Vec2i pos;
		pos.x = dirOffset[dir].x > 0 ? areaToMerge.min.x : areaToMerge.max.x;
		pos.y = dirOffset[dir].y > 0 ? areaToMerge.min.y : areaToMerge.max.y;

		Area testArea;
		testArea.min = pos.min(pos + size * dirOffset[dir]);
		testArea.max = pos.max(pos + size * dirOffset[dir]);

		for (int oy = 0; oy < size; ++oy)
		{
			for (int ox = 0; ox < size; ++ox)
			{
				auto const& tile = getTile(pos + Vec2i(ox, oy) * dirOffset[dir]);
				if (tile.plant != entity)
					return 0;

				if (tile.meta == -1)
					return 0;

				if (tile.meta > 0)
				{
					auto const& area = mAreas[tile.meta - 1];
					if (!testArea.contain(area))
					{
						return 0;
					}
				}
			}
		}

		int id = addArea(testArea);
		for (int oy = 0; oy < size; ++oy)
		{
			for (int ox = 0; ox < size; ++ox)
			{
				auto& tile = getTile(pos + Vec2i(ox, oy) * dirOffset[dir]);
				if (tile.meta > 0 && tile.meta != id)
				{
					removeArea(tile.meta - 1);
				}
				tile.meta = id + 1;
			}
		}
		return id + 1;
	}

	class CodeEditor;

	class CodeFileAsset : public CodeFile, public IAssetViewer
	{
	public:
		CodeEditor* editor;
		int id;
	};

	class ExecButton : public GButtonBase
	{
		typedef GButtonBase BaseClass;
	public:
		ExecButton(int id, Vec2i const& pos, Vec2i const& size, GWidget* parent)
			:BaseClass(id, pos, size, parent)
		{
			mID = id;
		}

		void onRender()
		{
			Vec2i pos = getWorldPos();
			Vec2i size = getSize();
			RHIGraphics2D& g = Global::GetRHIGraphics2D();

			g.enablePen(false);
			g.setBrush(Color3ub(96, 115, 13));
			g.drawRoundRect(pos, size, Vector2(4, 4));

			g.pushXForm();
			g.translateXForm(pos.x, pos.y);
			Vector2 vertices[] =
			{
				Vector2(size.x / 4, size.y / 4),
				Vector2(3 * size.x / 4, size.y / 2),
				Vector2(size.x / 4, 3 * size.y / 4),
			};

			RenderUtility::SetPen(g, EColor::White);
			RenderUtility::SetBrush(g, EColor::White);
			g.enablePen(true);
			g.enableBrush(!bStep);
			g.drawPolygon(vertices, ARRAY_SIZE(vertices));
			g.enableBrush(true);
			g.popXForm();
		}
		bool bStep = false;
	};


	class IViewModel
	{
	public:
		virtual bool isExecutingCode() { return false; }
		virtual void runExecution(CodeFileAsset& codeFile){}
		virtual void stopExecution(CodeFileAsset& codeFile) {}
		virtual void pauseExecution(CodeFileAsset& codeFile) {}
		virtual void runStepExecution(CodeFileAsset& codeFile) {}
		static IViewModel& Get()
		{
			return *Instance;
		}
		static IViewModel*Instance;
	};

	IViewModel* IViewModel::Instance = nullptr;

	class CodeEditor : public GFrame
	{
	public:
		using BaseClass = GFrame;
		enum 
		{
			UI_EXEC = BaseClass::NEXT_UI_ID,
			UI_EXEC_STEP,

			NEXT_UI_ID,
		};
		CodeEditor(int id, Vec2i const& pos, Vec2i const& size, GWidget* parent)
			:GFrame(id, pos, size, parent)
		{
			ExecButton* button;
			button = new ExecButton(UI_EXEC, Vec2i(5, 5), Vec2i(25, 25), this);
			button = new ExecButton(UI_EXEC_STEP, Vec2i(5 + 30, 5), Vec2i(25, 25), this);
			button->bStep = true;
			Color3ub DefaultKeywordColor{ 255, 188, 66 };
			Color3ub FlowKeywordColor{ 255, 188, 66 };
			WordColorMap["if"] = FlowKeywordColor;
			WordColorMap["elseif"] = FlowKeywordColor;
			WordColorMap["else"] = FlowKeywordColor;
			WordColorMap["end"] = FlowKeywordColor;
			WordColorMap["goto"] = FlowKeywordColor;
			WordColorMap["repeat"] = FlowKeywordColor;
			WordColorMap["for"] = FlowKeywordColor;
			WordColorMap["while"] = FlowKeywordColor;
			WordColorMap["do"] = FlowKeywordColor;
			WordColorMap["until"] = FlowKeywordColor;
			WordColorMap["return"] = FlowKeywordColor;
			WordColorMap["then"] = FlowKeywordColor;
			WordColorMap["break"] = FlowKeywordColor;
			WordColorMap["in"] = FlowKeywordColor;

			WordColorMap["function"] = DefaultKeywordColor;
			WordColorMap["local"] = DefaultKeywordColor;

			WordColorMap["true"] = DefaultKeywordColor;
			WordColorMap["false"] = DefaultKeywordColor;
			WordColorMap["nil"] = DefaultKeywordColor;

			WordColorMap["and"] = DefaultKeywordColor;
			WordColorMap["or"] = DefaultKeywordColor;
			WordColorMap["not"] = DefaultKeywordColor;
		}

		CodeFileAsset* codeFile;


		std::unordered_map<HashString, RHIGraphics2D::Color4Type > WordColorMap;


		Color4ub getColor(StringView word)
		{
			auto iter = WordColorMap.find(word);
			if (iter != WordColorMap.end())
				return iter->second;

			return Color4ub(255, 255, 255, 255);
		}

		virtual bool onChildEvent(int event, int id, GWidget* ui) 
		{
			switch (id)
			{
			case UI_EXEC:
				if (IViewModel::Get().isExecutingCode())
				{
					IViewModel::Get().stopExecution(*codeFile);
				}
				else
				{
					IViewModel::Get().runExecution(*codeFile);
				}

				break;
			case UI_EXEC_STEP:
				if (IViewModel::Get().isExecutingCode())
				{
					IViewModel::Get().pauseExecution(*codeFile);
				}
				else
				{
					IViewModel::Get().runStepExecution(*codeFile);
				}
				break;
			}
			return true; 
		}


		void parseCode()
		{
			char const* pCode = codeFile->code.c_str();

			while (*pCode != 0)
			{
				auto line = FStringParse::StringTokenLine(pCode);

				mCodeLines.push_back(CodeLine());

				auto& codeLine = mCodeLines.back();
				if (line.size() == 0)
					continue;
				
				mFont->generateLineVertices(Vector2::Zero(), line , 1.0, codeLine.vertices);
				codeLine.colors.resize(codeLine.vertices.size() / 4);

				StringView token;
				char const* pStr = line.data();
				char const* pStrEnd = line.data() + line.size();
				auto* pColor = codeLine.colors.data();
				int numWord = 0;
				while (pStr < pStrEnd)
				{
					char const* dropDelims = " \t\r\n";
					pStr = FStringParse::SkipChar(pStr, pStrEnd, dropDelims);
					if (pStr >= pStrEnd)
						break;

					char const* ptr = pStr;
					pStr = FStringParse::FindChar(pStr, pStrEnd, dropDelims);
					token = StringView(ptr, pStr - ptr);
					
					Color4ub color = getColor(token);

					for (int i = 0; i < token.size(); ++i)
					{
						pColor[i] = color;
					}
					pColor += token.size();
					numWord += token.size();
				}

				CHECK(numWord == codeLine.colors.size());
			}
		}
		void onRender()
		{
			Vec2i pos = getWorldPos();
			Vec2i size = getSize();
			RHIGraphics2D& g = Global::GetRHIGraphics2D();

			g.enablePen(false);
			g.setBrush(Color3ub(86, 86, 86));
			g.drawRoundRect(pos, size, Vector2(4, 4));


			g.setBrush(Color3ub(46, 46, 46));
			int border = 3;
			Vector2 textRectPos = pos + Vector2(border, border + 30);
			Vector2 textRectSize = size - Vector2(2 * border, 2 * border + 30);
			g.drawRoundRect(textRectPos, textRectSize, Vector2(8,8));


			g.beginClip(textRectPos, textRectSize);
			g.pushXForm();

			++showFrame;
			if (showFrame > 4)
			{
				if (!mLineShowQueue.empty())
				{
					mShowExecuteLine = mLineShowQueue.front();
					mLineShowQueue.removeIndex(0);
					showFrame = 0;
				}
			}
			g.setBrush(Color3ub(255, 255, 255));
			g.drawTextF(pos, "%d", mShowExecuteLine);
			g.translateXForm(textRectPos.x + 5, textRectPos.y + 5);

			g.setTexture(mFont->getTexture());
			g.setBlendState(ESimpleBlendMode::Translucent);

			int curline = 1;
			for (auto const& line : mCodeLines)
			{
				if (curline == mShowExecuteLine)
				{
					g.setBrush(Color3ub(32, 32, 32));
					g.setBlendState(ESimpleBlendMode::Add);
					g.drawRect(Vector2(-5, 0), Vector2(textRectSize.x, 16));
		

					g.setBrush(Color3ub(255, 255, 255));
					g.setTexture(mFont->getTexture());
					g.setBlendState(ESimpleBlendMode::Translucent);
				}
	
				g.drawTextQuad(line.vertices, line.colors);
				g.translateXForm(0, 16);
				curline += 1;
			}
			g.popXForm();
			g.endClip();

			g.enablePen(true);
		}

		void setExecuteLine(int line)
		{
			if (mLineShowQueue.empty() || mLineShowQueue.back() != line)
				mLineShowQueue.push_back(line);
		}

		TArray<int> mLineShowQueue;
		int showFrame = 0;

		struct CodeLine
		{
			int offset;
			TArray<GlyphVertex> vertices;
			TArray<RHIGraphics2D::Color4Type> colors;
		};
		TArray< CodeLine > mCodeLines;
		FontDrawer* mFont;
		int mShowExecuteLine = -1;

		struct Cursor
		{
			int line;
			int offset;
		};

		TArray<int> mBreakPoints;
	};


	class TestStage : public StageBase
		            , public IGameRenderSetup
		            , public IExecuteHookListener
					, public IViewModel
	{
		using BaseClass = StageBase;
	public:
		TestStage() {}

		enum
		{

			UI_DUMP = BaseClass::NEXT_UI_ID,

			UI_CODE_EDITOR,
		};
		

		GameState mGame;
		std::unordered_map<HashString, CodeFileAsset*> mCodeAssetMap;
		virtual bool isExecutingCode() 
		{
			return mGame.isExecutingCode();
		}

		virtual void runExecution(CodeFileAsset& codeFile) 
		{
			if (isExecutingCode())
				return;

			mGame.runExecution(*mGame.mDrones[0], codeFile);

		}
		
		virtual void stopExecution(CodeFileAsset& codeFile) 
		{
			for (Drone* drone : mGame.mDrones)
			{
				if (drone->executionCode == &codeFile)
				{
					mGame.stopExecution(*drone);
				}
			}
		}

		virtual void pauseExecution(CodeFileAsset& codeFile) 
		{
			for (Drone* drone : mGame.mDrones)
			{
				if (drone->executionCode == &codeFile)
				{
					drone->stateFlags |= ExecutableObject::ePause;
				}
			}
		}

		virtual void runStepExecution(CodeFileAsset& codeFile) 
		{
			for (Drone* drone : mGame.mDrones)
			{
				if (drone->executionCode == &codeFile)
				{
					drone->stateFlags &= ~ExecutableObject::ePause;
					drone->stateFlags |= ExecutableObject::eStep;
				}
			}

		}
		virtual void onHookLine(StringView const& fileName, int line)
		{
			auto iter = mCodeAssetMap.find(fileName);
			if (iter != mCodeAssetMap.end())
			{
				CodeFileAsset* asset = iter->second;
				asset->editor->setExecuteLine(line);
			}
		}

		virtual void onHookCall() 
		{

		}

		virtual void onHookReturn() 
		{

		}

		bool onInit() override
		{
			if (!BaseClass::onInit())
				return false;

			ExecuteManager::Get().mHookListener = this;
			IViewModel::Instance = this;

			::Global::GUI().cleanupWidget();

			auto codeFile = createCodeFile("Test");

			mGame.init();
			mGame.createDrone(*codeFile);


			Vector2 lookPos = 0.5 *  Vector2(mGame.mTiles.getSize());
			mWorldToScreen = RenderTransform2D::LookAt(::Global::GetScreenSize(), lookPos, Vector2(0, -1), ::Global::GetScreenSize().x / 10.0f);
			mScreenToWorld = mWorldToScreen.inverse();

			restart();
			return true;
		}

		void onEnd() override
		{

		}

		void restart() {}

		void onUpdate(GameTimeSpan deltaTime) override
		{
			BaseClass::onUpdate(deltaTime);
			mGame.update(deltaTime);
		}

		TArray< CodeFileAsset* > mCodeFiles;
		int mNextCodeId = 0;

		CodeFile* createCodeFile(char const* name)
		{
			CodeFileAsset* codeFile = new CodeFileAsset;
			codeFile->name = name;
			codeFile->id = mNextCodeId;
			codeFile->loadCode();

			CodeEditor* editor = new CodeEditor(UI_CODE_EDITOR + codeFile->id, Vec2i(100, 100), Vec2i(200, 400), nullptr);
			::Global::GUI().addWidget(editor);
			codeFile->editor = editor;
			editor->codeFile = codeFile;
			editor->mFont = &RenderUtility::GetFontDrawer(FONT_S10);
			mCodeFiles.push_back(codeFile);

			editor->parseCode();
			mCodeAssetMap.emplace(codeFile->getPath().c_str(), codeFile);

			return codeFile;
		}


		RenderTransform2D mWorldToScreen;
		RenderTransform2D mScreenToWorld;

		void onRender(float dFrame) override
		{
			RHICommandList& commandList = RHICommandList::GetImmediateList();
			auto screenSize = ::Global::GetScreenSize();

			RHISetFrameBuffer(commandList, nullptr);
			RHIClearRenderTargets(commandList, EClearBits::All, &LinearColor(0.2, 0.2, 0.2, 0.0), 1);
			RHISetViewport(commandList, 0, 0, screenSize.x, screenSize.y);

			RHIGraphics2D& g = ::Global::GetRHIGraphics2D();
			g.beginRender();

			g.pushXForm();
			g.transformXForm(mWorldToScreen, true);

			g.setTextRemoveRotation(true);
			g.setTextRemoveScale(true);

			RenderUtility::SetPen(g, EColor::Black);
			for (int j = 0; j < mGame.mTiles.getSizeY(); ++j)
			{
				for (int i = 0; i < mGame.mTiles.getSizeX(); ++i)
				{
					auto const& tile = mGame.getTile(Vec2i(i,j));

					RenderUtility::SetBrush(g, tile.ground == EGround::Grassland ? EColor::Green : EColor::Brown, COLOR_LIGHT);
					g.drawRect(Vector2(i,j), Vector2(1,1));
					if (tile.plant)
					{
						g.drawTextF(Vector2(i, j), Vector2(1, 1), tile.plant->getDebugInfo(tile).c_str());
					}
				}
			}

			RenderUtility::SetBrush(g, EColor::Yellow);
			for (auto drone : mGame.mDrones)
			{
				g.drawCircle( Vector2(drone->pos) + Vector2(0.5,0.5), 0.3 );
			}

			g.popXForm();


			g.endRender();
		}

		MsgReply onMouse(MouseMsg const& msg) override
		{
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


	REGISTER_STAGE_ENTRY("TFWR Game", TestStage, EExecGroup::Dev, "Game|Script");

}