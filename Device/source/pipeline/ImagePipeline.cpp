#include "pipeline/ImagePipeline.h"
#include <algorithm>
#include <numeric>
#include <cstring>

namespace helab {

// ============================================================================
// ImagePipeline Implementation
// ============================================================================

ImagePipeline::ImagePipeline() = default;

ImagePipeline::~ImagePipeline() {
    cleanupGpu();
}

void ImagePipeline::addStage(std::unique_ptr<IProcessingStage> stage) {
    stages_.push_back(std::move(stage));
}

void ImagePipeline::removeStage(const std::string& name) {
    stages_.erase(
        std::remove_if(stages_.begin(), stages_.end(),
            [&name](const auto& stage) { return stage->name() == name; }),
        stages_.end()
    );
}

void ImagePipeline::clearStages() {
    stages_.clear();
}

size_t ImagePipeline::stageCount() const {
    return stages_.size();
}

void ImagePipeline::process(void* rawData, size_t rawSize, std::vector<uint8_t>& outputImage) {
    // Calculate output size
    size_t outputSize = config_.width * config_.height * config_.channels * sizeof(uint16_t);
    outputImage.resize(outputSize);

    // Create intermediate buffers
    std::vector<uint8_t> intermediate(rawSize);
    std::vector<uint8_t> temp(outputSize);

    // Copy input to intermediate
    std::memcpy(intermediate.data(), rawData, rawSize);

    // Process through stages
    void* input = intermediate.data();
    void* output = outputImage.data();

    for (size_t i = 0; i < stages_.size(); ++i) {
        // Swap buffers for next stage
        if (i > 0) {
            std::swap(input, output);
        }

        stages_[i]->process(input, output, rawSize);
    }
}

void ImagePipeline::configure(const PipelineConfig& config) {
    config_ = config;

    // Initialize buffer pool
    bufferPool_.resize(config_.bufferSize);
    bufferInUse_.resize(config_.bufferSize, false);

    size_t bufferSize = config_.width * config_.height * config_.channels * sizeof(uint16_t);
    for (auto& buffer : bufferPool_) {
        buffer.resize(bufferSize);
    }

    if (config_.enableGpu) {
        initializeGpu();
    }
}

const PipelineConfig& ImagePipeline::config() const {
    return config_;
}

void ImagePipeline::enableGpuAcceleration(bool enable) {
    if (enable && !gpuEnabled_) {
        initializeGpu();
    } else if (!enable && gpuEnabled_) {
        cleanupGpu();
    }
    gpuEnabled_ = enable;
}

bool ImagePipeline::isGpuEnabled() const {
    return gpuEnabled_;
}

uint8_t* ImagePipeline::acquireBuffer() {
    for (size_t i = 0; i < bufferInUse_.size(); ++i) {
        if (!bufferInUse_[i]) {
            bufferInUse_[i] = true;
            return bufferPool_[i].data();
        }
    }
    return nullptr;  // No free buffer
}

void ImagePipeline::releaseBuffer(uint8_t* buffer) {
    for (size_t i = 0; i < bufferPool_.size(); ++i) {
        if (bufferPool_[i].data() == buffer) {
            bufferInUse_[i] = false;
            return;
        }
    }
}

void ImagePipeline::initializeGpu() {
    // GPU initialization will be implemented with CUDA
    // For now, just set the flag
    gpuEnabled_ = false;  // Disabled until CUDA is integrated
}

void ImagePipeline::cleanupGpu() {
    // GPU cleanup will be implemented with CUDA
    gpuEnabled_ = false;
}

// ============================================================================
// DescanStage Implementation
// ============================================================================

DescanStage::DescanStage(uint32_t width, uint32_t height, bool bidirectional)
    : width_(width), height_(height), bidirectional_(bidirectional) {}

void DescanStage::process(void* inputData, void* outputData, size_t size) {
    const uint16_t* input = static_cast<const uint16_t*>(inputData);
    uint16_t* output = static_cast<uint16_t*>(outputData);

    // Simple descanning: reorder data from scan pattern to image
    // For bidirectional scanning, reverse every other line
    for (uint32_t y = 0; y < height_; ++y) {
        const uint16_t* srcLine = input + y * width_;
        uint16_t* dstLine = output + y * width_;

        if (bidirectional_ && (y % 2 == 1)) {
            // Reverse line for bidirectional scanning
            for (uint32_t x = 0; x < width_; ++x) {
                dstLine[x] = srcLine[width_ - 1 - x];
            }
        } else {
            // Copy line as-is
            std::memcpy(dstLine, srcLine, width_ * sizeof(uint16_t));
        }
    }
}

void DescanStage::setBidirectional(bool bidirectional) {
    bidirectional_ = bidirectional;
}

// ============================================================================
// IntensityMapStage Implementation
// ============================================================================

IntensityMapStage::IntensityMapStage() {
    generateDefaultLut();
}

void IntensityMapStage::process(void* inputData, void* outputData, size_t size) {
    const uint16_t* input = static_cast<const uint16_t*>(inputData);
    uint16_t* output = static_cast<uint16_t*>(outputData);

    size_t numPixels = size / sizeof(uint16_t);
    for (size_t i = 0; i < numPixels; ++i) {
        uint16_t value = input[i];
        if (value < lut_.size()) {
            output[i] = lut_[value];
        } else {
            output[i] = lut_.back();
        }
    }
}

void IntensityMapStage::setLookupTable(const std::vector<uint16_t>& lut) {
    lut_ = lut;
}

void IntensityMapStage::generateDefaultLut() {
    lut_.resize(65536);
    for (size_t i = 0; i < lut_.size(); ++i) {
        lut_[i] = static_cast<uint16_t>(i);
    }
}

// ============================================================================
// BackgroundSubtractionStage Implementation
// ============================================================================

BackgroundSubtractionStage::BackgroundSubtractionStage() {
    background_.resize(512 * 512, 0);
}

void BackgroundSubtractionStage::process(void* inputData, void* outputData, size_t size) {
    const uint16_t* input = static_cast<const uint16_t*>(inputData);
    uint16_t* output = static_cast<uint16_t*>(outputData);

    size_t numPixels = size / sizeof(uint16_t);
    for (size_t i = 0; i < numPixels && i < background_.size(); ++i) {
        int32_t value = static_cast<int32_t>(input[i]) - static_cast<int32_t>(background_[i]);
        output[i] = static_cast<uint16_t>(std::max(0, value));
    }
}

void BackgroundSubtractionStage::setBackground(const std::vector<uint16_t>& background) {
    background_ = background;
}

void BackgroundSubtractionStage::computeBackgroundFromFrames(
    const std::vector<std::vector<uint8_t>>& frames) {
    if (frames.empty()) return;

    size_t numPixels = frames[0].size() / sizeof(uint16_t);
    background_.resize(numPixels);

    // Compute average
    std::vector<uint32_t> sum(numPixels, 0);
    for (const auto& frame : frames) {
        const uint16_t* data = reinterpret_cast<const uint16_t*>(frame.data());
        for (size_t i = 0; i < numPixels; ++i) {
            sum[i] += data[i];
        }
    }

    for (size_t i = 0; i < numPixels; ++i) {
        background_[i] = static_cast<uint16_t>(sum[i] / frames.size());
    }
}

// ============================================================================
// AveragingStage Implementation
// ============================================================================

AveragingStage::AveragingStage(uint32_t numFrames)
    : numFrames_(numFrames) {}

void AveragingStage::process(void* inputData, void* outputData, size_t size) {
    const uint16_t* input = static_cast<const uint16_t*>(inputData);
    uint16_t* output = static_cast<uint16_t*>(outputData);

    size_t numPixels = size / sizeof(uint16_t);

    // Initialize accumulator on first frame
    if (currentFrame_ == 0) {
        accumulator_.resize(numPixels, 0);
        std::fill(accumulator_.begin(), accumulator_.end(), 0);
    }

    // Accumulate
    for (size_t i = 0; i < numPixels; ++i) {
        accumulator_[i] += input[i];
    }

    currentFrame_++;

    // Output average when complete
    if (currentFrame_ >= numFrames_) {
        for (size_t i = 0; i < numPixels; ++i) {
            output[i] = static_cast<uint16_t>(accumulator_[i] / numFrames_);
        }
        currentFrame_ = 0;
    } else {
        // Pass through input while accumulating
        std::memcpy(output, input, size);
    }
}

void AveragingStage::setNumFrames(uint32_t numFrames) {
    numFrames_ = numFrames;
    reset();
}

void AveragingStage::reset() {
    currentFrame_ = 0;
    accumulator_.clear();
}

} // namespace helab
