#pragma once
#include "DeviceBase.h"
#include "NIDAQmx.h"
#include <string>
#include <vector>
#include <map>
#include <stdexcept>

extern const char* g_DeviceNameGalvo;

class Galvo : public CGenericBase<Galvo>
{
public:
    Galvo();
    ~Galvo();

    // Device API
    int Initialize();
    int Shutdown();
    void GetName(char* name) const;
    bool Busy();

    // --- Single-Task AO Control API ---

    // Properties for a single analog output channel
    struct ChannelProperty
    {
        std::string physicalChannelName; // Full name, e.g., "/Dev1/ao0"
        std::vector<double> baseSequence; // One cycle of the waveform
        double targetFrequency; // Target repetition frequency (Hz)
    };

    // Add or update a channel's properties.
    int AddChannel(const ChannelProperty& props);

    // Remove a channel by its physical name.
    int RemoveChannel(const std::string& physicalChannelName);

    // Get a list of currently added channel names.
    std::vector<std::string> GetChannelNames() const;

    void SetClockSource(const std::string& source);
    void SetTriggerSource(const std::string& source);
    // After adding all channels, this function calculates and prepares the data,
    // and configures the NI-DAQmx task.
    int Commit(double clockFrequency, bool isRetriggerable = false);

    // Control the prepared task.
    int Start();
    int Stop();
    bool IsDone() const;

private:
    void ClearTask();
    void CheckError(int32 error, const std::string& message) const;

    // --- Commit helper functions ---
    void CalculateTimingParameters(double clockFrequency,
        double& outMinFreq,
        std::map<std::string, double>& outUpsampleFactors) const;

    std::string GenerateInterleavedBuffer(double clockFrequency,
        double minFreq,
        const std::map<std::string, double>& upsampleFactors);

    void ConfigureDaqmxTask(const std::string& allPhysicalChannels,
        double clockFrequency,
        bool isRetriggerable);

    // Stores the properties for each channel, keyed by physical name.
    std::map<std::string, ChannelProperty> m_channels;

    std::string m_clockSource;
    std::string m_startTrigSource;

    TaskHandle m_taskHandle;
    bool m_isArmed; // True if Commit() was successful and task is ready.

    // Buffer to hold the final, interleaved data for all channels.
    std::vector<double> m_finalBuffer;

    // Cached properties from Commit()
    long long m_totalSamplesPerFrame;
};
