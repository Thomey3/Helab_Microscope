#include "mmadapter/HelabDevices.h"
#include "../hal/HardwareFactory.h"
#include <algorithm>

// ============================================================================
// HelabMicroscopeHub Implementation
// ============================================================================

HelabMicroscopeHub::HelabMicroscopeHub() {
    CreateHubID();
    SetErrorText(DEVICE_OK, "No error");
}

HelabMicroscopeHub::~HelabMicroscopeHub() {
    Shutdown();
}

int HelabMicroscopeHub::Initialize() {
    if (initialized_) {
        return DEVICE_OK;
    }

    // Create hardware factory
    auto factory = std::make_shared<helab::HardwareFactory>();

    // Create controller
    controller_ = std::make_unique<helab::MicroscopeController>(factory);

    // Initialize
    auto result = controller_->initialize();
    if (!result) {
        SetErrorText(result.error().code, result.error().message.c_str());
        return result.error().code;
    }

    initialized_ = true;
    return DEVICE_OK;
}

int HelabMicroscopeHub::Shutdown() {
    if (controller_) {
        controller_->shutdown();
        controller_.reset();
    }
    initialized_ = false;
    return DEVICE_OK;
}

void HelabMicroscopeHub::GetName(char* name) const {
    CDeviceUtils::CopyLimitedString(name, "HelabMicroscopeHub");
}

bool HelabMicroscopeHub::Busy() {
    return false;
}

int HelabMicroscopeHub::DetectInstalledDevices() {
    ClearInstalledDevices();

    // Register camera device
    AddInstalledDevice(new HelabCamera());

    // Register laser devices (if any)
    // These will be added dynamically based on configuration

    return DEVICE_OK;
}

helab::MicroscopeController* HelabMicroscopeHub::getController() {
    return controller_.get();
}

// ============================================================================
// HelabCamera Implementation
// ============================================================================

HelabCamera::HelabCamera() {
    SetErrorText(DEVICE_OK, "No error");

    // Initialize image buffer
    imageBuffer_.resize(width_ * height_ * 2);  // 16-bit images
}

HelabCamera::~HelabCamera() {
    Shutdown();
}

int HelabCamera::Initialize() {
    if (controller_ != nullptr) {
        return DEVICE_OK;
    }

    // Get hub
    hub_ = static_cast<HelabMicroscopeHub*>(GetParentHub());
    if (hub_ == nullptr) {
        return DEVICE_ERR;
    }

    controller_ = hub_->getController();
    if (controller_ == nullptr) {
        return DEVICE_ERR;
    }

    // Initialize properties
    int ret = initializeProperties();
    if (ret != DEVICE_OK) {
        return ret;
    }

    return DEVICE_OK;
}

int HelabCamera::Shutdown() {
    if (acquiring_) {
        StopSequenceAcquisition();
    }
    controller_ = nullptr;
    hub_ = nullptr;
    return DEVICE_OK;
}

void HelabCamera::GetName(char* name) const {
    CDeviceUtils::CopyLimitedString(name, "HelabCamera");
}

bool HelabCamera::Busy() {
    return busy_;
}

int HelabCamera::SnapImage() {
    if (controller_ == nullptr) {
        return DEVICE_ERR;
    }

    busy_ = true;

    // Acquire single frame
    auto result = controller_->acquireSingleFrame();
    if (!result) {
        busy_ = false;
        return result.error().code;
    }

    // Copy to image buffer
    const auto& frame = *result;
    size_t copySize = std::min(frame.size(), imageBuffer_.size());
    std::copy(frame.begin(), frame.begin() + copySize, imageBuffer_.begin());

    busy_ = false;
    return DEVICE_OK;
}

const unsigned char* HelabCamera::GetImageBuffer() {
    return imageBuffer_.data();
}

unsigned HelabCamera::GetImageWidth() const {
    return width_;
}

unsigned HelabCamera::GetImageHeight() const {
    return height_;
}

unsigned HelabCamera::GetImageBytesPerPixel() const {
    return 2;  // 16-bit images
}

unsigned HelabCamera::GetBitDepth() const {
    return 14;  // 14-bit ADC
}

int HelabCamera::GetBinning() const {
    return binning_;
}

int HelabCamera::SetBinning(int binSize) {
    if (binSize < 1 || binSize > 4) {
        return DEVICE_INVALID_INPUT_PARAM;
    }
    binning_ = binSize;
    return DEVICE_OK;
}

void HelabCamera::ClearROI() {
    if (controller_) {
        helab::ROI roi{0, 0, width_, height_};
        controller_->setROI(roi);
    }
}

int HelabCamera::SetROI(unsigned x, unsigned y, unsigned xSize, unsigned ySize) {
    if (controller_ == nullptr) {
        return DEVICE_ERR;
    }

    helab::ROI roi{x, y, xSize, ySize};
    auto result = controller_->setROI(roi);
    if (!result) {
        return result.error().code;
    }

    width_ = xSize;
    height_ = ySize;
    imageBuffer_.resize(width_ * height_ * 2);

    return DEVICE_OK;
}

int HelabCamera::GetROI(unsigned& x, unsigned& y, unsigned& xSize, unsigned& ySize) {
    if (controller_ == nullptr) {
        return DEVICE_ERR;
    }

    const auto& roi = controller_->roi();
    x = roi.x;
    y = roi.y;
    xSize = roi.width;
    ySize = roi.height;

    return DEVICE_OK;
}

int HelabCamera::StartSequenceAcquisition(long numImages, double interval_ms, bool stopOnOverflow) {
    if (controller_ == nullptr) {
        return DEVICE_ERR;
    }

    if (acquiring_) {
        return DEVICE_CAMERA_BUSY_ACQUIRING;
    }

    // Set image callback
    controller_->setImageCallback([this](const std::vector<uint8_t>& image, uint64_t timestamp) {
        handleImage(image, timestamp);
    });

    // Start imaging
    auto result = controller_->startImaging();
    if (!result) {
        return result.error().code;
    }

    acquiring_ = true;
    return DEVICE_OK;
}

int HelabCamera::StartSequenceAcquisition(double interval_ms) {
    return StartSequenceAcquisition(0, interval_ms, false);
}

int HelabCamera::StopSequenceAcquisition() {
    if (!acquiring_) {
        return DEVICE_OK;
    }

    if (controller_) {
        controller_->stopImaging();
    }

    acquiring_ = false;
    return DEVICE_OK;
}

int HelabCamera::InsertImage() {
    return DEVICE_OK;
}

void HelabCamera::SetExposure(double exp_ms) {
    exposure_ms_ = exp_ms;
}

double HelabCamera::GetExposure() const {
    return exposure_ms_;
}

int HelabCamera::OnScanType(MM::PropertyBase* pProp, MM::ActionType eAct) {
    if (eAct == MM::BeforeGet) {
        std::string typeStr;
        switch (controller_->scanType()) {
            case helab::ScanPattern::Type::Galvo:
                typeStr = "Galvo";
                break;
            case helab::ScanPattern::Type::Resonance:
                typeStr = "Resonance";
                break;
            default:
                typeStr = "Galvo";
        }
        pProp->Set(typeStr.c_str());
    } else if (eAct == MM::AfterSet) {
        std::string typeStr;
        pProp->Get(typeStr);

        helab::ScanPattern::Type type = helab::ScanPattern::Type::Galvo;
        if (typeStr == "Resonance") {
            type = helab::ScanPattern::Type::Resonance;
        }

        auto result = controller_->setScanType(type);
        if (!result) {
            return result.error().code;
        }
    }
    return DEVICE_OK;
}

int HelabCamera::OnFps(MM::PropertyBase* pProp, MM::ActionType eAct) {
    if (eAct == MM::BeforeGet) {
        pProp->Set(controller_->imagingConfig().fps);
    } else if (eAct == MM::AfterSet) {
        double fps;
        pProp->Get(fps);

        auto config = controller_->imagingConfig();
        config.fps = fps;
        auto result = controller_->setImagingConfig(config);
        if (!result) {
            return result.error().code;
        }
    }
    return DEVICE_OK;
}

int HelabCamera::OnResolution(MM::PropertyBase* pProp, MM::ActionType eAct) {
    if (eAct == MM::BeforeGet) {
        std::string res = std::to_string(width_) + "x" + std::to_string(height_);
        pProp->Set(res.c_str());
    } else if (eAct == MM::AfterSet) {
        std::string res;
        pProp->Get(res);

        // Parse resolution string
        size_t pos = res.find('x');
        if (pos != std::string::npos) {
            unsigned w = std::stoul(res.substr(0, pos));
            unsigned h = std::stoul(res.substr(pos + 1));

            auto config = controller_->imagingConfig();
            config.width = w;
            config.height = h;
            auto result = controller_->setImagingConfig(config);
            if (!result) {
                return result.error().code;
            }

            width_ = w;
            height_ = h;
            imageBuffer_.resize(width_ * height_ * 2);
        }
    }
    return DEVICE_OK;
}

int HelabCamera::OnLaserPower(MM::PropertyBase* pProp, MM::ActionType eAct) {
    // Placeholder for laser power control
    return DEVICE_OK;
}

int HelabCamera::OnLaserEnable(MM::PropertyBase* pProp, MM::ActionType eAct) {
    // Placeholder for laser enable control
    return DEVICE_OK;
}

int HelabCamera::initializeProperties() {
    // Scan type property
    CPropertyAction* pAct = new CPropertyAction(this, &HelabCamera::OnScanType);
    int ret = CreateProperty("ScanType", "Galvo", MM::String, false, pAct);
    if (ret != DEVICE_OK) return ret;

    AddAllowedValue("ScanType", "Galvo");
    AddAllowedValue("ScanType", "Resonance");

    // FPS property
    pAct = new CPropertyAction(this, &HelabCamera::OnFps);
    ret = CreateProperty("FPS", "1.5", MM::Float, false, pAct);
    if (ret != DEVICE_OK) return ret;

    SetPropertyLimits("FPS", 0.1, 10.0);

    // Resolution property
    pAct = new CPropertyAction(this, &HelabCamera::OnResolution);
    ret = CreateProperty("Resolution", "512x512", MM::String, false, pAct);
    if (ret != DEVICE_OK) return ret;

    AddAllowedValue("Resolution", "256x256");
    AddAllowedValue("Resolution", "512x512");
    AddAllowedValue("Resolution", "1024x1024");

    return DEVICE_OK;
}

void HelabCamera::handleImage(const std::vector<uint8_t>& image, uint64_t timestamp) {
    // Copy image to buffer and notify MM
    size_t copySize = std::min(image.size(), imageBuffer_.size());
    std::copy(image.begin(), image.begin() + copySize, imageBuffer_.begin());

    // Insert into MM core
    InsertImage();
}

// ============================================================================
// HelabLaserDevice Implementation
// ============================================================================

HelabLaserDevice::HelabLaserDevice(const std::string& laserName)
    : laserName_(laserName) {
}

HelabLaserDevice::~HelabLaserDevice() {
    Shutdown();
}

int HelabLaserDevice::Initialize() {
    if (initialized_) {
        return DEVICE_OK;
    }

    hub_ = static_cast<HelabMicroscopeHub*>(GetParentHub());
    if (hub_ == nullptr) {
        return DEVICE_ERR;
    }

    controller_ = hub_->getController();
    if (controller_ == nullptr) {
        return DEVICE_ERR;
    }

    int ret = initializeProperties();
    if (ret != DEVICE_OK) {
        return ret;
    }

    initialized_ = true;
    return DEVICE_OK;
}

int HelabLaserDevice::Shutdown() {
    initialized_ = false;
    controller_ = nullptr;
    hub_ = nullptr;
    return DEVICE_OK;
}

void HelabLaserDevice::GetName(char* name) const {
    std::string fullName = "HelabLaser_" + laserName_;
    CDeviceUtils::CopyLimitedString(name, fullName.c_str());
}

bool HelabLaserDevice::Busy() {
    return false;
}

int HelabLaserDevice::OnPower(MM::PropertyBase* pProp, MM::ActionType eAct) {
    if (eAct == MM::BeforeGet) {
        // Get current power
        // auto result = controller_->getLaserPower(laserName_);
        pProp->Set(0.0);
    } else if (eAct == MM::AfterSet) {
        double power;
        pProp->Get(power);

        auto result = controller_->setLaserPower(laserName_, power);
        if (!result) {
            return result.error().code;
        }
    }
    return DEVICE_OK;
}

int HelabLaserDevice::OnEnable(MM::PropertyBase* pProp, MM::ActionType eAct) {
    if (eAct == MM::BeforeGet) {
        pProp->Set(0L);
    } else if (eAct == MM::AfterSet) {
        long enable;
        pProp->Get(enable);

        auto result = controller_->enableLaser(laserName_, enable != 0);
        if (!result) {
            return result.error().code;
        }
    }
    return DEVICE_OK;
}

int HelabLaserDevice::initializeProperties() {
    // Power property
    CPropertyAction* pAct = new CPropertyAction(this, &HelabLaserDevice::OnPower);
    int ret = CreateProperty("Power_mW", "0.0", MM::Float, false, pAct);
    if (ret != DEVICE_OK) return ret;

    SetPropertyLimits("Power_mW", 0.0, 1000.0);

    // Enable property
    pAct = new CPropertyAction(this, &HelabLaserDevice::OnEnable);
    ret = CreateProperty("Enable", "0", MM::Integer, false, pAct);
    if (ret != DEVICE_OK) return ret;

    AddAllowedValue("Enable", "0");
    AddAllowedValue("Enable", "1");

    return DEVICE_OK;
}
