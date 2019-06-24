#ifndef ITI_NODE_H
#define ITI_NODE_H

#include "DataPoint.h"

#include <string>
#include <vector>
#include <map>
#include <memory>

struct PollPoint {
    int frequency;
    std::string eventClass;
};

class Node
{
public:
    Node();
    Node(std::string name, std::string role);
    ~Node();

    void Allocate();
    void Allocate(const std::string& nic);

    std::string name;
    std::string role; //master or outstation
    int msToSleep;
    std::vector<std::string> luaFileNames;
    std::string luaKeySwitch;

    int port; //generally 20000
    std::string vnic; //nic location if we need to create ip address in CIDR range
    std::string local_IPAddress; //ipv4 address
    std::string remote_IPAddress;

    uint16_t remoteDNP3Addr;
    uint16_t localDNP3Addr;

    std::vector<DataPoint> dataPoints;
    std::map<std::string, std::vector<MappedDataPoint>> dataSources;
    std::map<std::string, int> dbSize;
    std::map<std::string, int> evtBufferSize;

    bool allowUnsolicited;
    std::vector<PollPoint> pollPoints;
    std::vector<std::string> boundOutstations;

};

#endif
