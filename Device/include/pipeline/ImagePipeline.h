#pragma once

#include "../common/Result.h"
#include <memory>
#include <vector>
#include <string>
#include <cstdint>

/**
 * @file ImagePipeline.h
 * @brief Image processing pipeline for two-photon microscopy
 *
 * The ImagePipeline provides a modular, extensible framework for processing
 * raw detector data into final images. It supports:
 *
 * - **Modular Processing Stages**: Chain multiple processing steps
 * - **GPU Acceleration**: CUDA-accelerated processing for real-time imaging
 * - **Buffer Management**: Efficient memory handling for high-speed acquisition
 *
 * @section pipeline Pipeline Architecture
 *
 * Data flows through a series of processing stages:
 *
 * @code
 * Raw Data → Descan → Intensity Map → Background Sub → Average → Output
 * @endcode
 *
 * Each stage implements the IProcessingStage interface and can be
 * added, removed, or reordered as needed.
 *
 * @section stages Available Stages
 *
 * | Stage | Purpose | GPU Support |
 * |-------|---------|-------------|
 * | DescanStage | Map raw data to image coordinates | Yes |
 * | IntensityMapStage | Apply lookup table | Yes |
 * | BackgroundSubtractionStage | Remove background signal | Yes |
 * | AveragingStage | Frame averaging | No |
 *
 * @section example Example Usage
 *
 * @code
 * // Create pipeline
 * helab::ImagePipeline pipeline;
 *
 * // Configure
 * helab::PipelineConfig config;
 * config.width = 512;
 * config.height = 512;
 * config.enableGpu = true;
 * pipeline.configure(config);
 *
 * // Add processing stages
 * pipeline.addStage(std::make_unique<helab::DescanStage>(512, 512, true));
 * pipeline.addStage(std::make_unique<helab::IntensityMapStage>());
 *
 * // Process data
 * std::vector<uint8_t> output;
 * pipeline.process(rawData, rawSize, output);
 * @endcode
 *
 * @see IProcessingStage
 * @see MicroscopeController
 */

namespace helab {

/**
 * @class IProcessingStage
 * @brief Interface for image processing stages
 *
 * Each processing stage transforms input data to output data.
 * Stages can be chained together in the pipeline.
 *
 * @section impl Implementations
 * - DescanStage: Raw data to image mapping
 * - IntensityMapStage: LUT application
 * - BackgroundSubtractionStage: Background removal
 * - AveragingStage: Frame averaging
 */
class IProcessingStage {
public:
    virtual ~IProcessingStage() = default;

    /**
     * @brief Process data
     * @param inputData Pointer to input data buffer
     * @param outputData Pointer to output data buffer
     * @param size Size of data in bytes
     *
     * Transforms input data and writes to output buffer.
     * Input and output may be the same buffer for in-place processing.
     */
    virtual void process(void* inputData, void* outputData, size_t size) = 0;

    /**
     * @brief Get stage name
     * @return Name string for identification
     */
    virtual std::string name() const = 0;

    /**
     * @brief Check if GPU accelerated
     * @return true if this stage uses GPU acceleration
     */
    virtual bool isGpuAccelerated() const { return false; }
};

/**
 * @struct PipelineConfig
 * @brief Image pipeline configuration
 */
struct PipelineConfig {
    bool enableGpu = true;      ///< Enable GPU acceleration
    uint32_t width = 512;       ///< Image width in pixels
    uint32_t height = 512;      ///< Image height in pixels
    uint32_t channels = 1;      ///< Number of channels
    uint32_t bufferSize = 10;   ///< Number of buffers in pool

    /**
     * @brief Calculate image size in bytes
     * @return Size in bytes for single frame
     */
    uint32_t imageSize() const { return width * height * channels; }
};

/**
 * @class ImagePipeline
 * @brief Image processing pipeline manager
 *
 * Manages a chain of processing stages and handles buffer management
 * for efficient image processing.
 *
 * @section features Features
 * - Modular stage-based processing
 * - Optional GPU acceleration
 * - Buffer pooling for memory efficiency
 * - Thread-safe operation
 *
 * @section gpu GPU Acceleration
 *
 * When GPU acceleration is enabled:
 * - Data is copied to GPU memory once
 * - All GPU-capable stages process on GPU
 * - Final result copied back to CPU
 *
 * This minimizes CPU-GPU data transfers for real-time processing.
 */
class ImagePipeline {
public:
    /**
     * @brief Constructor
     *
     * Creates an empty pipeline with no processing stages.
     */
    ImagePipeline();

    /**
     * @brief Destructor
     *
     * Releases all resources including GPU memory.
     */
    ~ImagePipeline();

    /// @name Pipeline Management
    /// @{

    /**
     * @brief Add processing stage
     * @param stage Unique pointer to processing stage
     *
     * Adds a stage to the end of the processing chain.
     * Stages are processed in the order they are added.
     */
    void addStage(std::unique_ptr<IProcessingStage> stage);

    /**
     * @brief Remove processing stage by name
     * @param name Name of stage to remove
     *
     * Removes the first stage with the given name.
     */
    void removeStage(const std::string& name);

    /**
     * @brief Clear all processing stages
     */
    void clearStages();

    /**
     * @brief Get number of stages
     * @return Number of processing stages
     */
    size_t stageCount() const;

    /// @}

    /// @name Processing
    /// @{

    /**
     * @brief Process raw data through pipeline
     * @param rawData Pointer to raw detector data
     * @param rawSize Size of raw data in bytes
     * @param outputImage Output image vector (resized as needed)
     *
     * Processes the raw data through all stages in sequence.
     */
    void process(void* rawData, size_t rawSize, std::vector<uint8_t>& outputImage);

    /// @}

    /// @name Configuration
    /// @{

    /**
     * @brief Configure pipeline
     * @param config Configuration parameters
     */
    void configure(const PipelineConfig& config);

    /**
     * @brief Get current configuration
     * @return Reference to current PipelineConfig
     */
    const PipelineConfig& config() const;

    /// @}

    /// @name GPU Control
    /// @{

    /**
     * @brief Enable or disable GPU acceleration
     * @param enable true to enable GPU, false for CPU-only
     */
    void enableGpuAcceleration(bool enable);

    /**
     * @brief Check if GPU is enabled
     * @return true if GPU acceleration is active
     */
    bool isGpuEnabled() const;

    /// @}

    /// @name Buffer Management
    /// @{

    /**
     * @brief Acquire a buffer from the pool
     * @return Pointer to buffer or nullptr if pool exhausted
     */
    uint8_t* acquireBuffer();

    /**
     * @brief Release a buffer back to the pool
     * @param buffer Pointer to buffer to release
     */
    void releaseBuffer(uint8_t* buffer);

    /// @}

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

/**
 * @class DescanStage
 * @brief Maps raw detector data to image coordinates
 *
 * The descan stage transforms raw PMT data into a properly ordered image.
 * For bidirectional scanning, it handles the reversal of alternate lines.
 *
 * @section operation Operation
 *
 * 1. Reshape 1D raw data into 2D image
 * 2. For bidirectional: reverse every other line
 * 3. Apply fill fraction (discard non-imaging portion)
 *
 * @section example Example
 * @code
 * DescanStage stage(512, 512, true);  // 512x512, bidirectional
 * pipeline.addStage(std::make_unique<DescanStage>(512, 512, true));
 * @endcode
 */
class DescanStage : public IProcessingStage {
public:
    /**
     * @brief Constructor
     * @param width Image width in pixels
     * @param height Image height in pixels
     * @param bidirectional true for bidirectional scanning
     */
    DescanStage(uint32_t width, uint32_t height, bool bidirectional = true);

    void process(void* inputData, void* outputData, size_t size) override;
    std::string name() const override { return "Descan"; }

    /**
     * @brief Set bidirectional mode
     * @param bidirectional true for bidirectional scanning
     */
    void setBidirectional(bool bidirectional);

private:
    uint32_t width_;
    uint32_t height_;
    bool bidirectional_;
};

/**
 * @class IntensityMapStage
 * @brief Applies intensity lookup table (LUT)
 *
 * Maps raw intensity values to output values using a lookup table.
 * Used for:
 * - Gamma correction
 * - Contrast enhancement
 * - Color mapping (for multi-channel)
 *
 * @section example Example
 * @code
 * IntensityMapStage stage;
 *
 * // Create custom LUT
 * std::vector<uint16_t> lut(65536);
 * for (int i = 0; i < 65536; i++) {
 *     lut[i] = std::min(65535, (int)(std::pow(i / 65535.0, 0.5) * 65535));
 * }
 * stage.setLookupTable(lut);
 * @endcode
 */
class IntensityMapStage : public IProcessingStage {
public:
    /**
     * @brief Constructor
     *
     * Creates stage with default linear LUT.
     */
    IntensityMapStage();

    void process(void* inputData, void* outputData, size_t size) override;
    std::string name() const override { return "IntensityMap"; }

    /**
     * @brief Set custom lookup table
     * @param lut Vector of 16-bit output values (size 65536)
     */
    void setLookupTable(const std::vector<uint16_t>& lut);

private:
    std::vector<uint16_t> lut_;

    /**
     * @brief Generate default linear LUT
     */
    void generateDefaultLut();
};

/**
 * @class BackgroundSubtractionStage
 * @brief Removes background signal from images
 *
 * Subtracts a stored background image from each frame.
 * Background can be:
 * - Measured with laser off
 * - Computed from average of multiple frames
 * - Loaded from file
 *
 * @section example Example
 * @code
 * BackgroundSubtractionStage stage;
 *
 * // Measure background
 * std::vector<std::vector<uint8_t>> darkFrames;
 * // ... collect dark frames ...
 * stage.computeBackgroundFromFrames(darkFrames);
 * @endcode
 */
class BackgroundSubtractionStage : public IProcessingStage {
public:
    /**
     * @brief Constructor
     *
     * Creates stage with zero background (no subtraction).
     */
    BackgroundSubtractionStage();

    void process(void* inputData, void* outputData, size_t size) override;
    std::string name() const override { return "BackgroundSubtraction"; }

    /**
     * @brief Set background image
     * @param background Background image as 16-bit values
     */
    void setBackground(const std::vector<uint16_t>& background);

    /**
     * @brief Compute background from multiple frames
     * @param frames Vector of frames to average
     *
     * Computes the mean of all frames as the background.
     */
    void computeBackgroundFromFrames(const std::vector<std::vector<uint8_t>>& frames);

private:
    std::vector<uint16_t> background_;
};

/**
 * @class AveragingStage
 * @brief Accumulates and averages multiple frames
 *
 * Performs running average of N frames to improve signal-to-noise ratio.
 * Useful for:
 * - Reducing shot noise
 * - Improving image quality for static samples
 *
 * @section example Example
 * @code
 * AveragingStage stage(4);  // Average 4 frames
 * pipeline.addStage(std::make_unique<AveragingStage>(4));
 * @endcode
 */
class AveragingStage : public IProcessingStage {
public:
    /**
     * @brief Constructor
     * @param numFrames Number of frames to average
     */
    explicit AveragingStage(uint32_t numFrames = 1);

    void process(void* inputData, void* outputData, size_t size) override;
    std::string name() const override { return "Averaging"; }

    /**
     * @brief Set number of frames to average
     * @param numFrames Number of frames
     */
    void setNumFrames(uint32_t numFrames);

    /**
     * @brief Reset accumulator
     *
     * Clears the accumulator to start fresh.
     */
    void reset();

private:
    uint32_t numFrames_;
    uint32_t currentFrame_ = 0;
    std::vector<uint32_t> accumulator_;
};

} // namespace helab
