#pragma once

#include "../IHardware.h"
#include "../../common/Logger.h"
#include <NIDAQmx.h>
#include <atomic>
#include <string>
#include <vector>

/**
 * @file GalvoScanner.h
 * @brief Galvanometer scanner implementation using NI-DAQmx
 *
 * This file provides the galvanometer scanner implementation for two-photon
 * microscopy. Galvo scanners use mirror galvanometers to steer the laser beam
 * in a raster pattern across the sample.
 *
 * @section hardware Hardware Requirements
 * - NI-DAQmx driver installed
 * - Compatible NI DAQ device with analog outputs (e.g., PCIe-6363)
 * - Galvanometer mirrors with voltage control
 * - Typically ±10V control range
 *
 * @section theory Theory of Operation
 *
 * Galvo scanning generates synchronized X-Y waveforms:
 * 1. X-axis: Sawtooth or triangle wave (fast axis)
 * 2. Y-axis: Stepped sawtooth (slow axis)
 *
 * For bidirectional scanning:
 * - Both forward and backward sweeps are used
 * - Data from both directions must be properly ordered
 *
 * @section performance Performance Characteristics
 * - Frame rate: ~1-2 Hz for 512x512 pixels
 * - Line rate: 500-2000 Hz typical
 * - Advantages: High resolution, flexible scan patterns
 * - Limitations: Relatively slow compared to resonant scanners
 *
 * @section example Example Usage
 * @code
 * // Create scanner
 * helab::GalvoScanner scanner("Dev1");
 *
 * // Configure hardware channels
 * helab::ScannerConfig config;
 * config.deviceName = "Dev1";
 * config.xChannel = "ao0";
 * config.yChannel = "ao1";
 * config.scanRange = 5.0;  // ±5V
 * config.frequency = 1000.0;  // 1 kHz line rate
 * config.samplesPerLine = 512;
 *
 * auto result = scanner.configure(config);
 *
 * // Set scan pattern
 * helab::ScanPattern pattern;
 * pattern.type = helab::ScanPattern::Type::Galvo;
 * pattern.bidirectional = true;
 * pattern.fillFraction = 0.8;
 * scanner.setScanPattern(pattern);
 *
 * // Start scanning
 * scanner.start();
 *
 * // Access waveforms for synchronization
 * const auto& xWave = scanner.waveformX();
 * const auto& yWave = scanner.waveformY();
 *
 * // ... imaging ...
 *
 * scanner.stop();
 * @endcode
 *
 * @see IScanner
 * @see ScannerConfig
 * @see ScanPattern
 */

namespace helab {

/**
 * @class GalvoScanner
 * @brief Galvanometer scanner implementation using NI-DAQmx
 *
 * Implements the IScanner interface using NI-DAQ analog outputs to drive
 * galvanometer mirrors for beam scanning.
 *
 * @section features Features
 * - Configurable scan range and frequency
 * - Bidirectional and unidirectional scanning
 * - Synchronized X-Y waveform generation
 * - Support for arbitrary scan patterns
 *
 * @section waveform Waveform Generation
 *
 * The scanner generates voltage waveforms for X and Y mirrors:
 * - X waveform: Fast sawtooth/triangle for line scanning
 * - Y waveform: Slow ramp for frame advancement
 *
 * For a 512x512 image at 1 kHz line rate:
 * - X frequency: 1000 Hz (line rate)
 * - Y frequency: ~1.95 Hz (frame rate)
 */
class GalvoScanner : public IScanner {
public:
    /**
     * @brief Constructor with device name
     * @param deviceName NI-DAQ device name (e.g., "Dev1")
     *
     * Creates a scanner instance associated with the specified DAQ device.
     */
    explicit GalvoScanner(const std::string& deviceName);

    /**
     * @brief Destructor - ensures cleanup
     *
     * Automatically stops scanning and releases DAQ resources.
     */
    ~GalvoScanner() override;

    /// @name Rule of Five
    /// @{
    GalvoScanner(const GalvoScanner&) = delete;
    GalvoScanner& operator=(const GalvoScanner&) = delete;
    GalvoScanner(GalvoScanner&&) noexcept;
    GalvoScanner& operator=(GalvoScanner&&) noexcept;
    /// @}

    /// @name IScanner Interface Implementation
    /// @{

    /**
     * @brief Configure the scanner
     * @param config Configuration parameters
     * @return Result<void> Success or error
     *
     * Configures the analog output channels for scanning.
     */
    Result<void> configure(const ScannerConfig& config) override;

    /**
     * @brief Set scan pattern parameters
     * @param pattern Scan pattern configuration
     * @return Result<void> Success or error
     *
     * Sets the scan pattern and regenerates waveforms.
     * This can be called while scanning to change parameters.
     */
    Result<void> setScanPattern(const ScanPattern& pattern) override;

    /**
     * @brief Start scanning
     * @return Result<void> Success or error
     *
     * Starts continuous waveform output on both channels.
     *
     * @pre Scanner must be configured
     */
    Result<void> start() override;

    /**
     * @brief Stop scanning
     * @return Result<void> Success or error
     *
     * Stops waveform output and returns mirrors to center position.
     */
    Result<void> stop() override;

    /**
     * @brief Set scanner position for point scanning
     * @param x X position in Volts (-scanRange to +scanRange)
     * @param y Y position in Volts (-scanRange to +scanRange)
     * @return Result<void> Success or error
     *
     * Positions the mirrors at the specified location.
     * Only works when continuous scanning is not active.
     */
    Result<void> setPosition(double x, double y) override;

    /**
     * @brief Check if scanner is running
     * @return true if scanning, false otherwise
     */
    bool isRunning() const override;

    /**
     * @brief Get current configuration
     * @return Reference to current ScannerConfig
     */
    const ScannerConfig& config() const override;

    /**
     * @brief Get current scan pattern
     * @return Reference to current ScanPattern
     */
    const ScanPattern& pattern() const override;

    /**
     * @brief Get X waveform data
     * @return Reference to X waveform vector
     *
     * Returns the voltage values for the X-axis mirror.
     * Useful for synchronization with detector and descanning.
     */
    const std::vector<double>& waveformX() const override;

    /**
     * @brief Get Y waveform data
     * @return Reference to Y waveform vector
     *
     * Returns the voltage values for the Y-axis mirror.
     */
    const std::vector<double>& waveformY() const override;

    /// @}

private:
    std::string deviceName_;           ///< DAQ device name
    TaskHandle taskHandle_ = nullptr;  ///< NI-DAQmx task handle
    ScannerConfig config_;             ///< Current configuration
    ScanPattern pattern_;              ///< Current scan pattern
    std::vector<double> waveformX_;    ///< X-axis waveform
    std::vector<double> waveformY_;    ///< Y-axis waveform
    std::atomic<bool> running_{false}; ///< Running state flag
    std::atomic<bool> configured_{false}; ///< Configuration state flag

    /**
     * @brief Create AO task for scanning
     * @return Result<void> Success or error
     */
    Result<void> createAOTask();

    /**
     * @brief Generate galvo scan waveforms
     *
     * Generates X and Y voltage waveforms based on current
     * configuration and scan pattern.
     */
    void generateGalvoWaveform();

    /**
     * @brief Write waveforms to DAQ
     * @return Result<void> Success or error
     */
    Result<void> writeWaveforms();

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
