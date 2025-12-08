---@meta

---@class Direction
Direction = {}
---@return Direction
function Direction.new() end

East = Direction.new(),
West = Direction.new(),
North = Direction.new(),
South = Direction.new(),


--[[A member of the Grounds Class]]
---@class Ground
Ground = {}
---@return Ground
function Ground.new() end

---@enum Grounds
Grounds =
{
	--[[The default ground. Grass will automatically grow on it.]]
    Grassland = Ground.new(),
    --[[Calling `till()` turns the ground into this. Calling `till()` again changes it back to grassland.]]
    Soil = Ground.new(),
}

---@class Item
Item = {}
---@return Item
function Item.new() end

---@enum Items
Items =
{
	Hay = Item.new(),
	Wood = Item.new(),
    Pumpkin = Item.new(),
}

---@class Entity
Entity = {}

---@return Entity
function Entity.new() end

---@enum Entities : Entity
Entities =
{
	--[[]
    Grows automatically on grassland. Harvest it to obtain `Items.Hay`.

    Average seconds to grow: 0.5
    Grows on: grassland or soil
    ]]
	Grass = Entity.new(),

	--[[
    A small bush that drops `Items.Wood`.

    Average seconds to grow: 4
    Grows on: grassland or soil
    ]]
	Bush = Entity.new(),

	--[[
    Trees drop more wood than bushes. They take longer to grow if other trees grow next to them.

    Average seconds to grow: 7
    Grows on: grassland or soil
    ]]
	Tree = Entity.new(),

    --[[
    Pumpkins grow together when they are next to other fully grown pumpkins. About 1 in 5 pumpkins dies when it grows up.
    When you harvest a pumpkin you get `Items.Pumpkin` equal to the number of pumpkins in the mega pumpkin cubed.

    Average seconds to grow: 2
    Grows on: soil
    ]]
	Pumpkin = Entity.new(),

	--[[
    One in five pumpkins dies when it grows up, leaving behind a dead pumpkin. Dead pumpkins are useless and disappear when something new is planted.
    `can_harvest()` always returns `false` on dead pumpkins.
	]]
	Dead_Pumpkin = Entity.new(),

	--[[Part of the maze.]]
	Hedge = Entity.new(),

    --[[A treasure that contains gold equal to the side length of the maze in which it is hidden. It can be harvested like a plant.]]
    Treasure = Entity.new(),
}

--[[
	Moves the drone into the specified `direction` by one tile.
	If the drone moves over the edge of the farm it wraps back to the other side of the farm.

	- `East `  =  right
	- `West `  =  left
	- `North`  =  up
	- `South`  =  down

	returns `true` if the drone has moved, `false` otherwise.

	takes `200` ticks to execute if the drone has moved, `1` tick otherwise.

	example usage:
    ```
    move(North)
    ```
]]
---@param direction Direction 
---@return boolean
function move(direction) end

--[[
    Tills the ground under the drone into soil. If it's already soil it will change the ground back to grassland.

    returns `nil`

    takes `200` ticks to execute.

    example usage:
    ```
    till()
    ```
]]


---@return nil
function till() end

--[[
    Harvests the entity under the drone.
    If you harvest an entity that can't be harvested, it will be destroyed.

    returns `true` if an entity was removed, `false` otherwise.

    takes `200` ticks to execute if an entity was removed, `1` tick otherwise.

    example usage:
    ```
    harvest()
    ```
]]
---@return boolean
function harvest() end

--[[
    Used to find out if plants are fully grown.

    returns `true` if there is an entity under the drone that is ready to be harvested, `false` otherwise.

    takes `1` tick to execute.

    example usage:
    ```
    if can_harvest():
        harvest()
    ```
]]
---@return boolean
function can_harvest() end

--[[
    Spends the cost of the specified `entity` and plants it under the drone.
    It fails if you can't afford the plant, the ground type is wrong or there's already a plant there.

    returns `true` if it succeeded, `false` otherwise.

    takes `200` ticks to execute if it succeeded, `1` tick otherwise.

    example usage:
    ```
    plant(Entities.Bush)
    ```
]]
---@param entity Entity
---@return boolean
function plant(entity) end

--[[
    Gets the current x position of the drone.
    The x position starts at `0` in the `West` and increases in the `East` direction.

    returns a number representing the current x coordinate of the drone.

    takes `1` tick to execute.

    example usage:
    ```
    x, y = get_pos_x(), get_pos_y()
    ```
]]
---@return integer
function get_pos_x() end

--[[
    Gets the current y position of the drone.
    The y position starts at `0` in the `South` and increases in the `North` direction.

    returns a number representing the current y coordinate of the drone.

    takes `1` tick to execute.

    example usage:
    ```
    x, y = get_pos_x(), get_pos_y()
    ```
]]
---@return integer
function get_pos_y() end

--[[
    Get the current size of the farm.

    returns the side length of the grid in the north to south direction.

    takes `1` tick to execute.

    example usage:
    ```
    for i in range(get_world_size()) do
        move(North)
	end
    ```
]]
---@return integer
function get_world_size() end

--[[
    Find out what kind of entity is under the drone.

    returns `nil` if the tile is empty, otherwise returns the type of the entity under the drone.

    takes `1` tick to execute.

    example usage:
    ```
    if get_entity_type() == Entities.Grass then
        harvest()
	end
    ```
]]
---@return Entity?
function get_entity_type() end

--[[
    Find out what kind of ground is under the drone.

    returns the type of the ground under the drone.

    takes `1` tick to execute.

    example usage:
    ```
    if get_ground_type() != Grounds.Soil then
        till()
    end
    ```
]]
---@return Ground
function get_ground_type() end


--[[
    Get the companion preference of the plant under the drone.

    returns a table of the form `{companion_type, {companion_x_position, companion_y_position}}` or `nil` if there is no companion.

    takes `1` tick to execute.

    example usage:
    ```
    companion, pos = get_companion()
    if companion ~= nil then
        x, y = table.unpack(pos)
        print(f"Companion: {plant_type} at ({x}, {y})")
    end
    ```
]]
---@return Entity | nil
---@return {x, y} | nil
function get_companion() end

--[[
    Removes everything from the farm, moves the drone back to position `(0,0)` and changes the hat back to the default.

    returns `nil`

    takes `200` ticks to execute.

    example usage:
    ```
    clear()
    ```
]]
---@return nil
function clear() end

--[[
    Find out how much of `item` you currently have.

    returns the number of `item` currently in your inventory.

    takes `1` tick to execute.

    example usage:
    ```
    if num_items(Items.Fertilizer) > 0:
        use_item(Items.Fertilizer)
    ```
]]
---@param item Item
---@return number
function num_items(item) end


--[[
    Swaps the entity under the drone with the entity next to the drone in the specified `direction`.
    - Doesn't work on all entities.
    - Also works if one (or both) of the entities are `nil`.

    returns `true` if it succeeded, `false` otherwise.

    takes `200` ticks to execute on success, `1` tick otherwise.

    example usage:
    ```
    swap(North)
    ```
]]
---@param direction Direction
---@return boolean
function swap(direction) end

--[[
    Can measure some values on some entities. The effect of this depends on the entity.

    overloads:
    `measure()`: measures the entity under the drone.
    `measure(direction)`: measures the neighboring entity in the `direction` of the drone.

    Sunflower: returns the number of petals.
    Maze: returns the position of the current treasure from anywhere in the maze.
    Cactus: returns the size.
    Dinosaur: returns the number corresponding to the type.
    All other entities: returns `None`.

    takes `1` tick to execute.

    example usage:
    ```
    num_petals = measure()
    treasure_pos = measure()  # Works anywhere in maze
    ```
]]
---@param direction? Direction
---@return number | integer[] | nil
function measure(direction) end

--[[
    Attempts to use the specified `item` `n` times. Can only be used with some items including `Items.Water`, `Items.Fertilizer` and `Items.Weird_Substance`.

    returns `true` if an item was used, `false` if the item can't be used or you don't have enough.

    takes `200` ticks to execute if it succeeded, `1` tick otherwise.

    example usage:
    ```
    if use_item(Items.Fertilizer) then
        print("Fertilizer used successfully")
    end
    ```
]]
---@param item Item
---@param n integer
---@return boolean
function use_item(item, n) end


--[[
    Checks if the drone can move in the specified `direction`.

    returns `true` if the drone can move, `false` otherwise.

    takes `1` tick to execute.

    example usage:
    ```
    if can_move(North) then
        move(North)
    end
    ```
]]
---@param direction Direction
function can_move(direction) end

--[[
    Used to check if an unlock, entity, ground, item or hat is already unlocked.

    returns `1` plus the number of times `thing` has been upgraded if `thing` is upgradable. Otherwise returns `1` if `thing` is unlocked, `0` otherwise.

    takes `1` tick to execute.

    example usage:
    ```
    if num_unlocked(Unlocks.Carrots) > 0 then
        plant(Entities.Carrot)
    else
        print("Carrots not unlocked yet")
    end
    ```
]]
---@param thing Unlock | Entity | Ground | Item
---@return int
function num_unlocked(thing) end



