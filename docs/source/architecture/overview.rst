Architecture Overview
====================

The Helab Microscope device adapter follows a **layered architecture** with clear separation of concerns.

.. _architecture-diagram:

Architecture Diagram
--------------------

.. code-block:: text

   ┌─────────────────────────────────────────────────────────────┐
   │                    Micro-Manager Layer                       │
   │  HelabMicroscopeHub, HelabCamera, HelabLaserDevice          │
   │  (MM Device Adapter Interface)                               │
   └─────────────────────────────────────────────────────────────┘
                                 │
                                 ▼
   ┌─────────────────────────────────────────────────────────────┐
   │                    Application Layer                         │
   │  MicroscopeController - Business Logic Coordination          │
   │  (Imaging control, laser management, configuration)          │
   └─────────────────────────────────────────────────────────────┘
                                 │
                                 ▼
   ┌─────────────────────────────────────────────────────────────┐
   │                    Hardware Abstraction Layer                │
   │  IClock, IScanner, IDetector, ILaser, IAdaptiveOptics       │
   │  (Hardware interfaces, dependency injection, mock testing)   │
   └─────────────────────────────────────────────────────────────┘
                                 │
                                 ▼
   ┌─────────────────────────────────────────────────────────────┐
   │                    Hardware Driver Layer                     │
   │  NidaqClock, GalvoScanner, QtDetector, GenericLaser         │
   │  (Concrete hardware driver implementations)                  │
   └─────────────────────────────────────────────────────────────┘

Layer Responsibilities
----------------------

Micro-Manager Layer
~~~~~~~~~~~~~~~~~~~

The top layer implements Micro-Manager device interfaces:

+-----------------------------------+----------------------------------------+
| Class                             | Responsibility                         |
+===================================+========================================+
| :cpp:class:`HelabMicroscopeHub`   | Hub device, manages child devices      |
+-----------------------------------+----------------------------------------+
| :cpp:class:`HelabCamera`          | Camera device, imaging interface       |
+-----------------------------------+----------------------------------------+
| :cpp:class:`HelabLaserDevice`     | Generic device, laser control          |
+-----------------------------------+----------------------------------------+

This layer:

- Implements MMDevice API interfaces
- Handles property creation and callbacks
- Delegates to Application Layer
- Contains NO business logic

Application Layer
~~~~~~~~~~~~~~~~~

The business logic layer coordinates hardware operations:

+-----------------------------------+----------------------------------------+
| Class                             | Responsibility                         |
+===================================+========================================+
| :cpp:class:`MicroscopeController` | Coordinates all hardware               |
+-----------------------------------+----------------------------------------+
| :cpp:class:`ImagePipeline`        | Image processing workflow              |
+-----------------------------------+----------------------------------------+

This layer:

- Contains business logic
- Manages hardware lifecycle
- Coordinates imaging sequences
- Handles error recovery

Hardware Abstraction Layer
~~~~~~~~~~~~~~~~~~~~~~~~~~

Defines interfaces for all hardware components:

+-----------------------------------+----------------------------------------+
| Interface                         | Purpose                                |
+===================================+========================================+
| :cpp:class:`IClock`               | Clock generation interface             |
+-----------------------------------+----------------------------------------+
| :cpp:class:`IScanner`             | Scanner control interface              |
+-----------------------------------+----------------------------------------+
| :cpp:class:`IDetector`            | Data acquisition interface             |
+-----------------------------------+----------------------------------------+
| :cpp:class:`ILaser`               | Laser control interface                |
+-----------------------------------+----------------------------------------+
| :cpp:class:`IAdaptiveOptics`      | Adaptive optics interface              |
+-----------------------------------+----------------------------------------+
| :cpp:class:`IHardwareFactory`     | Hardware creation factory              |
+-----------------------------------+----------------------------------------+

This layer:

- Defines pure virtual interfaces
- Enables dependency injection
- Supports mock testing
- Allows hardware swapping

Hardware Driver Layer
~~~~~~~~~~~~~~~~~~~~~

Concrete implementations of HAL interfaces:

+-----------------------------------+----------------------------------------+
| Implementation                    | Hardware                               |
+===================================+========================================+
| :cpp:class:`NidaqClock`           | NI-DAQ clock generation                |
+-----------------------------------+----------------------------------------+
| :cpp:class:`GalvoScanner`         | Galvo mirror scanner                   |
+-----------------------------------+----------------------------------------+
| :cpp:class:`QtDetector`           | QT12136DC data acquisition             |
+-----------------------------------+----------------------------------------+
| :cpp:class:`GenericLaser`         | Generic laser control                  |
+-----------------------------------+----------------------------------------+

This layer:

- Implements specific hardware APIs
- Handles low-level communication
- Manages hardware resources
- Isolates driver-specific code

Data Flow
---------

Imaging Sequence
~~~~~~~~~~~~~~~~

.. code-block:: text

   1. User Request (MM Layer)
      └─→ HelabCamera::SnapImage()

   2. Business Logic (App Layer)
      └─→ MicroscopeController::startImaging()
          ├─→ clock_->start()
          ├─→ scanner_->start()
          └─→ detector_->startAcquisition()

   3. Hardware Control (Driver Layer)
      ├─→ NidaqClock: Generate pixel/line/frame clocks
      ├─→ GalvoScanner: Output X/Y waveforms
      └─→ QtDetector: Acquire PMT signals

   4. Data Processing
      └─→ ImagePipeline::process()
          ├─→ DescanStage
          ├─→ IntensityMapStage
          └─→ GPU Reconstruction (CUDA)

   5. Result Return
      └─→ Image buffer to MM Layer

Clock Generation
~~~~~~~~~~~~~~~~

.. code-block:: text

   NI-DAQ Counter (80 MHz timebase)
   │
   ├─→ Pixel Clock (ctr0)
   │   └─→ Triggers QT card sampling
   │
   ├─→ Line Clock (ctr1)
   │   └─→ Marks end of each line
   │
   └─→ Frame Clock (ctr2)
       └─→ Marks end of each frame

Scan Waveforms
~~~~~~~~~~~~~~

.. code-block:: text

   X-Galvo (ao0)          Y-Galvo (ao1)
   │                      │
   │  ┌───┐               │    ┌──────┐
   │  │   │               │    │      │
   └──┘   └──             └────┘      └────
       Line scan              Frame scan

Error Handling
--------------

All operations use :cpp:type:`Result<T>` (std::expected) for error handling:

.. code-block:: cpp

   auto result = controller->startImaging();
   if (!result) {
       // Handle error
       LOG_ERROR(result.error().message);
       return result.error().code;
   }

Error Propagation
~~~~~~~~~~~~~~~~~

.. code-block:: text

   Hardware Driver → HAL → Application → MM Adapter
   (throws)          (Result) (Result)   (MM error code)

Thread Safety
-------------

.. list-table::
   :header-rows: 1

   * - Component
     - Thread
     - Responsibility
   * - Control Thread
     - Main
     - Configuration, property access
   * - Acquisition Thread
     - Worker
     - Data acquisition, buffering
   * - Processing Thread
     - Worker
     - Image reconstruction

Synchronization Points
~~~~~~~~~~~~~~~~~~~~~~

- **Configuration**: Mutex-protected property access
- **Data Transfer**: Thread-safe queue between acquisition and processing
- **Callbacks**: Atomic flags for state changes

Extension Points
----------------

Adding New Hardware
~~~~~~~~~~~~~~~~~~~

1. Implement the appropriate interface:

   .. code-block:: cpp

      class MyNewScanner : public IScanner {
          // Implement all pure virtual methods
      };

2. Register in factory:

   .. code-block:: cpp

      std::unique_ptr<IScanner> HardwareFactory::createScanner(const std::string& type) {
          if (type == "my_scanner") {
              return std::make_unique<MyNewScanner>();
          }
          // ...
      }

3. Add to configuration:

   .. code-block:: json

      {
        "scanner": {
          "type": "my_scanner",
          ...
        }
      }

Adding New Processing Stages
~~~~~~~~~~~~~~~~~~~~~~~~~~~~

1. Implement :cpp:class:`IProcessingStage`:

   .. code-block:: cpp

      class MyStage : public IProcessingStage {
      public:
          void process(void* input, void* output, size_t size) override;
          std::string name() const override { return "MyStage"; }
      };

2. Add to pipeline:

   .. code-block:: cpp

      pipeline->addStage(std::make_unique<MyStage>());
