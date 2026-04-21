#pragma once

#include "../common/Result.h"
#include <memory>
#include <vector>
#include <string>
#include <cstdint>

namespace helab {

/// Processing stage interface
class IProcessingStage {
public:
    virtual ~IProcessingStage() = default;

    /// Process data
    virtual void process(void* inputData, void* outputData, size_t size) = 0;

    /// Get stage name
    virtual std::string name() const = 0;

    /// Check if GPU accelerated
    virtual bool isGpuAccelerated() const { return false; }
};

/// Image pipeline configuration
struct PipelineConfig {
    bool enableGpu = true;
    uint32_t width = 512;
    uint32_t height = 512;
    uint32_t channels = 1;
    uint32_t bufferSize = 10;  // Number of buffers in pool
};

/// Image processing pipeline
class ImagePipeline {
public:
    ImagePipeline();
    ~ImagePipeline();

    // Pipeline management
    void addStage(std::unique_ptr<IProcessingStage> stage);
    void removeStage(const std::string& name);
    void clearStages();
    size_t stageCount() const;

    // Processing
    void process(void* rawData, size_t rawSize, std::vector<uint8_t>& outputImage);

    // Configuration
    void configure(const PipelineConfig& config);
    const PipelineConfig& config() const;

    // GPU control
    void enableGpuAcceleration(bool enable);
    bool isGpuEnabled() const;

    // Buffer management
    uint8_t* acquireBuffer();
    void releaseBuffer(uint8_t* buffer);

private:
    std::vector<std::unique_ptr<IProcessingStage>> stages_;
    PipelineConfig config_;
    bool gpuEnabled_ = false;

    // Buffer pool
    std::vector<std::vector<uint8_t>> bufferPool_;
    std::vector<bool> bufferInUse_;
    size_t currentBuffer_ = 0;

    // GPU resources (opaque pointers for CUDA)
    void* cudaStream_ = nullptr;
    void* d_rawData_ = nullptr;
    void* d_imageData_ = nullptr;

    void initializeGpu();
    void cleanupGpu();
};

// ============================================================================
// Standard Processing Stages
// ============================================================================

/// Descan stage - maps raw data to image coordinates
class DescanStage : public IProcessingStage {
public:
    DescanStage(uint32_t width, uint32_t height, bool bidirectional = true);

    void process(void* inputData, void* outputData, size_t size) override;
    std::string name() const override { return "Descan"; }

    void setBidirectional(bool bidirectional);

private:
    uint32_t width_;
    uint32_t height_;
    bool bidirectional_;
};

/// Intensity mapping stage - applies lookup table
class IntensityMapStage : public IProcessingStage {
public:
    IntensityMapStage();

    void process(void* inputData, void* outputData, size_t size) override;
    std::string name() const override { return "IntensityMap"; }

    void setLookupTable(const std::vector<uint16_t>& lut);

private:
    std::vector<uint16_t> lut_;
    void generateDefaultLut();
};

/// Background subtraction stage
class BackgroundSubtractionStage : public IProcessingStage {
public:
    BackgroundSubtractionStage();

    void process(void* inputData, void* outputData, size_t size) override;
    std::string name() const override { return "BackgroundSubtraction"; }

    void setBackground(const std::vector<uint16_t>& background);
    void computeBackgroundFromFrames(const std::vector<std::vector<uint8_t>>& frames);

private:
    std::vector<uint16_t> background_;
};

/// Averaging stage - accumulates and averages frames
class AveragingStage : public IProcessingStage {
public:
    explicit AveragingStage(uint32_t numFrames = 1);

    void process(void* inputData, void* outputData, size_t size) override;
    std::string name() const override { return "Averaging"; }

    void setNumFrames(uint32_t numFrames);
    void reset();

private:
    uint32_t numFrames_;
    uint32_t currentFrame_ = 0;
    std::vector<uint32_t> accumulator_;
};

} // namespace helab
