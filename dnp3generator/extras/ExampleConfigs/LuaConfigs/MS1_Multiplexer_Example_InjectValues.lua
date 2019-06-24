

function modify_analog(table)
    --print("IN LUA SCRIPT FOR MODIFYING ANALOG VALUES!!!!!!!!!")
    -- return does not like local modifier to analog_data. Who knew??
    analog_data = {}
    for k,v in pairs(table) do
        print(k,v)
        analog_data[k] = v+10
    end
    return analog_data
end

function modify_binary(table)
    --print("IN LUA SCRIPT FOR MODIFYING BINARY VALUES!!!!!!!!!")
    -- return does not like local modifier to binary_data. Who knew??
    binary_data = {}
    for k,v in pairs(table) do
        binary_data[k] = v
    end
    return binary_data
end

function modify_counter(table)
    --print("IN LUA SCRIPT FOR MODIFYING COUNTER VALUES!!!!!!!!!")
    -- return does not like local modifier to counter_data. Who knew??
    counter_data = {}
    for k,v in pairs(table) do
        counter_data[k] = v+10
    end
    return counter_data
end
