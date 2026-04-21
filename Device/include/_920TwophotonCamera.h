#pragma once
#include "DeviceBase.h"
#include "CommonAPI.h"
#include "NIDAQmx.h"
#include "ImageBufferManagerAPI.h"
#include "galvo.h"
#include "clock.h"
#include "QT12136DC.h"
#include <atomic>
#include "ImgReconstruction.h"
#include "DEBUG.h"
using namespace std;
extern const char* g_920CameraDeviceName;
#define DAQmxErrChk(functionCall) if( DAQmxFailed(error=(functionCall)) ) goto Error; else
void calculate_achievable_frequencies(
	double desired_fps,
	int resolution,
	int over_resolution,
	const double TIMEBASE_HZ,
	// 使用引用作为输出参数
	double& actual_fps,
	double& actual_line_freq,
	double& actual_pixel_freq);
struct ImgDataSetting
{
	uint32_t buffersize;
	int nbLine;
	int nbChannel;
	int SampleRate;
	int dataResolution;		// 单位：byte
	int imgResolution;		// 图像分辨率
};
class CameraThread;
class _920TwophotonCamera : public CCameraBase<_920TwophotonCamera>
{
	friend class CameraThread;
public:
	_920TwophotonCamera();
	~_920TwophotonCamera();

	// MM::Device
	int		Initialize() override;
	int		Shutdown() override;
	void	GetName(char* name) const override;
	void	SetParentID(const char* parentId) override;
	void	GetParentID(char* parentID) const override;
	bool 	Busy() override;

	// MM::Camera
	int		SnapImage() override;
	const unsigned char* GetImageBuffer() override;
	const unsigned char* GetImageBuffer(unsigned channelNr) override;
	unsigned GetNumberOfComponents() const override;
	int unsigned 	GetNumberOfChannels() const override;
	int		GetChannelName(unsigned channel, char* name) override;
	long	GetImageBufferSize() const override;
	unsigned GetImageWidth() const override;
	unsigned GetImageHeight() const override;
	unsigned GetImageBytesPerPixel() const override;
	unsigned GetBitDepth() const override;
	double 	GetPixelSizeUm() const override;
	int 	GetBinning() const override;
	int 	SetBinning(int binSize) override;
	void 	SetExposure(double exp_ms) override;
	double 	GetExposure() const override;
	int 	SetROI(unsigned x, unsigned y, unsigned xSize, unsigned ySize) override;
	int 	GetROI(unsigned& x, unsigned& y, unsigned& xSize, unsigned& ySize) override;
	int 	ClearROI() override;
	bool 	SupportsMultiROI() override;
	bool 	IsMultiROISet() override;
	int 	GetMultiROICount(unsigned& count) override;
	int 	SetMultiROI(const unsigned* xs, const unsigned* ys, const unsigned* widths, const unsigned* heights, unsigned numROIs) override;
	int 	GetMultiROI(unsigned* xs, unsigned* ys, unsigned* widths, unsigned* heights, unsigned* length) override;
	int 	StartSequenceAcquisition(long numImages, double interval_ms, bool stopOnOverflow) override;
	int 	StartSequenceAcquisition(double interval_ms) override;
	int 	StopSequenceAcquisition() override;
	int 	PrepareSequenceAcqusition() override;
	bool 	IsCapturing() override;
	//void 	GetTags(char* serializedMetadata) override;
	//void 	AddTag(const char* key, const char* deviceLabel, const char* value) override;
	//void 	RemoveTag(const char* key) override;
	int 	IsExposureSequenceable(bool& isSequenceable) const override;
	int 	GetExposureSequenceMaxLength(long& nrEvents) const override;
	int 	StartExposureSequence() override;
	int 	StopExposureSequence() override;
	int 	ClearExposureSequence() override;
	int 	AddToExposureSequence(double exposureTime_ms) override;
	int 	SendExposureSequence() const override;

protected:
	int		ThreadRun(void) override;
	int		InsertImage() override;
private:
	int OnClock(MM::PropertyBase* pProp, MM::ActionType eAct);
	int OnGalvo(MM::PropertyBase* pProp, MM::ActionType eAct);
	int OnFps(MM::PropertyBase* pProp, MM::ActionType eAct);
	int OnResolution(MM::PropertyBase* pProp, MM::ActionType eAct);
	int OnFOV(MM::PropertyBase* pProp, MM::ActionType eAct);
	int OnOffset(MM::PropertyBase* pProp, MM::ActionType eAct);
	int OnChannel(MM::PropertyBase* pProp, MM::ActionType eAct);
	int OnBinning(MM::PropertyBase* pProp, MM::ActionType eAct);
	int OnClockControl(MM::PropertyBase* pProp, MM::ActionType eAct);
	int OnPhase(MM::PropertyBase* pProp, MM::ActionType eAct);

	void InitBuffer();
	void UpdateBuffer();
	int StartImaging(const bool isRetriggerable);
	int AbortImaging();
	int ClockUpdate(bool ContSample, int numImages = 1);
	int GalvoUpdate();
	int ClockStart();
	int GalvoStart(const bool isRetriggerable);
	int ClockStop();
	int GalvoStop();
	int QT12136DCSetting();
	int QT12136DCSetting2();
	int ClockOutput();
	int SetNIDAQDigtal(const string& port, bool volt);

	std::vector<unsigned char> m_imageBuffer;
	struct ROISetting
	{
		double x;
		double y;
		double xSize;
		double ySize;
		double voltsPerMicrometer;
	};
	ROISetting m_ROI;
	struct imageSetting
	{
		double fps;
		int resolution;
		int channel;
		int m_phase_pixel;
		uint32_t m_overResolution;
		inline void UpdataOverResolution() {
			this->m_overResolution = 2 * abs(m_phase_pixel) + this->resolution;
		}
	};
	imageSetting m_ImgSetting;

	Clock* pClock;
	Galvo* pGalvo;
	QT12136DC* pQT12136dc;

	bool m_initialized;
	unsigned m_img_width;
	unsigned m_img_height;
	unsigned m_img_bytesPerPixel;
	unsigned m_img_numChannels;
	unsigned m_img_numChunks;
	string m_parentId;
	atomic<bool> m_busy;
	bool m_isLive;
	struct galvo
	{
		Galvo::ChannelProperty pChannelProperty;
		double offset;
	};
	int m_LinePerAcq;
	galvo m_galvo_x;
	galvo m_galvo_y;
	string m_galvo_clockSource;
	string m_galvo_triggerSource;
	Clock::TaskProperty m_clock_pixel;
	Clock::TaskProperty m_clock_line;
	Clock::TaskProperty m_clock_frame;
	ImgDataSetting m_ImgDataSetting;
	BaseSequenceThread * thd_;
	CameraThread* camThd_;
	string m_ClockControl;
	string m_PMT1enable;
	string m_PMT2enable;
	bool m_PMT1enable_flag;
	bool m_PMT2enable_flag;
	bool snap_in_progress_;

#ifdef DEBUG
private:
	int OnClockControlOutput(MM::PropertyBase* pProp, MM::ActionType eAct);
	int OnGalvoOutput(MM::PropertyBase* pProp, MM::ActionType eAct);
	int OnPMT1ENABLE(MM::PropertyBase* pProp, MM::ActionType eAct);
	int OnPMT2ENABLE(MM::PropertyBase* pProp, MM::ActionType eAct);
#endif // DEBUG

};

class CameraThread : public MMDeviceThreadBase
{
	friend class _920TwophotonCamera;
public:
	explicit CameraThread(_920TwophotonCamera* cam)
		: cam_(cam),m_running(false), m_stop(false)
	{
		IR::init();
	}
	inline ~CameraThread()
	{

	}
	int svc() override;
	int start(uint32_t phase);
	void stop()
	{
		m_stop = true;
	}
	bool IsRunning() const
	{
		return m_running.load();
	}
private:
	_920TwophotonCamera* cam_;
	std::atomic<bool> m_running;
	std::atomic<bool> m_stop;
	uint32_t m_phase;
};