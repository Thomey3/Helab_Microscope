#pragma once

#include "NIDAQmx.h"

#include <vector>
#include <string>
#include <stdexcept>
#include <sstream>
#include <functional>

namespace NIDAQmxUtils {

    // 内部实现细节，对外隐藏
    namespace detail {
        // ... (checkDAQmxError 和 getDAQmxList 函数保持不变, 这里省略)
        inline void checkDAQmxError(int32 errorCode, const std::string& context) {
            if (DAQmxFailed(errorCode)) {
                char errBuff[2048] = { '\0' };
                DAQmxGetExtendedErrorInfo(errBuff, 2048);
                throw std::runtime_error(context + " - DAQmx Error " + std::to_string(errorCode) + ": " + errBuff);
            }
        }

        inline std::vector<std::string> getDAQmxList(const std::function<int32(char*, uInt32)>& queryFunction) {
            int32 bufferSize = queryFunction(nullptr, 0);
            checkDAQmxError(bufferSize, "Failed to get buffer size for list");
            if (bufferSize <= 0) { // Changed to <= to handle errors returned as bufferSize
                return {};
            }
            std::vector<char> buffer(bufferSize);
            int32 error = queryFunction(buffer.data(), bufferSize);
            checkDAQmxError(error, "Failed to retrieve list data");
            std::vector<std::string> result;
            std::string dataStr(buffer.data());
            std::stringstream ss(dataStr);
            std::string item;
            while (std::getline(ss, item, ',')) {
                const auto first = item.find_first_not_of(" \t");
                if (std::string::npos == first) continue;
                const auto last = item.find_last_not_of(" \t");
                result.push_back(item.substr(first, (last - first + 1)));
            }
            return result;
        }

    } // namespace detail


    /**
     * @brief 获取系统中所有可用的NI-DAQmx设备名称。
     */
    inline std::vector<std::string> getAvailableDevices() {
        // ... (保持不变)
        auto func = [](char* data, uInt32 size) {
            return DAQmxGetSysDevNames(data, size);
            };
        return detail::getDAQmxList(func);
    }

    /**
         * @brief [更新] 获取指定设备上所有可用的物理模拟输出(AO)通道。
         * 返回的通道名将被统一格式化，以'/'开头。
         * @return 一个包含AO通道完整名称的向量, 例如 {"/Dev1/ao0", "/Dev1/ao1"}。
         */
    inline std::vector<std::string> getAOChannels(const std::string& deviceName) {
        auto func = [&](char* data, uInt32 size) {
            return DAQmxGetDevAOPhysicalChans(deviceName.c_str(), data, size);
            };
        std::vector<std::string> channels = detail::getDAQmxList(func);

        // [修改] 为所有通道名添加'/'前缀以实现统一格式
        for (auto& chan : channels) {
            if (!chan.empty() && chan.front() != '/') {
                chan.insert(0, 1, '/');
            }
        }
        return channels;
    }

    /**
     * @brief [更新] 获取指定设备上所有可用的物理计数器输出(CO)通道。
     * 返回的通道名将被统一格式化，以'/'开头。
     * @return 一个包含CO通道完整名称的向量, 例如 {"/Dev1/ctr0", "/Dev1/ctr1"}。
     */
    inline std::vector<std::string> getCOChannels(const std::string& deviceName) {
        auto func = [&](char* data, uInt32 size) {
            return DAQmxGetDevCOPhysicalChans(deviceName.c_str(), data, size);
            };
        std::vector<std::string> channels = detail::getDAQmxList(func);

        // [修改] 为所有通道名添加'/'前缀以实现统一格式
        for (auto& chan : channels) {
            if (!chan.empty() && chan.front() != '/') {
                chan.insert(0, 1, '/');
            }
        }
        return channels;
    }

    /**
     * @brief [已修正] 获取指定设备上所有可用的可编程功能接口(PFI)端子。
     * 该函数返回的名称已包含前缀'/'，是我们的统一标准，因此无需修改。
     * @return 一个包含PFI端子完整名称的向量, 例如 {"/Dev1/PFI0", "/Dev1/PFI1"}。
     */
    inline std::vector<std::string> getPFIChannels(const std::string& deviceName) {
        // 定义一个 lambda 表达式来调用通用的 DAQmxGetDevTerminals 函数
        auto func = [&](char* data, uInt32 size) {
            return DAQmxGetDevTerminals(deviceName.c_str(), data, size);
            };

        // 使用我们的辅助函数来获取设备上 *所有* 端子的列表
        const std::vector<std::string> allTerminals = detail::getDAQmxList(func);

        // 现在，过滤这个列表，只保留 PFI 端子
        std::vector<std::string> pfiTerminals;
        for (const auto& terminal : allTerminals) {
            // std::string::find 返回 std::string::npos 表示未找到子字符串
            if (terminal.find("PFI") != std::string::npos) {
                pfiTerminals.push_back(terminal);
            }
        }

        return pfiTerminals;
    }

    /**
     * @brief [更新] 获取指定设备上所有可用的物理数字输出线(DO Lines)。
     * 返回的通道名将被统一格式化，以'/'开头。
     * @return 一个包含DO线完整名称的向量, 例如 {"/Dev1/port0/line0", "/Dev1/port0/line1"}.
     */
    inline std::vector<std::string> getDOLines(const std::string& deviceName) {
        auto func = [&](char* data, uInt32 size) {
            return DAQmxGetDevDOLines(deviceName.c_str(), data, size);
            };
        std::vector<std::string> channels = detail::getDAQmxList(func);

        // [修改] 为所有通道名添加'/'前缀以实现统一格式
        for (auto& chan : channels) {
            if (!chan.empty() && chan.front() != '/') {
                chan.insert(0, 1, '/');
            }
        }
        return channels;
    }

    /**
     * @brief [更新] 获取指定设备上所有可用的物理数字输出端口(DO Ports)。
     * 返回的通道名将被统一格式化，以'/'开头。
     * @return 一个包含DO端口完整名称的向量, 例如 {"/Dev1/port0", "/Dev1/port1"}.
     */
    inline std::vector<std::string> getDOPorts(const std::string& deviceName) {
        auto func = [&](char* data, uInt32 size) {
            return DAQmxGetDevDOPorts(deviceName.c_str(), data, size);
            };
        std::vector<std::string> channels = detail::getDAQmxList(func);

        // [修改] 为所有通道名添加'/'前缀以实现统一格式
        for (auto& chan : channels) {
            if (!chan.empty() && chan.front() != '/') {
                chan.insert(0, 1, '/');
            }
        }
        return channels;
    }


    /**
     * @brief 用于存储解析后的通道名称部分的结构体。
     */
    struct ChannelParts {
        std::string deviceName;  // 设备名称, 例如 "/Dev1" 或 "cDAQ1Mod2"
        std::string channelName; // 通道/端子部分, 例如 "ao0" 或 "port0/line1"
    };

    /**
     * @brief [更新] 解析一个完整的NI-DAQmx通道/端子字符串，分离出设备名和通道名。
     *        此函数会保留设备名前的'/'（如果存在），这在后续API调用中非常有用。
     * @param fullChannelName 完整的通道名称字符串，例如 "/Dev1/ao0" 或 "cDAQ1Mod2/port0/line0"。
     * @return 一个 ChannelParts 结构体，包含分离后的设备名和通道名。
     */
    inline ChannelParts parseChannelName(const std::string& fullChannelName) {
        ChannelParts parts;
        if (fullChannelName.empty()) {
            return parts; // 对空输入返回空的结构体
        }

        // 从索引 1 开始查找分隔符'/'。
        // 这可以正确处理 "/Dev1/ao0" (分隔符在ao0前) 和 "Dev1/ao0" (分隔符在ao0前) 的情况，
        // 同时能将 "/Dev1" 或 "Dev1" 视为一个整体。
        const size_t separatorPos = fullChannelName.find('/', 1);

        if (separatorPos != std::string::npos) {
            // 找到了分隔符
            parts.deviceName = fullChannelName.substr(0, separatorPos);
            parts.channelName = fullChannelName.substr(separatorPos);
        }
        else {
            // 没有找到分隔符，整个字符串都是设备名
            parts.deviceName = fullChannelName;
            // parts.channelName 默认为空字符串
        }

        return parts;
    }
    // ========== 更新部分结束 ==========

} // namespace NIDAQmxUtils

// ========== 波形生成 API ==========
namespace WaveformGenerator {

    /**
     * @brief 生成一个三角波形。
     *        波形从最小值开始，线性上升到最大值，然后线性下降回最小值。
     * @param offset 波形的直流偏置（中心点）。
     * @param amplitude 波形的振幅（从偏置到峰值的距离）。峰峰值为 2 * amplitude。
     * @param samplesPerCycle 每个周期内的采样点数（必须 >= 2）。
     * @param numCycles 要生成的周期总数。
     * @return 一个包含生成波形数据的 std::vector<double>。
     * @throws std::invalid_argument 如果 samplesPerCycle < 2。
     */
    inline std::vector<double> generateTriangleWave(
        double offset,
        double amplitude,
        uint32_t samplesPerCycle,
        uint32_t numCycles
    ) {
        if (samplesPerCycle < 2) {
            throw std::invalid_argument("Triangle wave requires at least 2 samples per cycle.");
        }
        if (numCycles == 0) {
            return {};
        }

        const uint64_t totalSamples = static_cast<uint64_t>(samplesPerCycle) * numCycles;
        std::vector<double> waveform(totalSamples);

        const double minVal = offset - amplitude;
        const double maxVal = offset + amplitude;
        const double peakToPeak = 2.0 * amplitude;

        const uint32_t risingSamples = samplesPerCycle / 2;
        const uint32_t fallingSamples = samplesPerCycle - risingSamples;

        for (uint64_t i = 0; i < totalSamples; ++i) {
            const uint32_t sampleInCycle = i % samplesPerCycle;

            if (sampleInCycle < risingSamples) {
                // 上升沿: 从 minVal 线性插值到 maxVal
                double progress = static_cast<double>(sampleInCycle) / risingSamples;
                waveform[i] = minVal + peakToPeak * progress;
            }
            else {
                // 下降沿: 从 maxVal 线性插值到 minVal
                double progress = static_cast<double>(sampleInCycle - risingSamples) / fallingSamples;
                waveform[i] = maxVal - peakToPeak * progress;
            }
        }
        return waveform;
    }

    /**
     * @brief 生成一个锯齿波形。
     *        波形从最小值开始，线性上升到最大值，然后立即返回最小值开始下一个周期。
     * @param offset 波形的直流偏置（中心点）。
     * @param amplitude 波形的振幅（从偏置到峰值的距离）。峰峰值为 2 * amplitude。
     * @param samplesPerCycle 每个周期内的采样点数（必须 >= 1）。
     * @param numCycles 要生成的周期总数。
     * @return 一个包含生成波形数据的 std::vector<double>。
     * @throws std::invalid_argument 如果 samplesPerCycle < 1。
     */
    inline std::vector<double> generateSawtoothWave(
        double offset,
        double amplitude,
        uint32_t samplesPerCycle,
        uint32_t numCycles
    ) {
        if (samplesPerCycle < 1) {
            throw std::invalid_argument("Sawtooth wave requires at least 1 sample per cycle.");
        }
        if (numCycles == 0) {
            return {};
        }

        const uint64_t totalSamples = static_cast<uint64_t>(samplesPerCycle) * numCycles;
        std::vector<double> waveform(totalSamples);

        const double minVal = offset - amplitude;
        const double peakToPeak = 2.0 * amplitude;

        for (uint64_t i = 0; i < totalSamples; ++i) {
            const uint32_t sampleInCycle = i % samplesPerCycle;
            // 上升沿: 从 minVal 线性插值到 maxVal
            double progress = static_cast<double>(sampleInCycle) / samplesPerCycle;
            waveform[i] = minVal + peakToPeak * progress;
        }

        return waveform;
    }

} // namespace WaveformGenerator

namespace mapGenerator {
    /**
 * @brief 生成一个均匀分布的像素数据点计数映射表。
 * * 该函数创建一个数组，其中每个元素都具有相同的值，用于表示每个输出像素
 * 对应相同数量的原始数据点。
 *
 * @param width 图像的宽度，即输出像素的数量。
 * @param samples_per_pixel 每个像素对应的数据点（或叫 segment）的数量。
 * @return std::vector<int> 一个包含了计数映射表的向量。
 * 如果输入参数无效，则可能抛出异常或返回空向量。
 */
    inline std::vector<int> generate_uniform_map(int width, int samples_per_pixel)
    {
        // --- 输入验证 ---
        if (width <= 0) {
            // 宽度必须是正数
            throw std::invalid_argument("Width must be positive.");
        }
        if (samples_per_pixel < 0) {
            // 每个像素的数据点数不能是负数
            throw std::invalid_argument("Samples per pixel cannot be negative.");
        }

        // --- 创建并填充向量 ---
        // 使用 std::vector 的构造函数，可以非常简洁地创建一个包含 width 个元素，
        // 并且每个元素都被初始化为 samples_per_pixel 的向量。
        std::vector<int> counts_map(width, samples_per_pixel);

        return counts_map;
    }
}