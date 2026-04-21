#pragma once

#include "DeviceBase.h"
#include "../app/MicroscopeController.h"
#include <memory>
#include <string>

/**
 * @file HelabDevices.h
 * @brief Micro-Manager device adapter classes for Helab Two-Photon Microscope
 *
 * This file provides the Micro-Manager device adapter implementation that
 * bridges the Helab microscope hardware to the Micro-Manager software platform.
 *
 * @section devices Device Classes
 *
 * | Class | Type | Description |
 * |-------|------|-------------|
 * | HelabMicroscopeHub | Hub | Main hub device, manages controller |
 * | HelabCamera | Camera | Two-photon camera device |
 * | HelabLaserDevice | Generic | Laser control device |
 *
 * @section arch Architecture
 *
 * @code
 * Micro-Manager Studio
 *        │
 *        ▼
 * HelabMicroscopeHub (Hub Device)
 *        │
 *        ├── HelabCamera (Camera Device)
 *        │       │
 *        │       └── MicroscopeController
 *        │               │
 *        │               ├── IClock
 *        │               ├── IScanner
 *        │               └── IDetector
 *        │
 *        └── HelabLaserDevice (Generic Device)
 *                │
 *                └── ILaser
 * @endcode
 *
 * @section usage Usage in Micro-Manager
 *
 * 1. Add HelabMicroscopeHub to your configuration
 * 2. The hub automatically creates child devices (Camera, Lasers)
 * 3. Use standard Micro-Manager controls for imaging
 *
 * @section properties Device Properties
 *
 * HelabCamera exposes these properties:
 * - ScanType: Galvo/Resonance selection
 * - FPS: Frame rate
 * - Resolution: Image size
 * - LaserPower_X: Power for laser X
 * - LaserEnable_X: Enable for laser X
 *
 * @see MicroscopeController
 * @see IHardwareFactory
 */

// ============================================================================
// Forward Declarations
// ============================================================================

/**
 * @class HelabMicroscopeHub
 * @brief Micro-Manager Hub device for Helab microscope
 *
 * The hub device is the entry point for all Helab microscope devices.
 * It manages the MicroscopeController and provides access to child devices.
 *
 * @section lifecycle Lifecycle
 *
 * 1. Micro-Manager creates hub instance
 * 2. Initialize() creates MicroscopeController
 * 3. DetectInstalledDevices() discovers child devices
 * 4. Child devices initialize and get controller from hub
 *
 * @section example Configuration
 *
 * In Micro-Manager configuration:
 * @code
 * Device,Hub,HelabMicroscopeHub,Helab Hub
 * Device,Camera,HelabCamera,Helab Camera
 * Device,Laser1,HelabLaserDevice,920nm Laser
 * @endcode
 */
class HelabMicroscopeHub : public HubBase<HelabMicroscopeHub> {
public:
    /**
     * @brief Constructor
     *
     * Creates an uninitialized hub instance.
     */
    HelabMicroscopeHub();

    /**
     * @brief Destructor
     *
     * Shuts down controller if initialized.
     */
    ~HelabMicroscopeHub() override;

    /// @name MMDevice Interface
    /// @{

    /**
     * @brief Initialize the hub
     * @return DEVICE_OK on success, error code otherwise
     *
     * Creates the MicroscopeController and initializes hardware.
     */
    int Initialize() override;

    /**
     * @brief Shutdown the hub
     * @return DEVICE_OK on success
     *
     * Stops imaging and releases hardware resources.
     */
    int Shutdown() override;

    /**
     * @brief Get device name
     * @param name Buffer to receive name
     */
    void GetName(char* name) const override;

    /**
     * @brief Check if device is busy
     * @return true if imaging or initializing
     */
    bool Busy() override;

    /// @}

    /// @name Hub Interface
    /// @{

    /**
     * @brief Detect installed child devices
     * @return DEVICE_OK on success
     *
     * Creates child device instances:
     * - HelabCamera: Main imaging device
     * - HelabLaserDevice: One per configured laser
     */
    int DetectInstalledDevices() override;

    /// @}

    /**
     * @brief Get controller instance
     * @return Pointer to MicroscopeController or nullptr if not initialized
     *
     * Used by child devices to access hardware functionality.
     */
    helab::MicroscopeController* getController();

private:
    std::unique_ptr<helab::MicroscopeController> controller_;
    bool initialized_ = false;
};

/**
 * @class HelabCamera
 * @brief Micro-Manager Camera device for two-photon imaging
 *
 * Implements the Micro-Manager camera interface for the Helab two-photon
 * microscope. Supports:
 *
 * - Single frame acquisition (SnapImage)
 * - Continuous acquisition (SequenceAcquisition)
 * - ROI and binning
 * - Property-based configuration
 *
 * @section properties Properties
 *
 * | Property | Type | Values | Description |
 * |----------|------|--------|-------------|
 * | ScanType | String | Galvo, Resonance | Scan mode |
 * | FPS | Float | 0.1-10.0 | Frame rate |
 * | Resolution | Integer | 256, 512, 1024 | Image size |
 * | LaserPower_920nm | Float | 0-2000 | 920nm laser power (mW) |
 * | LaserEnable_920nm | String | On, Off | 920nm laser state |
 *
 * @section example Usage
 *
 * @code
 * // In Micro-Manager
 * // 1. Add HelabMicroscopeHub
 * // 2. Add HelabCamera as child
 * // 3. Set properties
 * core.setProperty("HelabCamera", "FPS", 1.5);
 * core.setProperty("HelabCamera", "Resolution", 512);
 * // 4. Snap image
 * core.snapImage();
 * Image image = core.getImage();
 * @endcode
 */
class HelabCamera : public CCameraBase<HelabCamera> {
public:
    /**
     * @brief Constructor
     */
    HelabCamera();

    /**
     * @brief Destructor
     */
    ~HelabCamera() override;

    /// @name MMDevice Interface
    /// @{

    int Initialize() override;
    int Shutdown() override;
    void GetName(char* name) const override;
    bool Busy() override;

    /// @}

    /// @name Camera Interface
    /// @{

    /**
     * @brief Acquire single image
     * @return DEVICE_OK on success
     *
     * Triggers single frame acquisition and waits for completion.
     */
    int SnapImage() override;

    /**
     * @brief Get image buffer
     * @return Pointer to image data
     *
     * Returns the most recently acquired image.
     */
    const unsigned char* GetImageBuffer() override;

    /**
     * @brief Get image width
     * @return Width in pixels
     */
    unsigned GetImageWidth() const override;

    /**
     * @brief Get image height
     * @return Height in pixels
     */
    unsigned GetImageHeight() const override;

    /**
     * @brief Get bytes per pixel
     * @return 1 for 8-bit, 2 for 16-bit
     */
    unsigned GetImageBytesPerPixel() const override;

    /**
     * @brief Get bit depth
     * @return 8 or 16
     */
    unsigned GetBitDepth() const override;

    /**
     * @brief Get binning factor
     * @return Current binning (1, 2, or 4)
     */
    int GetBinning() const override;

    /**
     * @brief Set binning factor
     * @param binSize Binning factor
     * @return DEVICE_OK on success
     */
    int SetBinning(int binSize) override;

    /**
     * @brief Clear ROI (use full frame)
     * @return DEVICE_OK on success
     */
    void ClearROI() override;

    /**
     * @brief Set region of interest
     * @param x X offset
     * @param y Y offset
     * @param xSize Width
     * @param ySize Height
     * @return DEVICE_OK on success
     */
    int SetROI(unsigned x, unsigned y, unsigned xSize, unsigned ySize) override;

    /**
     * @brief Get current ROI
     * @param x X offset output
     * @param y Y offset output
     * @param xSize Width output
     * @param ySize Height output
     * @return DEVICE_OK on success
     */
    int GetROI(unsigned& x, unsigned& y, unsigned& xSize, unsigned& ySize) override;

    /// @}

    /// @name Sequence Acquisition
    /// @{

    /**
     * @brief Start sequence acquisition
     * @param numImages Number of images to acquire
     * @param interval_ms Interval between images
     * @param stopOnOverflow Stop if buffer full
     * @return DEVICE_OK on success
     */
    int StartSequenceAcquisition(long numImages, double interval_ms, bool stopOnOverflow) override;

    /**
     * @brief Start continuous acquisition
     * @param interval_ms Interval between images
     * @return DEVICE_OK on success
     */
    int StartSequenceAcquisition(double interval_ms) override;

    /**
     * @brief Stop sequence acquisition
     * @return DEVICE_OK on success
     */
    int StopSequenceAcquisition() override;

    /**
     * @brief Insert image into circular buffer
     * @return DEVICE_OK on success
     */
    int InsertImage() override;

    /// @}

    /// @name Exposure Control
    /// @{

    void SetExposure(double exp_ms) override;
    double GetExposure() const override;

    /// @}

    /// @name Property Callbacks
    /// @{

    /**
     * @brief Scan type property callback
     * @param pProp Property base
     * @param eAct Action type
     * @return DEVICE_OK on success
     */
    int OnScanType(MM::PropertyBase* pProp, MM::ActionType eAct);

    /**
     * @brief FPS property callback
     */
    int OnFps(MM::PropertyBase* pProp, MM::ActionType eAct);

    /**
     * @brief Resolution property callback
     */
    int OnResolution(MM::PropertyBase* pProp, MM::ActionType eAct);

    /**
     * @brief Laser power property callback
     */
    int OnLaserPower(MM::PropertyBase* pProp, MM::ActionType eAct);

    /**
     * @brief Laser enable property callback
     */
    int OnLaserEnable(MM::PropertyBase* pProp, MM::ActionType eAct);

    /// @}

private:
    HelabMicroscopeHub* hub_ = nullptr;
    helab::MicroscopeController* controller_ = nullptr;

    std::vector<uint8_t> imageBuffer_;
    std::atomic<bool> acquiring_{false};
    std::atomic<bool> busy_{false};

    double exposure_ms_ = 10.0;
    int binning_ = 1;

    // Image dimensions
    unsigned width_ = 512;
    unsigned height_ = 512;

    // Initialize properties
    int initializeProperties();

    // Handle image callback
    void handleImage(const std::vector<uint8_t>& image, uint64_t timestamp);
};

/**
 * @class HelabLaserDevice
 * @brief Micro-Manager Generic device for laser control
 *
 * Provides laser power and enable control through Micro-Manager properties.
 * Each laser wavelength gets its own device instance.
 *
 * @section properties Properties
 *
 * | Property | Type | Values | Description |
 * |----------|------|--------|-------------|
 * | Power | Float | 0-maxPower | Output power (mW) |
 * | Enable | String | On, Off | Output enable state |
 *
 * @section example Configuration
 *
 * @code
 * // In configuration file
 * Device,Laser920,HelabLaserDevice,920nm Laser
 * Property,Laser920,Name,920nm
 *
 * // In Micro-Manager
 * core.setProperty("Laser920", "Power", 500.0);
 * core.setProperty("Laser920", "Enable", "On");
 * @endcode
 */
class HelabLaserDevice : public CGenericBase<HelabLaserDevice> {
public:
    /**
     * @brief Constructor
     * @param laserName Name of laser to control (e.g., "920nm")
     */
    explicit HelabLaserDevice(const std::string& laserName = "");

    /**
     * @brief Destructor
     */
    ~HelabLaserDevice() override;

    /// @name MMDevice Interface
    /// @{

    int Initialize() override;
    int Shutdown() override;
    void GetName(char* name) const override;
    bool Busy() override;

    /// @}

    /// @name Property Callbacks
    /// @{

    /**
     * @brief Power property callback
     * @param pProp Property base
     * @param eAct Action type
     * @return DEVICE_OK on success
     */
    int OnPower(MM::PropertyBase* pProp, MM::ActionType eAct);

    /**
     * @brief Enable property callback
     * @param pProp Property base
     * @param eAct Action type
     * @return DEVICE_OK on success
     */
    int OnEnable(MM::PropertyBase* pProp, MM::ActionType eAct);

    /// @}

private:
    HelabMicroscopeHub* hub_ = nullptr;
    helab::MicroscopeController* controller_ = nullptr;
    std::string laserName_;
    bool initialized_ = false;

    int initializeProperties();
};
