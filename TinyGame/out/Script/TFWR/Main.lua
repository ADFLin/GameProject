function custom_require(name)
    -- Check the C++ map for the script
    if name == "__builtins__" then
        return nil  -- Prevent re-loading
    end
    -- Fallback to the default require behavior
    return original_require(name)
end

original_require = require
require = custom_require

---@return Entity | nil
---@return {x, y} | nil
function get_companion()
    local entity, x, y = __Internal_get_companion()
    if entity == nil then
        return nil
    end
    return entity, { x , y } 
end

--[[
    Returns a sequence of numbers from start (inclusive) to to (inclusive).

    returns a range object.

    takes `1` tick to execute.

    example usage:
    ```
    for i in range(5):
        print(i)  # 1, 2, 3, 4, 5
    ```
]]
---@param from integer
---@param to? integer @to value, if nil, from is treated as to and from is set to 1
---@param step? integer @step size, default is 1
---@return fun(): integer
function range(from, to, step)

    if to == nil then
        to = from
        from = 1
    end
    step = step or 1 -- Default step to 1
    local i = from - step -- Initialize for the first iteration

    local IterFunc = function()
        i = i + step
        if (step > 0 and i <= to) or (step < 0 and i >= to) then
            return i
        else
            return nil
        end
    end

    return IterFunc
end