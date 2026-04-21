#pragma once

#include "../hal/IHardware.h"
#include <gmock/gmock.h>
#include <string>

namespace helab {
namespace testing {

/// Mock Clock for unit testing
class MockClock : public IClock {
public:
    MOCK_METHOD(Result<void>, configure, (const ClockConfig& config), (override));
    MOCK_METHOD(Result<void>, start, (), (override));
    MOCK_METHOD(Result<void>, stop, (), (override));
    MOCK_METHOD(Result<void>, reset, (), (override));
    MOCK_METHOD(bool, isRunning, (), (const, override));
};

/// Mock Scanner for unit testing
class MockScanner : public IScanner {
public:
    MOCK_METHOD(Result<void>, configure, (const ScannerConfig& config), (override));
    MOCK_METHOD(Result<void>, setScanPattern, (const ScanPattern& pattern), (override));
    MOCK_METHOD(Result<void>, start, (), (override));
    MOCK_METHOD(Result<void>, stop, (), (override));
    MOCK_METHOD(Result<void>, setPosition, (double x, double y), (override));
    MOCK_METHOD(bool, isRunning, (), (const, override));
};

/// Mock Detector for unit testing
class MockDetector : public IDetector {
public:
    MOCK_METHOD(Result<void>, configure, (const DetectorConfig& config), (override));
    MOCK_METHOD(Result<void>, startAcquisition, (), (override));
    MOCK_METHOD(Result<void>, stopAcquisition, (), (override));
    MOCK_METHOD(Result<void*>, getDataBuffer, (), (override));
    MOCK_METHOD(Result<uint32_t>, getAvailableDataSize, (), (override));
    MOCK_METHOD(bool, isAcquiring, (), (const, override));
    MOCK_METHOD(void, setDataCallback, (DataCallback callback), (override));
};

/// Mock Laser for unit testing
class MockLaser : public ILaser {
public:
    MOCK_METHOD(Result<void>, initialize, (), (override));
    MOCK_METHOD(Result<void>, setPower, (double power_mW), (override));
    MOCK_METHOD(Result<void>, enable, (bool enable), (override));
    MOCK_METHOD(Result<double>, getPower, (), (const, override));
    MOCK_METHOD(Result<double>, getWavelength, (), (const, override));
    MOCK_METHOD(bool, isEnabled, (), (const, override));
};

/// Mock Adaptive Optics for unit testing
class MockAdaptiveOptics : public IAdaptiveOptics {
public:
    MOCK_METHOD(Result<void>, initialize, (), (override));
    MOCK_METHOD(Result<void>, setActuatorPositions, (const std::vector<double>& positions), (override));
    MOCK_METHOD(Result<void>, applyCorrection, (const std::vector<double>& wavefront), (override));
    MOCK_METHOD(Result<void>, reset, (), (override));
};

/// Mock Hardware Factory for unit testing
class MockHardwareFactory : public IHardwareFactory {
public:
    MOCK_METHOD(std::unique_ptr<IClock>, createClock, (const std::string& type), (override));
    MOCK_METHOD(std::unique_ptr<IScanner>, createScanner, (const std::string& type), (override));
    MOCK_METHOD(std::unique_ptr<IDetector>, createDetector, (const std::string& type), (override));
    MOCK_METHOD(std::unique_ptr<ILaser>, createLaser, (const std::string& type), (override));
    MOCK_METHOD(std::unique_ptr<IAdaptiveOptics>, createAdaptiveOptics, (const std::string& type), (override));
};

} // namespace testing
} // namespace helab
