#include "galvo.h"
#include <numeric> 
#include <cmath>   
#include <stdexcept>

const char* g_DeviceNameGalvo = "Galvo";
//#define DEBUG
#ifdef DEBUG
#include "clock.h"
#endif // DEBUG


Galvo::Galvo()
    : m_taskHandle(nullptr),
    m_isArmed(false),
    m_totalSamplesPerFrame(0)
{
}

Galvo::~Galvo()
{
    Shutdown();
}

int Galvo::Initialize()
{
#ifdef DEBUG
    // --- Test Case: Configure and arm a retriggerable, dual-channel galvo scan ---
    LogMessage("--- Running test case in Galvo::Initialize ---", false);

    // --- 1. Define constants for the test to improve readability and maintenance ---
    const char* CLOCK_DEVICE_NAME = g_DeviceNameClock;
    const char* NI_DEVICE_NAME = "/Dev1";
    const char* SAMPLE_CLOCK_TASK_NAME = "galvo_sample_clock";
    const char* TRIGGER_CLOCK_TASK_NAME = "galvo_frame_trigger";
    const char* SAMPLE_CLOCK_SOURCE = "/Dev1/Ctr0InternalOutput";
    const char* TRIGGER_SOURCE = "/Dev1/Ctr1InternalOutput";
    const char* X_GALVO_CHANNEL = "/Dev1/ao0";
    const char* Y_GALVO_CHANNEL = "/Dev1/ao1";

    const double X_FREQ = 256.0;
    const size_t WAVEFORM_POINTS = 256; // For this design, both waveforms must have the same number of points.
    const double Y_FREQ = 1.0;

    // --- 2. Calculate the required sample clock frequency from channel properties ---
    // The sample clock must be fast enough for the highest frequency waveform.
    // Required Clock Frequency = Waveform Points * Highest Waveform Frequency
    const double required_clock_freq = WAVEFORM_POINTS * X_FREQ;
    LogMessage("TEST: Required sample clock calculated to be " + std::to_string(required_clock_freq) + " Hz.", false);

    // --- 3. Configure the Clock device to provide the necessary signals ---
    Clock* pClock = static_cast<Clock*>(GetDevice(CLOCK_DEVICE_NAME));
    if (!pClock) {
        LogMessage("TEST FAILED: Could not get Clock device.", true);
        return DEVICE_ERR;
    }

    // a) Configure the high-frequency sample clock
    Clock::TaskProperty sampleClock;
    sampleClock.niDeviceName = NI_DEVICE_NAME;
    sampleClock.clockChannel = "/ctr0";
    sampleClock.TaskName = SAMPLE_CLOCK_TASK_NAME;
    sampleClock.dutyCycle = 0.5;
    sampleClock.frequency = required_clock_freq; // CORRECTNESS FIX: Use the calculated frequency
    sampleClock.pulsesPerRun = -1; // Continuous
    pClock->addTask(sampleClock);

    // b) Configure the low-frequency trigger clock
    Clock::TaskProperty triggerClock;
    triggerClock.niDeviceName = NI_DEVICE_NAME;
    triggerClock.clockChannel = "/ctr1";
    triggerClock.TaskName = TRIGGER_CLOCK_TASK_NAME;
    triggerClock.dutyCycle = 0.5;
    triggerClock.frequency = Y_FREQ; // The trigger fires once per frame (defined by the slowest waveform)
    triggerClock.pulsesPerRun = -1; // Continuous for retriggering
    pClock->addTask(triggerClock);

    // --- 4. Define and add Galvo channels ---
    ChannelProperty x_galvo;
    x_galvo.physicalChannelName = X_GALVO_CHANNEL;
    x_galvo.targetFrequency = X_FREQ;
    x_galvo.baseSequence.resize(WAVEFORM_POINTS);
    for (size_t i = 0; i < WAVEFORM_POINTS / 2; ++i) {
        x_galvo.baseSequence[i] = -5.0 + (static_cast<double>(i) / (WAVEFORM_POINTS / 2 - 1)) * 10.0;
        x_galvo.baseSequence[i + WAVEFORM_POINTS / 2] = 5.0 - (static_cast<double>(i) / (WAVEFORM_POINTS / 2 - 1)) * 10.0;
    }

    ChannelProperty y_galvo;
    y_galvo.physicalChannelName = Y_GALVO_CHANNEL;
    y_galvo.targetFrequency = Y_FREQ;
    y_galvo.baseSequence.resize(WAVEFORM_POINTS);
    for (size_t i = 0; i < WAVEFORM_POINTS; ++i) {
        y_galvo.baseSequence[i] = -5.0 + (static_cast<double>(i) / (WAVEFORM_POINTS - 1)) * 10.0;
    }

    AddChannel(x_galvo);
    AddChannel(y_galvo);
    LogMessage("TEST: Galvo channels added.", false);

    // --- 5. Commit the Galvo configuration ---
    SetClockSource(SAMPLE_CLOCK_SOURCE);
    SetTriggerSource(TRIGGER_SOURCE);
    LogMessage("TEST: Clock and trigger sources have been set.", false);
    if (Commit(required_clock_freq, true) != DEVICE_OK) {
        LogMessage("TEST FAILED: Galvo Commit failed.", true);
        Shutdown(); // Clean up on failure
        return DEVICE_ERR;
    }
    LogMessage("TEST: Galvo task committed successfully in retriggerable mode.", false);

    // --- 6. Start all tasks in the correct, robust order ---
    LogMessage("TEST: Starting tasks...", false);
    // a) Start the sample clock first, so it is running when the galvo task needs it.
    pClock->getTask(SAMPLE_CLOCK_TASK_NAME)->start(); // CORRECTNESS FIX: Use correct task name
    // b) Arm the Galvo task. It will now wait for a trigger from the trigger clock.
    Start();
    // c) Start the trigger clock. The first pulse will fire the galvo task.
    pClock->getTask(TRIGGER_CLOCK_TASK_NAME)->start(); // CORRECTNESS FIX: Use correct task name

    LogMessage("TEST: System is running. Galvo will generate waves on each trigger.", false);

#endif // DEBUG

    // The main setup happens in Commit().
    return DEVICE_OK;
}

int Galvo::Shutdown()
{
    ClearTask();
    m_channels.clear();
    return DEVICE_OK;
}

void Galvo::SetClockSource(const std::string& source)
{
    m_clockSource = source;
    ClearTask(); // Changing a source invalidates any previously committed task
}

void Galvo::SetTriggerSource(const std::string& source)
{
    m_startTrigSource = source;
    ClearTask(); // Changing a source invalidates any previously committed task
}


void Galvo::GetName(char* name) const
{
    CDeviceUtils::CopyLimitedString(name, g_DeviceNameGalvo);
}

bool Galvo::Busy()
{
    if (!m_isArmed) {
        return false;
    }
    return !IsDone();
}

int Galvo::AddChannel(const ChannelProperty& props)
{
    if (props.physicalChannelName.empty()) {
        LogMessage("Error: Physical channel name cannot be empty.", true);
        return DEVICE_ERR;
    }
    if (props.baseSequence.empty()) {
        LogMessage("Error: Base sequence for channel " + props.physicalChannelName + " cannot be empty.", true);
        return DEVICE_ERR;
    }
    if (props.targetFrequency <= 0) {
        LogMessage("Error: Target frequency for channel " + props.physicalChannelName + " must be positive.", true);
        return DEVICE_ERR;
    }

    // Adding a channel invalidates the previously committed task.
    ClearTask();
    m_channels[props.physicalChannelName] = props;
    return DEVICE_OK;
}

int Galvo::RemoveChannel(const std::string& physicalChannelName)
{
    if (m_channels.erase(physicalChannelName) > 0) {
        // Removing a channel also invalidates the task.
        ClearTask();
        return DEVICE_OK;
    }
    return DEVICE_ERR; // Channel not found
}

std::vector<std::string> Galvo::GetChannelNames() const
{
    std::vector<std::string> names;
    for (const auto& pair : m_channels) {
        names.push_back(pair.first);
    }
    return names;
}

int Galvo::Commit(double clockFrequency, bool isRetriggerable)
{
    // 0. Cleanup any previous task and check preconditions.
    ClearTask();
    if (m_channels.empty()) {
        LogMessage("Cannot commit: No channels have been added.", true);
        return DEVICE_ERR;
    }

    try {
        // 1. Validate parameters and calculate timing.
        double minFreq;
        std::map<std::string, double> upsampleFactors;
        CalculateTimingParameters(clockFrequency, minFreq, upsampleFactors);

        // 2. Generate the interleaved data buffer for the DAQ card.
        std::string allPhysicalChannels = GenerateInterleavedBuffer(clockFrequency, minFreq, upsampleFactors);

        // 3. Configure the NI-DAQmx hardware task.
        ConfigureDaqmxTask(allPhysicalChannels, clockFrequency, isRetriggerable);


        m_isArmed = true;

    }
    catch (const std::runtime_error& e) {
        LogMessage(e.what(), true);
        ClearTask();
        return DEVICE_ERR;
    }

    return DEVICE_OK;
}

int Galvo::Start()
{
    if (!m_isArmed) {
        LogMessage("Cannot start: Task is not armed. Call Commit() first.", true);
        return DEVICE_ERR;
    }
    try {
        CheckError(DAQmxStartTask(m_taskHandle), "DAQmxStartTask failed.");
    }
    catch (const std::runtime_error& e) {
        LogMessage(e.what(), true);
        return DEVICE_ERR;
    }
    return DEVICE_OK;
}

int Galvo::Stop()
{
    LogMessage("Galvo stop", true);
    if (m_taskHandle) {
        DAQmxStopTask(m_taskHandle);
    }
    return DEVICE_OK;
}

bool Galvo::IsDone() const
{
    if (!m_isArmed) {
        return true;
    }
    uInt32 isTaskDone = 0;
    int32 error = DAQmxIsTaskDone(m_taskHandle, &isTaskDone);
    if (error < 0) {
        char errBuff[2048] = { '\0' };
        DAQmxGetExtendedErrorInfo(errBuff, 2048);
        LogMessage(errBuff, true);
        return true;
    }
    return isTaskDone;
}

void Galvo::ClearTask()
{
    if (m_taskHandle) {
        DAQmxStopTask(m_taskHandle);
        DAQmxClearTask(m_taskHandle);
        m_taskHandle = nullptr;
    }
    m_isArmed = false;
    m_finalBuffer.clear();
    m_totalSamplesPerFrame = 0;
}

void Galvo::CheckError(int32 error, const std::string& message) const
{
    if (error < 0) {
        char errBuff[2048] = { '\0' };
        DAQmxGetExtendedErrorInfo(errBuff, 2048);
        std::string full_message = message + "\nNI-DAQmx error: " + errBuff;
        throw std::runtime_error(full_message);
    }
}

void Galvo::CalculateTimingParameters(double clockFrequency,
    double& outMinFreq,
    std::map<std::string, double>& outUpsampleFactors) const
{
    outMinFreq = -1.0;
    outUpsampleFactors.clear();

    for (const auto& pair : m_channels) {
        const auto& props = pair.second;
        if (outMinFreq < 0 || props.targetFrequency < outMinFreq) {
            outMinFreq = props.targetFrequency;
        }

        double L = static_cast<double>(props.baseSequence.size());
        double U = clockFrequency / (props.targetFrequency * L);

        // Use a small epsilon for floating point comparison
        if (std::abs(U - std::round(U)) > 1e-9) {
            std::string msg = "Error: Incompatible parameters for channel " + props.physicalChannelName +
                ". The ratio clock_freq / (wave_freq * wave_points) must be an integer. Got " + std::to_string(U);
            throw std::runtime_error(msg);
        }
        outUpsampleFactors[props.physicalChannelName] = std::round(U);
    }
}

std::string Galvo::GenerateInterleavedBuffer(double clockFrequency,
    double minFreq,
    const std::map<std::string, double>& upsampleFactors)
{
    // Calculate frame parameters
    double frame_duration = 1.0 / minFreq;
    m_totalSamplesPerFrame = static_cast<long long>(std::round(clockFrequency * frame_duration));

    m_finalBuffer.assign(m_totalSamplesPerFrame * m_channels.size(), 0.0);
    std::string all_physical_channels;

    size_t current_channel_index = 0;
    for (const auto& pair : m_channels) {
        const auto& props = pair.second;

        // Concatenate channel names for DAQmx
        if (!all_physical_channels.empty()) {
            all_physical_channels += ", ";
        }
        all_physical_channels += props.physicalChannelName;

        // Generate the full sequence for this channel
        std::vector<double> alignedSequence;
        alignedSequence.reserve(m_totalSamplesPerFrame);

        double upsample_factor = upsampleFactors.at(props.physicalChannelName);
        long long repetitions = static_cast<long long>(std::round(props.targetFrequency / minFreq));

        // Create upsampled base sequence
        std::vector<double> upsampled_base;
        upsampled_base.reserve(props.baseSequence.size() * static_cast<size_t>(upsample_factor));
        for (double point : props.baseSequence) {
            for (int i = 0; i < static_cast<int>(upsample_factor); ++i) {
                upsampled_base.push_back(point);
            }
        }

        // Repeat the upsampled base sequence to fill the frame
        for (long long i = 0; i < repetitions; ++i) {
            alignedSequence.insert(alignedSequence.end(), upsampled_base.begin(), upsampled_base.end());
        }

        if (alignedSequence.size() != m_totalSamplesPerFrame) {
            throw std::runtime_error("Internal logic error: Final sequence length mismatch for channel " + props.physicalChannelName);
        }

        // Interleave data into the final buffer
        for (long long i = 0; i < m_totalSamplesPerFrame; ++i) {
            m_finalBuffer[i * m_channels.size() + current_channel_index] = alignedSequence[i];
        }
        current_channel_index++;
    }
    return all_physical_channels;
}

void Galvo::ConfigureDaqmxTask(const std::string& allPhysicalChannels,
    double clockFrequency,
    bool isRetriggerable)
{
    CheckError(DAQmxCreateTask("", &m_taskHandle), "DAQmxCreateTask failed.");

    CheckError(DAQmxCreateAOVoltageChan(m_taskHandle, allPhysicalChannels.c_str(), "", -10.0, 10.0, DAQmx_Val_Volts, NULL),
        "DAQmxCreateAOVoltageChan failed.");

    CheckError(DAQmxCfgSampClkTiming(m_taskHandle, m_clockSource.c_str(), clockFrequency, DAQmx_Val_Rising, DAQmx_Val_FiniteSamps, m_totalSamplesPerFrame),
        "DAQmxCfgSampClkTiming failed.");

    if (!m_startTrigSource.empty()) {
        CheckError(DAQmxCfgDigEdgeStartTrig(m_taskHandle, m_startTrigSource.c_str(), DAQmx_Val_Rising),
            "DAQmxCfgDigEdgeStartTrig failed.");

        // This is the key change to enable retriggerable behavior
        if (isRetriggerable) {
            CheckError(DAQmxSetStartTrigRetriggerable(m_taskHandle, TRUE), "DAQmxSetStartTrigRetriggerable failed.");
        }
    }

    // Write data to buffer but do not start the task automatically.
    // Explicit call to Start() is required. Timeout of -1.0 means wait indefinitely.
    CheckError(DAQmxWriteAnalogF64(m_taskHandle, m_totalSamplesPerFrame, 0, -1.0, DAQmx_Val_GroupByScanNumber, m_finalBuffer.data(), NULL, NULL),
        "DAQmxWriteAnalogF64 failed.");
}