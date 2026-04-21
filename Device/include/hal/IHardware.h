#pragma once

#include "../common/Result.h"
#include <string>
#include <memory>
#include <vector>
#include <cstdint>
#include <functional>

namespace helab {

// ============================================================================
// Clock Interface
// ============================================================================

/// Clock configuration parameters
struct ClockConfig {
    std::string deviceName;           /// NI-DAQ device name (e.g., "Dev1")
    std::string counterChannel;       /// Counter channel (e.g., "ctr0")
    std::string triggerChannel;       /// Trigger line (e.g., "PFI0")
    uint32_t pixelCount;              /// Pixels per line
    double dutyCycle;                 /// Pixel clock duty cycle (0.0-1.0)
    uint32_t totalPixels;             /// Total pixels per frame

    /// Timebase source selection
    enum class TimebaseSource {
        HZ_20_MHZ,   /// 20 MHz onboard timebase
        HZ_80_MHZ,   /// 80 MHz onboard timebase
        PFI          /// External PFI input
    } timebaseSource = TimebaseSource::HZ_80_MHZ;

    double timebaseFrequency = 80e6;  /// Timebase frequency in Hz
    double lineRate = 1000.0;         /// Line rate in Hz
};

/// Clock generator interface
class IClock {
public:
    virtual ~IClock() = default;

    /// Configure the clock generator
    virtual Result<void> configure(const ClockConfig& config) = 0;

    /// Start clock generation
    virtual Result<void> start() = 0;

    /// Stop clock generation
    virtual Result<void> stop() = 0;

    /// Reset to initial state
    virtual Result<void> reset() = 0;

    /// Check if clock is running
    virtual bool isRunning() const = 0;

    /// Get current configuration
    virtual const ClockConfig& config() const = 0;
};

// ============================================================================
// Scanner Interface
// ============================================================================

/// Scanner configuration parameters
struct ScannerConfig {
    std::string deviceName;           /// NI-DAQ device name
    std::string xChannel;             /// X-axis output channel (e.g., "ao0")
    std::string yChannel;             /// Y-axis output channel (e.g., "ao1")
    double scanRange;                 /// Scan range in Volts (±)
    double frequency;                 /// Scan frequency in Hz
    uint32_t samplesPerLine;          /// Samples per scan line
};

/// Scan pattern parameters
struct ScanPattern {
    /// Scan type enumeration
    enum class Type {
        Galvo,       /// Galvanometer scanner (slow, ~1.5 Hz frame rate)
        Resonance,   /// Resonant scanner (fast, ~8 kHz line rate)
        Polygon      /// Polygon mirror scanner
    } type = Type::Galvo;

    bool bidirectional = true;        /// Bidirectional scanning
    double fillFraction = 0.8;        /// Effective scan fraction (0.0-1.0)

    /// Galvo-specific parameters
    double galvoAmplitude = 5.0;      /// Galvo amplitude in Volts

    /// Resonance-specific parameters
    double resonanceFrequency = 8000.0;  /// Resonance frequency in Hz
};

/// Scanner interface (supports multiple scan modes)
class IScanner {
public:
    virtual ~IScanner() = default;

    /// Configure the scanner
    virtual Result<void> configure(const ScannerConfig& config) = 0;

    /// Set scan pattern parameters
    virtual Result<void> setScanPattern(const ScanPattern& pattern) = 0;

    /// Start scanning
    virtual Result<void> start() = 0;

    /// Stop scanning
    virtual Result<void> stop() = 0;

    /// Set scanner position (for point scanning)
    virtual Result<void> setPosition(double x, double y) = 0;

    /// Check if scanner is running
    virtual bool isRunning() const = 0;

    /// Get current configuration
    virtual const ScannerConfig& config() const = 0;

    /// Get current scan pattern
    virtual const ScanPattern& pattern() const = 0;

    /// Get X waveform data (for synchronization)
    virtual const std::vector<double>& waveformX() const = 0;

    /// Get Y waveform data (for synchronization)
    virtual const std::vector<double>& waveformY() const = 0;
};

// ============================================================================
// Detector Interface
// ============================================================================

/// Detector configuration parameters
struct DetectorConfig {
    uint32_t channelMask = 0x0F;      /// Active channel bitmask
    uint32_t sampleRate = 100000000;  /// Sample rate in Hz (100 MS/s default)
    uint32_t segmentSize = 512;       /// Samples per segment
    uint32_t numSegments = 512;       /// Number of segments (lines)

    /// Trigger mode
    enum class TriggerMode {
        Internal,    /// Internal trigger
        External,    /// External trigger
        Software     /// Software trigger
    } triggerMode = TriggerMode::External;

    std::string triggerSource;        /// Trigger source line
    double triggerLevel = 0.0;        /// Trigger level in Volts
};

/// Detector data callback type
using DetectorCallback = std::function<void(const void* data, size_t size, uint64_t timestamp)>;

/// Detector interface (data acquisition)
class IDetector {
public:
    virtual ~IDetector() = default;

    /// Configure the detector
    virtual Result<void> configure(const DetectorConfig& config) = 0;

    /// Start acquisition
    virtual Result<void> startAcquisition() = 0;

    /// Stop acquisition
    virtual Result<void> stopAcquisition() = 0;

    /// Get raw data buffer pointer
    virtual Result<void*> getDataBuffer() = 0;

    /// Get available data size
    virtual Result<size_t> getAvailableDataSize() = 0;

    /// Set data callback for continuous acquisition
    virtual void setDataCallback(DetectorCallback callback) = 0;

    /// Check if acquiring
    virtual bool isAcquiring() const = 0;

    /// Get current configuration
    virtual const DetectorConfig& config() const = 0;

    /// Get number of active channels
    virtual uint32_t activeChannelCount() const = 0;
};

// ============================================================================
// Laser Interface
// ============================================================================

/// Laser configuration parameters
struct LaserConfig {
    std::string name;                 /// Laser name/identifier
    double wavelength;                /// Wavelength in nm
    double maxPower;                  /// Maximum power in mW
    std::string port;                 /// Control port (COM, TCP, etc.)
};

/// Laser interface
class ILaser {
public:
    virtual ~ILaser() = default;

    /// Initialize laser connection
    virtual Result<void> initialize() = 0;

    /// Set output power
    virtual Result<void> setPower(double power_mW) = 0;

    /// Enable/disable laser output
    virtual Result<void> enable(bool enable) = 0;

    /// Get current power setting
    virtual Result<double> getPower() const = 0;

    /// Get wavelength
    virtual Result<double> getWavelength() const = 0;

    /// Check if laser is enabled
    virtual bool isEnabled() const = 0;

    /// Check if laser is connected
    virtual bool isConnected() const = 0;

    /// Get laser name
    virtual const std::string& name() const = 0;
};

// ============================================================================
// Adaptive Optics Interface
// ============================================================================

/// Adaptive optics configuration
struct AdaptiveOpticsConfig {
    uint32_t actuatorCount;           /// Number of actuators
    double maxStroke;                 /// Maximum stroke in um
    std::string port;                 /// Control port
};

/// Adaptive optics interface (deformable mirror, SLM, etc.)
class IAdaptiveOptics {
public:
    virtual ~IAdaptiveOptics() = default;

    /// Initialize adaptive optics
    virtual Result<void> initialize() = 0;

    /// Set actuator positions
    virtual Result<void> setActuatorPositions(const std::vector<double>& positions) = 0;

    /// Apply wavefront correction
    virtual Result<void> applyCorrection(const std::vector<double>& wavefront) = 0;

    /// Reset to flat state
    virtual Result<void> reset() = 0;

    /// Get current actuator positions
    virtual Result<std::vector<double>> getActuatorPositions() const = 0;

    /// Check if connected
    virtual bool isConnected() const = 0;
};

// ============================================================================
// Hardware Factory Interface
// ============================================================================

/// Hardware factory interface for dependency injection
class IHardwareFactory {
public:
    virtual ~IHardwareFactory() = default;

    /// Create clock generator
    virtual std::unique_ptr<IClock> createClock(const std::string& type = "nidaq") = 0;

    /// Create scanner
    virtual std::unique_ptr<IScanner> createScanner(const std::string& type = "galvo") = 0;

    /// Create detector
    virtual std::unique_ptr<IDetector> createDetector(const std::string& type = "qt") = 0;

    /// Create laser
    virtual std::unique_ptr<ILaser> createLaser(const std::string& type = "generic") = 0;

    /// Create adaptive optics
    virtual std::unique_ptr<IAdaptiveOptics> createAdaptiveOptics(const std::string& type = "dm") = 0;
};

} // namespace helab
