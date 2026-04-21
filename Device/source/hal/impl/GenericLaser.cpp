#include "hal/impl/GenericLaser.h"
#include <algorithm>
#include <sstream>
#include <iomanip>

namespace helab {

GenericLaser::GenericLaser(const LaserConfig& config)
    : config_(config) {
    LOG_DEBUG(std::format("GenericLaser created: {} @ {}nm",
        config_.name, config_.wavelength));
}

GenericLaser::~GenericLaser() {
    if (enabled_) {
        enable(false);
    }
    closeCommunication();
    LOG_DEBUG(std::format("GenericLaser destroyed: {}", config_.name));
}

Result<void> GenericLaser::initialize() {
    if (initialized_) {
        LOG_WARN(std::format("Laser {} already initialized", config_.name));
        return success();
    }

    LOG_INFO(std::format("Initializing laser: {} @ {}nm, max power: {}mW",
        config_.name, config_.wavelength, config_.maxPower));

    auto result = openCommunication();
    if (!result) {
        return result;
    }

    // Query laser status
    auto responseResult = sendCommand("STATUS?");
    if (!responseResult) {
        closeCommunication();
        return responseResult;
    }

    initialized_ = true;
    LOG_INFO(std::format("Laser {} initialized successfully", config_.name));
    return success();
}

Result<void> GenericLaser::setPower(double power_mW) {
    if (!initialized_) {
        return failure(HardwareError::notInitialized());
    }

    // Clamp power to valid range
    power_mW = std::clamp(power_mW, minPower_, maxPower_);

    LOG_DEBUG(std::format("Setting laser {} power to {:.2f}mW", config_.name, power_mW));

    std::ostringstream cmd;
    cmd << "POWER " << std::fixed << std::setprecision(2) << power_mW;

    auto result = sendCommand(cmd.str());
    if (!result) {
        return result;
    }

    currentPower_ = power_mW;
    return success();
}

Result<void> GenericLaser::enable(bool enable) {
    if (!initialized_) {
        return failure(HardwareError::notInitialized());
    }

    LOG_INFO(std::format("{} laser {}", enable ? "Enabling" : "Disabling", config_.name));

    std::string cmd = enable ? "LASER ON" : "LASER OFF";
    auto result = sendCommand(cmd);
    if (!result) {
        return result;
    }

    enabled_ = enable;
    return success();
}

Result<double> GenericLaser::getPower() const {
    if (!initialized_) {
        return failure(HardwareError::notInitialized());
    }

    // Query current power
    // In a real implementation, this would query the hardware
    return currentPower_.load();
}

Result<double> GenericLaser::getWavelength() const {
    return config_.wavelength;
}

bool GenericLaser::isEnabled() const {
    return enabled_;
}

void GenericLaser::setPowerRange(double minPower_mW, double maxPower_mW) {
    minPower_ = minPower_mW;
    maxPower_ = std::min(maxPower_mW, config_.maxPower);
}

void GenericLaser::setCommunicationPort(const std::string& port) {
    commPort_ = port;
}

Result<void> GenericLaser::sendCommand(const std::string& command) {
    // Placeholder for actual communication
    // In a real implementation, this would send via serial/USB
    LOG_TRACE(std::format("Sending command to {}: {}", config_.name, command));
    return success();
}

Result<std::string> GenericLaser::readResponse() {
    // Placeholder for actual communication
    return std::string("OK");
}

Result<void> GenericLaser::openCommunication() {
    if (commPort_.empty()) {
        // No communication port configured - use simulation mode
        LOG_WARN(std::format("No comm port for laser {}, using simulation", config_.name));
        return success();
    }

    // Platform-specific communication opening
    // On Windows: CreateFile for COM port
    // On Linux: open() for /dev/ttyUSB*

    LOG_DEBUG(std::format("Opening communication on {}", commPort_));
    return success();
}

void GenericLaser::closeCommunication() {
    if (commHandle_) {
        // Platform-specific cleanup
        commHandle_ = nullptr;
    }
}

} // namespace helab
