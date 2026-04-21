#pragma once
#include <vector>
#include <queue>
#include <mutex>
#include <memory>
#include <condition_variable>
#include <atomic>
#include <cstdint>

struct DataBlock
{
    std::unique_ptr<uint8_t[]> data;
    size_t capacity;
    size_t used_size = 0;
};

class BufferPool final
{
public:
    // -------- Singleton 入口 ----------
    static BufferPool& instance(size_t num_blocks = 0, size_t block_size = 0);

    // -------- 构造/析构 ------------------
    BufferPool(size_t num_blocks, size_t block_size);
    ~BufferPool();

    // -------- 核心业务 API ----------
    DataBlock* get_writable_block();
    void       post_filled_block(DataBlock* blk);
    DataBlock* get_filled_block();
    void       release_block(DataBlock* blk);

    // 主动关闭 / 重置大小
    bool reconfigure(size_t num_blocks, size_t block_size);
    void shutdown();

private:
    // 禁止外部构造 / 拷贝
    BufferPool(const BufferPool&) = delete;
    BufferPool& operator=(const BufferPool&) = delete;

    // 内部资源
    std::atomic<bool>                 m_shutdown{ false };
    std::vector<std::unique_ptr<DataBlock>> m_all_blocks;
    std::queue<DataBlock*>            m_free_queue;
    std::queue<DataBlock*>            m_filled_queue;
    std::mutex                        m_mutex;
    std::condition_variable           m_cv_free;
    std::condition_variable           m_cv_filled;
};