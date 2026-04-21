#include "hal/impl/QtDetector.h"
#include <QT_Board.h>
#include <QT_Board_Project.h>
#include <chrono>

namespace helab {

QtDetector::QtDetector() {
    LOG_DEBUG("QtDetector created");
}

QtDetector::~QtDetector() {
    stopAcquisition();
    closeCard();
    LOG_DEBUG("QtDetector destroyed");
}

QtDetector::QtDetector(QtDetector&& other) noexcept
    : config_(std::move(other.config_))
    , callback_(std::move(other.callback_))
    , acquiring_(other.acquiring_.load())
    , configured_(other.configured_.load())
    , stopRequested_(other.stopRequested_.load())
    , buffer_(std::move(other.buffer_))
    , bufferSize_(other.bufferSize_)
    , rawBufferPtr_(other.rawBufferPtr_)
    , channelCount_(other.channelCount_) {
    other.rawBufferPtr_ = nullptr;
    other.bufferSize_ = 0;
    other.acquiring_ = false;
    other.configured_ = false;
}

QtDetector& QtDetector::operator=(QtDetector&& other) noexcept {
    if (this != &other) {
        stopAcquisition();
        closeCard();

        config_ = std::move(other.config_);
        callback_ = std::move(other.callback_);
        acquiring_ = other.acquiring_.load();
        configured_ = other.configured_.load();
        stopRequested_ = other.stopRequested_.load();
        buffer_ = std::move(other.buffer_);
        bufferSize_ = other.bufferSize_;
        rawBufferPtr_ = other.rawBufferPtr_;
        channelCount_ = other.channelCount_;

        other.rawBufferPtr_ = nullptr;
        other.bufferSize_ = 0;
        other.acquiring_ = false;
        other.configured_ = false;
    }
    return *this;
}

Result<void> QtDetector::configure(const DetectorConfig& config) {
    if (acquiring_) {
        LOG_ERROR("Cannot configure detector while acquiring");
        return failure(HardwareError::alreadyRunning());
    }

    // Validate configuration
    if (config.channelMask == 0) {
        return failure(HardwareError::invalidParameter("channelMask"));
    }
    if (config.sampleRate == 0) {
        return failure(HardwareError::invalidParameter("sampleRate"));
    }
    if (config.segmentSize == 0) {
        return failure(HardwareError::invalidParameter("segmentSize"));
    }

    // Store configuration
    config_ = config;

    // Count active channels
    channelCount_ = 0;
    for (uint32_t mask = config_.channelMask; mask != 0; mask >>= 1) {
        if (mask & 1) channelCount_++;
    }

    // Initialize card
    auto result = initializeCard();
    if (!result) {
        return result;
    }

    // Configure acquisition
    result = configureAcquisitionMode();
    if (!result) {
        closeCard();
        return result;
    }

    result = configureTrigger();
    if (!result) {
        closeCard();
        return result;
    }

    result = configureChannels();
    if (!result) {
        closeCard();
        return result;
    }

    // Allocate buffer
    bufferSize_ = calculateSegmentSize() * config_.numSegments;
    buffer_.resize(bufferSize_);

    configured_ = true;
    LOG_INFO(std::format("QtDetector configured: {} channels, {} MS/s, {} bytes buffer",
        channelCount_, config_.sampleRate / 1'000'000, bufferSize_));

    return success();
}

Result<void> QtDetector::startAcquisition() {
    if (!configured_) {
        LOG_ERROR("Detector not configured");
        return failure(HardwareError::notInitialized());
    }

    if (acquiring_) {
        LOG_WARN("Detector already acquiring");
        return success();
    }

    stopRequested_ = false;

    // Start QT card acquisition
    int result = QT_BoardSetupRecCollectData(CARD_INDEX, 1, 0);
    if (result < 0) {
        LOG_ERROR(std::format("Failed to start acquisition: {}", getQtErrorString(result)));
        return failure(qtError(result));
    }

    // Start acquisition thread
    acquiring_ = true;
    acquisitionThread_ = std::thread(&QtDetector::acquisitionLoop, this);

    LOG_INFO("QtDetector acquisition started");
    return success();
}

Result<void> QtDetector::stopAcquisition() {
    if (!acquiring_) {
        return success();
    }

    stopRequested_ = true;
    acquiring_ = false;

    if (acquisitionThread_.joinable()) {
        acquisitionThread_.join();
    }

    // Stop QT card acquisition
    int result = QT_BoardSetupRecCollectData(CARD_INDEX, 0, 0);
    if (result < 0) {
        LOG_ERROR(std::format("Failed to stop acquisition: {}", getQtErrorString(result)));
        return failure(qtError(result));
    }

    LOG_INFO("QtDetector acquisition stopped");
    return success();
}

Result<void*> QtDetector::getDataBuffer() {
    if (!configured_) {
        return failure(HardwareError::notInitialized());
    }
    return buffer_.data();
}

Result<size_t> QtDetector::getAvailableDataSize() {
    if (!configured_) {
        return failure(HardwareError::notInitialized());
    }

    int available = QT_BoardGetFIFODataAvail(CARD_INDEX);
    if (available < 0) {
        return failure(qtError(available));
    }

    return static_cast<size_t>(available);
}

void QtDetector::setDataCallback(DetectorCallback callback) {
    std::lock_guard<std::mutex> lock(mutex_);
    callback_ = std::move(callback);
}

bool QtDetector::isAcquiring() const {
    return acquiring_;
}

const DetectorConfig& QtDetector::config() const {
    return config_;
}

uint32_t QtDetector::activeChannelCount() const {
    return channelCount_;
}

Result<void> QtDetector::initializeCard() {
    // Open the board
    int result = QT_BoardOpenBoard(CARD_INDEX);
    if (result < 0) {
        LOG_ERROR(std::format("Failed to open QT board: {}", getQtErrorString(result)));
        return failure(qtError(result));
    }

    // Enable ADC
    result = QT_BoardSetADCEnable(CARD_INDEX);
    if (result < 0) {
        LOG_ERROR(std::format("Failed to enable ADC: {}", getQtErrorString(result)));
        closeCard();
        return failure(qtError(result));
    }

    LOG_DEBUG("QT board initialized");
    return success();
}

Result<void> QtDetector::configureAcquisitionMode() {
    // Set sample rate
    int result = QT_BoardSetupRecSamplerate(CARD_INDEX, config_.sampleRate);
    if (result < 0) {
        LOG_ERROR(std::format("Failed to set sample rate: {}", getQtErrorString(result)));
        return failure(qtError(result));
    }

    // Set clock mode (internal)
    result = QT_BoardSetupRecClockMode(CARD_INDEX, 0);  // 0 = internal clock
    if (result < 0) {
        LOG_ERROR(std::format("Failed to set clock mode: {}", getQtErrorString(result)));
        return failure(qtError(result));
    }

    // Configure FIFO multi-segment mode for continuous acquisition
    uint32_t segmentSize = calculateSegmentSize();
    result = QT_BoardSetupModeRecFIFOMulti(
        CARD_INDEX,
        0,              // DMA block number
        segmentSize,    // Segment size in bytes
        0,              // Frame switch
        0,              // Pre-trigger dots
        0.0             // Repetition frequency (0 = continuous)
    );

    if (result < 0) {
        LOG_ERROR(std::format("Failed to set acquisition mode: {}", getQtErrorString(result)));
        return failure(qtError(result));
    }

    LOG_DEBUG(std::format("Acquisition mode configured: {} bytes/segment", segmentSize));
    return success();
}

Result<void> QtDetector::configureTrigger() {
    int result = 0;

    switch (config_.triggerMode) {
        case DetectorConfig::TriggerMode::Internal:
            result = QT_BoardSetupTrigRecSoftware(CARD_INDEX);
            break;

        case DetectorConfig::TriggerMode::External:
            result = QT_BoardSetupTrigRecExternal(
                CARD_INDEX,
                0,  // External mode
                0,  // Trigger delay
                config_.triggerLevel
            );
            break;

        case DetectorConfig::TriggerMode::Software:
            result = QT_BoardSetupTrigRecSoftware(CARD_INDEX);
            break;
    }

    if (result < 0) {
        LOG_ERROR(std::format("Failed to configure trigger: {}", getQtErrorString(result)));
        return failure(qtError(result));
    }

    LOG_DEBUG("Trigger configured");
    return success();
}

Result<void> QtDetector::configureChannels() {
    // Set channel mask
    int result = QT_BoardSetupRecChannelMode(CARD_INDEX, config_.channelMask);
    if (result < 0) {
        LOG_ERROR(std::format("Failed to set channel mode: {}", getQtErrorString(result)));
        return failure(qtError(result));
    }

    LOG_DEBUG(std::format("Channels configured: mask=0x{:X}", config_.channelMask));
    return success();
}

void QtDetector::closeCard() {
    QT_BoardCloseBoard(CARD_INDEX);
    configured_ = false;
}

void QtDetector::acquisitionLoop() {
    constexpr int TIMEOUT_MS = 100;
    uint64_t frameCounter = 0;

    while (!stopRequested_ && acquiring_) {
        // Get available data
        int available = QT_BoardGetFIFODataAvail(CARD_INDEX);

        if (available > 0 && callback_) {
            // Get data pointer
            unsigned char* dataPtr = nullptr;
            int result = QT_BoardGetFIFODataBufferPointer(
                CARD_INDEX,
                &dataPtr,
                available,
                TIMEOUT_MS
            );

            if (result >= 0 && dataPtr != nullptr) {
                // Get timestamp
                auto timestamp = std::chrono::steady_clock::now().time_since_epoch().count();

                // Call callback
                {
                    std::lock_guard<std::mutex> lock(mutex_);
                    if (callback_) {
                        callback_(dataPtr, available, timestamp);
                    }
                }

                // Return buffer
                QT_BoardReturnFIFODataBufferPointer(CARD_INDEX, &dataPtr, available);
                frameCounter++;
            }
        } else if (available < 0) {
            LOG_ERROR(std::format("Error getting data availability: {}", getQtErrorString(available)));
            break;
        }

        // Small sleep to prevent busy-waiting
        std::this_thread::sleep_for(std::chrono::microseconds(100));
    }

    LOG_DEBUG(std::format("Acquisition thread stopped after {} frames", frameCounter));
}

HardwareError QtDetector::qtError(int errorCode) {
    return HardwareError::driverError(errorCode, getQtErrorString(errorCode));
}

std::string QtDetector::getQtErrorString(int errorCode) {
    return std::format("QT SDK error code: {}", errorCode);
}

uint32_t QtDetector::calculateSegmentSize() const {
    // Segment size = samples_per_segment * channels * bytes_per_sample
    // For 14-bit ADC, we use 2 bytes per sample
    constexpr uint32_t BYTES_PER_SAMPLE = 2;
    return config_.segmentSize * channelCount_ * BYTES_PER_SAMPLE;
}

} // namespace helab
