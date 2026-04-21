#include "QT12136DC.h"
using namespace std;
int QT12136DC::OnAcquisitionMode(MM::PropertyBase* pProp, MM::ActionType eAct)
{
	if (eAct == MM::AfterSet)
	{
		std::string modeStr;
		pProp->Get(modeStr);

		// 循环查找匹配的字符串，并设置对应的枚举值
		for (int i = 0; i < ACQUISITION_MODE_COUNT; ++i)
		{
			if (modeStr == g_AcqModeStrings[i])
			{
				m_CardSettings.AcquisitionMode = (AcquisitionMode)i;
				// 在这里应用硬件设置
				// return ApplySettingsToHardware();
				return DEVICE_OK;
			}
		}
	}
	else if (eAct == MM::BeforeGet)
	{
		// 直接使用枚举值作为索引来获取当前的设置字符串
		pProp->Set(string(g_AcqModeStrings[m_CardSettings.AcquisitionMode]).c_str());
	}

	return DEVICE_OK;
}
int QT12136DC::OnDataType(MM::PropertyBase* pProp, MM::ActionType eAct)
{
	if (eAct == MM::AfterSet)
	{
		std::string datatype;
		pProp->Get(datatype);

		// 循环查找匹配的字符串，并设置对应的枚举值
		for (int i = 0; i < DATATYPE_COUNT; ++i)
		{
			if (datatype == g_DataTypeStrings[i])
			{
				m_CardSettings.DataType = (DataType)i;
				// 在这里应用硬件设置
				// return ApplySettingsToHardware();
				return DEVICE_OK;
			}
		}
	}
	else if (eAct == MM::BeforeGet)
	{
		// 直接使用枚举值作为索引来获取当前的设置字符串
		pProp->Set(string(g_DataTypeStrings[m_CardSettings.DataType]).c_str());
	}

	return DEVICE_OK;
}
int QT12136DC::OnExtractMultiple(MM::PropertyBase* pProp, MM::ActionType eAct)
{
	if (eAct == MM::AfterSet)
	{
		double extractMultiple;
		pProp->Get(extractMultiple);
		m_CardSettings.extractMultiple = extractMultiple;
	}
	else if (eAct == MM::BeforeGet)
	{
		pProp->Set((double)m_CardSettings.extractMultiple);
	}

	return DEVICE_OK;
}
int QT12136DC::OnClockMode(MM::PropertyBase* pProp, MM::ActionType eAct)
{
	if (eAct == MM::AfterSet)
	{
		std::string modeStr;
		pProp->Get(modeStr);

		// 循环查找匹配的字符串，并设置对应的枚举值
		for (int i = 0; i < CLOCK_MODE_COUNT; ++i)
		{
			if (modeStr == g_ClockModeStrings[i])
			{
				m_CardSettings.ClockMode = (ClockMode)i;
				// 在这里应用硬件设置
				// return ApplySettingsToHardware();
				return DEVICE_OK;
			}
		}
	}
	else if (eAct == MM::BeforeGet)
	{
		// 直接使用枚举值作为索引来获取当前的设置字符串
		pProp->Set(string(g_ClockModeStrings[m_CardSettings.ClockMode]).c_str());
	}

	return DEVICE_OK;
}
int QT12136DC::OnTriggerMode(MM::PropertyBase* pProp, MM::ActionType eAct)
{
	if (eAct == MM::AfterSet)
	{
		std::string modeStr;
		pProp->Get(modeStr);

		// 循环查找匹配的字符串，并设置对应的枚举值
		for (int i = 0; i < TRIGGER_MODE_COUNT; ++i)
		{
			if (modeStr == g_TriggerModeStrings[i])
			{
				m_CardSettings.TriggerMode = (TriggerMode)i;
				// 在这里应用硬件设置
				// return ApplySettingsToHardware();
				return DEVICE_OK;
			}
		}
	}
	else if (eAct == MM::BeforeGet)
	{
		// 直接使用枚举值作为索引来获取当前的设置字符串
		pProp->Set(string(g_TriggerModeStrings[m_CardSettings.TriggerMode]).c_str());
	}

	return DEVICE_OK;
}

int QT12136DC::OnTriggerDelay(MM::PropertyBase* pProp, MM::ActionType eAct)
{
	if (eAct == MM::AfterSet)
	{
		double Trigdelay;
		pProp->Get(Trigdelay);

		m_TriggerSettings.trigDelay = Trigdelay;
	}
	else if (eAct == MM::BeforeGet)
	{
		
		pProp->Set(double(m_TriggerSettings.trigDelay));
	}

	return DEVICE_OK;
}

int QT12136DC::OnTriggerBiasVoltage(MM::PropertyBase* pProp, MM::ActionType eAct)
{
	if (eAct == MM::AfterSet)
	{
		double TrigBias;
		pProp->Get(TrigBias);

		m_TriggerSettings.biasVoltage = TrigBias;
	}
	else if (eAct == MM::BeforeGet)
	{

		pProp->Set(double(m_TriggerSettings.biasVoltage));
	}

	return DEVICE_OK;
}