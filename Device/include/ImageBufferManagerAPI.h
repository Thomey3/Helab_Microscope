#pragma once
#include "ImageBufferManager.h"
#include <vector>

// ======================================================================
//          ImageBufferManager C-Style Singleton API
//
// 使用这套API与图像缓冲管理器交互。所有函数均为线程安全。
// ======================================================================


// --- 生命周期管理 API ---

/**
 * @brief 一次性初始化图像缓冲管理器。
 * 必须在调用任何其他API函数之前调用。
 *
 * @param width 图像宽度。
 * @param height 图像高度。
 * @param bytesPerPixel 每像素每通道字节数。
 * @param numChannels 通道数。
 * @param numChunks 图像垂直方向的分块数量。
 */
inline void ib_init(unsigned width, unsigned height, unsigned bytesPerPixel, unsigned numChannels, unsigned numChunks) {
    ImageBufferManager::get_instance().init(width, height, bytesPerPixel, numChannels, numChunks);
}

/**
 * @brief 关闭并释放管理器占用的所有资源。
 */
inline void ib_shutdown() {
    ImageBufferManager::get_instance().shutdown();
}

/**
 * @brief 重新配置管理器。
 * @warning 这不是一个线程安全的操作！在调用此函数前，应用层必须确保所有
 *          正在使用管理器的生产者/消费者线程都已停止，否则行为未定义。
 *
 * @return 总是返回true（除非init内部抛出异常）。
 */
inline bool ib_reconfigure(unsigned width, unsigned height, unsigned bytesPerPixel, unsigned numChannels, unsigned numChunks) {
    auto& manager = ImageBufferManager::get_instance();
    manager.shutdown();
    manager.init(width, height, bytesPerPixel, numChannels, numChunks);
    return true;
}


// --- 生产者 API ---

/**
 * @brief 从池中获取一个可供写入的空闲数据块(Chunk)。
 * @details 调用者获得该数据块的【原始指针所有权】，并有责任在填充数据后，
 *          通过 `ib_submit_chunk()` 将其归还给管理器。
 *          如果忘记归还，将导致内存泄漏！
 * @return 指向数据块的原始指针，如果池已空则为 nullptr。
 */
inline ImageBufferManager::Chunk* ib_acquire_chunk() {
    return ImageBufferManager::get_instance().GetAvailableChunk();
}

/**
 * @brief 提交一个已填充完毕的数据块。
 * @details 管理器会收回指针的所有权，并负责其后续的生命周期。
 *          调用此函数后，不应再使用传入的指针。
 * @param chunk 一个指向已填充数据块的原始指针。
 */
inline void ib_submit_chunk(ImageBufferManager::Chunk* chunk) {
    ImageBufferManager::get_instance().SubmitFilledChunk(chunk);
}

// --- 消费者 API ---

/**
 * @brief (实时流) 获取当前图像的最新完整深拷贝，立即返回。
 */
inline void ib_get_image_copy(std::vector<unsigned char>& destination) {
    ImageBufferManager::get_instance().GetImageCopy(destination);
}

inline void ib_get_image_copy_with_tearing(std::vector<unsigned char>& destination) {
    return ImageBufferManager::get_instance().GetLiveImageCopyWithTearing(destination);
}

/**
 * @brief (按需快照) 等待一个新帧完成后，获取其完整快照。
 * @param destination 用于存放图像数据的目标vector。
 * @param timeout_ms 等待的超时时间（毫秒）。如果为负数，则无限等待。
 * @return 如果成功获取到快照则返回true，如果超时则返回false。
 */
inline bool ib_get_snapshot(std::vector<unsigned char>& destination, int timeout_ms = -1) {
    return ImageBufferManager::get_instance().GetSnapshot(destination, timeout_ms);
}

/**
 * @brief 将当前显示的图像数据重置为零（黑色图像）。
 * @details 这是一个线程安全的操作。调用后，消费者获取到的图像将是黑色的，
 *          直到生产者提交新的数据块来重新填充图像。
 */
inline void ib_reset_image() {
    ImageBufferManager::get_instance().ResetImage();
}

/**
 * @brief 检查管理器是否已经被初始化。
 * @return 如果 init() 已被成功调用且 shutdown() 尚未被调用，则返回 true。
 */
inline bool ib_is_initialized() {
    return ImageBufferManager::get_instance().IsInitialized();
}

/**
 * @brief 获取自初始化或上次重置以来已完成的完整图像帧数。
 * @return 已完成的帧数。
 */
inline uint64_t ib_get_completed_frame_count() {
    return ImageBufferManager::get_instance().GetCompletedFrameCount();
}

/**
 * @brief 获取当前图像缓冲区已更新到的行数。
 * @details 返回值 N 表示已有 N 行数据被写入。当一帧完成时，返回总高度。
 * @return 已更新的行数。
 */
inline unsigned ib_get_last_updated_row() {
    return ImageBufferManager::get_instance().GetLastUpdatedRow();
}

