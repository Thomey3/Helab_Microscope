#pragma once
typedef enum ChannelMask {
	// 基础通道定义 (位标志)
	CH1 = 1 << 0,  // 值为 1 (二进制 0001)
	CH2 = 1 << 1,  // 值为 2 (二进制 0010)
	CH3 = 1 << 2,  // 值为 4 (二进制 0100)
	CH4 = 1 << 3,  // 值为 8 (二进制 1000)

	// 双通道组合
	CH1_CH2 = CH1 | CH2,  // 值为 3
	CH1_CH3 = CH1 | CH3,  // 值为 5
	CH2_CH3 = CH2 | CH3,  // 值为 6 
	CH1_CH4 = CH1 | CH4,  // 值为 9
	CH2_CH4 = CH2 | CH4,  // 值为 10
	CH3_CH4 = CH3 | CH4,  // 值为 12

	// 四通道组合
	CH_ALL = CH1 | CH2 | CH3 | CH4 // 值为 15
} ChannelMask;

typedef enum DMABlockNum {
	one_Block = 1, // 单块
	two_Blocks = 2, // 双块
} DMABlockNum;

typedef enum ChannelNumber {
	One_Channel = 1, // 单通道
	Two_Channels = 2, // 双通道
	Four_Channels = 4 // 四通道
} ChannelNumber;

typedef enum AcquisitionMode {
	MODE_FINITE_SINGLE = 0, // 有限点单次采集
	MODE_FINITE_MULTI = 1, // 有限点多次采集
	MODE_INFINITE_SINGLE = 2, // 无限点单次采集
	MODE_INFINITE_MULTI = 3, // 无限点多次采集
	MODE_DDS_PLAY = 4, // DDS播放
	MODE_FILE_PLAY = 5,  // 外部数据文件播放
	ACQUISITION_MODE_COUNT // 采集模式数量
} AcquisitionMode;

typedef enum DataType {
	Raw_Data = 0, // 原始数据
	Accumulative_Number = 1, // 累加数
	Decreasing_Number = 2, // 递减数
	Triangle_Wave = 3, // 三角波
	Constant_Sequence_1 = 4, // 常数序列1：高位0低位1
	Constant_Sequence_2 = 5,  // 常数序列2：高位1低位0
	Reversal_Number = 6, // 反转数,01交替
	DATATYPE_COUNT
} DataType;

typedef enum ClockMode {
	Internal_Reference_Clock = 0, // 内参考时钟
	External_Sampling_Clock = 1, // 外采样时钟
	External_Reference_Clock = 2, // 外参考时钟
	CLOCK_MODE_COUNT
} ClockMode;

typedef enum TriggerMode {
	TRIG_SOFTWARE = 0,		// 软件触发
	TRIG_INTERNAL_PULSE = 1, // 内部脉冲触发
	TRIG_EXTERNAL_PULSE_RISING = 2, // 外部脉冲上升沿触发
	TRIG_EXTERNAL_PULSE_FALLING = 3, // 外部脉冲下降沿触发
	TRIG_CHANNEL_RISING = 4, // 通道上升沿触发
	TRIG_CHANNEL_FALLING = 5, // 通道下降沿触发
	TRIG_CHANNEL_BOTH_EDGES = 6, // 通道双边沿触发
	TRIG_EXTERNAL_PULSE_BOTH_EDGES = 7,  // 外部脉冲双边沿触发
	TRIGGER_MODE_COUNT
} TriggerMode;