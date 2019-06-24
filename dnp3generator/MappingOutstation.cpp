
#include "MappingOutstation.h"
#include "dnp3app.h"

#include <unistd.h>
#include <map>
#include <numeric>
#include <atomic>
#include <ctime>

#include <asiodnp3/DNP3Manager.h>
#include <asiodnp3/DefaultMasterApplication.h>
//#include <asiodnp3/PrintingSOEHandler.h>
#include <asiodnp3/ConsoleLogger.h>
#include <opendnp3/outstation/SimpleCommandHandler.h>
#include <opendnp3/LogLevels.h>
#include <opendnp3/outstation/DatabaseSizes.h>
#include <opendnp3/app/DNPTime.h>
#include <asiopal/ChannelRetry.h>
#include <asiodnp3/PrintingChannelListener.h>
#include <asiodnp3/OutstationStackConfig.h>
#include <asiodnp3/UpdateBuilder.h>

extern ThreadSafeUserInput luaSwitchObj;

MappingOutstation::MappingOutstation(std::unique_ptr<Node> node, std::atomic<bool>& quitFlag)
:OutStation(std::move(node), quitFlag)
{}

void MappingOutstation::initialize(){
}

void MappingOutstation::run()
{
    //allocate space for input arrays
    ConfigureInputMultiplexingArrays();

    for(std::map<std::string, std::vector<MappedDataPoint>>::iterator srcIter = node->dataSources.begin(); srcIter != node->dataSources.end(); ++srcIter){
        std::string srcName = srcIter->first;
        analogFlag[srcName]=false;
        binaryFlag[srcName]=false;
        counterFlag[srcName]=false;
    }

    const uint32_t FILTERS = opendnp3::levels::NORMAL | opendnp3::levels::ALL_COMMS ;

    // The main object for a outstation. The defaults are useable,
    // but understanding the options are important. OREDER IS IMPORTANT HERE. DO NOT CHANGE!
    asiodnp3::OutstationStackConfig outstationConfig(opendnp3::DatabaseSizes(
		node->dbSize["Binary Input"],
		node->dbSize["Double Binary Input"],
		node->dbSize["Analog Input"],
		node->dbSize["Counter"],
		node->dbSize["Frozen Counter"],
		node->dbSize["Binary Output"],
		node->dbSize["Analog Output"],
		node->dbSize["Time Interval"]
	));

	// Specify the size of the event buffers. Max size is currently 250 events
    outstationConfig.outstation.eventBufferConfig = opendnp3::EventBufferConfig(
	    node->evtBufferSize["Binary Input"],
	    node->evtBufferSize["Double Binary Input"],
	    node->evtBufferSize["Analog Input"],
	    node->evtBufferSize["Counter"],
	    node->evtBufferSize["Frozen Counter"],
	    node->evtBufferSize["Binary Output"],
	    node->evtBufferSize["Analog Output"],
        10 //maxSecurityStatisticEvents
    );

    // you can override an default outstation parameters here
    // in this example, we've enabled the oustation to use unsolicted reporting
    // if the master enables it
    outstationConfig.outstation.params.allowUnsolicited = node->allowUnsolicited;

    // You can override the default link layer settings here
    // in this example we've changed the default link layer addressing
    outstationConfig.link.LocalAddr = node->localDNP3Addr;
    outstationConfig.link.RemoteAddr = node->remoteDNP3Addr;
	outstationConfig.link.KeepAliveTimeout = openpal::TimeDuration::Max();

	// You can optionally change the default reporting variations or class assignment prior to enabling the outstation
    ConfigureDatabase(outstationConfig.dbConfig);
    // Create a new outstation with a log level, command handler, and
    // config info this	returns a thread-safe interface used for
    // updating the outstation's database.
    auto xChannel = manager->AddTCPServer((node->name + " server").c_str(), FILTERS, asiopal::ChannelRetry::Default(), node->local_IPAddress, node->port, asiodnp3::PrintingChannelListener::Create());

    outstationInstance = xChannel->AddOutstation(node->name.c_str(), opendnp3::SuccessCommandHandler::Create(),
											opendnp3::DefaultOutstationApplication::Create(), outstationConfig);

    /* initialize Lua */
	L = luaL_newstate();
	/* Load Lua base libraries */
	luaL_openlibs(L);
	/* load the script */
    bool lua_script_exists = node->luaFileNames.size() >0 ? FileExists(node->luaFileNames[0].c_str()) : false;
    if(lua_script_exists)
	   luaL_dofile(L, node->luaFileNames[0].c_str());

    // Enable the outstation and start communications
    outstationInstance->Enable();

    while(true)
    {
		if (this->quitFlag)
		    break;
	    usleep(node->msToSleep); //sleep for 1 millisecond
        if(lua_script_exists){
            while (luaSwitchObj.readCount() > localLuaFlag){
                localLuaFlag+=1;
                if (!node->luaKeySwitch.empty() && luaSwitchObj.readInputStr(localLuaFlag) == node->luaKeySwitch){
                    auto luaFileToLoad = GetNextLuaFile();
                    printf("Mapping Outstation:%s -- Changing lua file to %s\n", node->name.c_str(), luaFileToLoad.c_str());
                    luaL_dofile(L, luaFileToLoad.c_str());
                }
            }
            injectDataChanges();
        }
        else
            passThruDataChanges();
    }
    /* cleanup Lua */
	lua_close(L);
}

void MappingOutstation::ConfigureInputMultiplexingArrays(){

    for(auto srcIter = node->dataSources.begin(); srcIter != node->dataSources.end(); ++srcIter) {
        std::string srcName = srcIter->first;
        std::vector<MappedDataPoint> mdpList = srcIter->second;
        std::map<std::string, int> dbSize; //size for each data type

        for(auto& dp : mdpList) {
            if(dbSize[dp.pointType] < dp.index+1) {
                dbSize[dp.pointType] = dp.index+1;
            }
        }

        analogValues[srcName].resize(dbSize["Analog Input"]);
        for(auto& pt : analogValues[srcName]) {
            pt.value = 0;
            pt.timestamp = 0;
        }

        binaryValues[srcName].resize(dbSize["Binary Input"]);
        for(auto& pt : binaryValues[srcName]) {
            pt.value = 0;
            pt.timestamp = 0;
        }

        counterValues[srcName].resize(dbSize["Counter"]);
    }
    analogOutValues.resize(node->dbSize["Analog Input"]);
    binaryOutValues.resize(node->dbSize["Binary Input"]);
    counterOutValues.resize(node->dbSize["Counter"]);
}

void MappingOutstation::passThruDataChanges(){
    std::lock_guard<std::mutex> lock(mutex);
    asiodnp3::UpdateBuilder builder;
    std::time_t timestamp = std::time(nullptr) * 1000; //OpenDNP3 expects in ms

    for(std::map<std::string, std::vector<MappedDataPoint>>::iterator srcIter = node->dataSources.begin(); srcIter != node->dataSources.end(); ++srcIter){
        std::string srcName = srcIter->first;
        std::vector<MappedDataPoint> mdpList = srcIter->second;
        if(analogFlag[srcName]){
            analogFlag[srcName]=false;
            for(const auto& dp : mdpList) {
                if (dp.pointType != "Analog Input")
                    continue;
                analogOutValues[dp.index].value=analogValues[srcName][dp.input_index].value;
                analogOutValues[dp.index].timestamp=analogValues[srcName][dp.input_index].timestamp;
                builder.Update(opendnp3::Analog(analogOutValues[dp.index].value, int(opendnp3::AnalogQuality::ONLINE), opendnp3::DNPTime(analogOutValues[dp.index].timestamp)), dp.index);
            }
        }
        if(binaryFlag[srcName]){
            binaryFlag[srcName]=false;
            for(const auto& dp : mdpList) {
                if (dp.pointType != "Binary Input")
                    continue;
                binaryOutValues[dp.index].value=binaryValues[srcName][dp.input_index].value;
                binaryOutValues[dp.index].timestamp=binaryValues[srcName][dp.input_index].timestamp;
                builder.Update(opendnp3::Binary(binaryOutValues[dp.index].value, int(opendnp3::BinaryQuality::ONLINE), opendnp3::DNPTime(binaryOutValues[dp.index].timestamp)), dp.index);
            }
        }
        if(counterFlag[srcName]){
            counterFlag[srcName]=false;
            for(std::vector<MappedDataPoint>::iterator dIter=mdpList.begin(); dIter != mdpList.end(); ++dIter){
                MappedDataPoint dp = *dIter;
                if (dp.pointType != "Counter")
                    continue;
                counterOutValues[dp.index].value=counterValues[srcName][dp.input_index].value;
                counterOutValues[dp.index].timestamp=counterValues[srcName][dp.input_index].timestamp;
                builder.Update(opendnp3::Counter(counterOutValues[dp.index].value, int(opendnp3::CounterQuality::ONLINE),  opendnp3::DNPTime(counterOutValues[dp.index].timestamp)), dp.index);
            }
        }
    }
    outstationInstance->Apply(builder.Build());
}


void MappingOutstation::injectDataChanges(){
    std::lock_guard<std::mutex> lock(mutex);

    for(std::map<std::string, std::vector<MappedDataPoint>>::iterator srcIter = node->dataSources.begin(); srcIter != node->dataSources.end(); ++srcIter){
        std::string srcName = srcIter->first;
        std::vector<MappedDataPoint> mdpList = srcIter->second;
        if(analogFlag[srcName]){
            analogFlag[srcName]=false;
            for(std::vector<MappedDataPoint>::iterator dIter=mdpList.begin(); dIter != mdpList.end(); ++dIter){
                MappedDataPoint dp = *dIter;
                if (dp.pointType != "Analog Input")
                    continue;
                analogOutValues[dp.index].value=analogValues[srcName][dp.input_index].value;
                analogOutValues[dp.index].timestamp=analogValues[srcName][dp.input_index].timestamp;
            }
            LuaInvokeAnalogScript();
        }
        if(binaryFlag[srcName]){
            binaryFlag[srcName]=false;
            for(std::vector<MappedDataPoint>::iterator dIter=mdpList.begin(); dIter != mdpList.end(); ++dIter){
                MappedDataPoint dp = *dIter;
                if (dp.pointType != "Binary Input")
                    continue;
                binaryOutValues[dp.index].value=binaryValues[srcName][dp.input_index].value;
                binaryOutValues[dp.index].timestamp=binaryValues[srcName][dp.input_index].timestamp;
            }
            LuaInvokeBinaryScript();

        }
        if(counterFlag[srcName]) {
            counterFlag[srcName]=false;
            for(const auto& dp : mdpList) {
                if (dp.pointType != "Counter")
                    continue;
                counterOutValues[dp.index].value=counterValues[srcName][dp.input_index].value;
                counterOutValues[dp.index].timestamp=counterValues[srcName][dp.input_index].timestamp;
            }
        }
        LuaInvokeCounterScript();
    }
}

void MappingOutstation::LuaInvokeAnalogScript()
{
    //lua_checkstack(L, 2*analogValues.size()+20); //ensure enough space in stack for passing table
    //Call function modify_analog defined in lua
    lua_getglobal(L, "modify_analog");
    lua_newtable(L);
    for (int index = 0; index < analogOutValues.size(); ++index){
        lua_pushinteger(L, index+1); //key Lua does not like 0
        lua_pushnumber(L, analogOutValues[index].value);
        lua_settable(L, -3);
    }
    /* call the lua function generate_data, with 1 parameters, return 1 result (a table) */
    lua_call(L, 1, 1);

    asiodnp3::UpdateBuilder builder;
    std::time_t timestamp = std::time(nullptr) * 1000; //OpenDNP3 expects in ms
    //check if we got any tables back
	lua_getglobal(L, "analog_data");//does not have to match the return named variable from lua
	//-1 =>"data", -2=> return from calling lua function
	lua_pushnil(L);
	//-1 =>nil, -2=>"data", -3 =>fn call return val
	if (lua_next(L, -2) == 0) {
		lua_pop(L, lua_gettop(L)); //clean up stack
		return;
	}
	//-1=>value, -2=>key, -3 => "data", -4 =>return val from fn_Call
	lua_pop(L, 2);

	//-1=>"data", -2=>ret value
    lua_pushnil(L);
    //-1=>nil, -2=>"data", -3=>ret val
    while (lua_next(L, -2) != 0) {
        //-1=>value, -2=>key, -3=>"data", -4=>ret val
        /* uses 'key' (at index -2) and 'value' (at index -1) */
        int index = lua_tointeger(L, -2)-1;//lua indexes start at 1
        double dvalue = lua_tonumber(L, -1);
        //printf("Got Analog Input Table %d - %f at timestamp %ld \n", index, dvalue, analogOutValues[index].timestamp);
        //if db is not set up for this data type, break
        if (dpTypeIndexMap.count("Analog Input") == 0) {
            break;
        }
        else if (dpTypeIndexMap["Analog Input"].count(index) == 0) { //if index is not one we want to look at, continue
            //printf("Discarding analog value for point index:%d\n", index);
            lua_pop(L, 1); //if we are going to jump to lua_next loop, restore lua stack to where it should be
            continue;
        }
        //printf("Pushing value %f into index %d for analog\n", dvalue, index);
        builder.Update(opendnp3::Analog(dvalue,
            static_cast<int>(opendnp3::AnalogQuality::ONLINE),
            opendnp3::DNPTime(analogOutValues[index].timestamp)), index);

        /* removes 'value'; keeps 'key' for next iteration */
        lua_pop(L, 1);
        //-1=>key, -2=>"data", -3=>ret val
    }
    //pop whatever is left on stack we are getting out of here. Empty the stack.
    lua_pop(L, lua_gettop(L));
    //stack empty
    outstationInstance->Apply(builder.Build());
}

void MappingOutstation::LuaInvokeBinaryScript()
{
    //Call function modify_binary defined in lua
    lua_getglobal(L, "modify_binary");
    lua_newtable(L);
    for (int index = 0; index < binaryOutValues.size(); ++index){
        lua_pushinteger(L, index+1); //key Lua does not like 0
        lua_pushinteger(L, binaryOutValues[index].value);
        lua_settable(L, -3);
    }
    /* call the lua function generate_data, with 1 parameters, return 1 result (a table) */
    lua_call(L, 1, 1);

    asiodnp3::UpdateBuilder builder;
    std::time_t timestamp = std::time(nullptr) * 1000; //OpenDNP3 expects in ms
    //check if we got any tables back
	lua_getglobal(L, "binary_data");//does not have to match the return named variable from lua
	//-1 =>"data", -2=> return from calling lua function
	lua_pushnil(L);
	//-1 =>nil, -2=>"data", -3 =>fn call return val
	if (lua_next(L, -2) == 0) {
		lua_pop(L, lua_gettop(L)); //clean up stack
		return;
	}
	//-1=>value, -2=>key, -3 => "data", -4 =>return val from fn_Call
	lua_pop(L, 2);

	//-1=>"data", -2=>ret value
    lua_pushnil(L);
    //-1=>nil, -2=>"data", -3=>ret val
    while (lua_next(L, -2) != 0) {
        // uses 'key' (at index -2) and 'value' (at index -1)
        int index = lua_tointeger(L, -2)-1; //lua indexes start at 1
        bool bvalue = lua_tointeger(L, -1);
        //printf("Binary Input %d - %d\n", index, bvalue);
        if (dpTypeIndexMap.count("Binary Input") == 0){
            break;
        }
        else if (dpTypeIndexMap["Binary Input"].count(index) == 0) {
            //printf("Discarding binary value for point index:%d\n", index);
            lua_pop(L, 1); //if we are going to jump to lua_next loop, restore lua stack to where it should be
            continue;
        }
        builder.Update(opendnp3::Binary(bvalue,
            static_cast<int>(opendnp3::BinaryQuality::ONLINE),
             opendnp3::DNPTime(binaryOutValues[index].timestamp)), index);

        // removes 'value'; keeps 'key' for next iteration
        lua_pop(L, 1);
    }
    //pop whatever is left on stack we are getting out of here. Empty the stack.
    lua_pop(L, lua_gettop(L));
    //stack empty
    outstationInstance->Apply(builder.Build());
}

void MappingOutstation::LuaInvokeCounterScript()
{
    //Call function modify_counter defined in lua
    lua_getglobal(L, "modify_counter");
    lua_newtable(L);
    for (int index = 0; index < counterOutValues.size(); ++index){
        lua_pushinteger(L, index+1); //key Lua does not like 0
        lua_pushinteger(L, counterOutValues[index].value);
        lua_settable(L, -3);
    }
    /* call the lua function generate_data, with 1 parameters, return 1 result (a table) */
    lua_call(L, 1, 1);

    asiodnp3::UpdateBuilder builder;
    std::time_t timestamp = std::time(nullptr) * 1000; //OpenDNP3 expects in ms
    //check if we got any tables back
	lua_getglobal(L, "counter_data");//does not have to match the return named variable from lua
	//-1 =>"data", -2=> return from calling lua function
	lua_pushnil(L);
	//-1 =>nil, -2=>"data", -3 =>fn call return val
	if (lua_next(L, -2) == 0){
		lua_pop(L, lua_gettop(L)); //clean up stack
		return;
	}
	//-1=>value, -2=>key, -3 => "data", -4 =>return val from fn_Call
	lua_pop(L, 2);

	//-1=>"data", -2=>ret value
    lua_pushnil(L);
    //-1=>nil, -2=>"data", -3=>ret val
    while (lua_next(L, -2) != 0) {
        // uses 'key' (at index -2) and 'value' (at index -1)
        int index = lua_tointeger(L, -2)-1; //lua indexes start at 1
        uint32_t cvalue = lua_tointeger(L, -1);
        if (dpTypeIndexMap.count("Counter") == 0){
            break;
        }
        else if (dpTypeIndexMap["Counter"].count(index) == 0){
            lua_pop(L, 1); //if we are going to jump to lua_next loop, restore lua stack to where it should be
            continue;
        }
        //printf("Counter %d - %d\n", index, cvalue);
        builder.Update(opendnp3::Counter(cvalue,
            static_cast<int>(opendnp3::CounterQuality::ONLINE),
            opendnp3::DNPTime(counterOutValues[index].timestamp)), index);

        // removes 'value'; keeps 'key' for next iteration
        lua_pop(L, 1);
        //-1=>key, -2=>data["Counter"], -3=>"data", -4=>ret val
    }
    //pop whatever is left on stack we are getting out of here. Empty the stack.
    lua_pop(L, lua_gettop(L));
    //stack empty
    outstationInstance->Apply(builder.Build());
}
