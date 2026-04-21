#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include "QT_Board.h"
#include <string.h>
#include "QTSetFunction.h"

int temp = 0;
int chMode = 0;
int QT_BoardGetBoardInfo()
{
	//获取板卡型号
	printf("板卡型号:%d\n", QT_BoardGetCardType(unCardIndex));

	//获取板卡固件版本号
	printf("固件版本号:%d\n", QT_BoardGetCardSoftVersion(unCardIndex));

	//获取板卡ADC采样率信息
	printf("ADC采样率:%d\n", QT_BoardGetCardADCSamplerate(unCardIndex));

	//获取板卡ADC通道数信息
	printf("ADC通道数:%d\n", QT_BoardGetCardADCChNumbers(unCardIndex));

	//获取板卡ADC分辨率信息
	printf("ADC分辨率:%d\n", QT_BoardGetCardADCResolution(unCardIndex));

	//获取板卡DAC采样率信息
	printf("DAC采样率:%d\n", QT_BoardGetCardDACSamplerate(unCardIndex));

	//获取板卡DAC通道数信息
	printf("DAC通道数:%d\n", QT_BoardGetCardDACChNumbers(unCardIndex));

	//获取板卡DAC分辨率信息
	printf("DAC分辨率:%d\n", QT_BoardGetCardDACResolution(unCardIndex));

	//获取板卡状态值
	printf("板卡状态值:%d\n\n", QT_BoardGetCardStatus(unCardIndex));

	return 0;
}

int QT_BoardSetRecRepMode()
{
	printf("板卡支持的采集/播放模式:");

	//有限点单次采集
	temp = QT_BoardIfSupportFiniteSingle(unCardIndex);
	if (temp == 0)
	{
		printf("0:有限点单次采集模式\t");
	}

	//有限点多次采集
	temp = QT_BoardIfSupportFiniteMulti(unCardIndex);
	if (temp == 0)
	{
		printf("1:有限点多次采集模式\t");
	}

	//无限点单次采集
	temp = QT_BoardIfSupportInfiniteSingle(unCardIndex);
	if (temp == 0)
	{
		printf("2:无限点单次采集模式\t");
	}

	//无限点多次采集
	temp = QT_BoardIfSupportInfiniteMulti(unCardIndex);
	if (temp == 0)
	{
		printf("3:无限点多次采集模式\t");
	}

	//DDS播放
	temp = QT_BoardIfSupportDDSPlay(unCardIndex);
	if (temp == 0)
	{
		printf("4:DDS播放模式\t");
	}

	//外部数据文件播放
	temp = QT_BoardIfSupportFilePlay(unCardIndex);
	if (temp == 0)
	{
		printf("5:外部数据播放模式\t");
	}

	return 0;
}

int QT_BoardSetRecChSelect()
{
	//通道选择
	temp = QT_BoardIfSupportRecChannelSwitch(unCardIndex);
	if (temp == 0)
	{
		int chMode = 0;
		printf("通道选择(0:通道1 1:通道2 2:通道3 3:通道4(依次类推)):");
		scanf("%d", &chMode);
		QT_BoardSetupRecChannelMode(unCardIndex, chMode);
	}

	return 0;
}

int QT_BoardSetFanCtrl()
{
	//风扇控制
	temp = QT_BoardIfSupportFanControl(unCardIndex);
	if (temp == 0)
	{
		int ctrlSource = 0;
		int fangear = 0;
		printf("设置风扇控制源(0:板卡温度控制风扇转速 1:上位机控制风扇转速):");
		scanf("%d", &ctrlSource);
		printf("风扇转速挡位(0~7 数字越大转速越快(前提:需要设置风扇控制模式为上位机控制)):");
		scanf("%d", &fangear);
		QT_BoardSetFanControl(unCardIndex, ctrlSource, fangear);
	}

	return 0;
}

int QT_BoardSetAFE()
{
	//模拟前端调节
	temp = QT_BoardIfSupportOffsetAdjust(unCardIndex);
	if (temp == 0)
	{
		temp = QT_BoardIfSupport1144VG(unCardIndex);
		if (temp == 0)
		{
			double biasVoltage = 0;
			int iGainFalloff = 0;
			int chID = 0;
			printf("设置通道ID(0:通道1 1:通道2 2:通道3 3:通道4(依次类推)):");
			scanf("%d", &chID);
			printf("偏置电压参数:");
			scanf("%lf", &biasVoltage);
			printf("增益/衰减(bit0衰减、bit1前增益、bit2后增益、bit3 AMPX1、bit4 AMPX4):");
			scanf("%d", &iGainFalloff);
			QT_BoardConfigureAnalogFrontEndVG(unCardIndex, chID, biasVoltage, iGainFalloff);
		}
		else
		{
			//获取板卡量程范围个数
			int allchRangeNum = 0;
			allchRangeNum = QT_BoardObtainThechRangeNum(unCardIndex);

			//模拟前端调节
			if (QT_BoardIfSupportRecChannelSwitch(unCardIndex) < 0)
			{
				//获取板卡量程范围
				int allchRange = 0;
				printf("模拟前端量程范围: ");
				for (size_t i = 0; i < allchRangeNum; i++)
				{
					allchRange = QT_BoardObtainThechRange(unCardIndex, i);
					printf("%d:±%dmv\t", i, allchRange);
				}

				int chID = 0;
				int chRange = 0;
				uint32_t offsetValue = 0;
				double calibrationValue = 0;

				printf("模拟前端通道量程范围:");
				scanf("%d", &chRange);

				for (size_t i = 0; i < QT_BoardGetCardADCChNumbers(unCardIndex); i++)
				{
					temp = QT_BoardIfSupportUseEEPROM(unCardIndex);
					if (temp == 0)
					{
						QT_BoardSetAFEEEPROM(unCardIndex, i, chRange, &offsetValue, &calibrationValue);
					}
					else
					{
						QT_BoardSetAFEJSON(unCardIndex, i, chRange);
					}
				}
			}
			else
			{
				//获取板卡量程范围
				int allchRange = 0;
				int chID = 0;
				int chRange = 0;
				printf("模拟前端量程范围: ");
				for (size_t i = 0; i < allchRangeNum; i++)
				{
					allchRange = QT_BoardObtainThechRange(unCardIndex, i);
					printf("%d:±%dmv\t", i, allchRange);
				}
				printf("设置量程范围:");
				scanf("%d", &chRange);

				printf("模拟前端通道ID: ");
				for (size_t i = 0; i < QT_BoardGetCardADCChNumbers(unCardIndex); i++)
				{
					printf("%d:通道%d\t", i, i + 1);
				}
				printf("设置通道ID(0:通道1 1:通道2 2:通道3 3:通道4(依次类推)):");
				scanf("%d", &chID);

				temp = QT_BoardIfSupportUseEEPROM(unCardIndex);
				if (temp == 0)
				{
					uint32_t offsetValue = 0;
					double calibrationValue = 0;
					QT_BoardSetAFEEEPROM(unCardIndex, chID, chRange, &offsetValue, &calibrationValue);
				}
				else
				{
					QT_BoardSetAFEJSON(unCardIndex, chID, chRange);
				}
			}
		}
	}

	return 0;
}

int QT_BoardSetRecClockMode()
{
	//ADC时钟模式
	temp = QT_BoardIfSupportClockMode(unCardIndex);
	if (temp == 0)
	{
		int clockMode = 0;
		printf("时钟模式(0:内参考时钟 1:外采样时钟 2:外参考时钟):");
		scanf("%d", &clockMode);
		QT_BoardSetupRecClockMode(unCardIndex, clockMode);
	}
	else if (temp == -1)
	{
		//ADC时钟模式
		int clockMode = 0;
		printf("时钟模式(0:内参考时钟 1:外采样时钟):");
		scanf("%d", &clockMode);
		QT_BoardSetupRecClockMode(unCardIndex, clockMode);
	}
	else if (temp == -2)
	{
		//ADC时钟模式
		int clockMode = 0;
		printf("时钟模式(0:内参考时钟 2:外参考时钟):");
		scanf("%d", &clockMode);
		QT_BoardSetupRecClockMode(unCardIndex, clockMode);
	}
	else if (temp == -3)
	{
		//ADC时钟模式
		int clockMode = 0;
		printf("时钟模式(0:内参考时钟):");
		scanf("%d", &clockMode);
		QT_BoardSetupRecClockMode(unCardIndex, clockMode);
	}

	return 0;
}

int QT_BoardSetRecTrigMode(int workMode)
{
	//ADC触发方式
	int trigMode = 1;
	if (workMode == 0)
	{
		printf("触发模式(0:软件触发 1:内部脉冲触发 2:外部脉冲上升沿触发 3:外部脉冲下降沿触发 4:通道上升沿触发 5:通道下降沿触发 6:通道双边沿触发 7:外部脉冲双边沿触发):");
		scanf("%d", &trigMode);
		if (trigMode != 0 && trigMode != 1 && trigMode != 2 && trigMode != 3 && trigMode != 4 && trigMode != 5 && trigMode != 6 && trigMode != 7)
		{
			printf("触发模式设置错误!\n");
			return 0;
		}
	}
	else if (workMode == 1)
	{
		printf("触发模式(1:内部脉冲触发 2:外部脉冲上升沿触发 3:外部脉冲下降沿触发 4:通道上升沿触发 5:通道下降沿触发 6:通道双边沿触发 7:外部脉冲双边沿触发):");
		scanf("%d", &trigMode);
		if (trigMode != 1 && trigMode != 2 && trigMode != 3 && trigMode != 4 && trigMode != 5 && trigMode != 6 && trigMode != 7)
		{
			printf("触发模式设置错误!\n");
			return 0;
		}
	}
	else if (workMode == 2)
	{
		printf("触发模式(0:软件触发):");
		scanf("%d", &trigMode);
		if (trigMode != 0)
		{
			printf("触发模式设置错误!\n");
			return 0;
		}
	}
	else if (workMode == 3)
	{
		printf("触发模式(1:内部脉冲触发 2:外部脉冲上升沿触发 3:外部脉冲下降沿触发 4:通道上升沿触发 5:通道下降沿触发 6:通道双边沿触发 7:外部脉冲双边沿触发):");
		scanf("%d", &trigMode);
		if (trigMode != 1 && trigMode != 2 && trigMode != 3 && trigMode != 4 && trigMode != 5 && trigMode != 6 && trigMode != 7)
		{
			printf("触发模式设置错误!\n");
			return 0;
		}
	}

	if (trigMode == 0)
	{
		//ADC软件触发
		QT_BoardSetupTrigRecSoftware(unCardIndex);
	}
	else if (trigMode == 1)
	{
		//ADC内部脉冲触发
		uint32_t PulsePeriod = 100000;
		uint32_t PulseWidth = 100;
		/*printf("设置内部脉冲触发周期(单位:ns):");
		scanf("%d", &PulsePeriod);
		printf("设置内部脉冲宽度(单位:ns):");
		scanf("%d", &PulseWidth);*/
		QT_BoardSetupTrigRecInternalPulse(unCardIndex, PulsePeriod, PulseWidth);
	}
	else if (trigMode == 2 || trigMode == 3 || trigMode == 7)
	{
		//ADC外部脉冲触发
		uint32_t trigDelay = 0;
		printf("设置延迟触发长度:");
		scanf("%d", &trigDelay);
		double biasVoltage = 0;
		printf("设置偏置电压(0V~5V):");
		scanf("%lf", &biasVoltage);
		QT_BoardSetupTrigRecExternal(unCardIndex, trigMode, trigDelay, biasVoltage);
	}
	else if (trigMode == 4 || trigMode == 5 || trigMode == 6)
	{
		//ADC通道触发
		uint32_t Channel = 1;
		int32_t trigLevel = 0;
		int32_t hysteresis = 0;
		for (size_t i = 1; i < QT_BoardGetCardADCChNumbers(unCardIndex) + 1; i++)
		{
			printf("       %d:通道%d\n", i, i);
		}
		printf("通道ID:");
		scanf("%d", &Channel);
		printf("设置通道触发阈值:");
		scanf("%d", &trigLevel);
		printf("设置通道触发迟滞:");
		scanf("%d", &hysteresis);
		QT_BoardSetupTrigRecChannel(unCardIndex, trigMode, Channel, hysteresis, trigLevel);
	}

	return 0;
}

int QT_BoardSetRepChSelect()
{
	//通道选择
	temp = QT_BoardIfSupportRepChannelSwitch(unCardIndex);
	if (temp == 0)
	{
		int channelcount = 0;
		printf("通道选择(0:通道1 1:通道2 2:通道3 3:通道4(依次类推)):");
		scanf("%d", &channelcount);
		QT_BoardSetupRepChannelMode(unCardIndex, channelcount);
	}

	return 0;
}

int QT_BoardSetRepClockMode()
{
	temp = QT_BoardIfSupportClockMode(unCardIndex);
	if (temp == 0)
	{
		//DAC时钟模式
		int clockMode = 0;
		printf("时钟模式(0:内参考时钟 1:外采样时钟 2:外参考时钟):");
		scanf("%d", &clockMode);
		QT_BoardSetupRepClockMode(unCardIndex, clockMode);
	}
	else if (temp == -1)
	{
		//DAC时钟模式
		int clockMode = 0;
		printf("时钟模式(0:内参考时钟 1:外采样时钟):");
		scanf("%d", &clockMode);
		QT_BoardSetupRepClockMode(unCardIndex, clockMode);
	}
	else if (temp == -2)
	{
		//DAC时钟模式
		int clockMode = 0;
		printf("时钟模式(0:内参考时钟 2:外参考时钟):");
		scanf("%d", &clockMode);
		QT_BoardSetupRepClockMode(unCardIndex, clockMode);
	}
	else if (temp == -3)
	{
		//DAC时钟模式
		int clockMode = 0;
		printf("时钟模式(0:内参考时钟):");
		scanf("%d", &clockMode);
		QT_BoardSetupRepClockMode(unCardIndex, clockMode);
	}

	return 0;
}

int QT_BoardSetRepTrigMode(int workMode)
{
	if (workMode == 5)
	{
		uint32_t trigmode = 0;
		printf("设置触发模式:(0:软件触发  1:内部脉冲触发  2:外部上升沿触发   3:外部下降沿触发):");
		scanf("%d", &trigmode);
		if (trigmode == 0)
		{
			QT_BoardSetupTrigRepSoftware(unCardIndex);
		}
		else if (trigmode == 1)
		{
			uint32_t pulsePeriod = 0;
			uint32_t pulseWidth = 0;
			printf("设置内部脉冲周期(单位:ns):");
			scanf("%d", &pulsePeriod);
			printf("设置内部脉冲宽度(单位:ns):");
			scanf("%d", &pulseWidth);
			QT_BoardSetupTrigRepInternalPulse(unCardIndex, pulsePeriod, pulseWidth);
		}
		else if (trigmode == 2)
		{
			QT_BoardSetupTrigRepExternal(unCardIndex, 2);
		}
		else if (trigmode == 3)
		{
			QT_BoardSetupTrigRepExternal(unCardIndex, 3);
		}
	}

	return 0;
}


int QT_BoardSetRecStdSingle(int blockNum)
{
	//有限点单次触发采集模式

	uint32_t Frameswitch = 0;
	uint32_t Pretrigdots = 0;
	uint32_t Segment = 0;
	printf("设置段长(单位:字节):");
	scanf("%d", &Segment);
	printf("设置帧头(0:禁用  1:使能):");
	scanf("%d", &Frameswitch);
	printf("设置预触发(单位:点数):");
	scanf("%d", &Pretrigdots);

	if (blockNum == 1) {
		QT_BoardSetupModeRecStdSingle(unCardIndex, 0, Segment, Frameswitch, Pretrigdots);
	}
	else if (blockNum == 2) {
		QT_BoardSetupModeRecStdSingle(unCardIndex, 0, Segment, Frameswitch, Pretrigdots);
		QT_BoardSetupModeRecStdSingle(unCardIndex, 1, Segment, Frameswitch, Pretrigdots);
	}

	//上位机启动采集ADC数据
	QT_BoardSetupRecCollectData(unCardIndex, 1, 1);

	//获取数据
	if (blockNum == 1)
	{
		if (chMode == 1) {
			Segment = Segment / 4;
		}
		else if (chMode == 2) {
			Segment = Segment / 2;
		}
		Segment = Segment / 64 * 64;

		uint8_t* getdatabuffer0 = (uint8_t*)malloc(Segment);
		if (NULL == getdatabuffer0)
		{
			printf("申请缓存getdatabuffer0失败\n");
			return 0;
		}
		memset(getdatabuffer0, 0, Segment);

		QT_BoardSetupRecGetData(unCardIndex, 0, getdatabuffer0, Segment);

		//生成文件
		char Allfilepath[256] = { 0 };
		char Allfilename0[256] = { 0 };
		sprintf(Allfilename0, "%sadc_std_single_0.bin", Allfilepath, "");

		FILE* fp0 = fopen(Allfilename0, "wb+");
		if (NULL == fp0)
		{
			return 0;
		}

		fwrite(getdatabuffer0, 1, Segment, fp0);

		fclose(fp0);
		fp0 = NULL;
		free(getdatabuffer0);
		getdatabuffer0 = NULL;
	}
	else if (blockNum == 2)
	{
		uint8_t* getdatabuffer0 = (uint8_t*)malloc(Segment);
		if (NULL == getdatabuffer0)
		{
			printf("申请缓存getdatabuffer0失败\n");
			return 0;
		}
		memset(getdatabuffer0, 0, Segment);

		uint8_t* getdatabuffer1 = (uint8_t*)malloc(Segment);
		if (NULL == getdatabuffer1)
		{
			printf("申请缓存getdatabuffer1失败\n");
			return 0;
		}
		memset(getdatabuffer1, 0, Segment);

		QT_BoardSetupRecGetData(unCardIndex, 0, getdatabuffer0, Segment);
		QT_BoardSetupRecGetData(unCardIndex, 1, getdatabuffer1, Segment);

		//生成文件
		char Allfilepath[256] = { 0 };
		char Allfilename0[256] = { 0 };
		char Allfilename1[256] = { 0 };
		sprintf(Allfilename0, "%sadc_std_single_0.bin", Allfilepath, "");
		sprintf(Allfilename1, "%sadc_std_single_1.bin", Allfilepath, "");

		FILE* fp0 = fopen(Allfilename0, "wb+");
		if (NULL == fp0)
		{
			return 0;
		}

		FILE* fp1 = fopen(Allfilename1, "wb+");
		if (NULL == fp1)
		{
			return 0;
		}

		fwrite(getdatabuffer0, 1, Segment, fp0);
		fwrite(getdatabuffer1, 1, Segment, fp1);

		fclose(fp0);
		fp0 = NULL;
		free(getdatabuffer0);
		getdatabuffer0 = NULL;
		fclose(fp1);
		fp1 = NULL;
		free(getdatabuffer1);
		getdatabuffer1 = NULL;


		//上位机停止采集ADC数据
		QT_BoardSetupRecCollectData(unCardIndex, 0, 1);
	}
	return 0;
}

int QT_BoardSetRecStdMulti(int blockNum)
{
	//有限点单次触发采集模式
	uint32_t Frameswitch = 0;
	uint32_t Pretrigdots = 0;
	uint32_t segCount = 0;
	uint32_t Segment = 0;
	printf("设置段长(单位:字节):");
	scanf("%d", &Segment);
	printf("设置段数:");
	scanf("%d", &segCount);
	printf("设置帧头(0:禁用  1:使能):");
	scanf("%d", &Frameswitch);
	printf("设置预触发(单位:点数):");
	scanf("%d", &Pretrigdots);
	if (blockNum == 1)
	{
		QT_BoardSetupModeRecStdMulti(unCardIndex, 0, Segment, segCount, Frameswitch, Pretrigdots);
	}
	else if (blockNum == 2)
	{
		QT_BoardSetupModeRecStdMulti(unCardIndex, 0, Segment, segCount, Frameswitch, Pretrigdots);
		QT_BoardSetupModeRecStdMulti(unCardIndex, 1, Segment, segCount, Frameswitch, Pretrigdots);
	}

	//上位机启动采集ADC数据
	QT_BoardSetupRecCollectData(unCardIndex, 1, 1);
	if (chMode == 1) {
		Segment = Segment / 4;
	}
	else if (chMode == 2) {
		Segment = Segment / 2;
	}
	Segment = Segment / 64 * 64;
	//获取数据
	if (blockNum == 1)
	{
		uint8_t* getdatabuffer0 = (uint8_t*)malloc(Segment * segCount);
		if (NULL == getdatabuffer0)
		{
			printf("申请缓存getdatabuffer0失败\n");
			return 0;
		}
		memset(getdatabuffer0, 0, Segment * segCount);

		QT_BoardSetupRecGetData(unCardIndex, 0, getdatabuffer0, Segment * segCount);

		//生成文件
		char Allfilepath[256] = { 0 };
		char Allfilename0[256] = { 0 };
		sprintf(Allfilename0, "%sadc_std_multi_0.bin", Allfilepath, "");

		FILE* fp0 = fopen(Allfilename0, "wb+");
		if (NULL == fp0)
		{
			return 0;
		}

		fwrite(getdatabuffer0, 1, Segment * segCount, fp0);

		fclose(fp0);
		fp0 = NULL;
		free(getdatabuffer0);
		getdatabuffer0 = NULL;
	}
	else if (blockNum == 2)
	{
		uint8_t* getdatabuffer0 = (uint8_t*)malloc(Segment * segCount);
		if (NULL == getdatabuffer0)
		{
			printf("申请缓存getdatabuffer0失败\n");
			return 0;
		}
		memset(getdatabuffer0, 0, Segment * segCount);

		uint8_t* getdatabuffer1 = (uint8_t*)malloc(Segment * segCount);
		if (NULL == getdatabuffer1)
		{
			printf("申请缓存getdatabuffer1失败\n");
			return 0;
		}
		memset(getdatabuffer1, 0, Segment * segCount);

		QT_BoardSetupRecGetData(unCardIndex, 0, getdatabuffer0, Segment * segCount);
		QT_BoardSetupRecGetData(unCardIndex, 1, getdatabuffer1, Segment * segCount);

		//生成文件
		char Allfilepath[256] = { 0 };
		char Allfilename0[256] = { 0 };
		char Allfilename1[256] = { 0 };
		sprintf(Allfilename0, "%sadc_std_multi_0.bin", Allfilepath, "");
		sprintf(Allfilename1, "%sadc_std_multi_1.bin", Allfilepath, "");

		FILE* fp0 = fopen(Allfilename0, "wb+");
		if (NULL == fp0)
		{
			return 0;
		}

		FILE* fp1 = fopen(Allfilename1, "wb+");
		if (NULL == fp1)
		{
			return 0;
		}

		fwrite(getdatabuffer0, 1, Segment * segCount, fp0);
		fwrite(getdatabuffer1, 1, Segment * segCount, fp1);

		fclose(fp0);
		fp0 = NULL;
		free(getdatabuffer0);
		getdatabuffer0 = NULL;
		fclose(fp1);
		fp1 = NULL;
		free(getdatabuffer1);
		getdatabuffer1 = NULL;
	}

	//上位机停止采集ADC数据
	QT_BoardSetupRecCollectData(unCardIndex, 0, 1);

	return 0;
}

int QT_BoardSetRecFIFOSingle(int blockNum)
{
	//无限点单次触发采集模式
	uint32_t Frameswitch = 0;
	uint32_t Pretrigdots = 0;
	uint32_t Segment = 0;
	printf("设置段长(单位:字节):");
	scanf("%d", &Segment);
	printf("设置帧头(0:禁用  1:使能):");
	scanf("%d", &Frameswitch);
	printf("设置预触发(单位:点数):");
	scanf("%d", &Pretrigdots);
	Segment = Segment / 64 * 64;
	if (blockNum == 1)
	{
		QT_BoardSetupModeRecFIFOSingle(unCardIndex, 0, Segment, Frameswitch, Pretrigdots);
	}
	else if (blockNum == 2)
	{
		QT_BoardSetupModeRecFIFOSingle(unCardIndex, 0, Segment, Frameswitch, Pretrigdots);
		QT_BoardSetupModeRecFIFOSingle(unCardIndex, 1, Segment, Frameswitch, Pretrigdots);
	}

	//初始化buffer
	QT_BoardSetInitializeOperation(unCardIndex);

	//设置文件文件路径文件大小文件名
	char filepath[128] = { "./" };
	uint64_t fileSize = (uint64_t)570 * 1024 * 1024;
	printf("输入文件存储路径:");
	scanf("%s", &filepath);
	printf("输入文件大小(单位:字节):");
	scanf("%llu", &fileSize);
	const char* filename = "0";
	QT_BoardSetTheFilePathSizeName(unCardIndex, 0, filepath, fileSize, filename);


	//上位机启动采集ADC数据
	QT_BoardSetupRecCollectData(unCardIndex, 1, 1);

	char ch;
	int is_enter;
	while (1)
	{
		printf("\nPress 'q' + 'Enter' to stop collect!\n");
		while ('\n' == (ch = (char)getchar()));
		is_enter = getchar();
		if ('q' == ch && is_enter == 10)
		{
			//上位机停止采集ADC数据
			QT_BoardSetupRecCollectData(unCardIndex, 0, 1);

			//释放buffer
			QT_BoardSetFreeOperation(unCardIndex);

			break;
		}
		else
		{
			printf("printf\n");
			while ('\n' != (ch = (char)getchar()));
			printf("input invaild! please try again.\n");
		}
	}

	return 0;
}

int QT_BoardSetRecFIFOMulti(int blockNum)
{
	//有限点单次触发采集模式
	uint32_t Frameswitch = 0;
	uint32_t Pretrigdots = 0;
	double RepetitionFrequency = 1;
	uint32_t Segment = 81920;
	//printf("设置段长(单位:字节):");
	////scanf("%d", &Segment);
	//printf("设置帧头(0:禁用  1:使能):");
	////scanf("%d", &Frameswitch);
	//printf("设置预触发(单位:点数):");
	////scanf("%d", &Pretrigdots);
	//printf("设置触发频率(单位:Hz):");
	////scanf("%lf", &RepetitionFrequency);
	Segment = Segment / 64 * 64;
	if (blockNum == 1)
	{
		QT_BoardSetupModeRecFIFOMulti(unCardIndex, 0, Segment, Frameswitch, Pretrigdots, RepetitionFrequency);
	}
	else if (blockNum == 2)
	{
		QT_BoardSetupModeRecFIFOMulti(unCardIndex, 0, Segment, Frameswitch, Pretrigdots, RepetitionFrequency);
		QT_BoardSetupModeRecFIFOMulti(unCardIndex, 1, Segment, Frameswitch, Pretrigdots, RepetitionFrequency);
	}

	////初始化buffer
	//QT_BoardSetInitializeOperation(unCardIndex);

	////设置文件文件路径文件大小文件名
	//char filepath[128] = { "./" };
	//uint64_t fileSize = (uint64_t)570 * 1024 * 1024;
	//printf("输入文件存储路径:");
	//scanf("%s", &filepath);
	//printf("输入文件大小(单位:字节):");
	//scanf("%llu", &fileSize);
	//const char* filename = "0";
	//QT_BoardSetTheFilePathSizeName(unCardIndex, 0, filepath, fileSize, filename);
	//QT_BoardIfStoreFiles(unCardIndex, 0);

	//上位机启动采集ADC数据
	//QT_BoardSetupRecCollectData(unCardIndex, 1, 0);
	for (int i = 0; i < 1; i++) {

		QT_BoardSetInitializeOperation(unCardIndex);//初始化buffer
		QT_BoardSetupRecCollectData(unCardIndex, 1, 0);//开始采集
		long long count = 0;
		uint32_t buffersize = Segment * 250;
		unsigned char* data = (unsigned char*)malloc(buffersize);
		if (data == NULL) {
			return -1;
		}
		memset(data, 0, buffersize);
		uint32_t times = GetTickCount();
		uint32_t total_bytes = 0; //累计字节数
		while (1) {
			int a = 0;
			LARGE_INTEGER frequency;
			QueryPerformanceFrequency(&frequency);
			LARGE_INTEGER start, end;
			QueryPerformanceCounter(&start);
			a = QT_BoardGetFIFODataBuffer(unCardIndex, data, buffersize, 0x7FFFFFFF);
			QueryPerformanceCounter(&end);
			double elapsedTime = static_cast<double>(end.QuadPart - start.QuadPart) / frequency.QuadPart;
			count++;
			uint32_t bytes_this_times = a;
			total_bytes += a;
			printf("第%u次取数：字节数=%u，耗时=%.3fms，总字节数=%u\r",
				count, bytes_this_times, elapsedTime * 1000.0, total_bytes);
			if (a <= 0) {
				fprintf(stderr, "\n数据采集错误: %d\n", a);
				break;
			}
			if (a == -1) {
				printf("超时.....\n");
				printf("count = %d\n", count);
				break;
			}
			else if (a == -2) {
				printf("buffer溢出，取数慢.....a is %d\n", a);
				printf("count = %d\n", count);
				break;
			}
		}
	}


	char ch;
	int is_enter;
	uint8_t* getdatabuffer0 = (uint8_t*)malloc(64 * 1024 * 1024);
	if (NULL == getdatabuffer0)
	{
		printf("申请缓存getdatabuffer0失败\n");
		return 0;
	}
	int ii = 0;
	memset(getdatabuffer0, 0, 64 * 1024 * 1024);
	while (1)
	{
		/*printf("\nPress 'q' + 'Enter' to stop collect!\n");
		while ('\n' == (ch = (char)getchar()));
		is_enter = getchar();*/

		int err = QT_BoardGetFIFODataBuffer(unCardIndex, getdatabuffer0, 64 * 1024 * 1024, 0x7fffffff);
		ii++;
		printf("ii: %d \n err: %d \n", ii, err);

		if (ii == 700)
		{
			//上位机停止采集ADC数据
			QT_BoardSetupRecCollectData(unCardIndex, 0, 0);

			//释放buffer
			QT_BoardSetFreeOperation(unCardIndex);

			break;
		}
	}

	return 0;
}

int QT_BoardSetRepDDSPlay()
{
	//设置DDS播放频率
	puts("设置DDS播放频率(单位:MHz):");
	uint32_t dbFreq = 0;
	scanf("%d", &dbFreq);
	QT_BoardSetupRepSourceDDS(unCardIndex, dbFreq);

	char ch;
	int is_enter;
	while (1)
	{
		printf("\nPress 'q' + 'Enter' to exit!\n");
		while ('\n' == (ch = (char)getchar()));
		is_enter = getchar();
		if ('q' == ch && is_enter == 10)
		{
			//停止播放
			QT_BoardSetupRepPlayData(unCardIndex, 0);
			break;
		}
		else
		{
			printf("printf\n");
			while ('\n' != (ch = (char)getchar()));
			printf("input invaild! please try again.\n");
		}
	}

	return 0;
}

int QT_BoardSetRepFilePlay(int blockNum)
{
	char hfilehandle[128] = { "../IQ_2chan_40Mtone.bin" };
	printf("播放文件(文件路径及文件名):");
	scanf("%s", &hfilehandle);
	FILE* hFileHandle = NULL;
	hFileHandle = fopen(hfilehandle, "rb");
	if (hFileHandle == NULL)
	{
		printf("打开文件失败\n");
		return -1;
	}

	unsigned char* buffer = NULL;
	fseek(hFileHandle, 0, SEEK_END);//移动文件指针到末尾
	long long MBytes = 0; fgetpos(hFileHandle, &MBytes);//获取文件长度
	fseek(hFileHandle, 0, SEEK_SET);//移动文件指针到开始

	uint32_t segmentB = 0;
	printf("播放数据量(单位:字节):");
	scanf("%d", &segmentB);
	if (segmentB > MBytes)
	{
		printf("播放段长大于文件长度\n");
		return -1;
	}

	temp = QT_BoardIfSupportFilePlaySource(unCardIndex);
	if (temp == 0)//DMA
	{
		int loopflag = 0; int loopcount = 0;//00:单次   10:连续(当loopflag=1的时候，loopcount可设置)
		printf("设置播放模式(0:单次播放 1:连续播放):");
		scanf("%d", &loopflag);
		if (loopflag == 1)
		{
			printf("设置连续播放次数(0:无限次):");
			scanf("%d", &loopcount);
		}

		if (blockNum == 1)
		{
			QT_BoardSetupRepSourceFileDMA(unCardIndex, 0, loopflag, loopcount, hfilehandle, segmentB);
		}
		else if (blockNum == 2)
		{

			QT_BoardSetupRepSourceFileDMA(unCardIndex, 0, loopflag, loopcount, hfilehandle, segmentB);
			QT_BoardSetupRepSourceFileDMA(unCardIndex, 1, loopflag, loopcount, hfilehandle, segmentB);
		}
	}
	else if (temp == 1)//DATAMOVE
	{
		QT_BoardSetupRepSourceFileDatamove(unCardIndex, hfilehandle, segmentB);
	}

	//开始播放
	QT_BoardSetupRepPlayData(unCardIndex, 1);

	char ch;
	int is_enter;
	while (1)
	{
		printf("\nPress 'q' + 'Enter' to exit!\n");
		while ('\n' == (ch = (char)getchar()));
		is_enter = getchar();
		if ('q' == ch && is_enter == 10)
		{
			//停止播放
			QT_BoardSetupRepPlayData(unCardIndex, 0);
			break;
		}
		else
		{
			printf("printf\n");
			while ('\n' != (ch = (char)getchar()));
			printf("input invaild! please try again.\n");
		}
	}

	return 0;
}

int QT_BoardSet9009RFParam(int workMode)
{
	temp = QT_BoardIfSupport9009Rf(unCardIndex);
	if (temp == 0)
	{
		//切换参考源
		int clockmode = 0;
		printf("设置时钟模式(0:内参考  1:外参考(30.72mhz, 5dBm)):");
		scanf("%d", &clockmode);
		QT_BoardSetRefClockMode(unCardIndex, clockmode);

		//设置本振频率
		int freq = 0;
		printf("设置本振频率(单位:KHz(226000khz<=freq<=5903000khz)):");
		scanf("%d", &freq);
		QT_BoardSetUpdateFrequency(unCardIndex, freq);

		if (workMode == 0 || workMode == 1 || workMode == 2 || workMode == 3)
		{
			//设置软件触发
			QT_BoardSetupTrigRecSoftware(unCardIndex);

			//设置采集数据类型
			int dataType = 0;
			printf("设置采集数据类型(0:算法数据（对原始数据滤波） 1:adc原始数据 2:无符号累加数据（4 通道，16bit，无符号累加数，采样率 245.76mhz） 3:无符号累加数据（2 通道，32bit，无符号累加数，采样率 245.76mhz）):");
			scanf("%d", &dataType);
			QT_BoardSetRecDataType(unCardIndex, dataType);

			//设置接收数据带宽
			int bandWidth = 0;
			printf("设置接收数据带宽(1:复数带宽50M 2:复数带宽75M 3:复数带宽100M 4:复数带宽125M 5:复数带宽150M 6:复数带宽175M 7:复数带宽200M):");
			scanf("%d", &bandWidth);
			QT_BoardSetRecBandwidth(unCardIndex, bandWidth);

			//设置接收衰减
			double recAttenuation = 0;
			printf("设置接收衰减(0dB~30dB(步进:0.5dB)):");
			scanf("%lf", &recAttenuation);
			QT_BoardSetRecAttenuation(unCardIndex, recAttenuation);
		}
		else if (workMode == 4 || workMode == 5)
		{
			//设置软件触发
			QT_BoardSetupTrigRepSoftware(unCardIndex);

			//设置发射衰减
			double repAttenuation = 0;
			printf("设置发射衰减(0dB~41.5dB(步进:0.5dB)):");
			scanf("%lf", &repAttenuation);
			QT_BoardSetRepAttenuation(unCardIndex, repAttenuation);
		}
	}

	return 0;
}
