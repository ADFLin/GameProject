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
#include "Lua/lstate.h"
#include "Lua/lopcodes.h"

#include "DataStructure/Grid2D.h"
#include "Math/TVector2.h"
#include "Meta/MetaBase.h"



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
	void Move(lua_State* L)
	{
		int dir = luaL_checkinteger(L, 1);
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




	class Drone
	{
	public:

		Drone()
		{
			reset();
		}

		void reset()
		{
			iTicksWait = 0;
			fTicksAcc = 0;
			pos = Vec2i(0, 0);
		}

		int iTicksWait;

		float fTicksAcc;

		Vec2i pos;
		lua_State* mExecL;
	};

	class Ground
	{


	};

	struct MapTile;

	class Entity
	{
	public:
		virtual ~Entity() = default;
		virtual void grow(MapTile& tile, float deltaTime, float fTicks) = 0;
	};

	constexpr int TicksPerSecond = 400;



	enum EItem
	{



		COUNT,
	};

	enum EGround
	{
		Grassland,
		Soil,
	};


	struct MapTile
	{
		EGround ground;
		Entity* plant;
		float  growValue;

		void reset()
		{
			ground = EGround::Grassland;
			plant = nullptr;
			growValue = 0;
		}
	};


	class SimplePlantEntity : public Entity
	{

	public:

		void grow(MapTile& tile, float deltaTime, float fTicks)
		{



		}


		EItem production;
	};

	class CodeFile
	{
	public:
		std::string name;
	};


	enum EDirection
	{
		East,
		West,
		North,
		South,
	};

	using ScriptHandle = lua_State*;


	class Drone;
	class GameState;

	struct ExecutionContext
	{
		GameState* game;
		Drone* drone;
		int iTicks;
	};


	ExecutionContext* GExecContext = nullptr;

	struct ScopedExecutionContext : ExecutionContext
	{
		ScopedExecutionContext()
		{
			prevContext = GExecContext;
			GExecContext = this;
		}

		~ScopedExecutionContext()
		{
			GExecContext = prevContext;
		}
		ExecutionContext* prevContext;
	};


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

	struct FScriptAPI
	{

		static void Wait(int waitTicks)
		{
			GExecContext->iTicks -= waitTicks;
		}

		static void LuaHook(lua_State* L, lua_Debug *ar)
		{
			const Instruction* pc = L->ci->u.l.savedpc;
			int op = *(pc - 1);
			if (op != OP_CALL && op != OP_TAILCALL && op != OP_TFORCALL)
			{
				--GExecContext->iTicks;
			}
			if (GExecContext->iTicks <= 0)
			{
				GExecContext->drone->iTicksWait -= GExecContext->iTicks;
				lua_yield(L, 0);
			}
#if 0
#define CASE_OP(op) case op: LogMsg(#op); break;
			switch (GET_OPCODE(*(pc - 1)))
			{
				OP_CODE_LIST(CASE_OP)
			}
#endif
		}
	};


	class GameState
	{
	public:

		GameState()
		{
			mItems.resize(EItem::COUNT, 0);
		}

		void init();

		void release()
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


		void reset()
		{
			for (auto& tile : mTiles)
			{
				tile.reset();
			}

			for (auto drone : mDrones)
			{
				drone->reset();
			}
		}

		void update(float deltaTime)
		{
			float fTicks = deltaTime * TicksPerSecond;

			for (Drone* drone : mDrones)
			{
				update(*drone, deltaTime, fTicks);
			}
			for (auto& tile : mTiles)
			{
				update(tile, deltaTime, fTicks);
			}
		}

		void update(Drone& drone, float deltaTime, float fTicks)
		{	
			if (drone.mExecL == nullptr)
			{
				return;
			}
			drone.fTicksAcc += fTicks;
			int iTicks = Math::FloorToInt(drone.fTicksAcc);

			if (iTicks == 0)
				return;

			drone.fTicksAcc -= iTicks;

			bool bExecStep = true;
			if (drone.iTicksWait > 0)
			{
				if (iTicks >= drone.iTicksWait)
				{
					iTicks -= drone.iTicksWait;
					drone.iTicksWait = 0;
				}
				else
				{
					drone.iTicksWait -= iTicks;
					bExecStep = false;
				}
			}

			if (bExecStep)
			{
				ScopedExecutionContext context;
				context.drone = &drone;
				context.game = this;
				context.iTicks = iTicks;

				int nres;
				int status = lua_resume(drone.mExecL, mMainL, 0, &nres);

				if (status == LUA_YIELD)
				{

				}
				else if (status == LUA_OK)
				{
					drone.mExecL = nullptr;
					drone.fTicksAcc = 0;
					drone.iTicksWait = 0;
				}
				else
				{
					drone.mExecL = nullptr;
					drone.fTicksAcc = 0;
					drone.iTicksWait = 0;
				}
			}
		}


		void update(MapTile& tile, float deltaTime, float fTicks)
		{
			if (tile.plant)
			{
				tile.plant->grow(tile, deltaTime, fTicks);
			}
		}

		Drone* createDrone(CodeFile& file);



		void till(Drone& drone)
		{



		}

		bool harvest(Drone& drone)
		{

			return true;
		}

		bool canHarvest(Drone& drone)
		{
			auto& tile = getTile(drone.pos);
			if (tile.growValue < 1.0)
				return false;

			return true;

		}

		bool plant(Drone& drone, Entity& entity)
		{
			auto& tile = getTile(drone.pos);
			if (tile.plant)
			{
				return false;
			}

			tile.plant = &entity;
			tile.growValue = 0.0;
		}

		bool move(Drone& drone, EDirection direction)
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

		MapTile& getTile(Vec2i const& pos)
		{
			return mTiles(pos);
		}

		lua_State* mMainL = nullptr;

		TArray<Drone*> mDrones;
		TGrid2D<MapTile> mTiles;

		TArray<int> mItems;
	};


	struct FGameAPI : FScriptAPI
	{
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

#undef REGISTER
		}

		static lua_State* CreateGameState()
		{
			lua_State* L = luaL_newstate();
			luaL_openlibs(L);
			LoadLib(L);
			Register(L);
			FLuaBinding::Register(L, "TestFunc", TestFunc);
			return L;
		}

		static lua_State* CreateExecuteState(lua_State* gameL, char const* fileName)
		{
			InlineString<> path;
			path.format("TFWR/%s.lua", fileName);
			lua_State* L = lua_newthread(gameL);
			lua_sethook(L, LuaHook, LUA_MASKCOUNT, 1);
			luaL_loadfile(L, path.c_str());
			return L;
		}
	};


	void GameState::init()
	{
		mMainL = FGameAPI::CreateGameState();
		mTiles.resize(3, 3);
		reset();
	}

	Drone* GameState::createDrone(CodeFile& file)
	{
		Drone* drone = new Drone;
		drone->mExecL = FGameAPI::CreateExecuteState(mMainL, file.name.c_str());
		drone->reset();

		mDrones.push_back(drone);
		return drone;
	}



	class TestStage : public StageBase
		            , public IGameRenderSetup
	{
		using BaseClass = StageBase;
	public:
		TestStage() {}


		GameState mGmae;
		

		CodeFile codeFile;

		bool onInit() override
		{
			if (!BaseClass::onInit())
				return false;




			::Global::GUI().cleanupWidget();

			codeFile.name = "Test";

			mGmae.init();
			mGmae.createDrone(codeFile);


			Vector2 lookPos = 0.5 *  Vector2(mGmae.mTiles.getSize());
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
			mGmae.update(deltaTime);
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
			for (int j = 0; j < mGmae.mTiles.getSizeY(); ++j)
			{
				for (int i = 0; i < mGmae.mTiles.getSizeX(); ++i)
				{
					auto const& tile = mGmae.getTile(Vec2i(i,j));

					RenderUtility::SetBrush(g, tile.ground == EGround::Grassland ? EColor::Green : EColor::Brown, COLOR_LIGHT);
					g.drawRect(Vector2(i,j), Vector2(1,1));
					//g.drawTextF(Vector2(i,j), Vector2(1,1), "P = %d, G = %f", ;
				}
			}


			RenderUtility::SetBrush(g, EColor::Yellow);
			for (auto drone : mGmae.mDrones)
			{
				g.drawCircle( drone->pos + Vector2(0.5,0.5), 0.3 );
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