#pragma once
#ifndef LuaInspector_H_444757BF_C4B4_42AE_A37E_21F1B3AB78D6
#define LuaInspector_H_444757BF_C4B4_42AE_A37E_21F1B3AB78D6

#include "TFWRScript.h"
#include <string>
#include <functional>

namespace TFWR {

class LuaInspector
{
public:
    using KeyValueCallback = std::function<bool (const std::string& key, const std::string& value)>;
    using LocalCallback = std::function<bool (const char* name, const std::string& value)>;

    // Convert a Lua value at stack index 'idx' into a short string representation.
    // 'maxDepth' controls recursion into tables.
    static std::string ValueToString(lua_State* L, int idx, int maxDepth = 3);

    // Enumerate global table entries and call 'cb' with key/value string pairs.
    static void EnumerateGlobals(lua_State* L, KeyValueCallback cb, int maxDepth = 3);

    // Enumerate locals for a given stack frame 'level' (0 = current) and call 'cb'.
    static void EnumerateLocals(lua_State* L, int level, LocalCallback cb, int maxDepth = 3);

    // Enumerate upvalues for function at 'funcIndex' (absolute or relative) and call 'cb'.
    static void EnumerateUpvalues(lua_State* L, int funcIndex, LocalCallback cb, int maxDepth = 3);
};

} // namespace TFWR
#endif // LuaInspector_H_444757BF_C4B4_42AE_A37E_21F1B3AB78D6
