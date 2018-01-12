#include "SamplingManager.hpp"



void SamplingManager::initializeLoggingBuffers(const std::vector<LogType> &logTypes) {
    for (auto &&logType : logTypes) {
        long size = 1;
        if (logType.resolution > 0) {
            size = logType.duration / logType.resolution;
        }

        if (logType.resource == LogType::Resource::CPU) {
            cpuLog[logType] = std::make_pair<CpuLog, long>(
                    std::make_shared<boost::circular_buffer<CpuState>>(size),
                    0);
        }

        if (logType.resource == LogType::Resource::MEMORY) {
            ramLog[logType] = std::make_pair<RamLog, long>(
                    std::make_shared<boost::circular_buffer<RamState>>(size),
                    0);
        }

        if (logType.resource == LogType::Resource::DISK) {
            hddLog[logType] = std::make_pair<HddLog, long>(
                    std::make_shared<boost::circular_buffer<HddState>>(size),
                    0);
        }
    }
}

void SamplingManager::pollingFunction() {
    // function working in the separate thread
    while (isSampling) {
        auto start = std::chrono::steady_clock::now();
        cpuSamples.push_back(std::make_pair<ChronoTime, CpuState>(
                std::chrono::system_clock::now(),
                resUsageProvider.getCpuState()
        ));

        ramSamples.push_back(std::make_pair<ChronoTime, RamState>(
                std::chrono::system_clock::now(),
                resUsageProvider.getRamState()
        ));

        hddSamples.push_back(std::make_pair<ChronoTime, HddState>(
                std::chrono::system_clock::now(),
                resUsageProvider.getHddState()
        ));

        processTriggers();
        processLogs();

        if (!isSampling)
            break;
        auto end = std::chrono::steady_clock::now();
        std::chrono::duration<double, std::milli> duration = end - start;
        BOOST_LOG_TRIVIAL(debug) << "Poll time: " << duration.count() << " ms";
        auto sleepTime = static_cast<size_t>(SAMPLING_TIME_MS - duration.count());
        std::this_thread::sleep_for(std::chrono::milliseconds(sleepTime));
    }
}


void SamplingManager::processTriggers() {
    // get last read values
    CpuState lastCpuState = cpuSamples.back().second;
    RamState lastRamState = ramSamples.back().second;
    HddState lastHddState = hddSamples.back().second;

    for (auto &&triggerState : triggerStates) {
        auto &&trigger = triggerState.first;
        // process different types of resources
        if (trigger.resource == TriggerType::Resource::CPU) {
            if (lastCpuState > trigger)
                triggerState.second++;
            else
                triggerState.second = 0;
        }
        else if (trigger.resource == TriggerType::Resource::MEMORY) {
            if (lastRamState > trigger)
                triggerState.second++;
            else
                triggerState.second = 0;
        }
        else if (trigger.resource == TriggerType::Resource::DISK) {
            if (lastHddState > trigger)
                triggerState.second++;
            else
                triggerState.second = 0;
        }

        // if trigger duration exceedes, fire it
        if (triggerState.second > triggerState.first.duration) {
            fireTrigger(trigger);
            triggerState.second = 0;
        }
    }
}

void SamplingManager::fireTrigger(const TriggerType &trigger) {

}

void SamplingManager::processLogs() {
    for (auto &&cpu : cpuLog) {
        auto &&logType = cpu.first;
        // get how many seconds passed since last log store
        auto &&secondsSinceLastLog = cpu.second.second;
        if (secondsSinceLastLog >= logType.duration) {
            // time to compute mean of (logType.duration) samples
            auto &&cpuLogBuffer = cpu.second.first;
            auto &&cpuStateMean = getCpuSamplesMean(logType.duration);
            cpuLogBuffer->push_back(std::move(cpuStateMean));
            secondsSinceLastLog = 0;
        }
    }

    for (auto &&ram : ramLog) {
        auto &&logType = ram.first;
        // get how many seconds passed since last log store
        auto &&secondsSinceLastLog = ram.second.second;
        if (secondsSinceLastLog >= logType.duration) {
            // time to compute mean of (logType.duration) samples
            auto &&ramLogBuffer = ram.second.first;
            auto &&ramStateMean = getRamSamplesMean(logType.duration);
            ramLogBuffer->push_back(std::move(ramStateMean));
            secondsSinceLastLog = 0;
        }
    }

    for (auto &&hdd : hddLog) {
        auto &&logType = hdd.first;
        // get how many seconds passed since last log store
        auto &&secondsSinceLastLog = hdd.second.second;
        if (secondsSinceLastLog >= logType.duration) {
            // time to compute mean of (logType.duration) samples
            auto &&hddLogBuffer = hdd.second.first;
            auto &&hddStateMean = getHddSamplesMean(logType.duration);
            hddLogBuffer->push_back(std::move(hddStateMean));
            secondsSinceLastLog = 0;
        }
    }
}

CpuState SamplingManager::getCpuSamplesMean(long samplesNumber) {
    return CpuState();
}

RamState SamplingManager::getRamSamplesMean(long samplesNumber) {
    return RamState();
}

HddState SamplingManager::getHddSamplesMean(long samplesNumber) {
    return HddState();
}

SamplingManager::~SamplingManager() {
    isSampling = false;
    pollerThread.join();
    BOOST_LOG_TRIVIAL(debug) << "joined";
}

SamplingManager::SamplingManager(ResUsageProvider &&resUsageProvider,
                                 const std::vector<LogType> &logTypes,
                                 const std::vector<TriggerType> &triggerType)
        : cpuSamples(SAMPLES_BUFFER_SIZE),
          ramSamples(SAMPLES_BUFFER_SIZE),
          hddSamples(SAMPLES_BUFFER_SIZE),
          isSampling(true),
          resUsageProvider(resUsageProvider)
{
    initializeLoggingBuffers(logTypes);
    pollerThread = std::thread(&SamplingManager::pollingFunction, this);
}

const std::shared_ptr<const boost::circular_buffer<CpuState>> &
SamplingManager::getCpuLog(const LogType &logType) const {
    return cpuLog.at(logType).first;
}

const std::shared_ptr<const boost::circular_buffer<RamState>> &
SamplingManager::getRamLog(const LogType &logType) const {
    return ramLog.at(logType).first;
}

const std::shared_ptr<const boost::circular_buffer<HddState>> &
SamplingManager::getHddLog(const LogType &logType) const {
    return hddLog.at(logType).first;
}



