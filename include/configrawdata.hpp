#ifndef INCLUDE_CONFIGRAWDATA_HPP_
#define INCLUDE_CONFIGRAWDATA_HPP_

#include <boost/functional/hash.hpp>
#include <functional>


/// Structure describing resource value of particular trigger.
/// Eg. memory over {70%, 200MB}
struct ResourceValue {
	enum class ResourceUnit		{PERCENT, MB, GB};

	/// Resource unit - percent, MB or GB
	ResourceUnit unitType;
	/// Value of resource - may be percent/size/speed
	/// distinction between size/speed is being made by context
	/// Disk has speed, memory has size
	/// Speed is being implicit interpreted as UNIT_TYPE per seconds
	long value;
};


/// Raw structure describing trigger.
/// Used during parsing config file and
/// loading configuration into monitor.
struct TriggerType {
	enum class Resource 		{CPU, MEMORY, DISK};
	enum class FluctuationType 	{UNDER, OVER};

	/// Resource type
	Resource resource;
	/// Fluctuation type (under or over some value)
	FluctuationType fluctuationType;
	/// Fluctuation treshold
	ResourceValue value;
	/// Log duration in seconds
	long duration;
};


/// Raw structure describing logging type.
/// Used during parsing config file and
/// loading configuration into monitor.
struct LogType {
	enum class Resource	{CPU, MEMORY, DISK};

	/// Resource type
	Resource resource;
	/// Log duration in seconds
	long duration;
	/// Log resolution in seconds
	long resolution;

	/// Operator equals used in hash maps
	bool operator==(const LogType &other) const {
		return resource == other.resource && resolution == other.resolution;
	}
};

namespace std {
    template <>
    struct hash<LogType>
    {
        std::size_t operator()(const LogType& logType) const {
            int resourceId = static_cast<typename std::underlying_type<LogType::Resource>::type>(logType.resource);
            long resolutionValue = logType.resolution;
            std::size_t hashValue = 0;
            boost::hash_combine(hashValue, resourceId);
            boost::hash_combine(hashValue, resolutionValue);
            return hashValue;
        }
    };

}

#endif /* INCLUDE_CONFIGRAWDATA_HPP_ */
