#ifndef IMGRECONSTRUCTION_H
#define IMGRECONSTRUCTION_H

#include "cuda_runtime.h"
#include "DataStream.h"
#include "ImageBufferManagerAPI.h"
#include <cstdint> // For uint8_t, int16_t
#include "DEBUG.h"

class ImgReconstruction
{
public:
    inline static ImgReconstruction& getInstance()
    {
        static ImgReconstruction instance;
        return instance;
    }

    // 一次性设置和分配所有必需的内存
    int setup(size_t max_raw_data_size, int width, int height, int channels, int lineperBlock, int phase);

    int setReconstructionMap(const int* h_pixel_data_counts); 

    // 核心计算函数
    int compute(const DataBlock* block, ImageBufferManager::Chunk* chunk, unsigned int line,int phase);

private:
    ImgReconstruction();
    ~ImgReconstruction();
    // 禁止拷贝，因为我们手动管理资源
    ImgReconstruction(const ImgReconstruction&) = delete;
    ImgReconstruction& operator=(const ImgReconstruction&) = delete;

    void teardown(); // 内部清理函数

    // 主机端内存
    uint8_t* h_pinned_raw_data_; // 锁页内存暂存区
    size_t   h_pinned_raw_data_size_;

    // 设备端内存
    uint8_t* d_raw_data_;
    uint16_t* d_image_data_;
    size_t   d_image_data_size_;

    // CUDA 流
    cudaStream_t stream_;

    // 配置参数
    bool is_initialized_;
    int  width_;
    int  height_;
    int  channels_;
    int  lineperBlock_; // 每个块的行数，可能用于分块处理
    int* d_pixel_data_counts_;
    int* d_pixel_data_offsets_;
    bool is_map_initialized_; // 新增状态标志
    int samples_per_line_;
    int full_raw_width;
};

#endif // IMGRECONSTRUCTION_H
