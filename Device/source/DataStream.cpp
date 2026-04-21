#include "DataStream.h"

namespace {
    std::once_flag g_bp_flag;  // 保证仅初始化一次
    std::unique_ptr<BufferPool> g_bp_ptr;
}

BufferPool& BufferPool::instance(size_t num_blocks, size_t block_size) {
    std::call_once(g_bp_flag, [=] {
        if (num_blocks == 0 || block_size == 0) {
            throw std::runtime_error("BufferPool must be initialized with valid params the first time.");
        }
        g_bp_ptr = std::make_unique<BufferPool>(num_blocks, block_size);
        });
    return *g_bp_ptr;
}

// ------------------- ctor/dtor -------------------
BufferPool::BufferPool(size_t num_blocks, size_t block_size) {
    m_all_blocks.reserve(num_blocks);
    for (size_t i = 0; i < num_blocks; ++i) {
        auto blk = std::make_unique<DataBlock>();
        blk->data = std::make_unique<uint8_t[]>(block_size);
        blk->capacity = block_size;
        m_free_queue.push(blk.get());
        m_all_blocks.emplace_back(std::move(blk));
    }
}

BufferPool::~BufferPool() { shutdown(); }

// ------------------- public API -------------------
DataBlock* BufferPool::get_writable_block() {
    std::unique_lock<std::mutex> lk(m_mutex);
    m_cv_free.wait(lk, [&] { return !m_free_queue.empty() || m_shutdown; });
    if (m_shutdown) return nullptr;
    auto* blk = m_free_queue.front(); m_free_queue.pop();
    return blk;
}

void BufferPool::post_filled_block(DataBlock* blk) {
    {
        std::lock_guard<std::mutex> lk(m_mutex);
        if (m_shutdown) return;
        m_filled_queue.push(blk);
    }
    m_cv_filled.notify_one();
}

DataBlock* BufferPool::get_filled_block() {
    std::unique_lock<std::mutex> lk(m_mutex);
    m_cv_filled.wait(lk, [&] { return !m_filled_queue.empty() || m_shutdown; });
    if (m_shutdown) return nullptr;
    auto* blk = m_filled_queue.front(); m_filled_queue.pop();
    return blk;
}

void BufferPool::release_block(DataBlock* blk) {
    {
        std::lock_guard<std::mutex> lk(m_mutex);
        if (m_shutdown) return;
        blk->used_size = 0;
        m_free_queue.push(blk);
    }
    m_cv_free.notify_one();
}

bool BufferPool::reconfigure(size_t num_blocks, size_t block_size) {
    std::lock_guard<std::mutex> lk(m_mutex);
    // 只允许在已 shutdown 后重置
    if (!m_shutdown || num_blocks == 0 || block_size == 0) {
        return false;
    }

    // —— 清理旧资源 —— 
    m_all_blocks.clear();
    std::queue<DataBlock*>().swap(m_free_queue);
    std::queue<DataBlock*>().swap(m_filled_queue);

    // —— 重置状态 —— 
    m_shutdown = false;

    // —— 重新分配 —— 
    m_all_blocks.reserve(num_blocks);
    for (size_t i = 0; i < num_blocks; ++i) {
        auto blk = std::make_unique<DataBlock>();
        blk->data = std::make_unique<uint8_t[]>(block_size);
        blk->capacity = block_size;
        blk->used_size = 0;
        m_free_queue.push(blk.get());
        m_all_blocks.push_back(std::move(blk));
    }
    return true;
}

void BufferPool::shutdown() {
    {
        std::lock_guard<std::mutex> lk(m_mutex);
        if (m_shutdown) return;
        m_shutdown = true;
    }
    m_cv_free.notify_all();
    m_cv_filled.notify_all();
}