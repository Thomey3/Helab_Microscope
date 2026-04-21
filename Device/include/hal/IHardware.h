#pragma once

#include "../common/Result.h"
#include <string>
#include <memory>
#include <vector>
#include <cstdint>
#include <functional>

/**
 * @file IHardware.h
 * @brief Hardware Abstraction Layer interfaces for the Helab Two-Photon Microscope
 *
 * This file defines the core interfaces for the Hardware Abstraction Layer (HAL).
 * The HAL provides a clean separation between the microscope control logic and
 * the specific hardware implementations, enabling:
 *
 * - **Testability**: Mock implementations can be used for unit testing
 * - **Portability**: New hardware can be added by implementing these interfaces
 * - **Maintainability**: Hardware changes don't affect application logic
 * - **Extensibility**: New features (resonance scanning, adaptive optics) can be added
 *
 * @section arch Architecture Overview
 *
 * The HAL follows the Strategy pattern where each interface defines a contract
 * that concrete implementations must fulfill:
 *
 * @code
 * IClock          - Clock/pulse generation (pixel clock, line clock)
 * IScanner        - Beam scanning control (galvo, resonance)
 * IDetector       - Data acquisition (QT card, digitizer)
 * ILaser          - Laser power control
 * IAdaptiveOptics - Wavefront correction (deformable mirror)
 * IHardwareFactory - Factory for creating hardware instances
 * @endcode
 *
 * @section usage Usage Example
 *
 * @code
 * // Create hardware factory
 * auto factory = std::make_shared<HardwareFactory>();
 *
 * // Create clock
 * auto clock = factory->createClock("nidaq");
 * ClockConfig config;
 * config.pixelCount = 512;
 * clock->configure(config);
 * clock->start();
 * @endcode
 *
 * @see HardwareFactory for default implementations
 * @see MicroscopeController for high-level API
 */

namespace helab {

// ============================================================================
// Clock Interface
// ============================================================================

/**
 * @struct ClockConfig
 * @brief Configuration parameters for the pixel/line clock generator
 *
 * The clock generator produces timing signals that synchronize the scanner
 * and detector. In two-photon microscopy, this typically generates:
 * - Pixel clock: Marks each pixel acquisition
 * - Line clock: Marks the start of each scan line
 * - Frame clock: Marks the start of each frame
 *
 * @see IClock
 */
struct ClockConfig {
    std::string deviceName;           ///< NI-DAQ device name (e.g., "Dev1")
    std::string counterChannel;       ///< Counter channel (e.g., "ctr0")
    std::string triggerChannel;       ///< Trigger line (e.g., "PFI0")
    uint32_t pixelCount;              ///< Pixels per line
    double dutyCycle;                 ///< Pixel clock duty cycle (0.0-1.0)
    uint32_t totalPixels;             ///< Total pixels per frame

    /**
     * @enum TimebaseSource
     * @brief Timebase source for counter timing
     */
    enum class TimebaseSource {
        HZ_20_MHZ,   ///< 20 MHz onboard timebase
        HZ_80_MHZ,   ///< 80 MHz onboard timebase (default, highest resolution)
        PFI          ///< External PFI input for custom timebase
    } timebaseSource = TimebaseSource::HZ_80_MHZ;

    double timebaseFrequency = 80e6;  ///< Timebase frequency in Hz
    double lineRate = 1000.0;         ///< Line rate in Hz

    /**
     * @brief Calculate pixel time in seconds
     * @return Time per pixel in seconds
     */
    double pixelTime() const {
        return 1.0 / (lineRate * pixelCount);
    }
};

/**
 * @class IClock
 * @brief Interface for clock/pulse generation
 *
 * The clock generator produces timing signals for synchronized imaging.
 * It typically uses a counter/timer to generate precise pixel and line clocks.
 *
 * @section impl Implementations
 * - NidaqClock: NI-DAQmx based implementation
 *
 * @section example Example
 * @code
 * auto clock = factory->createClock("nidaq");
 *
 * ClockConfig config;
 * config.deviceName = "Dev1";
 * config.counterChannel = "ctr0";
 * config.pixelCount = 512;
 * config.lineRate = 1000.0;
 *
 * auto result = clock->configure(config);
 * if (result) {
 *     clock->start();
 * }
 * @endcode
 */
class IClock {
public:
    virtual ~IClock() = default;

    /**
     * @brief Configure the clock generator
     * @param config Configuration parameters
     * @return Result<void> Success or error
     * @throws HardwareError if configuration fails
     */
    virtual Result<void> configure(const ClockConfig& config) = 0;

    /**
     * @brief Start clock generation
     * @return Result<void> Success or error
     * @note Clock must be configured before starting
     */
    virtual Result<void> start() = 0;

    /**
     * @brief Stop clock generation
     * @return Result<void> Success or error
     */
    virtual Result<void> stop() = 0;

    /**
     * @brief Reset to initial state
     * @return Result<void> Success or error
     */
    virtual Result<void> reset() = 0;

    /**
     * @brief Check if clock is running
     * @return true if running, false otherwise
     */
    virtual bool isRunning() const = 0;

    /**
     * @brief Get current configuration
     * @return Reference to current ClockConfig
     */
    virtual const ClockConfig& config() const = 0;
};

// ============================================================================
// Scanner Interface
// ============================================================================

/**
 * @struct ScannerConfig
 * @brief Configuration parameters for the beam scanner
 *
 * The scanner controls the position of the laser beam in the sample plane.
 * Configuration includes output channels, scan range, and timing parameters.
 */
struct ScannerConfig {
    std::string deviceName;           ///< NI-DAQ device name
    std::string xChannel;             ///< X-axis output channel (e.g., "ao0")
    std::string yChannel;             ///< Y-axis output channel (e.g., "ao1")
    double scanRange;                 ///< Scan range in Volts (±)
    double frequency;                 ///< Scan frequency in Hz
    uint32_t samplesPerLine;          ///< Samples per scan line
};

/**
 * @struct ScanPattern
 * @brief Scan pattern parameters defining how the beam moves
 *
 * Different scan patterns are optimized for different imaging modes:
 * - Galvo: Slow, high-resolution, suitable for most applications
 * - Resonance: Fast, lower resolution, good for live imaging
 * - Polygon: Very fast, specialized applications
 */
struct ScanPattern {
    /**
     * @enum Type
     * @brief Scan type enumeration
     */
    enum class Type {
        Galvo,       ///< Galvanometer scanner (slow, ~1.5 Hz frame rate)
        Resonance,   ///< Resonant scanner (fast, ~8 kHz line rate)
        Polygon      ///< Polygon mirror scanner
    } type = Type::Galvo;

    bool bidirectional = true;        ///< Bidirectional scanning (both directions imaged)
    double fillFraction = 0.8;        ///< Effective scan fraction (0.0-1.0)

    /// @name Galvo-specific parameters
    /// @{
    double galvoAmplitude = 5.0;      ///< Galvo amplitude in Volts
    /// @}

    /// @name Resonance-specific parameters
    /// @{
    double resonanceFrequency = 8000.0;  ///< Resonance frequency in Hz
    /// @}
};

/**
 * @class IScanner
 * @brief Interface for beam scanning control
 *
 * The scanner controls the X-Y position of the laser beam through galvanometer
 * mirrors or resonant scanners. It generates waveforms that define the scan pattern.
 *
 * @section impl Implementations
 * - GalvoScanner: Galvanometer-based scanning
 * - ResonanceScanner: Resonant scanner (planned)
 *
 * @section example Example
 * @code
 * auto scanner = factory->createScanner("galvo");
 *
 * ScannerConfig config;
 * config.xChannel = "ao0";
 * config.yChannel = "ao1";
 * config.scanRange = 5.0;
 * scanner->configure(config);
 *
 * ScanPattern pattern;
 * pattern.type = ScanPattern::Type::Galvo;
 * pattern.bidirectional = true;
 * scanner->setScanPattern(pattern);
 *
 * scanner->start();
 * @endcode
 */
class IScanner {
public:
    virtual ~IScanner() = default;

    /**
     * @brief Configure the scanner
     * @param config Configuration parameters
     * @return Result<void> Success or error
     */
    virtual Result<void> configure(const ScannerConfig& config) = 0;

    /**
     * @brief Set scan pattern parameters
     * @param pattern Scan pattern configuration
     * @return Result<void> Success or error
     */
    virtual Result<void> setScanPattern(const ScanPattern& pattern) = 0;

    /**
     * @brief Start scanning
     * @return Result<void> Success or error
     */
    virtual Result<void> start() = 0;

    /**
     * @brief Stop scanning
     * @return Result<void> Success or error
     */
    virtual Result<void> stop() = 0;

    /**
     * @brief Set scanner position for point scanning
     * @param x X position in Volts (-scanRange to +scanRange)
     * @param y Y position in Volts (-scanRange to +scanRange)
     * @return Result<void> Success or error
     * @note Only works when scanner is not running
     */
    virtual Result<void> setPosition(double x, double y) = 0;

    /**
     * @brief Check if scanner is running
     * @return true if running, false otherwise
     */
    virtual bool isRunning() const = 0;

    /**
     * @brief Get current configuration
     * @return Reference to current ScannerConfig
     */
    virtual const ScannerConfig& config() const = 0;

    /**
     * @brief Get current scan pattern
     * @return Reference to current ScanPattern
     */
    virtual const ScanPattern& pattern() const = 0;

    /**
     * @brief Get X waveform data
     * @return Reference to X waveform vector
     * @note Useful for synchronization and descanning
     */
    virtual const std::vector<double>& waveformX() const = 0;

    /**
     * @brief Get Y waveform data
     * @return Reference to Y waveform vector
     * @note Useful for synchronization and descanning
     */
    virtual const std::vector<double>& waveformY() const = 0;
};

// ============================================================================
// Detector Interface
// ============================================================================

/**
 * @struct DetectorConfig
 * @brief Configuration parameters for the data acquisition system
 *
 * The detector acquires analog signals from photomultiplier tubes (PMTs)
 * and digitizes them for image formation.
 */
struct DetectorConfig {
    uint32_t channelMask = 0x0F;      ///< Active channel bitmask (bits 0-3 for channels 1-4)
    uint32_t sampleRate = 100000000;  ///< Sample rate in Hz (100 MS/s default)
    uint32_t segmentSize = 512;       ///< Samples per segment (line)
    uint32_t numSegments = 512;       ///< Number of segments (lines per frame)

    /**
     * @enum TriggerMode
     * @brief Trigger mode for acquisition
     */
    enum class TriggerMode {
        Internal,    ///< Internal trigger (free-running)
        External,    ///< External trigger (from clock)
        Software     ///< Software trigger
    } triggerMode = TriggerMode::External;

    std::string triggerSource;        ///< Trigger source line
    double triggerLevel = 0.0;        ///< Trigger level in Volts
};

/**
 * @typedef DetectorCallback
 * @brief Callback function type for detector data
 * @param data Pointer to raw data buffer
 * @param size Size of data in bytes
 * @param timestamp Acquisition timestamp (nanoseconds since epoch)
 */
using DetectorCallback = std::function<void(const void* data, size_t size, uint64_t timestamp)>;

/**
 * @class IDetector
 * @brief Interface for data acquisition
 *
 * The detector handles high-speed digitization of PMT signals.
 * It supports multi-channel acquisition and continuous streaming.
 *
 * @section impl Implementations
 * - QtDetector: QT12136DC high-speed digitizer card
 *
 * @section example Example
 * @code
 * auto detector = factory->createDetector("qt");
 *
 * DetectorConfig config;
 * config.channelMask = 0x01;  // Channel 1 only
 * config.sampleRate = 100000000;
 * config.segmentSize = 512;
 * detector->configure(config);
 *
 * // Set callback for continuous acquisition
 * detector->setDataCallback([](const void* data, size_t size, uint64_t ts) {
 *     // Process data
 * });
 *
 * detector->startAcquisition();
 * @endcode
 */
class IDetector {
public:
    virtual ~IDetector() = default;

    /**
     * @brief Configure the detector
     * @param config Configuration parameters
     * @return Result<void> Success or error
     */
    virtual Result<void> configure(const DetectorConfig& config) = 0;

    /**
     * @brief Start acquisition
     * @return Result<void> Success or error
     */
    virtual Result<void> startAcquisition() = 0;

    /**
     * @brief Stop acquisition
     * @return Result<void> Success or error
     */
    virtual Result<void> stopAcquisition() = 0;

    /**
     * @brief Get raw data buffer pointer
     * @return Result<void*> Pointer to buffer or error
     * @note Buffer contents are valid until next acquisition call
     */
    virtual Result<void*> getDataBuffer() = 0;

    /**
     * @brief Get available data size
     * @return Result<size_t> Size in bytes or error
     */
    virtual Result<size_t> getAvailableDataSize() = 0;

    /**
     * @brief Set data callback for continuous acquisition
     * @param callback Function to call when data is available
     */
    virtual void setDataCallback(DetectorCallback callback) = 0;

    /**
     * @brief Check if acquiring
     * @return true if acquiring, false otherwise
     */
    virtual bool isAcquiring() const = 0;

    /**
     * @brief Get current configuration
     * @return Reference to current DetectorConfig
     */
    virtual const DetectorConfig& config() const = 0;

    /**
     * @brief Get number of active channels
     * @return Number of channels based on channelMask
     */
    virtual uint32_t activeChannelCount() const = 0;
};

// ============================================================================
// Laser Interface
// ============================================================================

/**
 * @struct LaserConfig
 * @brief Configuration parameters for laser control
 */
struct LaserConfig {
    std::string name;                 ///< Laser name/identifier (e.g., "920nm", "1064nm")
    double wavelength;                ///< Wavelength in nm
    double maxPower;                  ///< Maximum power in mW
    std::string port;                 ///< Control port (COM, TCP, etc.)
};

/**
 * @class ILaser
 * @brief Interface for laser control
 *
 * The laser interface provides power control and enable/disable functionality.
 * Multiple lasers can be controlled independently.
 *
 * @section impl Implementations
 * - GenericLaser: Generic serial/USB controlled laser
 *
 * @section example Example
 * @code
 * auto laser = factory->createLaser("920nm");
 * laser->initialize();
 *
 * laser->setPower(500.0);  // 500 mW
 * laser->enable(true);
 *
 * // ... imaging ...
 *
 * laser->enable(false);
 * @endcode
 */
class ILaser {
public:
    virtual ~ILaser() = default;

    /**
     * @brief Initialize laser connection
     * @return Result<void> Success or error
     */
    virtual Result<void> initialize() = 0;

    /**
     * @brief Set output power
     * @param power_mW Power in milliwatts
     * @return Result<void> Success or error
     * @note Power is clamped to [0, maxPower]
     */
    virtual Result<void> setPower(double power_mW) = 0;

    /**
     * @brief Enable/disable laser output
     * @param enable true to enable, false to disable
     * @return Result<void> Success or error
     * @warning Laser must be enabled before output is active
     */
    virtual Result<void> enable(bool enable) = 0;

    /**
     * @brief Get current power setting
     * @return Result<double> Power in mW or error
     */
    virtual Result<double> getPower() const = 0;

    /**
     * @brief Get wavelength
     * @return Result<double> Wavelength in nm or error
     */
    virtual Result<double> getWavelength() const = 0;

    /**
     * @brief Check if laser is enabled
     * @return true if enabled, false otherwise
     */
    virtual bool isEnabled() const = 0;

    /**
     * @brief Check if laser is connected
     * @return true if connected, false otherwise
     */
    virtual bool isConnected() const = 0;

    /**
     * @brief Get laser name
     * @return Reference to laser name string
     */
    virtual const std::string& name() const = 0;
};

// ============================================================================
// Adaptive Optics Interface
// ============================================================================

/**
 * @struct AdaptiveOpticsConfig
 * @brief Configuration parameters for adaptive optics
 */
struct AdaptiveOpticsConfig {
    uint32_t actuatorCount;           ///< Number of actuators
    double maxStroke;                 ///< Maximum stroke in um
    std::string port;                 ///< Control port
};

/**
 * @class IAdaptiveOptics
 * @brief Interface for adaptive optics control
 *
 * Adaptive optics correct optical aberrations in real-time using
 * deformable mirrors or spatial light modulators (SLMs).
 *
 * @section impl Implementations
 * - Planned: Deformable mirror drivers (Boston Micromachines, ALPAO, etc.)
 *
 * @section example Example
 * @code
 * auto ao = factory->createAdaptiveOptics("dm");
 * ao->initialize();
 *
 * // Apply wavefront correction
 * std::vector<double> wavefront = computeWavefront();
 * ao->applyCorrection(wavefront);
 *
 * // Reset to flat
 * ao->reset();
 * @endcode
 */
class IAdaptiveOptics {
public:
    virtual ~IAdaptiveOptics() = default;

    /**
     * @brief Initialize adaptive optics
     * @return Result<void> Success or error
     */
    virtual Result<void> initialize() = 0;

    /**
     * @brief Set actuator positions directly
     * @param positions Vector of actuator positions (normalized 0-1)
     * @return Result<void> Success or error
     */
    virtual Result<void> setActuatorPositions(const std::vector<double>& positions) = 0;

    /**
     * @brief Apply wavefront correction
     * @param wavefront Wavefront coefficients (Zernike or similar)
     * @return Result<void> Success or error
     */
    virtual Result<void> applyCorrection(const std::vector<double>& wavefront) = 0;

    /**
     * @brief Reset to flat state
     * @return Result<void> Success or error
     */
    virtual Result<void> reset() = 0;

    /**
     * @brief Get current actuator positions
     * @return Result<std::vector<double>> Positions or error
     */
    virtual Result<std::vector<double>> getActuatorPositions() const = 0;

    /**
     * @brief Check if connected
     * @return true if connected, false otherwise
     */
    virtual bool isConnected() const = 0;
};

// ============================================================================
// Hardware Factory Interface
// ============================================================================

/**
 * @class IHardwareFactory
 * @brief Factory interface for creating hardware instances
 *
 * The factory provides a centralized way to create hardware objects,
 * enabling dependency injection and testability.
 *
 * @section impl Implementations
 * - HardwareFactory: Default factory with standard implementations
 * - ConfigurableHardwareFactory: Factory with custom device names
 *
 * @section example Example
 * @code
 * // Use default factory
 * auto factory = std::make_shared<HardwareFactory>();
 *
 * // Create hardware
 * auto clock = factory->createClock("nidaq");
 * auto scanner = factory->createScanner("galvo");
 * auto detector = factory->createDetector("qt");
 *
 * // Or use with MicroscopeController
 * auto controller = std::make_unique<MicroscopeController>(factory);
 * controller->initialize();
 * @endcode
 */
class IHardwareFactory {
public:
    virtual ~IHardwareFactory() = default;

    /**
     * @brief Create clock generator
     * @param type Clock type ("nidaq" for NI-DAQmx)
     * @return Unique pointer to IClock implementation
     */
    virtual std::unique_ptr<IClock> createClock(const std::string& type = "nidaq") = 0;

    /**
     * @brief Create scanner
     * @param type Scanner type ("galvo", "resonance")
     * @return Unique pointer to IScanner implementation
     */
    virtual std::unique_ptr<IScanner> createScanner(const std::string& type = "galvo") = 0;

    /**
     * @brief Create detector
     * @param type Detector type ("qt" for QT12136DC)
     * @return Unique pointer to IDetector implementation
     */
    virtual std::unique_ptr<IDetector> createDetector(const std::string& type = "qt") = 0;

    /**
     * @brief Create laser
     * @param type Laser type ("generic", "920nm", "1064nm", "808nm")
     * @return Unique pointer to ILaser implementation
     */
    virtual std::unique_ptr<ILaser> createLaser(const std::string& type = "generic") = 0;

    /**
     * @brief Create adaptive optics
     * @param type AO type ("dm" for deformable mirror)
     * @return Unique pointer to IAdaptiveOptics implementation
     */
    virtual std::unique_ptr<IAdaptiveOptics> createAdaptiveOptics(const std::string& type = "dm") = 0;
};

} // namespace helab
