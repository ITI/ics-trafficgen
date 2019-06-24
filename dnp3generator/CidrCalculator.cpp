
#include "CidrCalculator.h"
#include "StringUtilities.h"

#include <sstream>
#include <numeric>
#include <vector>
#include <list>

CidrCalculator::CidrCalculator(const std::string& cidr) : cidrNotation(cidr), netMask{0,0,0,0}, networkAddr{0,0,0,0} {
    std::list<std::string> ipAddrs;
    std::vector<std::string> parts = split(this->cidrNotation, '/');
    if (parts.size() != 2){
        //throw std::runtime_error("Network address not in CIDR notation");
    }
    std::string addrStr = parts.front();
    int cidrI;
    std::stringstream(parts.back()) >> cidrI;

    std::vector<int> addr;
    for (std::string value: split(addrStr, '.')){
        int dec;
        std::stringstream(value) >> dec;
        addr.push_back(dec);
    }
    /*
    printf("addr: ");
    for(int i = 0; i<4; i++){
        printf("%d.", addr[i]);
    }
    printf("\n");
    printf("mask: ");
    for(int i = 0; i<4; i++){
        printf("%d.", this->netMask[i]);
    }
    printf("\n");
    */
    std::vector<int> cidrRange(cidrI);
    std::iota(cidrRange.begin(), cidrRange.end(), 0);
    for (int i: cidrRange){
        this->netMask[i/8] = this->netMask[i/8] + (1 <<(7-i%8));
    }
    /*
    printf("mask: ");
    for(int i = 0; i<4; i++){
        printf("%d.", this->netMask[i]);
    }
    printf("\n");
    */
    for (int i=0; i<4; i++){
        this->networkAddr[i] = addr[i] & this->netMask[i];
    }

    /*
    printf("netaddr: ");
    for(int i = 0; i<4; i++){
        printf("%d.", this->networkAddr[i]);
    }
    printf("\n");
    */
};

std::string CidrCalculator::GetNextIpAddress(){
    //if(this->networkAddr[0] == 0 || this->networkAddr[0] == 255)
        //throw std::runtime_error("Check network address")
        //return "Check network address";

    if (this->networkAddr[3] < 254){
        this->networkAddr[3] += 1;
    } else {
        this->networkAddr[3] = 1;
        if (this->networkAddr[2] < 254){
            this->networkAddr[2] += 1;
        } else {
            this->networkAddr[2] = 0;
            if (this->networkAddr[1] < 254){
                this->networkAddr[1] += 1;
            } else {
                this->networkAddr[1] = 0;
                if (this->networkAddr[0] < 254){
                    this->networkAddr[0] += 1;
                } else {
                    //throw std::runtime_error("Check network address");
                }
            }
        }
    }
    std::stringstream ipAddress;
    ipAddress << this->networkAddr[0] << "." << this->networkAddr[1] << "." << this->networkAddr[2] << "." << this->networkAddr[3];
    return ipAddress.str();
}
