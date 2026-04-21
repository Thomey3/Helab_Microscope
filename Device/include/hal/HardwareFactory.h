#pragma once

#include "../IHardware.h"
#include "impl/NidaqClock.h"
#include "impl/GalvoScanner.h"
#include "impl/QtDetector.h"
#include "impl/GenericLaser.h"
#include <memory>
#include <string>
#include <unordered_map>

namespace helab {

/// Default hardware factory implementation
class HardwareFactory : public IHardwareFactory {
public:
    HardwareFactory() = default;
    ~HardwareFactory() override = default;

    /// Create clock generator
    std::unique_ptr<IClock> createClock(const std::string& type) override {
        if (type == "nidaq" || type.empty()) {
            return std::make_unique<NidaqClock>("Dev1");
        }
        return nullptr;
    }

    /// Create scanner
    std::unique_ptr<IScanner> createScanner(const std::string& type) override {
        if (type == "galvo" || type.empty()) {
            return std::make_unique<GalvoScanner>("Dev1");
        }
        // Future: add Resonance scanner support
        // if (type == "resonance") {
        //     return std::make_unique<ResonanceScanner>("Dev1");
        // }
        return nullptr;
    }

    /// Create detector
    std::unique_ptr<IDetector> createDetector(const std::string& type) override {
        if (type == "qt" || type.empty()) {
            return std::make_unique<QtDetector>();
        }
        return nullptr;
    }

    /// Create laser
    std::unique_ptr<ILaser> createLaser(const std::string& type) override {
        if (type == "generic" || type.empty()) {
            LaserConfig config;
            config.name = "Laser";
            config.wavelength = 920.0;
            config.maxPower = 1000.0;
            return std::make_unique<GenericLaser>(config);
        }
        // Specific laser types
        if (type == "920nm") {
            LaserConfig config;
            config.name = "920nm";
            config.wavelength = 920.0;
            config.maxPower = 2000.0;
            return std::make_unique<GenericLaser>(config);
        }
        if (type == "1064nm") {
            LaserConfig config;
            config.name = "1064nm";
            config.wavelength = 1064.0;
            config.maxPower = 1000.0;
            return std::make_unique<GenericLaser>(config);
        }
        if (type == "808nm") {
            LaserConfig config;
            config.name = "808nm";
            config.wavelength = 808.0;
            config.maxPower = 500.0;
            return std::make_unique<GenericLaser>(config);
        }
        return nullptr;
    }

    /// Create adaptive optics (placeholder for future implementation)
    std::unique_ptr<IAdaptiveOptics> createAdaptiveOptics(const std::string& type) override {
        // Future: implement adaptive optics drivers
        return nullptr;
    }
};

/// Factory with custom device names
class ConfigurableHardwareFactory : public IHardwareFactory {
public:
    explicit ConfigurableHardwareFactory(
        const std::string& nidaqDevice = "Dev1",
        const std::string& qtDevice = "0"
    ) : nidaqDevice_(nidaqDevice), qtDevice_(qtDevice) {}

    std::unique_ptr<IClock> createClock(const std::string& type) override {
        if (type == "nidaq" || type.empty()) {
            return std::make_unique<NidaqClock>(nidaqDevice_);
        }
        return nullptr;
    }

    std::unique_ptr<IScanner> createScanner(const std::string& type) override {
        if (type == "galvo" || type.empty()) {
            return std::make_unique<GalvoScanner>(nidaqDevice_);
        }
        return nullptr;
    }

    std::unique_ptr<IDetector> createDetector(const std::string& type) override {
        if (type == "qt" || type.empty()) {
            return std::make_unique<QtDetector>();
        }
        return nullptr;
    }

    std::unique_ptr<ILaser> createLaser(const std::string& type) override {
        if (type == "generic" || type.empty()) {
            LaserConfig config;
            config.name = "Laser";
            config.wavelength = 920.0;
            config.maxPower = 1000.0;
            return std::make_unique<GenericLaser>(config);
        }
        if (type == "920nm") {
            LaserConfig config;
            config.name = "920nm";
            config.wavelength = 920.0;
            config.maxPower = 2000.0;
            return std::make_unique<GenericLaser>(config);
        }
        if (type == "1064nm") {
            LaserConfig config;
            config.name = "1064nm";
            config.wavelength = 1064.0;
            config.maxPower = 1000.0;
            return std::make_unique<GenericLaser>(config);
        }
        if (type == "808nm") {
            LaserConfig config;
            config.name = "808nm";
            config.wavelength = 808.0;
            config.maxPower = 500.0;
            return std::make_unique<GenericLaser>(config);
        }
        return nullptr;
    }

    std::unique_ptr<IAdaptiveOptics> createAdaptiveOptics(const std::string& type) override {
        return nullptr;
    }

private:
    std::string nidaqDevice_;
    std::string qtDevice_;
};

} // namespace helab
