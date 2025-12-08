#include "LuaInspector.h"
#include <sstream>
#include <unordered_set>
#include <iomanip>

namespace TFWR
{

	namespace
	{
		// Helper: safe absolute index
		static int absIndex(lua_State* L, int idx)
		{
			return lua_absindex(L, idx);
		}

		// Forward declaration for recursive table summarization
		static void tableSummary(lua_State* L, int tableIndex, int depth, int maxDepth, std::unordered_set<const void*>& seen, std::ostringstream& out);
	}

	std::string LuaInspector::ValueToString(lua_State* L, int idx, int maxDepth)
	{
		idx = absIndex(L, idx);
		int t = lua_type(L, idx);
		std::ostringstream out;
		switch (t)
		{
		case LUA_TNIL:
			out << "nil";
			break;
		case LUA_TNUMBER:
#if LUA_VERSION_NUM >= 503
			if (lua_isinteger(L, idx)) {
				out << lua_tointeger(L, idx);
			}
			else {
				out << std::setprecision(14) << lua_tonumber(L, idx);
			}
#else
			out << std::setprecision(14) << lua_tonumber(L, idx);
#endif
			break;
		case LUA_TSTRING:
			out << '"' << lua_tostring(L, idx) << '"';
			break;
		case LUA_TBOOLEAN:
			out << (lua_toboolean(L, idx) ? "true" : "false");
			break;
		case LUA_TTABLE:
			{
				const void* p = lua_topointer(L, idx);
				out << "table:" << p;
				if (maxDepth > 0) {
					std::unordered_set<const void*> seen;
					tableSummary(L, idx, 0, maxDepth, seen, out);
				}
				break;
			}
		case LUA_TFUNCTION:
			{
				const void* p = lua_topointer(L, idx);
				out << "function:" << p;
				break;
			}
		case LUA_TUSERDATA:
			{
				const void* p = lua_touserdata(L, idx);
				out << "userdata:" << p;
				break;
			}
		case LUA_TTHREAD:
			{
				const void* p = lua_topointer(L, idx);
				out << "thread:" << p;
				break;
			}
		default:
			out << lua_typename(L, t);
			break;
		}

		return out.str();
	}

	void LuaInspector::EnumerateGlobals(lua_State* L, KeyValueCallback cb, int maxDepth)
	{
#if LUA_VERSION_NUM >= 502
		lua_pushglobaltable(L);
#else
		lua_getglobal(L, "_G");
#endif
		int gidx = absIndex(L, -1);

		lua_pushnil(L);
		while (lua_next(L, gidx) != 0) 
		{
			// -2 = key, -1 = value
			std::string key;
			if (lua_type(L, -2) == LUA_TSTRING) 
			{
				key = lua_tostring(L, -2);
			}
			else 
			{
				std::ostringstream k;
				int kt = lua_type(L, -2);
				if (kt == LUA_TNUMBER) {
					k << lua_tonumber(L, -2);
				}
				else {
					k << lua_typename(L, kt) << ':' << lua_topointer(L, -2);
				}
				key = k.str();
			}

			std::string val = ValueToString(L, -1, maxDepth);
			bool bKeep = cb(key, val);

			lua_pop(L, 1); // pop value, keep key
			if (!bKeep)
				break;
		}

		lua_pop(L, 1); // pop globals table
	}

	void LuaInspector::EnumerateLocals(lua_State* L, int level, LocalCallback cb, int maxDepth)
	{
		lua_Debug ar;
		if (lua_getstack(L, level, &ar) == 0)
		{
			return;
		}

		// optionally get extra info; this does not push values except when 'f' is used
		lua_getinfo(L, "nSl", &ar);

		for (int i = 1; ; ++i) 
		{
			const char* name = lua_getlocal(L, &ar, i);
			if (name == nullptr) break;
			// value is on top
			std::string val = ValueToString(L, -1, maxDepth);
			bool bKeep = cb(name, val);
			lua_pop(L, 1);
			if (!bKeep)
				break;
		}
	}

	void LuaInspector::EnumerateUpvalues(lua_State* L, int funcIndex, LocalCallback cb, int maxDepth)
	{
		int fidx = absIndex(L, funcIndex);
		for (int i = 1; ; ++i) 
		{
			const char* name = lua_getupvalue(L, fidx, i);
			if (name == nullptr) break;
			std::string val = ValueToString(L, -1, maxDepth);
			bool bKeep = cb(name, val);
			lua_pop(L, 1);
			if (!bKeep)
				break;
		}
	}

	namespace
	{

		static void tableSummary(lua_State* L, int tableIndex, int depth, int maxDepth, std::unordered_set<const void*>& seen, std::ostringstream& out)
		{
			if (depth >= maxDepth) return;
			tableIndex = absIndex(L, tableIndex);
			const void* ptr = lua_topointer(L, tableIndex);
			if (!ptr) return;
			if (!seen.insert(ptr).second) 
			{
				out << " <cycle>";
				return;
			}

			out << " {";
			bool first = true;
			lua_pushnil(L);
			while (lua_next(L, tableIndex) != 0) 
			{
				if (!first) out << ", ";
				first = false;
				// key at -2, value at -1
				std::string keyStr = (lua_type(L, -2) == LUA_TSTRING) ? lua_tostring(L, -2) : (std::string("[") + lua_typename(L, lua_type(L, -2)) + ":" + std::to_string((long long)lua_topointer(L, -2)) + "]");
				out << keyStr << "=";

				if (lua_istable(L, -1)) {
					// recurse
					tableSummary(L, lua_gettop(L), depth + 1, maxDepth, seen, out);
				}
				else {
					out << LuaInspector::ValueToString(L, -1, 0);
				}

				lua_pop(L, 1); // pop value, keep key
			}
			out << " }";
		}

	} // namespace

} // namespace TFWR
