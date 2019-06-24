ntime = os.time() +15

function operate_outstation()
    data = {}
    if os.time() > ntime then
        funcType = "DO"
        funcName = "CROB"
        index= math.floor(math.random()*10)
        --print("Printing to index")
        --print(index)
        value=math.floor(2*math.random())
        --print(value)
        table.insert(data, {funcType, funcName, index, value})

        funcName = "AnalogOutputFloat32"
        funcType = "SBO"
        index= math.floor(math.random()*10)
        --print("Printing to index")
        --print(index)
        value=15*math.random()
        --print(value)
        table.insert(data, {funcType, funcName, index, value})
        ntime=ntime+15
        --print(data)
        --comment out lines below to disable printing of table
        for k,v in pairs(data) do
            for k1,v1 in pairs(v) do
                print(k, k1, v1)
            end
        end
        --end print table
    end
    return data
end
