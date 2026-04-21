#pragma once

#include "../IHardware.h"
#include "../../common/Logger.h"
#include <NIDAQmx.h>
#include <atomic>
#include <string>
#include <vector>

namespace helab {

/// Galvanometer scanner implementation using NI-DAQmx
class GalvoScanner : public IScanner {
public:
    /// Constructor with device name
    explicit GalvoScanner(const std::string& deviceName);

    /// Destructor - ensures cleanup
    ~GalvoScanner() override;

    // Rule of Five
    GalvoScanner(const GalvoScanner&) = delete;
    GalvoScanner& operator=(const GalvoScanner&) = delete;
    GalvoScanner(GalvoScanner&&) noexcept;
    GalvoScanner& operator=(GalvoScanner&&) noexcept;

    // IScanner interface implementation
    Result<void> configure(const ScannerConfig& config) override;
    Result<void> setScanPattern(const ScanPattern& pattern) override;
    Result<void> start() override;
    Result<void> stop() override;
    Result<void> setPosition(double x, double y) override;
    bool isRunning() const override;
    const ScannerConfig& config() const override;
    const ScanPattern& pattern() const override;
    const std::vector<double>& waveformX() const override;
    const std::vector<double>& waveformY() const override;

private:
    std::string deviceName_;
    TaskHandle taskHandle_ = nullptr;
    ScannerConfig config_;
    ScanPattern pattern_;
    std::vector<double> waveformX_;
    std::vector<double> waveformY_;
    std::atomic<bool> running_{false};
    std::atomic<bool> configured_{false};

    /// Create AO task for scanning
    Result<void> createAOTask();

    /// Generate galvo scan waveforms
    void generateGalvoWaveform();

    /// Write waveforms to DAQ
    Result<void> writeWaveforms();

    /// Destroy task (RAII helper)
    void destroyTask();

    /// Convert DAQmx error to HardwareError
    static HardwareError daqError(int32 errorCode);

    /// Get error string from DAQmx
    static std::string getDaqErrorString(int32 errorCode);
};

} // namespace helab
