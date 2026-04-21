#pragma once

#include "DeviceBase.h"
#include "../app/MicroscopeController.h"
#include <memory>
#include <string>

/// Helab Microscope Hub device
/// Entry point for all Helab microscope devices
class HelabMicroscopeHub : public HubBase<HelabMicroscopeHub> {
public:
    HelabMicroscopeHub();
    ~HelabMicroscopeHub() override;

    // MMDevice interface
    int Initialize() override;
    int Shutdown() override;
    void GetName(char* name) const override;
    bool Busy() override;

    // Hub interface
    int DetectInstalledDevices() override;

    // Access to controller (for child devices)
    helab::MicroscopeController* getController();

private:
    std::unique_ptr<helab::MicroscopeController> controller_;
    bool initialized_ = false;
};

/// Helab Camera device
/// Two-photon camera implementation
class HelabCamera : public CCameraBase<HelabCamera> {
public:
    HelabCamera();
    ~HelabCamera() override;

    // MMDevice interface
    int Initialize() override;
    int Shutdown() override;
    void GetName(char* name) const override;
    bool Busy() override;

    // Camera interface
    int SnapImage() override;
    const unsigned char* GetImageBuffer() override;
    unsigned GetImageWidth() const override;
    unsigned GetImageHeight() const override;
    unsigned GetImageBytesPerPixel() const override;
    unsigned GetBitDepth() const override;
    int GetBinning() const override;
    int SetBinning(int binSize) override;
    void ClearROI() override;
    int SetROI(unsigned x, unsigned y, unsigned xSize, unsigned ySize) override;
    int GetROI(unsigned& x, unsigned& y, unsigned& xSize, unsigned& ySize) override;

    // Sequence acquisition
    int StartSequenceAcquisition(long numImages, double interval_ms, bool stopOnOverflow) override;
    int StartSequenceAcquisition(double interval_ms) override;
    int StopSequenceAcquisition() override;
    int InsertImage() override;

    // Exposure
    void SetExposure(double exp_ms) override;
    double GetExposure() const override;

    // Property callbacks
    int OnScanType(MM::PropertyBase* pProp, MM::ActionType eAct);
    int OnFps(MM::PropertyBase* pProp, MM::ActionType eAct);
    int OnResolution(MM::PropertyBase* pProp, MM::ActionType eAct);
    int OnLaserPower(MM::PropertyBase* pProp, MM::ActionType eAct);
    int OnLaserEnable(MM::PropertyBase* pProp, MM::ActionType eAct);

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

/// Helab Laser device
/// Generic laser control device
class HelabLaserDevice : public CGenericBase<HelabLaserDevice> {
public:
    explicit HelabLaserDevice(const std::string& laserName = "");
    ~HelabLaserDevice() override;

    // MMDevice interface
    int Initialize() override;
    int Shutdown() override;
    void GetName(char* name) const override;
    bool Busy() override;

    // Property callbacks
    int OnPower(MM::PropertyBase* pProp, MM::ActionType eAct);
    int OnEnable(MM::PropertyBase* pProp, MM::ActionType eAct);

private:
    HelabMicroscopeHub* hub_ = nullptr;
    helab::MicroscopeController* controller_ = nullptr;
    std::string laserName_;
    bool initialized_ = false;

    int initializeProperties();
};
