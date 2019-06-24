#include <set>

#include "MappingSoeHandler.h"

namespace
{
	template <class T>
	inline std::string ValueToString(const T& meas)
	{
		std::ostringstream oss;
		oss << meas.value;
		return oss.str();
	}

	inline std::string ValueToString(const opendnp3::DoubleBitBinary& meas)
	{
		return opendnp3::DoubleBitToString(meas.value);
	}

	template <class T>
	void Print(const opendnp3::HeaderInfo& info, const T& value, uint16_t index)
	{
		std::cout << "[" << index << "] : " <<
		          ValueToString(value) << " : " <<
		          static_cast<int>(value.flags.value) << " : " <<
		          value.time << std::endl;
	}

	template <class T>
	void PrintAll(const opendnp3::HeaderInfo& info, const opendnp3::ICollection<opendnp3::Indexed<T>>& values)
	{
		auto print = [&](const opendnp3::Indexed<T>& pair)
		{
			Print<T>(info, pair.value, pair.index);
		};
		values.ForeachItem(print);
	}

    template <class T>
    void PrintBinary(const opendnp3::HeaderInfo& info, const opendnp3::ICollection<opendnp3::Indexed<T>>& values);

	template <class T>
    void PrintCounter(const opendnp3::HeaderInfo& info, const opendnp3::ICollection<opendnp3::Indexed<T>>& values);

    template <class T>
	void PrintB(const opendnp3::HeaderInfo& info, const T& value, uint16_t index, asiodnp3::UpdateBuilder *);

	template <class T>
	void PrintC(const opendnp3::HeaderInfo& info, const T& value, uint16_t index, asiodnp3::UpdateBuilder *);

	std::string GetTimeString(opendnp3::TimestampMode tsmode)
	{
		switch (tsmode)
		{
		case(opendnp3::TimestampMode::SYNCHRONIZED) :
			return "synchronized";

		case(opendnp3::TimestampMode::UNSYNCHRONIZED) :
			return "unsynchronized";

		default:
			return "no timestamp";
		}

		return "";
	}

} // end of anonymous namespace

/************************************** Binary callback ********************************************************/
void MappingSoeHandler::Process(const opendnp3::HeaderInfo& info, const opendnp3::ICollection<opendnp3::Indexed<opendnp3::Binary>>& values)
{
	for(std::vector<std::shared_ptr<MappingOutstation>>::iterator oIter = DestinationList.begin(); oIter != DestinationList.end(); ++oIter){
		auto dest = *oIter;
		auto updateBinaryArray = [&](const opendnp3::Indexed<opendnp3::Binary>& pair)
		{
			/*
			std::cout << "Binary Indexes gotten from Outstation are--"<<"[Index:" << pair.index << "], Value: " <<
					  ValueToString(pair.value) << ", Flag value: " <<
					  static_cast<int>(pair.value.flags.value) << ", Timestamp: " <<
					  pair.value.time << std::endl;
			*/
			if(dest->binaryValues.count(SrcName) > 0 && dest->binaryValues[SrcName].size() > pair.index){
				dnp3Point d_pt = dest->binaryValues[SrcName].at(pair.index);
				if (d_pt.value == pair.value.value && pair.value.time == 0){
					return;
				}
				else {
					d_pt.value = pair.value.value;
					d_pt.timestamp = pair.value.time;
					dest->binaryValues[SrcName].at(pair.index) = d_pt;
				}
			}
		};

		std::lock_guard<std::mutex> lock(dest->mutex);
    	values.ForeachItem(updateBinaryArray);
    	dest->binaryFlag[SrcName]=true;
	}
}

/************************************** Analog callback ********************************************************/
void MappingSoeHandler::Process(const opendnp3::HeaderInfo& info, const opendnp3::ICollection<opendnp3::Indexed<opendnp3::Analog>>& values)
{
	for(auto dest : DestinationList) {

		auto updateAnalogArray = [&](const opendnp3::Indexed<opendnp3::Analog>& pair)
	    {
			/*
	        std::cout <<"Analog inputs Callback are--"<< "[" << pair.index << "] : " <<
	                  ValueToString(pair.value) << " : " <<
	                  static_cast<int>(pair.value.flags.value) << " : " <<
	                  pair.value.time << std::endl;
			*/
			if(dest->analogValues.count(SrcName) > 0 && dest->analogValues[SrcName].size() > pair.index){
				dnp3Point d_pt = dest->analogValues[SrcName].at(pair.index);
				if (d_pt.value == pair.value.value && pair.value.time == 0){
					return;
				}
				else {
					d_pt.value = pair.value.value;
					d_pt.timestamp = pair.value.time;
			        dest->analogValues[SrcName].at(pair.index) = d_pt;
				}
			}
	    };

		std::lock_guard<std::mutex> lock(dest->mutex);
		values.ForeachItem(updateAnalogArray);
		dest->analogFlag[SrcName]=true;
	}
}

/************************************** Counter callback ********************************************************/
void MappingSoeHandler::Process(const opendnp3::HeaderInfo& info, const opendnp3::ICollection<opendnp3::Indexed<opendnp3::Counter>>& values)
{
	for(auto dest : DestinationList) {

		auto updateCounterArray = [&](const opendnp3::Indexed<opendnp3::Counter>& pair)
	    {
			/*
			std::cout << "Counter gotten from Outstation are--"<<"[" << index << "] : " <<
		              ValueToString(value) << " : " <<
		              static_cast<int>(value.flags.value) << " : " <<
		              value.time << std::endl;
			*/
			if(dest->counterValues.count(SrcName) > 0 && dest->counterValues[SrcName].size() > pair.index) {
				dnp3Point d_pt = dest->counterValues[SrcName].at(pair.index);
				if (d_pt.value == pair.value.value && pair.value.time == 0) {
					return;
				}
				else {
					d_pt.value = pair.value.value;
					d_pt.timestamp = pair.value.time;
			        dest->counterValues[SrcName].at(pair.index) = d_pt;
				}
			}
	    };

		std::lock_guard<std::mutex> lock(dest->mutex);
		values.ForeachItem(updateCounterArray);
		dest->counterFlag[SrcName]=true;
	}
}

/************************************** Other types callback default copied from PrintingSOEHandler ********************************************************/
void MappingSoeHandler::Process(const opendnp3::HeaderInfo& info, const opendnp3::ICollection<opendnp3::Indexed<opendnp3::DoubleBitBinary>>& values)
{
	return PrintAll(info, values);
}

void MappingSoeHandler::Process(const opendnp3::HeaderInfo& info, const opendnp3::ICollection<opendnp3::Indexed<opendnp3::FrozenCounter>>& values)
{
	return PrintAll(info, values);
}

void MappingSoeHandler::Process(const opendnp3::HeaderInfo& info, const opendnp3::ICollection<opendnp3::Indexed<opendnp3::BinaryOutputStatus>>& values)
{
	return PrintAll(info, values);
}

void MappingSoeHandler::Process(const opendnp3::HeaderInfo& info, const opendnp3::ICollection<opendnp3::Indexed<opendnp3::AnalogOutputStatus>>& values)
{
	return PrintAll(info, values);
}

void MappingSoeHandler::Process(const opendnp3::HeaderInfo& info, const opendnp3::ICollection<opendnp3::Indexed<opendnp3::OctetString>>& values)
{
	auto print = [](const opendnp3::Indexed<opendnp3::OctetString>& pair)
	{
		std::cout << "OctetString " << " [" << pair.index << "] : Size : " << pair.value.ToRSlice().Size() << std::endl;
	};

	values.ForeachItem(print);
}

void MappingSoeHandler::Process(const opendnp3::HeaderInfo& info, const opendnp3::ICollection<opendnp3::Indexed<opendnp3::TimeAndInterval>>& values)
{
	auto print = [](const opendnp3::Indexed<opendnp3::TimeAndInterval>& pair)
	{
		std::cout << "TimeAndInterval: " <<
		          "[" << pair.index << "] : " <<
		          pair.value.time << " : " <<
		          pair.value.interval << " : " <<
		          IntervalUnitsToString(pair.value.GetUnitsEnum()) << std::endl;
	};

	values.ForeachItem(print);
}

void MappingSoeHandler::Process(const opendnp3::HeaderInfo& info, const opendnp3::ICollection<opendnp3::Indexed<opendnp3::BinaryCommandEvent>>& values)
{
	auto print = [](const opendnp3::Indexed<opendnp3::BinaryCommandEvent>& pair)
	{
		std::cout << "BinaryCommandEvent: " <<
		          "[" << pair.index << "] : " <<
		          pair.value.time << " : " <<
		          pair.value.value << " : " <<
		          CommandStatusToString(pair.value.status) << std::endl;
	};

	values.ForeachItem(print);
}

void MappingSoeHandler::Process(const opendnp3::HeaderInfo& info, const opendnp3::ICollection<opendnp3::Indexed<opendnp3::AnalogCommandEvent>>& values)
{
	auto print = [](const opendnp3::Indexed<opendnp3::AnalogCommandEvent>& pair)
	{
		std::cout << "AnalogCommandEvent: " <<
		          "[" << pair.index << "] : " <<
		          pair.value.time << " : " <<
		          pair.value.value << " : " <<
		          CommandStatusToString(pair.value.status) << std::endl;
	};

	values.ForeachItem(print);
}

void MappingSoeHandler::Process(const opendnp3::HeaderInfo& info, const opendnp3::ICollection<opendnp3::Indexed<opendnp3::SecurityStat>>& values)
{
	auto print = [](const opendnp3::Indexed<opendnp3::SecurityStat>& pair)
	{
		std::cout << "SecurityStat: " <<
		          "[" << pair.index << "] : " <<
		          pair.value.time << " : " <<
		          pair.value.value.count << " : " <<
		          static_cast<int>(pair.value.quality) << " : " <<
		          pair.value.value.assocId << std::endl;
	};

	values.ForeachItem(print);
}

void MappingSoeHandler::Process(const opendnp3::HeaderInfo& info, const opendnp3::ICollection<opendnp3::DNPTime>& values)
{
	auto print = [](const opendnp3::DNPTime & value)
	{
		std::cout << "DNPTime: " << value.value << std::endl;
	};

	values.ForeachItem(print);
}
