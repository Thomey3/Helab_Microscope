Extending the Adapter
=====================

This guide covers how to extend the Helab Microscope with new hardware and features.

Adding New Hardware
-------------------

Step 1: Implement the Interface
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Create a new class implementing the appropriate interface:

.. code-block:: cpp

   // Device/include/hal/impl/MyCustomScanner.h

   #pragma once
   #include "../IHardware.h"

   namespace helab {

   class MyCustomScanner : public IScanner {
   public:
       explicit MyCustomScanner(const std::string& deviceName);
       ~MyCustomScanner() override;

       // IScanner interface
       Result<void> configure(const ScannerConfig& config) override;
       Result<void> setScanPattern(const ScanPattern& pattern) override;
       Result<void> start() override;
       Result<void> stop() override;
       Result<void> setPosition(double x, double y) override;
       bool isRunning() const override;

   private:
       std::string deviceName_;
       ScannerConfig config_;
       ScanPattern pattern_;
       std::atomic<bool> running_{false};
   };

   } // namespace helab

Step 2: Implement the Methods
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

.. code-block:: cpp

   // Device/source/hal/impl/MyCustomScanner.cpp

   #include "hal/impl/MyCustomScanner.h"

   namespace helab {

   MyCustomScanner::MyCustomScanner(const std::string& deviceName)
       : deviceName_(deviceName) {}

   MyCustomScanner::~MyCustomScanner() {
       stop();
   }

   Result<void> MyCustomScanner::configure(const ScannerConfig& config) {
       config_ = config;

       // Initialize hardware here
       // ...

       return success();
   }

   Result<void> MyCustomScanner::start() {
       if (running_) return success();

       // Start scanning
       // ...

       running_ = true;
       return success();
   }

   Result<void> MyCustomScanner::stop() {
       if (!running_) return success();

       // Stop scanning
       // ...

       running_ = false;
       return success();
   }

   // ... other methods

   } // namespace helab

Step 3: Register in Factory
~~~~~~~~~~~~~~~~~~~~~~~~~~~~

.. code-block:: cpp

   // Device/source/hal/HardwareFactory.cpp

   std::unique_ptr<IScanner> HardwareFactory::createScanner(const std::string& type) {
       if (type == "my_custom") {
           return std::make_unique<MyCustomScanner>("Dev1");
       }
       // ... existing types
   }

Step 4: Add to CMakeLists.txt
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

.. code-block:: cmake

   set(NEW_ARCH_SOURCES
       # ... existing sources
       Device/source/hal/impl/MyCustomScanner.cpp
   )

Step 5: Create Tests
~~~~~~~~~~~~~~~~~~~~

.. code-block:: cpp

   // Device/tests/hal/MyCustomScannerTest.cpp

   #include <gtest/gtest.h>
   #include "hal/impl/MyCustomScanner.h"

   TEST(MyCustomScannerTest, Configure_WhenValid_Succeeds) {
       MyCustomScanner scanner("Dev1");

       ScannerConfig config;
       config.deviceName = "Dev1";

       auto result = scanner.configure(config);

       EXPECT_TRUE(result);
   }

Adding New Laser Types
----------------------

Generic Laser
~~~~~~~~~~~~~

For lasers with serial/USB control:

.. code-block:: cpp

   // Inherit from GenericLaser
   class MyLaser : public GenericLaser {
   public:
       MyLaser(const LaserConfig& config)
           : GenericLaser(config) {}

       Result<void> sendCommand(const std::string& cmd) override {
           // Send command via serial/USB
           // ...
       }

       Result<std::string> readResponse() override {
           // Read response
           // ...
       }
   };

Custom Laser
~~~~~~~~~~~~

For lasers with special control:

.. code-block:: cpp

   class CoherentLaser : public ILaser {
   public:
       Result<void> setPower(double power_mW) override {
           // Coherent-specific protocol
           // ...
       }
       // ...
   };

Adding Processing Stages
------------------------

Create a Custom Stage
~~~~~~~~~~~~~~~~~~~~~~

.. code-block:: cpp

   // Device/include/pipeline/stages/MyStage.h

   class MyStage : public IProcessingStage {
   public:
       void process(void* inputData, void* outputData, size_t size) override {
           auto* input = static_cast<uint16_t*>(inputData);
           auto* output = static_cast<uint8_t*>(outputData);

           // Custom processing
           for (size_t i = 0; i < size; ++i) {
               output[i] = transform(input[i]);
           }
       }

       std::string name() const override {
           return "MyStage";
       }

   private:
       uint8_t transform(uint16_t value) {
           // Your transformation
           return static_cast<uint8_t>(value >> 6);
       }
   };

Add to Pipeline
~~~~~~~~~~~~~~~

.. code-block:: cpp

   auto pipeline = std::make_unique<ImagePipeline>();
   pipeline->addStage(std::make_unique<MyStage>());

Adding Micro-Manager Devices
-----------------------------

New Device Type
~~~~~~~~~~~~~~~

.. code-block:: cpp

   // Device/include/mmadapter/MyDevice.h

   class MyDevice : public CGenericBase<MyDevice> {
   public:
       int Initialize() override;
       int Shutdown() override;
       void GetName(char* name) const override;
       bool Busy() override;

       // Property callbacks
       int OnMyProperty(MM::PropertyBase* pProp, MM::ActionType eAct);

   private:
       HelabMicroscopeHub* hub_ = nullptr;
   };

Register Device
~~~~~~~~~~~~~~~

.. code-block:: cpp

   // Hub.cpp

   MODULE_API void InitializeModuleData() {
       // ... existing devices
       RegisterDevice("MyDevice", MM::GenericDevice, "My Custom Device");
   }

   MODULE_API MM::Device* CreateDevice(const char* deviceName) {
       // ...
       if (strcmp(deviceName, "MyDevice") == 0) {
           return new MyDevice;
       }
       // ...
   }

Configuration Schema
--------------------

Add configuration options:

.. code-block:: json

   {
     "my_custom": {
       "option1": "value1",
       "option2": 123
     }
   }

Parse in code:

.. code-block:: cpp

   void MyCustomHardware::loadConfig(const json& config) {
       auto myConfig = config["my_custom"];
       option1_ = myConfig["option1"].get<std::string>();
       option2_ = myConfig["option2"].get<int>();
   }

Best Practices
--------------

Hardware Abstraction
~~~~~~~~~~~~~~~~~~~~

- Keep hardware-specific code in the Driver Layer
- Use interfaces for all hardware operations
- Never expose hardware details to Application Layer

Error Handling
~~~~~~~~~~~~~~

- Return ``Result<T>`` for all operations
- Provide meaningful error messages
- Handle all error cases

Testing
~~~~~~~

- Write tests using mocks
- Test error conditions
- Maintain 80%+ coverage

Documentation
~~~~~~~~~~~~~

- Document all public APIs
- Add usage examples
- Update README for new features

Example: Adding Resonance Scanner
---------------------------------

.. code-block:: cpp

   // Device/include/hal/impl/ResonanceScanner.h

   class ResonanceScanner : public IScanner {
   public:
       explicit ResonanceScanner(const std::string& deviceName);

       Result<void> configure(const ScannerConfig& config) override {
           config_ = config;
           // Configure resonance-specific hardware
           return success();
       }

       Result<void> setScanPattern(const ScanPattern& pattern) override {
           if (pattern.type != ScanPattern::Type::Resonance) {
               return make_error(HardwareErrorCode::InvalidParameter,
                   "Invalid scan type for resonance scanner");
           }
           pattern_ = pattern;
           resonanceFrequency_ = pattern.resonanceFrequency;
           return success();
       }

       // ... other methods

   private:
       double resonanceFrequency_ = 8000;  // 8 kHz default
   };

Then register:

.. code-block:: cpp

   // HardwareFactory.cpp
   if (type == "resonance") {
       return std::make_unique<ResonanceScanner>(deviceName);
   }
