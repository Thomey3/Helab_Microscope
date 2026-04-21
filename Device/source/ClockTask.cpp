#include "clock.h"

ClockTask::ClockTask(
	Clock* clk,
	const string niDeviceName,
	const string TaskName,
	const string clockChannel,
	const string TrigChannel,
	const float64 frequency,
	const float64 dutyCycle,
	const uInt64 pulsePerRun,
	Clock::TimebaseSource timebaseSrc) :
	m_TaskName(TaskName),
	m_niDeviceName(niDeviceName),
	m_clockChannel(clockChannel),
	m_TrigChannel(TrigChannel),
	m_frequency(frequency),
	m_dutyCycle(dutyCycle),
	m_pulsesPerRun(pulsePerRun),
	m_timebaseSrc(timebaseSrc),
	clkHub(clk),
	pTask(nullptr)
{
	// 使用 try-catch 块来保证即使配置中途失败，也能正确清理已创建的任务
	try {
		if (m_niDeviceName.empty()) {
			throw std::runtime_error("NI DAQ device name has not been set. Call ClockTask::setniDeviceName() first.");
		}

		// 拼接完整的通道路径，使调用者只需传入相对路径（如 "/ctr0"）
		std::string fullClockChannel = m_niDeviceName + m_clockChannel;
		std::string fullTrigChannel = m_TrigChannel.empty() ? "" : (m_niDeviceName + m_TrigChannel);

		// 1. 创建任务
		checkErr(DAQmxCreateTask(m_TaskName.c_str(), &pTask));

		// 2. 根据用户选择的时基来配置通道
		if (m_timebaseSrc == Clock::TimebaseSource::AUTO)
		{
			// --- 保持原有逻辑：让驱动自动选择时基 ---
			checkErr(DAQmxCreateCOPulseChanFreq(pTask, fullClockChannel.c_str(), "", DAQmx_Val_Hz, DAQmx_Val_Low, 0.0, m_frequency, m_dutyCycle));
		}
		else
		{
			// --- 新逻辑：手动固定时基，并使用 Ticks 来精确定义脉冲 ---
			double timebaseRate = 0.0;
			std::string timebaseSrcStr = "";

			switch (m_timebaseSrc)
			{
			case Clock::TimebaseSource::HZ_100_MHZ:
				timebaseRate = 100000000.0;
				timebaseSrcStr = m_niDeviceName + "/100MHzTimebase";
				break;
			case Clock::TimebaseSource::HZ_20_MHZ:
				timebaseRate = 20000000.0;
				timebaseSrcStr = m_niDeviceName + "/20MHzTimebase";
				break;
			case Clock::TimebaseSource::HZ_100_KHZ:
				timebaseRate = 100000.0;
				timebaseSrcStr = m_niDeviceName + "/100kHzTimebase";
				break;
			default: // AUTO case is already handled above
				throw std::runtime_error("Invalid timebase source selection.");
			}

			// 计算节拍数，并检查是否可以被精确生成
			if (m_frequency <= 0) throw std::runtime_error("Frequency must be positive.");

			double totalTicks_d = timebaseRate / m_frequency;
			// 检查总周期是否为整数个节拍
			if (std::abs(totalTicks_d - std::round(totalTicks_d)) > 1e-9) {
				throw std::runtime_error("The requested frequency cannot be generated exactly from the selected timebase. Frequency will be coerced. Please choose a different frequency or timebase.");
			}
			// 计算总节拍数 (此时 totalTicks_d 已经验证过可以取整)
			uInt64 totalTicks = static_cast<uInt64>(std::round(totalTicks_d));
			// 1. 根据理想占空比计算出理论上的高电平节拍数（可能是小数）
			double ideal_high_ticks_d = static_cast<double>(totalTicks) * m_dutyCycle;

			// 2. 将其四舍五入到最近的整数，这就是硬件将要使用的实际节拍数
			uInt64 highTicks = static_cast<uInt64>(std::round(ideal_high_ticks_d));

			// 3. (重要) 处理边界情况，确保高、低电平至少都有1个节拍，防止产生无效的0%或100%占空比
			if (totalTicks > 1) { // 仅当总节拍数大于1时这个调整才有意义
				if (highTicks == 0) {
					highTicks = 1;
				}
				if (highTicks >= totalTicks) {
					highTicks = totalTicks - 1;
				}
			}

			// 4. 计算实际的低电平节拍数
			uInt64 lowTicks = totalTicks - highTicks;

			// 5. (推荐) 根据实际的节拍数，反算出实际占空比，并更新成员变量
			if (totalTicks > 0) {
				m_dutyCycle = static_cast<double>(highTicks) / static_cast<double>(totalTicks);
			}
			// 现在 m_dutyCycle 存储的是硬件真正输出的占空比

			// 最终检查（以防万一 totalTicks 为1导致 lowTicks 为0）
			if (highTicks == 0 || lowTicks == 0) {
				throw std::runtime_error("无法生成有效脉冲，高电平或低电平节拍数为零。");
			}

			// 使用调整后、保证为整数的节拍数来创建通道
			checkErr(DAQmxCreateCOPulseChanTicks(pTask, fullClockChannel.c_str(), "", timebaseSrcStr.c_str(), DAQmx_Val_Low, 0, highTicks, lowTicks));

		}

		// 3. 配置有限/无限点输出模式
		if (m_pulsesPerRun == (uInt64)0) { // 使用 (uInt64)0 作为无限的标志
			// 对于连续模式，提供一个合理的缓冲区大小，而不是0
			checkErr(DAQmxCfgImplicitTiming(pTask, DAQmx_Val_ContSamps, 1000));
		}
		else {
			checkErr(DAQmxCfgImplicitTiming(pTask, DAQmx_Val_FiniteSamps, m_pulsesPerRun));
		}

		// 4. 配置数字边沿开始触发
		if (!fullTrigChannel.empty()) {
			checkErr(DAQmxCfgDigEdgeStartTrig(pTask, fullTrigChannel.c_str(), DAQmx_Val_Rising));
		}
	}
	catch (...) { // 捕获所有类型的异常
		// 如果构造过程中任何一步失败，确保清理已创建的 DAQmx 任务
		if (pTask != nullptr) {
			DAQmxClearTask(pTask);
		}
		// 将异常重新抛出，通知调用者对象创建失败
		throw;
	}
};

ClockTask::~ClockTask()
{
	if (pTask != nullptr) {
		// 在这里可以考虑停止任务，以防万一它还在运行
		DAQmxStopTask(pTask);
		DAQmxClearTask(pTask);
	}
}

void ClockTask::start() { checkErr(DAQmxStartTask(pTask)); }
void ClockTask::stop() { checkErr(DAQmxStopTask(pTask)); }
bool ClockTask::isDone() const
{
	if (pTask == nullptr) {
		return true; // 无效任务可视为已完成
	}

	uInt32 isTaskDone = 0;
	// 注意：我们不在这里的 checkErr 中抛出异常，因为 const 方法不应抛出
	// DAQmxIsTaskDone 是一个轻量级查询函数
	int32 error = DAQmxIsTaskDone(pTask, &isTaskDone);

	if (error < 0) {
		// 记录错误，但返回一个安全的值
		// 认为任务已完成是安全的，以避免调用方无限等待
		char errBuff[2048] = { '\0' };
		DAQmxGetExtendedErrorInfo(errBuff, 2048);
		if (clkHub) {
			clkHub->LogMessage(errBuff, true);
		}
		return true;
	}
	return (isTaskDone == TRUE);
}
void ClockTask::checkErr(int32 error)
{
	if (error < 0) {
		char errBuff[2048] = { '\0' };
		DAQmxGetExtendedErrorInfo(errBuff, 2048);
		if (clkHub) { // 检查 clkHub 是否有效
			clkHub->LogMessage(errBuff, true);
		}
		// 抛出异常以终止当前操作，并将错误信息传递给上层调用者
		throw std::runtime_error(errBuff);
	}
}