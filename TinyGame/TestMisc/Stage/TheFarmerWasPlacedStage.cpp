#include "Stage/TestStageHeader.h"
#include "GameRenderSetup.h"



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


namespace TFWR
{
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
	void Move(lua_State* L)
	{
		int dir = luaL_checkinteger(L, 1);
	}

	void LuaHook(lua_State *L, lua_Debug *ar)
	{
		lua_yield(L, 0);
	}

	luaL_Reg CustomLib[] =
	{
		{"print", Print},
		{NULL, NULL} /* end of array */
	};

	void LoadLib(lua_State* L)
	{
		lua_getglobal(L, "_G");
		luaL_setfuncs(L, CustomLib, 0);
		lua_pop(L, 1);
	}

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
		typedef int (T::*mfp)(lua_State *L);
		typedef struct { const char *name; mfp mfunc; } RegType;

		static void Register(lua_State *L)
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

		static void Register(lua_State *L)
		{
			lua_pushstring(L, l->name);
			lua_pushlightuserdata(L, (void*)l);
			lua_pushcclosure(L, thunk, 1);
			lua_settable(L, methods);
		}

		// call named lua method from userdata method table
		static int call(lua_State *L, const char *method,
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
		static int push(lua_State *L, T *obj, bool gc = false)
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
		static T *check(lua_State *L, int narg)
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
		static int thunk(lua_State *L)
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
		static int new_T(lua_State *L)
		{
			lua_remove(L, 1);   // use classname:new(), instead of classname.new()
			T *obj = new T(L);  // call constructor for T objects
			push(L, obj, true); // gc_T will delete this object
			return 1;           // userdata containing pointer to T object
		}

		// garbage collection metamethod
		static int gc_T(lua_State *L)
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

		static int tostring_T(lua_State *L)
		{
			char buff[32];
			userdataType *ud = static_cast<userdataType*>(lua_touserdata(L, 1));
			T *obj = ud->pT;
			sprintf(buff, "%p", (void*)obj);
			lua_pushfstring(L, "%s (%s)", T::className, buff);

			return 1;
		}

		static void set(lua_State *L, int table_index, const char *key)
		{
			lua_pushstring(L, key);
			lua_insert(L, -2);  // swap value and key
			lua_settable(L, table_index);
		}

		static void weaktable(lua_State *L, const char *mode)
		{
			lua_newtable(L);
			lua_pushvalue(L, -1);  // table is its own metatable
			lua_setmetatable(L, -2);
			lua_pushliteral(L, "__mode");
			lua_pushstring(L, mode);
			lua_settable(L, -3);   // metatable.__mode = mode
		}

		static void subtable(lua_State *L, int tindex, const char *name, const char *mode)
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

		static void *pushuserdata(lua_State *L, void *key, size_t sz)
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

	class TestStage : public StageBase
		            , public IGameRenderSetup
	{
		using BaseClass = StageBase;
	public:
		TestStage() {}

		
		lua_State* mMainL;
		lua_State* mExecL;
		bool onInit() override
		{
			if (!BaseClass::onInit())
				return false;
			::Global::GUI().cleanupWidget();


			mMainL = luaL_newstate();
			luaL_openlibs(mMainL);
			LoadLib(mMainL);

			mExecL = lua_newthread(mMainL);

			lua_sethook(mExecL, LuaHook, LUA_MASKLINE, 1);
			luaL_loadfile(mExecL, "TFWR/Test.lua");

			restart();
			return true;
		}

		void onEnd() override
		{
			lua_close(mMainL);
			BaseClass::onEnd();
		}

		void restart() {}

		void onUpdate(GameTimeSpan deltaTime) override
		{
			BaseClass::onUpdate(deltaTime);
		}

		void onRender(float dFrame) override
		{
			RHICommandList& commandList = RHICommandList::GetImmediateList();
			auto screenSize = ::Global::GetScreenSize();

			RHISetFrameBuffer(commandList, nullptr);
			RHIClearRenderTargets(commandList, EClearBits::All, &LinearColor(0.0, 0.0, 0.0, 0.0), 1);
			RHISetViewport(commandList, 0, 0, screenSize.x, screenSize.y);

			RHIGraphics2D& g = ::Global::GetRHIGraphics2D();

			g.beginRender();
			RenderUtility::SetBrush(g, EColor::White);
			g.endRender();
		}

		MsgReply onMouse(MouseMsg const& msg) override
		{
			return BaseClass::onMouse(msg);
		}

		bool executeStep()
		{
			int nres;
			int status = lua_resume(mExecL, mMainL, 0, &nres);
			
			if (status == LUA_YIELD)
			{
				return true;
			}
			else if (status == LUA_OK) 
			{
				return false;
			}
			else 
			{


			}

			return false;
		}
		MsgReply onKey(KeyMsg const& msg) override
		{
			if (msg.isDown())
			{
				switch (msg.getCode())
				{
				case EKeyCode::R: restart(); break;
				case EKeyCode::Z:
					{
						executeStep();
					}
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