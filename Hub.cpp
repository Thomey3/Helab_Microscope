#include "Hub.h"
#include <cstring>

// Device names for Micro-Manager registration
const char* g_HubDeviceName = "HelabMicroscopeHub";
const char* g_CameraDeviceName = "HelabCamera";

// ============================================================================
// Module Entry Points (Micro-Manager API)
// ============================================================================

MODULE_API void InitializeModuleData() {
    // Register the hub device
    RegisterDevice(g_HubDeviceName, MM::HubDevice,
        "Helab Two-Photon Microscope Hub");

    // Register the camera device
    RegisterDevice(g_CameraDeviceName, MM::CameraDevice,
        "Helab Two-Photon Camera");

    // Register laser devices (can be extended)
    RegisterDevice("HelabLaser_920nm", MM::GenericDevice,
        "920nm Laser Control");
    RegisterDevice("HelabLaser_1064nm", MM::GenericDevice,
        "1064nm Laser Control");
}

MODULE_API MM::Device* CreateDevice(const char* deviceName) {
    if (deviceName == nullptr) {
        return nullptr;
    }

    // Hub device
    if (std::strcmp(deviceName, g_HubDeviceName) == 0) {
        return new HelabMicroscopeHub;
    }

    // Camera device
    if (std::strcmp(deviceName, g_CameraDeviceName) == 0) {
        return new HelabCamera;
    }

    // Laser devices
    if (std::strcmp(deviceName, "HelabLaser_920nm") == 0) {
        return new HelabLaserDevice("920nm");
    }
    if (std::strcmp(deviceName, "HelabLaser_1064nm") == 0) {
        return new HelabLaserDevice("1064nm");
    }

    // Unknown device
    return nullptr;
}

MODULE_API void DeleteDevice(MM::Device* pDevice) {
    delete pDevice;
}

// ============================================================================
// Legacy Hub Implementation (delegates to new architecture)
// ============================================================================

Hub::Hub()
    : impl_(std::make_unique<HelabMicroscopeHub>())
    , initialized_(false)
    , busy_(false) {
}

Hub::~Hub() {
    Shutdown();
}

int Hub::Initialize() {
    if (initialized_) {
        return DEVICE_OK;
    }

    int ret = impl_->Initialize();
    if (ret == DEVICE_OK) {
        initialized_ = true;
    }
    return ret;
}

int Hub::Shutdown() {
    if (impl_) {
        impl_->Shutdown();
    }
    initialized_ = false;
    return DEVICE_OK;
}

void Hub::GetName(char* pName) const {
    CDeviceUtils::CopyLimitedString(pName, g_HubDeviceName);
}

bool Hub::Busy() {
    return impl_ ? impl_->Busy() : false;
}

int Hub::DetectInstalledDevices() {
    ClearInstalledDevices();

    // Register camera
    AddInstalledDevice(new HelabCamera());

    // Register lasers
    AddInstalledDevice(new HelabLaserDevice("920nm"));
    AddInstalledDevice(new HelabLaserDevice("1064nm"));

    return DEVICE_OK;
}
