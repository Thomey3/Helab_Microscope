#include "ImageBufferManager.h"
#include <numeric>
#include <algorithm>
#include <chrono>

// --- Chunk 结构体实现 ---
ImageBufferManager::Chunk::Chunk(unsigned width, unsigned lines, unsigned bpp, unsigned channels)
    : lines_in_chunk(lines)
{
    const size_t total_chunk_size = static_cast<size_t>(width) * lines * bpp * channels;
    pixel_data.resize(total_chunk_size, 0);
}

// --- 单例实现 ---
ImageBufferManager& ImageBufferManager::get_instance() {
    static ImageBufferManager instance;
    return instance;
}

// --- 生命周期管理 ---
void ImageBufferManager::init(unsigned width, unsigned height, unsigned bpp, unsigned channels, unsigned numChunks)
{
    std::lock_guard<std::mutex> lock(life_cycle_mutex_);
    if (initialized_) {
        throw std::runtime_error("管理器已初始化。请先调用 shutdown()。");
    }

    width_ = width;
    height_ = height;
    bytesPerPixel_ = bpp;
    numChannels_ = channels;
    numChunks_ = numChunks > 0 ? numChunks : 1;

    if (width_ == 0 || height_ == 0 || bpp == 0 || channels == 0 || numChunks_ == 0 || height_ % numChunks_ != 0) {
        throw std::invalid_argument("图像参数无效，或总行数不能被分块数整除。");
    }
    linesPerChunk_ = height_ / numChunks_;

    const size_t total_image_size = static_cast<size_t>(width_) * height_ * bytesPerPixel_ * numChannels_;
    try {
        display_buffers_[0].assign(total_image_size, 0);
        display_buffers_[1].assign(total_image_size, 0);
    }
    catch (const std::bad_alloc& e) {
        shutdown();
        throw std::runtime_error("无法为双缓冲分配内存: " + std::string(e.what()));
    }

    chunk_pool_.clear();
    const unsigned pool_size = numChunks_ + 2;
    chunk_pool_.reserve(pool_size);
    for (unsigned i = 0; i < pool_size; ++i) {
        chunk_pool_.push_back(std::make_unique<Chunk>(width_, linesPerChunk_, bytesPerPixel_, numChannels_));
    }

    next_chunk_to_write_.store(0, std::memory_order_relaxed);
    front_buffer_idx_.store(0, std::memory_order_relaxed);
    full_frames_completed_.store(0, std::memory_order_relaxed);
    last_updated_row_.store(0, std::memory_order_relaxed);

    initialized_.store(true, std::memory_order_release);
}

void ImageBufferManager::shutdown()
{
    std::lock_guard<std::mutex> lock(life_cycle_mutex_);
    if (!initialized_) return;

    chunk_pool_.clear();
    display_buffers_[0].clear();
    display_buffers_[1].clear();

    width_ = height_ = bytesPerPixel_ = numChannels_ = numChunks_ = linesPerChunk_ = 0;
    next_chunk_to_write_.store(0);
    full_frames_completed_.store(0);
    last_updated_row_.store(0);
    front_buffer_idx_.store(0);

    initialized_.store(false, std::memory_order_release);
}

// --- 核心API实现 ---
ImageBufferManager::Chunk* ImageBufferManager::GetAvailableChunk()
{
    if (!initialized_.load(std::memory_order_acquire)) return nullptr;

    std::lock_guard<std::mutex> lock(pool_mutex_);
    if (chunk_pool_.empty()) return nullptr;

    std::unique_ptr<Chunk> chunk = std::move(chunk_pool_.back());
    chunk_pool_.pop_back();
    return chunk.release();
}

void ImageBufferManager::SubmitFilledChunk(Chunk* filled_chunk_raw)
{
    if (!initialized_.load(std::memory_order_acquire) || !filled_chunk_raw) {
        delete filled_chunk_raw;
        return;
    }

    std::unique_ptr<Chunk> filled_chunk(filled_chunk_raw);

    unsigned global_chunk_idx = next_chunk_to_write_.fetch_add(1, std::memory_order_acq_rel);
    unsigned slot_idx = global_chunk_idx % numChunks_;

    const int back_buffer_idx = 1 - front_buffer_idx_.load(std::memory_order_relaxed);

    // 对后台缓冲区的写入操作加锁
    {
        std::unique_lock<std::shared_mutex> lock(back_buffer_access_mutex_);

        auto& back_buffer = display_buffers_[back_buffer_idx];
        const auto& chunk = *filled_chunk;
        const unsigned first_line_in_image = slot_idx * linesPerChunk_;
        const size_t bytes_per_line = static_cast<size_t>(width_) * bytesPerPixel_;
        const size_t total_image_channel_size = static_cast<size_t>(width_) * height_ * bytesPerPixel_;

        // 【修改点】移除行反转逻辑，并优化拷贝方式
        // 数据按通道平面存储，因此我们可以为每个通道执行一次大块内存拷贝，
        // 而不是逐行进行拷贝，这样效率更高。
        const size_t chunk_channel_size = static_cast<size_t>(chunk.lines_in_chunk) * bytes_per_line;

        for (unsigned c = 0; c < numChannels_; ++c) {
            // 计算源数据中当前通道的起始地址
            const unsigned char* src_channel_data = chunk.pixel_data.data() + c * chunk_channel_size;

            // 计算目标缓冲区中对应通道和对应行的起始地址
            unsigned char* dest_channel_start = back_buffer.data() + c * total_image_channel_size + first_line_in_image * bytes_per_line;

            // 将整个chunk中当前通道的数据一次性拷贝过去
            memcpy(dest_channel_start, src_channel_data, chunk_channel_size);
        }
    } // 写锁在此释放

    returnChunkToPool(std::move(filled_chunk));

    const unsigned updated_rows = (slot_idx + 1) * linesPerChunk_;
    last_updated_row_.store(updated_rows, std::memory_order_release);

    if ((slot_idx + 1) == numChunks_) {
        {
            std::unique_lock<std::shared_mutex> lock(display_swap_mutex_);

            // 【新增代码】
            // 在交换索引前，将刚刚完成的后台缓冲区 (back_buffer_idx) 的内容
            // 拷贝到当前的前台缓冲区 (1 - back_buffer_idx) 中。
            // back_buffer_idx 是这一帧写入的缓冲区的索引。
            // 另一个缓冲区 (1 - back_buffer_idx) 此时是旧的前台缓冲区。
            display_buffers_[1 - back_buffer_idx] = display_buffers_[back_buffer_idx];

            // 交换索引，让 front_buffer_idx_ 指向新帧
            front_buffer_idx_.store(back_buffer_idx, std::memory_order_release);
        } // 独占锁在此释放

        std::lock_guard<std::mutex> lock(frame_mutex_);
        full_frames_completed_.fetch_add(1, std::memory_order_relaxed);
        frame_cv_.notify_all();
    }
}

void ImageBufferManager::GetImageCopy(std::vector<unsigned char>& destination) const
{
    destination.clear();
    if (!initialized_.load(std::memory_order_acquire)) return;

    int current_front_idx = 0;
    {
        std::shared_lock<std::shared_mutex> lock(display_swap_mutex_);
        current_front_idx = front_buffer_idx_.load(std::memory_order_acquire);

        // 为防止在拷贝前台缓冲时，它恰好又被清空（ResetImage），
        // 我们需要确保拷贝和读取索引在同一个锁作用域内。
        destination = display_buffers_[current_front_idx];
    }
}

// 安全地实现实时预览函数
void ImageBufferManager::GetLiveImageCopyWithTearing(std::vector<unsigned char>& destination) const
{
    destination.clear();
    if (!initialized_.load(std::memory_order_acquire)) return;

    const int back_buffer_idx = 1 - front_buffer_idx_.load(std::memory_order_acquire);

    // 获取对后台缓冲区的共享读锁，以和生产者的写锁同步
    std::shared_lock<std::shared_mutex> lock(back_buffer_access_mutex_);
    destination = display_buffers_[back_buffer_idx];
}


bool ImageBufferManager::GetSnapshot(std::vector<unsigned char>& destination, int timeout_ms)
{
    destination.clear();
    if (!initialized_.load(std::memory_order_acquire)) return false;

    std::unique_lock<std::mutex> lock(frame_mutex_);
    const auto last_known_frame = full_frames_completed_.load(std::memory_order_relaxed);

    bool success;
    if (timeout_ms < 0) {
        frame_cv_.wait(lock, [&] { return full_frames_completed_.load(std::memory_order_relaxed) > last_known_frame; });
        success = true;
    }
    else {
        success = frame_cv_.wait_for(lock, std::chrono::milliseconds(timeout_ms), [&] {
            return full_frames_completed_.load(std::memory_order_relaxed) > last_known_frame;
            });
    }

    if (success) {
        lock.unlock();
        GetImageCopy(destination);
    }

    return success;
}

void ImageBufferManager::ResetImage()
{
    if (!initialized_.load(std::memory_order_acquire)) return;

    std::unique_lock<std::shared_mutex> swap_lock(display_swap_mutex_);
    std::unique_lock<std::shared_mutex> back_lock(back_buffer_access_mutex_);
    std::lock_guard<std::mutex> frame_lock(frame_mutex_);

    std::fill(display_buffers_[0].begin(), display_buffers_[0].end(), 0);
    std::fill(display_buffers_[1].begin(), display_buffers_[1].end(), 0);

    next_chunk_to_write_.store(0, std::memory_order_relaxed);
    last_updated_row_.store(0, std::memory_order_relaxed);
    full_frames_completed_.store(0, std::memory_order_relaxed);
    front_buffer_idx_.store(0, std::memory_order_relaxed);
}

void ImageBufferManager::returnChunkToPool(std::unique_ptr<Chunk> chunk)
{
    if (!chunk) return;
    std::lock_guard<std::mutex> lock(pool_mutex_);
    chunk_pool_.push_back(std::move(chunk));
}

bool ImageBufferManager::IsInitialized() const
{
    return initialized_.load(std::memory_order_acquire);
}

uint64_t ImageBufferManager::GetCompletedFrameCount() const
{
    return full_frames_completed_.load(std::memory_order_relaxed);
}

unsigned ImageBufferManager::GetLastUpdatedRow() const
{
    return last_updated_row_.load(std::memory_order_acquire);
}
