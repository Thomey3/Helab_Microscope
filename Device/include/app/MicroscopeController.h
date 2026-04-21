#pragma once

#include "../hal/IHardware.h"
#include "../hal/HardwareFactory.h"
#include "../common/Result.h"
#include "../common/Logger.h"
#include <memory>
#include <map>
#include <string>
#include <functional>
#include <atomic>
#include <mutex>

namespace helab {

/// Microscope imaging parameters
struct ImagingConfig {
    double fps = 1.5;              /// Frames per second
    uint32_t width = 512;          /// Image width in pixels
    uint32_t height = 512;         /// Image height in pixels
    uint32_t channels = 1;         /// Number of detection channels
    double pixelSizeUm = 1.0;      /// Pixel size in micrometers
    double scanRangeV = 5.0;       /// Scan range in Volts
};

/// ROI configuration
struct ROI {
    uint32_t x = 0;
    uint32_t y = 0;
    uint32_t width = 512;
    uint32_t height = 512;
};

/// Microscope state enumeration
enum class MicroscopeState {
    Uninitialized,
    Ready,
    Imaging,
    Error
};

/// Convert state to string
inline std::string stateToString(MicroscopeState state) {
    switch (state) {
        case MicroscopeState::Uninitialized: return "Uninitialized";
        case MicroscopeState::Ready: return "Ready";
        case MicroscopeState::Imaging: return "Imaging";
        case MicroscopeState::Error: return "Error";
        default: return "Unknown";
    }
}

/// Image callback type
using ImageCallback = std::function<void(const std::vector<uint8_t>&, uint64_t timestamp)>;

/// Microscope controller - core business logic
class MicroscopeController {
public:
    /// Constructor with hardware factory
    explicit MicroscopeController(std::shared_ptr<IHardwareFactory> factory);

    /// Destructor
    ~MicroscopeController();

    // Rule of Five
    MicroscopeController(const MicroscopeController&) = delete;
    MicroscopeController& operator=(const MicroscopeController&) = delete;
    MicroscopeController(MicroscopeController&&) = delete;
    MicroscopeController& operator=(MicroscopeController&&) = delete;

    // ========================================================================
    // Lifecycle
    // ========================================================================

    /// Initialize all hardware components
    Result<void> initialize();

    /// Shutdown all hardware
    Result<void> shutdown();

    /// Check if initialized
    bool isInitialized() const;

    /// Get current state
    MicroscopeState state() const;

    // ========================================================================
    // Configuration
    // ========================================================================

    /// Set imaging parameters
    Result<void> setImagingConfig(const ImagingConfig& config);

    /// Get imaging parameters
    const ImagingConfig& imagingConfig() const;

    /// Set ROI
    Result<void> setROI(const ROI& roi);

    /// Get ROI
    const ROI& roi() const;

    /// Set scan type
    Result<void> setScanType(ScanPattern::Type type);

    /// Get scan type
    ScanPattern::Type scanType() const;

    // ========================================================================
    // Imaging Control
    // ========================================================================

    /// Start continuous imaging
    Result<void> startImaging();

    /// Stop imaging
    Result<void> stopImaging();

    /// Acquire single frame
    Result<std::vector<uint8_t>> acquireSingleFrame();

    /// Check if imaging
    bool isImaging() const;

    /// Set image callback
    void setImageCallback(ImageCallback callback);

    // ========================================================================
    // Laser Control (Extensible)
    // ========================================================================

    /// Add laser
    Result<void> addLaser(std::unique_ptr<ILaser> laser, const std::string& name);

    /// Add laser from factory by type
    Result<void> addLaserByType(const std::string& type, const std::string& name);

    /// Remove laser
    Result<void> removeLaser(const std::string& name);

    /// Set laser power
    Result<void> setLaserPower(const std::string& name, double power_mW);

    /// Enable/disable laser
    Result<void> enableLaser(const std::string& name, bool enable);

    /// Get laser names
    std::vector<std::string> laserNames() const;

    // ========================================================================
    // Adaptive Optics (Extensible)
    // ========================================================================

    /// Add adaptive optics
    Result<void> addAdaptiveOptics(std::unique_ptr<IAdaptiveOptics> ao);

    /// Apply wavefront correction
    Result<void> applyWavefrontCorrection(const std::vector<double>& wavefront);

    /// Reset adaptive optics
    Result<void> resetAdaptiveOptics();

    // ========================================================================
    // Hardware Access (for advanced use)
    // ========================================================================

    /// Get clock (read-only)
    IClock* clock() const;

    /// Get scanner (read-only)
    IScanner* scanner() const;

    /// Get detector (read-only)
    IDetector* detector() const;

private:
    // Hardware factory
    std::shared_ptr<IHardwareFactory> factory_;

    // Core hardware components
    std::unique_ptr<IClock> clock_;
    std::unique_ptr<IScanner> scanner_;
    std::unique_ptr<IDetector> detector_;

    // Optional extensions
    std::map<std::string, std::unique_ptr<ILaser>> lasers_;
    std::unique_ptr<IAdaptiveOptics> adaptiveOptics_;

    // Configuration
    ImagingConfig imagingConfig_;
    ROI roi_;
    ScanPattern scanPattern_;

    // State
    std::atomic<MicroscopeState> state_{MicroscopeState::Uninitialized};
    std::atomic<bool> imaging_{false};
    std::mutex mutex_;

    // Callbacks
    ImageCallback imageCallback_;

    // Internal methods
    Result<void> initializeHardware();
    Result<void> configureClock();
    Result<void> configureScanner();
    Result<void> configureDetector();
    void handleDetectorData(const void* data, size_t size, uint64_t timestamp);
};

} // namespace helab
