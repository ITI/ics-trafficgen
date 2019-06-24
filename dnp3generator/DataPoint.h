#ifndef ITI_DATAPOINT_H
#define ITI_DATAPOINT_H

#include <string>

struct DataPoint
{
    int index;
    int sVariation;
    int eVariation;
    int eventClass;
    std::string pointType;
    float deadband;

    DataPoint();
};

struct MappedDataPoint
{
    int index;
    int input_index;
    std::string pointType;
};

#endif
