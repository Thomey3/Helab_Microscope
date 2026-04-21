#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "../mocks/MockHardware.h"
#include "app/MicroscopeController.h"

using namespace helab;
using namespace helab::testing;
using ::testing::_;
using ::testing::Return;
using ::testing::NiceMock;

class MicroscopeControllerTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Create mock hardware
        mockClock = std::make_unique<NiceMock<MockClock>>();
        mockScanner = std::make_unique<NiceMock<MockScanner>>();
        mockDetector = std::make_unique<NiceMock<MockDetector>>();

        // Create mock factory
        mockFactory = std::make_shared<MockHardwareFactory>();

        // Setup factory to return our mocks
        ON_CALL(*mockFactory, createClock(_))
            .WillByDefault(Return(std::move(mockClock)));
        ON_CALL(*mockFactory, createScanner(_))
            .WillByDefault(Return(std::move(mockScanner)));
        ON_CALL(*mockFactory, createDetector(_))
            .WillByDefault(Return(std::move(mockDetector)));
    }

    std::shared_ptr<MockHardwareFactory> mockFactory;
    std::unique_ptr<MockClock> mockClock;
    std::unique_ptr<MockScanner> mockScanner;
    std::unique_ptr<MockDetector> mockDetector;
};

// ============================================================================
// Initialization Tests
// ============================================================================

TEST_F(MicroscopeControllerTest, Initialize_WhenNotInitialized_InitializesAllHardware) {
    // Arrange
    EXPECT_CALL(*mockFactory, createClock("nidaq")).Times(1);
    EXPECT_CALL(*mockFactory, createScanner("galvo")).Times(1);
    EXPECT_CALL(*mockFactory, createDetector("qt")).Times(1);

    MicroscopeController controller(mockFactory);

    // Act
    auto result = controller.initialize();

    // Assert
    EXPECT_TRUE(result);
    EXPECT_TRUE(controller.isInitialized());
    EXPECT_EQ(controller.state(), MicroscopeState::Ready);
}

TEST_F(MicroscopeControllerTest, Initialize_WhenAlreadyInitialized_ReturnsSuccess) {
    // Arrange
    MicroscopeController controller(mockFactory);
    controller.initialize();

    // Act
    auto result = controller.initialize();

    // Assert
    EXPECT_TRUE(result);
}

// ============================================================================
// Imaging Tests
// ============================================================================

TEST_F(MicroscopeControllerTest, StartImaging_WhenNotInitialized_ReturnsError) {
    // Arrange
    MicroscopeController controller(mockFactory);

    // Act
    auto result = controller.startImaging();

    // Assert
    EXPECT_FALSE(result);
    EXPECT_EQ(result.error().code, static_cast<int>(HardwareErrorCode::NotInitialized));
}

TEST_F(MicroscopeControllerTest, SetImagingConfig_WhenValid_UpdatesConfig) {
    // Arrange
    MicroscopeController controller(mockFactory);
    controller.initialize();

    ImagingConfig config;
    config.fps = 2.0;
    config.width = 1024;
    config.height = 1024;

    // Act
    auto result = controller.setImagingConfig(config);

    // Assert
    EXPECT_TRUE(result);
    EXPECT_DOUBLE_EQ(controller.imagingConfig().fps, 2.0);
    EXPECT_EQ(controller.imagingConfig().width, 1024u);
    EXPECT_EQ(controller.imagingConfig().height, 1024u);
}

TEST_F(MicroscopeControllerTest, SetImagingConfig_WhenInvalidFps_ReturnsError) {
    // Arrange
    MicroscopeController controller(mockFactory);
    controller.initialize();

    ImagingConfig config;
    config.fps = -1.0;  // Invalid

    // Act
    auto result = controller.setImagingConfig(config);

    // Assert
    EXPECT_FALSE(result);
}

// ============================================================================
// Laser Tests
// ============================================================================

TEST_F(MicroscopeControllerTest, AddLaser_WhenValid_AddsLaser) {
    // Arrange
    MicroscopeController controller(mockFactory);
    controller.initialize();

    auto mockLaser = std::make_unique<NiceMock<MockLaser>>();
    EXPECT_CALL(*mockLaser, initialize())
        .WillOnce(Return(success()));

    // Act
    auto result = controller.addLaser(std::move(mockLaser), "920nm");

    // Assert
    EXPECT_TRUE(result);
    auto names = controller.laserNames();
    EXPECT_EQ(names.size(), 1u);
    EXPECT_EQ(names[0], "920nm");
}

TEST_F(MicroscopeControllerTest, SetLaserPower_WhenLaserExists_SetsPower) {
    // Arrange
    MicroscopeController controller(mockFactory);
    controller.initialize();

    auto mockLaser = std::make_unique<NiceMock<MockLaser>>();
    EXPECT_CALL(*mockLaser, initialize())
        .WillOnce(Return(success()));
    EXPECT_CALL(*mockLaser, setPower(100.0))
        .WillOnce(Return(success()));

    controller.addLaser(std::move(mockLaser), "920nm");

    // Act
    auto result = controller.setLaserPower("920nm", 100.0);

    // Assert
    EXPECT_TRUE(result);
}

TEST_F(MicroscopeControllerTest, SetLaserPower_WhenLaserNotFound_ReturnsError) {
    // Arrange
    MicroscopeController controller(mockFactory);
    controller.initialize();

    // Act
    auto result = controller.setLaserPower("nonexistent", 100.0);

    // Assert
    EXPECT_FALSE(result);
}

TEST_F(MicroscopeControllerTest, EnableLaser_WhenLaserExists_EnablesLaser) {
    // Arrange
    MicroscopeController controller(mockFactory);
    controller.initialize();

    auto mockLaser = std::make_unique<NiceMock<MockLaser>>();
    EXPECT_CALL(*mockLaser, initialize())
        .WillOnce(Return(success()));
    EXPECT_CALL(*mockLaser, enable(true))
        .WillOnce(Return(success()));

    controller.addLaser(std::move(mockLaser), "920nm");

    // Act
    auto result = controller.enableLaser("920nm", true);

    // Assert
    EXPECT_TRUE(result);
}

// ============================================================================
// ROI Tests
// ============================================================================

TEST_F(MicroscopeControllerTest, SetROI_WhenValid_UpdatesROI) {
    // Arrange
    MicroscopeController controller(mockFactory);
    controller.initialize();

    ROI roi{100, 100, 256, 256};

    // Act
    auto result = controller.setROI(roi);

    // Assert
    EXPECT_TRUE(result);
    EXPECT_EQ(controller.roi().x, 100u);
    EXPECT_EQ(controller.roi().y, 100u);
    EXPECT_EQ(controller.roi().width, 256u);
    EXPECT_EQ(controller.roi().height, 256u);
}

TEST_F(MicroscopeControllerTest, SetROI_WhenZeroDimensions_ReturnsError) {
    // Arrange
    MicroscopeController controller(mockFactory);
    controller.initialize();

    ROI roi{0, 0, 0, 0};  // Invalid

    // Act
    auto result = controller.setROI(roi);

    // Assert
    EXPECT_FALSE(result);
}

// ============================================================================
// Scan Type Tests
// ============================================================================

TEST_F(MicroscopeControllerTest, SetScanType_WhenNotImaging_UpdatesScanType) {
    // Arrange
    MicroscopeController controller(mockFactory);
    controller.initialize();

    // Act
    auto result = controller.setScanType(ScanPattern::Type::Galvo);

    // Assert
    EXPECT_TRUE(result);
    EXPECT_EQ(controller.scanType(), ScanPattern::Type::Galvo);
}
