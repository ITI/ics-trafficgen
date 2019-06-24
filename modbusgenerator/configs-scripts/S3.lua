ntime = os.time() +10
function generate_data()
    data = {}
    if os.time() > ntime then
        data = {"Discretes Input", "Coils", "Input Registers", "Holding Registers"}
        data["Discretes Input"]={}
        data["Coils"]={}
        data["Input Registers"]={}
        data["Holding Registers"]={}
        for i =1,10 do
            data["Discretes Input"][i]=i*math.random()*4.32
            data["Coils"][i]=math.floor(2*math.random())
            data["Input Registers"][i]=math.floor(i*math.random())
            data["Holding Registers"][i]=i*math.random()*1.66
        end
        ntime=ntime+10
    end
    return data
end
