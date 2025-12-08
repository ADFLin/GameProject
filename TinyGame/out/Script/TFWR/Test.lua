require("__builtins__")


local Data = TestData.new()
Data.a = 1
Data.b = 2.5
Data:foo()

local Data2 = TestData2.new()
Data2.a = 1
Data2.b = 2.5
Data2:foo()

local pos = measure(East)
print("measure:", pos[1], pos[2])
clear()

local worldSize = get_world_size()

local bOver = false
while not bOver do
    for i in range(worldSize) do
        for j in range(worldSize) do
            local x, y = get_pos_x(), get_pos_y()
            local entity, pos = get_companion()
            if entity ~= nil then
                print("Companion:", entity, "at", pos[1], pos[2])
            end
            if get_ground_type() ~= Grounds.Soil then
                till()
            end
            --print("Pos=",x,y)
            plant(Entities.Pumpkin)
            move(East)
        end
        move(North)
    end
end

