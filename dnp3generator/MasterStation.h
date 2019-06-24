#ifndef ITI_MASTER_H
#define ITI_MASTER_H

#include "Station.h"
#include "MappingOutstation.h"
#include "Node.h"
#include "MappingSoeHandler.h"

#include <string>
#include <list>

struct cmdStruct{
    std::string fName;
    std::string fType;
    int index;
    double value;
};

class MasterStation : public Station
{
public:
    MasterStation(std::unique_ptr<Node>, std::atomic<bool>&);

    void run() override;
    void UpdateDestinations();
    void initialize();

    std::vector<std::shared_ptr<MappingOutstation>> destStations;

private:
    void LuaInvokeScript(std::shared_ptr<asiodnp3::IMaster>);
    void FireOffMasterCommand(std::shared_ptr<asiodnp3::IMaster>, std::list<cmdStruct *>);
    std::shared_ptr<MappingSoeHandler> customDataCallback;
};
#endif
