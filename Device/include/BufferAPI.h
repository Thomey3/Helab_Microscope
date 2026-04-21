#pragma once
#include "DataStream.h"

// 提前一次性初始化；可放在 main 或框架启动钩子
inline void bp_init(size_t num_blocks, size_t block_size) {
    BufferPool::instance(num_blocks, block_size);  // 第一次调用带参数
}

// 以下调用均线程安全、**不关心**初始化细节
inline DataBlock* bp_acquire_write() { return BufferPool::instance().get_writable_block(); }
inline void       bp_push_filled(DataBlock* b) { BufferPool::instance().post_filled_block(b); }
inline DataBlock* bp_acquire_read() { return BufferPool::instance().get_filled_block(); }
inline void       bp_release(DataBlock* b) { BufferPool::instance().release_block(b); }

// 主动关闭 / 重置大小
inline void bp_shutdown() { BufferPool::instance().shutdown(); }
inline bool bp_reconfigure(size_t num_blocks, size_t block_size) {
    BufferPool& pool = BufferPool::instance();
    pool.shutdown();                    // 先通知所有线程退出
    return pool.reconfigure(num_blocks, block_size);
}