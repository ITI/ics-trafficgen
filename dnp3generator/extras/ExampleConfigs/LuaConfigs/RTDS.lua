mappingTable = {}
colNameTable = {}
dataFilePos = "set"
dataLine = ""

steadyFile = "S1_Steady_State.csv"
tocFile = "S2_Overcurrent_Delay_Fault4.csv"
instFile = "S3_Overcurrent_Instant_Fault6.csv"
s4ovFile = "S4_Overvoltage_Tripping.csv"
s5ovFile = "S5_Overvoltage_Warning.csv"
s6uvFile = "S6_Undervoltage_Warning.csv"
s7uvFile = "S7_Undervoltage_Tripping.csv"

finalFile = "Nonexistent_File.csv"

mappingFileName = "DNP point list_0808.txt"
dataFileName = steadyFile
fileIndex = 0

function string.starts(String,Start)
   return string.sub(String,1,string.len(Start))==Start
end

function CreateMappingTable()
    io.input(mappingFileName)
    for line in io.lines() do
        if string.starts(line, "#") ~= true then
            sep = ":"
            for dType, ptIndex, colName, deadband in string.gmatch(line, "(%a+)%s*:%s*(%d+)%s*([%w_]+)%s*(%g+)") do
                --print(colName, dType, ptIndex+1)
               mappingTable[colName] = {dType, ptIndex+1} --lua is index 1 based. So index 0 will cause problems.
            end
        end
    end
end

function ReadHeaderFromDataFile()
    local fHandle = io.open(dataFileName)
    local nCol = 1
    local sep = ","
    for headerLine in fHandle:lines() do
        --print(headerLine)
        for d in string.gmatch(headerLine,"([^"..sep.."]+)") do
            colNameTable[nCol] = d
            nCol = nCol + 1
        end
        dataFilePos = fHandle:seek()
        --print(dataFilePos)
        fHandle:close()
        return
    end
end

function ReadLineFromDataFile()
    local fHandle = io.input(dataFileName)
    fHandle:seek("set", dataFilePos)
    for dline in fHandle:lines() do
        dataFilePos = fHandle:seek()
        fHandle.close()
        return dline
    end
end

function SingleChangeDataFile()
    dataFileName = finalFile
    ReadHeaderFromDataFile()
    dataLine = ReadLineFromDataFile()
    return true
end

function ChangeDataFile()
    fileIndex = fileIndex +1
    if fileIndex < 2 then
        dataFileName = steadyFile
    elseif fileIndex == 2 then
        dataFileName = tocFile

    elseif fileIndex > 2 and fileIndex < 5 then
        dataFileName = steadyFile
    elseif fileIndex == 5 then
        dataFileName = instFile

    elseif fileIndex > 5 and fileIndex < 8 then
        dataFileName = steadyFile
    elseif fileIndex == 8 then
        dataFileName = s6uvFile

    elseif fileIndex > 8 and fileIndex < 11 then
        dataFileName = steadyFile
    elseif fileIndex == 11 then
        dataFileName = s7uvFile

    elseif fileIndex > 11 and fileIndex < 14 then
        dataFileName = steadyFile
    elseif fileIndex == 14 then
        dataFileName = s4ovFile

    elseif fileIndex > 14 and fileIndex < 17 then
        dataFileName = steadyFile
    elseif fileIndex == 17 then
        dataFileName = s5ovFile

    elseif fileIndex > 17 and fileIndex < 19 then
        dataFileName = steadyFile
    elseif fileIndex == 19 then
        dataFileName = finalFile
    else
        return false
    end
    --print("LUA: DATAFILE IS NOW", dataFileName)
    ReadHeaderFromDataFile()
    dataLine = ReadLineFromDataFile()
    return true
end

function generate_data()
    data = {}
    data["Analog Input"]={}
    data["Counter"]={}
    data["Binary Input"]={}
    dataLine = ReadLineFromDataFile()
    local sep = ","
    local nCol=1
    if dataLine == nil then
        if ChangeDataFile() == false then
            return data
        end
    end

    for d in string.gmatch(dataLine,"([^"..sep.."]+)") do
        colName = colNameTable[nCol]
        if nCol == 1 then
            data["Timestamp"] = d
        end
        if mappingTable[colName] then
            type = mappingTable[colName][1]
            index = mappingTable[colName][2]
            --print(type, index, d)
            if type == "AI" then
                data["Analog Input"][index] = d
            elseif type == "BI" then
                data["Binary Input"][index] = d
            elseif type == "CI" then
                data["Counter"][index] = d
            end
        end
        nCol = nCol + 1
    end
    return data
end

CreateMappingTable()
ReadHeaderFromDataFile()
