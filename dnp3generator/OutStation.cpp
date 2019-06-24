
#include "Node.h"
#include "OutStation.h"
#include "dnp3app.h"

#include <unistd.h>
#include <map>
#include <numeric>
#include <atomic>
#include <ctime>
#include <chrono>

#include <asiodnp3/DNP3Manager.h>
#include <asiodnp3/PrintingSOEHandler.h>
#include <asiodnp3/ConsoleLogger.h>
#include <opendnp3/outstation/SimpleCommandHandler.h>
#include <opendnp3/LogLevels.h>
#include <opendnp3/outstation/DatabaseSizes.h>
#include <opendnp3/app/DNPTime.h>
#include <asiodnp3/UpdateBuilder.h>
#include <asiopal/ChannelRetry.h>
#include <asiodnp3/PrintingChannelListener.h>
#include <asiodnp3/OutstationStackConfig.h>

extern ThreadSafeUserInput luaSwitchObj;
void OutStation::LuaInvokeScript(std::shared_ptr<asiodnp3::IOutstation> outstation)
{
	//stack empty
    lua_getglobal(L, "generate_data");
    /* call the lua function generate_data, with 0 parameters, return 1 result (a table) */
    lua_call(L, 0, 1);
	//-1 =>return from fn call
	//check if we got any tables back
	lua_getglobal(L, "data");
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

	asiodnp3::UpdateBuilder builder;
	/*
	//get the timestamp from data
	lua_pushstring(L, "Timestamp");
	//-1 => "Timestamp", -2 =>data, -3=>ret_value
	lua_gettable(L, -2);
	//-1=>data["Timestamp"], -2=>"Timestamp", -3=>data, -4=>ret value
	float timestamp = lua_tonumber(L, -1);
	printf("***********Got time stamp %f\n", timestamp);
	lua_pop(L, 2);
	//-1=>data, -2=>ret value
	*/
    auto duration = std::chrono::system_clock::now().time_since_epoch();
    auto timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(duration).count();//OpenDNP3 expects in ms
	//Get Analog Input table
	lua_pushstring(L, "Analog Input");
	//-1=> "Analog Input", -2=>"data", -3 =>ret value
	lua_gettable(L, -2); //gets table at -2, with key at top of stack, so gets data["Analaog Input"]
	//-1=>data["AnalogInput"], -2=>"data", -3=>ret val
	if(lua_istable(L, -1) == 1){
		//printf("In Analog input table");
		lua_pushnil(L);
		//-1=>nil, -2=>data["Analog Input"], -3=>"data", -4=>ret val
		while (lua_next(L, -2) != 0) {
			//-1=>value, -2=>key, -3=>data["Analog Input"], -4=>"data", -5=>ret val
	  		/* uses 'key' (at index -2) and 'value' (at index -1) */
			int index = lua_tointeger(L, -2)-1;//lua indexes start at 1
			double dvalue = lua_tonumber(L, -1);
			//printf("Got Analog Input Table %d - %f\n", index, dvalue);
			//if db is not set up for this data type, break
			if (dpTypeIndexMap.count("Analog Input") == 0){
				lua_pop(L, 2); //if we are going to break out of while lua_next loop, restore lua stack to where it should be
				break;
			}
			else if (dpTypeIndexMap["Analog Input"].count(index) == 0){ //if index is not one we want to look at, continue
				//printf("Discarding analog value for point index:%d\n", index);
				lua_pop(L, 1); //if we are going to jump to lua_next loop, restore lua stack to where it should be
				continue;
			}
			builder.Update(opendnp3::Analog(dvalue,
				static_cast<int>(opendnp3::AnalogQuality::ONLINE),
				opendnp3::DNPTime(timestamp)), index);

	  		/* removes 'value'; keeps 'key' for next iteration */
	  		lua_pop(L, 1);
			//-1=>key, -2=>data["Analaog Input"], -3=>"data", -4=>ret val
		}
	}
	//-1=>data["AnalogInput"], -2=>"data", -3=>ret val
	lua_pop(L, 1);
	//-1=>"data", -2=>ret val

	//REady to get "Counter" table
	lua_pushstring(L, "Counter");
	//-1=>"Counter", -2=>"data", -3=>ret val
	lua_gettable(L, -2);
	if(lua_istable(L, -1) == 1){
		lua_pushnil(L);
		//printf("***********Retrieving counter tabel\n");
		while (lua_next(L, -2) != 0) {
	  		// uses 'key' (at index -2) and 'value' (at index -1)
			int index = lua_tointeger(L, -2)-1; //lua indexes start at 1
			uint32_t cvalue = lua_tonumber(L, -1);
			if (dpTypeIndexMap.count("Counter") == 0){
				lua_pop(L, 2);
				break;
			}
			else if (dpTypeIndexMap["Counter"].count(index) == 0){
				lua_pop(L, 1); //if we are going to jump to lua_next loop, restore lua stack to where it should be
				continue;
			}
			//printf("Counter %d - %d\n", index, cvalue);
			builder.Update(opendnp3::Counter(cvalue,
				static_cast<int>(opendnp3::CounterQuality::ONLINE),
				opendnp3::DNPTime(timestamp)), index);

	  		// removes 'value'; keeps 'key' for next iteration
	  		lua_pop(L, 1);
			//-1=>key, -2=>data["Counter"], -3=>"data", -4=>ret val
		}
	}
	//-1=>data["Counter"], -2=>"data", -3=>ret val
	lua_pop(L, 1);
	//-1=>"data", -2=>ret val

	//Ready for Binary Input table
	lua_pushstring(L, "Binary Input");
	lua_gettable(L, -2);
	if(lua_istable(L, -1) == 1){
		lua_pushnil(L);
		//printf("***********Retrieveing Binary Inputs table\n");
		while (lua_next(L, -2) != 0) {
	  		// uses 'key' (at index -2) and 'value' (at index -1)
			int index = lua_tointeger(L, -2)-1; //lua indexes start at 1
			bool bvalue = lua_tointeger(L, -1);
			//printf("Binary Input %d - %d\n", index, bvalue);
			if (dpTypeIndexMap.count("Binary Input") == 0){
				lua_pop(L, 2);
				break;
			}
			else if (dpTypeIndexMap["Binary Input"].count(index) == 0){
				//printf("Discarding binary value for point index:%d\n", index);
				lua_pop(L, 1); //if we are going to jump to lua_next loop, restore lua stack to where it should be
				continue;
			}
			builder.Update(opendnp3::Binary(bvalue,
				static_cast<int>(opendnp3::BinaryQuality::ONLINE),
				opendnp3::DNPTime(timestamp)), index);

	  		// removes 'value'; keeps 'key' for next iteration
	  		lua_pop(L, 1);
		}
	}
	//pop whatever is left on stack we are getting out of here. Empty the stack.
	lua_pop(L, lua_gettop(L));
	//stack empty
	outstation->Apply(builder.Build());
}

OutStation::OutStation(std::unique_ptr<Node> node, std::atomic<bool>& quitFlag)
	: Station(std::move(node), quitFlag)
{
}

void OutStation::run()
{
    const uint32_t FILTERS = opendnp3::levels::NORMAL | opendnp3::levels::ALL_COMMS ;

    // Create a TCP server (listener)
	auto channel = manager->AddTCPServer((node->name + " server").c_str(),
					FILTERS, asiopal::ChannelRetry::Default(),
					node->local_IPAddress, node->port,
					asiodnp3::PrintingChannelListener::Create());

    // The main object for a outstation. The defaults are useable,
    // but understanding the options are important.
    asiodnp3::OutstationStackConfig stackConfig(opendnp3::DatabaseSizes(
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
    stackConfig.outstation.eventBufferConfig = opendnp3::EventBufferConfig(
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
    stackConfig.outstation.params.allowUnsolicited = node->allowUnsolicited;

    // You can override the default link layer settings here
    // in this example we've changed the default link layer addressing
    stackConfig.link.LocalAddr = node->localDNP3Addr;
    stackConfig.link.RemoteAddr = node->remoteDNP3Addr;
	stackConfig.link.KeepAliveTimeout = openpal::TimeDuration::Max();

	// You can optionally change the default reporting variations or class assignment prior to enabling the outstation
    ConfigureDatabase(stackConfig.dbConfig);

    // Create a new outstation with a log level, command handler, and
    // config info this	returns a thread-safe interface used for
    // updating the outstation's database.
    auto outstation = channel->AddOutstation(node->name.c_str(), opendnp3::SuccessCommandHandler::Create(),
											opendnp3::DefaultOutstationApplication::Create(), stackConfig);

    // Enable the outstation and start communications
    outstation->Enable();

	/* initialize Lua */
	L = luaL_newstate();
	/* Load Lua base libraries */
	luaL_openlibs(L);
	/* load the script */
    bool lua_script_exists = node->luaFileNames.size() >0 ? true : false;//FileExists(node->luaFileName.c_str());
	luaL_dofile(L, node->luaFileNames[0].c_str());

    while(!this->quitFlag)
    {
		/* call the lua getdata function */
        if (lua_script_exists){
            while (luaSwitchObj.readCount() > localLuaFlag){
                localLuaFlag+=1;
                if (!node->luaKeySwitch.empty() && luaSwitchObj.readInputStr(localLuaFlag) == node->luaKeySwitch){
                    auto luaFileToLoad = GetNextLuaFile();
                    printf("Outstation:%s -- Changing lua file to %s\n", node->name.c_str(), luaFileToLoad.c_str());
                    luaL_dofile(L, luaFileToLoad.c_str());
                }
            }
            LuaInvokeScript(outstation);
        }
	    usleep(node->msToSleep);
    }
	/* cleanup Lua */
	lua_close(L);
}
