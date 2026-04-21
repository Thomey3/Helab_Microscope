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

/**
 * @file MicroscopeController.h
 * @brief Core microscope controller - high-level API for two-photon imaging
 *
 * The MicroscopeController is the main application-layer class that coordinates
 * all hardware components for two-photon microscopy. It provides a clean,
 * high-level API that hides the complexity of hardware synchronization.
 *
 * @section arch Architecture Position
 *
 * The controller sits between the Micro-Manager adapter layer and the HAL:
 *
 * @code
 * Micro-Manager Layer (HelabCamera, HelabLaserDevice)
 *            │
 *            ▼
 * Application Layer (MicroscopeController)  ← This file
 *            │
 *            ▼
 * Hardware Abstraction Layer (IClock, IScanner, IDetector, ILaser)
 *            │
 *            ▼
 * Hardware Drivers (NidaqClock, GalvoScanner, QtDetector)
 * @endcode
 *
 * @section features Features
 *
 * - **Hardware Management**: Automatic initialization and coordination
 * - **Imaging Control**: Start/stop continuous or single-frame acquisition
 * - **Laser Control**: Multi-laser support with individual power control
 * - **ROI Scanning**: Configurable region of interest
 * - **Extensibility**: Adaptive optics, resonance scanning (planned)
 *
 * @section example Basic Usage
 *
 * @code
 * // Create factory and controller
 * auto factory = std::make_shared<helab::HardwareFactory>();
 * helab::MicroscopeController controller(factory);
 *
 * // Initialize hardware
 * auto result = controller.initialize();
 * if (!result) {
 *     std::cerr << "Init failed: " << result.error().message << std::endl;
 *     return;
 * }
 *
 * // Configure imaging parameters
 * helab::ImagingConfig imaging;
 * imaging.fps = 1.5;
 * imaging.width = 512;
 * imaging.height = 512;
 * controller.setImagingConfig(imaging);
 *
 * // Set image callback
 * controller.setImageCallback([](const std::vector<uint8_t>& img, uint64_t ts) {
 *     std::cout << "Received image at " << ts << std::endl;
 * });
 *
 * // Start imaging
 * controller.startImaging();
 *
 * // ... run for some time ...
 *
 * controller.stopImaging();
 * controller.shutdown();
 * @endcode
 *
 * @section laser Multi-Laser Example
 *
 * @code
 * // Add multiple lasers
 * controller.addLaserByType("920nm", "primary");
 * controller.addLaserByType("1064nm", "secondary");
 *
 * // Control individual lasers
 * controller.setLaserPower("primary", 500.0);  // 500 mW
 * controller.enableLaser("primary", true);
 *
 * controller.setLaserPower("secondary", 300.0);
 * controller.enableLaser("secondary", true);
 * @endcode
 *
 * @see IHardwareFactory
 * @see IClock
 * @see IScanner
 * @see IDetector
 * @see ILaser
 */

namespace helab {

/**
 * @struct ImagingConfig
 * @brief Microscope imaging parameters
 *
 * Defines the imaging configuration including frame rate, resolution,
 * and physical parameters.
 */
struct ImagingConfig {
    double fps = 1.5;              ///< Frames per second
    uint32_t width = 512;          ///< Image width in pixels
    uint32_t height = 512;         ///< Image height in pixels
    uint32_t channels = 1;         ///< Number of detection channels
    double pixelSizeUm = 1.0;      ///< Pixel size in micrometers
    double scanRangeV = 5.0;       ///< Scan range in Volts (±)

    /**
     * @brief Calculate total pixels per frame
     * @return Width × Height
     */
    uint32_t totalPixels() const { return width * height; }

    /**
     * @brief Calculate field of view
     * @return FOV in micrometers (width, height)
     */
    std::pair<double, double> fieldOfView() const {
        return {width * pixelSizeUm, height * pixelSizeUm};
    }
};

/**
 * @struct ROI
 * @brief Region of interest configuration
 *
 * Defines a rectangular region within the full scan field.
 * Used for zoomed imaging or specific sample regions.
 */
struct ROI {
    uint32_t x = 0;        ///< X offset in pixels
    uint32_t y = 0;        ///< Y offset in pixels
    uint32_t width = 512;  ///< ROI width in pixels
    uint32_t height = 512; ///< ROI height in pixels

    /**
     * @brief Check if ROI is valid
     * @param maxWidth Maximum image width
     * @param maxHeight Maximum image height
     * @return true if ROI is within bounds
     */
    bool isValid(uint32_t maxWidth, uint32_t maxHeight) const {
        return x + width <= maxWidth && y + height <= maxHeight;
    }
};

/**
 * @enum MicroscopeState
 * @brief Microscope controller state enumeration
 *
 * The state machine transitions:
 * @code
 * Uninitialized → Ready → Imaging → Ready
 *       ↓            ↓        ↓
 *    Error        Error    Error
 * @endcode
 */
enum class MicroscopeState {
    Uninitialized,  ///< Hardware not initialized
    Ready,          ///< Ready for imaging
    Imaging,        ///< Currently acquiring images
    Error           ///< Error state
};

/**
 * @brief Convert state to string
 * @param state State to convert
 * @return String representation
 */
inline std::string stateToString(MicroscopeState state) {
    switch (state) {
        case MicroscopeState::Uninitialized: return "Uninitialized";
        case MicroscopeState::Ready: return "Ready";
        case MicroscopeState::Imaging: return "Imaging";
        case MicroscopeState::Error: return "Error";
        default: return "Unknown";
    }
}

/**
 * @typedef ImageCallback
 * @brief Callback function type for received images
 * @param image Image data as byte vector
 * @param timestamp Acquisition timestamp (nanoseconds since epoch)
 */
using ImageCallback = std::function<void(const std::vector<uint8_t>&, uint64_t timestamp)>;

/**
 * @class MicroscopeController
 * @brief Core microscope controller - business logic coordination
 *
 * The MicroscopeController manages all hardware components and provides
 * a high-level API for two-photon imaging operations.
 *
 * @section lifecycle Lifecycle
 *
 * 1. Create controller with hardware factory
 * 2. Call initialize() to set up hardware
 * 3. Configure imaging parameters
 * 4. Start/stop imaging as needed
 * 5. Call shutdown() to release resources
 *
 * @section threadsafety Thread Safety
 *
 * The controller is thread-safe:
 * - All public methods can be called from any thread
 * - Internal state is protected by mutexes
 * - Callbacks are invoked from acquisition thread
 *
 * @section error Error Handling
 *
 * All operations return Result<T> which contains either:
 * - The expected value on success
 * - A HardwareError with code and message on failure
 */
class MicroscopeController {
public:
    /**
     * @brief Constructor with hardware factory
     * @param factory Shared pointer to hardware factory
     *
     * Creates a controller that will use the provided factory
     * to create hardware instances during initialization.
     *
     * @code
     * auto factory = std::make_shared<HardwareFactory>();
     * MicroscopeController controller(factory);
     * @endcode
     */
    explicit MicroscopeController(std::shared_ptr<IHardwareFactory> factory);

    /**
     * @brief Destructor
     *
     * Automatically calls shutdown() if not already done.
     */
    ~MicroscopeController();

    /// @name Rule of Five
    /// @{
    MicroscopeController(const MicroscopeController&) = delete;
    MicroscopeController& operator=(const MicroscopeController&) = delete;
    MicroscopeController(MicroscopeController&&) = delete;
    MicroscopeController& operator=(MicroscopeController&&) = delete;
    /// @}

    // ========================================================================
    // Lifecycle
    // ========================================================================

    /**
     * @brief Initialize all hardware components
     * @return Result<void> Success or error
     *
     * Creates and configures all hardware instances:
     * - Clock generator
     * - Scanner
     * - Detector
     *
     * After initialization, the controller is in Ready state.
     *
     * @note This may take several seconds for hardware initialization
     */
    Result<void> initialize();

    /**
     * @brief Shutdown all hardware
     * @return Result<void> Success or error
     *
     * Stops any active imaging and releases all hardware resources.
     * After shutdown, the controller is in Uninitialized state.
     */
    Result<void> shutdown();

    /**
     * @brief Check if initialized
     * @return true if hardware is initialized
     */
    bool isInitialized() const;

    /**
     * @brief Get current state
     * @return Current MicroscopeState
     */
    MicroscopeState state() const;

    // ========================================================================
    // Configuration
    // ========================================================================

    /**
     * @brief Set imaging parameters
     * @param config Imaging configuration
     * @return Result<void> Success or error
     *
     * Configures frame rate, resolution, and other imaging parameters.
     * Cannot be changed while imaging is active.
     */
    Result<void> setImagingConfig(const ImagingConfig& config);

    /**
     * @brief Get imaging parameters
     * @return Reference to current ImagingConfig
     */
    const ImagingConfig& imagingConfig() const;

    /**
     * @brief Set region of interest
     * @param roi ROI configuration
     * @return Result<void> Success or error
     *
     * Sets the imaging region within the full scan field.
     */
    Result<void> setROI(const ROI& roi);

    /**
     * @brief Get region of interest
     * @return Reference to current ROI
     */
    const ROI& roi() const;

    /**
     * @brief Set scan type
     * @param type Scan pattern type (Galvo, Resonance)
     * @return Result<void> Success or error
     *
     * Changes the scanning mode. Requires reconfiguration of scanner.
     */
    Result<void> setScanType(ScanPattern::Type type);

    /**
     * @brief Get scan type
     * @return Current scan pattern type
     */
    ScanPattern::Type scanType() const;

    // ========================================================================
    // Imaging Control
    // ========================================================================

    /**
     * @brief Start continuous imaging
     * @return Result<void> Success or error
     *
     * Starts continuous image acquisition. Images are delivered
     * via the registered callback.
     *
     * @pre Controller must be initialized and in Ready state
     * @pre Image callback should be set before starting
     */
    Result<void> startImaging();

    /**
     * @brief Stop imaging
     * @return Result<void> Success or error
     *
     * Stops continuous acquisition and returns to Ready state.
     */
    Result<void> stopImaging();

    /**
     * @brief Acquire single frame
     * @return Result<std::vector<uint8_t>> Image data or error
     *
     * Acquires a single frame and returns the image data.
     * This is a blocking call that waits for frame completion.
     */
    Result<std::vector<uint8_t>> acquireSingleFrame();

    /**
     * @brief Check if imaging
     * @return true if actively imaging
     */
    bool isImaging() const;

    /**
     * @brief Set image callback
     * @param callback Function to call when images are received
     *
     * The callback is invoked for each frame during continuous imaging.
     * @note Callback is invoked from acquisition thread
     */
    void setImageCallback(ImageCallback callback);

    // ========================================================================
    // Laser Control (Extensible)
    // ========================================================================

    /**
     * @brief Add laser instance
     * @param laser Unique pointer to ILaser implementation
     * @param name Name identifier for this laser
     * @return Result<void> Success or error
     *
     * Adds a laser to the controller for power control.
     */
    Result<void> addLaser(std::unique_ptr<ILaser> laser, const std::string& name);

    /**
     * @brief Add laser from factory by type
     * @param type Laser type string (e.g., "920nm", "1064nm")
     * @param name Name identifier for this laser
     * @return Result<void> Success or error
     *
     * Convenience method to create and add a laser using the factory.
     */
    Result<void> addLaserByType(const std::string& type, const std::string& name);

    /**
     * @brief Remove laser
     * @param name Name of laser to remove
     * @return Result<void> Success or error
     */
    Result<void> removeLaser(const std::string& name);

    /**
     * @brief Set laser power
     * @param name Laser name
     * @param power_mW Power in milliwatts
     * @return Result<void> Success or error
     */
    Result<void> setLaserPower(const std::string& name, double power_mW);

    /**
     * @brief Enable/disable laser
     * @param name Laser name
     * @param enable true to enable, false to disable
     * @return Result<void> Success or error
     */
    Result<void> enableLaser(const std::string& name, bool enable);

    /**
     * @brief Get list of laser names
     * @return Vector of laser name strings
     */
    std::vector<std::string> laserNames() const;

    // ========================================================================
    // Adaptive Optics (Extensible)
    // ========================================================================

    /**
     * @brief Add adaptive optics
     * @param ao Unique pointer to IAdaptiveOptics implementation
     * @return Result<void> Success or error
     *
     * Adds adaptive optics for wavefront correction.
     */
    Result<void> addAdaptiveOptics(std::unique_ptr<IAdaptiveOptics> ao);

    /**
     * @brief Apply wavefront correction
     * @param wavefront Wavefront coefficients
     * @return Result<void> Success or error
     *
     * Applies the specified wavefront correction to the adaptive optics.
     */
    Result<void> applyWavefrontCorrection(const std::vector<double>& wavefront);

    /**
     * @brief Reset adaptive optics
     * @return Result<void> Success or error
     *
     * Resets the adaptive optics to flat (no correction).
     */
    Result<void> resetAdaptiveOptics();

    // ========================================================================
    // Hardware Access (for advanced use)
    // ========================================================================

    /**
     * @brief Get clock (read-only)
     * @return Pointer to IClock or nullptr if not initialized
     *
     * @note For advanced use only. Prefer high-level API methods.
     */
    IClock* clock() const;

    /**
     * @brief Get scanner (read-only)
     * @return Pointer to IScanner or nullptr if not initialized
     */
    IScanner* scanner() const;

    /**
     * @brief Get detector (read-only)
     * @return Pointer to IDetector or nullptr if not initialized
     */
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
