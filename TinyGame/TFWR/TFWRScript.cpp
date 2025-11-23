#include "TFWRScript.h"

#include "CString.h"

#include "Lua/lopcodes.h"
#include "Lua/lua.h"
#include "Lua/lauxlib.h"
#include "Lua/lualib.h"
#include "Lua/lstate.h"

#include "Lua/ldebug.h"

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

namespace TFWR
{
	ExecutionContext* GExecutionContext = nullptr;

	class ExecuteManager : public IExecuteManager
	{
	public:


		int  mOpTicks[1 << SIZE_OP];

		ExecuteManager()
		{
			std::fill_n(mOpTicks, ARRAY_SIZE(mOpTicks), 0);
			std::fill_n(mOpTicks, OP_EXTRAARG + 1, 1);
			mOpTicks[OP_CALL] = 0;
			mOpTicks[OP_TAILCALL] = 0;
			mOpTicks[OP_TFORCALL] = 0;
			mOpTicks[OP_EXTRAARG] = 0;
		}

		ExecutionContext* mExecutingContext = nullptr;

		EExecuteStatus execute(ExecutionContext& context)
		{
			ExecutableObject& execObject = *context.object;

			int nres;
			int status;

			{
				TGuardValue< ExecutionContext*> scopedValue(mExecutingContext, &context);
				execObject.stateFlags |= ExecutableObject::eExecuting;
				status = lua_resume(execObject.executeL, context.mainL, 0, &nres);
				execObject.stateFlags &= ~ExecutableObject::eExecuting;
			}

			if (status == LUA_YIELD)
			{
				lua_pop(execObject.executeL, nres);
				return EExecuteStatus::Ok;
			}
			else if (status == LUA_OK)
			{
				onExecutionCompleted(execObject);
				lua_pop(execObject.executeL, nres);

				execObject.clearExecution();
				return EExecuteStatus::Completed;
			}
			else
			{
				onExecutionError(execObject, lua_tostring(execObject.executeL, -1));
				execObject.clearExecution();
				return EExecuteStatus::Error;
			}
		}

		ScriptHandle createThread(ScriptHandle masterL, char const* fileName)
		{
			InlineString<> path;
			path.format( TFWR_SCRIPT_DIR"/%s.lua", fileName);
			lua_State* L = lua_newthread(masterL);
			lua_sethook(L, HookEntry, LUA_MASKCOUNT | LUA_MASKLINE | LUA_MASKCALL | LUA_MASKRET, 1);
			luaL_loadfile(L, path.c_str());
			return L;
		}

		static void HookEntry(lua_State* L, lua_Debug *ar)
		{
			static_cast<ExecuteManager&>(IExecuteManager::Get()).hookExecution(L, ar);
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

			execObject.clearExecution();
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

		void hookExecution(lua_State* L, lua_Debug *ar);
	};


	IExecuteManager& IExecuteManager::Get()
	{
		static ExecuteManager StaticInstance;
		return StaticInstance;
	}

	ExecutionContext* IExecuteManager::GetCurrentContext()
	{
		return static_cast<ExecuteManager&>(ExecuteManager::Get()).mExecutingContext;
	}

	void ExecuteManager::hookExecution(lua_State* L, lua_Debug *ar)
	{
		auto& execObject = *mExecutingContext->object;

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

			if (ar->source[0] != '@' || FCString::CompareN(ar->source + 1, TFWR_SCRIPT_DIR"/Main.lua", 20) == 0)
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

			if (ar->source[0] == '@' && FCString::CompareN(ar->source + 1, TFWR_SCRIPT_DIR"/Main.lua", 20) == 0)
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
			if (lua_isinteger(L, i))
			{
				output += FStringConv::From(lua_tointeger(L, i));
			}
			else if (lua_isnumber(L, i))
			{
				output += FStringConv::From(lua_tonumber(L, i));
			}
			else if (lua_isstring(L, i))
			{
				char const* str = lua_tostring(L, i);
				output += str;
			}
		}

		LogMsg(output.c_str());
		return 0;
	}

}//namespace TFWR


