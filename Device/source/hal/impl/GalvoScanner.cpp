#include "hal/impl/GalvoScanner.h"
#include <cmath>
#include <numbers>

namespace helab {

GalvoScanner::GalvoScanner(const std::string& deviceName)
    : deviceName_(deviceName) {
    LOG_DEBUG(std::format("GalvoScanner created for device: {}", deviceName));
}

GalvoScanner::~GalvoScanner() {
    destroyTask();
    LOG_DEBUG("GalvoScanner destroyed");
}

GalvoScanner::GalvoScanner(GalvoScanner&& other) noexcept
    : deviceName_(std::move(other.deviceName_))
    , taskHandle_(other.taskHandle_)
    , config_(std::move(other.config_))
    , pattern_(std::move(other.pattern_))
    , waveformX_(std::move(other.waveformX_))
    , waveformY_(std::move(other.waveformY_))
    , running_(other.running_.load())
    , configured_(other.configured_.load()) {
    other.taskHandle_ = nullptr;
    other.running_ = false;
    other.configured_ = false;
}

GalvoScanner& GalvoScanner::operator=(GalvoScanner&& other) noexcept {
    if (this != &other) {
        destroyTask();

        deviceName_ = std::move(other.deviceName_);
        taskHandle_ = other.taskHandle_;
        config_ = std::move(other.config_);
        pattern_ = std::move(other.pattern_);
        waveformX_ = std::move(other.waveformX_);
        waveformY_ = std::move(other.waveformY_);
        running_ = other.running_.load();
        configured_ = other.configured_.load();

        other.taskHandle_ = nullptr;
        other.running_ = false;
        other.configured_ = false;
    }
    return *this;
}

Result<void> GalvoScanner::configure(const ScannerConfig& config) {
    if (running_) {
        LOG_ERROR("Cannot configure scanner while running");
        return failure(HardwareError::alreadyRunning());
    }

    // Validate configuration
    if (config.scanRange <= 0.0) {
        return failure(HardwareError::invalidParameter("scanRange"));
    }
    if (config.frequency <= 0.0) {
        return failure(HardwareError::invalidParameter("frequency"));
    }
    if (config.samplesPerLine == 0) {
        return failure(HardwareError::invalidParameter("samplesPerLine"));
    }

    // Destroy existing task
    destroyTask();

    // Store configuration
    config_ = config;

    // Create and configure task
    auto result = createAOTask();
    if (!result) {
        return result;
    }

    configured_ = true;
    LOG_INFO(std::format("Scanner configured: {:.1f} V range, {:.1f} Hz, {} samples/line",
        config_.scanRange, config_.frequency, config_.samplesPerLine));

    return success();
}

Result<void> GalvoScanner::setScanPattern(const ScanPattern& pattern) {
    if (pattern.type != ScanPattern::Type::Galvo) {
        return failure(HardwareError::invalidParameter("scanType - only Galvo supported"));
    }

    pattern_ = pattern;

    // Regenerate waveforms
    generateGalvoWaveform();

    // If already configured, update the waveforms
    if (configured_ && taskHandle_ != nullptr) {
        return writeWaveforms();
    }

    return success();
}

Result<void> GalvoScanner::start() {
    if (!configured_) {
        LOG_ERROR("Scanner not configured");
        return failure(HardwareError::notInitialized());
    }

    if (running_) {
        LOG_WARN("Scanner already running");
        return success();
    }

    // Generate waveforms if not done
    if (waveformX_.empty() || waveformY_.empty()) {
        generateGalvoWaveform();
    }

    // Write waveforms
    auto result = writeWaveforms();
    if (!result) {
        return result;
    }

    // Start task
    int32 error = DAQmxStartTask(taskHandle_);
    if (error < 0) {
        LOG_ERROR(std::format("Failed to start scanner: {}", getDaqErrorString(error)));
        return failure(daqError(error));
    }

    running_ = true;
    LOG_INFO("Scanner started");

    return success();
}

Result<void> GalvoScanner::stop() {
    if (!running_) {
        return success();
    }

    int32 error = DAQmxStopTask(taskHandle_);
    if (error < 0) {
        LOG_ERROR(std::format("Failed to stop scanner: {}", getDaqErrorString(error)));
        return failure(daqError(error));
    }

    running_ = false;
    LOG_INFO("Scanner stopped");

    return success();
}

Result<void> GalvoScanner::setPosition(double x, double y) {
    // For point scanning - write single values
    if (!configured_) {
        return failure(HardwareError::notInitialized());
    }

    // Clamp to scan range
    x = std::clamp(x, -config_.scanRange, config_.scanRange);
    y = std::clamp(y, -config_.scanRange, config_.scanRange);

    double values[2] = {x, y};
    int32 error = DAQmxWriteAnalogF64(
        taskHandle_,
        1,                      // 1 sample
        true,                   // Auto start
        10.0,                   // Timeout
        DAQmx_Val_GroupByChannel,
        values,
        nullptr,
        nullptr
    );

    if (error < 0) {
        LOG_ERROR(std::format("Failed to set position: {}", getDaqErrorString(error)));
        return failure(daqError(error));
    }

    return success();
}

bool GalvoScanner::isRunning() const {
    return running_;
}

const ScannerConfig& GalvoScanner::config() const {
    return config_;
}

const ScanPattern& GalvoScanner::pattern() const {
    return pattern_;
}

const std::vector<double>& GalvoScanner::waveformX() const {
    return waveformX_;
}

const std::vector<double>& GalvoScanner::waveformY() const {
    return waveformY_;
}

Result<void> GalvoScanner::createAOTask() {
    std::string taskName = "GalvoScanner_" + deviceName_;

    int32 error = DAQmxCreateTask(taskName.c_str(), &taskHandle_);
    if (error < 0) {
        LOG_ERROR(std::format("Failed to create task: {}", getDaqErrorString(error)));
        return failure(daqError(error));
    }

    // Create AO channels for X and Y
    std::string xChannel = "/" + config_.deviceName + "/" + config_.xChannel;
    std::string yChannel = "/" + config_.deviceName + "/" + config_.yChannel;

    error = DAQmxCreateAOVoltageChan(
        taskHandle_,
        (xChannel + "," + yChannel).c_str(),
        "",
        -config_.scanRange,
        config_.scanRange,
        DAQmx_Val_Volts,
        nullptr
    );

    if (error < 0) {
        LOG_ERROR(std::format("Failed to create AO channels: {}", getDaqErrorString(error)));
        destroyTask();
        return failure(daqError(error));
    }

    // Configure timing for continuous generation
    error = DAQmxCfgSampClkTiming(
        taskHandle_,
        "",
        config_.frequency * config_.samplesPerLine,  // Sample rate
        DAQmx_Val_Rising,
        DAQmx_Val_ContSamps,
        config_.samplesPerLine
    );

    if (error < 0) {
        LOG_ERROR(std::format("Failed to configure timing: {}", getDaqErrorString(error)));
        destroyTask();
        return failure(daqError(error));
    }

    return success();
}

void GalvoScanner::generateGalvoWaveform() {
    size_t samplesPerLine = config_.samplesPerLine;
    size_t activeSamples = static_cast<size_t>(samplesPerLine * pattern_.fillFraction);

    waveformX_.resize(samplesPerLine);
    waveformY_.resize(samplesPerLine);

    double amplitude = pattern_.galvoAmplitude;

    // Generate X waveform (sawtooth for bidirectional, triangle for unidirectional)
    for (size_t i = 0; i < samplesPerLine; ++i) {
        if (i < activeSamples) {
            // Active scan region
            double t = static_cast<double>(i) / activeSamples;
            if (pattern_.bidirectional) {
                // Sawtooth: linear ramp
                waveformX_[i] = amplitude * (2.0 * t - 1.0);
            } else {
                // Triangle: forward scan only
                waveformX_[i] = amplitude * (2.0 * t - 1.0);
            }
        } else {
            // Flyback region
            double t = static_cast<double>(i - activeSamples) / (samplesPerLine - activeSamples);
            waveformX_[i] = amplitude * (1.0 - 2.0 * t);  // Return ramp
        }
    }

    // Generate Y waveform (slow ramp for frame scanning)
    // For now, just a constant (Y is controlled separately for frame scanning)
    std::fill(waveformY_.begin(), waveformY_.end(), 0.0);

    LOG_DEBUG(std::format("Generated galvo waveforms: {} samples", samplesPerLine));
}

Result<void> GalvoScanner::writeWaveforms() {
    if (waveformX_.empty() || waveformY_.empty()) {
        return failure(HardwareError::configuration("Waveforms not generated"));
    }

    // Interleave X and Y samples
    std::vector<double> interleaved(waveformX_.size() * 2);
    for (size_t i = 0; i < waveformX_.size(); ++i) {
        interleaved[i * 2] = waveformX_[i];
        interleaved[i * 2 + 1] = waveformY_[i];
    }

    int32 error = DAQmxWriteAnalogF64(
        taskHandle_,
        static_cast<int32>(waveformX_.size()),
        false,                  // Not auto start
        10.0,                   // Timeout
        DAQmx_Val_GroupByScanNumber,
        interleaved.data(),
        nullptr,
        nullptr
    );

    if (error < 0) {
        LOG_ERROR(std::format("Failed to write waveforms: {}", getDaqErrorString(error)));
        return failure(daqError(error));
    }

    return success();
}

void GalvoScanner::destroyTask() {
    if (taskHandle_ != nullptr) {
        DAQmxStopTask(taskHandle_);
        DAQmxClearTask(taskHandle_);
        taskHandle_ = nullptr;
        running_ = false;
    }
}

HardwareError GalvoScanner::daqError(int32 errorCode) {
    return HardwareError::driverError(errorCode, getDaqErrorString(errorCode));
}

std::string GalvoScanner::getDaqErrorString(int32 errorCode) {
    char buffer[2048];
    DAQmxGetExtendedErrorInfo(buffer, sizeof(buffer));
    return std::string(buffer);
}

} // namespace helab
