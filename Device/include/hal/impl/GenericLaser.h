#pragma once

#include "../IHardware.h"
#include <string>
#include <atomic>

namespace helab {

/// Generic laser implementation
/// Can be used as a base for various laser controllers
/// Supports serial/USB communication for power control
class GenericLaser : public ILaser {
public:
    explicit GenericLaser(const LaserConfig& config);
    ~GenericLaser() override;

    // ILaser interface
    Result<void> initialize() override;
    Result<void> setPower(double power_mW) override;
    Result<void> enable(bool enable) override;
    Result<double> getPower() const override;
    Result<double> getWavelength() const override;
    bool isEnabled() const override;

    // Configuration
    void setPowerRange(double minPower_mW, double maxPower_mW);
    void setCommunicationPort(const std::string& port);

protected:
    // Protected for testing/mocking
    virtual Result<void> sendCommand(const std::string& command);
    virtual Result<std::string> readResponse();

private:
    LaserConfig config_;
    std::atomic<bool> initialized_{false};
    std::atomic<bool> enabled_{false};
    std::atomic<double> currentPower_{0.0};
    double minPower_ = 0.0;
    double maxPower_ = 1000.0;
    std::string commPort_;

    // Communication handle (platform-specific)
    void* commHandle_ = nullptr;

    Result<void> openCommunication();
    void closeCommunication();
};

} // namespace helab
