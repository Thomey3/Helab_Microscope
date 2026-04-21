# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

Helab Microscope is a two-photon microscope device adapter for Micro-Manager, supporting dual-wavelength lasers (920nm, 1064nm), galvanometer scanning, and high-speed data acquisition.

## Architecture

### Three-Layer Structure

1. **HAL (Hardware Abstraction Layer)** - `Device/include/hal/`
   - Core interfaces: `IClock`, `IScanner`, `IDetector`, `ILaser`, `IAdaptiveOptics`
   - Implementations: `NidaqClock`, `GalvoScanner`, `QtDetector`, `GenericLaser`

2. **Application Layer** - `Device/include/app/`
   - `MicroscopeController` - Core business logic, hardware lifecycle management
   - State machine: Uninitialized → Ready → Imaging → Error

3. **MM Adapter Layer** - `Device/include/mmadapter/`
   - `HelabMicroscopeHub` - Entry point for all devices
   - `HelabCamera` - Implements `CCameraBase`
   - `HelabLaserDevice` - Implements `CGenericBase`

### Key Directories

- `Device/include/hal/` - Hardware abstraction interfaces
- `Device/include/hal/impl/` - Hardware implementations (NI-DAQmx, QT12136DC)
- `Device/include/app/` - Application logic
- `Device/include/mmadapter/` - Micro-Manager adapter
- `Device/include/pipeline/` - Image processing pipeline
- `Device/include/common/` - Utilities (Result<T>, Logger, ThreadSafeQueue)
- `docs/` - Sphinx documentation

### Hardware Specifications

| Component | Model | Specs |
|-----------|-------|-------|
| DAQ (Clock/Scanner) | NI PCIe-6363 | AO: 2ch, ±10V, 3.3MS/s |
| DAQ (Detector) | QT12136DC | 4ch, 1GS/s, 14-bit |
| Scanner | Galvanometer | ±5V, 1.5 Hz max frame rate |
| Lasers | 920nm + 1064nm | Dual-photon excitation |

## Build Commands

### Device Adapter
```bash
# Build with CMake (from project root)
mkdir build && cd build
cmake ..
make -j
```

### Documentation
```bash
# Build documentation locally
cd docs
python3 -m venv .venv
source .venv/bin/activate
pip install -r requirements.txt
doxygen Doxyfile                    # Generate C++ API XML
sphinx-build -b html source _build/html

# Preview locally
cd _build/html && python3 -m http.server 8888
```

## Documentation Requirements

**IMPORTANT**: All changes to this project MUST be reflected in the documentation.

When making any code changes:

1. **Update inline documentation** - Add/update Doxygen comments in header files
2. **Update architecture docs** - If changing structure, update `docs/source/architecture/*.rst`
3. **Update API reference** - If adding/modifying public APIs, update relevant `.rst` files
4. **Update user guide** - If changing user-facing behavior, update `docs/source/user_guide/*.rst`
5. **Rebuild and verify** - Run `sphinx-build` locally to verify documentation builds correctly

### Documentation Structure

```
docs/source/
├── getting_started/    # Installation, quickstart, hardware setup
├── architecture/       # HAL, app layer, pipeline, MM adapter
├── user_guide/         # Imaging, laser control, ROI scanning
├── api/                # API reference (auto-generated from Doxygen)
├── development/        # Building, testing, contributing
└── reference/          # Glossary, changelog, license
```

### File Encoding

All source files must use **UTF-8 encoding**. If you encounter GBK/GB18030 encoded files, convert them:

```bash
iconv -f GB18030 -t UTF-8 input.h -o output.h
```

## Code Patterns

### Error Handling
```cpp
auto result = clock_->start();
if (!result) {
    LOG_ERROR("Failed to start clock");
    return result;  // Propagate HardwareError
}
```

### RAII Resource Management
```cpp
NidaqClock::~NidaqClock() {
    destroyTask();  // DAQmxStopTask + DAQmxClearTask
}
```

### Dependency Injection
```cpp
auto factory = std::make_shared<HardwareFactory>();
controller_ = std::make_unique<MicroscopeController>(factory);
```

## Dependencies

- C++20 (for `std::expected`, `std::source_location`, `std::format`)
- NI-DAQmx SDK
- QT12136DC SDK (坤驰科技)
- Micro-Manager device interface (DeviceBase.h)
- Sphinx + Breathe + Doxygen (for documentation)
