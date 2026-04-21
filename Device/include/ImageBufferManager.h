#pragma once

#include <vector>
#include <mutex>
#include <shared_mutex>
#include <atomic>
#include <memory>
#include <stdexcept>
#include <functional>
#include <condition_variable>

/**
 * @class ImageBufferManager
 * @brief 高性能、线程安全的双缓冲图像管理器。
 *
 * 该类为生产者（重建线程）和消费者（显示线程）提供了无撕裂的图像数据交换机制。
 * 它同时提供了两种图像获取方式：一种是获取无撕裂的完整帧，另一种是获取可实时
 * 看到更新但可能带有撕裂的预览图像。
 */
class ImageBufferManager
{
public:
    // --- 单例访问 ---
    static ImageBufferManager& get_instance();

    ImageBufferManager(const ImageBufferManager&) = delete;
    ImageBufferManager& operator=(const ImageBufferManager&) = delete;

    struct Chunk
    {
        std::vector<unsigned char> pixel_data;
        unsigned lines_in_chunk;
        Chunk(unsigned width, unsigned lines, unsigned bpp, unsigned channels);
    };

    // --- 生命周期管理 ---
    void init(unsigned width, unsigned height, unsigned bytesPerPixel, unsigned numChannels, unsigned numChunks);
    void shutdown();

    // --- 核心API方法 ---
    Chunk* GetAvailableChunk();
    void SubmitFilledChunk(Chunk* filled_chunk);

    /**
     * @brief 无撕裂地获取当前最新的完整图像帧。
     * @details 此函数总是从已完成的前台缓冲区拷贝数据，保证了图像的完整性。
     * @param destination 用于存放图像数据的目标vector。
     */
    void GetImageCopy(std::vector<unsigned char>& destination) const;

    /**
     * @brief 【新增】获取带有撕裂的实时图像拷贝，用于高实时性预览。
     * @details 此函数直接从正在被生产者写入的后台缓冲区拷贝数据。
     * 这提供了最高的实时性，可以看到逐块更新的效果，但代价是获取的图像
     * 可能会有撕裂（即图像的一部分是新帧，一部分是旧帧）。
     * 此操作是线程安全的，不会导致数据损坏。
     * @param destination 用于存放图像数据的目标vector。
     */
    void GetLiveImageCopyWithTearing(std::vector<unsigned char>& destination) const;

    void ResetImage();
    bool GetSnapshot(std::vector<unsigned char>& destination, int timeout_ms = -1);

    // --- 工具函数 ---
    unsigned GetLastUpdatedRow() const;
    unsigned GetWidth() const { return width_; }
    unsigned GetHeight() const { return height_; }
    unsigned GetNumChunks() const { return numChunks_; }
    uint64_t GetCompletedFrameCount() const;
    bool IsInitialized() const;

private:
    ImageBufferManager() = default;
    ~ImageBufferManager() = default;

    void returnChunkToPool(std::unique_ptr<Chunk> chunk);

    // --- 状态与锁 ---
    std::mutex life_cycle_mutex_;
    std::atomic<bool> initialized_{ false };

    mutable std::mutex frame_mutex_;
    mutable std::condition_variable frame_cv_;
    std::atomic<uint64_t> full_frames_completed_{ 0 };

    // --- 图像属性 ---
    unsigned width_ = 0, height_ = 0, bytesPerPixel_ = 0, numChannels_ = 0, numChunks_ = 0, linesPerChunk_ = 0;

    // --- Chunk池 ---
    std::vector<std::unique_ptr<Chunk>> chunk_pool_;
    std::mutex pool_mutex_;

    // --- 内部状态 ---
    std::atomic<unsigned> next_chunk_to_write_{ 0 };
    std::atomic<unsigned> last_updated_row_{ 0 };

    // --- 【核心修改】双缓冲机制 ---
    std::vector<unsigned char> display_buffers_[2];
    std::atomic<int> front_buffer_idx_{ 0 };

    // 用于保护“缓冲区交换”这个瞬时操作
    mutable std::shared_mutex display_swap_mutex_;

    // 【新增】用于保护对“后台缓冲区”的并发读写
    mutable std::shared_mutex back_buffer_access_mutex_;
};
