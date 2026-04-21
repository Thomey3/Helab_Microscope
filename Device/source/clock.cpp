#include "clock.h"

#include <boost/algorithm/string/classification.hpp>
#include <boost/algorithm/string/split.hpp>
#include <boost/math/common_factor_rt.hpp>
#include <boost/scoped_array.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>

const char* g_DeviceNameClock = "Clock";

//#define DEBUG

Clock::Clock()
{

}

Clock::~Clock()
{
    // RAII 的威力：当 Clock 对象销毁时，m_ClockTask 会自动销毁。
    // 这会依次调用其中每个 ClockTask 对象的析构函数，
    // 从而确保所有 DAQmx 任务都被安全清除。
}

int Clock::Initialize()
{
#ifdef DEBUG
    TaskProperty test;
    test.niDeviceName = "/Dev1";
    test.clockChannel = "/ctr0";
    test.dutyCycle = 0.5;
    test.frequency = 1000;
    test.pulsesPerRun = 1000;
    test.TaskName = "test clock";
    test.TrigChannel = "/PFI0";
    addTask(test);
    getTask(test.TaskName)->start();
    getTask(test.TaskName)->isDone();
    getTask(test.TaskName)->stop();
    removeTask(test.TaskName);
#endif // DEBUG

    return DEVICE_OK;
}

int Clock::Shutdown()
{
    // 清理所有任务的最优雅方式。
    // clear() 会销毁 map 中的所有元素，触发它们的析构。
    m_ClockTask.clear();
    return DEVICE_OK;
}

bool Clock::Busy()
{
    // 只要有一个任务没有完成，整个时钟模块就视为“忙碌”。
    for (const auto& pair : m_ClockTask) {
        // isDone() 必须是 const 函数才能在这里调用
        if (!pair.second.isDone()) {
            return true; // 发现一个正在运行的任务，提前返回
        }
    }
    return false; // 所有任务都已完成
}
void Clock::GetName(char* name) const
{
    // Return the name used to referr to this device adapte
    CDeviceUtils::CopyLimitedString(name, g_DeviceNameClock);
}

int Clock::addTask(const TaskProperty& props)
{
    if (m_ClockTask.count(props.TaskName)) {
        LogMessage("Error: Task with name '" + props.TaskName + "' already exists.", true);
        return DEVICE_ERR;
    }

    try {
        // 使用 emplace 和 piecewise_construct 在 map 中“就地”构造 ClockTask。
        // 这是最高效的方式，避免了任何不必要的对象拷贝或移动。
        m_ClockTask.emplace(
            std::piecewise_construct,
            std::forward_as_tuple(props.TaskName), // tuple for map's key
            std::forward_as_tuple(this, props.niDeviceName, props.TaskName, props.clockChannel, props.TrigChannel, props.frequency, props.dutyCycle, props.pulsesPerRun, props.timebaseSrc) // tuple for ClockTask's constructor args
        );
    }
    catch (const std::runtime_error& e) {
        // ClockTask 构造函数抛出异常 (例如 DAQmx 错误)
        // emplace 保证，如果构造失败，不会有任何元素被添加到 map 中。
        LogMessage(e.what(), true);
        return DEVICE_ERR;
    }

    return DEVICE_OK;
}

int Clock::resetTask(const std::string& taskName, const TaskProperty& props)
{
    // “重置”一个任务最安全、最可靠的方法是：销毁旧的，创建新的。
    // 这可以确保所有底层的 DAQmx 资源都得到正确的重新配置。

    auto it = m_ClockTask.find(taskName);
    if (it == m_ClockTask.end()) {
        LogMessage("Error: Cannot reset non-existent task '" + taskName + "'.", true);
        return DEVICE_ERR; // 任务不存在
    }

    // 如果新属性中的任务名与旧任务名不同
    if (taskName != props.TaskName) {
        // 还需要检查新任务名是否与另一个已存在的任务冲突
        if (m_ClockTask.count(props.TaskName)) {
            LogMessage("Error: New task name '" + props.TaskName + "' conflicts with an existing task.", true);
            return DEVICE_ERR;
        }
    }

    // 1. 移除旧任务。这会自动调用其析构函数，清理 DAQmx 资源。
    m_ClockTask.erase(it);

    // 2. 使用新属性添加任务。我们可以直接复用 addTask 的逻辑。
    return addTask(props);
}

int Clock::removeTask(const std::string& taskName)
{
    // map::erase(key) 返回被删除的元素数量（对于 map 来说是 0 或 1）
    if (m_ClockTask.erase(taskName) > 0) {
        return DEVICE_OK;
    }

    LogMessage("Warning: Attempted to remove non-existent task '" + taskName + "'.", false);
    return DEVICE_ERR; // 任务未找到
}

ClockTask* Clock::getTask(const std::string& taskName)
{
    auto it = m_ClockTask.find(taskName);
    if (it != m_ClockTask.end()) {
        // it->second 就是存储的 ClockTask 对象，我们返回它的地址。
        return &(it->second);
    }
    return nullptr; // 未找到，返回空指针
}
