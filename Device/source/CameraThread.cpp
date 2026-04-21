#include "_920TwophotonCamera.h"
#include <chrono>   // 包含计时所需的头文件

int CameraThread::start(uint32_t phase)
{
	if (IsRunning())
		return DEVICE_ERR;

	m_stop = false;
	m_phase = phase;
	activate();
	return DEVICE_OK;
}

int CameraThread::svc()
{
	m_running = true; // 线程开始，设置运行标志

	DataBlock* readable_block = nullptr;
	ImageBufferManager::Chunk* current_chunk = nullptr;
	unsigned int line = 0;
	while (!m_stop.load())
	{
		//cam_->LogMessage("compute" + to_string(line),true);
		//1. 从缓冲池获取一个填满数据的可读块
		//获取可读缓冲区
		readable_block = bp_acquire_read();
		if (readable_block == nullptr) {
			// 如果缓冲池被关闭，这里会返回 nullptr
			break;
		}
		current_chunk = ib_acquire_chunk();
		if (current_chunk == nullptr) {
			// 如果获取失败，必须释放之前获取的 readable_block 以防泄漏
			bp_release(readable_block);

			// 短暂休眠，避免在池满时空转浪费CPU
			std::this_thread::sleep_for(std::chrono::milliseconds(1));
			continue;
		}
		// 2. 将数据块传递给重建模块进行处理
		//    该模块内部会处理到锁页内存的拷贝和GPU传输
		IR::compute(readable_block, current_chunk, line, m_phase);

		line += cam_->m_LinePerAcq;
		if (line >= cam_->m_ImgSetting.resolution) line = 0;

		// 3. 处理完毕后，必须将数据块释放回缓冲池
		bp_release(readable_block);
		ib_submit_chunk(current_chunk);
	}
	cam_->LogMessage("CameraThread stop.", true);
	m_running = false; // 线程即将退出，清除运行标志
	return DEVICE_OK;
}