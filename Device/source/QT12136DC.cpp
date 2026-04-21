#include "QT12136DC.h"

const char* g_DeviceNameQT12136DC = "QT12136DC";
unsigned int unCardIndex = 0;
const char* const g_AcqModeStrings[] = {
	"Finite-Single",
	"Finite-Multi",
	"Infinite-Single",
	"Infinite-Multi",
	"DDS-Play",
	"File-Play"
};
const char* const g_DataTypeStrings[] = {
	"Raw Data", "Accumulative Number", "Decreasing Number",
	"Triangle Wave", "Constant Sequence 1 (0->1)", "Constant Sequence 2 (1->0)",
	"Reversal Number (0101)"
};
const char* const g_ClockModeStrings[] = {
	"Internal Reference Clock", "External Sampling Clock", "External Reference Clock"
};
const char* const g_TriggerModeStrings[] = {
	"Software Trigger","Internal Pulse", "External Pulse Rising", "External Pulse Falling",
	"Channel Rising", "Channel Falling", "Channel Both Edges",
	"External Pulse Both Edges"
};

QT12136DC::QT12136DC()
	: CGenericBase<QT12136DC>(),
	m_Initialized(false),
	m_Busy(false),
	m_CardSettings{ AcquisitionMode::MODE_INFINITE_MULTI,
					DMABlockNum::one_Block,
					DataType::Raw_Data,			
					0,	// 提取倍数		
					ChannelNumber::Two_Channels,
					ChannelMask::CH1_CH2, 
					ClockMode::Internal_Reference_Clock,
					TriggerMode::TRIG_EXTERNAL_PULSE_RISING},
	m_TriggerSettings{	0,  // 延迟触发
						0   // 偏置电压
					},
	m_CollectSettings{  0,  // 帧头
						0,  // 预触发点数
						1                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                           , // 重复频率
						15624960  // 段长   1s / 512(行) * 10e9(采样率) * 4(通道) * 2(字节) = 7812500 [取64的倍数]
					},
	m_bufferConfig{     512,
						15624960
					}
{
	InitializeDefaultErrorMessages();
	int err;
	//err = QT_BoardOpenBoard(unCardIndex); //打开板卡
	err = QT_Reset(); //打开并复位板卡

	err = CreateStringProperty("Board model number", std::to_string(QT_BoardGetCardType(unCardIndex)).c_str(), true, nullptr, true);
	err = CreateStringProperty("Firmware version number", std::to_string(QT_BoardGetCardSoftVersion(unCardIndex)).c_str(), true, nullptr, true);
	err = CreateStringProperty("ADC sampling rate", std::to_string(QT_BoardGetCardADCSamplerate(unCardIndex)).c_str(), true, nullptr, true);
	err = CreateStringProperty("Number of ADC channels", std::to_string(QT_BoardGetCardADCChNumbers(unCardIndex)).c_str(), true, nullptr, true);
	err = CreateStringProperty("ADC resolution", std::to_string(QT_BoardGetCardADCResolution(unCardIndex)).c_str(), true, nullptr, true);
	err = CreateStringProperty("Board Status Value", std::to_string(QT_BoardGetCardStatus(unCardIndex)).c_str(), true, nullptr, true);
	m_CardSettings.DMABlockNum = QT_BoardObtainTheADCBlockNum(unCardIndex);
}

QT12136DC::~QT12136DC()
{
	if (m_Initialized)
	{
		Shutdown();
	}
	delete m_thread;
}

int QT12136DC::Initialize()
{
	if (m_Initialized)
		return DEVICE_OK;
	bp_init(m_bufferConfig.num_blocks,m_bufferConfig.block_size); // 初始化缓冲池
	m_thread = new QT12136DCThread(this); // 创建采集线程
	int err;
	err = CreateStringProperty("AcquisitionMode", g_AcqModeStrings[m_CardSettings.AcquisitionMode], false, new CPropertyAction(this, &QT12136DC::OnAcquisitionMode), false);
	err = CreateIntegerProperty("DMA BlockNum", m_CardSettings.DMABlockNum, true, nullptr, false);
	err = CreateStringProperty("DataType", g_DataTypeStrings[m_CardSettings.DataType], false, new CPropertyAction(this, &QT12136DC::OnDataType), false);
	err = CreateIntegerProperty("ExtractMultiple", m_CardSettings.extractMultiple, false, new CPropertyAction(this, &QT12136DC::OnExtractMultiple), false);
	//err = CreateIntegerProperty("Channel Number", m_CardSettings.ChannelNumber, false, new CPropertyAction(this, &QT12136DC::OnChannelNumber), false);
	//err = CreateIntegerProperty("Channel", m_CardSettings.ChannelMask, false, new CPropertyAction(this, &QT12136DC::OnChannelMask), false);
	err = CreateStringProperty("ClockMode", g_ClockModeStrings[m_CardSettings.ClockMode], false, new CPropertyAction(this, &QT12136DC::OnClockMode), false);
	err = CreateStringProperty("TriggerMode", g_TriggerModeStrings[m_CardSettings.TriggerMode], false, new CPropertyAction(this, &QT12136DC::OnTriggerMode), false);
	err = CreateIntegerProperty("TriggerDelay", m_TriggerSettings.trigDelay, false, new CPropertyAction(this, &QT12136DC::OnTriggerDelay), false);
	err = CreateIntegerProperty("TriggerBiasVoltage", m_TriggerSettings.biasVoltage, false, new CPropertyAction(this, &QT12136DC::OnTriggerBiasVoltage), false);

	// 使用循环为属性添加所有允许值
	for (int i = 0; i < ACQUISITION_MODE_COUNT; i++) AddAllowedValue("AcquisitionMode", g_AcqModeStrings[i]);
	for (int i = 0; i < DATATYPE_COUNT; i++) AddAllowedValue("DataType", g_DataTypeStrings[i]);
	for (int i = 0; i < CLOCK_MODE_COUNT; i++) AddAllowedValue("ClockMode", g_ClockModeStrings[i]);
	for (int i = 0; i < TRIGGER_MODE_COUNT; i++) AddAllowedValue("TriggerMode", g_TriggerModeStrings[i]);
	SetPropertyLimits("ExtractMultiple", 0, 65536);

	m_Initialized = true;
	return DEVICE_OK;
}

int QT12136DC::Shutdown()
{
	if (!m_Initialized)
		return DEVICE_OK;
	// Clean up resources, stop hardware, etc.
	//int err = QT_BoardCloseBoard(unCardIndex); //关闭板卡
	m_Initialized = false;
	return DEVICE_OK;
}

void QT12136DC::GetName(char* name) const
{
	CDeviceUtils::CopyLimitedString(name, g_DeviceNameQT12136DC);
}

bool QT12136DC::Busy()
{
	// Implement the logic to check if the device is busy.
	// This could involve checking hardware status or internal flags.
	return m_Busy;
}

int QT12136DC::start()
{
	std::lock_guard<std::mutex> lock(m_mutex); // 在函数开始处加锁
	if (m_Busy || !m_Initialized)
		return DEVICE_ERR;
	m_Busy = true;
	int err = -1;
	bp_reconfigure(m_bufferConfig.num_blocks, m_bufferConfig.block_size);
	//err = QT_Reset();
	//if (err != 0) {
	//	m_Busy = false; // 如果复位失败，设置为未忙状态
	//	return err; // 返回错误码
	//}
	err = QT_BoardOpenBoard(unCardIndex); //打开板卡
	err = QT_BoardSetSoftwareReset(unCardIndex, 0);//ADC软件复位
	if (err != 0) {
		return err; // 如果复位失败，返回错误码
	}
	err = QT_BoardSetSoftwareReset(unCardIndex, 1);//DAC软件复位
	if (err != 0) {
		return err; // 如果复位失败，返回错误码
	}
	err = QT_Init();
	if (err != 0) {
		m_Busy = false; // 如果初始化失败，设置为未忙状态
		return err; // 返回错误码
	}
	err = m_thread->start(); // 启动采集线程
	if (err != DEVICE_OK) {
		m_Busy = false; // 如果线程启动失败，设置为未忙状态
		return err; // 返回错误码
	}
	return DEVICE_OK;
}

int QT12136DC::stop()
{
	std::lock_guard<std::mutex> lock(m_mutex); // 在函数开始处加锁
	if (!m_Initialized || !m_Busy)
		return DEVICE_OK;

	signalStop();
	wait();
	hardwareShutdown();

	return DEVICE_OK;
}

int QT12136DC::wait() {
	if (m_thread && m_thread->is_running()) {
		m_thread->wait();
	}
	return DEVICE_OK;
}

int QT12136DC::signalStop()
{
	if (m_thread) {
		m_thread->stop();
	}
	return DEVICE_OK;
}

int QT12136DC::hardwareShutdown()
{
	LogMessage("hardwareShutdown start.", true);
	if (!m_Initialized || !m_Busy)
		return DEVICE_OK;

	int err = -1;
	//上位机停止采集ADC数据
	err = QT_BoardSetupRecCollectData(unCardIndex, 0, 0);
	if (err != 0) {
		LogMessage("Failed to stop data collection.", false);
	}
	//释放buffer
	err = QT_BoardSetFreeOperation(unCardIndex);
	if (err != 0) {
		LogMessage("Failed to free board operation.", false);
	}
	err = QT_BoardCloseBoard(unCardIndex); //关闭板卡
	if (err != 0) {
		LogMessage("Failed to close board.", false);
	}
	LogMessage("hardwareShutdown success.", true);
	m_Busy = false;
	return DEVICE_OK;
}

int QT12136DC::QT_Init()
{
	int err = -1;
	//设置数据类型
	err = QT_BoardSetTheDataType(unCardIndex, m_CardSettings.DataType);
	if (err != 0) {
		return err; // 如果设置数据类型失败，返回错误码
	}
	//设置抽取倍率
	err = QT_BoardSetTheExtractionMultiple(unCardIndex, m_CardSettings.extractMultiple);
	if (err != 0) {
		return err; // 如果设置提取倍数失败，返回错误码
	}
	//使能通道数
	err = QT_BoardSetupRecWorkingMode(unCardIndex, m_CardSettings.ChannelNumber);
	if (err != 0) {
		return err; // 如果设置通道数失败，返回错误码
	}
	//ADC通道选择
	err = QT_BoardSetupRecChannelMode(unCardIndex, m_CardSettings.ChannelMask);
	if (err != 0) {
		return err; // 如果设置通道使能失败，返回错误码
	}
	//设置时钟模式
	err = QT_BoardSetupRecClockMode(unCardIndex, m_CardSettings.ClockMode);
	if (err != 0) {
		return err; // 如果设置时钟模式失败，返回错误码
	}
	//设置外部触发模式
	err = QT_BoardSetupTrigRecExternal(unCardIndex, m_CardSettings.TriggerMode,m_TriggerSettings.trigDelay,m_TriggerSettings.biasVoltage);
	if (err != 0) {
		return err; // 如果设置触发模式失败，返回错误码
	}
	for (int i = 0; i < 2; i++) {
		QT_BoardSetAFEGain(unCardIndex, i, 1024);
		QT_BoardSetBiasAdjustment(unCardIndex, i, 0);
	}
	if (m_CardSettings.DMABlockNum == 1)
	{
		err = QT_BoardSetupModeRecFIFOMulti(unCardIndex, 0, m_CollectSettings.Segment, m_CollectSettings.Frameswitch, m_CollectSettings.Pretrigdots, m_CollectSettings.RepetitionFrequency);
	}
	else if (m_CardSettings.DMABlockNum == 2)
	{
		err = QT_BoardSetupModeRecFIFOMulti(unCardIndex, 0, m_CollectSettings.Segment, m_CollectSettings.Frameswitch, m_CollectSettings.Pretrigdots, m_CollectSettings.RepetitionFrequency);
		err = QT_BoardSetupModeRecFIFOMulti(unCardIndex, 1, m_CollectSettings.Segment, m_CollectSettings.Frameswitch, m_CollectSettings.Pretrigdots, m_CollectSettings.RepetitionFrequency);
	}
	if (err != 0) {
		return err; // 如果设置采集模式失败，返回错误码
	}
	err = QT_BoardSetInitializeOperation(unCardIndex);//初始化buffer
	if (err != 0) {
		return err; // 如果初始化操作失败，返回错误码
	}
	err = QT_BoardSetupRecCollectData(unCardIndex, 1, 0);//开始采集
	if (err != 0) {
		return err; // 如果开始采集失败，返回错误码
	}
}

int QT12136DC::QT_Reset()
{
	int err;
	err = QT_BoardOpenBoard(unCardIndex); //打开板卡
	err = QT_BoardSetSoftwareReset(unCardIndex, 0);//ADC软件复位
	if (err != 0) {
		return err; // 如果复位失败，返回错误码
	}
	err = QT_BoardSetSoftwareReset(unCardIndex, 1);//DAC软件复位
	if (err != 0) {
		return err; // 如果复位失败，返回错误码
	}
	err = QT_BoardSetupRecCollectData(unCardIndex, 0, 0);//停止采集
	if (err != 0) {
		return err; // 如果停止采集失败，返回错误码
	}
	err = QT_BoardSetupRepPlayData(unCardIndex, 0);//停止播放
	if (err != 0) {
		return err; // 如果停止播放失败，返回错误码
	}
	return err;
}