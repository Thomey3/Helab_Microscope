#pragma once

#include "../IHardware.h"
#include "../../common/Logger.h"
#include "../../common/ThreadSafeQueue.h"
#include <atomic>
#include <string>
#include <vector>
#include <thread>
#include <mutex>

// Forward declarations for QT SDK
struct CardSettings;
struct TriggerSettings;
struct CollectSetting;

namespace helab {

/// QT12136DC high-speed data acquisition card implementation
class QtDetector : public IDetector {
public:
    /// Card index constant (single card use)
    static constexpr unsigned int CARD_INDEX = 0;

    /// Constructor
    explicit QtDetector();

    /// Destructor - ensures cleanup
    ~QtDetector() override;

    // Rule of Five
    QtDetector(const QtDetector&) = delete;
    QtDetector& operator=(const QtDetector&) = delete;
    QtDetector(QtDetector&&) noexcept;
    QtDetector& operator=(QtDetector&&) noexcept;

    // IDetector interface implementation
    Result<void> configure(const DetectorConfig& config) override;
    Result<void> startAcquisition() override;
    Result<void> stopAcquisition() override;
    Result<void*> getDataBuffer() override;
    Result<size_t> getAvailableDataSize() override;
    void setDataCallback(DetectorCallback callback) override;
    bool isAcquiring() const override;
    const DetectorConfig& config() const override;
    uint32_t activeChannelCount() const override;

private:
    DetectorConfig config_;
    DetectorCallback callback_;
    std::atomic<bool> acquiring_{false};
    std::atomic<bool> configured_{false};
    std::atomic<bool> stopRequested_{false};

    // Buffer management
    std::vector<uint8_t> buffer_;
    size_t bufferSize_ = 0;
    void* rawBufferPtr_ = nullptr;

    // Acquisition thread
    std::thread acquisitionThread_;
    std::mutex mutex_;

    // Channel count
    uint32_t channelCount_ = 0;

    /// Initialize QT card
    Result<void> initializeCard();

    /// Configure acquisition mode
    Result<void> configureAcquisitionMode();

    /// Configure trigger
    Result<void> configureTrigger();

    /// Configure channels
    Result<void> configureChannels();

    /// Close card
    void closeCard();

    /// Acquisition loop (runs in separate thread)
    void acquisitionLoop();

    /// Convert QT SDK error to HardwareError
    static HardwareError qtError(int errorCode);

    /// Get error string
    static std::string getQtErrorString(int errorCode);

    /// Calculate segment size in bytes
    uint32_t calculateSegmentSize() const;
};

} // namespace helab
