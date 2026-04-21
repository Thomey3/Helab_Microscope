#include <QT12136DC.h>
#include <chrono>
#include <string> // 用于 std::to_string

int QT12136DCThread::start()
{
	if (Busy()) return DEVICE_ERR;
	isBusy_ = true;
	m_stop = false;
	activate();
	return DEVICE_OK;
}

int QT12136DCThread::svc()
{
	int coreAffinity_ = 2;
	// --- 在线程开始时设置优先级和亲和性 ---
	HANDLE hCurrentThread = GetCurrentThread();

	// 设置为高优先级
	if (!SetThreadPriority(hCurrentThread, THREAD_PRIORITY_TIME_CRITICAL)) {
		// 建议在这里添加日志，记录设置失败的警告
		m_QT->LogMessage("Warning: Failed to set thread priority.", false);
	}

	// 如果指定了CPU核心，则进行绑定
	if (coreAffinity_ >= 0) {
		DWORD_PTR affinityMask = (static_cast<DWORD_PTR>(1) << coreAffinity_);
		if (!SetThreadAffinityMask(hCurrentThread, affinityMask)) {
			// 建议在这里添加日志，记录设置失败的警告
			m_QT->LogMessage("Warning: Failed to set thread affinity.", false);
		}
	}
	m_thread_running = true;
	uint32_t buffersize = m_QT->m_CollectSettings.buffersize;
	unsigned char* data = nullptr;
	DataBlock* writable_block = nullptr;
    //int count = 0;
	// In constructor or init function
	m_last_log_time = std::chrono::high_resolution_clock::now();
    while (1) {
        if (m_stop) {
            m_QT->LogMessage("QTstop", true);
            break;
        }
        //m_QT->LogMessage("QT:" + std::to_string(count), true);
        //count++;
        auto t1 = std::chrono::high_resolution_clock::now();
        writable_block = bp_acquire_write();//获得一个内存块
        auto t2 = std::chrono::high_resolution_clock::now();

        if (writable_block == nullptr) {
            m_QT->LogMessage("QT12136DCThread failed to acquire writable block.", true);
            return -1;  // 获取内存失败return -1; 
        }
        data = reinterpret_cast<unsigned char*>(writable_block->data.get());

        writable_block->used_size = QT_BoardGetFIFODataBuffer(unCardIndex, data, buffersize, 0xffffffffffffffff);
        auto t3 = std::chrono::high_resolution_clock::now();

        // ... 错误处理 ...
        if (writable_block->used_size < 0) {
            bp_release(writable_block); // 在返回前释放内存块
            return DEVICE_ERR;
        }
        else if (writable_block->used_size > buffersize) {
            int a = 0;
        }

        bp_push_filled(writable_block);
        auto t4 = std::chrono::high_resolution_clock::now();

        // --- 累加耗时 ---
        long long acquire_us = std::chrono::duration_cast<std::chrono::microseconds>(t2 - t1).count();
        long long fill_us = std::chrono::duration_cast<std::chrono::microseconds>(t3 - t2).count();
        long long push_us = std::chrono::duration_cast<std::chrono::microseconds>(t4 - t3).count();

        m_acquire_total_us += acquire_us;
        m_fill_total_us += fill_us;
        m_push_total_us += push_us;
        m_loop_count++;

        m_used_size_total += writable_block->used_size;
        // 更新最大填充耗时
        if (fill_us > m_fill_max_us.load()) {
            m_fill_max_us = fill_us;
        }

        // --- 定期（例如每秒）打印一次统计日志 ---
        const auto log_interval = std::chrono::milliseconds(100);
        if (t4 - m_last_log_time > log_interval) {
            long long count = m_loop_count.load();
            if (count > 0) {
                long long avg_acquire = m_acquire_total_us.load() / count;
                long long avg_fill = m_fill_total_us.load() / count;
                long long avg_push = m_push_total_us.load() / count;
                long long avg_used_size = m_used_size_total.load() / count;
                std::string log_msg = "Perf stats (last " + std::to_string(std::chrono::duration_cast<std::chrono::milliseconds>(t4 - m_last_log_time).count()) + "ms, " +
                    std::to_string(count) + " loops): " +
                    "Avg Acquire=" + std::to_string(avg_acquire) + "us, " +
                    "Avg Fill=" + std::to_string(avg_fill) + "us (Max=" + std::to_string(m_fill_max_us.load()) + "us), " +
                    "Avg Push=" + std::to_string(avg_push) + "us." +
                    "Avg Size=" + std::to_string(avg_used_size) + "B.";
                //m_QT->LogMessage(log_msg, true);
            }

            // 重置统计数据
            m_acquire_total_us = 0;
            m_fill_total_us = 0;
            m_push_total_us = 0;
            m_loop_count = 0;
            m_fill_max_us = 0;
            m_last_log_time = t4;
            m_used_size_total = 0; 
        }
    }
    m_QT->LogMessage("QT12136DC Thread stop.", true);
	isBusy_ = false;
	m_thread_running = false;
	return DEVICE_OK;
}