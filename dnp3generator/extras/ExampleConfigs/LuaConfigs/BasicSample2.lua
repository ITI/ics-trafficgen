ntime = os.time() +10

function generate_data()
    data = {}
    if os.time() > ntime then
        data["Analog Input"]={}
        data["Counter"]={}
        data["Binary Input"]={}
        for i =1,5 do
            data["Analog Input"][i]=i*2*math.random()
            data["Binary Input"][i]=math.floor(math.random()*2)
            data["Counter"][i]=math.floor(i*2*math.random())
        end
        ntime=ntime+10
    end
    return data
end
