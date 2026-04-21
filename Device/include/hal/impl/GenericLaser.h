#pragma once

#include "../IHardware.h"
#include <string>
#include <atomic>

/**
 * @file GenericLaser.h
 * @brief Generic laser implementation for serial/USB controlled lasers
 *
 * This file provides a generic laser implementation that can be adapted
 * to various laser controllers with serial or USB interfaces.
 *
 * @section hardware Hardware Requirements
 * - Laser with serial or USB control interface
 * - Appropriate communication driver
 *
 * @section protocol Communication Protocol
 *
 * The generic laser uses a simple text-based command protocol:
 * - Power setting: "POWER <value>" or similar
 * - Enable: "ON" or "ENABLE"
 * - Disable: "OFF" or "DISABLE"
 * - Status query: "STATUS?" or "POWER?"
 *
 * Subclasses can override sendCommand() and readResponse() for
 * custom protocols.
 *
 * @section example Example Usage
 * @code
 * // Create laser with configuration
 * helab::LaserConfig config;
 * config.name = "920nm";
 * config.wavelength = 920.0;
 * config.maxPower = 2000.0;  // 2W max
 * config.port = "COM3";
 *
 * helab::GenericLaser laser(config);
 *
 * // Initialize communication
 * auto result = laser.initialize();
 * if (!result) {
 *     std::cerr << "Failed to initialize: " << result.error().message << std::endl;
 *     return;
 * }
 *
 * // Set power and enable
 * laser.setPower(500.0);  // 500 mW
 * laser.enable(true);
 *
 * // ... imaging ...
 *
 * // Disable when done
 * laser.enable(false);
 * @endcode
 *
 * @see ILaser
 * @see LaserConfig
 */

namespace helab {

/**
 * @class GenericLaser
 * @brief Generic laser implementation
 *
 * Provides a base implementation for lasers with serial/USB control.
 * Can be used directly for simple protocols or subclassed for
 * manufacturer-specific implementations.
 *
 * @section features Features
 * - Configurable power range
 * - Enable/disable control
 * - Serial/USB communication
 * - Thread-safe operations
 *
 * @section extending Extending for Custom Lasers
 *
 * To support a specific laser model:
 * @code
 * class CoherentLaser : public GenericLaser {
 * protected:
 *     Result<void> sendCommand(const std::string& command) override {
 *         // Send command using Coherent protocol
 *     }
 *
 *     Result<std::string> readResponse() override {
 *         // Read response using Coherent protocol
 *     }
 * };
 * @endcode
 */
class GenericLaser : public ILaser {
public:
    /**
     * @brief Constructor with configuration
     * @param config Laser configuration parameters
     *
     * Creates a laser instance with the specified configuration.
     * The laser is not initialized until initialize() is called.
     */
    explicit GenericLaser(const LaserConfig& config);

    /**
     * @brief Destructor - ensures cleanup
     *
     * Closes communication port if open.
     */
    ~GenericLaser() override;

    /// @name ILaser Interface Implementation
    /// @{

    /**
     * @brief Initialize laser connection
     * @return Result<void> Success or error
     *
     * Opens the communication port and verifies laser connectivity.
     */
    Result<void> initialize() override;

    /**
     * @brief Set output power
     * @param power_mW Power in milliwatts
     * @return Result<void> Success or error
     *
     * Sets the laser output power. The value is clamped to the
     * valid range [minPower, maxPower].
     */
    Result<void> setPower(double power_mW) override;

    /**
     * @brief Enable/disable laser output
     * @param enable true to enable, false to disable
     * @return Result<void> Success or error
     *
     * Controls the laser emission state.
     * @warning Laser must be properly interlocked before enabling
     */
    Result<void> enable(bool enable) override;

    /**
     * @brief Get current power setting
     * @return Result<double> Power in mW or error
     */
    Result<double> getPower() const override;

    /**
     * @brief Get wavelength
     * @return Result<double> Wavelength in nm
     */
    Result<double> getWavelength() const override;

    /**
     * @brief Check if laser is enabled
     * @return true if enabled, false otherwise
     */
    bool isEnabled() const override;

    /// @}

    /// @name Configuration
    /// @{

    /**
     * @brief Set valid power range
     * @param minPower_mW Minimum power in mW
     * @param maxPower_mW Maximum power in mW
     */
    void setPowerRange(double minPower_mW, double maxPower_mW);

    /**
     * @brief Set communication port
     * @param port Port name (e.g., "COM3", "/dev/ttyUSB0")
     */
    void setCommunicationPort(const std::string& port);

    /// @}

protected:
    /**
     * @brief Send command to laser
     * @param command Command string
     * @return Result<void> Success or error
     *
     * Override this method to implement custom communication protocols.
     */
    virtual Result<void> sendCommand(const std::string& command);

    /**
     * @brief Read response from laser
     * @return Result<std::string> Response string or error
     *
     * Override this method to implement custom communication protocols.
     */
    virtual Result<std::string> readResponse();

private:
    LaserConfig config_;                ///< Laser configuration
    std::atomic<bool> initialized_{false}; ///< Initialization state
    std::atomic<bool> enabled_{false};  ///< Enable state
    std::atomic<double> currentPower_{0.0}; ///< Current power setting
    double minPower_ = 0.0;             ///< Minimum power (mW)
    double maxPower_ = 1000.0;          ///< Maximum power (mW)
    std::string commPort_;              ///< Communication port name

    // Communication handle (platform-specific)
    void* commHandle_ = nullptr;

    /**
     * @brief Open communication port
     * @return Result<void> Success or error
     */
    Result<void> openCommunication();

    /**
     * @brief Close communication port
     */
    void closeCommunication();
};

} // namespace helab
