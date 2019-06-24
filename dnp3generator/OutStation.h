#ifndef ITI_OUTSTATION_H
#define ITI_OUTSTATION_H

#include "DataPoint.h"
#include "Station.h"
#include <asiodnp3/DatabaseConfig.h>
#include <atomic>

class OutStation : public Station
{
public:
	OutStation(std::unique_ptr<Node>, std::atomic<bool>&);
	void run() override;

protected:
    void LuaInvokeScript(std::shared_ptr<asiodnp3::IOutstation>);
};
#endif
