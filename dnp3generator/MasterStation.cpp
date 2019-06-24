
#include "MappingSoeHandler.h"
#include "MasterStation.h"
#include "dnp3app.h"

#include <unistd.h>
#include <map>
#include <atomic>
#include <vector>
#include <regex>

#include <asiodnp3/DNP3Manager.h>
#include <asiodnp3/PrintingSOEHandler.h>
#include <asiodnp3/DefaultMasterApplication.h>
#include <opendnp3/LogLevels.h>
#include <asiopal/ChannelRetry.h>
#include <asiodnp3/MasterStackConfig.h>
#include <asiodnp3/PrintingChannelListener.h>

extern ThreadSafeUserInput luaSwitchObj;

MasterStation::MasterStation(std::unique_ptr<Node> node, std::atomic<bool>& quitFlag)
    : Station(std::move(node), quitFlag) {
}

void MasterStation::initialize() {
    customDataCallback = nullptr;
    if(!node->boundOutstations.empty()){
        std::shared_ptr<MappingSoeHandler> ss= std::make_shared<MappingSoeHandler>();
        ss->SrcName = node->name;
        customDataCallback = ss;
    }
}

void MasterStation::run()
{
    // Specify what log levels to use. NORMAL is warning and above
    // You can add all the comms logging by uncommenting below
    const uint32_t FILTERS = opendnp3::levels::NORMAL | opendnp3::levels::ALL_APP_COMMS;

    // Connect via a TCPClient socket to a outstation
    auto pChannel = manager->AddTCPClient((node->name + " tcpclient").c_str(), FILTERS, asiopal::ChannelRetry::Default(), node->remote_IPAddress,
                                            node->local_IPAddress, node->port, asiodnp3::PrintingChannelListener::Create());
    // The master config object for a master. The default are
    // useable, but understanding the options are important.
    asiodnp3::MasterStackConfig stackConfig;

    // you can override application layer settings for the master here
    // in this example, we've change the application layer timeout to 2 seconds
    stackConfig.master.responseTimeout = openpal::TimeDuration::Seconds(2);
    stackConfig.master.disableUnsolOnStartup = !node->allowUnsolicited;


    // You can override the default link layer settings here
    // in this example we've changed the default link layer addressing
    stackConfig.link.LocalAddr = node->localDNP3Addr;
    stackConfig.link.RemoteAddr = node->remoteDNP3Addr;

    // Create a new master on a previously declared port, with a
    // name, log level, command acceptor, and config info. This
    // returns a thread-safe interface used for sending commands.
    std::shared_ptr<opendnp3::ISOEHandler> dataCallback(nullptr);
    if(customDataCallback)
        dataCallback = customDataCallback;
    else
        dataCallback = asiodnp3::PrintingSOEHandler::Create();
    auto master = pChannel->AddMaster(
	    node->name.c_str(),								// id for logging
	    dataCallback,	                                // callback for data processing
	    asiodnp3::DefaultMasterApplication::Create(),	// master application instance
	    stackConfig										// stack configuration
	);

 //   std::vector<PollPoint>::iterator pp;
	for(auto& pp : node->pollPoints) {
        if(pp.eventClass == "0123"){
            auto pscan = master->AddClassScan(opendnp3::ClassField::AllClasses(), openpal::TimeDuration::Seconds(pp.frequency));
        }
        else if(pointClassVarMap.count(pp.eventClass) == 1){
            auto pscan = master->AddClassScan(opendnp3::ClassField(pointClassVarMap[pp.eventClass]), openpal::TimeDuration::Seconds(pp.frequency));
        }
    }

    // Enable the master. This will start communications.
    master->Enable();
    if (!node->allowUnsolicited){
        master->PerformFunction("disable unsol", opendnp3::FunctionCode::DISABLE_UNSOLICITED,
                                { opendnp3::Header::AllObjects(60, 2), opendnp3::Header::AllObjects(60, 3), opendnp3::Header::AllObjects(60, 4) }
                               );
    }

    /* initialize Lua */
	L = luaL_newstate();
	/* Load Lua base libraries */
	luaL_openlibs(L);
	/* load the script */
    bool lua_script_exists = node->luaFileNames.size()>0? FileExists(node->luaFileNames[0].c_str()) : false;
    if (lua_script_exists)
        luaL_dofile(L, node->luaFileNames[0].c_str());
    while(!this->quitFlag)
    {
		/* call the lua getdata function */
        if (lua_script_exists){
            while (luaSwitchObj.readCount() > localLuaFlag){
                localLuaFlag+=1;
                if (!node->luaKeySwitch.empty() && luaSwitchObj.readInputStr(localLuaFlag) == node->luaKeySwitch){
                    auto luaFileToLoad = GetNextLuaFile();
                    printf("Master:%s -- Changing lua file to %s\n", node->name.c_str(), luaFileToLoad.c_str());
                    luaL_dofile(L, luaFileToLoad.c_str());
                }
            }
            LuaInvokeScript(master);
        }
	    usleep(node->msToSleep); //sleep for 1 second
    }
	/* cleanup Lua */
	lua_close(L);
}

void MasterStation::UpdateDestinations(){
    if (customDataCallback == nullptr)
        return;
    customDataCallback->DestinationList = destStations;
}

void MasterStation::LuaInvokeScript(std::shared_ptr<asiodnp3::IMaster> master)
{
    lua_getglobal(L, "operate_outstation");

    /* call the lua function generate_data, with 0 parameters, return 1 result (a table) */
    lua_call(L, 0, 1);

    //check if we got any tables back
    lua_getglobal(L, "data");
    lua_pushnil(L);
    if (lua_next(L, -2) == 0){
        return;
    }

    std::list<cmdStruct *> cmdList;

    //empty stack
    lua_getglobal(L, "data");
    //-1=>table
    lua_pushnil(L); //first key
    //-1 => nil, -2 =>table
    while(lua_next(L, -2)){ //lua_next pops top of stack(key), and pushes key-value pair at key
        //-1 => value, -2 => key, -3 => table
        lua_pushvalue(L, -2);
        // -1=> key, -2 =>value, -3=>key, -4=>table
        //now to unravel the inner table
        if(lua_istable(L, -2)){
            lua_pushvalue(L, -2);
            //-1 =>inner table, -2 =>key, -3=>value(innertable), -4=>key, -5=>table
            lua_pushnil(L);
            //-1=>nil, -2=>inner table, -3=>key, -4=>value(innertable), -5=>key, -6 =>table
            cmdStruct *cmd = new cmdStruct();
            while(lua_next(L,-2)){
                //-1 =>value -2 =>key, -3=>inner table, -4 =>key, -5 =>value(innertable), -6=>key, -7=>table
                lua_pushvalue(L, -2);
                //-1=>key, -2=>value, -3=>key, -4 =>innertable, -5=>key, -6=>value(innertable), -7=>key, -8=>table
                int i = lua_tointeger(L, -1) -1; //lua indexes start at 1
                switch(i){
                    case 0:
                        cmd->fType = lua_tostring(L, -2);
                        break;
                    case 1:
                        cmd->fName = lua_tostring(L, -2);
                        break;
                    case 2:
                        cmd->index = lua_tonumber(L, -2);
                        break;
                    case 3:
                        cmd->value = lua_tonumber(L, -2);
                }
                lua_pop(L,2);
                //-1=>key, -2=>innertable, -3=>key, -4=>value(innertable), -5=>key, -6=>table
            }
            cmdList.push_back(cmd);
            //lua_next pops one from stack at the end
            //-1=>innertable, -2=>key, -3=>value(innertable), -4=>key, -5=>table
            lua_pop(L,1);
            //-1=>key, -2=>value(innertable), -3=>key, -4=>table
        }
        // pop value + copy of key, leaving original key
        lua_pop(L, 2);
        //-1=>key, -2 =>table
    }
    //-1=>table
    lua_pop(L,1);
    //stack empty again
    this->FireOffMasterCommand(master, cmdList);
}

void MasterStation::FireOffMasterCommand(std::shared_ptr<asiodnp3::IMaster> master, std::list<cmdStruct *> cmdList){
    auto callback = [](const opendnp3::ICommandTaskResult& result) -> void
    {
        std::cout << "Summary: " << opendnp3::TaskCompletionToString(result.summary) << std::endl;
        auto print = [](const opendnp3::CommandPointResult& res)
        {
            std::cout
                << "Header: " << res.headerIndex
                << " Index: " << res.index
                << " State: " << opendnp3::CommandPointStateToString(res.state)
                << " Status: " << opendnp3::CommandStatusToString(res.status);
        };
        result.ForeachItem(print);
    };

    opendnp3::CommandSet sboCommands;
    opendnp3::CommandSet doCommands;
    opendnp3::CommandSet *cmdSet;
    bool sboFuncCodes = false;
    bool doFuncCodes = false;
    for (auto& c : cmdList) {

        std::string funcName = c->fName;
        std::string funcType = c->fType;
        int index = c->index;
        double value = c->value;

        if(funcType == "SBO"){
            sboFuncCodes = true;
            cmdSet = &sboCommands;
        }
        else if (funcType == "DO"){
            doFuncCodes = true;
            cmdSet = &doCommands;
        }
        else if(funcType == "Scan"){//scans cannot be accumulated, have to be fired off as they are parsed.
            std::regex re("Group([0-9]+)Var([0-9])");
            std::smatch re_match;
            printf("IN SCAN, funcName:%s, funcType:%s!!!!!!!!!!!!!!!!!!!!Matched regex:%d\n", funcName.c_str(), funcType.c_str(), std::regex_match(funcName, re));
            if(std::regex_match(funcName, re_match, re) && re_match.size()==3){
                int groupId = std::stoi(re_match[1].str());
                int varId = std::stoi(re_match[2].str());
                printf("Smart Reading %s, for groupID:%d, var:%d, index %d, to index %d\n", funcType.c_str(), groupId, varId, index, (int)std::round(value));
                master->ScanRange(opendnp3::GroupVariationID(groupId, varId), index, (int)std::round(value));
            }
            continue;
        }
        else{ //we don't recognize what the user is asking us to do
            printf("funcType:%s not recognized. Contact ITI for further information.\n", funcType);
            continue;
        }
        printf("funcName %s, func type:%s, index %d, value %f\n", funcName.c_str(), funcType.c_str(), index, value);
        if(funcName == "CROB"){ //CROB
            auto& header = cmdSet->StartHeader<opendnp3::ControlRelayOutputBlock>();
            if(value == 1){
                header.Add(opendnp3::ControlRelayOutputBlock(opendnp3::ControlCode::PULSE_ON), index);
            }
            else if (value == 2){
                header.Add(opendnp3::ControlRelayOutputBlock(opendnp3::ControlCode::PULSE_OFF), index);
            }
            else if(value == 3){
                header.Add(opendnp3::ControlRelayOutputBlock(opendnp3::ControlCode::LATCH_ON), index);
            }
            else if (value == 4){
                header.Add(opendnp3::ControlRelayOutputBlock(opendnp3::ControlCode::LATCH_OFF), index);
            }
        }
        else if (funcName == "AnalogOutputInt16"){ //AnalogOutputInt16
            auto& header = cmdSet->StartHeader<opendnp3::AnalogOutputInt16>();
            header.Add(opendnp3::AnalogOutputInt16(value), index);
        }
        else if (funcName == "AnalogOutputInt32"){ //AnalogOutputInt32
            auto& header = cmdSet->StartHeader<opendnp3::AnalogOutputInt32>();
            header.Add(opendnp3::AnalogOutputInt32(value), index);
        }
        else if (funcName == "AnalogOutputFloat32"){ //SBO AnalogOutputFloat32
            auto& header = cmdSet->StartHeader<opendnp3::AnalogOutputFloat32>();
            header.Add(opendnp3::AnalogOutputFloat32(value), index);
        }
        else if (funcName == "AnalogOutputDouble64"){ //SBO AnalogOutputDouble64
            auto& header = cmdSet->StartHeader<opendnp3::AnalogOutputDouble64>();
            header.Add(opendnp3::AnalogOutputDouble64(value), index);
        }
    }
    if (sboFuncCodes){
        master->SelectAndOperate(std::move(sboCommands), callback);
    }

    if(doFuncCodes){
        master->DirectOperate(std::move(doCommands), callback);
    }

}
