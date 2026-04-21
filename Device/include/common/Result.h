#pragma once

#include <expected>
#include <string>
#include <format>

namespace helab {

/// Hardware error information
struct HardwareError {
    int code;
    std::string message;

    HardwareError(int c, std::string_view msg)
        : code(c), message(msg) {}

    static HardwareError configuration(std::string_view msg) {
        return {-1, std::format("Configuration error: {}", msg)};
    }

    static HardwareError notInitialized() {
        return {-2, "Device not initialized"};
    }

    static HardwareError alreadyRunning() {
        return {-3, "Device already running"};
    }

    static HardwareError notRunning() {
        return {-4, "Device not running"};
    }

    static HardwareError driverError(int code, std::string_view msg) {
        return {code, std::format("Driver error: {}", msg)};
    }

    static HardwareError timeout(std::string_view operation) {
        return {-5, std::format("Timeout during: {}", operation)};
    }

    static HardwareError outOfMemory() {
        return {-6, "Out of memory"};
    }

    static HardwareError invalidParameter(std::string_view param) {
        return {-7, std::format("Invalid parameter: {}", param)};
    }
};

/// Result type for operations that can fail
template<typename T = void>
using Result = std::expected<T, HardwareError>;

/// Specialization for void results
template<>
struct Result<void> : std::expected<void, HardwareError> {
    using std::expected<void, HardwareError>::expected;
};

/// Helper to create success result
inline Result<void> success() {
    return {};
}

/// Helper to create error result
inline Result<void> failure(HardwareError error) {
    return std::unexpected(error);
}

/// Helper to create error result with message
inline Result<void> failure(int code, std::string_view message) {
    return std::unexpected(HardwareError{code, std::string(message)});
}

} // namespace helab
