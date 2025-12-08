#include "TFWRScript.h"

#include "CString.h"
#include "HashString.h"
#include "LogSystem.h"

#include "Lua/lopcodes.h"
#include "Lua/lua.h"
#include "Lua/lauxlib.h"
#include "Lua/lualib.h"
#include "Lua/lstate.h"

#include "Lua/ldebug.h"
#include "LuaInspector.h"

#include <unordered_set>


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

namespace TFWR
{
	int GObjectCount = 0;

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
		bool mJustEnteredNewLine = false;

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
			lua_sethook(L, ExecuteHook, LUA_MASKCOUNT | LUA_MASKLINE | LUA_MASKCALL | LUA_MASKRET, 1);
			luaL_loadfile(L, path.c_str());
			return L;
		}

		static void ExecuteHook(lua_State* L, lua_Debug *ar)
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

			// 在 LUA_HOOKLINE 時處理 step 邏輯（此時指令還沒執行）
			if (execObject.stateFlags & ExecutableObject::eStep)
			{
				execObject.stateFlags &= ~ExecutableObject::eStep;
				execObject.stateFlags |= ExecutableObject::ePause;

				execObject.line = ar->currentline;
				auto fileName = StringView(ar->source + 1, ar->srclen - 1);
				onHookLine(fileName, ar->currentline);

				lua_yield(L, 0);
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

		// LUA_HOOKCOUNT 事件處理
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


