#include "ImgReconstruction.cuh"
#include <stdexcept>
#include <cstring>   // for memcpy
#include <cstdio>    // for fprintf

// 辅助宏，用于检查并报告CUDA API调用的错误
#define CUDA_CHECK(call) do { \
    cudaError_t err = call; \
    if (err != cudaSuccess) { \
        fprintf(stderr, "CUDA Error in %s:%d -> %s\n", __FILE__, __LINE__, cudaGetErrorString(err)); \
        throw std::runtime_error("A CUDA call failed."); \
    } \
} while(0)

ImgReconstruction::ImgReconstruction()
    : h_pinned_raw_data_(nullptr), h_pinned_raw_data_size_(0),
    d_raw_data_(nullptr), d_image_data_(nullptr),
    stream_(nullptr), is_initialized_(false),
    width_(0), height_(0), channels_(0), lineperBlock_(0),
    d_pixel_data_counts_(nullptr), d_pixel_data_offsets_(nullptr),
    is_map_initialized_(false)
{
}

ImgReconstruction::~ImgReconstruction()
{
    teardown();
}

int ImgReconstruction::setup(size_t max_raw_data_size, int width, int height, int channels, int lineperBlock, int phase)
{
    if (is_initialized_) {
        teardown();
    }

    width_ = width;
    height_ = height;
    channels_ = channels;
    lineperBlock_ = lineperBlock;

    h_pinned_raw_data_size_ = max_raw_data_size;
    d_image_data_size_ = static_cast<size_t>(width_) * lineperBlock_ * channels_ * sizeof(int16_t);

    try {
        // --- 分配内存 ---
        // 1. 为原始数据分配主机端锁页内存
        CUDA_CHECK(cudaHostAlloc((void**)&h_pinned_raw_data_, h_pinned_raw_data_size_, cudaHostAllocDefault));

        // 3. 分配设备端内存
        CUDA_CHECK(cudaMalloc((void**)&d_raw_data_, h_pinned_raw_data_size_));
        CUDA_CHECK(cudaMalloc((void**)&d_image_data_, d_image_data_size_));

        // 为 counts 和 offsets 数组分配设备内存
        full_raw_width = width_ + 2 * abs(phase);
        size_t pixel_map_size = static_cast<size_t>(full_raw_width) * sizeof(int);
        CUDA_CHECK(cudaMalloc((void**)&d_pixel_data_counts_, pixel_map_size));
        CUDA_CHECK(cudaMalloc((void**)&d_pixel_data_offsets_, pixel_map_size));

        // --- 创建流 ---
        CUDA_CHECK(cudaStreamCreate(&stream_));

#ifdef DEBUG
        CUDA_CHECK(cudaMemsetAsync(d_image_data_, 1, d_image_data_size_, stream_));
        CUDA_CHECK(cudaStreamSynchronize(stream_)); // 确保初始化完成
#endif // DEBUG


		lineperBlock_ = lineperBlock; // 保存每个块的行数
    }
    catch (const std::runtime_error& e) {
        fprintf(stderr, "Error during ImgReconstruction setup: %s\n", e.what());
        teardown(); // 出错时清理已分配的资源
        return -1;
    }

    is_initialized_ = true;
    return 0;
}

void ImgReconstruction::teardown()
{
    if (stream_) { cudaStreamDestroy(stream_); stream_ = nullptr; }
    if (h_pinned_raw_data_) { cudaFreeHost(h_pinned_raw_data_); h_pinned_raw_data_ = nullptr; }
    if (d_raw_data_) { cudaFree(d_raw_data_); d_raw_data_ = nullptr; }
    if (d_image_data_) { cudaFree(d_image_data_); d_image_data_ = nullptr; }
    if (d_pixel_data_counts_) { cudaFree(d_pixel_data_counts_); d_pixel_data_counts_ = nullptr; }
    if (d_pixel_data_offsets_) { cudaFree(d_pixel_data_offsets_); d_pixel_data_offsets_ = nullptr; }

    is_initialized_ = false;
    width_ = 0;
    height_ = 0;
    channels_ = 0;
    lineperBlock_ = 0;
    is_map_initialized_ = false;
    full_raw_width = 0;
}


// 实现 setReconstructionMap 函数
int ImgReconstruction::setReconstructionMap(const int* h_pixel_data_counts)
{
    if (!is_initialized_) {
        fprintf(stderr, "Error: Must call setup() before setReconstructionMap().\n");
        return -1;
    }
    if (!h_pixel_data_counts) {
        fprintf(stderr, "Error: Input h_pixel_data_counts cannot be null.\n");
        return -1;
    }

    try {
        // 在CPU上准备偏移量数组
        std::vector<int> h_pixel_data_offsets(full_raw_width);
        int current_offset = 0;
        for (int i = 0; i < full_raw_width; ++i) {
            h_pixel_data_offsets[i] = current_offset;
            current_offset += h_pixel_data_counts[i];
        }
        samples_per_line_ = current_offset;
        // 异步将 counts 和 offsets 数组复制到设备内存
        size_t pixel_map_size = static_cast<size_t>(full_raw_width) * sizeof(int);
        CUDA_CHECK(cudaMemcpyAsync(d_pixel_data_counts_, h_pixel_data_counts, pixel_map_size, cudaMemcpyHostToDevice, stream_));
        CUDA_CHECK(cudaMemcpyAsync(d_pixel_data_offsets_, h_pixel_data_offsets.data(), pixel_map_size, cudaMemcpyHostToDevice, stream_));

        // 同步流以确保映射数据上传完成
        CUDA_CHECK(cudaStreamSynchronize(stream_));

        is_map_initialized_ = true;
    }
    catch (const std::runtime_error& e) {
        fprintf(stderr, "Error during setReconstructionMap: %s\n", e.what());
        return -1;
    }
    return 0;
}

//__global__ void chunk_reconstruction_kernel(
//    uint16_t* d_image_chunk_data,    // 输出: 指向一个完整的 chunk 大小的设备内存
//    const uint8_t* d_raw_data,
//    const int* d_pixel_data_counts,   // 映射表：每个像素对应多少个采样点
//    const int* d_pixel_data_offsets,  // 映射表：每个像素的采样点起始偏移
//    int            width,                 // 图像宽度
//    int            lines_in_chunk,        // 这个 chunk 包含的行数 (即 lineperBlock_)
//    int            samples_per_line,      // 单行原始数据的总采样点数
//    int            channels               // 【新增参数】图像的通道数
//) {
//    int pixel_x = blockIdx.x; // 列号
//    int pixel_y = blockIdx.y; // 行号
//
//    int pixel_1D_index = pixel_y * width + pixel_x;
//
//    int channel_index = threadIdx.x;
//
//    if (pixel_x >= width || channel_index >= channels) {
//        return;
//    }
//
//    // --- 像素值计算逻辑 (与原版保持一致) ---
//    // 每个线程(x, y)仍然只计算一次像素值。
//    int num_samples = d_pixel_data_counts[pixel_1D_index];
//    long long sum = 0;
//    int16_t pixel_value = 0; // 最终要写入的像素值
//    const uint16_t  PEDESTAL_VALUE = 0x3FFF; // 16383
//
//    if (num_samples > 0) {
//
//        int intra_line_offset = d_pixel_data_offsets[pixel_1D_index];
//        int total_byte_count = d_pixel_data_counts[pixel_1D_index];
//
//        for (int i = 0; i < total_byte_count; i += 2 * channels) {
//            int byte_index = intra_line_offset + i + 2 * channel_index;
//            uint8_t byte1 = d_raw_data[byte_index];
//            uint8_t byte2 = d_raw_data[byte_index + 1];
//            uint16_t data_point = (static_cast<uint16_t>(byte2) << 8) | static_cast<uint16_t>(byte1);
//            data_point &= 0x3FFF;
//            uint16_t corrected_value = PEDESTAL_VALUE - data_point;
//            sum += data_point;
//        }
//        int samples_per_channel = total_byte_count / (2 * channels);
//        if (samples_per_channel > 0) {
//            pixel_value = static_cast<uint16_t>(sum / samples_per_channel);
//        }
//    }
//
//    // 计算正确的平面式布局输出地址
//    size_t channel_plane_size = static_cast<size_t>(width) * lines_in_chunk;
//    size_t dest_index = (static_cast<size_t>(channel_index) * channel_plane_size) + pixel_1D_index;
//
//    d_image_chunk_data[dest_index] = pixel_value;
//}

__global__ void chunk_reconstruction_kernel(
    uint16_t* d_image_chunk_data,
    const uint8_t* d_raw_data,
    const int* d_pixel_data_counts,
    const int* d_pixel_data_offsets,
    int            width,
    int            lines_in_chunk,
    int            samples_per_line,
    int            channels,
    unsigned int   line,
    int            phase
) {
    // 目标图像的列号 (逻辑上的，反转前)
    int dest_col_raw = blockIdx.x;
    // 当前 chunk 内的目标行号
    int dest_row = blockIdx.y;
    // 通道索引
    int channel_index = threadIdx.x;

    // Kernel 启动时已保证 dest_col_raw, dest_row, channel_index 在范围内，
    // 因此这里可以省略基本的边界检查，以追求极致性能。
    // 如果启动配置可能不精确，保留它是更安全的选择。
    // if (dest_col_raw >= width || ... ) return;

    // 1. 根据 phase 符号，计算源像素的列号
    int source_col;
    if (phase >= 0) {
        // 右移: output[i] <- raw[i]
        source_col = dest_col_raw;
    }
    else {
        // 左移: output[i] <- raw[i - phase]
        source_col = dest_col_raw - phase * 2;
    }

    // 根据问题的约束，这里的 source_col 永远不会越界，所以移除检查。

    // 2. 计算源像素在一维查找表中的索引
    int full_raw_width = width + 2 * abs(phase);
    int source_pixel_1D_index = dest_row * full_raw_width + source_col;

    // 3. 从查找表获取采样信息并计算像素值
    int num_samples = d_pixel_data_counts[source_pixel_1D_index];
    uint16_t pixel_magnitude = 0;

    if (num_samples > 0) {
        long long sum = 0;
        int intra_line_offset = d_pixel_data_offsets[source_pixel_1D_index];
        int total_byte_count = d_pixel_data_counts[source_pixel_1D_index];

        for (int i = 0; i < total_byte_count; i += 2 * channels) {
            int byte_index = intra_line_offset + i + 2 * channel_index;
            uint8_t byte1 = d_raw_data[byte_index];
            uint8_t byte2 = d_raw_data[byte_index + 1];
            uint16_t raw_value = (static_cast<uint16_t>(byte2) << 8) | static_cast<uint16_t>(byte1);
            int16_t signed_data_point = static_cast<int16_t>(raw_value);
            sum += abs(signed_data_point);
        }

        int samples_per_channel = total_byte_count / (2 * channels);
        if (samples_per_channel > 0) {
            pixel_magnitude = static_cast<uint16_t>(sum / samples_per_channel);
        }
    }

    // 4. 计算最终的目标写入位置 (包含行反转)
    int final_dest_col = dest_col_raw;
    int global_line_idx = line + dest_row;
    if (global_line_idx % 2 == 0) {
        final_dest_col = width - 1 - dest_col_raw;
    }

    int dest_pixel_1D_index = dest_row * width + final_dest_col;
    size_t channel_plane_size = static_cast<size_t>(width) * lines_in_chunk;
    size_t dest_index = (static_cast<size_t>(channel_index) * channel_plane_size) + dest_pixel_1D_index;

    // 5. 写入最终结果
    d_image_chunk_data[dest_index] = pixel_magnitude;
}

int ImgReconstruction::compute(const DataBlock* block, ImageBufferManager::Chunk* chunk, unsigned int line,int phase)
{
    if (!is_initialized_ || !block || block->used_size == 0) {
        return -1; // 未初始化或无有效数据
    }
    if (block->used_size > h_pinned_raw_data_size_) {
        fprintf(stderr, "Error: DataBlock size (%zu) exceeds allocated pinned buffer size (%zu).\n", block->used_size, h_pinned_raw_data_size_);
        return -1;
    }

    try {
        // 步骤 1: 将数据从 BufferPool 的常规内存块复制到锁页内存暂存区
        memcpy(h_pinned_raw_data_, block->data.get(), block->used_size);

        // 步骤 2: 异步将数据从锁页内存复制到设备内存
        CUDA_CHECK(cudaMemcpyAsync(d_raw_data_, h_pinned_raw_data_, block->used_size, cudaMemcpyHostToDevice, stream_));

        // 步骤 3: 启动 2D 核函数，直接在 d_image_data_ 中生成完整的 int16_t chunk
        {
            // 新的启动配置
            // 1. 每个块的线程数等于通道数
            dim3 threads_per_block(channels_);

            // 2. 网格的维度现在直接对应于图像块的像素维度
            //    总块数 = width_ * lineperBlock_
            dim3 blocks_in_grid(width_, lineperBlock_);

            chunk_reconstruction_kernel << <blocks_in_grid, threads_per_block, 0, stream_ >> > (
                reinterpret_cast<uint16_t*>(d_image_data_),
                reinterpret_cast<const uint8_t*>(d_raw_data_),
                d_pixel_data_counts_,
                d_pixel_data_offsets_,
                width_,
                lineperBlock_,
                samples_per_line_, // 假设 samples_per_line_ 是一个已正确计算的成员变量
                channels_,
                line,
                phase
                );
            //chunk_reconstruction_kernel << <blocks_in_grid, threads_per_block, 0, stream_ >> > (
            //    reinterpret_cast<int16_t*>(d_image_data_),
            //    reinterpret_cast<const uint8_t*>(d_raw_data_),
            //    d_pixel_data_counts_,
            //    d_pixel_data_offsets_,
            //    width_,
            //    lineperBlock_,
            //    samples_per_line_, // 假设 samples_per_line_ 是一个已正确计算的成员变量
            //    channels_
            //    );
        }

        // 步骤 4: 异步将结果从设备内存复制回主机内存
        CUDA_CHECK(cudaMemcpyAsync(chunk->pixel_data.data(), d_image_data_, d_image_data_size_, cudaMemcpyDeviceToHost, stream_));

        // 步骤 5: 同步流，确保所有操作完成。这使得 compute 函数对调用者是同步的。
        CUDA_CHECK(cudaStreamSynchronize(stream_));
    }
    catch (const std::runtime_error& e) {
        fprintf(stderr, "Error during CUDA computation: %s\n", e.what());
        return -1;
    }
    return 0;
}
