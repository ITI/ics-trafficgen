#ifndef ITI_STATION_H
#define ITI_STATION_H

#include "Node.h"

#include <map>
#include <set>
#include <atomic>

#include <asiodnp3/DNP3Manager.h>
#include <asiodnp3/DatabaseConfig.h>

extern "C" {
	#include <lua5.2/lua.h>
	#include <lua5.2/lualib.h>
	#include <lua5.2/lauxlib.h>
}
/***************** Analog Input variations ***************************/
extern std::map <int, opendnp3::StaticAnalogVariation> sAnalogInputVarMap;
extern std::map <int, opendnp3::EventAnalogVariation> eAnalogInputVarMap;
/***************** Counter variations ***************************/
extern std::map <int, opendnp3::StaticCounterVariation> sCounterVarMap ;
extern std::map <int, opendnp3::EventCounterVariation> eCounterVarMap;
/***************** Frozen Counter variations ***************************/
extern std::map <int, opendnp3::StaticFrozenCounterVariation> sFrozenCounterVarMap;
extern std::map <int, opendnp3::EventFrozenCounterVariation> eFrozenCounterVarMap;
/***************** Binary Input variations ***************************/
extern std::map <int, opendnp3::StaticBinaryVariation> sBinaryVarMap ;
extern std::map <int, opendnp3::EventBinaryVariation> eBinaryVarMap;
/***************** Double Binary Input variations ********************/
extern std::map <int, opendnp3::StaticDoubleBinaryVariation> sDoubleBinaryVarMap;
extern std::map <int, opendnp3::EventDoubleBinaryVariation> eDoubleBinaryVarMap;

extern std::map <std::string, opendnp3::PointClass> pointClassVarMap;

class Station
{
public:
    Station(std::unique_ptr<Node>, std::atomic<bool> &);
    virtual ~Station();

    virtual void run() = 0;
    void ConfigureDatabase(asiodnp3::DatabaseConfig&);
    bool FileExists(const char *fileName);
	std::string GetNextLuaFile();

    std::unique_ptr<Node> node;
    asiodnp3::DNP3Manager* manager;
    std::map<std::string, std::set<int>> dpTypeIndexMap;

protected:
	std::atomic<bool> &quitFlag;
	lua_State *L;
	int localLuaFlag; //dont need our in thread flag to be atomic
	int currentLuaFileIndex;
};

#endif
