
#include "CfgJsonParser.h"

#include <iostream>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>
#include <boost/optional.hpp>
#include <boost/foreach.hpp>

#include <regex>

#define EVENT_BUFFER_SIZE 100

using boost::property_tree::ptree;
using boost::optional;
using boost::property_tree::read_json;
using boost::property_tree::write_json;

void CfgJsonParser::readAllDnp3Addresses(const ptree &propTree){
    BOOST_FOREACH(const boost::property_tree::ptree::value_type &v, propTree.get_child("Nodes")) {
        boost::optional<int> dnp3Addr = v.second.get_optional<int>("DNP3 Address.Master");
        if (dnp3Addr){
            localDnp3Addrs.push_back(*dnp3Addr);
        }
        dnp3Addr = v.second.get_optional<int>("DNP3 Address.Outstation");
        if (dnp3Addr){
            localDnp3Addrs.push_back(*dnp3Addr);
        }
    }
}

CfgJsonParser::CfgJsonParser(const std::string &configFile){
    ptree propTree;
    read_json(configFile, propTree);
    int vnictrack = 0;
    boost::optional<std::string> virtInterfaceProp = propTree.get_optional<std::string>("Virtual Interface");
    if (virtInterfaceProp){
        this->virtualNetworkInterfaceCard = *virtInterfaceProp;
    }
    boost::optional<std::string> cidrProp = propTree.get_optional<std::string>("CIDR Notation");
    if (cidrProp){
        this->cidrGenerator = std::unique_ptr<CidrCalculator>(new CidrCalculator(*cidrProp));
    }
    this->readAllDnp3Addresses(propTree);
    BOOST_FOREACH(const ptree::value_type &nodeTree, propTree.get_child("Nodes")) {
        std::unique_ptr<Node> master(nullptr);
        std::unique_ptr<Node> outstation(nullptr);
        boost::optional<std::string> masterName = nodeTree.second.get_optional<std::string>("Name.Master");
        boost::optional<std::string> outStnName = nodeTree.second.get_optional<std::string>("Name.Outstation");
        if (masterName){
            master = std::unique_ptr<Node>(new Node(*masterName, "Master"));
        }
        if (outStnName){
            outstation = std::unique_ptr<Node>(new Node(*outStnName, "Outstation"));
        }
        readAssignIpAddresses(nodeTree, master.get(), outstation.get());
        readAssignDNP3Addresses(nodeTree, master.get(), outstation.get());
        readIpPorts(nodeTree, master.get(), outstation.get());
        readUnsolicited(nodeTree, master.get(), outstation.get());
        readSleepTimes(nodeTree, master.get(), outstation.get());
        readLuaFileNames(nodeTree, master.get(), outstation.get());
        readLuaFileKeySwitch(nodeTree, master.get(), outstation.get());

        if (masterName) {
            readPollingSpecs(nodeTree, master.get());
            readBoundOutstations(nodeTree, master.get());
            //printf("######Master IP ADdr:%s, remote IP:%s\n", master->local_IPAddress.c_str(), master->remote_IPAddress.c_str());
            nodes.push_back(std::move(master));
        }

        if (outstation) {
            //printf("Reading in details of outstation %s\n", outstation->name.c_str());
            readDataGenerationSpecs(nodeTree, outstation.get());
            readMappedDataGenerationSpecs(nodeTree, outstation.get());
            readEventBufferSpecs(nodeTree, outstation.get());
            this->nodes.push_back(std::move(outstation));
        }
    }
}

void CfgJsonParser::readBoundOutstations(const ptree::value_type & nodeTree, Node* const master){
    optional<const ptree&> outStnList = nodeTree.second.get_child_optional("Bound Outstations");
    if (outStnList){
        BOOST_FOREACH(const ptree::value_type& pointTree, nodeTree.second.get_child("Bound Outstations")) {
            master->boundOutstations.push_back(pointTree.second.get_value<std::string>());
        }
    }
}


void CfgJsonParser::readUnsolicited(const ptree::value_type & nodeTree, Node* const master, Node* const outstation){
    boost::optional<bool> allowUnsolicited = nodeTree.second.get_optional<bool>("Allow Unsolicited");
    if (allowUnsolicited && *allowUnsolicited==true) {
        if (master != nullptr)
            master->allowUnsolicited = *allowUnsolicited;

        if (outstation != nullptr)
            outstation->allowUnsolicited = *allowUnsolicited;
    }
}

void CfgJsonParser::readSleepTimes(const ptree::value_type & nodeTree, Node* const master, Node* const outstation){
    boost::optional<float> sleepMaster = nodeTree.second.get_optional<float>("Sleep Duration.Master");
    boost::optional<float> sleepOutStn = nodeTree.second.get_optional<float>("Sleep Duration.Outstation");
    if (sleepMaster && master != nullptr)
        master->msToSleep = static_cast<int>(*sleepMaster *1000*1000);

    if (sleepOutStn && outstation != nullptr)
        outstation->msToSleep = static_cast<int>(*sleepOutStn *1000*1000);
}

void CfgJsonParser::readLuaFileNames(const ptree::value_type &nodeTree, Node* const master, Node* const outstation){
    optional<const ptree&> masterLua = nodeTree.second.get_child_optional("Lua File.Master");
    optional<const ptree&> outStnLuaList = nodeTree.second.get_child_optional("Lua File.Outstation");
    if (masterLua && master != nullptr){
        master->luaFileNames.clear();
        BOOST_FOREACH(const ptree::value_type& luaTree, nodeTree.second.get_child("Lua File.Master")) {
            master->luaFileNames.push_back(luaTree.second.get_value<std::string>());
        }
    }
    if(outStnLuaList && outstation != nullptr){
        outstation->luaFileNames.clear(); //clear out any defaults
        BOOST_FOREACH(const ptree::value_type& luaTree, nodeTree.second.get_child("Lua File.Outstation")) {
            outstation->luaFileNames.push_back(luaTree.second.get_value<std::string>());
        }
    }
}

void CfgJsonParser::readLuaFileKeySwitch(const ptree::value_type &nodeTree, Node* const master, Node* const outstation){
    boost::optional<std::string> masterLuaKey = nodeTree.second.get_optional<std::string>("Lua Switch Trigger.Master");
    boost::optional<std::string> outStnLuaKey = nodeTree.second.get_optional<std::string>("Lua Switch Trigger.Outstation");
    if (masterLuaKey && master != nullptr){
        master->luaKeySwitch = *masterLuaKey;
    }
    if(outStnLuaKey && outstation != nullptr){
        outstation->luaKeySwitch = *outStnLuaKey;
    }
}

void CfgJsonParser::readAssignIpAddresses(const ptree::value_type &nodeTree, Node* const master, Node* const outstation){
    //is Master IP Address specified in the json config file
    std::string outAddr;
    boost::optional<std::string> ipAddr = nodeTree.second.get_optional<std::string>("IP Address.Master");

    if (master != nullptr) {
        if (ipAddr){
            master->local_IPAddress = *ipAddr;
        } else {
            if (cidrGenerator){
                allocateIpAddress(master, cidrGenerator->GetNextIpAddress()); //if not in config, assign one.
            } else{
                std::cout<<"Did not find CIDR notation in the config file to allocate IP address\n";
            }
        }
    }

    //Outstation IP Address assigned in the json config file
    ipAddr = nodeTree.second.get_optional<std::string>("IP Address.Outstation");
    if (ipAddr) {
        if (master != nullptr)
            master->remote_IPAddress = *ipAddr;
        if (outstation != nullptr)
            outstation->local_IPAddress = *ipAddr;
    } else {
        if (cidrGenerator && outstation != nullptr) {
            allocateIpAddress(outstation, cidrGenerator->GetNextIpAddress()); //if not in config assign one.
        } else {
            std::cout<<"Did not find CIDR notation in the config file to allocate IP address\n";
        }
    }

    if (master != nullptr && outstation != nullptr)
        master->remote_IPAddress = outstation->local_IPAddress; //master needs to know outstation IP address that it is connected to.
}

void CfgJsonParser::readIpPorts(const ptree::value_type & nodeTree, Node* const master, Node* const outstation) {

    boost::optional<int> ipPort = nodeTree.second.get_optional<int>("IP Port.Master");
    if (ipPort && master != nullptr){
        master->port = *ipPort;
    }
    ipPort = nodeTree.second.get_optional<int>("IP Port.Outstation");
    if (ipPort && outstation != nullptr) {
        outstation->port = *ipPort;
    }
}

void CfgJsonParser::allocateIpAddress(Node* const node, const std::string& ipAddress) {
    static int vnictrack = 0;
    node->local_IPAddress = ipAddress;
    node->vnic = virtualNetworkInterfaceCard + ":" + std::to_string(vnictrack);
    ++vnictrack;
    node->Allocate();
    printf("Node allocated at IP ADDress:%s, vnic:%s\n", node->local_IPAddress.c_str(), node->vnic.c_str());
}

void CfgJsonParser::readDataGenerationSpecs(const ptree::value_type & nodeTree, Node* const station) {
    optional<const ptree&> dataTree = nodeTree.second.get_child_optional( "Data" );
    auto pushPtToStn = [&](DataPoint &dpt){
        if(station->dbSize[dpt.pointType] < dpt.index+1){
            station->dbSize[dpt.pointType] = dpt.index+1;
        }
        station->dataPoints.push_back(dpt);
    };
    if (dataTree){
        BOOST_FOREACH(const ptree::value_type &pointTree, nodeTree.second.get_child("Data")) {
            DataPoint dp = DataPoint();
            readDataPoint(pointTree, dp);
            boost::optional<int> index = pointTree.second.get_optional<int>("Index");
            boost::optional<std::string> indexList = pointTree.second.get_optional<std::string>("Index List");
            if (index){
                dp.index = *index;
                pushPtToStn(dp);
            }
            if (indexList){
                std::string indexes = *indexList;
                std::regex re("([0-9]+)-*([0-9]+)*");
                for(std::sregex_iterator reg_i = std::sregex_iterator(indexes.begin(), indexes.end(), re); reg_i != std::sregex_iterator(); ++reg_i)
                {
                    std::smatch mtch = *reg_i;
                    int startIndex = std::stoi(mtch[1]);
                    int endIndex = startIndex;
                    try{ //mtch.size() is not useful to see if we got second match?
                        endIndex = std::stoi(mtch[2]);
                    } catch(...){}

                    for(int i = startIndex; i <= endIndex; i++){
                        DataPoint dpI = DataPoint(dp);
                        dpI.index = i;
                        pushPtToStn(dpI);
                    }
                }
            }
        }
        for(auto const& pval: station->dbSize){
            if(pval.second > 0){
                station->evtBufferSize[pval.first.c_str()] = EVENT_BUFFER_SIZE;
            }
        }
    }
}

void CfgJsonParser::readDataPoint(const ptree::value_type &pointTree, DataPoint &dp){
    dp.pointType = pointTree.second.get<std::string>("Type");
    dp.eventClass = pointTree.second.get<int>("Event Class");
    dp.sVariation = pointTree.second.get<int>("sVariation");
    dp.eVariation = pointTree.second.get<int>("eVariation");
    boost::optional<float> deadband = pointTree.second.get_optional<float>("Deadband");
    if(deadband){
        dp.deadband = *deadband;
    }
}

void CfgJsonParser::readEventBufferSpecs(const ptree::value_type &nodeTree, Node* const station)
{
    optional<const ptree&> bufferTree = nodeTree.second.get_child_optional( "Event Data" );
    if (bufferTree){
        BOOST_FOREACH(const ptree::value_type& pTree, nodeTree.second.get_child("Event Data")) {
            station->evtBufferSize[pTree.second.get<std::string>("Type")] = pTree.second.get<int>("Size");
        }
    }

}

void CfgJsonParser::readMappedDataGenerationSpecs(const ptree::value_type &nodeTree, Node* const station){
    optional<const ptree&> sourceTree = nodeTree.second.get_child_optional("Data Sources");
    if (sourceTree){
        BOOST_FOREACH(const ptree::value_type &sTree, nodeTree.second.get_child("Data Sources")) {
            auto source = sTree.second.get<std::string>("Source");
            optional<const ptree&> dataTree = sTree.second.get_child_optional("Mapped Data");
            if(dataTree){
                BOOST_FOREACH(const ptree::value_type &pointTree, sTree.second.get_child("Mapped Data")) {
                    MappedDataPoint mdp = MappedDataPoint();
                    DataPoint dp = DataPoint();
                    readDataPoint(pointTree, dp);
                    dp.index = pointTree.second.get<int>("Index");
                    if(station->dbSize[dp.pointType] < dp.index+1){
                        station->dbSize[dp.pointType] = dp.index+1;
                    }
                    station->dataPoints.push_back(dp);

                    mdp.input_index = pointTree.second.get<int>("InputIndex");
                    mdp.index = dp.index;
                    mdp.pointType = dp.pointType;
                    station->dataSources[source].push_back(mdp);
                }
                for(auto const& pval: station->dbSize){
                    if(pval.second > 0){
                        station->evtBufferSize[pval.first.c_str()] = EVENT_BUFFER_SIZE;
                    }
                }
            }
        }
    }
}


void CfgJsonParser::readPollingSpecs (const ptree::value_type &nodeTree,  Node* const station){
    optional<const ptree&> pollTree = nodeTree.second.get_child_optional( "Poll Interval" );
    if (pollTree){
        BOOST_FOREACH(const ptree::value_type &pTree, nodeTree.second.get_child("Poll Interval")) {
            PollPoint ppt;
            ppt.eventClass = pTree.second.get<std::string>("Event Class");
            ppt.frequency = pTree.second.get<int>("Frequency");
            station->pollPoints.push_back(ppt);
        }
    }
}


void CfgJsonParser::readAssignDNP3Addresses(const ptree::value_type & nodeTree, Node* const master, Node* const outstation) {

    boost::optional<int> masterCfgDnp3Addr = nodeTree.second.get_optional<int>("DNP3 Address.Master");
    boost::optional<int> outCfgDnp3Addr = nodeTree.second.get_optional<int>("DNP3 Address.Outstation");

    int mAddr;
    if (masterCfgDnp3Addr){
        mAddr = *masterCfgDnp3Addr;
    } else {
        mAddr = getNextDNP3Address();
    }

    int oAddr;
    if (outCfgDnp3Addr){
        oAddr = *outCfgDnp3Addr;
    } else {
        oAddr = getNextDNP3Address();
    }

    if (master != nullptr) {
        master->localDNP3Addr = mAddr;
        master->remoteDNP3Addr = oAddr;
    }

    if (outstation != nullptr) {
        outstation->localDNP3Addr = oAddr;
        outstation->remoteDNP3Addr = mAddr;
    }
}

int CfgJsonParser::getNextDNP3Address(){
    int allocatableDnp3Addr;
    if (localDnp3Addrs.empty()) {
        allocatableDnp3Addr = 90;
    } else {
        std::vector<uint16_t>::iterator largestAllocatedDnp3Addr = std::max_element(std::begin(localDnp3Addrs), std::end(localDnp3Addrs));
        allocatableDnp3Addr = *largestAllocatedDnp3Addr + 1;
    }
    localDnp3Addrs.push_back(allocatableDnp3Addr);
    return allocatableDnp3Addr;
}

std::vector<std::unique_ptr<Node>>& CfgJsonParser::GetConfiguredNodes() {
    return this->nodes;
}
