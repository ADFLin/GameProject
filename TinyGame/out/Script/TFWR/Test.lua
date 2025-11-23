require("__builtins__")

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

