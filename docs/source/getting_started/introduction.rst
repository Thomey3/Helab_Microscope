Introduction
============

.. _overview:

Overview
--------

**Helab Microscope** is a two-photon microscope device adapter for `Micro-Manager <https://micro-manager.org/>`_,
the premier software platform for automated microscope control.

.. figure:: _static/architecture_overview.svg
   :alt: Architecture Overview
   :width: 80%

   High-level architecture of the Helab Microscope device adapter.

Key Features
~~~~~~~~~~~~

- **Modular Architecture**: Hardware Abstraction Layer (HAL) + Application Layer + Micro-Manager Adapter
- **Multi-Laser Support**: 2-3 laser wavelengths with extensible design
- **GPU Acceleration**: CUDA-based image reconstruction
- **Cross-Platform**: Windows and Linux support
- **Testable Design**: Dependency injection with mock support for unit testing

Hardware Specifications
-----------------------

Data Acquisition Card
~~~~~~~~~~~~~~~~~~~~~

====================  ==========================
Parameter             Specification
====================  ==========================
Vendor                坤驰科技 (KunChi Tech)
Model                 QT12136DC
Sample Rate           1 GS/s
Channels              4 channels
Resolution            14 bit
Interface             PCIe
====================  ==========================

NI-DAQ Card
~~~~~~~~~~~

====================  ==========================
Parameter             Specification
====================  ==========================
Model                 PCIe-6363
Vendor                National Instruments
Usage                 Clock generation, galvo control
====================  ==========================

Galvo Scanner
~~~~~~~~~~~~~

====================  ==========================
Parameter             Specification
====================  ==========================
Vendor                大族 (Hans)
Scan Range            ±5 V
Max Frame Rate        1.5 Hz
====================  ==========================

Design Principles
-----------------

The architecture follows these core principles:

Dependency Inversion
~~~~~~~~~~~~~~~~~~~

High-level modules do not depend on low-level modules. Both depend on abstractions.

.. code-block:: cpp

   // High-level controller depends on interface, not implementation
   class MicroscopeController {
       std::unique_ptr<IClock> clock_;      // Interface
       std::unique_ptr<IScanner> scanner_;  // Interface
       std::unique_ptr<IDetector> detector_; // Interface
   };

Single Responsibility
~~~~~~~~~~~~~~~~~~~~~

Each class has one reason to change.

+--------------------------------+----------------------------------------+
| Class                          | Responsibility                         |
+================================+========================================+
| :cpp:class:`NidaqClock`        | Generate pixel/line/frame clocks       |
+--------------------------------+----------------------------------------+
| :cpp:class:`GalvoScanner`      | Control galvo scanning waveforms       |
+--------------------------------+----------------------------------------+
| :cpp:class:`QtDetector`        | Acquire data from QT card              |
+--------------------------------+----------------------------------------+
| :cpp:class:`GenericLaser`      | Control laser power and enable         |
+--------------------------------+----------------------------------------+

Open/Closed Principle
~~~~~~~~~~~~~~~~~~~~~

Open for extension, closed for modification.

.. code-block:: cpp

   // Add new scanner without modifying existing code
   class ResonanceScanner : public IScanner {
       // New scanner type
   };

   // Register in factory
   factory->registerScanner("resonance", []() {
       return std::make_unique<ResonanceScanner>();
   });

Composition Over Inheritance
~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Build complex objects through composition, not inheritance.

.. code-block:: cpp

   // MicroscopeController COMPOSES hardware components
   class MicroscopeController {
       std::unique_ptr<IClock> clock_;
       std::unique_ptr<IScanner> scanner_;
       std::unique_ptr<IDetector> detector_;
       std::map<std::string, std::unique_ptr<ILaser>> lasers_;
       std::unique_ptr<IAdaptiveOptics> adaptiveOptics_;
   };

Comparison with Other Adapters
------------------------------

+-------------------+--------------------+--------------------+
| Feature           | Helab Microscope   | Traditional MM     |
+===================+====================+====================+
| Architecture      | Layered (HAL+App)  | Monolithic         |
+-------------------+--------------------+--------------------+
| Testability       | Mock-based testing | Hardware required  |
+-------------------+--------------------+--------------------+
| Extensibility     | Plugin pattern     | Modify source      |
+-------------------+--------------------+--------------------+
| Error Handling    | std::expected      | Return codes       |
+-------------------+--------------------+--------------------+
| Multi-Laser       | Built-in           | Per-laser adapter  |
+-------------------+--------------------+--------------------+
| GPU Support       | CUDA pipeline      | CPU only           |
+-------------------+--------------------+--------------------+
