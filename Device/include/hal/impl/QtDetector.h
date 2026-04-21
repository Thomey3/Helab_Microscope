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

/**
 * @file QtDetector.h
 * @brief QT12136DC high-speed data acquisition card implementation
 *
 * This file provides the implementation for the QT12136DC high-speed
 * digitizer card, which is used for PMT signal acquisition in two-photon
 * microscopy.
 *
 * @section hardware Hardware Requirements
 * - QT12136DC digitizer card (Qutech)
 * - QT SDK installed
 * - PCIe slot for card
 *
 * @section specs Specifications
 * - Sample rate: Up to 130 MS/s
 * - Channels: 4 simultaneous
 * - Resolution: 12-bit
 * - Input range: ±1V
 *
 * @section theory Theory of Operation
 *
 * The detector acquires continuous data streams:
 * 1. Configure channels and sample rate
 * 2. Set up segmented acquisition (one segment per line)
 * 3. Start acquisition with external trigger
 * 4. Data is transferred to host via DMA
 * 5. Callback notifies when data is available
 *
 * @section example Example Usage
 * @code
 * // Create detector
 * helab::QtDetector detector;
 *
 * // Configure
 * helab::DetectorConfig config;
 * config.channelMask = 0x01;  // Channel 1
 * config.sampleRate = 100000000;  // 100 MS/s
 * config.segmentSize = 512;
 * config.numSegments = 512;
 * config.triggerMode = helab::DetectorConfig::TriggerMode::External;
 *
 * auto result = detector.configure(config);
 *
 * // Set data callback
 * detector.setDataCallback([](const void* data, size_t size, uint64_t ts) {
 *     // Process frame data
 *     const uint16_t* pixels = static_cast<const uint16_t*>(data);
 *     size_t numPixels = size / sizeof(uint16_t);
 *     // ... process pixels ...
 * });
 *
 * // Start acquisition
 * detector.startAcquisition();
 *
 * // ... imaging ...
 *
 * detector.stopAcquisition();
 * @endcode
 *
 * @see IDetector
 * @see DetectorConfig
 */

namespace helab {

/**
 * @class QtDetector
 * @brief QT12136DC high-speed data acquisition card implementation
 *
 * Implements the IDetector interface using the QT12136DC digitizer card.
 * This card provides high-speed, multi-channel data acquisition suitable
 * for two-photon microscopy.
 *
 * @section features Features
 * - Up to 4 simultaneous channels
 * - 130 MS/s maximum sample rate
 * - Segmented acquisition for line-by-line data
 * - External trigger synchronization
 * - DMA data transfer for high throughput
 *
 * @section data Data Format
 *
 * Data is returned as 16-bit unsigned integers (uint16_t).
 * For multi-channel acquisition, channels are interleaved:
 * - Channel 1: samples 0, 4, 8, ...
 * - Channel 2: samples 1, 5, 9, ...
 * - Channel 3: samples 2, 6, 10, ...
 * - Channel 4: samples 3, 7, 11, ...
 */
class QtDetector : public IDetector {
public:
    /// Card index constant (single card use)
    static constexpr unsigned int CARD_INDEX = 0;

    /**
     * @brief Constructor
     *
     * Creates a detector instance for the QT12136DC card.
     */
    explicit QtDetector();

    /**
     * @brief Destructor - ensures cleanup
     *
     * Automatically stops acquisition and releases card resources.
     */
    ~QtDetector() override;

    /// @name Rule of Five
    /// @{
    QtDetector(const QtDetector&) = delete;
    QtDetector& operator=(const QtDetector&) = delete;
    QtDetector(QtDetector&&) noexcept;
    QtDetector& operator=(QtDetector&&) noexcept;
    /// @}

    /// @name IDetector Interface Implementation
    /// @{

    /**
     * @brief Configure the detector
     * @param config Configuration parameters
     * @return Result<void> Success or error
     *
     * Configures the QT card with the specified parameters.
     */
    Result<void> configure(const DetectorConfig& config) override;

    /**
     * @brief Start acquisition
     * @return Result<void> Success or error
     *
     * Starts continuous segmented acquisition.
     * Data is delivered via the registered callback.
     */
    Result<void> startAcquisition() override;

    /**
     * @brief Stop acquisition
     * @return Result<void> Success or error
     *
     * Stops acquisition and waits for acquisition thread to finish.
     */
    Result<void> stopAcquisition() override;

    /**
     * @brief Get raw data buffer pointer
     * @return Result<void*> Pointer to buffer or error
     *
     * Returns the internal buffer containing the most recent data.
     */
    Result<void*> getDataBuffer() override;

    /**
     * @brief Get available data size
     * @return Result<size_t> Size in bytes or error
     */
    Result<size_t> getAvailableDataSize() override;

    /**
     * @brief Set data callback for continuous acquisition
     * @param callback Function to call when data is available
     */
    void setDataCallback(DetectorCallback callback) override;

    /**
     * @brief Check if acquiring
     * @return true if acquiring, false otherwise
     */
    bool isAcquiring() const override;

    /**
     * @brief Get current configuration
     * @return Reference to current DetectorConfig
     */
    const DetectorConfig& config() const override;

    /**
     * @brief Get number of active channels
     * @return Number of channels based on channelMask
     */
    uint32_t activeChannelCount() const override;

    /// @}

private:
    DetectorConfig config_;                ///< Current configuration
    DetectorCallback callback_;            ///< Data callback function
    std::atomic<bool> acquiring_{false};   ///< Acquisition state flag
    std::atomic<bool> configured_{false};  ///< Configuration state flag
    std::atomic<bool> stopRequested_{false}; ///< Stop request flag

    // Buffer management
    std::vector<uint8_t> buffer_;          ///< Data buffer
    size_t bufferSize_ = 0;                ///< Buffer size in bytes
    void* rawBufferPtr_ = nullptr;         ///< Raw buffer pointer from QT SDK

    // Acquisition thread
    std::thread acquisitionThread_;        ///< Acquisition thread
    std::mutex mutex_;                     ///< Mutex for thread safety

    // Channel count
    uint32_t channelCount_ = 0;            ///< Number of active channels

    /**
     * @brief Initialize QT card
     * @return Result<void> Success or error
     */
    Result<void> initializeCard();

    /**
     * @brief Configure acquisition mode
     * @return Result<void> Success or error
     */
    Result<void> configureAcquisitionMode();

    /**
     * @brief Configure trigger
     * @return Result<void> Success or error
     */
    Result<void> configureTrigger();

    /**
     * @brief Configure channels
     * @return Result<void> Success or error
     */
    Result<void> configureChannels();

    /**
     * @brief Close card
     */
    void closeCard();

    /**
     * @brief Acquisition loop (runs in separate thread)
     */
    void acquisitionLoop();

    /**
     * @brief Convert QT SDK error to HardwareError
     * @param errorCode QT SDK error code
     * @return HardwareError with message
     */
    static HardwareError qtError(int errorCode);

    /**
     * @brief Get error string
     * @param errorCode QT SDK error code
     * @return Error description string
     */
    static std::string getQtErrorString(int errorCode);

    /**
     * @brief Calculate segment size in bytes
     * @return Segment size in bytes
     */
    uint32_t calculateSegmentSize() const;
};

} // namespace helab
