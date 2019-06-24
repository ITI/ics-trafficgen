#ifndef ITI_CIDRCALCULATOR_H
#define ITI_CIDRCALCULATOR_H

#include <string>

class CidrCalculator{
public:
    CidrCalculator(const std::string& cidr);
    std::string GetNextIpAddress();

private:
    std::string cidrNotation;
    int netMask[4];
    int networkAddr[4];
};

#endif
