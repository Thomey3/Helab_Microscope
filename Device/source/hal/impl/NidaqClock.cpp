#include "hal/impl/NidaqClock.h"
#include <sstream>
#include <iomanip>

namespace helab {

NidaqClock::NidaqClock(const std::string& deviceName)
    : deviceName_(deviceName) {
    LOG_DEBUG(std::format("NidaqClock created for device: {}", deviceName));
}

NidaqClock::~NidaqClock() {
    destroyTask();
    LOG_DEBUG("NidaqClock destroyed");
}

NidaqClock::NidaqClock(NidaqClock&& other) noexcept
    : deviceName_(std::move(other.deviceName_))
    , taskHandle_(other.taskHandle_)
    , config_(std::move(other.config_))
    , running_(other.running_.load())
    , configured_(other.configured_.load()) {
    other.taskHandle_ = nullptr;
    other.running_ = false;
    other.configured_ = false;
}

NidaqClock& NidaqClock::operator=(NidaqClock&& other) noexcept {
    if (this != &other) {
        destroyTask();

        deviceName_ = std::move(other.deviceName_);
        taskHandle_ = other.taskHandle_;
        config_ = std::move(other.config_);
        running_ = other.running_.load();
        configured_ = other.configured_.load();

        other.taskHandle_ = nullptr;
        other.running_ = false;
        other.configured_ = false;
    }
    return *this;
}

Result<void> NidaqClock::configure(const ClockConfig& config) {
    if (running_) {
        LOG_ERROR("Cannot configure clock while running");
        return failure(HardwareError::alreadyRunning());
    }

    // Validate configuration
    if (config.pixelCount == 0) {
        return failure(HardwareError::invalidParameter("pixelCount"));
    }
    if (config.dutyCycle <= 0.0 || config.dutyCycle >= 1.0) {
        return failure(HardwareError::invalidParameter("dutyCycle"));
    }

    // Destroy existing task
    destroyTask();

    // Store configuration
    config_ = config;

    // Create and configure task
    auto result = createCounterTask();
    if (!result) {
        return result;
    }

    result = configureCounter();
    if (!result) {
        destroyTask();
        return result;
    }

    configured_ = true;
    LOG_INFO(std::format("Clock configured: {} pixels, {:.1f} Hz line rate",
        config_.pixelCount, config_.lineRate));

    return success();
}

Result<void> NidaqClock::start() {
    if (!configured_) {
        LOG_ERROR("Clock not configured");
        return failure(HardwareError::notInitialized());
    }

    if (running_) {
        LOG_WARN("Clock already running");
        return success();  // Idempotent
    }

    int32 error = DAQmxStartTask(taskHandle_);
    if (error < 0) {
        LOG_ERROR(std::format("Failed to start clock: {}", getDaqErrorString(error)));
        return failure(daqError(error));
    }

    running_ = true;
    LOG_INFO("Clock started");

    return success();
}

Result<void> NidaqClock::stop() {
    if (!running_) {
        return success();  // Idempotent
    }

    int32 error = DAQmxStopTask(taskHandle_);
    if (error < 0) {
        LOG_ERROR(std::format("Failed to stop clock: {}", getDaqErrorString(error)));
        return failure(daqError(error));
    }

    running_ = false;
    LOG_INFO("Clock stopped");

    return success();
}

Result<void> NidaqClock::reset() {
    auto result = stop();
    if (!result) {
        return result;
    }

    destroyTask();
    configured_ = false;

    // Recreate task with existing configuration
    result = createCounterTask();
    if (!result) {
        return result;
    }

    result = configureCounter();
    if (!result) {
        destroyTask();
        return result;
    }

    configured_ = true;
    LOG_INFO("Clock reset");

    return success();
}

bool NidaqClock::isRunning() const {
    return running_;
}

const ClockConfig& NidaqClock::config() const {
    return config_;
}

Result<void> NidaqClock::createCounterTask() {
    std::string taskName = "PixelClock_" + deviceName_;

    int32 error = DAQmxCreateTask(taskName.c_str(), &taskHandle_);
    if (error < 0) {
        LOG_ERROR(std::format("Failed to create task: {}", getDaqErrorString(error)));
        return failure(daqError(error));
    }

    return success();
}

Result<void> NidaqClock::configureCounter() {
    // Build channel name
    std::string counterChannel = "/" + config_.deviceName + "/" + config_.counterChannel;

    // Calculate timing parameters
    double timebaseFreq = config_.timebaseFrequency;
    double linePeriod = 1.0 / config_.lineRate;
    double pixelPeriod = linePeriod / config_.pixelCount;
    double lowTime = pixelPeriod * (1.0 - config_.dutyCycle);
    double highTime = pixelPeriod * config_.dutyCycle;

    // Create pulse channel for pixel clock
    int32 error = DAQmxCreateCOPulseChanTicks(
        taskHandle_,
        counterChannel.c_str(),
        "",
        DAQmx_Val_Hz,
        DAQmx_Val_Low,
        0,                          // Initial delay
        timebaseFreq,               // Timebase frequency
        static_cast<uInt64>(lowTime * timebaseFreq),
        static_cast<uInt64>(highTime * timebaseFreq)
    );

    if (error < 0) {
        LOG_ERROR(std::format("Failed to create pulse channel: {}", getDaqErrorString(error)));
        return failure(daqError(error));
    }

    // Configure implicit timing for continuous generation
    error = DAQmxCfgImplicitTiming(
        taskHandle_,
        DAQmx_Val_ContSamps,
        config_.totalPixels
    );

    if (error < 0) {
        LOG_ERROR(std::format("Failed to configure timing: {}", getDaqErrorString(error)));
        return failure(daqError(error));
    }

    return success();
}

void NidaqClock::destroyTask() {
    if (taskHandle_ != nullptr) {
        DAQmxStopTask(taskHandle_);
        DAQmxClearTask(taskHandle_);
        taskHandle_ = nullptr;
        running_ = false;
    }
}

HardwareError NidaqClock::daqError(int32 errorCode) {
    return HardwareError::driverError(errorCode, getDaqErrorString(errorCode));
}

std::string NidaqClock::getDaqErrorString(int32 errorCode) {
    char buffer[2048];
    DAQmxGetExtendedErrorInfo(buffer, sizeof(buffer));
    return std::string(buffer);
}

} // namespace helab
