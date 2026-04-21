#pragma once

#include "../IHardware.h"
#include "../../common/Logger.h"
#include <NIDAQmx.h>
#include <atomic>
#include <string>

/**
 * @file NidaqClock.h
 * @brief NI-DAQmx based clock generator implementation
 *
 * This file provides the NI-DAQmx implementation of the IClock interface.
 * It uses National Instruments counter/timer hardware to generate precise
 * pixel and line clock signals for synchronized imaging.
 *
 * @section hardware Hardware Requirements
 * - NI-DAQmx driver installed
 * - Compatible NI DAQ device (e.g., PCIe-6363, USB-6343)
 * - Counter/timer channel available
 *
 * @section theory Theory of Operation
 *
 * The clock generator produces pulses at the pixel rate:
 * 1. Configure a counter for pulse train generation
 * 2. Set frequency based on line rate and pixel count
 * 3. Start continuous generation
 * 4. Other hardware (scanner, detector) synchronize to this clock
 *
 * @section example Example Usage
 * @code
 * // Create clock
 * helab::NidaqClock clock("Dev1");
 *
 * // Configure
 * helab::ClockConfig config;
 * config.deviceName = "Dev1";
 * config.counterChannel = "ctr0";
 * config.pixelCount = 512;
 * config.lineRate = 1000.0;  // 1000 lines/sec
 *
 * auto result = clock.configure(config);
 * if (!result) {
 *     std::cerr << "Error: " << result.error().message << std::endl;
 *     return;
 * }
 *
 * // Start clock
 * clock.start();
 *
 * // ... imaging ...
 *
 * clock.stop();
 * @endcode
 *
 * @see IClock
 * @see ClockConfig
 */

namespace helab {

/**
 * @class NidaqClock
 * @brief NI-DAQmx based clock generator implementation
 *
 * Implements the IClock interface using National Instruments DAQ hardware.
 * This class provides precise pulse train generation for pixel and line clocks.
 *
 * @section features Features
 * - High-precision pulse generation using counter/timer
 * - Configurable timebase (20MHz, 80MHz, or external)
 * - Support for external triggering
 * - Thread-safe operation
 *
 * @section limitations Limitations
 * - Requires NI-DAQmx driver
 * - Windows or Linux with NI drivers
 * - Single device per instance
 */
class NidaqClock : public IClock {
public:
    /**
     * @brief Constructor with device name
     * @param deviceName NI-DAQ device name (e.g., "Dev1")
     *
     * Creates a clock instance associated with the specified DAQ device.
     * The device must exist and be accessible through NI-DAQmx.
     */
    explicit NidaqClock(const std::string& deviceName);

    /**
     * @brief Destructor - ensures cleanup
     *
     * Automatically stops the clock and releases DAQ resources.
     */
    ~NidaqClock() override;

    /// @name Rule of Five
    /// @{
    NidaqClock(const NidaqClock&) = delete;
    NidaqClock& operator=(const NidaqClock&) = delete;
    NidaqClock(NidaqClock&&) noexcept;
    NidaqClock& operator=(NidaqClock&&) noexcept;
    /// @}

    /// @name IClock Interface Implementation
    /// @{

    /**
     * @brief Configure the clock generator
     * @param config Configuration parameters
     * @return Result<void> Success or error
     *
     * Configures the counter for pulse train generation with the specified
     * parameters. Must be called before start().
     *
     * @note Reconfiguration requires stopping the clock first
     */
    Result<void> configure(const ClockConfig& config) override;

    /**
     * @brief Start clock generation
     * @return Result<void> Success or error
     *
     * Starts the pulse train output. The clock will generate pulses
     * continuously until stop() is called.
     *
     * @pre Clock must be configured
     */
    Result<void> start() override;

    /**
     * @brief Stop clock generation
     * @return Result<void> Success or error
     *
     * Stops the pulse train output and releases DAQ resources.
     */
    Result<void> stop() override;

    /**
     * @brief Reset to initial state
     * @return Result<void> Success or error
     *
     * Stops the clock and clears the configuration.
     */
    Result<void> reset() override;

    /**
     * @brief Check if clock is running
     * @return true if generating pulses, false otherwise
     */
    bool isRunning() const override;

    /**
     * @brief Get current configuration
     * @return Reference to current ClockConfig
     */
    const ClockConfig& config() const override;

    /// @}

private:
    std::string deviceName_;           ///< DAQ device name
    TaskHandle taskHandle_ = nullptr;  ///< NI-DAQmx task handle
    ClockConfig config_;               ///< Current configuration
    std::atomic<bool> running_{false}; ///< Running state flag
    std::atomic<bool> configured_{false}; ///< Configuration state flag

    /**
     * @brief Create counter task for pixel clock
     * @return Result<void> Success or error
     */
    Result<void> createCounterTask();

    /**
     * @brief Configure counter for pulse generation
     * @return Result<void> Success or error
     */
    Result<void> configureCounter();

    /**
     * @brief Destroy task (RAII helper)
     */
    void destroyTask();

    /**
     * @brief Convert DAQmx error to HardwareError
     * @param errorCode NI-DAQmx error code
     * @return HardwareError with message
     */
    static HardwareError daqError(int32 errorCode);

    /**
     * @brief Get error string from DAQmx
     * @param errorCode NI-DAQmx error code
     * @return Error description string
     */
    static std::string getDaqErrorString(int32 errorCode);
};

} // namespace helab
