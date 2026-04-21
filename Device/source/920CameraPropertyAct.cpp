#include "_920TwophotonCamera.h"

int _920TwophotonCamera::OnClock(MM::PropertyBase* pProp, MM::ActionType eAct)
{
    if (eAct == MM::BeforeGet)
    {
        string propName = pProp->GetName();
        string result;
        int index = propName[0] - 'a';
        switch (index)
        {
        case 0:
        {
            result = m_clock_pixel.niDeviceName + m_clock_pixel.clockChannel;
            break;
        }
        case 1:
        {
            result = m_clock_pixel.niDeviceName + m_clock_pixel.TrigChannel;
            break;
        }
        case 2:
        {
            result = m_clock_line.niDeviceName + m_clock_line.clockChannel;
            break;
        }
        case 3:
        {
            result = m_clock_line.niDeviceName + m_clock_line.TrigChannel;
            break;
        }
        case 4:
        {
            result = m_clock_frame.niDeviceName + m_clock_frame.clockChannel;
            break;
        }
        case 5:
        {
            result = m_clock_frame.niDeviceName + m_clock_frame.TrigChannel;
            break;
        }
        default:
            break;
        }
        pProp->Set(result.c_str());
    }
    else if (eAct == MM::AfterSet)
    {
        string Name;
        string propName = pProp->GetName();
        int index = propName[0] - 'a';
        pProp->Get(Name);
        NIDAQmxUtils::ChannelParts parts = NIDAQmxUtils::parseChannelName(Name);
        string deviceName = parts.deviceName;
        string channelName = parts.channelName;
        switch (index)
        {
        case 0:
        {
            m_clock_pixel.niDeviceName = deviceName;
            m_clock_pixel.clockChannel = channelName;
            break;
        }
        case 1:
        {
            m_clock_pixel.niDeviceName = deviceName;
            m_clock_pixel.TrigChannel = channelName;
            break;
        }
        case 2:
        {
            m_clock_line.niDeviceName = deviceName;
            m_clock_line.clockChannel = channelName;
            break;
        }
        case 3:
        {
            m_clock_line.niDeviceName = deviceName;
            m_clock_line.TrigChannel = channelName;
            break;
        }case 4:
        {
            m_clock_frame.niDeviceName = deviceName;
            m_clock_frame.clockChannel = channelName;
            break;
        }
        case 5:
        {
            m_clock_frame.niDeviceName = deviceName;
            m_clock_frame.TrigChannel = channelName;
            break;
        }
        default:
            break;
        }
    }
    return DEVICE_OK;
}

int _920TwophotonCamera::OnGalvo(MM::PropertyBase* pProp, MM::ActionType eAct)
{
    if (eAct == MM::BeforeGet)
    {
        string propName = pProp->GetName();
        string result = "";
        int index = propName[0] - 'g';
        switch (index)
        {
        case 0:
        {
            result = m_galvo_x.pChannelProperty.physicalChannelName;
            break;
        }
        case 1:
        {
            result = m_galvo_y.pChannelProperty.physicalChannelName;
            break;
        }
        case 2:
        {
            result = m_galvo_clockSource;
            break;
        }
        case 3:
        {
            result = m_galvo_triggerSource;
            break;
        }
        default:
            break;
        }
        pProp->Set(result.c_str());
    }
    else if (eAct == MM::AfterSet)
    {
        string Name;
        string propName = pProp->GetName();
        int index = propName[0] - 'g';
        pProp->Get(Name);
        switch (index)
        {
        case 0:
        {
            m_galvo_x.pChannelProperty.physicalChannelName = Name;
            break;
        }
        case 1:
        {
            m_galvo_y.pChannelProperty.physicalChannelName = Name;
            break;
        }
        case 2:
        {
            m_galvo_clockSource = Name;
            break;
        }
        case 3:
        {
            m_galvo_triggerSource = Name;
            break;
        }
        default:
            break;
        }
    }
    return DEVICE_OK;
}

int _920TwophotonCamera::OnFps(MM::PropertyBase* pProp, MM::ActionType eAct)
{
    if (eAct == MM::BeforeGet)
    {
        pProp->Set(m_ImgSetting.fps);
    }
    else if (eAct == MM::AfterSet)
    {
        double fps;
        pProp->Get(fps);
        m_ImgSetting.fps = fps;
        m_ImgSetting.UpdataOverResolution();
    }
    return DEVICE_OK;
}
int _920TwophotonCamera::OnResolution(MM::PropertyBase* pProp, MM::ActionType eAct)
{
    if (eAct == MM::BeforeGet)
    {
        pProp->Set((double)m_ImgSetting.resolution);
    }
    else if (eAct == MM::AfterSet)
    {
        double tmp;
        pProp->Get(tmp);
        m_ImgSetting.resolution = (int)tmp;
        m_ImgSetting.UpdataOverResolution();
    }
    return DEVICE_OK;
}
int _920TwophotonCamera::OnFOV(MM::PropertyBase* pProp, MM::ActionType eAct)
{
    if (eAct == MM::BeforeGet)
    {
        string name = pProp->GetName();
        double result;
        if (name == "xSize(um)") result = double(m_ROI.xSize);
        else if (name == "ySize(um)") result = double(m_ROI.ySize);
        else return DEVICE_ERR;
        pProp->Set(result);
    }
    else if (eAct == MM::AfterSet)
    {
        string name = pProp->GetName();
        double tmp;
        pProp->Get(tmp);
        if (name == "xSize(um)") m_ROI.xSize = tmp;
        else if (name == "ySize(um)") m_ROI.ySize = tmp;
        else return DEVICE_ERR;
    }
    return DEVICE_OK;
}
int _920TwophotonCamera::OnOffset(MM::PropertyBase* pProp, MM::ActionType eAct)
{
    if (eAct == MM::BeforeGet)
    {
        string name = pProp->GetName();
        double result;
        if (name == "offsetX") result = double(m_galvo_x.offset);
        else if (name == "offsetY") result = double(m_galvo_y.offset);
        else return DEVICE_ERR;
        pProp->Set(result);
    }
    else if (eAct == MM::AfterSet)
    {
        string name = pProp->GetName();
        double tmp;
        pProp->Get(tmp);
        if (name == "offsetX") m_galvo_x.offset = tmp;
        else if (name == "offsetY") m_galvo_y.offset = tmp;
        else return DEVICE_ERR;
    }
    return DEVICE_OK;
}
int _920TwophotonCamera::OnChannel(MM::PropertyBase* pProp, MM::ActionType eAct)
{
    if (eAct == MM::BeforeGet)
    {
        pProp->Set((double)m_ImgSetting.channel);
    }
    else if (eAct == MM::AfterSet)
    {
        double tmp;
        pProp->Get(tmp);
        m_ImgSetting.channel = (int)tmp;
    }
    return DEVICE_OK;
}

int _920TwophotonCamera::OnBinning(MM::PropertyBase* pProp, MM::ActionType eAct)
{
    if (eAct == MM::BeforeGet)
    {
        pProp->Set((long)GetBinning());
    }
    else if (eAct == MM::AfterSet)
    {
        long binning;
        pProp->Get(binning);
        return SetBinning((int)binning);
    }
    return DEVICE_OK;
}

int _920TwophotonCamera::OnClockControl(MM::PropertyBase* pProp, MM::ActionType eAct)
{
    if (eAct == MM::BeforeGet)
    {
        pProp->Set(m_ClockControl.c_str());
    }
    else if (eAct == MM::AfterSet)
    {
        string g;
        pProp->Get(g);
        m_ClockControl = g;
    }
    return DEVICE_OK;
}

int _920TwophotonCamera::OnPhase(MM::PropertyBase* pProp, MM::ActionType eAct)
{
    if (eAct == MM::BeforeGet)
    {
        pProp->Set((double)m_ImgSetting.m_phase_pixel);
    }
    else if (eAct == MM::AfterSet)
    {
        double g;
        pProp->Get(g);
        m_ImgSetting.m_phase_pixel = (int)g;
        m_ImgSetting.UpdataOverResolution();
    }
    return DEVICE_OK;
}



#ifdef DEBUG
int _920TwophotonCamera::OnClockControlOutput(MM::PropertyBase* pProp, MM::ActionType eAct)
{
    static int set = 0;
    if (eAct == MM::BeforeGet)
    {
        pProp->Set((double)set);
    }
    else if (eAct == MM::AfterSet)
    {
        double g;
        pProp->Get(g);
        set = (int)g;
        if (set == 0) {
            ClockOutput();
        }
        else if(set == 1){
            ClockUpdate(true);
            ClockStart();
        }
        else if (set == 2) {
            ClockStop();
        }
    }
    return DEVICE_OK;
}
int _920TwophotonCamera::OnGalvoOutput(MM::PropertyBase* pProp, MM::ActionType eAct)
{
    static int set = 0;
    if (eAct == MM::BeforeGet)
    {
        pProp->Set((double)set);
    }
    else if (eAct == MM::AfterSet)
    {
        double g;
        pProp->Get(g);
        set = (int)g;
        if (set == 0) {
            GalvoStart(true);
        }
        else if (set == 1) {
            GalvoStop();
        }
    }
    return DEVICE_OK;
}
int _920TwophotonCamera::OnPMT1ENABLE(MM::PropertyBase* pProp, MM::ActionType eAct)
{
    static int set = 0;
    if (eAct == MM::BeforeGet)
    {
        pProp->Set((double)set);
    }
    else if (eAct == MM::AfterSet)
    {
        double g;
        pProp->Get(g);
        set = (int)g;
        if (set == 0) {
            m_PMT1enable_flag = false;
            //SetNIDAQDigtal(m_PMTenable, 0);
        }
        else if (set == 1) {
            m_PMT1enable_flag = true;
            //SetNIDAQDigtal(m_PMTenable, 1);
        }
    }
    return DEVICE_OK;
}
int _920TwophotonCamera::OnPMT2ENABLE(MM::PropertyBase* pProp, MM::ActionType eAct)
{
    static int set = 0;
    if (eAct == MM::BeforeGet)
    {
        pProp->Set((double)set);
    }
    else if (eAct == MM::AfterSet)
    {
        double g;
        pProp->Get(g);
        set = (int)g;
        if (set == 0) {
            m_PMT2enable_flag = false;
            //SetNIDAQDigtal(m_PMTenable, 0);
        }
        else if (set == 1) {
            m_PMT2enable_flag = true;
            //SetNIDAQDigtal(m_PMTenable, 1);
        }
    }
    return DEVICE_OK;
}
#endif // DEBUG
