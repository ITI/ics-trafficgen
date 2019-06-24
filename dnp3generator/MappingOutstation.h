#ifndef ITI_MAPPINGOUTSTATION_H
#define ITI_MAPPINGOUTSTATION_H

#include "OutStation.h"
#include <map>
#include <set>
#include <atomic>
#include <mutex>

#include <asiodnp3/IOutstation.h>

struct dnp3Point {
    double value;
    std::time_t timestamp;
};

class MappingOutstation : public OutStation
{
    public:
        MappingOutstation(std::unique_ptr<Node>, std::atomic<bool>&);
        void initialize();

        void run() override;

        std::shared_ptr<asiodnp3::IOutstation> outstationInstance; //TODO Can be localized later
		std::mutex mutex;
        std::map<std::string, std::vector<dnp3Point>> analogValues; //these are input arrays
		std::map<std::string, std::vector<dnp3Point>> binaryValues;
		std::map<std::string, std::vector<dnp3Point>> counterValues;
		std::map<std::string, std::atomic<bool>> analogFlag;
		std::map<std::string, std::atomic<bool>> binaryFlag;
		std::map<std::string, std::atomic<bool>> counterFlag;

        std::vector<dnp3Point> analogOutValues;
        std::vector<dnp3Point> binaryOutValues;
        std::vector<dnp3Point> counterOutValues;

	private:
		void LuaInvokeAnalogScript();
		void LuaInvokeBinaryScript();
		void LuaInvokeCounterScript();

        void ConfigureInputMultiplexingArrays();
        void passThruDataChanges();
        void injectDataChanges();
};
#endif
