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
finalFile = "Non_Existing.csv"

mappingFileName = "DNP3PointListTest.txt"
dataFileName = s7uvFile
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
    fileIndex = fileIndex +1
    if fileIndex < 2 then
dataFileName = finalFile
    else
	return false
    end 
    ReadHeaderFromDataFile()
dataFileName = steadyFile
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
    print(dataFileName)
    if dataLine == nil then
        if SingleChangeDataFile() == false then
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
