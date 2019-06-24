ntime = os.time() +15

function generate_data()
    data = {}
    if os.time() > ntime then
        data["Analog Input"]={}
        data["Counter"]={}
        data["Binary Input"]={}
        for i =1,10 do
            data["Analog Input"][i]=i
            data["Binary Input"][i]=math.floor(math.random()*2)
            data["Counter"][i]=math.floor(i*math.random())
        end
        ntime=ntime+15
    end
    return data
end
