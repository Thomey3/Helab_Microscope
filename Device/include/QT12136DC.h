#pragma once
#include "DeviceBase.h"
#include "QT_Board.h"
#include "QT_Board_Project.h"
#include "QTSetFunction.h"
#include "QT12136DC_datatype.h"
#include "BufferAPI.h"
#include <string>
#include "memory"
#include <chrono>
#define Rate 1000000000 // 采样率为1GHz
extern const char* g_DeviceNameQT12136DC;
extern const char* const g_AcqModeStrings[AcquisitionMode::ACQUISITION_MODE_COUNT];
extern const char* const g_DataTypeStrings[DataType::DATATYPE_COUNT];
extern const char* const g_ClockModeStrings[ClockMode::CLOCK_MODE_COUNT];
extern const char* const g_TriggerModeStrings[TriggerMode::TRIGGER_MODE_COUNT];
extern unsigned int unCardIndex; // 全局变量，表示板卡索引
class QT12136DCThread;

struct CardSettings
{
	int AcquisitionMode;	// 采集模式
	int DMABlockNum;		// DMA块数
	int DataType;			// 数据类型
	int extractMultiple;	// 提取倍数(0~65536)
	int ChannelNumber;		// 通道数量
	int ChannelMask;		// 通道掩码
	int ClockMode;			// 时钟模式
	int TriggerMode;		// 触发模式
};
struct TriggerSettings
{
	uint32_t trigDelay;		// 延迟触发长度
	double biasVoltage;		// 偏置电压
};
struct CollectSetting
{
	uint32_t Frameswitch; // 帧头开关
	uint32_t Pretrigdots; // 预触发点数
	double RepetitionFrequency; // 重复频率
	uint32_t Segment; // 段长（单位：字节）( 64 的 倍数 )
	uint32_t buffersize;	// 缓冲区大小（单位：字节）( Segment的整数倍，64的倍数 )
};
struct bufferConfig
{
	size_t num_blocks;
	size_t block_size;
};

class QT12136DC : public CGenericBase<QT12136DC>
{
public:
	QT12136DC();
	~QT12136DC();

	int Initialize();
	int Shutdown();
	void GetName(char* name) const;
	bool Busy();

	int start();
	int wait();
	int stop();
	int signalStop(); // Only sets the stop flag for the thread
	int hardwareShutdown(); // Shuts down the hardware, must be called after thread has stopped


	inline int SetCardSettings(CardSettings& settings)
	{
		m_CardSettings = settings;
		return DEVICE_OK(); 
	}
	inline int SetTriggerSettings(TriggerSettings& settings)
	{
		m_TriggerSettings = settings;
		return DEVICE_OK();
	}
	inline int SetCollectSetting(CollectSetting& settings)
	{
		m_CollectSettings = settings;
		return DEVICE_OK();
	}
	inline int SetBufferConfig(bufferConfig& config)
	{
		m_bufferConfig = config;
		return DEVICE_OK();
	}

	inline uint32_t Cal_Segment(double duration_s) const
	{
		if (duration_s <= 0) return DEVICE_ERR;
		uint32_t segment = (uint64_t)4 * Rate * 2 * duration_s; // 计算段长，单位为字节
		segment /= 2;
		//segment = segment / m_CardSettings.ChannelNumber / 64 * 64 * m_CardSettings.ChannelNumber; // 确保是64的倍数
		segment = segment / 4 / 64 * 64 * 4;
		return segment;
	}
private:
	// ------- QT12136DC API -------
	int QT_Init();
	int QT_Reset();
	// ------- MMCallback API -------
	int OnAcquisitionMode(MM::PropertyBase* pProp, MM::ActionType eAct);
	int OnDataType(MM::PropertyBase* pProp, MM::ActionType eAct);
	int OnExtractMultiple(MM::PropertyBase* pProp, MM::ActionType eAct);
	//int OnChannelNumber(MM::PropertyBase* pProp, MM::ActionType eAct);
	//int OnChannelMask(MM::PropertyBase* pProp, MM::ActionType eAct);
	int OnClockMode(MM::PropertyBase* pProp, MM::ActionType eAct);
	int OnTriggerMode(MM::PropertyBase* pProp, MM::ActionType eAct);
	int OnTriggerDelay(MM::PropertyBase* pProp, MM::ActionType eAct);
	int OnTriggerBiasVoltage(MM::PropertyBase* pProp, MM::ActionType eAct);

	bool m_Initialized;
	std::atomic<bool> m_Busy; // 建议改为 std::atomic<bool>
	std::mutex m_mutex;       // 添加互斥锁


	CardSettings m_CardSettings;
	TriggerSettings m_TriggerSettings;
	CollectSetting m_CollectSettings;
	bufferConfig m_bufferConfig;

	friend class QT12136DCThread; // 线程类友元，允许访问私有成员
	QT12136DCThread* m_thread; // 线程指针
};


class QT12136DCThread : public MMDeviceThreadBase
{
public:
	explicit QT12136DCThread(QT12136DC* qt)
		: m_QT(qt), isBusy_(false), m_stop(false)
	{
	}
	inline ~QT12136DCThread()
	{
	}

	int svc() override;
	int start();
	inline bool is_running() {
		return m_thread_running;
	}
	void stop()
	{
		m_stop = true;
	}
	bool Busy() const
	{
		return isBusy_;
	}

private:
	QT12136DC* m_QT;
	std::atomic<bool> isBusy_;
	std::atomic<bool> m_stop;
	std::atomic<bool> m_thread_running{ false };

	// 用于统计的成员变量
	std::atomic<long long> m_acquire_total_us{ 0 };
	std::atomic<long long> m_fill_total_us{ 0 };
	std::atomic<long long> m_push_total_us{ 0 };
	std::atomic<long long> m_loop_count{ 0 };

	// 用于记录最大耗时，帮助发现性能抖动
	std::atomic<long long> m_fill_max_us{ 0 };

	// 上次记录日志的时间点
	std::chrono::high_resolution_clock::time_point m_last_log_time;

	// 用于累加 used_size 的变量
	std::atomic<long long> m_used_size_total{ 0 };
};