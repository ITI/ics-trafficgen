#ifndef ITI_CFGJSONPARSER_H
#define ITI_CFGJSONPARSER_H

#include "Node.h"
#include "CidrCalculator.h"

#include <string>
#include <vector>
#include <boost/property_tree/ptree.hpp>

using boost::property_tree::ptree;

class CfgJsonParser
{
    public:
        CfgJsonParser(std::string const& filename);
        std::vector<std::unique_ptr<Node>>& GetConfiguredNodes();

    private:
        std::vector<std::unique_ptr<Node>> nodes; //create a vector to keep track of Node objects

        std::string virtualNetworkInterfaceCard;
        std::unique_ptr<CidrCalculator> cidrGenerator;
        std::vector<uint16_t> localDnp3Addrs;
        void readDataGenerationSpecs(const ptree::value_type &,  Node* const);
        void readEventBufferSpecs(const ptree::value_type &, Node* const);

        void readMappedDataGenerationSpecs(const ptree::value_type &, Node* const);
        void readDataPoint(const ptree::value_type &, DataPoint &);

        void readPollingSpecs       (const ptree::value_type &,  Node* const);
        void readBoundOutstations   (const ptree::value_type &, Node* const);

        void readAllDnp3Addresses(const ptree &propTree);
        void readAssignDNP3Addresses(const ptree::value_type &, Node* const, Node* const);
        int  getNextDNP3Address();

        void readUnsolicited        (const ptree::value_type &, Node* const, Node* const);
        void readSleepTimes         (const ptree::value_type &, Node* const, Node* const);
        void readLuaFileNames       (const ptree::value_type &, Node* const, Node* const);
        void readLuaFileKeySwitch   (const ptree::value_type &, Node* const, Node* const);

        void readIpPorts            (const ptree::value_type &, Node* const, Node* const);
        void readAssignIpAddresses  (const ptree::value_type &, Node* const, Node* const);
        void allocateIpAddress(Node* const, const std::string&);
};

#endif
