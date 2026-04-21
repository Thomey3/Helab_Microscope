#include "app/MicroscopeController.h"
#include <algorithm>

namespace helab {

MicroscopeController::MicroscopeController(std::shared_ptr<IHardwareFactory> factory)
    : factory_(std::move(factory)) {
    if (!factory_) {
        factory_ = std::make_shared<HardwareFactory>();
    }
    LOG_DEBUG("MicroscopeController created");
}

MicroscopeController::~MicroscopeController() {
    shutdown();
    LOG_DEBUG("MicroscopeController destroyed");
}

// ============================================================================
// Lifecycle
// ============================================================================

Result<void> MicroscopeController::initialize() {
    if (state_ != MicroscopeState::Uninitialized) {
        LOG_WARN("Microscope already initialized");
        return success();
    }

    LOG_INFO("Initializing microscope...");

    auto result = initializeHardware();
    if (!result) {
        state_ = MicroscopeState::Error;
        return result;
    }

    state_ = MicroscopeState::Ready;
    LOG_INFO("Microscope initialized successfully");
    return success();
}

Result<void> MicroscopeController::shutdown() {
    if (state_ == MicroscopeState::Uninitialized) {
        return success();
    }

    LOG_INFO("Shutting down microscope...");

    // Stop imaging if running
    if (imaging_) {
        stopImaging();
    }

    // Shutdown hardware
    if (scanner_) {
        scanner_->stop();
    }
    if (clock_) {
        clock_->stop();
    }
    if (detector_) {
        detector_->stopAcquisition();
    }

    // Disable lasers
    for (auto& [name, laser] : lasers_) {
        if (laser && laser->isEnabled()) {
            laser->enable(false);
        }
    }

    // Reset adaptive optics
    if (adaptiveOptics_) {
        adaptiveOptics_->reset();
    }

    state_ = MicroscopeState::Uninitialized;
    LOG_INFO("Microscope shutdown complete");
    return success();
}

bool MicroscopeController::isInitialized() const {
    return state_ != MicroscopeState::Uninitialized;
}

MicroscopeState MicroscopeController::state() const {
    return state_;
}

// ============================================================================
// Configuration
// ============================================================================

Result<void> MicroscopeController::setImagingConfig(const ImagingConfig& config) {
    if (imaging_) {
        return failure(HardwareError::alreadyRunning());
    }

    // Validate
    if (config.fps <= 0) {
        return failure(HardwareError::invalidParameter("fps"));
    }
    if (config.width == 0 || config.height == 0) {
        return failure(HardwareError::invalidParameter("width/height"));
    }

    imagingConfig_ = config;

    // Update hardware configurations if initialized
    if (isInitialized()) {
        auto result = configureClock();
        if (!result) return result;

        result = configureScanner();
        if (!result) return result;

        result = configureDetector();
        if (!result) return result;
    }

    LOG_INFO(std::format("Imaging config: {:.1f} fps, {}x{}, {} channels",
        imagingConfig_.fps, imagingConfig_.width, imagingConfig_.height, imagingConfig_.channels));
    return success();
}

const ImagingConfig& MicroscopeController::imagingConfig() const {
    return imagingConfig_;
}

Result<void> MicroscopeController::setROI(const ROI& roi) {
    if (imaging_) {
        return failure(HardwareError::alreadyRunning());
    }

    // Validate
    if (roi.width == 0 || roi.height == 0) {
        return failure(HardwareError::invalidParameter("ROI dimensions"));
    }

    roi_ = roi;
    LOG_INFO(std::format("ROI set: ({}, {}) {}x{}", roi_.x, roi_.y, roi_.width, roi_.height));
    return success();
}

const ROI& MicroscopeController::roi() const {
    return roi_;
}

Result<void> MicroscopeController::setScanType(ScanPattern::Type type) {
    if (imaging_) {
        return failure(HardwareError::alreadyRunning());
    }

    scanPattern_.type = type;

    if (scanner_) {
        return scanner_->setScanPattern(scanPattern_);
    }

    return success();
}

ScanPattern::Type MicroscopeController::scanType() const {
    return scanPattern_.type;
}

// ============================================================================
// Imaging Control
// ============================================================================

Result<void> MicroscopeController::startImaging() {
    if (!isInitialized()) {
        return failure(HardwareError::notInitialized());
    }

    if (imaging_) {
        LOG_WARN("Already imaging");
        return success();
    }

    LOG_INFO("Starting imaging...");

    // Start clock
    auto result = clock_->start();
    if (!result) {
        LOG_ERROR("Failed to start clock");
        return result;
    }

    // Start scanner
    result = scanner_->start();
    if (!result) {
        clock_->stop();
        LOG_ERROR("Failed to start scanner");
        return result;
    }

    // Set detector callback
    detector_->setDataCallback([this](const void* data, size_t size, uint64_t timestamp) {
        handleDetectorData(data, size, timestamp);
    });

    // Start detector
    result = detector_->startAcquisition();
    if (!result) {
        scanner_->stop();
        clock_->stop();
        LOG_ERROR("Failed to start detector");
        return result;
    }

    imaging_ = true;
    state_ = MicroscopeState::Imaging;
    LOG_INFO("Imaging started");
    return success();
}

Result<void> MicroscopeController::stopImaging() {
    if (!imaging_) {
        return success();
    }

    LOG_INFO("Stopping imaging...");

    // Stop detector first
    detector_->stopAcquisition();

    // Stop scanner
    scanner_->stop();

    // Stop clock
    clock_->stop();

    imaging_ = false;
    state_ = MicroscopeState::Ready;
    LOG_INFO("Imaging stopped");
    return success();
}

Result<std::vector<uint8_t>> MicroscopeController::acquireSingleFrame() {
    if (!isInitialized()) {
        return failure(HardwareError::notInitialized());
    }

    if (imaging_) {
        return failure(HardwareError::alreadyRunning());
    }

    // Start hardware for single frame
    auto result = clock_->start();
    if (!result) return result;

    result = scanner_->start();
    if (!result) {
        clock_->stop();
        return result;
    }

    result = detector_->startAcquisition();
    if (!result) {
        scanner_->stop();
        clock_->stop();
        return result;
    }

    // Wait for data
    auto bufferResult = detector_->getDataBuffer();
    if (!bufferResult) {
        stopImaging();
        return failure(bufferResult.error());
    }

    // Get data size
    auto sizeResult = detector_->getAvailableDataSize();
    if (!sizeResult) {
        stopImaging();
        return failure(sizeResult.error());
    }

    // Copy data
    void* buffer = *bufferResult;
    size_t size = *sizeResult;
    std::vector<uint8_t> frame(static_cast<uint8_t*>(buffer),
                               static_cast<uint8_t*>(buffer) + size);

    // Stop hardware
    stopImaging();

    return frame;
}

bool MicroscopeController::isImaging() const {
    return imaging_;
}

void MicroscopeController::setImageCallback(ImageCallback callback) {
    std::lock_guard<std::mutex> lock(mutex_);
    imageCallback_ = std::move(callback);
}

// ============================================================================
// Laser Control
// ============================================================================

Result<void> MicroscopeController::addLaser(std::unique_ptr<ILaser> laser, const std::string& name) {
    if (!laser) {
        return failure(HardwareError::invalidParameter("laser"));
    }

    auto result = laser->initialize();
    if (!result) {
        return result;
    }

    lasers_[name] = std::move(laser);
    LOG_INFO(std::format("Laser '{}' added", name));
    return success();
}

Result<void> MicroscopeController::addLaserByType(const std::string& type, const std::string& name) {
    auto laser = factory_->createLaser(type);
    if (!laser) {
        return failure(HardwareError::configuration(
            std::format("Failed to create laser of type '{}'", type)));
    }
    return addLaser(std::move(laser), name);
}

Result<void> MicroscopeController::removeLaser(const std::string& name) {
    auto it = lasers_.find(name);
    if (it == lasers_.end()) {
        return failure(HardwareError::invalidParameter("laser name"));
    }

    if (it->second && it->second->isEnabled()) {
        it->second->enable(false);
    }

    lasers_.erase(it);
    LOG_INFO(std::format("Laser '{}' removed", name));
    return success();
}

Result<void> MicroscopeController::setLaserPower(const std::string& name, double power_mW) {
    auto it = lasers_.find(name);
    if (it == lasers_.end()) {
        return failure(HardwareError::invalidParameter("laser name"));
    }

    return it->second->setPower(power_mW);
}

Result<void> MicroscopeController::enableLaser(const std::string& name, bool enable) {
    auto it = lasers_.find(name);
    if (it == lasers_.end()) {
        return failure(HardwareError::invalidParameter("laser name"));
    }

    return it->second->enable(enable);
}

std::vector<std::string> MicroscopeController::laserNames() const {
    std::vector<std::string> names;
    names.reserve(lasers_.size());
    for (const auto& [name, _] : lasers_) {
        names.push_back(name);
    }
    return names;
}

// ============================================================================
// Adaptive Optics
// ============================================================================

Result<void> MicroscopeController::addAdaptiveOptics(std::unique_ptr<IAdaptiveOptics> ao) {
    if (!ao) {
        return failure(HardwareError::invalidParameter("adaptive optics"));
    }

    auto result = ao->initialize();
    if (!result) {
        return result;
    }

    adaptiveOptics_ = std::move(ao);
    LOG_INFO("Adaptive optics added");
    return success();
}

Result<void> MicroscopeController::applyWavefrontCorrection(const std::vector<double>& wavefront) {
    if (!adaptiveOptics_) {
        return failure(HardwareError::notInitialized());
    }

    return adaptiveOptics_->applyCorrection(wavefront);
}

Result<void> MicroscopeController::resetAdaptiveOptics() {
    if (!adaptiveOptics_) {
        return failure(HardwareError::notInitialized());
    }

    return adaptiveOptics_->reset();
}

// ============================================================================
// Hardware Access
// ============================================================================

IClock* MicroscopeController::clock() const {
    return clock_.get();
}

IScanner* MicroscopeController::scanner() const {
    return scanner_.get();
}

IDetector* MicroscopeController::detector() const {
    return detector_.get();
}

// ============================================================================
// Internal Methods
// ============================================================================

Result<void> MicroscopeController::initializeHardware() {
    // Create hardware instances
    clock_ = factory_->createClock("nidaq");
    if (!clock_) {
        return failure(HardwareError::configuration("Failed to create clock"));
    }

    scanner_ = factory_->createScanner("galvo");
    if (!scanner_) {
        return failure(HardwareError::configuration("Failed to create scanner"));
    }

    detector_ = factory_->createDetector("qt");
    if (!detector_) {
        return failure(HardwareError::configuration("Failed to create detector"));
    }

    // Configure hardware
    auto result = configureClock();
    if (!result) return result;

    result = configureScanner();
    if (!result) return result;

    result = configureDetector();
    if (!result) return result;

    return success();
}

Result<void> MicroscopeController::configureClock() {
    ClockConfig config;
    config.deviceName = "Dev1";
    config.counterChannel = "ctr0";
    config.triggerChannel = "PFI0";
    config.pixelCount = imagingConfig_.width;
    config.totalPixels = imagingConfig_.width * imagingConfig_.height;
    config.dutyCycle = 0.5;
    config.lineRate = imagingConfig_.fps * imagingConfig_.height;

    return clock_->configure(config);
}

Result<void> MicroscopeController::configureScanner() {
    ScannerConfig config;
    config.deviceName = "Dev1";
    config.xChannel = "ao0";
    config.yChannel = "ao1";
    config.scanRange = imagingConfig_.scanRangeV;
    config.frequency = imagingConfig_.fps;
    config.samplesPerLine = imagingConfig_.width;

    auto result = scanner_->configure(config);
    if (!result) return result;

    return scanner_->setScanPattern(scanPattern_);
}

Result<void> MicroscopeController::configureDetector() {
    DetectorConfig config;
    config.channelMask = 0x0F;  // 4 channels
    config.sampleRate = 100'000'000;  // 100 MS/s
    config.segmentSize = imagingConfig_.width;
    config.numSegments = imagingConfig_.height;
    config.triggerMode = DetectorConfig::TriggerMode::External;

    return detector_->configure(config);
}

void MicroscopeController::handleDetectorData(const void* data, size_t size, uint64_t timestamp) {
    std::lock_guard<std::mutex> lock(mutex_);
    if (imageCallback_) {
        std::vector<uint8_t> frame(static_cast<const uint8_t*>(data),
                                   static_cast<const uint8_t*>(data) + size);
        imageCallback_(frame, timestamp);
    }
}

} // namespace helab
