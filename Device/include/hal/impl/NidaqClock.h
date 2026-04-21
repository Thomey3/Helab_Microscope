#pragma once

#include "../IHardware.h"
#include "../../common/Logger.h"
#include <NIDAQmx.h>
#include <atomic>
#include <string>

namespace helab {

/// NI-DAQmx based clock generator implementation
class NidaqClock : public IClock {
public:
    /// Constructor with device name
    explicit NidaqClock(const std::string& deviceName);

    /// Destructor - ensures cleanup
    ~NidaqClock() override;

    // Rule of Five
    NidaqClock(const NidaqClock&) = delete;
    NidaqClock& operator=(const NidaqClock&) = delete;
    NidaqClock(NidaqClock&&) noexcept;
    NidaqClock& operator=(NidaqClock&&) noexcept;

    // IClock interface implementation
    Result<void> configure(const ClockConfig& config) override;
    Result<void> start() override;
    Result<void> stop() override;
    Result<void> reset() override;
    bool isRunning() const override;
    const ClockConfig& config() const override;

private:
    std::string deviceName_;
    TaskHandle taskHandle_ = nullptr;
    ClockConfig config_;
    std::atomic<bool> running_{false};
    std::atomic<bool> configured_{false};

    /// Create counter task for pixel clock
    Result<void> createCounterTask();

    /// Configure counter for pulse generation
    Result<void> configureCounter();

    /// Destroy task (RAII helper)
    void destroyTask();

    /// Convert DAQmx error to HardwareError
    static HardwareError daqError(int32 errorCode);

    /// Get error string from DAQmx
    static std::string getDaqErrorString(int32 errorCode);
};

} // namespace helab
