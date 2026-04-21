# Helab_Microscope Device Adapter

双光子显微镜设备适配器，支持 Micro-Manager 显微镜控制软件。

## 特性

- **模块化架构**：硬件抽象层 + 应用层 + 适配层，易于扩展和维护
- **多激光器支持**：支持 2-3 个激光器，可扩展更多
- **可扩展设计**：预留自适应光学、Resonance 扫描等接口
- **GPU 加速**：CUDA 图像重建
- **跨平台**：Windows / Linux 支持

## 硬件规格

### 数据采集卡

| 参数 | 规格 |
|------|------|
| 厂商 | 坤驰科技 |
| 型号 | QT12136DC |
| 采样率 | 1 GS/s |
| 通道数 | 4 通道 |
| 分辨率 | 14 bit |
| 接口 | PCIe |

### NI-DAQ 卡

| 参数 | 规格 |
|------|------|
| 型号 | PCIe-6363 |
| 厂商 | National Instruments |
| 用途 | 时钟发生、检流计控制 |

### 检流计扫描器 (Galvo)

| 参数 | 规格 |
|------|------|
| 厂商 | 大族 |
| 扫描范围 | ±5 V |
| 最大帧率 | 1.5 Hz |

---

## 架构设计

### 分层架构

```
┌─────────────────────────────────────────────────────────────┐
│                    Micro-Manager Layer                       │
│  HelabMicroscopeHub, HelabCamera, HelabLaserDevice          │
│  (MM设备适配器接口)                                          │
└─────────────────────────────────────────────────────────────┘
                              │
                              ▼
┌─────────────────────────────────────────────────────────────┐
│                    Application Layer                         │
│  MicroscopeController - 业务逻辑协调                         │
│  (成像控制、激光管理、配置管理)                               │
└─────────────────────────────────────────────────────────────┘
                              │
                              ▼
┌─────────────────────────────────────────────────────────────┐
│                    Hardware Abstraction Layer                │
│  IClock, IScanner, IDetector, ILaser, IAdaptiveOptics       │
│  (硬件抽象接口，支持依赖注入和 Mock 测试)                     │
└─────────────────────────────────────────────────────────────┘
                              │
                              ▼
┌─────────────────────────────────────────────────────────────┐
│                    Hardware Driver Layer                     │
│  NidaqClock, GalvoScanner, QtDetector, GenericLaser         │
│  (具体硬件驱动实现)                                          │
└─────────────────────────────────────────────────────────────┘
```

### 核心设计原则

| 原则 | 说明 |
|------|------|
| **依赖倒置** | 高层模块不依赖低层模块，都依赖抽象接口 |
| **单一职责** | 每个类只做一件事 |
| **开闭原则** | 对扩展开放，对修改关闭 |
| **组合优于继承** | 用组合构建复杂对象 |

### 数据流

```
1. 时钟生成 (IClock)
   NI-DAQ Counter → Pixel Clock → Line Clock → Frame Clock

2. 扫描控制 (IScanner)
   NI-DAQ AO → X-Galvo → Y-Galvo

3. 数据采集 (IDetector)
   PMT Signal → ADC → Ring Buffer → Callback

4. 图像处理 (ImagePipeline)
   Raw Data → Descan → Intensity Map → Image Buffer
```

---

## 目录结构

```
Device/
├── include/
│   ├── hal/                      # 硬件抽象层
│   │   ├── IHardware.h           # 核心接口定义
│   │   ├── HardwareFactory.h     # 硬件工厂
│   │   └── impl/                 # 具体实现
│   │       ├── NidaqClock.h      # NI-DAQ 时钟
│   │       ├── GalvoScanner.h    # Galvo 扫描器
│   │       ├── QtDetector.h      # QT 采集卡
│   │       └── GenericLaser.h    # 通用激光器
│   │
│   ├── app/                      # 应用层
│   │   └── MicroscopeController.h
│   │
│   ├── pipeline/                 # 图像处理流水线
│   │   └── ImagePipeline.h
│   │
│   ├── mmadapter/                # Micro-Manager 适配层
│   │   └── HelabDevices.h
│   │
│   └── common/                   # 公共工具
│       ├── Result.h              # 错误处理 (std::expected)
│       ├── Logger.h              # 日志
│       └── ThreadSafeQueue.h     # 线程安全队列
│
├── source/                       # 源文件
│   ├── hal/impl/
│   ├── app/
│   ├── pipeline/
│   └── mmadapter/
│
└── tests/                        # 单元测试
    ├── mocks/
    │   └── MockHardware.h
    └── app/
        └── MicroscopeControllerTest.cpp
```

---

## 快速开始

### 1. 安装驱动程序

| 驱动 | 下载地址 | 用途 |
|------|----------|------|
| **NI-DAQmx** | [National Instruments](https://www.ni.com/en-us/support/downloads/drivers/download.ni-daqmx.html) | 数据采集卡驱动 |
| **CUDA Toolkit** | [NVIDIA](https://developer.nvidia.com/cuda-downloads) (推荐 12.x) | GPU 图像重建 |

### 2. 编译 (Windows)

**方法一：Visual Studio**

1. 打开 `MMCoreAndDevices.sln`
2. 添加 `Helab_Microscope.vcxproj`
3. 选择 `x64` 平台，编译

**方法二：CMake**

```powershell
mkdir build && cd build
cmake .. -A x64
cmake --build . --config Release
```

### 3. 编译 (Linux)

```bash
mkdir build && cd build
cmake .. -DUSE_NI_DAQMX=ON -DUSE_CUDA=ON
make -j$(nproc)
```

### 4. 运行测试

```bash
cmake .. -DBUILD_TESTS=ON
make -j$(nproc)
ctest --output-on-failure
```

---

## 设备列表

| 设备名称 | 类型 | 描述 |
|---------|------|------|
| HelabMicroscopeHub | HubDevice | 管理所有设备 |
| HelabCamera | CameraDevice | 双光子相机 |
| HelabLaser_920nm | GenericDevice | 920nm 激光器控制 |
| HelabLaser_1064nm | GenericDevice | 1064nm 激光器控制 |

---

## 扩展功能

### 添加新激光器

```cpp
// 1. 在 HardwareFactory 中添加新类型
if (type == "808nm") {
    LaserConfig config;
    config.name = "808nm";
    config.wavelength = 808.0;
    config.maxPower = 500.0;
    return std::make_unique<GenericLaser>(config);
}

// 2. 在 Hub.cpp 中注册
RegisterDevice("HelabLaser_808nm", MM::GenericDevice, "808nm Laser");
```

### 添加自适应光学

```cpp
// 1. 实现 IAdaptiveOptics 接口
class AlpaoDM : public IAdaptiveOptics {
    // ...
};

// 2. 添加到控制器
controller->addAdaptiveOptics(std::make_unique<AlpaoDM>(97));

// 3. 应用校正
controller->applyWavefrontCorrection(wavefrontData);
```

### 添加 Resonance 扫描

```cpp
// 1. 实现 IScanner 接口
class ResonanceScanner : public IScanner {
    // ...
};

// 2. 切换扫描模式
controller->setScanType(ScanPattern::Type::Resonance);
```

---

## API 参考

### MicroscopeController

```cpp
// 初始化
auto result = controller->initialize();

// 配置成像参数
ImagingConfig config;
config.fps = 1.5;
config.width = 512;
config.height = 512;
controller->setImagingConfig(config);

// 成像控制
controller->startImaging();
controller->stopImaging();

// 激光控制
controller->addLaserByType("920nm", "main");
controller->setLaserPower("main", 100.0);  // 100 mW
controller->enableLaser("main", true);
```

### 错误处理

使用 C++23 `std::expected` 进行错误处理：

```cpp
auto result = controller->startImaging();
if (!result) {
    LOG_ERROR(result.error().message);
    return result.error().code;
}
```

---

## 依赖项

### 必需依赖
- **Micro-Manager MMDevice API** - 设备适配器接口
- **C++20** 编译器 (支持 `std::expected`, `std::format`)
- **Visual Studio 2022** (Windows) 或 **GCC 12+** (Linux)

### 第三方库

| 库 | 用途 |
|----|------|
| NI-DAQmx | NI 数据采集 |
| CUDA | GPU 图像重建 |
| QT_Board SDK | QT 采集卡驱动 |
| GoogleTest | 单元测试 |

---

## 平台支持

| 平台 | 状态 | 备注 |
|------|------|------|
| Windows | ✅ 完整支持 | 需要 NI-DAQmx 驱动 |
| Linux | ✅ 支持 | 需要 NI-DAQmx Base |
| macOS | ⚠️ 部分支持 | NI-DAQmx 不支持 |

---

## 故障排除

### 编译错误：找不到 NIDAQmx.h

**解决**：从 [NI官网](https://www.ni.com/en-us/support/downloads/drivers/download.ni-daqmx.html) 下载安装 NI-DAQmx

### 编译错误：找不到 cuda_runtime.h

**解决**：从 [NVIDIA官网](https://developer.nvidia.com/cuda-downloads) 下载安装 CUDA Toolkit

### 测试失败

```bash
# 确保安装了 GoogleTest
cmake .. -DBUILD_TESTS=ON
ctest --output-on-failure
```

---

## 许可证

请参考 Micro-Manager 许可证。

## 贡献

欢迎提交 Issue 和 Pull Request！
