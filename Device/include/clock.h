#pragma once
#include "DeviceBase.h"
#include "NIDAQmx.h"
#include <string>
#include <unordered_map>

//#define DEBUG

using namespace std;

extern const char* g_DeviceNameClock;

class ClockTask;


class Clock : public CGenericBase<Clock>
{
	friend ClockTask;
public:
	Clock();
	~Clock();

	int Initialize();
	int Shutdown();

	void GetName(char* name) const;
	bool Busy();
	enum TimebaseSource {
		AUTO,           // 驱动自动选择（默认行为）
		HZ_100_MHZ,
		HZ_20_MHZ,
		HZ_100_KHZ
	};
	struct TaskProperty
	{
		string TaskName;
		string niDeviceName;
		string clockChannel;		// e.g /ctr0
		string TrigChannel = "";
		float64 frequency;
		float64 dutyCycle;
		uInt64 pulsesPerRun;
		TimebaseSource timebaseSrc = AUTO;
	};

	int addTask(const TaskProperty& props);
	int resetTask(const std::string& taskName, const TaskProperty& props);
	int removeTask(const std::string& taskName);
	ClockTask* getTask(const std::string& taskName);
private:

	string m_niDeviceName;
	std::unordered_map<std::string, ClockTask> m_ClockTask;
};

class ClockTask
{
	friend Clock;
public:
	/*
	@brief: create a CO output task.
	@param: niDeviceName -- ni Device e.g "/Dev1"
			clockChannel -- output Channel e.g "/ctr0"
			TrigChannel -- start trigger Channel e.g "/PFI0"
	@ret: NULL
	@birth: created by OYJJ on 20250610
	*/
	ClockTask(
		Clock* clk,
		const string niDeviceName,
		const string TaskName,
		const string clockChannel,
		const string TrigChannel,
		const float64 frequency,
		const float64 dutyCycle,
		const uInt64 pulsePerRun,
		Clock::TimebaseSource timebaseSrc);
	// 析构函数，用于自动释放资源 (RAII)
	~ClockTask();
	// "Rule of Five"：禁止拷贝，因为 TaskHandle 是独占资源
	ClockTask(const ClockTask&) = delete;
	ClockTask& operator=(const ClockTask&) = delete;
	// 可以实现移动语义，以允许安全地转移任务所有权
	ClockTask(ClockTask&& other) noexcept
		: m_TaskName(std::move(other.m_TaskName)),
		m_niDeviceName(other.m_niDeviceName),
		pTask(other.pTask),
		clkHub(other.clkHub),
		m_clockChannel(other.m_clockChannel),
		m_dutyCycle(other.m_dutyCycle),
		m_frequency(other.m_frequency),
		m_pulsesPerRun(other.m_pulsesPerRun)
	{
		// 将被移动的对象的句柄设为 nullptr，防止其析构函数重复释放资源
		other.pTask = nullptr;
	}

	ClockTask& operator=(ClockTask&& other) noexcept
	{
		if (this != &other) {
			// 清理当前资源
			if (pTask != nullptr) DAQmxClearTask(pTask);
			// 转移资源和成员
			m_TaskName = std::move(other.m_TaskName);
			m_niDeviceName = std::move(other.m_niDeviceName);
			m_clockChannel = std::move(other.m_clockChannel);
			m_dutyCycle = std::move(other.m_dutyCycle);
			m_frequency = std::move(other.m_frequency);
			m_pulsesPerRun = std::move(other.m_pulsesPerRun);
			pTask = other.pTask;
			clkHub = other.clkHub;
			// 清空源对象
			other.pTask = nullptr;
		}
		return *this;
	}

	void start();
	void stop();
	bool isDone() const;
protected:
	string m_TaskName;				// 任务名称
private:
	void checkErr(int32 error);
	string m_niDeviceName;			// ni设备名称
	string m_clockChannel;			// 输出通道 (相对路径, e.g., "/ctr0")
	string m_TrigChannel;			// startTrig通道 (相对路径, e.g., "/PFI0")
	float64 m_frequency;			// 频率
	float64 m_dutyCycle;			// 占空比
	uInt64 m_pulsesPerRun;			// 每次输出多少个脉冲, 0 表示无限
	TaskHandle pTask;				// 任务指针
	Clock::TimebaseSource m_timebaseSrc;
	Clock* clkHub;					// clockHub
};