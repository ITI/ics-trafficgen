#ifndef ITI_MappingSoeHandler_H
#define ITI_MappingSoeHandler_H

#include "MappingOutstation.h"

#include <opendnp3/master/ISOEHandler.h>
#include <opendnp3/master/HeaderInfo.h>
#include <opendnp3/app/parsing/ICollection.h>
#include <opendnp3/app/Indexed.h>
#include <opendnp3/app/SecurityStat.h>
#include <asiodnp3/UpdateBuilder.h>
#include <iostream>
#include <sstream>
#include <memory>
#include <map>

/**
*	MappingSoeHandler singleton that prints to the console.
*/

class MappingSoeHandler final : public opendnp3::ISOEHandler
{

public:

	MappingSoeHandler()
	{}
	std::string SrcName;
	std::vector<std::shared_ptr<MappingOutstation>> DestinationList;

	virtual void Process(const opendnp3::HeaderInfo& info, const opendnp3::ICollection<opendnp3::Indexed<opendnp3::Binary>>& values) override;
	virtual void Process(const opendnp3::HeaderInfo& info, const opendnp3::ICollection<opendnp3::Indexed<opendnp3::DoubleBitBinary>>& values) override;
	virtual void Process(const opendnp3::HeaderInfo& info, const opendnp3::ICollection<opendnp3::Indexed<opendnp3::Analog>>& values) override;
	virtual void Process(const opendnp3::HeaderInfo& info, const opendnp3::ICollection<opendnp3::Indexed<opendnp3::Counter>>& values) override;
	virtual void Process(const opendnp3::HeaderInfo& info, const opendnp3::ICollection<opendnp3::Indexed<opendnp3::FrozenCounter>>& values) override;
	virtual void Process(const opendnp3::HeaderInfo& info, const opendnp3::ICollection<opendnp3::Indexed<opendnp3::BinaryOutputStatus>>& values) override;
	virtual void Process(const opendnp3::HeaderInfo& info, const opendnp3::ICollection<opendnp3::Indexed<opendnp3::AnalogOutputStatus>>& values) override;
	virtual void Process(const opendnp3::HeaderInfo& info, const opendnp3::ICollection<opendnp3::Indexed<opendnp3::OctetString>>& values) override;
	virtual void Process(const opendnp3::HeaderInfo& info, const opendnp3::ICollection<opendnp3::Indexed<opendnp3::TimeAndInterval>>& values) override;
	virtual void Process(const opendnp3::HeaderInfo& info, const opendnp3::ICollection<opendnp3::Indexed<opendnp3::BinaryCommandEvent>>& values) override;
	virtual void Process(const opendnp3::HeaderInfo& info, const opendnp3::ICollection<opendnp3::Indexed<opendnp3::AnalogCommandEvent>>& values) override;
	virtual void Process(const opendnp3::HeaderInfo& info, const opendnp3::ICollection<opendnp3::Indexed<opendnp3::SecurityStat>>& values) override;
	virtual void Process(const opendnp3::HeaderInfo& info, const opendnp3::ICollection<opendnp3::DNPTime>& values) override;

protected:

	void Start() final {}
	void End() final {}
};

#endif
