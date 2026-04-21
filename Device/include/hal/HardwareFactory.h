#pragma once

#include "../IHardware.h"
#include "impl/NidaqClock.h"
#include "impl/GalvoScanner.h"
#include "impl/QtDetector.h"
#include "impl/GenericLaser.h"
#include <memory>
#include <string>
#include <unordered_map>

/**
 * @file HardwareFactory.h
 * @brief Hardware factory implementations for creating hardware instances
 *
 * This file provides factory classes that create hardware instances based on
 * the Abstract Factory pattern. The factory pattern enables:
 *
 * - **Dependency Injection**: Pass factories to constructors for testability
 * - **Configuration Flexibility**: Create different hardware based on config
 * - **Loose Coupling**: Client code depends on interfaces, not implementations
 *
 * @section factories Available Factories
 *
 * - HardwareFactory: Default factory with standard device names
 * - ConfigurableHardwareFactory: Factory with custom device names
 *
 * @section example Example Usage
 *
 * @code
 * // Using default factory
 * auto factory = std::make_shared<helab::HardwareFactory>();
 *
 * auto clock = factory->createClock("nidaq");
 * auto scanner = factory->createScanner("galvo");
 * auto detector = factory->createDetector("qt");
 * auto laser = factory->createLaser("920nm");
 *
 * // Using with MicroscopeController
 * auto controller = std::make_unique<helab::MicroscopeController>(factory);
 * controller->initialize();
 *
 * // Using configurable factory for custom device names
 * auto configFactory = std::make_shared<helab::ConfigurableHardwareFactory>(
 *     "Dev2",  // NI-DAQ device
 *     "1"      // QT card index
 * );
 * @endcode
 *
 * @see IHardwareFactory
 * @see MicroscopeController
 */

namespace helab {

/**
 * @class HardwareFactory
 * @brief Default hardware factory implementation
 *
 * Creates hardware instances with default device names:
 * - NI-DAQ device: "Dev1"
 * - QT card: index 0
 *
 * This factory is suitable for standard microscope configurations
 * with a single DAQ device and single QT card.
 *
 * @section supported Supported Hardware Types
 *
 * | Type String | Implementation | Description |
 * |-------------|----------------|-------------|
 * | "nidaq" | NidaqClock | NI-DAQmx clock generator |
 * | "galvo" | GalvoScanner | Galvanometer scanner |
 * | "qt" | QtDetector | QT12136DC digitizer |
 * | "generic" | GenericLaser | Generic laser controller |
 * | "920nm" | GenericLaser | 920nm laser (2W max) |
 * | "1064nm" | GenericLaser | 1064nm laser (1W max) |
 * | "808nm" | GenericLaser | 808nm laser (500mW max) |
 */
class HardwareFactory : public IHardwareFactory {
public:
    /**
     * @brief Default constructor
     *
     * Creates a factory with default device names.
     */
    HardwareFactory() = default;

    /**
     * @brief Destructor
     */
    ~HardwareFactory() override = default;

    /**
     * @brief Create clock generator
     * @param type Clock type ("nidaq" for NI-DAQmx, empty for default)
     * @return Unique pointer to IClock implementation
     *
     * Creates a clock generator for pixel/line timing.
     * Currently supports "nidaq" type using NI-DAQmx.
     */
    std::unique_ptr<IClock> createClock(const std::string& type) override {
        if (type == "nidaq" || type.empty()) {
            return std::make_unique<NidaqClock>("Dev1");
        }
        return nullptr;
    }

    /**
     * @brief Create scanner
     * @param type Scanner type ("galvo" for galvanometer, empty for default)
     * @return Unique pointer to IScanner implementation
     *
     * Creates a beam scanner for X-Y control.
     * Currently supports "galvo" type. Resonance scanner planned.
     */
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

    /**
     * @brief Create detector
     * @param type Detector type ("qt" for QT12136DC, empty for default)
     * @return Unique pointer to IDetector implementation
     *
     * Creates a data acquisition detector.
     * Currently supports "qt" type for QT12136DC card.
     */
    std::unique_ptr<IDetector> createDetector(const std::string& type) override {
        if (type == "qt" || type.empty()) {
            return std::make_unique<QtDetector>();
        }
        return nullptr;
    }

    /**
     * @brief Create laser
     * @param type Laser type ("generic", "920nm", "1064nm", "808nm")
     * @return Unique pointer to ILaser implementation
     *
     * Creates a laser controller with appropriate wavelength and power settings.
     */
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

    /**
     * @brief Create adaptive optics
     * @param type AO type (placeholder for future implementation)
     * @return nullptr (not yet implemented)
     *
     * @note Adaptive optics support is planned for future releases.
     */
    std::unique_ptr<IAdaptiveOptics> createAdaptiveOptics(const std::string& type) override {
        // Future: implement adaptive optics drivers
        return nullptr;
    }
};

/**
 * @class ConfigurableHardwareFactory
 * @brief Factory with custom device names
 *
 * Creates hardware instances with configurable device names.
 * Use this factory when your hardware uses non-default device identifiers.
 *
 * @section example Example
 * @code
 * // Create factory with custom device names
 * auto factory = std::make_shared<ConfigurableHardwareFactory>(
 *     "Dev2",  // NI-DAQ device is Dev2 instead of Dev1
 *     "1"      // QT card index 1 instead of 0
 * );
 *
 * auto clock = factory->createClock("nidaq");
 * // clock will use Dev2
 * @endcode
 */
class ConfigurableHardwareFactory : public IHardwareFactory {
public:
    /**
     * @brief Constructor with device names
     * @param nidaqDevice NI-DAQ device name (default: "Dev1")
     * @param qtDevice QT card index as string (default: "0")
     *
     * Creates a factory with the specified device names.
     */
    explicit ConfigurableHardwareFactory(
        const std::string& nidaqDevice = "Dev1",
        const std::string& qtDevice = "0"
    ) : nidaqDevice_(nidaqDevice), qtDevice_(qtDevice) {}

    /**
     * @brief Create clock generator
     * @param type Clock type
     * @return Unique pointer to IClock implementation
     */
    std::unique_ptr<IClock> createClock(const std::string& type) override {
        if (type == "nidaq" || type.empty()) {
            return std::make_unique<NidaqClock>(nidaqDevice_);
        }
        return nullptr;
    }

    /**
     * @brief Create scanner
     * @param type Scanner type
     * @return Unique pointer to IScanner implementation
     */
    std::unique_ptr<IScanner> createScanner(const std::string& type) override {
        if (type == "galvo" || type.empty()) {
            return std::make_unique<GalvoScanner>(nidaqDevice_);
        }
        return nullptr;
    }

    /**
     * @brief Create detector
     * @param type Detector type
     * @return Unique pointer to IDetector implementation
     */
    std::unique_ptr<IDetector> createDetector(const std::string& type) override {
        if (type == "qt" || type.empty()) {
            return std::make_unique<QtDetector>();
        }
        return nullptr;
    }

    /**
     * @brief Create laser
     * @param type Laser type
     * @return Unique pointer to ILaser implementation
     */
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

    /**
     * @brief Create adaptive optics
     * @param type AO type
     * @return nullptr (not yet implemented)
     */
    std::unique_ptr<IAdaptiveOptics> createAdaptiveOptics(const std::string& type) override {
        return nullptr;
    }

private:
    std::string nidaqDevice_;  ///< NI-DAQ device name
    std::string qtDevice_;     ///< QT card index
};

} // namespace helab
