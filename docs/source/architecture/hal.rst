Hardware Abstraction Layer
=========================

The Hardware Abstraction Layer (HAL) defines interfaces for all hardware components.

.. _hal-interfaces:

Interface Overview
------------------

.. figure:: _static/hal_interfaces.svg
   :alt: HAL Interfaces
   :width: 100%

   Class diagram of HAL interfaces and their relationships.

Core Interfaces
---------------

IClock - Clock Generation
~~~~~~~~~~~~~~~~~~~~~~~~~

.. doxygenclass:: helab::IClock
   :members:
   :protected-members:
   :undoc-members:

Clock Configuration
^^^^^^^^^^^^^^^^^^^

.. doxygenstruct:: helab::ClockConfig
   :members:

IScanner - Scanner Control
~~~~~~~~~~~~~~~~~~~~~~~~~~

.. doxygenclass:: helab::IScanner
   :members:
   :protected-members:
   :undoc-members:

Scanner Configuration
^^^^^^^^^^^^^^^^^^^^^

.. doxygenstruct:: helab::ScannerConfig
   :members:

Scan Pattern
^^^^^^^^^^^^

.. doxygenstruct:: helab::ScanPattern
   :members:

IDetector - Data Acquisition
~~~~~~~~~~~~~~~~~~~~~~~~~~~~

.. doxygenclass:: helab::IDetector
   :members:
   :protected-members:
   :undoc-members:

Detector Configuration
^^^^^^^^^^^^^^^^^^^^^^

.. doxygenstruct:: helab::DetectorConfig
   :members:

ILaser - Laser Control
~~~~~~~~~~~~~~~~~~~~~~

.. doxygenclass:: helab::ILaser
   :members:
   :protected-members:
   :undoc-members:

Laser Configuration
^^^^^^^^^^^^^^^^^^^

.. doxygenstruct:: helab::LaserConfig
   :members:

IAdaptiveOptics - Adaptive Optics
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

.. doxygenclass:: helab::IAdaptiveOptics
   :members:
   :protected-members:
   :undoc-members:

Adaptive Optics Configuration
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

.. doxygenstruct:: helab::AdaptiveOpticsConfig
   :members:

Factory Interface
-----------------

IHardwareFactory
~~~~~~~~~~~~~~~~

.. doxygenclass:: helab::IHardwareFactory
   :members:
   :undoc-members:

Usage Example
-------------

Creating Hardware Through Factory
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

.. code-block:: cpp

   auto factory = std::make_shared<HardwareFactory>();

   // Create hardware components
   auto clock = factory->createClock("nidaq");
   auto scanner = factory->createScanner("galvo");
   auto detector = factory->createDetector("qt");
   auto laser = factory->createLaser("920nm");

   // Configure
   ClockConfig clockCfg;
   clockCfg.deviceName = "Dev1";
   clockCfg.pixelCount = 512;
   clock->configure(clockCfg);

   // Start
   clock->start();
   scanner->start();
   detector->startAcquisition();

Implementing Custom Hardware
~~~~~~~~~~~~~~~~~~~~~~~~~~~~

To implement a custom hardware component:

1. Inherit from the interface:

   .. code-block:: cpp

      class MyCustomScanner : public IScanner {
      public:
          Result<void> configure(const ScannerConfig& config) override;
          Result<void> setScanPattern(const ScanPattern& pattern) override;
          Result<void> start() override;
          Result<void> stop() override;
          Result<void> setPosition(double x, double y) override;
          bool isRunning() const override;

      private:
          // Implementation details
      };

2. Implement all pure virtual methods.

3. Register in factory:

   .. code-block:: cpp

      std::unique_ptr<IScanner> HardwareFactory::createScanner(const std::string& type) {
          if (type == "my_custom") {
              return std::make_unique<MyCustomScanner>();
          }
          // ...
      }

Error Handling
--------------

All interface methods return :cpp:type:`Result<T>`:

.. code-block:: cpp

   // Result is std::expected<T, HardwareError>
   auto result = clock->start();
   if (!result) {
       // Error occurred
       HardwareError err = result.error();
       std::cerr << "Error " << err.code << ": " << err.message << std::endl;
   }

HardwareError Structure
~~~~~~~~~~~~~~~~~~~~~~~

.. doxygenstruct:: helab::HardwareError
   :members:

Common Error Codes
~~~~~~~~~~~~~~~~~~

.. list-table::
   :header-rows: 1

   * - Code
     - Name
     - Description
   * - 0
     - Success
     - Operation completed successfully
   * - -1
     - NotInitialized
     - Hardware not initialized
   * - -2
     - InvalidParameter
     - Invalid parameter value
   * - -3
     - HardwareError
     - Hardware communication error
   * - -4
     - Timeout
     - Operation timed out
   * - -5
     - OutOfRange
     - Value out of valid range

Thread Safety
-------------

All interfaces are designed for thread-safe operation:

- **Configuration methods**: Should be called from control thread
- **Start/Stop methods**: Thread-safe, can be called from any thread
- **Status queries**: Atomic operations, always safe

.. note::

   The actual thread safety depends on the implementation. Check the
   implementation documentation for specific guarantees.
