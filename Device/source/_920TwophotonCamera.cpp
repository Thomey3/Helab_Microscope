#include "_920TwophotonCamera.h"
#include <algorithm>

const char* g_920CameraDeviceName = "920Camera";

_920TwophotonCamera::_920TwophotonCamera() :  
   m_ImgSetting{1.0, 512, 2, 30, 572},  
   m_ROI{0, 0, 525, 525, 0.01},  
   m_clock_pixel{ "PixelClock", "/Dev1", "/ctr0", "/PFI9", 512 * 512, 0.5, 512 * 512, Clock::TimebaseSource::HZ_20_MHZ },
   m_clock_line{ "LineClock", "/Dev1", "/ctr1", "/PFI4", 512 * 512, 0.5, 512 * 512, Clock::TimebaseSource::HZ_20_MHZ },
   m_clock_frame{ "FrameClock", "/Dev1", "/ctr2", "/PFI1", 512 * 512, 0.5, 512 * 512, Clock::TimebaseSource::HZ_20_MHZ },
   m_galvo_x{{"/Dev1/ao0", {0}, 0}, 0},  
   m_galvo_y{{"/Dev1/ao1", {0}, 0}, 0},  
	m_ImgDataSetting{ 0, 0, 0, 1000000000, 2 },
   m_isLive(false),  
   m_img_bytesPerPixel(0),  
   m_img_height(0),  
   m_img_numChannels(0),  
   m_img_numChunks(0),  
   m_img_width(0),  
   m_initialized(false),  
   pClock(nullptr),  
   pGalvo(nullptr),  
   pQT12136dc(nullptr),  
   thd_(nullptr),
   camThd_(nullptr),
   m_LinePerAcq(1),
   snap_in_progress_(false),
   m_PMT1enable("/Dev1/port0/line14"),
   m_PMT2enable("/Dev1/port0/line15"),
   m_PMT1enable_flag(false),
   m_PMT2enable_flag(false)
{  
   vector<string> devices = NIDAQmxUtils::getAvailableDevices();  
   vector<string> ao;  
   vector<string> do_lines;  
   vector<string> co;  
   vector<string> PFI;  
   int err;  
   for (const auto& dev : devices) {  
       vector<string> tmp = NIDAQmxUtils::getAOChannels(dev);  
       ao.insert(ao.end(), tmp.begin(), tmp.end());  
       tmp = NIDAQmxUtils::getDOLines(dev);  
       do_lines.insert(do_lines.end(), tmp.begin(), tmp.end());  
       tmp = NIDAQmxUtils::getCOChannels(dev);  
       co.insert(co.end(), tmp.begin(), tmp.end());  
       tmp = NIDAQmxUtils::getPFIChannels(dev);  
       PFI.insert(PFI.end(), tmp.begin(), tmp.end());  
   }  
   err = CreateStringProperty("a. Pixel Clock ctr channel", co[0].c_str(), false, new CPropertyAction(this, &_920TwophotonCamera::OnClock), true);
   err = CreateStringProperty("b. Pixel Clock PFI channel", PFI[0].c_str(), false, new CPropertyAction(this, &_920TwophotonCamera::OnClock), true);
   err = CreateStringProperty("c. Line  Clock ctr channel", co[0].c_str(), false, new CPropertyAction(this, &_920TwophotonCamera::OnClock), true);
   err = CreateStringProperty("d. Line  Clock PFI channel", PFI[0].c_str(), false, new CPropertyAction(this, &_920TwophotonCamera::OnClock), true);
   err = CreateStringProperty("e. Frame Clock ctr channel", co[0].c_str(), false, new CPropertyAction(this, &_920TwophotonCamera::OnClock), true);
   err = CreateStringProperty("f. Frame Clock PFI channel", PFI[0].c_str(), false, new CPropertyAction(this, &_920TwophotonCamera::OnClock), true);
   err = CreateStringProperty("g. Galvo-X ao    channel", ao[0].c_str(), false, new CPropertyAction(this, &_920TwophotonCamera::OnGalvo), true);
   err = CreateStringProperty("h. Galvo-Y ao    channel", ao[0].c_str(), false, new CPropertyAction(this, &_920TwophotonCamera::OnGalvo), true);
   err = CreateStringProperty("i. Galvo clk source channel", co[0].c_str(), false, new CPropertyAction(this, &_920TwophotonCamera::OnGalvo), true);
   err = CreateStringProperty("j. Galvo tri source channel", co[0].c_str(), false, new CPropertyAction(this, &_920TwophotonCamera::OnGalvo), true);
   err = CreateStringProperty("k. Clock output control", (devices[0] + "/port0/line0").c_str(), false, new CPropertyAction(this, &_920TwophotonCamera::OnClockControl), true);
   for (std::string device : co) {  
       AddAllowedValue("a. Pixel Clock ctr channel", device.c_str());  
       AddAllowedValue("c. Line  Clock ctr channel", device.c_str());  
       AddAllowedValue("e. Frame Clock ctr channel", device.c_str());  
       string tmp = device + "InternalOutput";  
       AddAllowedValue("i. Galvo clk source channel", tmp.c_str());  
       AddAllowedValue("j. Galvo tri source channel", tmp.c_str());  
   }  
   for (std::string device : ao) {  
       AddAllowedValue("g. Galvo-X ao    channel", device.c_str());  
       AddAllowedValue("h. Galvo-Y ao    channel", device.c_str());  
   }  
   for (std::string device : PFI) {  
       AddAllowedValue("b. Pixel Clock PFI channel", device.c_str());  
       AddAllowedValue("d. Line  Clock PFI channel", device.c_str());  
       AddAllowedValue("f. Frame Clock PFI channel", device.c_str());  
   }  
}

_920TwophotonCamera::~_920TwophotonCamera()
{
	Shutdown();
}

int _920TwophotonCamera::Initialize()
{
    pClock = static_cast<Clock*>(GetDevice(g_DeviceNameClock));
    pGalvo = static_cast<Galvo*>(GetDevice(g_DeviceNameGalvo));
    pQT12136dc = static_cast<QT12136DC*>(GetDevice(g_DeviceNameQT12136DC));
    pGalvo->SetClockSource(m_galvo_clockSource);
    pGalvo->SetTriggerSource(m_galvo_triggerSource);

    CreateFloatProperty("Fps(Hz)", (double)m_ImgSetting.fps, false, new CPropertyAction(this, &_920TwophotonCamera::OnFps), false);
    CreateIntegerProperty("Resolution", (long)m_ImgSetting.resolution, false, new CPropertyAction(this, &_920TwophotonCamera::OnResolution), false);
    CreateFloatProperty("xSize(um)", (double)m_ROI.xSize, false, new CPropertyAction(this, &_920TwophotonCamera::OnFOV), false);
    CreateFloatProperty("ySize(um)", (double)m_ROI.ySize, false, new CPropertyAction(this, &_920TwophotonCamera::OnFOV), false);
    CreateFloatProperty("offsetX", (double)m_galvo_x.offset, false, new CPropertyAction(this, &_920TwophotonCamera::OnOffset), false);
    CreateFloatProperty("offsetY", (double)m_galvo_y.offset, false, new CPropertyAction(this, &_920TwophotonCamera::OnOffset), false);
    CreateIntegerProperty("Channel", (long)m_ImgSetting.channel, false, new CPropertyAction(this, &_920TwophotonCamera::OnChannel), false);
    CreateIntegerProperty(MM::g_Keyword_Binning, 1, false, new CPropertyAction(this, &_920TwophotonCamera::OnBinning));
    CreateIntegerProperty("Phase", 1, false, new CPropertyAction(this, &_920TwophotonCamera::OnPhase));
    AddAllowedValue(MM::g_Keyword_Binning, "1");
    ib_init(m_ImgSetting.resolution, m_ImgSetting.resolution, 2,m_ImgSetting.channel, m_ImgSetting.resolution / m_LinePerAcq);
    InitBuffer();
    ClockUpdate(false);
    pClock->addTask(m_clock_pixel);
    pClock->addTask(m_clock_line);
    pClock->addTask(m_clock_frame);
    GalvoUpdate();
    pGalvo->AddChannel(m_galvo_x.pChannelProperty);
    pGalvo->AddChannel(m_galvo_y.pChannelProperty);
    thd_ = new BaseSequenceThread(this);
    camThd_ = new CameraThread(this);
#ifdef DEBUG
    CreateIntegerProperty("ClockOutput", 1, false, new CPropertyAction(this, &_920TwophotonCamera::OnClockControlOutput));
    AddAllowedValue("ClockOutput", "0");
    AddAllowedValue("ClockOutput", "1");
    AddAllowedValue("ClockOutput", "2");
    CreateIntegerProperty("GalvoOutput", 1, false, new CPropertyAction(this, &_920TwophotonCamera::OnGalvoOutput));
    AddAllowedValue("GalvoOutput", "0");
    AddAllowedValue("GalvoOutput", "1");
    CreateIntegerProperty("PMT1ENABLE", 1, false, new CPropertyAction(this, &_920TwophotonCamera::OnPMT1ENABLE));
    AddAllowedValue("PMT1ENABLE", "0");
    AddAllowedValue("PMT1ENABLE", "1");
    CreateIntegerProperty("PMT2ENABLE", 1, false, new CPropertyAction(this, &_920TwophotonCamera::OnPMT2ENABLE));
    AddAllowedValue("PMT2ENABLE", "0");
    AddAllowedValue("PMT2ENABLE", "1");
#endif // DEBUG

	return DEVICE_OK;
}

int _920TwophotonCamera::Shutdown()
{
    SetNIDAQDigtal(m_PMT1enable, 0);
    SetNIDAQDigtal(m_PMT2enable, 0);
    AbortImaging();
    ClockStop();
    ib_shutdown();
	return DEVICE_OK;
}

void _920TwophotonCamera::GetName(char* name) const
{
	// Return the name used to referr to this device adapte
	CDeviceUtils::CopyLimitedString(name, g_920CameraDeviceName);
}

void _920TwophotonCamera::SetParentID(const char* parentId)
{
	m_parentId = parentId;
}

void _920TwophotonCamera::GetParentID(char* parentID) const
{
	CDeviceUtils::CopyLimitedString(parentID, m_parentId.c_str());
}

bool _920TwophotonCamera::Busy()
{
    bool tmp = m_busy.load();
	return tmp;
}

int	_920TwophotonCamera::SnapImage()
{
//    AbortImaging();
    if (Busy()) return DEVICE_ERR;
    m_busy.store(true);
    snap_in_progress_ = true; // 在开始成像前，设置标志位
    m_isLive = false;
    ClockUpdate(true);
    GalvoUpdate();
    StartImaging(false);
    return DEVICE_OK;
}

const unsigned char* _920TwophotonCamera::GetImageBuffer()
{
    // 1. 从管理器更新内部缓冲区
    UpdateBuffer();

    // 2. 检查数据和索引有效性
    if (m_imageBuffer.empty()) {
        return nullptr;
    }
    return m_imageBuffer.data();
}

const unsigned char* _920TwophotonCamera::GetImageBuffer(unsigned channelNr)
{
    // 首先从管理器更新内部缓冲区
    UpdateBuffer();

    // 检查数据和索引有效性
    if (m_imageBuffer.empty() || channelNr >= m_ImgSetting.channel) {
        return nullptr;
    }

    // 计算并返回指针
    const size_t channel_size = static_cast<size_t>(m_ImgSetting.resolution) * m_ImgSetting.resolution * 2;
    return m_imageBuffer.data() + channelNr * channel_size;
}

unsigned _920TwophotonCamera::GetNumberOfComponents() const
{
    return 1;
}

int unsigned _920TwophotonCamera::GetNumberOfChannels() const
{
    return m_ImgSetting.channel;
}

int _920TwophotonCamera::GetChannelName(unsigned channel, char* name)
{
    switch (channel)
    {
    case 0:
        // 将通道0的名字设置为 "Red"
        CDeviceUtils::CopyLimitedString(name, "Channel 1");
        break;
    case 1:
        // 将通道1的名字设置为 "Green"
        CDeviceUtils::CopyLimitedString(name, "Channel 2");
        break;
    }
    return DEVICE_OK;
}

long _920TwophotonCamera::GetImageBufferSize() const
{
    return m_ImgSetting.resolution * m_ImgSetting.resolution * 2;
}

unsigned _920TwophotonCamera::GetImageWidth() const
{
    return m_ImgSetting.resolution;
}

unsigned _920TwophotonCamera::GetImageHeight() const
{
    return m_ImgSetting.resolution;
}

unsigned _920TwophotonCamera::GetImageBytesPerPixel() const
{
    return 2;
}

unsigned _920TwophotonCamera::GetBitDepth() const
{
    return 16;
}

double _920TwophotonCamera::GetPixelSizeUm() const
{
    return 1;
}

int _920TwophotonCamera::GetBinning() const
{
    return 1;
}

int _920TwophotonCamera::SetBinning(int binSize)
{
    return DEVICE_OK;
}

void _920TwophotonCamera::SetExposure(double exp_ms)
{
    m_ImgSetting.fps = exp_ms / 1000;
}

double _920TwophotonCamera::GetExposure() const
{
    return m_ImgSetting.fps * 1000;
}

int _920TwophotonCamera::SetROI(unsigned x, unsigned y, unsigned xSize, unsigned ySize)
{   
    return DEVICE_OK;
}

int _920TwophotonCamera::GetROI(unsigned& x, unsigned& y, unsigned& xSize, unsigned& ySize)
{
    return DEVICE_OK;
}

int _920TwophotonCamera::ClearROI()
{
    return DEVICE_OK;
}

bool _920TwophotonCamera::SupportsMultiROI()
{
    return false;
}

bool _920TwophotonCamera::IsMultiROISet()
{
    return false;
}

int _920TwophotonCamera::GetMultiROICount(unsigned& count)
{
    count = 0;
    return DEVICE_OK;
}

int _920TwophotonCamera::SetMultiROI(const unsigned* xs, const unsigned* ys, const unsigned* widths, const unsigned* heights, unsigned numROIs)
{
    return DEVICE_UNSUPPORTED_COMMAND;
}

int _920TwophotonCamera::GetMultiROI(unsigned* xs, unsigned* ys, unsigned* widths, unsigned* heights, unsigned* length)
{
    return DEVICE_UNSUPPORTED_COMMAND;
}

int _920TwophotonCamera::StartSequenceAcquisition(long numImages, double interval_ms, bool stopOnOverflow)
{
    if (Busy()) return DEVICE_ERR;
    if (GetCoreCallback()->PrepareForAcq(this) != DEVICE_OK) return DEVICE_ERR;
    m_busy.store(true);
    m_isLive = true;
    InitBuffer();
    ib_reset_image();
    ClockUpdate(true,numImages);
    GalvoUpdate();
    thd_->Start(numImages, interval_ms);
    StartImaging(true);
    return DEVICE_OK;
}

int _920TwophotonCamera::StartSequenceAcquisition(double interval_ms)
{
    return StartSequenceAcquisition(LONG_MAX, interval_ms, false);
}

int _920TwophotonCamera::StopSequenceAcquisition()
{
    // 1. 仅向MMCore的标准序列采集线程发送停止信号
        //    这个线程(thd_)的作用是调用InsertImage()将图像推送到MMCore的循环缓冲区。
    if (!thd_->IsStopped()) {
        thd_->Stop();
        thd_->wait(); // 等待该线程完全退出
    }

    // 2. 调用集中的清理函数
    //    这个函数会处理所有硬件和自定义线程的关闭。
    AbortImaging();

    m_busy.store(false);
    GetCoreCallback()->AcqFinished(this, 0);
    return DEVICE_OK;
}

// temporary debug methods
int _920TwophotonCamera::PrepareSequenceAcqusition()
{
    return DEVICE_OK;
}

bool _920TwophotonCamera::IsCapturing()
{
    return Busy();
}

int _920TwophotonCamera::IsExposureSequenceable(bool& isSequenceable) const
{
    isSequenceable = false;
    return DEVICE_OK;
}

int _920TwophotonCamera::GetExposureSequenceMaxLength(long& nrEvents) const
{
    nrEvents = 0;
    return DEVICE_OK;
}

int _920TwophotonCamera::StartExposureSequence()
{
    return DEVICE_UNSUPPORTED_COMMAND;
}

int _920TwophotonCamera::StopExposureSequence()
{
    return DEVICE_OK;
}

int _920TwophotonCamera::ClearExposureSequence()
{
    return DEVICE_OK;
}

int _920TwophotonCamera::AddToExposureSequence(double exposureTime_ms)
{
    return DEVICE_UNSUPPORTED_COMMAND;
}

int _920TwophotonCamera::SendExposureSequence() const
{
    return DEVICE_UNSUPPORTED_COMMAND;
}

int _920TwophotonCamera::ThreadRun(void)
{
    int ret = DEVICE_ERR;
    UpdateBuffer();
    Sleep(100);
    ret = InsertImage();
    if (ret != DEVICE_OK)
    {
        return ret;
    }
    return ret;
};  

int _920TwophotonCamera::InsertImage()
{
    int ret = DEVICE_ERR;
    char label[MM::MaxStrLength];

    GetLabel(label);

    MM::MMTime timestamp = GetCurrentMMTime();
    Metadata md;
    md.put(MM::g_Keyword_Metadata_CameraLabel, label);
    for (unsigned i = 0; i < GetNumberOfChannels(); i++)
    {
        char buf[MM::MaxStrLength];
        MetadataSingleTag mstChannel(MM::g_Keyword_CameraChannelIndex, label, true);
        snprintf(buf, MM::MaxStrLength, "%d", i);
        mstChannel.SetValue(buf);
        md.SetTag(mstChannel);

        MetadataSingleTag mstChannelName(MM::g_Keyword_CameraChannelName, label, true);
        GetChannelName(i, buf);
        mstChannelName.SetValue(buf);
        md.SetTag(mstChannelName);

        ret = GetCoreCallback()->InsertImage(this, GetImageBuffer(i),
            GetImageWidth(),
            GetImageHeight(),
            GetImageBytesPerPixel(),
            md.Serialize().c_str());
        if (ret == DEVICE_BUFFER_OVERFLOW) {
            GetCoreCallback()->ClearImageBuffer(this);
            GetCoreCallback()->InsertImage(this, GetImageBuffer(i),
                GetImageWidth(),
                GetImageHeight(),
                GetImageBytesPerPixel(),
                md.Serialize().c_str());
        }
        else if (ret != DEVICE_OK) {
            GetCoreCallback()->LogMessage(this, "InsertImage: error inserting image", true);
            return ret;
        }
    }
    return DEVICE_OK;
}

int _920TwophotonCamera::BaseSequenceThread::svc(){
    int ret = DEVICE_ERR;
    try
    {
        do
        {
            ret = camera_->ThreadRun();
            imageCounter_ = ib_get_completed_frame_count();
            //camera_->LogMessage("imageCounter:" + to_string(imageCounter_));
        } while (DEVICE_OK == ret && !IsStopped() && imageCounter_ < numImages_ - 1);
        if (IsStopped())
            camera_->LogMessage("SeqAcquisition interrupted by the user\n");

    }
    catch (...) {
        camera_->LogMessage(g_Msg_EXCEPTION_IN_THREAD, false);
    }
    stop_ = true;
    UpdateActualDuration();
    camera_->OnThreadExiting();
    return ret;
}

// ------------- 工具函数 ----------------
void _920TwophotonCamera::InitBuffer()
{
    // 1. 根据图像维度计算单个通道的大小，乘2是因为数据为两个字节
    const size_t channel_size = static_cast<size_t>(m_ImgSetting.resolution) * m_ImgSetting.resolution * 2;

    // 2. 计算所有通道所需的总大小
    const size_t total_size = channel_size * m_ImgSetting.channel;

    // 3. 为内部缓冲区分配内存，并初始化为0
    //    这样即使第一次获取图像失败，缓冲区也是一个有效的全黑图像。
    m_imageBuffer.resize(total_size, 0);
}

void _920TwophotonCamera::UpdateBuffer()
{
    if (!m_isLive) {
        if (snap_in_progress_) { // 仅当 snap 正在进行时才执行
            ib_get_snapshot(m_imageBuffer, -1);
            m_busy.store(false);
            AbortImaging();
            snap_in_progress_ = false; // 重置标志位，完成本次 snap
        }
    }
    else {
        ib_get_image_copy_with_tearing(m_imageBuffer);
    }
}

int _920TwophotonCamera::StartImaging(const bool isRetriggerable)
{
    InitBuffer();
    ib_reset_image();
    // ib_init is now called in QT12136DCSetting to ensure correct sequence

    GalvoStop();
    if (pQT12136dc) pQT12136dc->stop(); // Ensure stopped before start
    ClockStop();

    ClockStart();
    GalvoStart(isRetriggerable);

    //if (QT12136DCSetting() != DEVICE_OK) {
    //    // Stop hardware if settings fail
    //    GalvoStop();
    //    ClockStop();
    //    return DEVICE_ERR;
    //}

    if (QT12136DCSetting2() != DEVICE_OK) {
        // Stop hardware if settings fail
        GalvoStop();
        ClockStop();
        return DEVICE_ERR;
    }

    camThd_->start(m_ImgSetting.m_phase_pixel);
    if (pQT12136dc) pQT12136dc->start();
    if (m_PMT1enable_flag) SetNIDAQDigtal(m_PMT1enable, 1);
    if (m_PMT2enable_flag) SetNIDAQDigtal(m_PMT2enable, 1);
    ClockOutput();
    return DEVICE_OK;
}

int _920TwophotonCamera::AbortImaging()
{
    LogMessage("AbortImaging start.", true);
    // --- Step 1: Signal all threads to stop ---
    // 通知消费者线程(CameraThread)停止，它将不再从缓冲池取数据。
    if (camThd_ && camThd_->IsRunning()) {
        LogMessage("camThd_ stop start.", true);
        camThd_->stop();
    }
    // 通知生产者线程(QT12136DCThread)停止。
    // stop()现在应该只设置标志，不操作硬件。
    if (pQT12136dc) {
        LogMessage("pQT12136dc stop start.", true);
        pQT12136dc->signalStop();
    }
    bp_shutdown();
    // --- Step 2: Wait for all threads to exit gracefully ---
    // 等待消费者线程(CameraThread)处理完剩余数据并退出。
    if (camThd_ && camThd_->IsRunning()) {
        LogMessage("camThd_ wait start.", true);
        camThd_->wait();
    }
    // 等待生产者线程(QT12136DCThread)完成最后一次读操作并退出。
    if (pQT12136dc) {
        LogMessage("pQT12136dc wait start.", true);
        pQT12136dc->wait();
    }

    // --- Step 3: Threads are stopped, now safely shut down hardware & resources ---
    // 停止并清理硬件。
    GalvoStop();
    //ClockStop();

    // 调用QT设备的硬件关闭例程。
    if (pQT12136dc) {
        pQT12136dc->hardwareShutdown();
    }

    // 关闭缓冲池(BufferPool)。现在没有任何线程在访问它了。
    // bp_shutdown();

    // 重置图像缓冲区管理器。
    ib_reset_image();
    SetNIDAQDigtal(m_PMT1enable, 0);
    SetNIDAQDigtal(m_PMT2enable, 0);
    LogMessage("AbortImaging success.", true);
    return DEVICE_OK;
}

int _920TwophotonCamera::ClockUpdate(bool ContSample, int numImages)
{
    // 从设置中获取理想参数
    double desired_fps = (int)m_ImgSetting.fps;
    int resolution = m_ImgSetting.resolution;
    int over_resolution = m_ImgSetting.m_overResolution;
    const double TIMEBASE = 20000000.0;

    // 定义变量来接收计算结果
    double actual_fps, actual_line_freq, actual_pixel_freq;

    calculate_achievable_frequencies(
        desired_fps,
        resolution,
        over_resolution,
        TIMEBASE,
        actual_fps,
        actual_line_freq,
        actual_pixel_freq
    );
    //m_ImgSetting.fps = actual_fps;
    m_clock_pixel.timebaseSrc = Clock::TimebaseSource::HZ_20_MHZ;
    m_clock_line.timebaseSrc = Clock::TimebaseSource::HZ_20_MHZ;
    m_clock_frame.timebaseSrc = Clock::TimebaseSource::HZ_20_MHZ;

    m_clock_pixel.timebaseSrc = Clock::TimebaseSource::HZ_20_MHZ;
    m_clock_pixel.frequency = actual_pixel_freq;
    m_clock_pixel.pulsesPerRun = ContSample ? 0 : over_resolution * resolution * numImages;

    m_clock_line.timebaseSrc = Clock::TimebaseSource::HZ_20_MHZ;
    m_clock_line.frequency = actual_line_freq;
    m_clock_line.pulsesPerRun = ContSample ? 0 : resolution * numImages;

    m_clock_frame.timebaseSrc = Clock::TimebaseSource::HZ_20_MHZ;
    m_clock_frame.frequency = actual_fps;
    m_clock_frame.pulsesPerRun = ContSample ? 0 : numImages;
    auto taskPixel = pClock->resetTask(m_clock_pixel.TaskName, m_clock_pixel);
    auto taskLine = pClock->resetTask(m_clock_line.TaskName, m_clock_line);
    auto taskFrame = pClock->resetTask(m_clock_frame.TaskName, m_clock_frame);

    return DEVICE_OK;
}

int _920TwophotonCamera::GalvoUpdate()
{
    vector<double> sequence;
    sequence = WaveformGenerator::generateTriangleWave(m_galvo_x.offset, m_ROI.xSize * m_ROI.voltsPerMicrometer, m_ImgSetting.m_overResolution * 2, m_ImgSetting.resolution / 2);
    m_galvo_x.pChannelProperty.baseSequence = sequence;
    m_galvo_x.pChannelProperty.targetFrequency = m_ImgSetting.fps;
    sequence = WaveformGenerator::generateSawtoothWave(m_galvo_y.offset, m_ROI.ySize * m_ROI.voltsPerMicrometer, m_ImgSetting.resolution, 1);
    m_galvo_y.pChannelProperty.baseSequence = sequence;
    m_galvo_y.pChannelProperty.targetFrequency = m_ImgSetting.fps;
    return DEVICE_OK;
}

int _920TwophotonCamera::ClockStart()
{
    auto taskPixel = pClock->getTask(m_clock_pixel.TaskName);
    auto taskLine = pClock->getTask(m_clock_line.TaskName);
    auto taskFrame = pClock->getTask(m_clock_frame.TaskName);

    if (taskPixel) taskPixel->start();
    if (taskLine)  taskLine->start();
    if (taskFrame) taskFrame->start();
    return DEVICE_OK;
}

int _920TwophotonCamera::GalvoStart(const bool isRetriggerable)
{
	GalvoUpdate();
	pGalvo->AddChannel(m_galvo_x.pChannelProperty);
	pGalvo->AddChannel(m_galvo_y.pChannelProperty);
    pGalvo->Commit(m_ImgSetting.fps * m_ImgSetting.resolution * m_ImgSetting.m_overResolution, isRetriggerable);
    pGalvo->Start();
    return DEVICE_OK;
}

int _920TwophotonCamera::ClockStop()
{
    auto taskPixel = pClock->getTask(m_clock_pixel.TaskName);
    auto taskLine = pClock->getTask(m_clock_line.TaskName);
    auto taskFrame = pClock->getTask(m_clock_frame.TaskName);

    if (taskPixel) taskPixel->stop();
    if (taskLine)  taskLine->stop();
    if (taskFrame) taskFrame->stop();
    return DEVICE_OK;
}

int _920TwophotonCamera::GalvoStop()
{
    pGalvo->Stop();
    return DEVICE_OK;
}

int _920TwophotonCamera::QT12136DCSetting()
{
    int err;
	CardSettings cardsettings;
    cardsettings.AcquisitionMode = AcquisitionMode::MODE_INFINITE_MULTI;
    cardsettings.ChannelMask = ChannelMask::CH1_CH2;
    cardsettings.ChannelNumber = ChannelNumber::Two_Channels;
    cardsettings.ClockMode = ClockMode::Internal_Reference_Clock;
    cardsettings.DataType = DataType::Raw_Data;
    cardsettings.DMABlockNum = DMABlockNum::one_Block;
    cardsettings.extractMultiple = 0;
    cardsettings.TriggerMode = TriggerMode::TRIG_EXTERNAL_PULSE_RISING;
	err = pQT12136dc->SetCardSettings(cardsettings);
    if (err != DEVICE_OK) {
        LogMessage("CardSetting failure!", true);
        return err;
    }
    TriggerSettings Trigsettings;
	Trigsettings.trigDelay = 0; // 延迟触发长度
	Trigsettings.biasVoltage = 0; // 偏置电压
    err = pQT12136dc->SetTriggerSettings(Trigsettings);
    if (err != DEVICE_OK) {
        LogMessage("Trigsettings failure!", true);
        return err;
    }
    CollectSetting Collectsettings;
	Collectsettings.Frameswitch = 0; // 帧头
    Collectsettings.Pretrigdots = 0;
    Collectsettings.RepetitionFrequency = m_ImgSetting.fps * m_ImgSetting.resolution;
    Collectsettings.Segment = pQT12136dc->Cal_Segment(1 / (double)Collectsettings.RepetitionFrequency);
    Collectsettings.buffersize = Collectsettings.Segment * m_LinePerAcq;
    err = pQT12136dc->SetCollectSetting(Collectsettings);
    if (err != DEVICE_OK) {
        LogMessage("Collectsettings failure!", true);
        return err;
    }
    bufferConfig bufferConfig_;
    bufferConfig_.block_size = Collectsettings.buffersize;
    bufferConfig_.num_blocks = m_ImgSetting.resolution / m_LinePerAcq + 100;
    err = pQT12136dc->SetBufferConfig(bufferConfig_);
    if (err != DEVICE_OK) {
        LogMessage("bufferConfig_ failure!", true);
        return err;
    }
    err = IR::setup(bufferConfig_.block_size, m_ImgSetting.resolution, m_ImgSetting.resolution, m_ImgSetting.channel, m_LinePerAcq,m_ImgSetting.m_phase_pixel);
    if (err != DEVICE_OK) {
        LogMessage("IR::setup failure!", true);
        return err;
    }
    return DEVICE_OK;
}

// 按照pixel成像
int _920TwophotonCamera::QT12136DCSetting2()
{
    int err;
    CardSettings cardsettings;
    cardsettings.AcquisitionMode = AcquisitionMode::MODE_INFINITE_MULTI;
    cardsettings.ChannelMask = ChannelMask::CH1_CH2;
    cardsettings.ChannelNumber = ChannelNumber::Two_Channels;
    cardsettings.ClockMode = ClockMode::Internal_Reference_Clock;
    cardsettings.DataType = DataType::Raw_Data;
    cardsettings.DMABlockNum = DMABlockNum::one_Block;
    cardsettings.extractMultiple = 0;
    cardsettings.TriggerMode = TriggerMode::TRIG_EXTERNAL_PULSE_RISING;
    err = pQT12136dc->SetCardSettings(cardsettings);
    if (err != DEVICE_OK) {
        LogMessage("CardSetting failure!", true);
        return err;
    }
    TriggerSettings Trigsettings;
    Trigsettings.trigDelay = 0; // 延迟触发长度
    Trigsettings.biasVoltage = 0; // 偏置电压
    err = pQT12136dc->SetTriggerSettings(Trigsettings);
    if (err != DEVICE_OK) {
        LogMessage("Trigsettings failure!", true);
        return err;
    }
    CollectSetting Collectsettings;
    Collectsettings.Frameswitch = 0; // 帧头
    Collectsettings.Pretrigdots = 0;
    Collectsettings.RepetitionFrequency = m_ImgSetting.fps * m_ImgSetting.resolution * m_ImgSetting.m_overResolution;
    Collectsettings.Segment = pQT12136dc->Cal_Segment(1 / (double)Collectsettings.RepetitionFrequency);
    Collectsettings.buffersize = Collectsettings.Segment * m_LinePerAcq * m_ImgSetting.m_overResolution / 4 * cardsettings.ChannelNumber;
    err = pQT12136dc->SetCollectSetting(Collectsettings);
    if (err != DEVICE_OK) {
        LogMessage("Collectsettings failure!", true);
        return err;
    }
    bufferConfig bufferConfig_;
    bufferConfig_.block_size = Collectsettings.buffersize;
    bufferConfig_.num_blocks = (m_ImgSetting.resolution / m_LinePerAcq) * (1 + 1);
    err = pQT12136dc->SetBufferConfig(bufferConfig_);
    if (err != DEVICE_OK) {
        LogMessage("bufferConfig_ failure!", true);
        return err;
    }
    err = IR::setup(bufferConfig_.block_size, m_ImgSetting.resolution, m_ImgSetting.resolution, m_ImgSetting.channel, m_LinePerAcq, m_ImgSetting.m_phase_pixel);
    std::vector<int> h_counts_map = mapGenerator::generate_uniform_map(m_ImgSetting.m_overResolution, Collectsettings.Segment / 4 * cardsettings.ChannelNumber);
    err = IR::setReconstructionMap(h_counts_map.data());
    if (err != DEVICE_OK) {
        LogMessage("IR::setup failure!", true);
        return err;
    }
    return DEVICE_OK;
}

int _920TwophotonCamera::ClockOutput() {
    // NI-DAQmx 变量
    TaskHandle  taskHandle = 0;
    int32       error = 0;
    char        errBuff[2048] = { '\0' };
    int32       written;

    // 要写入的数据 (uInt32 数组)
    // 对于单线操作，数组只有一个元素: 1 (高电平) 或 0 (低电平)
    uInt8      dataHigh[1] = { 1 };
    uInt8      dataLow[1] = { 0 };


    printf("开始 NI-DAQmx 数字输出任务...\n");

    /*********************************************/
    // DAQmx 任务代码
    /*********************************************/
    DAQmxErrChk(DAQmxCreateTask("", &taskHandle));
    DAQmxErrChk(DAQmxCreateDOChan(taskHandle, m_ClockControl.c_str(), "", DAQmx_Val_ChanPerLine));
    DAQmxErrChk(DAQmxStartTask(taskHandle));

    // 写入高电平
    LogMessage("输出高电平 (1)" ,true);
    DAQmxErrChk(DAQmxWriteDigitalLines(taskHandle, 1, TRUE, 10.0, DAQmx_Val_GroupByChannel, dataHigh, &written, NULL));

    // 保持高电平3秒
    Sleep(500);

    // 写入低电平
    LogMessage("输出低电平 (0)",true);
    DAQmxErrChk(DAQmxWriteDigitalLines(taskHandle, 1, TRUE, 10.0, DAQmx_Val_GroupByChannel, dataLow, &written, NULL));

    LogMessage("任务完成。",true);

Error:
    if (DAQmxFailed(error))
    {
        // 获取并显示详细的错误信息
        DAQmxGetExtendedErrorInfo(errBuff, 2048);
        LogMessage(errBuff, false);
    }

    // 无论成功与否，都清理任务
    if (taskHandle != 0)
    {
        DAQmxStopTask(taskHandle);
        DAQmxClearTask(taskHandle);
    }

    return DEVICE_OK;
}

int _920TwophotonCamera::SetNIDAQDigtal(const string& port, bool volt) {
    // NI-DAQmx 变量
    TaskHandle  taskHandle = 0;
    int32       error = 0;
    char        errBuff[2048] = { '\0' };
    int32       written;

    // 要写入的数据 (uInt32 数组)
    // 对于单线操作，数组只有一个元素: 1 (高电平) 或 0 (低电平)
    uInt8      dataHigh[1] = { 1 };
    uInt8      dataLow[1] = { 0 };


    printf("开始 NI-DAQmx 数字输出任务...\n");

    /*********************************************/
    // DAQmx 任务代码
    /*********************************************/
    DAQmxErrChk(DAQmxCreateTask("", &taskHandle));
    DAQmxErrChk(DAQmxCreateDOChan(taskHandle, port.c_str(), "", DAQmx_Val_ChanPerLine));
    DAQmxErrChk(DAQmxStartTask(taskHandle));

    if (volt == 1) {
        // 写入高电平
        LogMessage("输出高电平 (1)", true);
        DAQmxErrChk(DAQmxWriteDigitalLines(taskHandle, 1, TRUE, 10.0, DAQmx_Val_GroupByChannel, dataHigh, &written, NULL));
    }
    else {
        // 写入低电平
        LogMessage("输出低电平 (0)", true);
        DAQmxErrChk(DAQmxWriteDigitalLines(taskHandle, 1, TRUE, 10.0, DAQmx_Val_GroupByChannel, dataLow, &written, NULL));
    }
    // 保持高电平3秒
    Sleep(500);
    LogMessage("任务完成。", true);

Error:
    if (DAQmxFailed(error))
    {
        // 获取并显示详细的错误信息
        DAQmxGetExtendedErrorInfo(errBuff, 2048);
        LogMessage(errBuff, false);
    }

    // 无论成功与否，都清理任务
    if (taskHandle != 0)
    {
        DAQmxStopTask(taskHandle);
        DAQmxClearTask(taskHandle);
    }

    return DEVICE_OK;
}

// 这个函数接收你的理想参数，并计算出硬件上可实现的精确频率
void calculate_achievable_frequencies(
    double desired_fps,
    int resolution,
    int over_resolution,
    const double TIMEBASE_HZ,
    // 使用引用作为输出参数
    double& actual_fps,
    double& actual_line_freq,
    double& actual_pixel_freq)
{
    // 1. 计算理想的像素频率
    double desired_pixel_freq = desired_fps * resolution * over_resolution;
    if (desired_pixel_freq <= 0) {
        throw std::runtime_error("计算出的像素频率必须为正数。");
    }

    // 2. 计算理想的节拍数 (可能是小数)
    double ideal_pixel_ticks = TIMEBASE_HZ / desired_pixel_freq;

    // 3. 四舍五入到最近的整数节拍
    long long actual_pixel_ticks = static_cast<long long>(std::round(ideal_pixel_ticks));
    if (actual_pixel_ticks == 0) {
        throw std::runtime_error("频率过高，无法从所选时基生成。");
    }

    // 4. 根据整数节拍数，反向计算出所有可实现的精确频率
    actual_pixel_freq = TIMEBASE_HZ / actual_pixel_ticks;
    actual_line_freq = actual_pixel_freq / over_resolution;
    actual_fps = actual_line_freq / resolution;
}