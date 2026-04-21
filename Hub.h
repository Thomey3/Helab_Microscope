#pragma once

#include "DeviceBase.h"
#include "Device/include/mmadapter/HelabDevices.h"

extern const char* g_HubDeviceName;
extern const char* g_CameraDeviceName;

/// Legacy Hub class for backward compatibility
/// Delegates to new HelabMicroscopeHub architecture
class Hub : public HubBase<Hub> {
public:
    Hub();
    ~Hub();

    // Device API
    int Initialize();
    int Shutdown();
    void GetName(char* pName) const;
    bool Busy();

    // HUB API
    int DetectInstalledDevices();

private:
    std::unique_ptr<HelabMicroscopeHub> impl_;
    bool initialized_;
    bool busy_;
};
