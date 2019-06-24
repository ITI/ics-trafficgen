ntime = os.time() +15

function operate_outstation()
    data = {}
    if os.time() > ntime then
        ntime=ntime+15
        funcType = "Scan"
        funcName = "Group30Var5"
        start_index= 5
        --print("Printing to index")
        --print(start_index)
        end_index=9
        --print(end_index)
        table.insert(data, {funcType, funcName, start_index, end_index})

        --comment out lines below to disable printing of table
--        for k,v in pairs(data) do
--            for k1,v1 in pairs(v) do
--                print(k, k1, v1)
--            end
--        end
        --end print table
    end
    return data
end
